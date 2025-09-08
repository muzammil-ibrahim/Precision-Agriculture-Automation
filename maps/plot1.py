from pymavlink import mavutil
import time

# ---------------------------
# Connect to Pixhawk
# ---------------------------
print("[INFO] Connecting to Pixhawk...")

# Replace with your actual port (Linux: /dev/ttyUSB0, Windows: COM3, etc.)
master = mavutil.mavlink_connection('COM4', baud=57600)

# Wait for heartbeat (Pixhawk → GCS)
print("[INFO] Waiting for heartbeat...")
master.wait_heartbeat()
print(f"[OK] Heartbeat received from system {master.target_system} component {master.target_component}")


# ---------------------------
# Function to send servo PWM
# ---------------------------
def set_servo(channel, pwm):
    print(f"[DEBUG] Sending PWM {pwm} to channel {channel}")
    
    master.mav.command_long_send(
        master.target_system,             # Target system
        master.target_component,          # Target component
        mavutil.mavlink.MAV_CMD_DO_SET_SERVO, # Command ID
        0,                                # Confirmation
        channel,                          # Servo channel (1-16)
        pwm,                              # PWM microseconds (1000-2000 typical)
        0, 0, 0, 0, 0                     # Unused parameters
    )

    # Listen for ACK (optional, not all firmwares respond)
    ack = master.recv_match(type='COMMAND_ACK', blocking=False)
    if ack:
        print(f"[ACK] {ack}")


# ---------------------------
# Example test sequence
# ---------------------------
test_pwms = [1000, 1500, 2000]

for pwm in test_pwms:
    set_servo(10, pwm)    # Channel 1 → Servo
    time.sleep(2)

print("[INFO] Test sequence complete.")
