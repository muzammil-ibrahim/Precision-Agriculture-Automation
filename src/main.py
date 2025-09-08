from fastapi import FastAPI, Query
from fastapi.middleware.cors import CORSMiddleware
from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.responses import HTMLResponse
import pandas as pd
import numpy as np
import random
from io import StringIO
from pyproj import Transformer
from shapely.geometry import Polygon, Point
from pymavlink import mavutil
from fastapi.staticfiles import StaticFiles
from fastapi.responses import FileResponse
import paho.mqtt.client as mqtt
import threading
import time
from datetime import datetime
import math
import csv
import serial
import os
from dronekit import connect, VehicleMode
import asyncio
app = FastAPI()

# base_dir = r"/home/kmitj4/Desktop/Precision-Agriculture-Automation-main"
base_dir = r"C:\Users\Muzammil\OneDrive\Desktop\my-app"


origins = [
    "http://localhost:8080",
    "http://127.0.0.1:8080",
    "http://0.0.0.0:8080"
]


# Enable CORS for React
app.add_middleware(
    CORSMiddleware,
    allow_origins=origins,  # Change to ["http://localhost:3000"] for security
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Helper: find UTM zone
def utm_zone_from_lon(lon):
    return int((lon + 180) / 6) + 1

# File paths

geofence_input_path = os.path.join(base_dir, "src", "geofence.csv")
geofence_output_path = os.path.join(base_dir, "dist", "geofence_converted.csv")
points_output_path = os.path.join(base_dir, "dist", "points_converted.csv")
points_latlon = os.path.join(base_dir, "dist", "points_latlon.csv")


@app.get("/generate")
async def generate_points(
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
async def clear_csv():
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
    # file_path = "/home/kmitj4/Desktop/Precision-Agriculture-Automation-main/dist/geofence_converted.csv"
    file_path = "C:/Users/Muzammil/OneDrive/Desktop/my-app/dist/geofence_converted.csv"
    headers = {"Cache-Control": "no-cache, no-store,must-revalidate"}
    return FileResponse(
        path=file_path,
        media_type="text/csv",
        filename="geofence_converted.csv",
        headers=headers
    )
@app.get("/get_csv1")
async def get_csv1():
    # file_path = "/home/kmitj4/Desktop/Precision-Agriculture-Automation-main/dist/points_converted.csv"
    file_path = "C:/Users/Muzammil/OneDrive/Desktop/my-app/dist/points_converted.csv"
    headers = {"Cache-Control": "no-cache, no-store,must-revalidate"}
    return FileResponse(
        path=file_path,
        media_type="text/csv",
        filename="points_converted.csv",
        headers=headers
    )
@app.get("/get_csv2")
async def get_csv2():
    # file_path = "/home/kmitj4/Desktop/Precision-Agriculture-Automation-main/src/geofence.csv"
    file_path = "C:/Users/Muzammil/OneDrive/Desktop/my-app/src/geofence.csv"
    headers = {"Cache-Control": "no-cache, no-store,must-revalidate"}
    return FileResponse(
        path=file_path,
        media_type="text/csv",
        filename="geofence.csv",
        headers=headers
    )
    '''try:
        with open(file_path, "r") as f:
            content = f.read()
        df = pd.read_csv(StringIO(content), sep=';')
        return df.to_string()
    except Exception as e:
        return str(e)'''


UDP_PORT = "/dev/ttyACM0"  # your rover’s UDP connection string
# UDP_PORT = "udp:127.0.0.1:14550"
DIST_THRESHOLD = 1.0  # meters

logging_active = False
logging_thread = None

# API endpoints for logging
@app.post("/start")
async def start_logging():
    global logging_active, logging_thread
    if logging_active:
        return JSONResponse({"status": "already_running"})

    logging_active = True
    logging_thread = threading.Thread(target=mavlink_logger, daemon=True)
    logging_thread.start()
    return {"status": "started"}

@app.post("/stop")
async def stop_logging():
    global logging_active
    logging_active = False
    return {"status": "stopped"}

# Store connected clients
connected_clients = []

# CSV_PATH = geofence_output_path  # path to your CSV file


# @app.websocket("/ws/location")
# async def websocket_csv(websocket: WebSocket):
#     await websocket.accept()
#     try:
#         df = pd.read_csv(CSV_PATH)

#         # Ensure CSV has x, y columns
#         if not {"x", "y"}.issubset(df.columns):
#             await websocket.send_json({"error": "CSV must contain x,y columns"})
#             await websocket.close()
#             return

#         # Stream row by row
#         for _, row in df.iterrows():
#             data = {"x": float(row["x"]), "y": float(row["y"])}
#             await websocket.send_json(data)
#             await asyncio.sleep(1)  # delay between messages

#         # Done sending
#         await websocket.close()

#     except Exception as e:
#         await websocket.send_json({"error": str(e)})
#         await websocket.close()


@app.websocket("/ws/location")
async def websocket_endpoint(websocket: WebSocket):
    await websocket.accept()
    connected_clients.append(websocket)

    try:
        # --- Connect via MAVLink ---
        # master = mavutil.mavlink_connection(UDP_PORT)``
        # master.wait_heartbeat()
        # print("✅ Connected to autopilot via MAVLink")

        # --- Load geofence & setup transformer ---
        df_geofence = pd.read_csv(geofence_input_path)
        mean_lon = df_geofence["longitude"].mean()
        zone = utm_zone_from_lon(mean_lon)
        epsg_code = 32600 + zone if df_geofence["latitude"].mean() >= 0 else 32700 + zone
        transformer = Transformer.from_crs("EPSG:4326", f"EPSG:{epsg_code}", always_xy=True)

        ref_lat = df_geofence["latitude"].min()
        ref_lon = df_geofence["longitude"].min()
        ref_x, ref_y = transformer.transform(ref_lon, ref_lat)

        # --- Loop for MAVLink position ---
        while True:
            # Wait for position message (non-blocking with timeout)
            msg = master.recv_match(type="GLOBAL_POSITION_INT", blocking=True, timeout=5)
            if not msg:
                continue  # skip if nothing received

            # Convert raw MAVLink (1e7 scaled integers) to degrees
            lat = msg.lat / 1e7
            lon = msg.lon / 1e7
            # alt = msg.relative_alt / 1000.0  # mm → meters

            # Transform to field-relative XY
            x_m, y_m = transformer.transform(lon, lat)
            x_ref = (x_m - ref_x) * 100
            y_ref = (y_m - ref_y) * 100

            data = {"x": x_ref, "y": y_ref}

            # Send to this client
            await websocket.send_json(data)

            # Optionally broadcast to others
            for client in connected_clients:
                if client != websocket:
                    await client.send_json(data)

            await asyncio.sleep(1)

    except WebSocketDisconnect:
        connected_clients.remove(websocket)
        print("❌ Client disconnected")

    except Exception as e:
        print(f"⚠️ Error in websocket loop: {e}")
        if websocket in connected_clients:
            connected_clients.remove(websocket)


@app.websocket("/ws/yaw")
async def websocket_yaw(websocket: WebSocket):
    await websocket.accept()
    try:
        while True:
            yaw = get_yaw()
            if yaw is not None:
                await websocket.send_json({"yaw": yaw})
            await asyncio.sleep(0.1)  # adjust frequency as needed
    except WebSocketDisconnect:
        print("Client disconnected")

# MISSION endpoints

master = mavutil.mavlink_connection('COM4', baud=57600)

@app.on_event("startup")
async def startup_event():
    master.wait_heartbeat()
    print("Connected to vehicle")
    # convert_points_to_latlon(geofence_input_path,points_output_path,points_latlon)

    
    

@app.get("/start_mission")
async def start_mission():
    convert_points_to_latlon(geofence_input_path,points_output_path,points_latlon)
    waypoints = load_waypoints(points_latlon)
    triggered = [False] * len(waypoints)
    print(f"Loaded {len(waypoints)} waypoints.")

    while not all(triggered):
        msg = master.recv_match(type='GLOBAL_POSITION_INT', blocking=True)
        lat = msg.lat / 1e7
        lon = msg.lon / 1e7

        for i, (wlat, wlon) in enumerate(waypoints):
            if not triggered[i]:
                distance = haversine(lat, lon, wlat, wlon)
                if distance < 0.5:  # within 0.5 meters
                    print(f"Reached waypoint {i+1} (distance: {distance:.2f} m)")
                    send_command(master, 10, 2000)  # Trigger servo
                    time.sleep(1)
                    send_command(master, 10, 1000)  # Reset servo
                    triggered[i] = True

        if all(triggered):
            print("All waypoints completed.")
            break
        time.sleep(1)

    return {"status": "Mission completed"}


app.mount("/", StaticFiles(directory=base_dir+"/dist", html=True), name="static")




def get_yaw():
    msg = master.recv_match(type='ATTITUDE', blocking=True, timeout=5)
    if msg:
        yaw_rad = msg.yaw  # radians
        yaw_deg = math.degrees(yaw_rad)
        return yaw_deg
    return None



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

    # Reset CSV at the start of each session
    with open(geofence_input_path, mode="w", newline="") as file:
        writer = csv.writer(file)
        writer.writerow(["latitude", "longitude", "label", "timestamp", "fix_quality", "h_accuracy"])

  
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
            with open(geofence_input_path, mode="a", newline="") as file:
                writer = csv.writer(file)
                writer.writerow([lat, lon, label, timestamp, fix_type, h_acc])
            print(f"Logged {label}: {lat}, {lon}, fix={fix_quality}, acc={h_acc}m")
            last_lat, last_lon = lat, lon

        time.sleep(0.5)

    print("Logging stopped.")


# Send a command to the vehicle (e.g., set servo PWM)
def send_command(master, channel, pwm):
    master.mav.command_long_send(
        master.target_system,
        master.target_component,
        mavutil.mavlink.MAV_CMD_DO_SET_SERVO,
        0,
        channel, pwm,
        0, 0, 0, 0, 0
    )

# Load waypoints from a CSV file
def load_waypoints(csv_path):
    waypoints = []
    with open(csv_path, newline='') as csvfile:
        reader = csv.DictReader(csvfile)
        for row in reader:
            waypoints.append((float(row['latitude']), float(row['longitude'])))
    return waypoints


def convert_points_to_latlon(
    geofence_input_path: str,
    points_input_path: str,
    output_path: str
):
    try:
        # Load geofence CSV to recover reference and transformer
        df_geofence = pd.read_csv(geofence_input_path)

        # Detect UTM zone (same logic as generate_points)
        mean_lon = df_geofence["longitude"].mean()
        zone = int((mean_lon + 180) / 6) + 1  # inline utm_zone_from_lon
        epsg_code = 32600 + zone if df_geofence["latitude"].mean() >= 0 else 32700 + zone
        transformer_to_utm = Transformer.from_crs("EPSG:4326", f"EPSG:{epsg_code}", always_xy=True)
        transformer_to_latlon = Transformer.from_crs(f"EPSG:{epsg_code}", "EPSG:4326", always_xy=True)

        # Reference point (same as before)
        ref_lat = df_geofence["latitude"].min()
        ref_lon = df_geofence["longitude"].min()
        ref_x, ref_y = transformer_to_utm.transform(ref_lon, ref_lat)

        # Load points CSV (converted coordinates)
        df_points = pd.read_csv(points_input_path)

        # Convert back: x,y → UTM → lat/lon
        x_m = df_points["x"].values / 100 + ref_x
        y_m = df_points["y"].values / 100 + ref_y
        lon, lat = transformer_to_latlon.transform(x_m, y_m)

        # Combine results
        df_result = pd.DataFrame({
            "latitude": lat,
            "longitude": lon
        })

        # Save result
        df_result.to_csv(output_path, index=False)

        return df_result

    except Exception as e:
        raise RuntimeError(f"Conversion failed: {e}")