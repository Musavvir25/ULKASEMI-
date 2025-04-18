#define BLYNK_TEMPLATE_ID "TMPL6zNfGHv5p"
#define BLYNK_DEVICE_NAME "SmartHomeController"
#define BLYNK_AUTH_TOKEN "a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6"

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <ESPmDNS.h>
#include <Update.h>

// Credentials
char ssid[] = "Home_Network_5G";
char pass[] = "SecurePass123";
char auth[] = BLYNK_AUTH_TOKEN;

// Pins
const int lightPin = 18;
const int fanPin = 19;

// Virtual Pins
#define VPIN_LIGHT V1
#define VPIN_FAN   V2
#define VPIN_RESET V10  // Reset device via Blynk
#define VPIN_STATUS V20 // Online/offline indicator

// Device states
bool lightState = false;
bool fanState = false;

// WiFi Watchdog Timer
unsigned long lastWiFiCheck = 0;
const unsigned long wifiCheckInterval = 10000; // 10 sec

// Sync relay with app state
void updateRelay(int pin, bool state, int vpin) {
  digitalWrite(pin, state);
  Blynk.virtualWrite(vpin, state);
}

// Blynk Button for Light
BLYNK_WRITE(VPIN_LIGHT) {
  lightState = param.asInt();
  updateRelay(lightPin, lightState, VPIN_LIGHT);
  Serial.println("Light: " + String(lightState ? "ON" : "OFF"));
}

// Blynk Button for Fan
BLYNK_WRITE(VPIN_FAN) {
  fanState = param.asInt();
  updateRelay(fanPin, fanState, VPIN_FAN);
  Serial.println("Fan: " + String(fanState ? "ON" : "OFF"));
}

// Optional: Trigger reset from Blynk app
BLYNK_WRITE(VPIN_RESET) {
  int resetCmd = param.asInt();
  if (resetCmd == 1) {
    Serial.println("Reset command received from Blynk.");
    ESP.restart();
  }
}

// Show online status
void heartbeat() {
  Blynk.virtualWrite(VPIN_STATUS, 1); // 1 means online
}

// Reconnect logic
void checkWiFi() {
  if (millis() - lastWiFiCheck > wifiCheckInterval) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi disconnected. Attempting reconnect...");
      WiFi.begin(ssid, pass);
    }
    lastWiFiCheck = millis();
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(lightPin, OUTPUT);
  pinMode(fanPin, OUTPUT);
  digitalWrite(lightPin, LOW);
  digitalWrite(fanPin, LOW);

  Blynk.begin(auth, ssid, pass);

  // Optional OTA setup
  if (!MDNS.begin("esp32")) {
    Serial.println("Error starting mDNS");
  } else {
    Serial.println("OTA Ready - Type: http://esp32.local");
  }

  // Sync state with app
  Blynk.virtualWrite(VPIN_LIGHT, lightState);
  Blynk.virtualWrite(VPIN_FAN, fanState);
  heartbeat();
}

void loop() {
  Blynk.run();
  checkWiFi();

  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat > 30000) {
    heartbeat(); // update Blynk every 30 sec
    lastHeartbeat = millis();
  }
}
