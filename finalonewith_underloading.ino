#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "HX711.h"
#include <WiFi.h>
#include "WiFiProv.h"
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <WiFiMulti.h>
#include <BlynkSimpleEsp32.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);  // Set the LCD address and dimensions (change address if necessary)

#define DOUT  32
#define CLK  33
#define BUTTON_PIN 13
#define BUZZER_PIN 12

HX711 scale;

const char* BLYNK_TEMPLATE_ID = "TMPL3UWKXwkbf";
const char* BLYNK_TEMPLATE_NAME = "load cell";

char auth[] = "KYY7zrOb7PaI99joDJ3v2qqt8BeKOCj_";  // Blynk authentication token
// Define global variables to store SSID and password
char ssid[20]="OnePlus";      
char password[20]="8431748007";  

//use this if u don't want default values i.e shown above but make sure to comment the below 2 lines
// char ssid[32];      
// char password[64]; 

// #define USE_SOFT_AP // Uncomment if you want to enforce using Soft AP method instead of BLE

const char * pop = "abcd1234"; // Proof of possession - otherwise called a PIN - string provided by the device, entered by user in the phone app
const char * service_name = "PROV_ScaleBot"; // Name of your device (the Espressif apps expects by default device name starting with "Prov_")
const char * service_key = NULL; // Password used for SofAP method (NULL = no password needed)
bool reset_provisioned = true; //make this true if u want to change wifi ssid and password even if it's connected to any network and false to use the first time connected network

float calibration_factor = -96650;

// Initialize Telegram BOT
#define BOTtoken "6616164017:AAGWiCORRQJghTEd2ZCoEgFFFVDczD3F8WE"
#define CHAT_ID "@ScaleMasterBot"

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

WiFiMulti wifiMulti;

BlynkTimer timer;

bool isScaleOn = true;  // Variable to track the scale on/off state
bool prevWifiStatus = false;  // Previous Wi-Fi connection status
bool prevScaleState = false;  // Previous scale state

float thresholdWeight = 10.0;  // Default threshold weight in kg
float weight = 0.0;

void SysProvEvent(arduino_event_t *sys_event)
{
    switch (sys_event->event_id) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        Serial.print("\nConnected IP address : ");
        Serial.println(IPAddress(sys_event->event_info.got_ip.ip_info.ip.addr));
        break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        Serial.println("\nDisconnected. Connecting to the AP again... ");
        break;
    case ARDUINO_EVENT_PROV_START:
        Serial.println("\nProvisioning started\nGive Credentials of your access point using smartphone app");
        break;
    case ARDUINO_EVENT_PROV_CRED_RECV: {
        Serial.println("\nReceived Wi-Fi credentials");
        Serial.print("\tSSID : ");
        Serial.println((const char *) sys_event->event_info.prov_cred_recv.ssid);
        Serial.print("\tPassword : ");
        Serial.println((char const *) sys_event->event_info.prov_cred_recv.password);
                    // Copy the received SSID and password to the global variables
        strncpy(ssid, (const char *)sys_event->event_info.prov_cred_recv.ssid, sizeof(ssid));
        strncpy(password, (const char *)sys_event->event_info.prov_cred_recv.password, sizeof(password));
        break;
    }
    case ARDUINO_EVENT_PROV_CRED_FAIL: {
        Serial.println("\nProvisioning failed!\nPlease reset to factory and retry provisioning\n");
        if(sys_event->event_info.prov_fail_reason == WIFI_PROV_STA_AUTH_ERROR)
            Serial.println("\nWi-Fi AP password incorrect");
        else
            Serial.println("\nWi-Fi AP not found....Add API \" nvs_flash_erase() \" before beginProvision()");
        break;
    }
    case ARDUINO_EVENT_PROV_CRED_SUCCESS:
        Serial.println("\nProvisioning Successful");
        break;
    case ARDUINO_EVENT_PROV_END:
        Serial.println("\nProvisioning Ends");
        break;
    default:
        break;
    }
}


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
      }
      else if (text == "/status") {
        String statusMessage = "Scale is " + String(isScaleOn ? "ON" : "OFF") + "\n";      
        statusMessage += "Current Weight: " + String(weight, 3) + " kg\n";
        statusMessage += "Current threshold: " + String(thresholdWeight) + " kg";
        bot.sendMessage(chat_id, statusMessage);
        Serial.println(statusMessage);
      }
      else{
        String eocMessage ="Please select a valid option";
        bot.sendMessage(chat_id, eocMessage);
        Serial.println(eocMessage);
      }
    }
    numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.onEvent(SysProvEvent);

  #if CONFIG_IDF_TARGET_ESP32 && CONFIG_BLUEDROID_ENABLED && not USE_SOFT_AP
    Serial.println("Begin Provisioning using BLE");
    // Sample uuid that user can pass during provisioning using BLE
    uint8_t uuid[16] = {0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
                        0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02 };
    WiFiProv.beginProvision(WIFI_PROV_SCHEME_BLE, WIFI_PROV_SCHEME_HANDLER_FREE_BTDM, WIFI_PROV_SECURITY_1, pop, service_name, service_key, uuid, reset_provisioned);
#else
    Serial.println("Begin Provisioning using Soft AP");
    WiFiProv.beginProvision(WIFI_PROV_SCHEME_SOFTAP, WIFI_PROV_SCHEME_HANDLER_NONE, WIFI_PROV_SECURITY_1, pop, service_name, service_key);
#endif

  #if CONFIG_BLUEDROID_ENABLED && not USE_SOFT_AP
    log_d("ble qr");
    WiFiProv.printQR(service_name, pop, "ble");
  #else
    log_d("wifi qr");
    WiFiProv.printQR(service_name, pop, "softap");
  #endif

  // Print SSID and password to verify
  Serial.println("SSID: " + String(ssid));
  Serial.println("Password: " + String(password));

  Serial.println("Press T to tare");
  lcd.init(); 
  scale.begin(DOUT, CLK);  // Initialize the HX711 instance with the pin numbers

  scale.set_scale(-96650);
  scale.tare();

  // Set up Wi-Fi network connection
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(ssid, password);
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);

  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  // Configure timer to read weight and sync with Blynk every second
  timer.setInterval(1000L, myTimer);

  pinMode(BUTTON_PIN, INPUT_PULLUP);  // Set the push button pin as INPUT_PULLUP
  pinMode(BUZZER_PIN, OUTPUT);  // Set the buzzer pin as OUTPUT

  // Initialize threshold weight from Blynk app or default value
  Blynk.syncVirtual(V4);  // Synchronize the threshold weight from the Blynk app

  lcd.begin(16, 2);  // Initialize the LCD display
  lcd.backlight();  // Turn on the backlight
  lcd.setCursor(0, 0);
  lcd.print("Weight: Initializing...");
  lcd.setCursor(0, 1);
  lcd.print("Threshold: ");
  lcd.print(thresholdWeight);
  lcd.print(" kg");

  // Initialize Blynk
  Blynk.begin(auth, ssid, password);
}

void loop() {
  Blynk.run();
  timer.run();
  handleTelegramMessages(); // Check for and process Telegram messages
  // Check if there is any data available to read from the serial monitor
  if (Serial.available()) {
    char input = Serial.read();

    // Perform actions based on the user input
    if (input == 't' || input == 'T') {
      scale.tare();
      Serial.println("Tared");
      Blynk.virtualWrite(V1, 1); // Send a signal to Blynk to trigger the tare action
    } else if (input == 'w' || input == 'W') {
      Serial.println("Enter new threshold weight:");
      while (Serial.available() == 0); // Wait until input is received
      String weightInput = Serial.readString();
      thresholdWeight = weightInput.toFloat();
      Serial.print("New threshold weight set to: ");
      Serial.print(thresholdWeight);
      lcd.setCursor(0, 1);
      lcd.print("Threshold: ");
      lcd.print(thresholdWeight);
      lcd.print("kg");
      Serial.println(" kg");
      Blynk.virtualWrite(V4, thresholdWeight); // Update threshold weight in Blynk app
    }
  }

  // Check the state of the push button
  if (digitalRead(BUTTON_PIN) == LOW) {
    isScaleOn = !isScaleOn;  // Toggle the scale on/off state
    delay(200);  // Add a small delay to debounce the button
    Blynk.virtualWrite(V3, isScaleOn ? 1 : 0); // Update the on/off button state in the Blynk app/website
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
    Serial.print("Weight: ");
    Serial.print(weight, 3);
    Serial.println(" kg");
    checkThreshold(weight);  // Check if weight exceeds the threshold

    // Update LCD display
    lcd.setCursor(0, 0);
    lcd.print("Weight: ");
    lcd.print(weight, 3);
    lcd.print(" kg");

    // Send weight data to Blynk virtual pin V2 (connected to gauge widget)
    Blynk.virtualWrite(V2, weight);
  }
}

void checkThreshold(float weight) {
  if (weight >= thresholdWeight) {
    digitalWrite(BUZZER_PIN, HIGH);  // Turn on the buzzer
    Blynk.virtualWrite(V5, 1);  // Send alert to Blynk app (virtual pin V5)
    Blynk.logEvent("alert","The Threshold is reached");
  } else {
    digitalWrite(BUZZER_PIN, LOW);  // Turn off the buzzer
    Blynk.virtualWrite(V5, 0);  // Clear alert in Blynk app (virtual pin V5)
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
  lcd.print("Threshold: ");
  lcd.print(thresholdWeight);
  lcd.print(" kg");
}
BLYNK_READ(V6) {
    if (weight != thresholdWeight) {
    Blynk.virtualWrite(V6, 1);
    Serial.print("Underweight by or the remaining capacity to fill: ");
    float uweight=thresholdWeight-weight; 
    Serial.println(uweight);
    }
}
