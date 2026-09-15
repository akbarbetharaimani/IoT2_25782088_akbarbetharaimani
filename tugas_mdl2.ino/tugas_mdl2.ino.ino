#include <DHT.h>

// ================== KONFIGURASI PIN ==================
#define DHTPIN 13         // D7 di NodeMCU (GPIO13)
#define DHTTYPE DHT11     // Ganti ke DHT22 kalau modulmu tipe itu
#define LDR_PIN A0        // LDR di pin analog A0
#define RELAY_PIN 12      // D6 (GPIO12) -> pin IN/Signal Relay
#define LED_PIN 5         // D1 (GPIO5)  -> LED indikator opsional

// ================== AMBANG BATAS (THRESHOLD) ==================
#define SUHU_BATAS_ATAS   34.0   // di atas ini dianggap "terlalu panas"
#define LDR_BATAS_GELAP   300    // ambang batas gelap/terang (kalibrasi manual pakai Serial Monitor)

// ================== ARAH PEMBACAAN LDR ==================
// Wiring standar (LDR dari VCC ke A0, resistor tetap dari A0 ke GND):
//   makin terang -> nilai ADC makin BESAR -> "gelap" = nilai di BAWAH ambang batas.
// Kalau LDR & resistor tetapmu posisinya kebalik (resistor dari VCC ke A0, LDR dari A0 ke GND),
// hasilnya justru terbalik: disenter malah nilainya TURUN. Itu tandanya wiring-mu begini.
// Set true kalau kamu ngalamin gejala itu (disenter = makin "gelap" menurut kode).
#define LDR_INVERTED true

// ================== SIFAT MODUL RELAY ==================
// Mayoritas modul relay 1-channel 5V yang beredar untuk Arduino/NodeMCU
// bersifat ACTIVE-LOW: sinyal LOW -> relay ON, sinyal HIGH -> relay OFF.
// Kalau pas ditest ternyata kebalik (relay malah nyala waktu harusnya mati),
// tinggal ubah nilai di bawah jadi "false".
#define RELAY_ACTIVE_LOW true

DHT dht(DHTPIN, DHTTYPE);

// Fungsi bantu supaya logika Active-Low/Active-High tidak bikin bingung
// di bagian rule engine. Cukup panggil setRelay(true) = "nyalakan aktuator".
void setRelay(bool aktif) {
  if (RELAY_ACTIVE_LOW) {
    digitalWrite(RELAY_PIN, aktif ? LOW : HIGH);
  } else {
    digitalWrite(RELAY_PIN, aktif ? HIGH : LOW);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  // Kondisi aman di awal (safety first): pastikan aktuator mati dulu
  setRelay(false);
  digitalWrite(LED_PIN, LOW);

  dht.begin();

  Serial.println(F("=== Sistem Otomasi Kontrol Lokal Terpadu - Smart Warehouse ==="));
  Serial.println(F("Inisialisasi sensor selesai. Memulai monitoring..."));
}

void loop() {
  // Akuisisi multi-sensor: DHT (digital) dan LDR (analog) dalam satu siklus
  float suhu = dht.readTemperature();
  int nilaiLDR = analogRead(LDR_PIN);

  // Validasi pembacaan DHT supaya sistem tidak salah ambil keputusan
  // gara-gara nilai NaN (gagal baca)
  if (isnan(suhu)) {
    Serial.println(F("Peringatan: Gagal membaca sensor DHT (NaN). Melewati siklus ini."));
    delay(2000); // DHT11 perlu jeda >=2 detik antar pembacaan
    return;
  }

  // ================== RULE ENGINE (kondisi OR) ==================
  bool terlaluPanas = suhu > SUHU_BATAS_ATAS;
  bool terlaluGelap;
  if (LDR_INVERTED) {
    terlaluGelap = nilaiLDR > LDR_BATAS_GELAP; // wiring terbalik: gelap = nilai TINGGI
  } else {
    terlaluGelap = nilaiLDR < LDR_BATAS_GELAP; // wiring standar: gelap = nilai RENDAH
  }

  if (terlaluPanas || terlaluGelap) {
    setRelay(true);
    digitalWrite(LED_PIN, HIGH);
    Serial.print(F("Peringatan: Aktuator Aktif! | "));
  } else {
    setRelay(false);
    digitalWrite(LED_PIN, LOW);
    Serial.print(F("Kondisi Aman | "));
  }

  // Log detail kondisi lingkungan ke Serial Monitor
  Serial.print(F("Suhu: "));
  Serial.print(suhu);
  Serial.print(F(" C | LDR: "));
  Serial.print(nilaiLDR);
  Serial.print(F(" | Panas: "));
  Serial.print(terlaluPanas ? F("Ya") : F("Tidak"));
  Serial.print(F(" | Gelap: "));
  Serial.println(terlaluGelap ? F("Ya") : F("Tidak"));

  delay(2000);
}