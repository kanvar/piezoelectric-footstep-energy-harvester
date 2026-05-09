import network
import espnow
from machine import Pin, I2C, SoftI2C
from ssd1306 import SSD1306_I2C
from ina219 import INA219
import dht
from time import sleep
import time



# --------------- advertisemet setup ---------------------------
ads = [
    "VOTE ANWAR",
    "FOR PRESIDENT",
    "",
    "POWER TILE",
    "CHANGES LIVES",
    "",
    "FREE WIFI",
    "TRUST ME BRO",
    "",
    "Enjoy Break!",
    "The Three Phase Amigos",
]
ad_index = 0



# --------------- oled #1 setup ---------------------------
SDA_PIN = 15
SCL_PIN = 32
OLED_WIDTH = 128
OLED_HEIGHT = 64
OLED1_ADDR = 0x3D

i2c_oled = I2C(1, scl=Pin(SCL_PIN), sda=Pin(SDA_PIN), freq=400000)
oled = SSD1306_I2C(OLED_WIDTH, OLED_HEIGHT, i2c_oled, addr=OLED1_ADDR)



# --------------- oled #2 setup ---------------------------
OLED2_ADDR = 0x3D
i2c_oled2 = SoftI2C(scl=Pin(33), sda=Pin(12), freq=400000)
oled2 = SSD1306_I2C(OLED_WIDTH, OLED_HEIGHT, i2c_oled2, addr=OLED2_ADDR)



# --------------- night mode settings ----------------
NIGHT_THRESHOLD_V = 3.0     # if (solar input) < 3V => dim
DAY_CONTRAST = 255
NIGHT_CONTRAST = 20
_last_is_night = None

def set_oled_brightness(v_solar_now):
    """Dim OLED #1 when solar voltage is below threshold."""
    global _last_is_night
    is_night = (v_solar_now < NIGHT_THRESHOLD_V)


    # only update when the state changes
    if _last_is_night is not None and is_night == _last_is_night:
        return
    _last_is_night = is_night

    try:
        oled.contrast(NIGHT_CONTRAST if is_night else DAY_CONTRAST)
        oled2.contrast(NIGHT_CONTRAST if is_night else DAY_CONTRAST)
        
    except AttributeError:
        pass



# --------------- ina setup -----------------------------
# (piezo_motherboard)
i2c_ina1 = I2C(0, scl=Pin(19), sda=Pin(21), freq=100000)
SHUNT_RESISTOR_OHMS = 0.1
ina1 = INA219(SHUNT_RESISTOR_OHMS, i2c_ina1)
ina1.configure()

# (solar_motherboard)
i2c_ina2 = SoftI2C(scl=Pin(20), sda=Pin(22), freq=100000)
ina2 = INA219(SHUNT_RESISTOR_OHMS, i2c_ina2)
ina2.configure()

# (solar_daughterboard)
i2c_ina3 = SoftI2C(scl=Pin(25), sda=Pin(26), freq=100000)
ina3 = INA219(SHUNT_RESISTOR_OHMS, i2c_ina3)
ina3.configure()

# (piezo_daughterboard)
i2c_ina4 = SoftI2C(scl=Pin(14), sda=Pin(7), freq=100000)
ina4 = INA219(SHUNT_RESISTOR_OHMS, i2c_ina4)
ina4.configure()



# --------------- temp/humid sensor setup ---------------
DHT_PIN = 4
dht_sensor = dht.DHT11(Pin(DHT_PIN))



# --------------- fire sensor setup ---------------------
FIRE_PIN = 27
fire_sensor = Pin(FIRE_PIN, Pin.IN, Pin.PULL_UP)



# --------------- esp-now setup -------------------------
sta = network.WLAN(network.STA_IF)
sta.active(True)
sta.disconnect()
sta.config(channel=1)

e = espnow.ESPNow()
e.active(True)

peer = b'\x14\x2b\x2f\xae\xe4\x88'  # mac daddy addy: 142b2faee488
e.add_peer(peer)

print("Sensor sender ready")
print("Scanning I2C buses...")
print("I2C OLED1:", i2c_oled.scan())
print("I2C OLED2:", i2c_oled2.scan())
print("I2C INA1 :", i2c_ina1.scan())
print("I2C INA2 :", i2c_ina2.scan())
print("I2C INA3 :", i2c_ina3.scan())
print("I2C INA4 :", i2c_ina4.scan())

sleep(2)



# -------- peak voltage setup (5s) --------
PEAK_MS = 5000
peak_start = time.ticks_ms()

v1_max = 0.0
v2_max = 0.0
v3_max = 0.0
v4_max = 0.0



# -------- send setup (1s) --------
LIVE_MS = 1000
last_live_ms = time.ticks_ms()

# AD cycle + DHT
last_ad_ms = time.ticks_ms()
last_dht_ms = time.ticks_ms()

temperature = None
humidity = None
fire_detected = 1  # default no fire (1 = no fire)



# --------------- main loop ---------------------------
while True:
    try:
        now = time.ticks_ms()


        # ----- cycle through list of ads -----
        if time.ticks_diff(now, last_ad_ms) >= 2000:
            oled2.fill(0)
            oled2.text("- Power Tiles -", 0, 0)
            oled2.text(ads[ad_index], 0, 20)
            if ad_index + 1 < len(ads):
                oled2.text(ads[ad_index + 1], 0, 32)
            oled2.text("#VoteAnwar", 0, 52)
            oled2.show()

            ad_index = (ad_index + 2) % len(ads)
            last_ad_ms = now



        # ----- temp sensor read -----
        if time.ticks_diff(now, last_dht_ms) >= 2000:
            try:
                dht_sensor.measure()
                temperature = dht_sensor.temperature()
                humidity = dht_sensor.humidity()
            except:
                pass
            last_dht_ms = now



        # ----- getting ina voltages -----
        v1 = ina1.voltage()
        v2 = ina2.voltage()
        v3 = ina3.voltage()
        v4 = ina4.voltage()



        # night mode brightness
        v_solar_now = v2 + v4
        set_oled_brightness(v_solar_now)



        # update peaks
        if v1 > v1_max: v1_max = v1
        if v2 > v2_max: v2_max = v2
        if v3 > v3_max: v3_max = v3
        if v4 > v4_max: v4_max = v4



        # ----- update fire sensor -----
        fire_detected = fire_sensor.value()



        # ----- updating oled1 with current readings -----
        oled.fill(0)
        oled.text("Power Tile", 20, 0)

        oled.text("Temp:", 0, 20)
        oled.text("--.-C" if temperature is None else "{:.1f}C".format(temperature), 70, 20)

        oled.text("Humid:", 0, 40)
        oled.text("--.-%" if humidity is None else "{:.1f}%".format(humidity), 70, 40)


        oled.show()

        # live send (temp, humid, fire)
        if time.ticks_diff(now, last_live_ms) >= LIVE_MS:
            live_msg = "LIVE:T:{},H:{},F:{}".format(
                "--" if temperature is None else "{:.1f}".format(temperature),
                "--" if humidity is None else "{:.1f}".format(humidity),
                fire_detected
            )
            e.send(peer, live_msg)
            print("Sent:", live_msg)
            last_live_ms = now



        # send peak voltages every 5 seconds
        if time.ticks_diff(now, peak_start) >= PEAK_MS:
            peak_msg = "PEAK:V1max:{:.3f},V2max:{:.3f},V3max:{:.3f},V4max:{:.3f}".format(
                v1_max, v2_max, v3_max, v4_max
            )
            e.send(peer, peak_msg)
            print("Sent:", peak_msg)



            # reset peaks
            v1_max = v2_max = v3_max = v4_max = 0.0
            peak_start = now

        sleep(0.05)

    except OSError as err:
        print("Sensor read error:", err)
        sleep(0.5)

    except Exception as err:
        print("Error:", err)
        sleep(0.5)