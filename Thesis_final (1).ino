#define BLYNK_TEMPLATE_ID "TMPL6Usufs_QK"
#define BLYNK_TEMPLATE_NAME "AquaFlex"
#define BLYNK_AUTH_TOKEN "z1L-DQ3rYMi4zanyN9-P8VgGHJ1Xg7uO"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <BlynkTimer.h>

// === PIN CONFIGURATION ===
#define TURBIDITY_SENSOR_PIN 32  // 
#define TDS_SENSOR_PIN 34
#define ONE_WIRE_BUS 4
#define RELAY_PUMP_OUT 14
#define RELAY_PUMP_IN 27

// === TURBIDITY CALIBRATION ===
int clearWaterADC = 2340;
int turbidWaterADC = 251;
float maxNTU = 1000.0;

// === TDS CALIBRATION ===
const float VREF = 3.3;
const float TdsFactor = 0.5;
const int SCOUNT = 30;
int tdsSamples[SCOUNT];

// === TEMPERATURE SENSOR ===
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// === WIFI CREDENTIALS ===
const char* ssid = "pp"; 
const char* password = "12345678"; 

// === TIMING VARIABLES ===
unsigned long lastActionTime = 0;
unsigned long cooldownTime = 30000; // 30 sec startup cooldown
bool replenishedOnce = false;
bool replenishedTwice = false;

// Pump state tracking
bool pumpOutState = false;
bool pumpInState = false;

// Blynk timer
BlynkTimer timer;

// === MEDIAN FILTER FOR TDS ===
int getMedianNum(int bArray[], int iFilterLen) {
  for (int i = 0; i < iFilterLen - 1; i++) {
    for (int j = 0; j < iFilterLen - i - 1; j++) {
      if (bArray[j] > bArray[j + 1]) {
        int temp = bArray[j];
        bArray[j] = bArray[j + 1];
        bArray[j + 1] = temp;
      }
    }
  }
  if ((iFilterLen & 1) > 0) {
    return bArray[(iFilterLen - 1) / 2];
  } else {
    return (bArray[iFilterLen / 2] + bArray[iFilterLen / 2 - 1]) / 2;
  }
}

// === MAIN SENSOR + LOGIC TASK ===
void readSensorsAndLogic() {
  // === TURBIDITY ===
  long sum = 0;
  for (int i = 0; i < 20; i++) {
    sum += analogRead(TURBIDITY_SENSOR_PIN);
  }
  int turbRaw = sum / 20;
  float ntu = (float)(clearWaterADC - turbRaw) /
              (float)(clearWaterADC - turbidWaterADC) * maxNTU;
  if (ntu < 0) ntu = 0;
  if (ntu > maxNTU) ntu = maxNTU;

  String waterCondition;
  if (ntu < 20) {
    waterCondition = "Clean water";
  } else if (ntu < 200) {
    waterCondition = "Cloudy water";
  } else if (ntu < 700) {
    waterCondition = "Dirty water";
  } else {
    waterCondition = "Very dirty water";
  }

  // === TDS ===
  for (int i = 0; i < SCOUNT; i++) {
    tdsSamples[i] = analogRead(TDS_SENSOR_PIN);
    delayMicroseconds(500); // tiny delay, non-blocking enough
  }
  int medianTds = getMedianNum(tdsSamples, SCOUNT);
  float tdsVoltage = medianTds * (VREF / 4095.0);
  float tdsValue = (133.42 * pow(tdsVoltage, 3) -
                    255.86 * pow(tdsVoltage, 2) +
                    857.39 * tdsVoltage) * TdsFactor;

  // === TEMPERATURE ===
  sensors.requestTemperatures();
  float waterTemp = sensors.getTempCByIndex(0);

  // === MAIN LOGIC ===
  unsigned long currentMillis = millis();
  String message = "";
  unsigned long remainingCooldown = 0;

  if (currentMillis - lastActionTime >= cooldownTime) {
    if (waterCondition == "Dirty water" || waterCondition == "Very dirty water") {
      if (!replenishedOnce) {
        runReplenishment();
        replenishedOnce = true;
        cooldownTime = 30UL * 60UL * 1000UL; // 30 min
        lastActionTime = millis();
        message = "First replenishment done";
      }
      else if (replenishedOnce && !replenishedTwice) {
        runReplenishment();
        replenishedTwice = true;
        cooldownTime = 30UL * 60UL * 1000UL; // 30 min
        lastActionTime = millis();
        message = "Second replenishment done";
      }
      else if (replenishedOnce && replenishedTwice) {
        message = "Water still dirty — check tank personally";
        cooldownTime = 7UL * 24UL * 60UL * 60UL * 1000UL; // 1 week
        lastActionTime = millis();
      }
    }
    else {
      replenishedOnce = false;
      replenishedTwice = false;
      cooldownTime = 30000; 
      message = "Water clean/cloudy — cooldown 30 sec";
      lastActionTime = millis();
    }
  } else {
    remainingCooldown = (cooldownTime - (currentMillis - lastActionTime)) / 1000UL;
    if (waterCondition == "Dirty water" || waterCondition == "Very dirty water") {
      message = replenishedOnce && replenishedTwice ? 
                "Waiting — check tank personally" : 
                "Waiting for cooldown to retry";
    } else {
      message = "Monitoring";
    }
  }

  // === OUTPUT STATUS ===
  Serial.print("Temp : "); Serial.print(waterTemp, 0); Serial.print("C | ");
  Serial.print("TDS : "); Serial.print(tdsValue, 0); Serial.print(" ppm | ");
  Serial.print("Turbidity : "); Serial.print(waterCondition); Serial.print(" || ");
  Serial.print("Pump out : "); Serial.print(pumpOutState ? "ON" : "OFF"); Serial.print(" | ");
  Serial.print("Pump in : "); Serial.print(pumpInState ? "ON" : "OFF"); Serial.print(" | ");
  Serial.print("Message : "); Serial.print(message); Serial.print(" | ");
  Serial.print("Cooldown : ");
  Serial.println(remainingCooldown);

  if (WiFi.status() == WL_CONNECTED) {
    Blynk.virtualWrite(V0, waterCondition);
    Blynk.virtualWrite(V1, tdsValue);
    Blynk.virtualWrite(V2, waterTemp);
    Blynk.virtualWrite(V3, message);
    Blynk.virtualWrite(V4, pumpOutState ? "Pump Out: ON" : "Pump Out: OFF");
  }
}

// === NON-BLOCKING PUMP CONTROL ===
void runReplenishment() {
  // Pump out for 40s
  pumpOutState = true;
  digitalWrite(RELAY_PUMP_OUT, HIGH);
  timer.setTimeout(40000L, []() {
    digitalWrite(RELAY_PUMP_OUT, LOW);
    pumpOutState = false;

    // Then pump in for 40s
    pumpInState = true;
    digitalWrite(RELAY_PUMP_IN, HIGH);
    timer.setTimeout(40000L, []() {
      digitalWrite(RELAY_PUMP_IN, LOW);
      pumpInState = false;
    });
  });
}

void setup() {
  Serial.begin(115200);
  pinMode(TURBIDITY_SENSOR_PIN, INPUT);
  pinMode(TDS_SENSOR_PIN, INPUT);
  analogReadResolution(12);

  pinMode(RELAY_PUMP_OUT, OUTPUT);
  pinMode(RELAY_PUMP_IN, OUTPUT);
  digitalWrite(RELAY_PUMP_OUT, LOW);
  digitalWrite(RELAY_PUMP_IN, LOW);

  sensors.begin();
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);

  lastActionTime = millis();

  // Schedule sensor reads every 2s
  timer.setInterval(2000L, readSensorsAndLogic);
}

void loop() {
  Blynk.run();
  timer.run();
}