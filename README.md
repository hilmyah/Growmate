<div align="center">
  <img src="asset/growmate1.png" alt="Growmate Logo" width="180"/>
  <h1>Growmate</h1>
  <p>Sistem irigasi cerdas berbasis ESP8266 dengan Web Dashboard, Blynk, dan bot WhatsApp</p>
  <p>
    <a href="https://github.com/hilmyah/Growbot">🤖 Growbot — WhatsApp Gateway</a>
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
- [Tahap 4 — Upload ke ESP8266](#tahap-4--upload-ke-esp8266)
- [Tahap 5 — Remote Access](#tahap-5--remote-access)
- [Penjelasan Kode](#penjelasan-kode)
- [API Endpoint ESP8266](#api-endpoint-esp8266)

---

## Tentang Proyek

Growmate adalah firmware untuk NodeMCU / ESP8266 yang mengotomasi penyiraman tanaman berdasarkan pembacaan sensor kelembaban tanah. Sistem dapat diakses melalui tiga antarmuka: **Web Dashboard** yang berjalan langsung di ESP8266, **aplikasi Blynk** di smartphone, dan **bot WhatsApp** melalui [Growbot](https://github.com/hilmyah/Growbot).

---

## Fitur

- **Pembacaan sensor real-time** — nilai ADC (0–1024) dan persentase kelembaban ditampilkan di dashboard.
- **Mode otomatis** — pompa menyala/mati otomatis berdasarkan nilai threshold yang dapat dikustomisasi.
- **Mode manual** — kontrol pompa langsung dengan timer otomatis 60 detik sebagai pengaman.
- **Light / dark mode** — dashboard web dengan toggle tema terang dan gelap.
- **Grafik historis** — riwayat 40 pembacaan kelembaban terakhir dalam bentuk chart.
- **Preset tanaman** — simpan hingga 10 konfigurasi threshold per jenis tanaman, dapat ditambah via WhatsApp.
- **Persistensi EEPROM** — threshold dan preset tersimpan dan tidak hilang saat ESP8266 restart.
- **mDNS** — akses dashboard via `http://growmate.local` tanpa perlu mengingat IP.
- **OTA update** — upload firmware baru tanpa kabel melalui jaringan WiFi.
- **Integrasi Blynk** — pantau dan kontrol pompa dari aplikasi Blynk (V0, V1, V2, V3).
- **API JSON** — endpoint HTTP untuk integrasi dengan Growbot (WhatsApp gateway).

---

## Struktur Repository

```
Growmate/
├── growmate/
│   └── growmate.ino      # Firmware utama ESP8266
├── asset/
│   ├── growmate.png      # Logo proyek
│   └── growmate.svg      # Logo versi vektor
├── index.html            # Halaman link proyek (portal publik)
└── README.md
```

### Ringkasan `growmate.ino`

| Fungsi / Bagian | Keterangan |
|---|---|
| `loadFromEEPROM()` | Membaca threshold dan preset tanaman dari EEPROM saat boot |
| `saveThreshold()` | Menyimpan nilai threshold ke EEPROM setiap kali diubah |
| `savePresetsToEEPROM()` | Menyimpan seluruh data preset tanaman ke EEPROM |
| `updatePumpState()` | Mengontrol relay pompa dan memperbarui status ke Blynk |
| `MAIN_page[]` | Halaman HTML dashboard (inline di PROGMEM, mendukung light/dark mode dan Chart.js) |
| `handleApi()` | Endpoint `/api/data` — mengembalikan JSON status sensor dan sistem |
| `handleSetThreshold()` | Endpoint `/api/threshold` — mengubah nilai threshold secara dinamis |
| `handleGetPresets()` | Endpoint `GET /api/presets` — mengambil daftar preset tanaman |
| `handlePostPresets()` | Endpoint `POST /api/presets` — menyimpan preset baru dari Growbot |
| `handleGetHistory()` | Endpoint `/api/history` — mengembalikan 5 data kelembaban terakhir |
| `sendToBlynk()` | Timer 1 detik — mengirim data sensor ke virtual pin Blynk |
| `BLYNK_WRITE(V3)` | Handler kontrol pompa dari tombol di aplikasi Blynk |
| `loop()` | Mengelola OTA, mDNS, Blynk, logika auto mode, dan timeout manual |

---

## Persyaratan

**Perangkat keras:**

- NodeMCU / ESP8266
- Sensor kelembaban tanah (soil moisture sensor) — pin `A0`
- Relay module — pin `D4` (GPIO 2, aktif LOW)
- Pompa air mini DC

**Perangkat lunak:**

- [Arduino IDE](https://www.arduino.cc/en/software) versi 1.8 ke atas (atau Arduino IDE 2.x)
- Board package ESP8266 untuk Arduino IDE
- Library: `Blynk`, `ESP8266WiFi`, `ESP8266WebServer`, `ESP8266mDNS`, `ArduinoOTA`, `EEPROM`
- Akun [Blynk](https://blynk.io) (gratis)

---

## Tahap 1 — Persiapan Arduino IDE

### Instal board ESP8266

1. Buka Arduino IDE → **File → Preferences**.
2. Di kolom **Additional Boards Manager URLs**, tambahkan:
   ```
   https://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
3. Buka **Tools → Board → Boards Manager**, cari `esp8266`, instal paket **ESP8266 by ESP8266 Community**.
4. Pilih board: **Tools → Board → ESP8266 Boards → NodeMCU 1.0 (ESP-12E Module)**.

### Instal library yang dibutuhkan

Buka **Sketch → Include Library → Manage Libraries**, lalu cari dan instal:

| Library | Catatan |
|---|---|
| `Blynk` | Cari "Blynk" oleh Volodymyr Shymanskyy, versi 1.3.x ke atas |
| `ESP8266WiFi` | Sudah termasuk dalam board package ESP8266 |
| `ESP8266WebServer` | Sudah termasuk dalam board package ESP8266 |
| `ESP8266mDNS` | Sudah termasuk dalam board package ESP8266 |
| `ArduinoOTA` | Sudah termasuk dalam board package ESP8266 |
| `EEPROM` | Sudah termasuk dalam board package ESP8266 |

> Library yang sudah termasuk dalam board package tidak perlu diinstal terpisah — akan tersedia otomatis setelah board package ESP8266 terpasang.

---

## Tahap 2 — Setup Blynk

Blynk digunakan untuk memantau dan mengontrol sistem dari aplikasi smartphone.

1. Buat akun di [blynk.io](https://blynk.io) atau unduh aplikasi **Blynk IoT** (bukan Blynk Legacy).
2. Buat **Template** baru di Blynk Console:
   - Nama template: `Growmate` (bebas)
   - Hardware: **ESP8266**
   - Connection type: **WiFi**
3. Di dalam template, buat **Datastream** berikut:

   | Virtual Pin | Nama | Tipe Data | Keterangan |
   |:---:|---|:---:|---|
   | V0 | Soil ADC | Integer | Nilai ADC mentah sensor (0–1024) |
   | V1 | Mode | String | Status mode: Auto / Manual ON / Manual OFF |
   | V2 | Kelembaban % | Integer | Persentase kelembaban (0–100%) |
   | V3 | Kontrol Pompa | Integer | 1 = pompa ON, 0 = pompa OFF |

4. Buat **Device** dari template tersebut, lalu salin:
   - **Template ID** (format: `TMPLxxxxxxxxxx`)
   - **Template Name**
   - **Auth Token**
5. (Opsional) Tambahkan widget di aplikasi Blynk untuk visualisasi:
   - **Gauge** atau **Value Display** → V0, V2
   - **Label** → V1
   - **Button** (mode Switch) → V3

---

## Tahap 3 — Konfigurasi Firmware

Buka file `growmate/growmate.ino` dengan Arduino IDE, lalu ubah bagian-bagian berikut:

### 3.1 Kredensial Blynk

```cpp
#define BLYNK_TEMPLATE_ID   "TMPLxxxxxxxxxx"   // Template ID dari Blynk Console
#define BLYNK_TEMPLATE_NAME "Growmate"          // Nama template yang dibuat
#define BLYNK_AUTH_TOKEN    "xxxxxxxxxxxx"      // Auth Token dari device Blynk
```

### 3.2 Kredensial WiFi

```cpp
const char* ssid     = "NamaWiFiKamu";    // SSID jaringan WiFi 2.4 GHz
const char* password = "PasswordWiFi";    // Password WiFi
```

> ESP8266 hanya mendukung jaringan WiFi **2.4 GHz**. Pastikan tidak menggunakan jaringan 5 GHz.

### 3.3 Konfigurasi pin hardware

Sesuaikan jika menggunakan wiring yang berbeda:

```cpp
const int soilPin  = A0;   // Pin sensor kelembaban (hanya A0 yang tersedia untuk ADC)
const int relayPin = 2;    // GPIO 2 = pin D4 pada NodeMCU, relay aktif LOW
```

### 3.4 Nilai threshold default

```cpp
int threshold = 700;   // Rentang valid: 0–1024
```

Nilai ini hanya berlaku saat pertama kali upload (EEPROM kosong). Setelahnya, threshold yang tersimpan di EEPROM akan digunakan. Threshold dapat diubah sewaktu-waktu dari dashboard web atau bot WhatsApp tanpa perlu upload ulang firmware.

**Panduan nilai threshold:**

| Nilai | Interpretasi |
|:---:|---|
| 200–400 | Target tanah sangat basah |
| 500–700 | Target tanah normal / sedang |
| 700–900 | Target tanah agak kering sebelum disiram |
| 900–1024 | Target tanah sangat kering (sensor di udara terbuka) |

---

## Tahap 4 — Upload ke ESP8266

1. Hubungkan NodeMCU ke komputer menggunakan kabel USB (pastikan kabel mendukung data, bukan hanya charging).
2. Di Arduino IDE, pastikan pengaturan berikut sudah benar:

   | Pengaturan | Nilai |
   |---|---|
   | Board | NodeMCU 1.0 (ESP-12E Module) |
   | Upload Speed | 115200 |
   | Port | Pilih port COM yang muncul |

   > Windows: port biasanya bernama `COM3`, `COM4`, dst. Mac/Linux: `/dev/ttyUSB0` atau `/dev/cu.usbserial-...`

3. Klik tombol **Upload** dan tunggu hingga muncul pesan `Done uploading`.
4. Buka **Serial Monitor** (Tools → Serial Monitor atau Ctrl+Shift+M).
5. Atur baud rate ke **115200**.
6. Setelah terhubung ke WiFi, Serial Monitor akan menampilkan:

   ```
   EEPROM loaded — threshold: 700, presets: 0
   WiFi connected — 192.168.1.X
   mDNS started — http://growmate.local
   System ready — http://192.168.1.X
   ```

7. Catat **IP lokal ESP8266** yang tertera — diperlukan untuk konfigurasi tunnel di tahap berikutnya.
8. Buka browser di perangkat yang terhubung ke WiFi yang sama, akses `http://growmate.local` atau `http://192.168.1.X` untuk memverifikasi dashboard berjalan.

---

## Tahap 5 — Remote Access

Agar [Growbot](https://github.com/hilmyah/Growbot) yang berjalan di cloud dapat berkomunikasi dengan ESP8266 di jaringan lokal, diperlukan salah satu metode tunneling. Jalankan di komputer yang **terhubung ke WiFi yang sama dengan ESP8266** dan biarkan berjalan selama sistem aktif digunakan.

### Opsi A — Cloudflare Tunnel (Direkomendasikan)

Tidak memerlukan akun, URL publik langsung aktif tanpa konfigurasi port forwarding.

**Instalasi `cloudflared`:**

```bash
# macOS
brew install cloudflared

# Windows (via winget)
winget install Cloudflare.cloudflared

# Linux (Debian/Ubuntu)
curl -L https://pkg.cloudflare.com/cloudflared-stable-linux-amd64.deb -o cloudflared.deb
sudo dpkg -i cloudflared.deb
```

**Jalankan tunnel:**

```bash
cloudflared tunnel --url http://192.168.1.X
# Ganti dengan IP ESP8266 dari Serial Monitor
```

URL publik akan muncul di terminal:
```
https://random-name.trycloudflare.com
```

Salin URL ini ke variabel `ESP_URL` di konfigurasi Growbot.

> URL berubah setiap kali `cloudflared` dijalankan ulang. Untuk URL permanen, daftar akun Cloudflare gratis dan buat [Named Tunnel](https://developers.cloudflare.com/cloudflare-one/connections/connect-networks/).

### Opsi B — Tailscale

```bash
# Linux / macOS
sudo tailscale up --advertise-routes=192.168.1.0/24

# Windows (PowerShell sebagai Administrator)
tailscale up --advertise-routes=192.168.1.0/24
```

Setujui route di [Tailscale Admin Console](https://login.tailscale.com/admin/machines), lalu gunakan IP lokal ESP8266 langsung sebagai `ESP_URL` di Growbot.

---

## Penjelasan Kode

### Logika mode otomatis

Di dalam `loop()`, firmware membaca ADC setiap iterasi. Jika mode aktif adalah **Auto** (`systemMode == 0`):

- Pompa **menyala** jika `ADC > threshold + 20` (tanah kering) dan pompa sedang mati.
- Pompa **mati** jika `ADC < threshold - 20` (tanah sudah cukup basah) dan pompa sedang menyala.

Histeresis ±20 mencegah pompa menyala-mati terlalu cepat (relay chattering).

### Timeout manual

Saat mode **Manual ON** aktif, timer mulai berjalan. Setelah 60 detik, sistem otomatis kembali ke mode Auto untuk mencegah pompa terus menyala tanpa pengawasan.

```cpp
const long manualTimeout = 60000;   // 60 detik dalam milidetik
```

### Penyimpanan EEPROM

| Alamat | Data | Ukuran |
|:---:|---|:---:|
| `0–1` | Nilai threshold (int) | 2 byte |
| `2` | Jumlah preset tersimpan (byte) | 1 byte |
| `3–N` | Data preset: 13 byte nama + 2 byte threshold = 15 byte/slot | maks 150 byte |

Total EEPROM yang digunakan: 153 byte dari 512 byte yang dialokasikan.

### Koneksi Blynk non-blocking

Blynk dijalankan secara non-blocking: jika koneksi ke server Blynk terputus, sistem mencoba reconnect setiap 10 detik tanpa menghentikan web server atau pembacaan sensor.

```cpp
} else if (millis() - lastBlynkReconnect > 10000) {
  lastBlynkReconnect = millis();
  Blynk.connect(3000);
}
```

---

## API Endpoint ESP8266

Endpoint berikut dapat diakses langsung dari browser, curl, atau digunakan oleh Growbot.

| Method | Endpoint | Keterangan |
|:---:|---|---|
| GET | `/` | Halaman HTML dashboard |
| GET | `/api/data` | Status lengkap sistem (JSON) |
| GET | `/api/threshold?val=700` | Ubah nilai threshold |
| GET | `/api/presets` | Ambil daftar preset tanaman (JSON array) |
| POST | `/api/presets` | Simpan preset baru (JSON body) |
| GET | `/api/history` | 5 data ADC terakhir (JSON array) |
| GET | `/on` | Nyalakan pompa — Manual ON |
| GET | `/off` | Matikan pompa — Manual OFF |
| GET | `/auto` | Aktifkan mode otomatis |

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
  <sub>Growmate · Tugas PKK · 2025 &nbsp;|&nbsp; <a href="https://github.com/hilmyah/Growbot">Growbot — WhatsApp Gateway</a></sub>
</div>