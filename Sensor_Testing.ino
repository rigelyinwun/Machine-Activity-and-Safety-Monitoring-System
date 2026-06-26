#define Rled 16
#define Yled 18
#define Gled 19
#define VIB_Sensor 34
#define SND_Sensor 35

int SOUND_THRESHOLD = 100;
int VIB_THRESHOLD   = 60;   

void setup() {
    Serial.begin(115200);
    analogReadResolution(12);
    pinMode(Rled, OUTPUT);
    pinMode(Yled, OUTPUT);
    pinMode(Gled, OUTPUT);
    pinMode(VIB_Sensor, INPUT);
    pinMode(SND_Sensor, INPUT);
}

void loop() {
    int vibration = analogRead(VIB_Sensor);
    int sound     = analogRead(SND_Sensor);

    // INITIAL IDLE STATE
    if (vibration == 0 || sound == 0) {
        digitalWrite(Gled, LOW);
        digitalWrite(Yled, LOW);
        digitalWrite(Rled, LOW);
        Serial.printf("[IDLE STATE] Vib=%d | Sound=%d\n", vibration, sound);
    }
    // Safe State
    else if (vibration < VIB_THRESHOLD && sound < SOUND_THRESHOLD) {
            digitalWrite(Yled, LOW);   
            digitalWrite(Gled, HIGH);  
            digitalWrite(Rled, LOW);   
            Serial.printf("[GREEN SAFE] Vib=%d | Sound=%d\n", vibration, sound);
        }
        
    // Warning State
    else if (vibration > VIB_THRESHOLD && sound < SOUND_THRESHOLD) {
        digitalWrite(Yled, HIGH);
        digitalWrite(Gled, LOW);  
        digitalWrite(Rled, LOW);   
        Serial.printf("[YELLOW WARNING] Vib=%d | Sound=%d \n", vibration, sound);
    }
    // Danger State
    else if (vibration > 0 && sound > SOUND_THRESHOLD) {
        digitalWrite(Yled, LOW);   
        digitalWrite(Gled, LOW);  
        digitalWrite(Rled, HIGH); 
        Serial.printf("[RED DANGER] Vib=%d | Sound=%d n", vibration, sound);
      }
    delay(1000);
}