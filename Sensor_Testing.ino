#define Rled 16
#define Yled 18
#define Gled 19
#define VIB_Sensor 34
#define SND_Sensor 35

// 设定正常安静区间
int SOUND_MIN_SAFE = 15;   
int SOUND_MAX_SAFE = 90;   
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

    // ⚪ 1. INITIAL IDLE STATE (完全没信号或断开)
    if (vibration == 0 || sound == 0) {
        digitalWrite(Gled, LOW);
        digitalWrite(Yled, LOW);
        digitalWrite(Rled, LOW);
        Serial.printf("[IDLE STATE] Vib=%d | Sound=%d\n", vibration, sound);
        delay(100); // 空闲状态保持小延迟即可
    }
    else {
        bool abnormalSound = (sound < SOUND_MIN_SAFE || sound > SOUND_MAX_SAFE);
        bool abnormalVib   = (vibration > VIB_THRESHOLD);

        // 🟢 SAFE STATE (安全状态，绿灯常亮，不需要延迟 2 秒)
        if (!abnormalVib && !abnormalSound) {
            digitalWrite(Gled, HIGH);
            digitalWrite(Yled, LOW);
            digitalWrite(Rled, LOW);
            Serial.printf("[GREEN SAFE] Vib=%d | Sound=%d\n", vibration, sound);
            delay(100); // 正常检测间隔
        }
        // 🟡 WARNING STATE (仅震动异常)
        else if (abnormalVib && !abnormalSound) {
            digitalWrite(Gled, LOW);
            digitalWrite(Yled, HIGH);
            digitalWrite(Rled, LOW);
            Serial.printf("[YELLOW WARNING VIB] Vib=%d | Sound=%d -> Hold for 2s\n", vibration, sound);
            
            delay(1000); // 💡 亮点两秒钟再继续下一次 detect
        }
        // 🟡 WARNING STATE (仅声音异常)
        else if (!abnormalVib && abnormalSound) {
            digitalWrite(Gled, LOW);
            digitalWrite(Yled, HIGH);
            digitalWrite(Rled, LOW);
            Serial.printf("[YELLOW WARNING SOUND] Vib=%d | Sound=%d -> Hold for 2s\n", vibration, sound);
            
            delay(1000); // 💡 亮点两秒钟再继续下一次 detect
        }
        // 🔴 DANGER STATE (震动和声音同时异常)
        else {
            digitalWrite(Gled, LOW);
            digitalWrite(Yled, LOW);
            digitalWrite(Rled, HIGH);
            Serial.printf("[RED DANGER] Vib=%d | Sound=%d -> Hold for 2s\n", vibration, sound);
            
            delay(1000); // 💡 亮点两秒钟再继续下一次 detect
        }
    }
}