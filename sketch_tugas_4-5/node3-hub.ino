#include <painlessMesh.h>
#include <ArduinoJson.h>

#define MESH_PREFIX   "Redmi 12"
#define MESH_PASSWORD "22345678"
#define MESH_PORT     5555

#define LED_PIN 12            // D6 = GPIO12 (aktif-HIGH)

// Untuk uji: turunkan SUHU_BATAS (misal 20.0) supaya LED pasti menyala.
// Setelah berhasil, kembalikan ke 31.0
const float SUHU_BATAS = 35.0;
const int   ADC_GELAP  = 300;

Scheduler userScheduler;
painlessMesh mesh;

float suhuTerakhir = 0.0;
int   adcTerakhir  = 1023;

void evaluasiRule() {
  bool panas = suhuTerakhir > SUHU_BATAS;
  bool gelap = adcTerakhir < ADC_GELAP;
  bool ledOn = panas || gelap;

  digitalWrite(LED_PIN, ledOn ? HIGH : LOW);
  Serial.printf("[RULE] suhu=%.1f (batas %.1f) adc=%d (batas %d) -> panas=%d gelap=%d -> LED %s\n",
                suhuTerakhir, SUHU_BATAS, adcTerakhir, ADC_GELAP,
                panas, gelap, ledOn ? "ON" : "OFF");
}

void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("[RX] dari %u: %s\n", from, msg.c_str());

  StaticJsonDocument<200> doc;
  DeserializationError err = deserializeJson(doc, msg);
  if (err) {
    Serial.printf("[JSON] Gagal parsing: %s\n", err.c_str());
    return;
  }

  const char* tipe = doc["tipe"] | "";

  if (strcmp(tipe, "suhu_node") == 0) {
    suhuTerakhir = doc["suhu"] | suhuTerakhir;
    Serial.printf("[DATA] Node 1: suhu=%.1f\n", suhuTerakhir);
  } else if (strcmp(tipe, "cahaya_node") == 0) {
    adcTerakhir = doc["adc"] | adcTerakhir;
    Serial.printf("[DATA] Node 2: adc=%d\n", adcTerakhir);
  } else {
    Serial.println("[JSON] Tipe tidak dikenal");
    return;
  }

  evaluasiRule();
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("[MESH] Koneksi baru: %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("[MESH] Topologi: %s\n", mesh.subConnectionJson().c_str());
  auto nodes = mesh.getNodeList(false);
  Serial.printf("[MESH] Jumlah node terhubung: %u\n", (unsigned)nodes.size());
}

void nodeTimeAdjustedCallback(int32_t offset) {}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  // TES LED: harus menyala 2 detik saat boot
  Serial.println("Tes LED...");
  digitalWrite(LED_PIN, HIGH);
  delay(2000);
  digitalWrite(LED_PIN, LOW);
  Serial.println("Tes LED selesai");

  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  Serial.printf("Node 3 aktif, ID: %u\n", mesh.getNodeId());
}

void loop() {
  mesh.update();
}