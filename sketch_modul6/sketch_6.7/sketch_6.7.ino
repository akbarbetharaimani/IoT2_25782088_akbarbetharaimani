#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

// Kredensial Jaringan Wi-Fi
const char* ssid = "trrrr";
const char* password = "12345678";

// Konfigurasi Broker MQTT Publik
const char* mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;
const char* clientId = "ESP8266_LabIoT_25782088_Full";

// Pembagian Topik Mandiri
const char* topicPublish   = "kampus/labiot/25782088/telemetri";
const char* topicSubscribe = "kampus/labiot/25782088/kendali";
const char* topicAlert     = "kampus/labiot/25782088/alert";
const char* topicStatus    = "kampus/labiot/25782088/status";

// Alokasi Pin Perangkat Keras
const byte dhtPin = 2; // Pin D4 (GPIO 2)
const byte ledPin = 5; // Pin D1 (GPIO 5)
#define DHTTYPE DHT22

DHT dht(dhtPin, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

// Ambang batas suhu bahaya (derajat Celsius)
const float ambangSuhu = 31.0;

// Pengatur waktu non-blocking (millis)
unsigned long lastTelemetryTime = 0;
const unsigned long telemetryInterval = 4000;  // Siklus kirim 4 detik
unsigned long lastReconnectAttempt = 0;
const unsigned long reconnectInterval = 4000;  // Jeda percobaan konek MQTT
unsigned long lastWifiDot = 0;

bool firstMqttAttempt = true;  // Percobaan konek pertama tanpa menunggu
bool wifiReported = false;     // Penanda agar IP hanya dicetak sekali
bool alertAktif = false;       // Penanda (latch): alert hanya terkirim sekali per kejadian

// Mengirim status ONLINE (retain agar menimpa pesan OFFLINE dari LWT)
void publishOnline() {
  StaticJsonDocument<128> doc;
  doc["status"] = "ONLINE";
  doc["keterangan"] = "Perangkat Beroperasi Normal";
  char buf[128];
  serializeJson(doc, buf);
  client.publish(topicStatus, buf, true);
}

void callback(char* topic, byte* payload, unsigned int length) {
  char incomingBuffer[length + 1];
  memcpy(incomingBuffer, payload, length);
  incomingBuffer[length] = '\0';

  Serial.println("----------------------------------------");
  Serial.printf("[KONTROL DITERIMA] Topik: %s | Payload: %s\n", topic, incomingBuffer);

  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, incomingBuffer);
  if (err) {
    Serial.println("Gagal membedah payload JSON!");
    return;
  }

  if (doc.containsKey("led")) {
    int val = doc["led"];

    // Pengaman (asumsi): saat suhu bahaya, perintah mematikan LED ditolak
    if (alertAktif && val != 1) {
      Serial.println("--> [DITOLAK] Suhu masih di atas ambang, LED tetap menyala");
      Serial.println("----------------------------------------");
      return;
    }

    digitalWrite(ledPin, val == 1 ? HIGH : LOW);
    Serial.printf("--> [INDIKATOR] Status LED diubah: %s\n", val == 1 ? "MENYALA" : "PADAM");
  }
  Serial.println("----------------------------------------");
}

// Reconnect MQTT non-blocking: tanpa delay(), memakai millis()
void reconnect() {
  if (client.connected()) return;

  unsigned long now = millis();
  if (!firstMqttAttempt && now - lastReconnectAttempt < reconnectInterval) return;
  firstMqttAttempt = false;
  lastReconnectAttempt = now;

  Serial.print("Menghubungkan kembali ke Broker...");

  // Last Will and Testament: dikirim broker jika perangkat putus tak terduga
  const char* willMsg = "{\"status\":\"OFFLINE\",\"keterangan\":\"Perangkat Terputus Tak Terduga\"}";

  // connect(id, user, pass, willTopic, willQos, willRetain, willMessage)
  if (client.connect(clientId, NULL, NULL, topicStatus, 1, true, willMsg)) {
    Serial.println(" Sukses Terhubung!");
    client.subscribe(topicSubscribe);
    Serial.printf("Subskripsi Topik: %s\n", topicSubscribe);
    publishOnline();
  } else {
    Serial.printf("Gagal (rc=%d). Coba lagi dalam 4 detik...\n", client.state());
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  dht.begin();

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);
  Serial.print("Menghubungkan Wi-Fi");

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  client.setBufferSize(512);
}

void loop() {
  // Tunggu Wi-Fi tanpa memblokir program
  if (WiFi.status() != WL_CONNECTED) {
    wifiReported = false;
    if (millis() - lastWifiDot >= 500) {
      lastWifiDot = millis();
      Serial.print(".");
    }
    return;
  }
  if (!wifiReported) {
    wifiReported = true;
    Serial.println("\nWi-Fi Terkoneksi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  }

  reconnect();
  client.loop(); // Memelihara sesi TCP dan memproses callback masuk

  unsigned long now = millis();
  if (client.connected() && now - lastTelemetryTime >= telemetryInterval) {
    lastTelemetryTime = now;

    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (!isnan(t) && !isnan(h)) {
      // 1) Klasifikasi kondisi suhu
      const char* kondisi = (t > ambangSuhu) ? "PANAS" : "NORMAL";

      // 2) Alert bahaya: dikirim sekali, LED menyala otomatis
      if (t > ambangSuhu) {
        if (!alertAktif) {
          alertAktif = true;
          digitalWrite(ledPin, HIGH);

          StaticJsonDocument<128> docAlert;
          docAlert["status"] = "BAHAYA";
          docAlert["parameter"] = "OVERHEAT";
          docAlert["nilai_suhu"] = serialized(String(t, 1));

          char alertBuf[128];
          serializeJson(docAlert, alertBuf);
          client.publish(topicAlert, alertBuf);
          Serial.printf("[ALERT TERKIRIM] %s\n", alertBuf);
        }
      } else {
        alertAktif = false; // Reset penanda; LED dimatikan manual lewat kendali
      }

      // 3) Telemetri rutin
      StaticJsonDocument<256> docOut;
      docOut["node_id"] = clientId;
      docOut["suhu"] = serialized(String(t, 2));
      docOut["kelembapan"] = serialized(String(h, 2));
      docOut["status_led"] = digitalRead(ledPin);
      docOut["kondisi_suhu"] = kondisi;

      char buffer[256];
      serializeJson(docOut, buffer);

      bool success = client.publish(topicPublish, buffer);
      if (success) {
        Serial.printf("[TELEMETRI TERKIRIM] %s\n", buffer);
      } else {
        Serial.println("[ERROR] Gagal memublikasikan telemetri!");
      }
    }
  }
}