# main.py
from fastapi import FastAPI
from fastapi.responses import HTMLResponse
from motor_control import MotorWrapper

app = FastAPI()
motor = MotorWrapper(motor_id=1)

@app.get("/", response_class=HTMLResponse)
def home():
    return """
    <!DOCTYPE html>
    <html>
    <head>
        <title>DDSM115 Control</title>
    </head>
    <body style="text-align:center; font-family:Arial; margin-top:50px;">
        <h1>DDSM115 Motor Control</h1>
        <button onclick="move('forward')" style="font-size:20px; padding:10px 20px;">Forward</button>
        <button onclick="move('backward')" style="font-size:20px; padding:10px 20px; margin-left:20px;">Backward</button>
        
        <h2 id="status">Motor angle: 0°</h2>

        <script>
            async function move(direction) {
                let res = await fetch('/' + direction, {method: 'POST'});
                let data = await res.json();
                document.getElementById('status').innerText = "Motor angle: " + data.angle + "°";
            }
        </script>
    </body>
    </html>
    """

@app.post("/forward")
def forward():
    angle = motor.move(+30)  # rotate +30°
    return {"angle": angle}

@app.post("/backward")
def backward():
    angle = motor.move(-30)  # rotate -30°
    return {"angle": angle}
