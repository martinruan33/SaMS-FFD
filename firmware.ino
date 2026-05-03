enum FlashFloodRisk { NONE, WARNING, ALERT };
FlashFloodRisk flashFloodRisk = NONE;

/**
* Constants
*/
const int RAIN_PIN = 2;       // Hardware Interrupt Pin for Tipping Bucket
const int SOIL_PIN = A0;      // Analog pin for Simulated Soil Moisture (Potentiometer)
const int SOIL_MOISTURE_LIMIT = 80;   // 80% saturated
const int PRECIPITATION_LIMIT = 5; // 5 rain tips within the rolling window
const int NUM_BUCKETS = 10;          // 10 time slots
const int BUCKET_DURATION = 1000;    // 1 second per slot (Scaled fast for presentation)
int rainBuffer[NUM_BUCKETS] = {0};   // Array to hold rain history
int currentBucket = 0;
unsigned long lastBucketShift = 0;
volatile int newTips = 0; 
unsigned long lastDebounceTime = 0;
unsigned long lastCloudUpdate = 0;
const unsigned long CLOUD_COOLDOWN = 10000; // Generate Wi-Fi JSON every 10 seconds

void rain_interupt() {
  if (millis() - lastDebounceTime > 200) {
    newTips++;
    lastDebounceTime = millis();
  }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(RAIN_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RAIN_PIN), rain_interupt, FALLING);
  
  Serial.println("Flash flood detector by monitoring precipitation and soil moisture");
}

void loop() {
  unsigned long currentMillis = millis();

  // Update the Circular Buffer to track intensity over time
  if (currentMillis - lastBucketShift >= BUCKET_DURATION) {
    
    // Safely pull the new tips out of the interrupt memory
    noInterrupts();
    int tipsToAdd = newTips;
    newTips = 0;
    interrupts();

    // Add tips to the current bucket
    rainBuffer[currentBucket] += tipsToAdd;

    // Shift to the next bucket and wipe it clean for new data
    currentBucket = (currentBucket + 1) % NUM_BUCKETS;
    rainBuffer[currentBucket] = 0; 

    lastBucketShift = currentMillis;
    
    // Evaluate the risk based on the new data
    checkFlashFloodRisk(); 
  }
}

void checkFlashFloodRisk() {
  // Calculate total rain in the rolling window
  int totalTipsPastWindow = 0;
  for (int i = 0; i < NUM_BUCKETS; i++) {
    totalTipsPastWindow += rainBuffer[i];
  }

  // Read Simulated Soil Moisture from Potentiometer
  int rawSoil = analogRead(SOIL_PIN);
  int soilMoisturePercent = map(rawSoil, 0, 1023, 0, 100); 

  // Print the Local Dashboard to the Serial Monitor
  Serial.print("Soil: ");
  Serial.print(soilMoisturePercent);
  Serial.print("% | Rain Intensity: ");
  Serial.print(totalTipsPastWindow);
  Serial.print(" tips | Flash flood risk: ");

  // Check flash flood risk
  if (soilMoisturePercent < SOIL_MOISTURE_LIMIT) {
    flashFloodRisk = NONE;
    Serial.println("NONE");
  } 
  else if (soilMoisturePercent >= SOIL_MOISTURE_LIMIT && totalTipsPastWindow < PRECIPITATION_LIMIT) {
    flashFloodRisk = WARNING;
    Serial.println("WARNING");
  } 
  else if (soilMoisturePercent >= SOIL_MOISTURE_LIMIT && totalTipsPastWindow >= PRECIPITATION_LIMIT) {
    flashFloodRisk = ALERT;
    Serial.println("!!! ALERT!!!");
  }

  // Send flash flood alert
  if (millis() - lastCloudUpdate > CLOUD_COOLDOWN || lastCloudUpdate == 0) {
    
    String alertStatus = (flashFloodRisk == ALERT) ? "true" : "false";

    // Format data to JSON string
    String jsonPayload = "{";
    jsonPayload += "\"node\":\"Arduino_1\",";
    jsonPayload += "\"soil_pct\":" + String(soilMoisturePercent) + ",";
    jsonPayload += "\"pre_tips\":" + String(totalTipsPastWindow) + ",";
    jsonPayload += "\"flood_alert\":" + alertStatus;
    jsonPayload += "}";

    Serial.println("-> Simulated Wi-Fi Payload: " + jsonPayload);
    
    lastCloudUpdate = millis();
  }
}