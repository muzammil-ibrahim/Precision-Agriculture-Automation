import serial
import time
import threading
# from gpiozero import DistanceSensor
import websocket
import json
# import rel

switch_vp = '01 A0 00 00 00 00 00 00 00 02'
br = '01 64 00 00 00 00 00 FF 00 D1'
rpm_0 = '01 64 00 00 00 00 00 00 00 50'
rpm_50p = '01 64 00 32 00 00 00 00 00 D3'
rpm_50m = '01 64 FF CE 00 00 00 00 00 DA'
rpm_100p = '01 64 00 64 00 00 00 00 00 4F'
rpm_100m = '01 64 FF 9C 00 00 00 00 00 9A'
x = "01 64 75 30 00 00 00 00 00 A7 "
#serial_ports = ['COM10', 'COM11']
serial_ports = ['COM12']
baud_rate = 115200
ser_dict = {port: serial.Serial(port, baudrate=baud_rate, timeout=1) for port in serial_ports}

def send_hex_command(ser, hex_command):
    try:

        command_bytes = bytes.fromhex(hex_command)
        ser.write(command_bytes)
        time.sleep(1)

    except serial.SerialException as e: 
        print(f"Error: {e}")
    return

def run_motor(ser, duration, hex_command):
    send_hex_command(ser, hex_command)
    time.sleep(duration)
    send_hex_command(ser, br)
    send_hex_command(ser, br)
    return

def brake(ser,hex_command):
    send_hex_command(ser, hex_command)
    send_hex_command(ser, br)
    return

# def on_message(ws, message):
#     try:
#         x = json.loads(message)
#         if x["direction"].upper()=="EMERGENCYSTOP":
#             thread1 = threading.Thread(target=brake, args=(ser_dict['/dev/ttyUSB0'],br))
#             thread2 = threading.Thread(target=brake, args=(ser_dict['/dev/ttyUSB1'],br))    

#             thread1.start()
#             thread2.start()
#             ack_message = {"CAR": "Reached car"+x['direction']}
#             ack_message = json.dumps(ack_message)
#             ws.send(ack_message)
#             return "The Car has been stopped"
#         elif x["direction"].upper()=="STATUS":
#             return "The Car is online"
#         elif x["direction"].upper()=="FORWARD":
#             print("Forward")
#             thread1 = threading.Thread(target=run_motor, args=(ser_dict['/dev/ttyUSB0'], 5, rpm_100m))
#             thread2 = threading.Thread(target=run_motor, args=(ser_dict['/dev/ttyUSB1'], 5, rpm_100p))
            

#             thread1.start()
#             thread2.start()
#             ultrasonic = DistanceSensor(echo = 17, trigger = 4,max_distance = 4,threshold_distance=0.8)
#             start_time = time.time()
#             end_time = start_time+5
#             ack_message = {"CAR": "Reached car"+x['direction']}
#             ack_message = json.dumps(ack_message)
#             ws.send(ack_message)

#             while time.time()< end_time: 
#                 #a=ultrasonic.distance +1-1
            
#                 if ultrasonic.distance<0.8:
                    

#                     print("ultrasoniworkingforward")
#                     thread1 = threading.Thread(target=brake, args=(ser_dict['/dev/ttyUSB0'],br))
#                     thread2 = threading.Thread(target=brake, args=(ser_dict['/dev/ttyUSB1'],br))
#                     thread1.start()
#                     thread2.start()
#                     ultrasonic.close()

#                     return "Obstacle detected in vicinity"
            
#             return "Car is moving forward"
#         elif x["direction"].upper()=="BACKWARD":
#             print("Backward")
#             thread1 = threading.Thread(target=run_motor, args=(ser_dict['/dev/ttyUSB0'], 5, rpm_50p))
#             thread2 = threading.Thread(target=run_motor, args=(ser_dict['/dev/ttyUSB1'], 5, rpm_50m))

#             thread1.start()
#             thread2.start()
#             ack_message = {"CAR": "Reached car"+x['direction']}
#             ack_message = json.dumps(ack_message)
#             ws.send(ack_message)
                
#             return "Car is moving Backward"
#         elif x["direction"].upper()=="LEFT":
#             print("Left")
#             thread1 = threading.Thread(target=run_motor, args=(ser_dict['/dev/ttyUSB0'], 3, rpm_100m))
#             thread2 = threading.Thread(target=run_motor, args=(ser_dict['/dev/ttyUSB1'], 3, rpm_100m))
#             thread1.start() 
#             thread2.start()
#             ack_message = {"CAR": "Reached car"+x['direction']}
#             ack_message = json.dumps(ack_message)
#             ws.send(ack_message)

#             return "Car is turning left"
#         elif x["direction"].upper()=="RIGHT":
#             print("Right")
#             thread1 = threading.Thread(target=run_motor, args=(ser_dict['/dev/ttyUSB0'], 3, rpm_100p))
#             thread2 = threading.Thread(target=run_motor, args=(ser_dict['/dev/ttyUSB1'], 3, rpm_100p))

#             thread1.start()
#             thread2.start()
#             ack_message = {"CAR": "Reached car"+x['direction']}
#             ack_message = json.dumps(ack_message)
#             ws.send(ack_message)


#             return "car is turning right"
#         else:
#             print("Not Working")
#             return "PASS"
#     except json.JSONDecodeError:
#         print("Decode Error")
#         pass
def on_message(message):
    try:
        # x = json.loads(message)
        if message=="EMERGENCYSTOP":
            thread1 = threading.Thread(target=brake, args=(ser_dict['COM12'],br))
            # thread2 = threading.Thread(target=brake, args=(ser_dict['COM11'],br))    

            thread1.start()
            # thread2.start()
            # ack_message = {"CAR": "Reached car"+x['direction']}
            # ack_message = json.dumps(ack_message)
            # ws.send(ack_message)
            return "The Car has been stopped"
        elif message=="STATUS":
            return "The Car is online"
        elif message=="FORWARD":
            print("Forward")
            thread1 = threading.Thread(target=run_motor, args=(ser_dict['COM12'], 2, rpm_50m))
            # thread2 = threading.Thread(target=run_motor, args=(ser_dict['COM11'], 2, rpm_100p))
            

            thread1.start()
            # thread2.start()
            # ultrasonic = DistanceSensor(echo = 17, trigger = 4,max_distance = 4,threshold_distance=0.8)
            # start_time = time.time()
            # end_time = start_time+5
            # ack_message = {"CAR": "Reached car"+x['direction']}
            # ack_message = json.dumps(ack_message)
            # ws.send(ack_message)

            # while time.time()< end_time: 
            #     #a=ultrasonic.distance +1-1
            
            #     if ultrasonic.distance<0.8:
                    

            #         print("ultrasoniworkingforward")
            #         thread1 = threading.Thread(target=brake, args=(ser_dict['/dev/ttyUSB0'],br))
            #         thread2 = threading.Thread(target=brake, args=(ser_dict['/dev/ttyUSB1'],br))
            #         thread1.start()
            #         thread2.start()
            #         ultrasonic.close()

            #         return "Obstacle detected in vicinity"
            
            return "Car is moving forward"
        elif message=="BACKWARD":
            print("Backward")
            thread1 = threading.Thread(target=run_motor, args=(ser_dict['COM12'], 5, rpm_50p))
            # thread2 = threading.Thread(target=run_motor, args=(ser_dict['COM11'], 5, rpm_50m))

            thread1.start()
            # thread2.start()
            # ack_message = {"CAR": "Reached car"+x['direction']}
            # ack_message = json.dumps(ack_message)
            # ws.send(ack_message)
                
            return "Car is moving Backward"
        elif message=="LEFT":
            print("Left")
            thread1 = threading.Thread(target=run_motor, args=(ser_dict['COM12'], 3, rpm_100m))
            #thread2 = threading.Thread(target=run_motor, args=(ser_dict['COM11'], 3, rpm_100m))
            thread1.start() 
            #thread2.start()
            # ack_message = {"CAR": "Reached car"+x['direction']}
            # ack_message = json.dumps(ack_message)
            # ws.send(ack_message)

            return "Car is turning left"
        elif message=="RIGHT":
            print("Right")
            thread1 = threading.Thread(target=run_motor, args=(ser_dict['COM12'], 3, rpm_100p))
            #thread2 = threading.Thread(target=run_motor, args=(ser_dict['COM11'], 3, rpm_100p))

            thread1.start()
            #thread2.start()
            # ack_message = {"CAR": "Reached car"+x['direction']}
            # ack_message = json.dumps(ack_message)
            # ws.send(ack_message)


            return "car is turning right"
        else:
            print("Not Working")
            return "PASS"
    except Exception:
        print("Decode Error")
        pass


# def on_error(ws, error):
#     print(error)

# def on_close(ws, close_status_code, close_msg):
#     print("### closed ###")

# def on_open(ws):
#     print("Opened connection")

if __name__ == "__main__":
    # websocket.enableTrace(True)
    # ws = websocket.WebSocketApp("172.168.2.90:8081",
    #                           on_open=on_open,
    #                           on_message=on_message,
    #                           on_error=on_error,    
    #                           on_close=on_close)

    # ws.run_forever(dispatcher=rel, reconnect=5)  # Set dispatcher to automatic reconnection, 
    # #5 second reconnect delay if connection closed unexpectedly
    # rel.signal(2, rel.abort)  # Keyboard Interrupt
    # rel.dispatch()
    on_message("FORWARD")
    # time.sleep(5)
    # on_message("EMERGENCYSTOP")
    # on_message("BACKWARD")
