#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Keypad.h>

// Network Configuration
// Network credentials moved to secrets.h
#include "secrets.h"

const char* ssid = WIFI_SSID;             // Wi-Fi Name from secrets.h
const char* password = WIFI_PASSWORD;     // Wi-Fi Password from secrets.h
const char* mqtt_server = MQTT_SERVER;    // MQTT server from secrets.h
const int mqtt_port = MQTT_PORT;

const char* topic_publish = "industrial/machine/telemetry";
const char* topic_subscribe = "industrial/machine/control";

// Pin Configurations
#define BUZZER_PIN 13
#define Rled 21
#define Yled 22
#define Gled 23
#define VIB_Sensor 34
#define SND_Sensor 35

const int SOUND_THRESHOLD = 90;
const int VIB_THRESHOLD = 10;

// Keypad Setup
const byte ROWS = 4; 
const byte COLS = 4; 
char keys[ROWS][COLS] = {
  {'1','4','7','*'},
  {'2','5','8','0'},
  {'3','6','9','#'},
  {'A','B','C','D'}
};

byte rowPins[ROWS] = {17, 5, 18, 19};
byte colPins[COLS] = {15, 2, 4, 16};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

enum SystemState { STATE_IDLE, STATE_NORMAL, STATE_WARNING, STATE_ABNORMAL, STATE_MAINTENANCE };
SystemState currentSystemState = STATE_NORMAL;

// Safety Management Variables
const String PASS_MAINTENANCE = "123C"; 
const String PASS_FIXED       = "789D";
String inputPassword = "";               
bool overrideActive = false;             
bool currentPythonAlarmState = false;

// Global Logging & Timing Cache Variables
String lastPrintedState = "";
unsigned long lastNetworkSendTime = 0;

// Raw Vibration Tracking Registers
int lastVibState = HIGH;
int vibrationCount = 0;
int vibrationIntensity = 0; // Quantified Feature Extraction: Real events per second

WiFiClient espClient;
PubSubClient client(espClient);

void setupWiFi() {
  delay(10);
  Serial.println("\n--- Connecting to Wi-Fi ---");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\n✅ Wi-Fi Connected!");
}

void callback(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<256> doc;
  deserializeJson(doc, payload, length);

  if (doc.containsKey("alarm")) {
    currentPythonAlarmState = doc["alarm"];
    
    if (!currentPythonAlarmState && currentSystemState != STATE_MAINTENANCE) {
      overrideActive = false;
    }

    if (currentSystemState == STATE_MAINTENANCE) {
      digitalWrite(BUZZER_PIN, HIGH);  // Active-Low Buzzer OFF
      digitalWrite(Rled, LOW);
      digitalWrite(Gled, LOW);
      digitalWrite(Yled, HIGH);        
    } 
    else if (currentPythonAlarmState && !overrideActive) {
      digitalWrite(BUZZER_PIN, LOW);   // Active-Low Buzzer ON
      digitalWrite(Rled, HIGH);
      digitalWrite(Gled, LOW);
      digitalWrite(Yled, LOW);
    } 
    else {
      digitalWrite(BUZZER_PIN, HIGH);  // Active-Low Buzzer OFF
    }
  }
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to Mosquitto Broker...");
    String clientId = "ESP32IndustrialNode-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe(topic_subscribe);
    } else {
      Serial.print("failed, rc="); Serial.print(client.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);

  pinMode(Rled, OUTPUT);
  pinMode(Yled, OUTPUT);
  pinMode(Gled, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  pinMode(VIB_Sensor, INPUT); // Handled correctly by internal module pull-ups
  pinMode(SND_Sensor, INPUT);

  digitalWrite(BUZZER_PIN, HIGH); // Active-Low off
  digitalWrite(Rled, LOW);
  digitalWrite(Yled, LOW);
  digitalWrite(Gled, HIGH); 

  setupWiFi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
  Serial.println("\nCONTROL PANEL INITIALIZATION COMPLETE. READY FOR OPERATION.");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) { setupWiFi(); }
  if (!client.connected()) { reconnectMQTT(); }
  client.loop();

  // Real-Time Vibration Event Detection & Counting (Executes every loop cycle)
  int vibState = digitalRead(VIB_Sensor);
  if (lastVibState == HIGH && vibState == LOW) {
    vibrationCount++; // Log a discrete spring compression strike against the outer post
  }
  lastVibState = vibState;

  // Keypad Input Handling for Maintenance Override (Executes every loop cycle)
  char key = keypad.getKey();
  if (key) {
    if (key == '#') {
      Serial.print("\nChecking Password Entered: [ ");
      Serial.print(inputPassword);
      Serial.println(" ]");
      
      if (inputPassword == PASS_MAINTENANCE) {
        currentSystemState = STATE_MAINTENANCE;
        overrideActive = true;          
        digitalWrite(BUZZER_PIN, HIGH); 
        digitalWrite(Rled, LOW);
        digitalWrite(Gled, LOW);
        digitalWrite(Yled, HIGH);       
        Serial.println("🔒 [STATE: MAINTENANCE] Access granted. Sensors muted for fixing.");
      } 
      else if (inputPassword == PASS_FIXED) {
        if (currentSystemState == STATE_MAINTENANCE) {
          currentSystemState = STATE_NORMAL;
          overrideActive = true;        
          digitalWrite(Yled, LOW);
          digitalWrite(Gled, HIGH);     
          Serial.println("🔄 [STATE: RESET] Machine fixed. Re-arming monitoring system...");
        }
      } else {
        Serial.println("❌ [INVALID ACCESS] Command sequence rejected.");
      }
      inputPassword = "";
    } 
    else if (key == '*') {
      inputPassword = "";
      Serial.println("\n❌ Passcode buffer cleared.");
    } 
    else {
      inputPassword += key;
      Serial.print("Current Entry Buffer: ");
      Serial.println(inputPassword);
    }
  }

  // Timed Data Processing & Telemetry Regulator (Executes exactly every 1000ms)
  unsigned long currentMillis = millis();
  if (currentMillis - lastNetworkSendTime >= 1000) {
    lastNetworkSendTime = currentMillis;

    // Fetch total accumulated pulses and instantly refresh the counter for the next window
    vibrationIntensity = vibrationCount;
    vibrationCount = 0; 

    int sound = analogRead(SND_Sensor);
    String telemetryReportState = "NORMAL";
    String currentConsoleLog = "";

    // Process output configurations unless locked under a maintenance override
    if (currentSystemState != STATE_MAINTENANCE) {
      
      // A. IDLE STATE
      if (vibrationIntensity == 0 || sound == 0) {
        if (!overrideActive) {
          digitalWrite(Gled, LOW);
          digitalWrite(Yled, LOW);
          digitalWrite(Rled, LOW);
        }
        telemetryReportState = "IDLE";
        currentSystemState = STATE_IDLE;
        currentConsoleLog = "[IDLE STATE] Vibration=" + String(vibrationIntensity) + " Hz | Sound=" + String(sound);
      }
      
      // B. NORMAL SAFE STATE
      else if (vibrationIntensity < VIB_THRESHOLD && sound < SOUND_THRESHOLD) {
        if (!overrideActive) {
          digitalWrite(Yled, LOW);   
          digitalWrite(Gled, HIGH);  
          digitalWrite(Rled, LOW);   
        }
        telemetryReportState = "NORMAL";
        currentSystemState = STATE_NORMAL; 
        currentConsoleLog = "[GREEN SAFE] Vibration=" + String(vibrationIntensity) + " Hz | Sound=" + String(sound);
      }
      
      // C. WARNING STATE
      else if (vibrationIntensity > VIB_THRESHOLD && sound < SOUND_THRESHOLD) {
        if (!overrideActive) {
          digitalWrite(Yled, HIGH);   
          digitalWrite(Gled, LOW);  
          digitalWrite(Rled, LOW);   
        }
        telemetryReportState = "WARNING";
        currentSystemState = STATE_WARNING; 
        currentConsoleLog = "[YELLOW WARNING] Vibration=" + String(vibrationIntensity) + " Hz | Sound=" + String(sound);
      }
      
      // D. ABNORMAL STATE
      else if (vibrationIntensity > 0 && sound > SOUND_THRESHOLD){
        if (!overrideActive) {
          digitalWrite(Yled, LOW);   
          digitalWrite(Gled, LOW);  
          digitalWrite(Rled, HIGH); 
        }
        telemetryReportState = "ABNORMAL";
        currentSystemState = STATE_ABNORMAL;
        currentConsoleLog = "[RED DANGER] Vibration=" + String(vibrationIntensity) + " Hz | Sound=" + String(sound);
      }

      else{
        currentConsoleLog = "Unknown State";
      }

      // Safety Override Auto-Reset Protocol
      if (currentSystemState == STATE_NORMAL || currentSystemState == STATE_IDLE) {
        overrideActive = false; 
      }

      // Console Logging with State Change Detection (Prevents log flooding by only printing on state changes)
      if (currentConsoleLog != lastPrintedState) {
        Serial.println("\n" + currentConsoleLog);
        lastPrintedState = currentConsoleLog;
      }

      // Outbound Network Telemetry Transmission (Sends extracted event data instead of raw noise)
      StaticJsonDocument<200> doc;
      doc["state"] = telemetryReportState;
      doc["vibration"] = vibrationIntensity;
      doc["sound"] = sound;

      char jsonBuffer[256];
      serializeJson(doc, jsonBuffer);
      client.publish(topic_publish, jsonBuffer);
    }
  }

  delay(10); // Small yield keeps watchdog happy while ensuring optimal keypad response
}