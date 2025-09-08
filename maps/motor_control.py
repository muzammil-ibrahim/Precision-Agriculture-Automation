# motor_control.py
from ddsm115 import ddsm115

class MotorWrapper:
    def __init__(self, motor_id=1):
        self.motor = ddsm115.MotorControl()
        self.motor.set_drive_mode(_id=motor_id, _mode=3)  # mode 3 = position mode
        self.motor_id = motor_id
        self.current_angle = 0

    def move(self, delta):
        """Move motor by delta degrees"""
        self.current_angle += delta
        self.motor.send_degree(_id=self.motor_id, deg=self.current_angle)
        return self.current_angle
