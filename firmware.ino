#include <WiFi.h>
#include <HTTPClient.h>

enum FlashFloodRisk { NONE, WARNING, ALERT };
FlashFloodRisk flashFloodRisk = NONE;

/**
* Constants
*/
const int RAIN_PIN = 5;       // Hardware Interrupt Pin for Tipping Bucket
const int SOIL_PIN = 34;      // Analog pin for Simulated Soil Moisture
const int SOIL_MOISTURE_LIMIT = 80;  
const int PRECIPITATION_LIMIT = 5;   
const int NUM_BUCKETS = 10;          
const int BUCKET_DURATION = 1000;    
int rainBuffer[NUM_BUCKETS] = {0};   
int currentBucket = 0;
unsigned long lastBucketShift = 0;
volatile int newTips = 0; 
unsigned long lastDebounceTime = 0;
unsigned long lastCloudUpdate = 0;
const unsigned long CLOUD_COOLDOWN = 10000; 
const char* ssid = "Wokwi-GUEST"; // Wokwi's virtual router
const char* password = "";
const char* cloudServerUrl = "https://httpbin.org/post"; // Free API testing endpoint

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
  
  Serial.println("Starting Flash Flood Detector...");

  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Wi-Fi Connected!");
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastBucketShift >= BUCKET_DURATION) {
    
    noInterrupts();
    int tipsToAdd = newTips;
    newTips = 0;
    interrupts();

    rainBuffer[currentBucket] += tipsToAdd;
    currentBucket = (currentBucket + 1) % NUM_BUCKETS;
    rainBuffer[currentBucket] = 0; 
    lastBucketShift = currentMillis;
    
    checkFlashFloodRisk(); 
  }
}

void checkFlashFloodRisk() {
  int totalTipsPastWindow = 0;
  for (int i = 0; i < NUM_BUCKETS; i++) {
    totalTipsPastWindow += rainBuffer[i];
  }

  int rawSoil = analogRead(SOIL_PIN);
  int soilMoisturePercent = map(rawSoil, 0, 4095, 0, 100); 

  Serial.print("Soil: ");
  Serial.print(soilMoisturePercent);
  Serial.print("% | Rain Intensity: ");
  Serial.print(totalTipsPastWindow);
  Serial.print(" tips | Flash flood risk: ");

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

  if (millis() - lastCloudUpdate > CLOUD_COOLDOWN || lastCloudUpdate == 0) {
    String alertStatus = (flashFloodRisk == ALERT) ? "true" : "false";

    String jsonPayload = "{";
    jsonPayload += "\"node\":\"ESP32_1\",";
    jsonPayload += "\"soil_pct\":" + String(soilMoisturePercent) + ",";
    jsonPayload += "\"pre_tips\":" + String(totalTipsPastWindow) + ",";
    jsonPayload += "\"flood_alert\":" + alertStatus;
    jsonPayload += "}";

    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(cloudServerUrl);
      http.addHeader("Content-Type", "application/json");
      
      int httpResponseCode = http.POST(jsonPayload);
      
      Serial.print("-> Sent to server. JSON Payload: ");
      Serial.println(jsonPayload);
      Serial.print("-> Server Response Code: ");
      Serial.println(httpResponseCode);
      
      http.end();
    } else {
      Serial.println("Error: Wi-Fi Disconnected.");
    }
    
    lastCloudUpdate = millis();
  }
}