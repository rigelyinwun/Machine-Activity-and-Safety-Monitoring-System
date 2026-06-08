#define Rled 16
#define Yled 18
#define Gled 19
#define VIB_Sensor 34
#define SND_Sensor 35

int SOUND_THRESHOLD = 50;
int VIB_THRESHOLD   = 80;

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

    // ⚪ INITIAL IDLE STATE
    if (vibration == 0 || sound == 0) {
        digitalWrite(Gled, LOW);
        digitalWrite(Yled, LOW);
        digitalWrite(Rled, LOW);
        Serial.printf("[IDLE STATE] Sensor Disconnected | Vib=%d | Sound=%d\n", vibration, sound);
    }
    else {
        bool abnormalVib   = (vibration > VIB_THRESHOLD);
        bool abnormalSound = (sound > SOUND_THRESHOLD);

        // 🟢 SAFE STATE
        if (!abnormalVib && !abnormalSound) {
            digitalWrite(Gled, HIGH);
            digitalWrite(Yled, LOW);
            digitalWrite(Rled, LOW);
            Serial.printf("[GREEN SAFE] Vib=%d | Sound=%d\n", vibration, sound);
        }
        // 🟡 WARNING STATE
        else if (abnormalVib && !abnormalSound) {
            digitalWrite(Gled, LOW);
            digitalWrite(Yled, HIGH);
            digitalWrite(Rled, LOW);
            Serial.printf("[YELLOW WARNING] Vib=%d | Sound=%d\n", vibration, sound);
        }
        // 🟡 WARNING STATE
        else if (!abnormalVib && abnormalSound) {
            digitalWrite(Gled, LOW);
            digitalWrite(Yled, HIGH);
            digitalWrite(Rled, LOW);
            Serial.printf("[YELLOW SOUND ONLY] Vib=%d | Sound=%d\n", vibration, sound);
        }
        // 🔴 DANGER STATE
        else {
            digitalWrite(Gled, LOW);
            digitalWrite(Yled, LOW);
            digitalWrite(Rled, HIGH);
            Serial.printf("[RED DANGER] Vib=%d | Sound=%d\n", vibration, sound);
        }
    }
    delay(20);
}