from pymavlink import mavutil
import csv
import math
import time
from datetime import datetime

# Config
UDP_PORT = "udp:127.0.0.1:14550"  # change to your rover’s UDP connection string
CSV_FILE = "geofence_survey_data.csv"
DIST_THRESHOLD = 1.0  # meters

# Initialize CSV
with open(CSV_FILE, mode='w', newline='') as file:
    writer = csv.writer(file)
    writer.writerow(["latitude", "longitude", "label", "timestamp", "fix_quality", "h_accuracy"])

# Haversine distance
def haversine(lat1, lon1, lat2, lon2):
    R = 6371000
    phi1, phi2 = math.radians(lat1), math.radians(lat2)
    dphi = math.radians(lat2 - lat1)
    dlambda = math.radians(lon2 - lon1)
    a = math.sin(dphi/2)**2 + math.cos(phi1)*math.cos(phi2)*math.sin(dlambda/2)**2
    return 2 * R * math.atan2(math.sqrt(a), math.sqrt(1-a))

# Connect to MAVLink
print(f"🔗 Connecting to MAVLink on {UDP_PORT}...")
master = mavutil.mavlink_connection(UDP_PORT)
master.wait_heartbeat()
print("✅ Heartbeat received from system", master.target_system, "component", master.target_component)

last_lat, last_lon = None, None
point_count = 0
logging = False

print("👉 Press ENTER to START logging, type 'stop' to END")

while True:
    user_input = input(">> ").strip().lower()
    if user_input == "":
        logging = True
        print("📍 Started logging geofence points...")
    elif user_input == "stop":
        logging = False
        print("🛑 Stopped logging.")
        break

    while logging:
        msg = master.recv_match(type="GPS_RAW_INT", blocking=True, timeout=5)
        if not msg:
            continue

        # Extract data
        lat = msg.lat / 1e7
        lon = msg.lon / 1e7
        h_acc = msg.h_acc / 1000.0 if msg.h_acc != 0xFFFF else None  # meters
        fix_type = msg.fix_type  # 0=no fix, 2=2D, 3=3D, 4=RTK Float, 5=RTK Fixed
        timestamp = datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%SZ")

        # Map fix_type to string
        fix_quality_map = {
            0: "No Fix", 1: "Dead Reckoning", 2: "2D-Fix", 3: "3D-Fix",
            4: "RTK-Float", 5: "RTK-Fixed"
        }
        fix_quality = fix_quality_map.get(fix_type, "Unknown")

        # Check distance threshold
        if last_lat is None or haversine(last_lat, last_lon, lat, lon) >= DIST_THRESHOLD:
            point_count += 1
            label = f"P{point_count}"

            with open(CSV_FILE, mode='a', newline='') as file:
                writer = csv.writer(file)
                writer.writerow([lat, lon, label, timestamp, fix_quality, h_acc])

            print(f"✅ Logged {label}: {lat}, {lon}, fix={fix_quality}, acc={h_acc}m")
            last_lat, last_lon = lat, lon

        time.sleep(0.5)  # adjust based on GPS update rate
