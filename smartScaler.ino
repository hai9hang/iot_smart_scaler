#include <HX711.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <map>
#include <vector>

// ============ CONFIG ============

#define HX_DT 15
#define HX_SCK 2

#define SS_PIN 18
#define RST_PIN 4
#define MOSI_PIN 17
#define MISO_PIN 16
#define SCK_PIN 5

#define TARE_PIN 34

int segmentPins[8] = { 32, 33, 25, 26, 27, 14, 12, 13 };  // a,b,c,d,e,f,g,dp
int digitPins[4] = { 23, 22, 21, 19 };                    // DIG1 -> DIG4

float scaleOffset = -474600;
float scaleFactor = -104.33;

unsigned int sampleInterval = 200;  // millis
unsigned int averageSamples = 10;
float weightThreshold = 0.05;  // kg

const int refreshDelay = 0;  // LED multiplex refresh (ms)

// ============ GLOBAL ============

HX711 scale;
MFRC522 rfid(SS_PIN, RST_PIN);

float currentWeight = 0.0;
float lastWeight = -1;
String lastUID = "";
String lastName = "";
unsigned long lastSampleTime = 0;
unsigned long lastRefresh = 0;
int currentDigit = 0;

int digits[4] = { 0, 0, 0, 0 };

std::map<String, String> uidToName;
std::map<String, std::vector<String>> historyByUID;

int sampleIndex = 0;
float sampleSum = 0;
unsigned long lastSampleMicros = 0;

const byte SEGMENT_MAP[10] = {
  0b00000011, 0b10011111, 0b00100101, 0b00001101,
  0b10011001, 0b01001001, 0b01000001, 0b00011111,
  0b00000001, 0b00001001
};

// ============ SETUP ============

void setup() {
  Serial.begin(115200);
  Wire.begin();
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN);
  rfid.PCD_Init();

  scale.begin(HX_DT, HX_SCK);
  scale.set_offset(scaleOffset);
  scale.set_scale(scaleFactor);

  for (int i = 0; i < 8; i++) pinMode(segmentPins[i], OUTPUT);
  for (int i = 0; i < 4; i++) pinMode(digitPins[i], OUTPUT);
  pinMode(TARE_PIN, INPUT);

  Serial.println("Command: WRITE <UID> <Name>, DELETE <UID>, RESET, HISTORY <UID>, HISTORY, SETID, TARE");
}

// ============ LOOP ============

void loop() {
  checkTareButton();
  multiplexDisplay();
  checkRFID();
  checkWeightNonBlocking();

  if (Serial.available()) handleSerial();
}

// ============ DISPLAY ============

void multiplexDisplay() {
  if (millis() - lastRefresh < refreshDelay) return;
  lastRefresh = millis();

  for (int i = 0; i < 4; i++) digitalWrite(digitPins[i], LOW);

  byte seg = SEGMENT_MAP[digits[currentDigit]];
  if (currentDigit == 1) seg &= 0b11111110;  // bật dp sau chữ số thứ 2

  for (int i = 0; i < 8; i++)
    digitalWrite(segmentPins[i], (seg & (1 << (7 - i))) ? HIGH : LOW);

  digitalWrite(digitPins[currentDigit], HIGH);
  currentDigit = (currentDigit + 1) % 4;
}

void updateDisplayDigits(float weight) {
  int val = int(weight * 100);  // xx.xx -> xxxx
  val = constrain(val, 0, 9999);
  digits[0] = (val / 1000) % 10;
  digits[1] = (val / 100) % 10;
  digits[2] = (val / 10) % 10;
  digits[3] = val % 10;
}

// ============ RFID ============

void checkRFID() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++)
    uid += String(rfid.uid.uidByte[i], HEX);
  rfid.PICC_HaltA();

  if (uid != lastUID) {
    lastUID = uid;
    lastName = uidToName.count(uid) ? uidToName[uid] : "Unknown";
    Serial.print("[RFID] UID: ");
    Serial.print(uid);
    Serial.print(" | Name: ");
    Serial.println(lastName);
  }
}
// ============ TARE =============

void checkTareButton() {
  static bool lastPressed = false;
  bool pressed = (digitalRead(TARE_PIN) == LOW);
  if (pressed && !lastPressed) {
    scale.tare();
    Serial.println("[TARE] Reset");
  }
  lastPressed = pressed;
}

// ============ WEIGHT ============

void checkWeightNonBlocking() {
  if (micros() - lastSampleMicros >= 2000) {
    sampleSum += scale.get_units(1);
    sampleIndex++;
    lastSampleMicros = micros();
  }

  if (sampleIndex >= averageSamples) {
    float avg = sampleSum / averageSamples / 1000.0;
    if (avg < 0) avg = 0.0;

    if (abs(avg - lastWeight) >= weightThreshold) {
      currentWeight = avg;
      lastWeight = avg;
      updateDisplayDigits(avg);

      String record = String(avg, 2) + " kg";
      historyByUID[lastUID].push_back(record);

      Serial.print("[WEIGHT] UID: ");
      Serial.print(lastUID);
      Serial.print(" | Weight: ");
      Serial.println(record);
    }

    sampleSum = 0;
    sampleIndex = 0;
  }
}

// ============ UART COMMAND ============

void handleSerial() {
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();

  if (cmd.startsWith("WRITE")) {
    int space1 = cmd.indexOf(' ');
    int space2 = cmd.indexOf(' ', space1 + 1);
    if (space1 > 0 && space2 > space1) {
      String uid = cmd.substring(space1 + 1, space2);
      String name = cmd.substring(space2 + 1);
      uidToName[uid] = name;
      Serial.print("[WRITE] UID ");
      Serial.print(uid);
      Serial.print(" = ");
      Serial.println(name);
    } else {
      Serial.println("[ERROR] Usage: WRITE <UID> <Name>");
    }
  } else if (cmd.startsWith("DELETE")) {
    int space = cmd.indexOf(' ');
    if (space > 0) {
      String uid = cmd.substring(space + 1);
      uidToName.erase(uid);
      historyByUID.erase(uid);
      Serial.print("[DELETE] UID ");
      Serial.print(uid);
      Serial.println(" removed.");
    } else {
      Serial.println("[ERROR] Usage: DELETE <UID>");
    }

  } else if (cmd == "RESET") {
    uidToName.clear();
    historyByUID.clear();
    Serial.println("[RESET] All UID and history cleared.");

  } else if (cmd.startsWith("HISTORY")) {
    int space = cmd.indexOf(' ');
    if (space > 0) {
      String uid = cmd.substring(space + 1);
      if (historyByUID.count(uid)) {
        Serial.print("-- History for UID ");
        Serial.print(uid);
        Serial.println(" --");
        for (auto &entry : historyByUID[uid]) {
          Serial.print("  - ");
          Serial.println(entry);
        }
        Serial.println("----------------------------");
      } else {
        Serial.println("[HISTORY] No history found for UID.");
      }
    } else {
      Serial.println("-- Weight History for ALL UID --");
      for (auto &pair : historyByUID) {
        Serial.print("UID: ");
        Serial.println(pair.first);
        for (auto &entry : pair.second) {
          Serial.print("  - ");
          Serial.println(entry);
        }
      }
      Serial.println("--------------------------------");
    }

  } else if (cmd.startsWith("SETID")) {
    if (lastUID == "") {
      Serial.println("[SETID] ❌ No RFID card detected recently.");
    } else {
      String name = cmd.substring(6);  // sau "SETID "
      name.trim();
      if (name.length() > 0) {
        uidToName[lastUID] = name;
        Serial.print("[SETID] ✅ UID ");
        Serial.print(lastUID);
        Serial.print(" set as: ");
        Serial.println(name);
      } else {
        Serial.println("[ERROR] Usage: SETID <Name>");
      }
    }
  } else if (cmd == "TARE") {
    Serial.println("[TARE] Scale reset to zero.");
    scale.tare();
  } else {
    Serial.println("[ERROR] Unknown command");
  }
}
