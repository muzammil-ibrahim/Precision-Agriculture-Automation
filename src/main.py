from fastapi import FastAPI, Query
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import FileResponse
from fastapi.responses import JSONResponse
import pandas as pd
import numpy as np
from pyproj import Transformer
from shapely.geometry import Polygon, Point
from pymavlink import mavutil
from datetime import datetime
import paho.mqtt.client as mqtt
import threading
import csv
import time
import math
import os

app = FastAPI()

# Enable CORS for React
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # Change to ["http://localhost:3000"] for security
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Helper: find UTM zone
def utm_zone_from_lon(lon):
    return int((lon + 180) / 6) + 1

# File paths
base_dir = r"C:\Users\Muzammil\OneDrive\Desktop\my-app"
geofence_input_path = os.path.join(base_dir, "src", "geofence.csv")
geofence_output_path = os.path.join(base_dir, "dist", "geofence_converted.csv")
points_output_path = os.path.join(base_dir, "dist", "points_converted.csv")

@app.get("/generate")
def generate_points(
    col_spacing_ft: float = Query(4),
    row_spacing_ft: float = Query(10),
    border_margin_ft: float = Query(4)
):
    try:
        # Convert feet → meters
        ft_to_m = 0.3048
        col_spacing_m = col_spacing_ft * ft_to_m
        row_spacing_m = row_spacing_ft * ft_to_m
        border_margin_m = border_margin_ft * ft_to_m

        # Load geofence CSV
        df_geofence = pd.read_csv(geofence_input_path)

        # Detect UTM zone
        mean_lon = df_geofence["longitude"].mean()
        zone = utm_zone_from_lon(mean_lon)
        epsg_code = 32600 + zone if df_geofence["latitude"].mean() >= 0 else 32700 + zone
        transformer = Transformer.from_crs("EPSG:4326", f"EPSG:{epsg_code}", always_xy=True)

        # Reference point
        ref_lat = df_geofence["latitude"].min()
        ref_lon = df_geofence["longitude"].min()
        ref_x, ref_y = transformer.transform(ref_lon, ref_lat)

        # Convert geofence to UTM
        df_geofence["x_m"], df_geofence["y_m"] = transformer.transform(
            df_geofence["longitude"].values,
            df_geofence["latitude"].values
        )
        df_geofence["x"] = (df_geofence["x_m"] - ref_x) * 100
        df_geofence["y"] = (df_geofence["y_m"] - ref_y) * 100
        df_geofence[["x", "y"]].to_csv(geofence_output_path, index=False)

        # Create polygon & shrink
        poly = Polygon(zip(df_geofence["x_m"], df_geofence["y_m"]))
        poly_shrunk = poly.buffer(-border_margin_m)

        # Generate points
        min_x, min_y, max_x, max_y = poly_shrunk.bounds
        points_x = np.arange(min_x, max_x, col_spacing_m)
        points_y = np.arange(min_y, max_y, row_spacing_m)

        grid_points = []
        for y in points_y:
            for x in points_x:
                if poly_shrunk.contains(Point(x, y)):
                    grid_points.append(((x - ref_x) * 100, (y - ref_y) * 100))

        # Save points CSV
        df_points = pd.DataFrame(grid_points, columns=["x", "y"])
        df_points.to_csv(points_output_path, index=False)

        return {
            "status": "success",
            "total_points": len(df_points),
            "points": df_points.to_dict(orient="records"),
            "params": {
                "col_spacing_ft": col_spacing_ft,
                "row_spacing_ft": row_spacing_ft,
                "border_margin_ft": border_margin_ft
            }
        }

    except Exception as e:
        return {"status": "error", "message": str(e)}

@app.get("/clear-csv")
def clear_csv():
    try:
        # Clear points CSV
        pd.DataFrame(columns=["x", "y"]).to_csv(points_output_path, index=False)
        # Optionally clear geofence CSV too
        pd.DataFrame(columns=["x", "y"]).to_csv(geofence_output_path, index=False)

        return {"status": "success", "message": "CSV files cleared"}
    except Exception as e:
        return {"status": "error", "message": str(e)}
    
@app.get("/get_csv")
async def get_csv():
    file_path = r"C:\Users\Muzammil\OneDrive\Desktop\my-app\dist\geofence_converted.csv"
    return FileResponse(
        path=file_path,
        media_type="text/csv",
        filename="geofence_converted.csv"
    )

@app.get("/get_csv1")
async def get_csv1():
    file_path = r"C:\Users\Muzammil\OneDrive\Desktop\my-app\dist\points_converted.csv"
    return FileResponse(
        path=file_path,
        media_type="text/csv",
        filename="points_converted.csv"
    )

@app.get("/get_csv2")
async def get_csv1():
    file_path = r"C:\Users\Muzammil\OneDrive\Desktop\my-app\src\geofence.csv"
    return FileResponse(
        path=file_path,
        media_type="text/csv",
        filename="geofence.csv"
    )

# Config
UDP_PORT = "udp:127.0.0.1:14550"  # your rover’s UDP connection string
CSV_FILE = "geofence_survey_data.csv"
DIST_THRESHOLD = 1.0  # meters

# Globals
logging_active = False
logging_thread = None


# Haversine distance
def haversine(lat1, lon1, lat2, lon2):
    R = 6371000
    phi1, phi2 = math.radians(lat1), math.radians(lat2)
    dphi = math.radians(lat2 - lat1)
    dlambda = math.radians(lon2 - lon1)
    a = math.sin(dphi / 2) ** 2 + math.cos(phi1) * math.cos(phi2) * math.sin(dlambda / 2) ** 2
    return 2 * R * math.atan2(math.sqrt(a), math.sqrt(1 - a))


# Background logging function
def mavlink_logger():
    global logging_active

    # ✅ Reset CSV at the start of each session
    with open(CSV_FILE, mode="w", newline="") as file:
        writer = csv.writer(file)
        writer.writerow(["latitude", "longitude", "label", "timestamp", "fix_quality", "h_accuracy"])

    # Connect to MAVLink
    print(f"🔗 Connecting to MAVLink on {UDP_PORT}...")
    master = mavutil.mavlink_connection(UDP_PORT)
    master.wait_heartbeat()
    print("✅ Heartbeat received")

    last_lat, last_lon = None, None
    point_count = 0

    while logging_active:
        msg = master.recv_match(type="GPS_RAW_INT", blocking=True, timeout=5)
        if not msg:
            continue

        lat = msg.lat / 1e7
        lon = msg.lon / 1e7
        h_acc = msg.h_acc / 1000.0 if msg.h_acc != 0xFFFF else None
        fix_type = msg.fix_type
        timestamp = datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%SZ")

        fix_quality_map = {
            0: "No Fix", 1: "Dead Reckoning", 2: "2D-Fix", 3: "3D-Fix",
            4: "RTK-Float", 5: "RTK-Fixed"
        }
        fix_quality = fix_quality_map.get(fix_type, "Unknown")

        if last_lat is None or haversine(last_lat, last_lon, lat, lon) >= DIST_THRESHOLD:
            point_count += 1
            label = f"P{point_count}"
            with open(CSV_FILE, mode="a", newline="") as file:
                writer = csv.writer(file)
                writer.writerow([lat, lon, label, timestamp, fix_quality, h_acc])
            print(f"✅ Logged {label}: {lat}, {lon}, fix={fix_quality}, acc={h_acc}m")
            last_lat, last_lon = lat, lon

        time.sleep(0.5)

    print("🛑 Logging stopped.")


# API endpoints
@app.post("/start")
def start_logging():
    global logging_active, logging_thread
    if logging_active:
        return JSONResponse({"status": "already_running"})

    logging_active = True
    logging_thread = threading.Thread(target=mavlink_logger, daemon=True)
    logging_thread.start()
    return {"status": "started"}


@app.post("/stop")
def stop_logging():
    global logging_active
    logging_active = False
    return {"status": "stopped"}

