#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <WiFiManager.h>

const char *BLYNK_TEMPLATE_ID = "TMPL3UWKXwkbf";
const char *BLYNK_TEMPLATE_NAME = "load cell";

char auth[] = "KYY7zrOb7PaI99joDJ3v2qqt8BeKOCj_";  // Blynk authentication token
// Define global variables to store SSID and password
int ledPin = 2; // GPIO pin connected to the LED

BLYNK_WRITE(V1) {
  int pinValue = param.asInt(); // Get the value from the Blynk app
  digitalWrite(ledPin, pinValue); // Set the LED state based on the Blynk value
}

void setup() {
  pinMode(ledPin, OUTPUT); // Set the LED pin as an output

  Serial.begin(115200);

  // Create an instance of WiFiManager
  WiFiManager wm;

  // Uncomment the following line to reset WiFi settings for testing
  wm.resetSettings();

  // Start WiFiManager and wait for configuration via OTA
  if (!wm.autoConnect("Scale_Bot", "12345678")) {
    Serial.println("Failed to connect and hit timeout");
    delay(3000);
    // Reset and try again, or you can put other error handling code here
    ESP.restart();
  }

  Serial.println("Connected to WiFi");

  Blynk.config(auth);
  Blynk.connect();
}

void loop() {
  Blynk.run();
  
  // Your main code here
}
