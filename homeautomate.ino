#define BLYNK_TEMPLATE_ID "TMPL6EIroafQm"
#define BLYNK_TEMPLATE_NAME "Home Automotionar"
#define BLYNK_AUTH_TOKEN "6Moa_KKVdsMgQeNjYEGbV5UstK04dL8m"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

// Blynk Auth Token
char auth[] = "6Moa_KKVdsMgQeNjYEGbV5UstK04dL8m";
// Your WiFi credentials
char ssid[] = "Apricus 2.0~2.4 GHz";
char pass[] = "Tsrj4W8ijDn3z4A";

// Relay pins
#define RELAY1 5
#define RELAY2 4
#define RELAY3 0
#define RELAY4 2

void setup()
{
  // Debug console
  Serial.begin(9600);

  // Set relay pins as output
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);
  pinMode(RELAY4, OUTPUT);

  // Initialize all relays to off
  digitalWrite(RELAY1, HIGH);
  digitalWrite(RELAY2, HIGH);
  digitalWrite(RELAY3, HIGH);
  digitalWrite(RELAY4, HIGH);

  // Connect to Blynk
  Blynk.begin(auth, ssid, pass);
}

void loop()
{
  Blynk.run();
}

// Blynk virtual pin handlers
BLYNK_WRITE(V1)
{
  int pinValue = param.asInt();
  digitalWrite(RELAY1, pinValue ? LOW : HIGH);
}

BLYNK_WRITE(V2)
{
  int pinValue = param.asInt();
  digitalWrite(RELAY2, pinValue ? LOW : HIGH);
}

BLYNK_WRITE(V3)
{
  int pinValue = param.asInt();
  digitalWrite(RELAY3, pinValue ? LOW : HIGH);
}

BLYNK_WRITE(V4)
{
  int pinValue = param.asInt();
  digitalWrite(RELAY4, pinValue ? LOW : HIGH);
}
