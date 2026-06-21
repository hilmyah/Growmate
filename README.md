<div align="center">
  <img src="asset/growmate1.png" alt="Growmate Logo" width="180"/>
  <h1>Growmate</h1>
  <p>Sistem irigasi cerdas berbasis WEMOS D1 Mini / ESP8266 dengan Web Dashboard, LCD, Blynk, dan bot WhatsApp & Telegram.</p>
  <p>
    <a href="https://github.com/hilmyah/Growbot">🤖 Growbot — WhatsApp & Telegram Gateway</a>
  </p>
</div>

![Platform](https://img.shields.io/badge/Platform-ESP8266%20%7C%20WEMOS%20D1%20Mini-orange?logo=espressif&logoColor=white)
![Framework](https://img.shields.io/badge/Framework-Arduino%20IDE-00979D?logo=arduino&logoColor=white)
![IoT Services](https://img.shields.io/badge/IoT-Blynk-04D683?logo=blynk&logoColor=white)

## Fitur

| Fitur | Deskripsi |
| --- | --- |
| Otomasi & Mode Dual | Penyiraman otomatis berdasarkan batas ambang kelembaban tanah (*threshold*) atau eksekusi manual (Web/Blynk/Bot). |
| Multi-Interface Monitor | Pemantauan data sensor secara *real-time* melalui LCD 16x2 fisik dan Web Dashboard lokal. |
| Integrasi Cloud Blynk | Kontrol non-blocking jarak jauh menggunakan aplikasi Blynk IoT. |
| Kompatibilitas Gateway | Menyediakan REST API lokal yang diintegrasikan dengan Growbot untuk interaksi via WhatsApp dan Telegram. |
| Alokasi EEPROM | Menyimpan konfigurasi batas ambang, jadwal intermiten, dan kustomisasi preset tanaman secara persisten saat mati listrik. |

## Konsep dan Arsitektur

Firmware berjalan menggunakan penjadwalan fungsi modular (Ticker/BlynkTimer) untuk menjamin stabilitas fungsionalitas server HTTP lokal tanpa terinterupsi oleh kegagalan jaringan luar.

```text
               +--------------------------------------------+
               |        Sistem Utama WEMOS D1 Mini          |
               |                                            |
+----------+   |  +------------------+   +---------------+  |   +-------------+
|Kelembaban|-->|  |  Pembacaan ADC   |   |  Lokal Web    | <|>  |   Growbot   |
|  Tanah   |   |  |  & Kondisional   |   |  Server API   |  |   |   Gateway   |
+----------+   |  +------------------+   +---------------+  |   +-------------+
               |           |                     ^          |
               |           v                     v          |
+----------+   |  +------------------+   +---------------+  |   +-------------+
| Relay &  |<--|  | Kontrol Aktuator |   | Eksekusi Data | <|>  | Blynk Cloud |
| Pompa    |   |  |   (Relay/PWM)    |   |  EEPROM / RTC |  |   |   Service   |
+----------+   |  +------------------+   +---------------+  |   +-------------+
               +--------------------------------------------+

```

## Prasyarat

### Perangkat Keras (Hardware)

* WEMOS D1 Mini (ESP8266)
* Sensor Kelembaban Tanah Kapasitif / Resistif
* Modul Relay 5V + Pompa Air DC
* LCD I2C 16x2
* Kabel Jumper & Breadboard

### Dependensi Pustaka (Library Arduino)

Pastikan pustaka berikut dipasang melalui *Library Manager* Arduino IDE sebelum melakukan kompilasi berkas `.ino`:

| Nama Pustaka | Versi | Fungsi |
| --- | --- | --- |
| `ESP8266WiFi` | Bawaan Board | Manajemen konektivitas Wi-Fi ESP8266 |
| `ESP8266WebServer` | Bawaan Board | Penyedia layanan REST API dan Web Dashboard |
| `BlynkSimpleEsp8266` | Terbaru | Komunikasi data duplex ke server cloud Blynk |
| `LiquidCrystal_I2C` | Terbaru | Driver penampilan visual data ke modul LCD 16x2 |
| `ArduinoJson` | v6.x / v7.x | Pemrosesan serialisasi dan parsing data struktur JSON |

## Struktur Repository

| Direktori/File | Fungsi |
| --- | --- |
| `growmate/` | Folder root sketsa program Arduino utama. |
| `growmate/growmate.ino` | Berkas kode utama yang berisi alur logika inisialisasi, perulangan, dan penanganan HTTP request. |
| `asset/` | Folder aset statis lokal untuk dokumentasi grafis (Logo, Skema). |
| `index.html` | Kode sumber mentah visualisasi Web Dashboard yang ditanamkan ke dalam memori program flash ESP8266. |

## Konfigurasi Firmware

Buka berkas `growmate.ino`, lalu sesuaikan parameter makro dan variabel global berikut sebelum melakukan *flashing* ke mikrokontroler:

```cpp
// Konfigurasi Kredensial Wi-Fi Lokal
const char* ssid = "NAMA_WIFI_ANDA";
const char* password = "PASSWORD_WIFI_ANDA";

// Konfigurasi Otentikasi Blynk IoT
char auth[] = "BLYNK_AUTH_TOKEN_ANDA";
char templateId[] = "BLYNK_TEMPLATE_ID_ANDA";
char templateName[] = "BLYNK_TEMPLATE_NAME_ANDA";

// Batas Nilai Analog Sensor Kelembaban (Kalibrasi)
int thresholdSore = 700; // Nilai default ambang batas penyiraman otomatis

```

## Referensi API Endpoint

ESP8266 menjalankan server HTTP lokal pada port 80. Endpoint berikut dapat diakses oleh browser atau dihubungkan ke server perantara (Growbot Gateway):

| Method | Endpoint | Fungsi |
| --- | --- | --- |
| **GET** | `/` | Memuat antarmuka web dashboard berbasis HTML grafis. |
| **GET** | `/api/data` | Mengembalikan status terkini sistem (ADC, status pompa, mode aktif, ambang batas, jadwal) dalam format JSON. |
| **GET** | `/api/threshold?val=[nilai]` | Mengubah nilai ambang batas otomatis secara dinamis dan menyimpannya ke EEPROM. |
| **GET** | `/api/presets` | Mengambil daftar preset tanaman khusus yang tersimpan di dalam EEPROM. |
| **POST** | `/api/presets` | Menyimpan modifikasi data pengaturan preset tanaman ke EEPROM. |
| **GET** | `/api/history` | Mengembalikan susunan data riwayat pembacaan 5 nilai ADC terakhir. |
| **GET** | `/api/schedule?min=[menit]&en=[0/1]` | Mengonfigurasi penjadwalan waktu siram intermiten berkelanjutan. |
| **GET** | `/on` | Memaksa aktuator pompa untuk aktif secara manual (mengubah mode ke MANUAL). |
| **GET** | `/off` | Memaksa aktuator pompa untuk mati secara manual (mengubah mode ke MANUAL). |
| **GET** | `/auto` | Mengembalikan alur kontrol pompa ke logika pemrosesan otomatis berdasarkan sensor. |

## Peta Memori Struktur EEPROM

Penyimpanan data parameter konfigurasi di dalam EEPROM menggunakan tata letak alokasi byte berikut secara persisten:

| Alokasi Byte | Parameter Komponen | Ukuran Data |
| --- | --- | --- |
| `0–1` | Nilai Batas Ambang Utama (*Threshold*) | 2 Byte (uint16_t) |
| `2` | Jumlah total slot kustom preset tanaman | 1 Byte |
| `3–152` | Blok Data Kustom Preset (Maksimal 10 Slot Tanaman @15 Byte: 13 Byte Nama + 2 Byte Batas Nilai) | 150 Byte |
| `163` | Status Penjadwalan Aktif (`schedEnabled`, 1 = Aktif) | 1 Byte |
| `164–165` | Durasi Interval Interval Jadwal (`schedIntervalMin`) | 2 Byte (uint16_t) |

## Manajemen dan Operasional

1. Sambungkan papan pengembang WEMOS D1 Mini ke komputer menggunakan kabel data micro-USB yang kompatibel.
2. Buka aplikasi **Arduino IDE**, arahkan opsi menu ke `Tools` -> `Board` -> `ESP8266 Boards` -> Pilih `WEMOS D1 R1 & mini`.
3. Tentukan port COM yang sesuai pada menu `Tools` -> `Port`.
4. Tekan tombol **Verify** (ikon centang) untuk memastikan seluruh pustaka dependensi telah lengkap dan kompilasi berhasil.
5. Tekan tombol **Upload** (ikon panah kanan) untuk memuat kode biner langsung ke papan pengembang.
6. Buka fitur **Serial Monitor** pada *baud rate* `115200` untuk memantau alamat IP lokal yang didapatkan perangkat dari jaringan Wi-Fi.
