import paho.mqtt.client as mqtt

def on_message(client, userdata, msg):
    print(f"{msg.topic} {msg.payload.decode()}")

client = mqtt.Client()
client.connect("13.232.191.178", 1883, 60)
client.subscribe("drone/yaw")

client.on_message = on_message
client.loop_forever()
