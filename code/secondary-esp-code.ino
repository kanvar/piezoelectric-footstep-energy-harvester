import network
import espnow
from machine import Pin, I2C, PWM
from ssd1306 import SSD1306_I2C
from time import sleep
import time



# ---------------- oled setup ----------------
SDA_PIN = 15
SCL_PIN = 32
OLED_WIDTH = 128
OLED_HEIGHT = 64
OLED_ADDR = 0x3D

i2c_oled = I2C(0, scl=Pin(SCL_PIN), sda=Pin(SDA_PIN), freq=400000)
print("I2C scan:", i2c_oled.scan())
oled = SSD1306_I2C(OLED_WIDTH, OLED_HEIGHT, i2c_oled, addr=OLED_ADDR)



# ---------------- speaker setup ----------------
SPEAKER_PIN = 4
speaker_pwm = PWM(Pin(SPEAKER_PIN))
speaker_pwm.duty(0)

def alarm_beep():
    """Short non-blocking-ish beep pattern (~0.2s total)."""
    speaker_pwm.freq(900)
    speaker_pwm.duty(512)
    sleep(0.08)
    speaker_pwm.duty(0)
    sleep(0.04)
    speaker_pwm.freq(600)
    speaker_pwm.duty(512)
    sleep(0.08)
    speaker_pwm.duty(0)



# ---------------- receiver setup ----------------
sta = network.WLAN(network.STA_IF)
sta.active(True)
sta.disconnect()
sta.config(channel=1)

print("Receiver MAC Address:", sta.config('mac').hex())

e = espnow.ESPNow()
e.active(True)



# ---------------- connecting screen ----------------
oled.fill(0)
oled.text("Receiver Ready", 10, 20)
oled.text("Waiting for data", 0, 40)
oled.show()



# ---------------- input variables ----------------
temperature = None
humidity = None

v1_max = 0.0
v2_max = 0.0
v3_max = 0.0
v4_max = 0.0

fire_detected = 1



# alarm timing
ALARM_PERIOD_MS = 1000
last_alarm_ms = time.ticks_ms()



# updating oled
def redraw():
    """Update OLED with latest values (same layout as before)."""
    voltage_piezo = v1_max + v3_max
    voltage_solar = v2_max + v4_max

    oled.fill(0)

    if temperature is not None and humidity is not None:
        oled.text("{:.1f}C  {:.1f}%".format(temperature, humidity), 0, 0)
    else:
        oled.text("--.-C  --.-%", 0, 0)

    oled.text("v_p: {:.2f} V".format(voltage_piezo), 0, 28)
    oled.text("v_s: {:.2f} V".format(voltage_solar), 0, 52)



    # oled fire indicator
    if fire_detected == 0:
        oled.text("FIRE!", 80, 0)

    oled.show()



# ---------------- main loop ----------------
while True:
    try:
        host, msg = e.recv()
        if not msg:
            continue

        msg = msg.decode("utf-8")
        print("Received:", msg)



        # -------- 1s data read --------
        if msg.startswith("LIVE:"):
            payload = msg[5:]
            parts = payload.split(',')

            for part in parts:
                if ':' not in part:
                    continue
                key, val = part.split(':', 1)

                if key == "T" and val != "--":
                    temperature = float(val)
                elif key == "H" and val != "--":
                    humidity = float(val)
                elif key == "F":
                    fire_detected = int(val)

            redraw()



        # -------- 5s data read --------
        elif msg.startswith("PEAK:"):
            payload = msg[5:]
            parts = payload.split(',')

            for part in parts:
                if ':' not in part:
                    continue
                key, val = part.split(':', 1)

                if key == "V1max":
                    v1_max = float(val)
                elif key == "V2max":
                    v2_max = float(val)
                elif key == "V3max":
                    v3_max = float(val)
                elif key == "V4max":
                    v4_max = float(val)

            redraw()



        # -------- alarm --------
        now = time.ticks_ms()
        if fire_detected == 0 and time.ticks_diff(now, last_alarm_ms) >= ALARM_PERIOD_MS:
            alarm_beep()
            last_alarm_ms = now

    except Exception as err:
        print("Receiver error:", err)
        oled.fill(0)
        oled.text("RX Error", 0, 20)
        oled.show()
        sleep(1)