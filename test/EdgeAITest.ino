#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Keypad.h>

// --- Network Configuration ---
const char* ssid = "Mewtwo-2.4G";             
const char* password = "42_Health";     
const char* mqtt_server = "192.168.0.3";      
const int mqtt_port = 1883;

const char* topic_publish = "industrial/machine/telemetry";
const char* topic_subscribe = "industrial/machine/control";

// --- Hardware Pin Assignments ---
#define BUZZER_PIN 13
#define Rled 21
#define Yled 22
#define Gled 23

// --- Keypad Configuration Layout ---
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

// --- State Variables ---
enum SystemState { STATE_NORMAL, STATE_WARNING, STATE_ABNORMAL, STATE_MAINTENANCE };
SystemState currentSystemState = STATE_ABNORMAL; 

// --- Testing Control & Timing Variable Blocks ---
unsigned long lastStateChange = 0;
unsigned long stateDuration = 5000; // Starts at 5s for the initial Red state
int automatedTestStep = 0;          // 0 = RED (ABNORMAL), 1 = GREEN (NORMAL), 2 = YELLOW (WARNING)
String lastPrintedState = "";

const String PASS_MAINTENANCE = "123C"; 
const String PASS_FIXED       = "789D"; 
String inputPassword = "";               
bool overrideActive = false;             
bool currentPythonAlarmState = false;    

WiFiClient espClient;
PubSubClient client(espClient);

void setupWiFi() {
  delay(10);
  Serial.println("\n--- Connecting to Wi-Fi ---");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\n✅ Wi-Fi Connected!");
}

// 📩 MQTT Callback (Silent Background Actions - Zero console spam!)
void callback(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<256> doc;
  deserializeJson(doc, payload, length);

  if (doc.containsKey("alarm")) {
    currentPythonAlarmState = doc["alarm"];
    
    if (!currentPythonAlarmState) {
      overrideActive = false;
    }

    if (currentSystemState == STATE_MAINTENANCE) {
      digitalWrite(BUZZER_PIN, HIGH);  // Active-Low OFF
      digitalWrite(Rled, LOW);
      digitalWrite(Gled, LOW);
      digitalWrite(Yled, HIGH);        // Solid Yellow for Maintenance
    } 
    else if (currentSystemState == STATE_ABNORMAL && currentPythonAlarmState && !overrideActive) {
      digitalWrite(BUZZER_PIN, LOW);   // Active-Low ON (Buzz only in Red + Person present)
    } 
    else {
      digitalWrite(BUZZER_PIN, HIGH);  // Active-Low OFF
    }
  }
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to Mosquitto Broker...");
    String clientId = "ESP32TesterNode-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe(topic_subscribe);
    } else {
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(Rled, OUTPUT);
  pinMode(Yled, OUTPUT);
  pinMode(Gled, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  digitalWrite(BUZZER_PIN, HIGH); // Start off
  setupWiFi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
  lastStateChange = millis();
  Serial.println("\n🚀 COMBINED TIMING & KEYPAD TEST RUNNING.");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) { setupWiFi(); }
  if (!client.connected()) { reconnectMQTT(); }
  client.loop();

  unsigned long currentMillis = millis();

  // 🎹 1. REAL-TIME KEYPAD INTERCEPT BLOCK (Takes priority over timers)
  char key = keypad.getKey();
  if (key) {
    if (key == '#') {
      Serial.print("\nChecking Password Entered: [ ");
      Serial.print(inputPassword);
      Serial.println(" ]");
      
      if (inputPassword == PASS_MAINTENANCE) {
        currentSystemState = STATE_MAINTENANCE;
        overrideActive = true;          
        digitalWrite(BUZZER_PIN, HIGH); // Shut up buzzer instantly
        digitalWrite(Rled, LOW);   digitalWrite(Gled, LOW);   digitalWrite(Yled, HIGH);       
        Serial.println("🔒 [STATE: MAINTENANCE] Access Granted. Automated cycling PAUSED.");
      } 
      else if (inputPassword == PASS_FIXED) {
        if (currentSystemState == STATE_MAINTENANCE) {
          // Exit maintenance and throw straight back into the automated cycling routine
          currentSystemState = STATE_NORMAL; 
          automatedTestStep =  1; // Set to Green Safe step
          overrideActive =     true; 
          lastStateChange =    currentMillis;
          stateDuration =      2000; // 2 seconds for green
          digitalWrite(Yled, LOW);   digitalWrite(Gled, HIGH);  digitalWrite(Rled, LOW);
          Serial.println("🔄 [STATE: RESET] Maintenance Complete. Resuming automated state loops...");
        }
      } else {
        Serial.println("❌ [INVALID ACCESS] Wrong code combination.");
      }
      inputPassword = "";
    } 
    else if (key == '*') {
      inputPassword = "";
      Serial.println("\n❌ Entry Buffer Cleared.");
    } 
    else {
      inputPassword += key;
      Serial.print(key); // Prints keys inline cleanly as you press them
    }
  }

  // ⏱️ 2. AUTOMATED STATE CYCLE GENERATOR (Bypassed if user enters Maintenance Mode)
  if (currentSystemState != STATE_MAINTENANCE) {
    if (currentMillis - lastStateChange >= stateDuration) {
      lastStateChange = currentMillis;
      automatedTestStep = (automatedTestStep + 1) % 3; // Smooth transition sequence: 0 -> 1 -> 2 -> 0
      
      String telemetryState = "NORMAL";

      if (automatedTestStep == 0) {
        // DANGER / ABNORMAL STATE (Runs for 5 seconds)
        currentSystemState = STATE_ABNORMAL;
        stateDuration = 5000; 
        telemetryState = "ABNORMAL";
        
        if (!overrideActive) {
          digitalWrite(Rled, HIGH); digitalWrite(Yled, LOW); digitalWrite(Gled, LOW);
        }
        Serial.println("\n🔴 [AUTOMATED STATE: RED DANGER] Duration: 5s. Buzzer ARMED.");
      } 
      else if (automatedTestStep == 1) {
        // SAFE STATE (Runs for 2 seconds)
        currentSystemState = STATE_NORMAL;
        stateDuration = 2000;
        telemetryState = "NORMAL";
        digitalWrite(BUZZER_PIN, HIGH); // Force clear buzzer immediately
        
        if (!overrideActive) {
          digitalWrite(Rled, LOW); digitalWrite(Yled, LOW); digitalWrite(Gled, HIGH);
        }
        Serial.println("\n... 🟢 [AUTOMATED STATE: GREEN SAFE] Duration: 2s. Buzzer Muted.");
      } 
      else if (automatedTestStep == 2) {
        // WARNING STATE (Runs for 2 seconds)
        currentSystemState = STATE_WARNING;
        stateDuration = 2000;
        telemetryState = "WARNING";
        digitalWrite(BUZZER_PIN, HIGH); // Force clear buzzer immediately
        
        if (!overrideActive) {
          digitalWrite(Rled, LOW); digitalWrite(Yled, HIGH); digitalWrite(Gled, LOW);
        }
        Serial.println("\n... 🟡 [AUTOMATED STATE: YELLOW WARNING] Duration: 2s. Buzzer Muted.");
      }

      // 📤 Network Transmission Block: Updates your Python WebCam GUI instantly every time the state changes
      StaticJsonDocument<200> doc;
      doc["state"] = telemetryState;
      doc["vibration"] = (automatedTestStep == 0) ? 999 : 20;
      doc["sound"] = (automatedTestStep == 0) ? 999 : 5;
      char jsonBuffer[256];
      serializeJson(doc, jsonBuffer);
      client.publish(topic_publish, jsonBuffer);
    }
  }

  delay(50); // High scanning speed for lightning-fast keypad tracking
}