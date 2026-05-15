<div align="center">
  <img src="asset/growmate1.png" alt="Growmate Logo" width="180"/>
  <h1>Growmate</h1>
  <p>Sistem irigasi cerdas berbasis WEMOS D1 Mini / ESP8266 dengan Web Dashboard, LCD, Blynk, dan bot WhatsApp & Telegram</p>
  <p>
    <a href="https://github.com/hilmyah/Growbot">🤖 Growbot — WhatsApp & Telegram Gateway</a>
  </p>
</div>

---

## Daftar Isi

- [Tentang Proyek](#tentang-proyek)
- [Fitur](#fitur)
- [Struktur Repository](#struktur-repository)
- [Persyaratan](#persyaratan)
- [Tahap 1 — Persiapan Arduino IDE](#tahap-1--persiapan-arduino-ide)
- [Tahap 2 — Setup Blynk](#tahap-2--setup-blynk)
- [Tahap 3 — Konfigurasi Firmware](#tahap-3--konfigurasi-firmware)
- [Tahap 4 — Upload ke WEMOS D1 Mini](#tahap-4--upload-ke-wemos-d1-mini)
- [Tahap 5 — Remote Access](#tahap-5--remote-access)
- [Penjelasan Kode](#penjelasan-kode)
- [API Endpoint ESP8266](#api-endpoint-esp8266)

---

## Tentang Proyek

Growmate adalah firmware untuk **WEMOS D1 Mini / ESP8266** yang mengotomasi penyiraman tanaman berdasarkan pembacaan sensor kelembaban tanah. Status sistem ditampilkan secara real-time di **layar LCD I2C** yang terpasang langsung pada modul. Sistem dapat diakses melalui tiga antarmuka sekaligus: **Web Dashboard** yang berjalan langsung di ESP8266, **aplikasi Blynk** di smartphone, dan **bot WhatsApp & Telegram** melalui [Growbot](https://github.com/hilmyah/Growbot).

---

## Fitur

- **LCD I2C real-time** — tiga halaman bergantian setiap 3 detik: kelembaban, mode & pompa, threshold & counter.
- **Pembacaan sensor real-time** — nilai ADC (0–1024) dan persentase kelembaban ditampilkan di dashboard.
- **Mode otomatis** — pompa menyala/mati berdasarkan nilai threshold dengan hysteresis ±20 ADC.
- **Mode manual** — kontrol pompa langsung dengan timer otomatis 60 detik sebagai pengaman.
- **Light / dark mode** — dashboard web dengan toggle tema terang dan gelap, tersimpan di browser.
- **Grafik historis** — riwayat 40 pembacaan kelembaban terakhir dalam bentuk chart.
- **Preset tanaman** — 10 preset bawaan, dapat ditambah hingga 10 preset kustom via dashboard atau bot.
- **Persistensi EEPROM** — threshold dan preset tersimpan dan tidak hilang saat restart.
- **mDNS** — akses dashboard via `http://growmate.local` tanpa perlu mengingat IP.
- **OTA update** — upload firmware baru tanpa kabel melalui jaringan WiFi.
- **Integrasi Blynk** — pantau dan kontrol pompa dari aplikasi Blynk (V0, V1, V2, V3).
- **API JSON** — endpoint HTTP untuk integrasi dengan [Growbot](https://github.com/hilmyah/Growbot) (WhatsApp & Telegram gateway).

---

## Struktur Repository

```
Growmate/
├── growmate/
│   └── growmate.ino      # Firmware utama ESP8266
├── asset/
│   ├── growmate.png      # Logo proyek (ikon)
│   ├── growmate1.png     # Logo proyek (full)
│   ├── flowchart.png     # Diagram alur sistem
│   └── growmate.svg      # Logo versi vektor
├── index.html            # Halaman link proyek (portal publik / QR pamflet)
└── README.md
```

### Ringkasan `growmate.ino`

| Fungsi / Bagian | Keterangan |
|---|---|
| `loadFromEEPROM()` | Membaca threshold dan preset dari EEPROM saat boot |
| `saveThreshold()` | Menyimpan nilai threshold ke EEPROM setiap kali diubah |
| `savePresetsToEEPROM()` | Menyimpan seluruh data preset ke EEPROM |
| `updateLCD()` | Memperbarui tampilan LCD — 3 halaman bergantian tiap 3 detik |
| `updatePumpState()` | Mengontrol relay pompa dan memperbarui status ke Blynk |
| `MAIN_page[]` | Halaman HTML dashboard (inline di PROGMEM, light/dark mode, Chart.js) |
| `handleApi()` | Endpoint `/api/data` — JSON status sensor dan sistem |
| `handleSetThreshold()` | Endpoint `/api/threshold` — ubah threshold secara dinamis |
| `handleGetPresets()` | Endpoint `GET /api/presets` — ambil daftar preset kustom |
| `handlePostPresets()` | Endpoint `POST /api/presets` — simpan preset baru dari Growbot |
| `handleGetHistory()` | Endpoint `/api/history` — 5 data ADC terakhir (dipakai Growbot) |
| `sendToBlynk()` | Timer 1 detik — kirim data sensor ke virtual pin Blynk |
| `BLYNK_WRITE(V3)` | Handler kontrol pompa dari tombol di aplikasi Blynk |
| `loop()` | Mengelola OTA, mDNS, Blynk, LCD paging, logika auto mode, timeout manual |

---

## Persyaratan

**Perangkat keras:**

| Komponen | Pin |
|---|---|
| WEMOS D1 Mini / NodeMCU ESP8266 | — |
| Sensor kelembaban tanah | `A0` |
| Relay module | `D4` / GPIO 2, aktif LOW |
| LCD I2C 16×2 | SDA (`D2`) · SCL (`D1`) |
| Pompa air mini DC | via relay |

> ESP8266 hanya mendukung WiFi **2.4 GHz**. Pastikan tidak menggunakan jaringan 5 GHz.

**Perangkat lunak:**

- Arduino IDE 1.8+ atau 2.x
- Board package: ESP8266 by ESP8266 Community
- Library: `Blynk`, `LiquidCrystal_I2C`, `Wire`, `ESP8266WiFi`, `ESP8266WebServer`, `ESP8266mDNS`, `ArduinoOTA`, `EEPROM`
- Akun [Blynk](https://blynk.io) (gratis)

---

## Tahap 1 — Persiapan Arduino IDE

**Instal board ESP8266:**

1. Buka Arduino IDE → **File → Preferences**
2. Di kolom **Additional Boards Manager URLs**, tambahkan:
   ```
   https://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
3. Buka **Tools → Board → Boards Manager**, cari `esp8266`, instal **ESP8266 by ESP8266 Community**
4. Pilih board: **Tools → Board → ESP8266 Boards → LOLIN(WEMOS) D1 R2 & Mini**

**Instal library yang dibutuhkan** (via Sketch → Include Library → Manage Libraries):

| Library | Sumber |
|---|---|
| `Blynk` | Library Manager — "Blynk" oleh Volodymyr Shymanskyy |
| `LiquidCrystal I2C` | Library Manager — "LiquidCrystal I2C" oleh Frank de Brabander |
| `ESP8266WiFi`, `ESP8266WebServer`, `ESP8266mDNS`, `ArduinoOTA`, `EEPROM` | Otomatis tersedia setelah board package terinstal |
| `Wire` | Built-in |

---

## Tahap 2 — Setup Blynk

1. Buat akun di [blynk.io](https://blynk.io) → buat **Template** baru (Hardware: ESP8266, Connection: WiFi)
2. Buat **Datastream** berikut:

   | Virtual Pin | Nama | Tipe | Keterangan |
   |:---:|---|:---:|---|
   | V0 | Soil ADC | Integer | Nilai ADC mentah (0–1024) |
   | V1 | Mode | String | Auto / Manual ON / Manual OFF |
   | V2 | Kelembaban % | Integer | Persentase kelembaban (0–100%) |
   | V3 | Kontrol Pompa | Integer | 1 = ON · 0 = OFF |

3. Buat **Device** dari template, salin **Template ID**, **Template Name**, dan **Auth Token**

---

## Tahap 3 — Konfigurasi Firmware

Buka `growmate/growmate.ino`, ubah bagian berikut:

**Kredensial Blynk:**
```cpp
#define BLYNK_TEMPLATE_ID   "TMPLxxxxxxxxxx"
#define BLYNK_TEMPLATE_NAME "Growmate"
#define BLYNK_AUTH_TOKEN    "xxxxxxxxxxxx"
```

**Kredensial WiFi:**
```cpp
const char* ssid     = "NamaWiFiKamu";
const char* password = "PasswordWiFi";
```

**Konfigurasi pin dan LCD:**
```cpp
const int soilPin  = A0;
const int relayPin = 2;                        // GPIO2 = D4, aktif LOW
LiquidCrystal_I2C lcd(0x27, 16, 2);           // Coba 0x3F jika LCD tidak tampil
```

**Threshold default** (hanya berlaku saat EEPROM kosong pertama kali):
```cpp
int threshold = 700;   // 200–1024; rendah = basah, tinggi = kering
```

---

## Tahap 4 — Upload ke WEMOS D1 Mini

1. Hubungkan WEMOS ke komputer via USB Micro-B
2. Atur di Arduino IDE:

   | Pengaturan | Nilai |
   |---|---|
   | Board | LOLIN(WEMOS) D1 R2 & Mini |
   | Upload Speed | 115200 |
   | Port | Pilih port COM yang muncul |

3. Klik **Upload**, tunggu `Done uploading`
4. Buka **Serial Monitor** @ 115200 baud — catat IP ESP8266:
   ```
   WiFi connected — 192.168.1.X
   mDNS started — http://growmate.local
   System ready — http://192.168.1.X
   ```
5. Buka browser di perangkat yang satu jaringan, akses `http://growmate.local`

> **Troubleshooting LCD:** Jika menampilkan karakter acak, pastikan VCC LCD terhubung ke pin **5V** Wemos (bukan 3.3V). Jika tidak tampil sama sekali, coba ubah alamat dari `0x27` ke `0x3F`. Upload sketch `i2c_scanner.ino` untuk menemukan alamat yang benar.

---

## Tahap 5 — Remote Access

Agar [Growbot](https://github.com/hilmyah/Growbot) yang berjalan di cloud dapat berkomunikasi dengan ESP8266 di jaringan lokal, jalankan salah satu metode berikut di komputer yang **satu jaringan WiFi dengan ESP8266**.

### Opsi A — Cloudflare Tunnel (Direkomendasikan)

```bash
# Instalasi
brew install cloudflared           # macOS
winget install Cloudflare.cloudflared  # Windows

# Jalankan tunnel
cloudflared tunnel --url http://192.168.1.X
```

URL publik muncul di terminal — salin ke `ESP_URL` di Growbot.

> URL berubah setiap `cloudflared` dijalankan ulang. Untuk URL permanen, buat [Named Tunnel](https://developers.cloudflare.com/cloudflare-one/connections/connect-networks/) dengan akun Cloudflare gratis.

### Opsi B — Tailscale

```bash
# Linux / macOS
sudo tailscale up --advertise-routes=192.168.1.0/24

# Windows (PowerShell sebagai Administrator)
tailscale up --advertise-routes=192.168.1.0/24
```

Setujui route di [Tailscale Admin Console](https://login.tailscale.com/admin/machines), lalu gunakan IP lokal ESP8266 sebagai `ESP_URL`.

---

## Penjelasan Kode

### LCD paging

LCD I2C diperbarui tiap 1 detik secara non-blocking (`millis()`). Tiga halaman bergantian setiap 3 detik:

| Halaman | Baris 1 | Baris 2 |
|:---:|---|---|
| 1 | `Tanah: BASAH` | `ADC: 412   60%` |
| 2 | `Mode: AUTO` | `Pompa: OFF` |
| 3 | `Batas: 700` | `Disiram:   0 kali` |

### Logika mode otomatis

```cpp
if (raw > threshold + 20 && !isPumpOn) { isPumpOn = true; wateringCount++; }
if (raw < threshold - 20 &&  isPumpOn) { isPumpOn = false; }
```

Hysteresis ±20 mencegah relay chattering saat ADC berada di sekitar nilai threshold.

### Timeout manual

```cpp
const long manualTimeout = 60000;   // pompa mati otomatis setelah 60 detik
```

### Penyimpanan EEPROM

| Alamat | Data | Ukuran |
|:---:|---|:---:|
| `0–1` | Threshold (int) | 2 byte |
| `2` | Jumlah preset kustom (byte) | 1 byte |
| `3–152` | Data preset: 13 byte nama + 2 byte threshold per slot, maks 10 slot | 150 byte |

### Blynk non-blocking

Jika koneksi ke server Blynk terputus, sistem mencoba reconnect setiap 10 detik tanpa menghentikan web server, pembacaan sensor, atau tampilan LCD.

---

## API Endpoint ESP8266

| Method | Endpoint | Keterangan |
|:---:|---|---|
| GET | `/` | Halaman HTML dashboard |
| GET | `/api/data` | Status lengkap (ADC, kondisi, pompa, mode, threshold, count) |
| GET | `/api/threshold?val=700` | Ubah nilai threshold |
| GET | `/api/presets` | Ambil daftar preset kustom (JSON array) |
| POST | `/api/presets` | Simpan preset kustom (JSON body) |
| GET | `/api/history` | 5 data ADC terakhir — dipakai Growbot |
| GET | `/on` | Manual ON |
| GET | `/off` | Manual OFF |
| GET | `/auto` | Mode otomatis |

**Contoh respons `/api/data`:**
```json
{
  "adc": 732,
  "kondisi": "KERING",
  "pump": "ON",
  "mode": "AUTO",
  "threshold": 700,
  "lastWatered": 143,
  "count": 3
}
```

---

<div align="center">
  <sub>Growmate · Tugas PKK · 2025 &nbsp;|&nbsp; <a href="https://github.com/hilmyah/Growbot">Growbot — WhatsApp & Telegram Gateway</a></sub>
</div>