#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "HX711.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <WiFiMulti.h>
#include <WiFiManager.h>
#include <BlynkSimpleEsp32.h>
#include "painlessMesh.h"
#include <ArduinoJson.h> 
//....................................................<painlessMesh_init>.........................................
#define MESH_PREFIX     "whateverYouLike"
#define MESH_PASSWORD   "somethingSneaky"
#define MESH_PORT       5555
painlessMesh mesh;
//................................................................................................................
//....................................................<LCD_Init>..................................................
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Set the LCD address and dimensions (change address if necessary)
//................................................................................................................
//...........................................<HX711_Pins_&_led&buzz pin>..........................................
#define DOUT 32
#define CLK 33
#define BUTTON_PIN 13
#define BUZZER_PIN 12
#define LED_PIN 14

HX711 scale;

float calibration_factor = -9111;
float thresholdWeight = 10.0;  // Default threshold weight 10 in kg
float weight = 0.0;
float uweight = thresholdWeight;
//..................................................................................................................
//.................................................<Blynk_init>.....................................................
const char *BLYNK_TEMPLATE_ID = "TMPL3UWKXwkbf";
const char *BLYNK_TEMPLATE_NAME = "load cell";

char auth[] = "KYY7zrOb7PaI99joDJ3v2qqt8BeKOCj_";  // Blynk authentication token
BlynkTimer timer;
//..................................................................................................................
//............................................<Initialize Telegram BOT>.............................................
#define BOTtoken "6616164017:AAGWiCORRQJghTEd2ZCoEgFFFVDczD3F8WE"
#define CHAT_ID "@ScaleMasterBot"

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);
//..................................................................................................................
WiFiMulti wifiMulti;

bool isScaleOn = true;        // Variable to track the scale on/off state
bool prevWifiStatus = false;  // Previous Wi-Fi connection status
bool prevScaleState = false;  // Previous scale state
//.........................................<telegram_msg_handling>..................................................
void handleTelegramMessages() {
  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

  while (numNewMessages) {
    Serial.println("New message(s) received");

    for (int i = 0; i < numNewMessages; i++) {
      String chat_id = String(bot.messages[i].chat_id);
      String text = bot.messages[i].text;

      if (text == "/start") {
        bot.sendMessage(chat_id, "Let's boot up.");
        Serial.println("Let's boot up. sent");
      } else if (text == "/status") {
        String statusMessage = "Scale is " + String(isScaleOn ? "ON" : "OFF") + "\n";
        statusMessage += "Current Weight: " + String(weight, 3) + " kg\n";
        statusMessage += "Current threshold: " + String(thresholdWeight) + " kg\n";
        statusMessage += "Currently Underload by: " + String(uweight, 3) + " kg\n";
        bot.sendMessage(chat_id, statusMessage);
        Serial.println(statusMessage);
      } else {
        String eocMessage = "Please select a valid option";
        bot.sendMessage(chat_id, eocMessage);
        Serial.println(eocMessage);
      }
    }
    numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  }
}
//................................................................................................................
void setup() {
    Serial.begin(115200);
//....................................................<wifiManager_init>..........................................
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
//................................................................................................................

//.........................................Initialize PainlessMesh................................................

//................................................................................................................ 
  Serial.println("Press T to tare");
  lcd.init();
  scale.begin(DOUT, CLK);  // Initialize the HX711 instance with the pin numbers

  scale.set_scale(calibration_factor);
  scale.tare();

  // Initialize Blynk
  //Blynk.begin(auth, ssid, password);

  // Set up Wi-Fi network connection
  WiFi.mode(WIFI_STA);
  //wifiMulti.addAP(ssid, password);
  //.................................................................telegram.......................
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  //................................................................................................
  // Configure timer to read weight and sync with Blynk every second
  timer.setInterval(1000L, myTimer);

  pinMode(BUTTON_PIN, INPUT_PULLUP);  // Set the push button pin as INPUT_PULLUP
  pinMode(BUZZER_PIN, OUTPUT);        // Set the buzzer pin as OUTPUT

  // Initialize threshold weight from Blynk app or default value
  Blynk.syncVirtual(V4);  // Synchronize the threshold weight from the Blynk app

  lcd.begin(16, 2);  // Initialize the LCD display
  lcd.backlight();   // Turn on the backlight
  lcd.setCursor(0, 0);
  lcd.print("Weight:Init...");
  lcd.setCursor(0, 1);
  lcd.print("Threshold: ");
  lcd.print(thresholdWeight);
  lcd.print(" kg");

}

void loop() {
  Blynk.run();
  timer.run();
  handleTelegramMessages();  // Check and process Telegram messages
  // Check if there is any data available to read from the serial monitor
  if (Serial.available()) {
    char input = Serial.read();

    // Perform actions based on the user input
    if (input == 't' || input == 'T') {
      scale.tare();
      Serial.println("Tared");
      Blynk.virtualWrite(V1, 1);  // Send a signal to Blynk to trigger the tare action
    } else if (input == 'w' || input == 'W') {
      Serial.println("Enter new threshold weight:");
      while (Serial.available() == 0)
        ;  // Wait until input is received
      String weightInput = Serial.readString();
      thresholdWeight = weightInput.toFloat();
      Serial.print("New threshold weight set to: ");
      Serial.print(thresholdWeight);
      lcd.setCursor(0, 1);
      lcd.print("Threshold:");
      lcd.print(thresholdWeight);
      lcd.print("kg");
      Serial.println("kg");
      Blynk.virtualWrite(V4, thresholdWeight);  // Update threshold weight in Blynk app
    }
  }

  // Check the state of the push button
  if (digitalRead(BUTTON_PIN) == LOW) {
    isScaleOn = !isScaleOn;                     // Toggle the scale on/off state
    delay(200);                                 // Add a small delay to debounce the button
    Blynk.virtualWrite(V3, isScaleOn ? 1 : 0);  // Update the on/off button state in the Blynk app/website
  }

  // Check Wi-Fi connection and display available networks
  bool currentWifiStatus = WiFi.status() == WL_CONNECTED;
  if (currentWifiStatus != prevWifiStatus) {
    prevWifiStatus = currentWifiStatus;
    if (prevWifiStatus) {
      Serial.println("Connected to Wi-Fi");
    } else {
      Serial.println("Disconnected from Wi-Fi");
    }
  }

  if (prevWifiStatus && isScaleOn != prevScaleState) {
    if (isScaleOn) {
      scale.power_up();  // Power up the scale
      Serial.println("Scale: ON");
    } else {
      scale.power_down();  // Power down the scale
      Serial.println("Scale: OFF");
      lcd.clear();
    }
    prevScaleState = isScaleOn;
  }
}

void myTimer() {
  if (prevWifiStatus && isScaleOn) {
    weight = scale.get_units();
    if (weight < 0) { scale.tare();weight = scale.get_units(); }//..........................................Need to check if it works...........................................
    Serial.print("Weight:");
    Serial.print(weight, 3);
    Serial.println("kg");
    checkThreshold(weight);                   // Check if weight exceeds the threshold
    checkUnderload(weight, thresholdWeight);  // Check for underload condition

    // Update LCD display
    lcd.setCursor(0, 0);
    lcd.print("Weight:");
    lcd.print(weight, 2);
    lcd.print("kg");

    // Send weight data to Blynk virtual pin V2 (connected to gauge widget)
    Blynk.virtualWrite(V2, weight);
  }
}

void checkThreshold(float weight) {
  if (weight >= thresholdWeight) {
    digitalWrite(BUZZER_PIN, HIGH);  // Turn on the buzzer
    Blynk.virtualWrite(V5, 1);       // Send alert to Blynk app (virtual pin V5)
    Blynk.logEvent("alert", "The Threshold is reached");
  } else {
    digitalWrite(BUZZER_PIN, LOW);  // Turn off the buzzer
    Blynk.virtualWrite(V5, 0);      // Clear alert in Blynk app (virtual pin V5)
  }
}

BLYNK_WRITE(V1) {
  if (param.asInt() == 1) {
    scale.tare();
    Serial.println("Tared");
  }
}

BLYNK_READ(V2) {
  float weight = scale.get_units();
  Blynk.virtualWrite(V2, weight);
}

BLYNK_WRITE(V3) {
  int buttonState = param.asInt();
  if (buttonState == 1) {
    isScaleOn = true;  // Turn on the scale
  } else {
    isScaleOn = false;  // Turn off the scale
  }
}

BLYNK_WRITE(V4) {
  thresholdWeight = param.asFloat();  // Update threshold weight from Blynk app
  Serial.print("New threshold weight set to: ");
  Serial.print(thresholdWeight);
  Serial.println(" kg");

  // Update LCD display with new threshold weight
  lcd.setCursor(0, 1);
  lcd.print("Threshold:");
  lcd.print(thresholdWeight);
  lcd.print("kg");
}
//.........................................<Update display with new underweight weight>..................................
void checkUnderload(float weight, float thresholdWeight) {
  if (weight < thresholdWeight) {
    // Underload condition is met
    uweight = thresholdWeight - weight;
    Serial.print("Underweight by or the remaining capacity to fill: ");
    Serial.print(uweight);
    Serial.println(" kg");
    digitalWrite(LED_PIN, HIGH);  // Turn on the LED
    Blynk.virtualWrite(V6, 1);    // Send underload alert to Blynk app (virtual pin V6)
    Blynk.logEvent("ualert", "Underload detected");
  } else {
    // Underload condition is not met
    digitalWrite(LED_PIN, LOW);  // Turn off the LED
    Blynk.virtualWrite(V6, 0);   // Clear underload alert in Blynk app (virtual pin V6)
    uweight = 0;                 //if value goes above threshold value
  }
}
