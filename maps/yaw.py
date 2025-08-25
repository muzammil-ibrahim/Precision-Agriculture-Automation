from pymavlink import mavutil
import math

# Connect to the autopilot
master = mavutil.mavlink_connection('COM4', baud=57600)  # or 'udp:127.0.0.1:14550'
master.wait_heartbeat()
print("Connected to autopilot")

while True:
    msg = master.recv_match(type='ATTITUDE', blocking=True)
    if msg:
        yaw_deg = math.degrees(msg.yaw)
        print(f"Yaw: {yaw_deg:.2f}°")
