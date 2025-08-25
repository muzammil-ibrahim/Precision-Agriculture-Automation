from fastapi import FastAPI, Query
from fastapi.middleware.cors import CORSMiddleware
import pandas as pd
import numpy as np
from pyproj import Transformer
from shapely.geometry import Polygon, Point
from pymavlink import mavutil
import paho.mqtt.client as mqtt
import threading
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
geofence_output_path = os.path.join(base_dir, "public", "geofence_converted.csv")
points_output_path = os.path.join(base_dir, "public", "points_converted.csv")

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
    

# MAVLink & MQTT config
MAVLINK_URI = "COM4:57600"  # Or COM3:57600, /dev/ttyUSB0:57600
MQTT_BROKER = "13.232.191.178"
MQTT_PORT = 1883
MQTT_TOPIC = "drone/yaw"

# MQTT client
mqtt_client = mqtt.Client()
mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)

# Shared yaw variable
latest_yaw = 0.0

def mavlink_reader():
    global latest_yaw
    
    master = mavutil.mavlink_connection("COM4", baud=57600)


    print("[mavlink] waiting for heartbeat...")
    master.wait_heartbeat()
    print("[mavlink] heartbeat received")

    while True:
        msg = master.recv_match(type='ATTITUDE', blocking=True)
        if msg:
            yaw_deg = math.degrees(msg.yaw) % 360
            latest_yaw = yaw_deg
            mqtt_client.publish(MQTT_TOPIC, f"{yaw_deg:.2f}")

# Start MAVLink reading in a background thread
# threading.Thread(target=mavlink_reader, daemon=True).start()

# Keep MQTT alive in another thread
def mqtt_loop():
    while True:
        mqtt_client.loop()
        time.sleep(0.1)

# threading.Thread(target=mqtt_loop, daemon=True).start()

# HTTP endpoint just for debugging
# @app.get("/yaw")
# def get_yaw():
#     return {"yaw": latest_yaw}