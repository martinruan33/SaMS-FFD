# SaMS-FFD

# Flash flood detection sensor design (Fritzing)

The hardware design maps the electrical connections necessary for the Flash Flood Predictor. The system uses an Arduino Uno as the central controller, gathering data from environmental sensors and triggering a remote cloud warning via Wi-Fi. 

**Components and Functions:**
* Arduino Uno
* Capacitive Soil Moisture Sensor
* Pushbutton (Rain Gauge)
* Adafruit HUZZAH ESP8266 - Wi-Fi module.
* Breadboard

**Wiring Connections:**
* **Power & Ground Rails:** Arduino **5V** connected to the top Red rail. Arduino **GND** connected to the top Blue rail.
* **Soil Moisture Sensor:** Signal Pin to Arduino **A0**, VCC to the top **Red (5V) rail**, GND to the top **Blue (GND) rail**.
* **Rain Gauge** One leg to Arduino **Pin 2** (hardware interrupt), diagonal leg to the top **Blue (GND) rail** (relying on the Arduino's internal pull-up resistor).
* **ESP8266** TX to Arduino **Pin 10**, RX to Arduino **Pin 11**, V+ connected directly to Arduino **3.3V** (bypassing the 5V breadboard rail for safety), GND to the top **Blue (GND) rail**.

---

# Firmware implementation

The firmware evaluates flash flood risk by comparing rainfall intensity against the current soil moisture. For the evaluation, the implementation uses soil moisture limit 80% and precipitation limit 5 times 

**Core Logic:**
1. **Hardware Interrupts:** A debounce-protected interrupt on Pin 2 registers rapid clicks from the rain gauge tipping bucket, ensuring no data is lost during heavy downpours.
2. **Circular Buffer (Rain Intensity):** The system stores rainfall data in an array representing a rolling time window. As time passes, old data drops off and new data is added, calculating the true *rate* of rainfall rather than just total volume.
3. **Soil Saturation Mapping:** The analog reading from A0 is mapped to a 0-100% saturation scale. 
4. **Flash flood risk evaluation:** 
   * **NONE:** Soil is absorbing rain efficiently (< 80% saturation).
   * **WARNING:** Soil is highly saturated (>= 80%), but current rain intensity is below the 5-tip precipitation limit.
   * **ALERT:** Soil is highly saturated (>= 80%) AND rolling rain intensity meets or exceeds the 5-tip precipitation limit.
5. **Simulated JSON Telemetry:** The firmware formats the current soil data, rain intensity, and alert status into a standardized JSON string (e.g., `{"node":"Arduino_1", "soil_pct":85, "rain_tips":6, "flood_alert":"true"}`). Instead of using physical hardware pins, this payload is printed directly to the Serial Monitor every 10 seconds to simulate cloud transmission.
---

# Simulation in Wokwi

The firmware implementation is tested on Wokwi simulation. A simplified setup of the Flash floor detector system proposed in Fritzing is defined using `diagram.json` file to simulate the components in the design.

**Simulation Setup:**
* **Soil Sensor:** A **Rotary Potentiometer** is wired to Pin A0. Dragging the dial simulates the soil transitioning from 0% (bone dry) to 100% (fully saturated)."
* **Rain Gauge:** The interactive blue **Pushbutton** wired to Pin 2 allows the user to manually trigger the hardware interrupt and simulate precipitation.
* **Flash flood alert over Wi-Fi:** Because Wokwi does not support ESP8266 component in the Arduino Uno view, in this simulation we only print JSON payloads to the console to prove that the Arduino generates the correct data.