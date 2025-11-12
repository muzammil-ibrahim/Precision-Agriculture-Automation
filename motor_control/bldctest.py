from pymavlink import mavutil
import time

# adjust port / baud
m = mavutil.mavlink_connection('COM19', baud=115200)
m.wait_heartbeat()

CHAN = 5   # change to the servo channel your ESC is on

time.sleep(2)

# send 1100 us -> low throttle (may start motor)
m.mav.command_long_send(
    m.target_system,
    m.target_component,
    mavutil.mavlink.MAV_CMD_DO_SET_SERVO,
    0, CHAN, 1100, 0,0,0,0,0
)
print("Sent 1100 µs (low throttle). Holding 3s...")
time.sleep(3)

# send 1000 us -> stop
m.mav.command_long_send(
    m.target_system,
    m.target_component,
    mavutil.mavlink.MAV_CMD_DO_SET_SERVO,
    0, CHAN, 1000, 0,0,0,0,0
)
print("Sent 1000 µs (stop).")
