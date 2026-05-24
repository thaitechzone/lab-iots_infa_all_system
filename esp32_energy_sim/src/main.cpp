// ════════════════════════════════════════════════════════════════════════════
//  Factory Energy Simulator — ESP32
//  Simulates 3-phase industrial power meter
//
//  Features:
//    • 3-Phase V, I, P, PF, Hz simulation
//    • Daily load curve (shift pattern)
//    • kWh accumulation with NVS persistence (survives reboot)
//    • 15-minute demand window tracking
//    • MQTT publish every 5s → factory/{device_id}/telemetry
//    • MQTT subscribe ← factory/{device_id}/control
//
//  MQTT Topics:
//    PUB  factory/factory_01/telemetry  — JSON payload every 5s
//    PUB  factory/factory_01/status     — online/offline
//    SUB  factory/factory_01/control    — {"cmd":"reset_demand"}
//                                        {"cmd":"set_load","value":0.8}
//                                        {"cmd":"auto_load"}
// ════════════════════════════════════════════════════════════════════════════

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <time.h>
#include "config.h"

// ─── Data Structures ──────────────────────────────────────────────────────────

struct Phase {
  float v;     // Voltage L-N (V)
  float i;     // Current (A)
  float p_kw;  // Active Power (kW)
  float pf;    // Power Factor
};

struct EnergySnapshot {
  Phase l1, l2, l3;
  float p_total_kw;
  float freq_hz;
  float kwh_total;
  float demand_15m_kw;
  char  timestamp[25];
};

// ─── Global State ─────────────────────────────────────────────────────────────

WiFiClient    wifiClient;
PubSubClient  mqttClient(wifiClient);
Preferences   prefs;
EnergySnapshot energy = {};

float manual_load = -1.0f;  // -1 = auto daily curve, 0-1 = fixed

float demand_max_kw  = 0.0f;
ulong demand_win_ms  = 0;

ulong last_pub_ms = 0;

// ─── Utility ──────────────────────────────────────────────────────────────────

float rndF(float lo, float hi) {
  return lo + (float)esp_random() / (float)UINT32_MAX * (hi - lo);
}

float clampF(float v, float lo, float hi) {
  return v < lo ? lo : (v > hi ? hi : v);
}

// ─── Daily Load Curve ─────────────────────────────────────────────────────────
//
//  Pattern (Thailand factory shift):
//   00-06:30 → 18-22%  กลางคืน (standby)
//   06:30-07:30 → ramp up
//   07:30-12:00 → 85-95%  เช้า production
//   12:00-13:00 → 60-75%  พักกลางวัน
//   13:00-17:00 → 88-98%  บ่าย peak
//   17:00-18:00 → ramp down
//   18:00-22:00 → 30-40%  เย็น light load
//   22:00-24:00 → 18-22%  ดึก (standby)
// ─────────────────────────────────────────────────────────────────────────────

float dailyLoadFactor() {
  struct tm t;
  if (!getLocalTime(&t)) return 0.70f;

  float h = t.tm_hour + t.tm_min / 60.0f;

  if (h < 6.5f)  return rndF(0.18f, 0.22f);
  if (h < 7.5f)  return 0.20f + (h - 6.5f) * 0.70f + rndF(-0.02f, 0.02f);
  if (h < 12.0f) return rndF(0.85f, 0.95f);
  if (h < 13.0f) return rndF(0.60f, 0.75f);
  if (h < 17.0f) return rndF(0.88f, 0.98f);
  if (h < 18.0f) return 0.90f - (h - 17.0f) * 0.60f + rndF(-0.02f, 0.02f);
  if (h < 22.0f) return rndF(0.30f, 0.40f);
  return rndF(0.18f, 0.22f);
}

float getLoadFactor() {
  if (manual_load >= 0.0f) return manual_load;
  return dailyLoadFactor();
}

// ─── Simulate One Phase ───────────────────────────────────────────────────────
// I = P / (V × PF)   [P in Watts, V in Volts]

Phase simPhase(float p_kw, float lf) {
  Phase ph;
  ph.v    = 220.0f + rndF(-3.5f, 3.5f);
  ph.pf   = clampF(0.75f + lf * 0.15f + rndF(-0.02f, 0.02f), 0.70f, 0.98f);
  ph.p_kw = p_kw;
  ph.i    = (ph.p_kw * 1000.0f) / (ph.v * ph.pf);
  return ph;
}

// ─── Run Simulation Cycle ─────────────────────────────────────────────────────

void runSimulation() {
  float lf      = clampF(getLoadFactor(), 0.05f, 1.0f);
  float p_total = BASE_KW * lf;

  // Slight 3-phase imbalance (realistic industrial load)
  float d1 = rndF(-0.025f, 0.025f);
  float d2 = rndF(-0.025f, 0.025f);
  float d3 = -(d1 + d2);  // ensures sum ~= 0

  energy.l1 = simPhase(p_total / 3.0f * (1.0f + d1), lf);
  energy.l2 = simPhase(p_total / 3.0f * (1.0f + d2), lf);
  energy.l3 = simPhase(p_total / 3.0f * (1.0f + d3), lf);

  energy.p_total_kw = energy.l1.p_kw + energy.l2.p_kw + energy.l3.p_kw;
  energy.freq_hz    = 50.0f + rndF(-0.05f, 0.05f);

  // kWh accumulation: E = P × t  (t = 5s = 5/3600 h)
  float delta_kwh = energy.p_total_kw * (PUBLISH_INTERVAL_MS / 1000.0f / 3600.0f);
  energy.kwh_total += delta_kwh;

  // 15-min demand window tracking
  ulong now = millis();
  if (now - demand_win_ms >= DEMAND_WINDOW_MS) {
    demand_max_kw = energy.p_total_kw;
    demand_win_ms = now;
    Serial.printf("[DEMAND] Window reset → %.1f kW\n", demand_max_kw);
  } else if (energy.p_total_kw > demand_max_kw) {
    demand_max_kw = energy.p_total_kw;
  }
  energy.demand_15m_kw = demand_max_kw;

  // Save kWh to NVS every 10 min (reduce flash write cycles)
  static ulong nvs_ms = 0;
  if (now - nvs_ms > 600000UL) {
    nvs_ms = now;
    prefs.begin("nrg", false);
    prefs.putFloat("kwh", energy.kwh_total);
    prefs.end();
    Serial.printf("[NVS] kWh saved: %.2f\n", energy.kwh_total);
  }

  // Timestamp
  struct tm t;
  if (getLocalTime(&t))
    strftime(energy.timestamp, sizeof(energy.timestamp), "%Y-%m-%dT%H:%M:%S", &t);
  else
    snprintf(energy.timestamp, sizeof(energy.timestamp), "1970-01-01T00:00:00");
}

// ─── Publish Telemetry ────────────────────────────────────────────────────────

void publishTelemetry() {
  if (!mqttClient.connected()) return;

  StaticJsonDocument<512> doc;

  doc["device_id"]  = DEVICE_ID;
  doc["timestamp"]  = energy.timestamp;

  JsonObject v = doc.createNestedObject("v");
  v["l1"] = round(energy.l1.v * 10) / 10.0;
  v["l2"] = round(energy.l2.v * 10) / 10.0;
  v["l3"] = round(energy.l3.v * 10) / 10.0;

  JsonObject i = doc.createNestedObject("i");
  i["l1"] = round(energy.l1.i * 10) / 10.0;
  i["l2"] = round(energy.l2.i * 10) / 10.0;
  i["l3"] = round(energy.l3.i * 10) / 10.0;

  JsonObject p = doc.createNestedObject("p_kw");
  p["l1"]    = round(energy.l1.p_kw * 10) / 10.0;
  p["l2"]    = round(energy.l2.p_kw * 10) / 10.0;
  p["l3"]    = round(energy.l3.p_kw * 10) / 10.0;
  p["total"] = round(energy.p_total_kw * 10) / 10.0;

  JsonObject pf = doc.createNestedObject("pf");
  pf["l1"] = round(energy.l1.pf * 1000) / 1000.0;
  pf["l2"] = round(energy.l2.pf * 1000) / 1000.0;
  pf["l3"] = round(energy.l3.pf * 1000) / 1000.0;

  doc["freq_hz"]       = round(energy.freq_hz * 100) / 100.0;
  doc["kwh_total"]     = round(energy.kwh_total * 10) / 10.0;
  doc["demand_15m_kw"] = round(energy.demand_15m_kw * 10) / 10.0;

  char buf[512];
  size_t len = serializeJson(doc, buf);

  char topic[64];
  snprintf(topic, sizeof(topic), "factory/%s/telemetry", DEVICE_ID);
  mqttClient.publish(topic, (const uint8_t*)buf, len, false);

  // Serial Monitor display
  struct tm t;
  if (getLocalTime(&t)) {
    Serial.printf("[%02d:%02d:%02d] P=%5.1fkW | V=%.0f/%.0f/%.0fV | I=%5.1f/%5.1f/%5.1fA | PF=%.2f | kWh=%7.1f | D15=%5.1fkW\n",
      t.tm_hour, t.tm_min, t.tm_sec,
      energy.p_total_kw,
      energy.l1.v, energy.l2.v, energy.l3.v,
      energy.l1.i, energy.l2.i, energy.l3.i,
      (energy.l1.pf + energy.l2.pf + energy.l3.pf) / 3.0f,
      energy.kwh_total,
      energy.demand_15m_kw
    );
  }
}

// ─── MQTT Callback ────────────────────────────────────────────────────────────

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char buf[256];
  length = length < sizeof(buf) - 1 ? length : sizeof(buf) - 1;
  memcpy(buf, payload, length);
  buf[length] = '\0';

  Serial.printf("[CTRL] %s → %s\n", topic, buf);

  StaticJsonDocument<128> doc;
  if (deserializeJson(doc, buf) != DeserializationError::Ok) return;

  const char* cmd = doc["cmd"];
  if (!cmd) return;

  if (strcmp(cmd, "reset_demand") == 0) {
    demand_max_kw = energy.p_total_kw;
    demand_win_ms = millis();
    Serial.println("[CMD] Demand window reset ✓");

  } else if (strcmp(cmd, "set_load") == 0) {
    float v = doc["value"] | -1.0f;
    if (v >= 0.0f && v <= 1.0f) {
      manual_load = v;
      Serial.printf("[CMD] Load fixed → %.0f%%\n", v * 100);
    }

  } else if (strcmp(cmd, "auto_load") == 0) {
    manual_load = -1.0f;
    Serial.println("[CMD] Load → auto daily curve ✓");
  }
}

// ─── WiFi Connect ─────────────────────────────────────────────────────────────

void connectWiFi() {
  Serial.printf("\nConnecting WiFi: %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  for (int i = 0; i < 40 && WiFi.status() != WL_CONNECTED; i++) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED)
    Serial.printf("WiFi OK  IP: %s\n", WiFi.localIP().toString().c_str());
  else
    Serial.println("WiFi FAILED — running offline (no MQTT)");
}

// ─── MQTT Connect ─────────────────────────────────────────────────────────────

void connectMQTT() {
  if (WiFi.status() != WL_CONNECTED) return;
  char cid[36];
  snprintf(cid, sizeof(cid), "energy-%s-%04x", DEVICE_ID, (uint16_t)esp_random());
  Serial.printf("MQTT → %s:%d ...", MQTT_BROKER, MQTT_PORT);
  if (mqttClient.connect(cid)) {
    Serial.println(" OK ✓");
    char ctrl[64];
    snprintf(ctrl, sizeof(ctrl), "factory/%s/control", DEVICE_ID);
    mqttClient.subscribe(ctrl);
    char stat[64];
    snprintf(stat, sizeof(stat), "factory/%s/status", DEVICE_ID);
    mqttClient.publish(stat, "{\"status\":\"online\"}");
  } else {
    Serial.printf(" FAILED rc=%d\n", mqttClient.state());
  }
}

// ─── Setup ────────────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("╔═══════════════════════════════════════════╗");
  Serial.println("║   Factory Energy Simulator  v1.0          ║");
  Serial.println("║   ESP32  →  MQTT  →  InfluxDB  →  Grafana ║");
  Serial.println("╚═══════════════════════════════════════════╝");

  // Restore kWh from NVS (persistent across reboot)
  prefs.begin("nrg", true);
  energy.kwh_total = prefs.getFloat("kwh", 0.0f);
  prefs.end();
  Serial.printf("kWh restored from NVS: %.2f kWh\n", energy.kwh_total);

  demand_win_ms = millis();

  connectWiFi();

  // Sync NTP time (required for daily load curve)
  if (WiFi.status() == WL_CONNECTED) {
    configTime(7 * 3600, 0, "pool.ntp.org", "time.google.com");
    Serial.print("NTP sync");
    struct tm t;
    for (int i = 0; i < 20 && !getLocalTime(&t); i++) {
      delay(500); Serial.print(".");
    }
    Serial.println();
    if (getLocalTime(&t))
      Serial.printf("Time: %02d:%02d:%02d Bangkok (GMT+7)\n",
                    t.tm_hour, t.tm_min, t.tm_sec);
    else
      Serial.println("NTP sync failed — using millis() based time");
  }

  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(512);
  connectMQTT();

  Serial.println();
  Serial.println("─── Publishing every 5s ────────────────────────");
  Serial.println("Time      P_Total  V(L1/L2/L3)      I(L1/L2/L3)      PF    kWh     D15m");
}

// ─── Loop ─────────────────────────────────────────────────────────────────────

void loop() {
  // Maintain MQTT connection
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) {
      static ulong retry_ms = 0;
      if (millis() - retry_ms > 5000UL) {
        retry_ms = millis();
        connectMQTT();
      }
    }
    mqttClient.loop();
  }

  // Simulate & publish every 5s
  if (millis() - last_pub_ms >= PUBLISH_INTERVAL_MS) {
    last_pub_ms = millis();
    runSimulation();
    publishTelemetry();
  }
}
