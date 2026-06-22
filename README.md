<div align="center">
  <img src="asset/growmate1.png" alt="Growmate Logo" width="180"/>
  <h1>Growmate</h1>
  <p>Sistem irigasi cerdas berbasis WEMOS D1 Mini / ESP8266 dengan Web Dashboard, LCD, Blynk, dan bot WhatsApp & Telegram.</p>
  <p>
    <a href="https://github.com/hilmyah/Growbot">Growbot - WhatsApp & Telegram Gateway</a>
  </p>
</div>

<p align="center">
  <img src="https://img.shields.io/badge/Platform-ESP8266%20%7C%20WEMOS%20D1%20Mini-orange?logo=espressif&logoColor=white" alt="Platform">
  <img src="https://img.shields.io/badge/Framework-Arduino%20IDE-00979D?logo=arduino&logoColor=white" alt="Framework">
  <img src="https://img.shields.io/badge/IoT-Blynk-04D683?logo=blynk&logoColor=white" alt="IoT Services">
</p>

---

Growmate adalah firmware untuk WEMOS D1 Mini / ESP8266 yang mengotomasi penyiraman tanaman berdasarkan pembacaan sensor kelembaban tanah. Status sistem ditampilkan secara real-time di layar LCD I2C yang terpasang langsung pada modul. Sistem dapat diakses melalui tiga antarmuka sekaligus: Web Dashboard yang berjalan langsung di ESP8266, aplikasi Blynk di smartphone, dan bot WhatsApp & Telegram melalui [Growbot](https://github.com/hilmyah/Growbot).

## Daftar Isi

- [Fitur](#fitur)
- [Konsep dan Arsitektur](#konsep-dan-arsitektur)
- [Struktur Repository](#struktur-repository)
- [Prasyarat](#prasyarat)
- [Instalasi](#instalasi)
- [Konfigurasi Firmware](#konfigurasi-firmware)
- [Referensi API Endpoint](#referensi-api-endpoint)
- [Peta Memori EEPROM](#peta-memori-eeprom)
- [Manajemen dan Operasional](#manajemen-dan-operasional)
- [Penjelasan Kode](#penjelasan-kode)
- [Troubleshooting](#troubleshooting)
- [Keamanan](#keamanan)
- [Lisensi](#lisensi)

---

## Fitur

| Fitur | Deskripsi |
| --- | --- |
| Otomasi & Mode Dual | Penyiraman otomatis berdasarkan threshold kelembaban tanah (hysteresis +/-20 ADC), atau eksekusi manual via Web/Blynk/Bot dengan timeout pengaman 60 detik. |
| Multi-Interface Monitor | LCD 16x2 fisik (3 halaman bergantian tiap 3 detik: kelembaban, mode & pompa, threshold & counter) dan Web Dashboard lokal. |
| Integrasi Cloud Blynk | Kontrol non-blocking jarak jauh via 4 virtual pin (V0 ADC, V1 Mode, V2 persentase, V3 kontrol pompa). Reconnect otomatis tiap 10 detik bila terputus, tanpa mengganggu web server atau LCD. |
| Kompatibilitas Gateway | REST API lokal berformat JSON yang diintegrasikan dengan Growbot untuk interaksi via WhatsApp dan Telegram. |
| Jadwal Penyiraman | Interval penyiraman terjadwal (satuan menit, maksimum 10.080 menit / 1 minggu), persisten di EEPROM, dapat diatur dari dashboard web maupun bot Growbot. |
| Light / Dark Mode | Dashboard web dengan toggle tema terang dan gelap. Tema default saat pertama dibuka adalah dark; pilihan tersimpan di localStorage browser. |
| Grafik Historis | Chart.js menampilkan hingga 40 titik data kelembaban, diakumulasi di sisi browser melalui polling `/api/data` setiap 2 detik. Riwayat ini direset setiap halaman dimuat ulang dan berbeda dari endpoint `/api/history` (lihat Referensi API Endpoint). |
| Preset Tanaman | 10 preset bawaan tertanam di dashboard (Cabai, Tomat, Kangkung, Bayam, Padi, Jagung, Bawang, Kentang, Kaktus, Stroberi), ditambah maksimum 10 preset kustom yang tersimpan di EEPROM. |
| Persistensi EEPROM | Threshold, preset kustom, dan jadwal tersimpan dan tidak hilang saat restart atau mati listrik. |
| mDNS | Akses dashboard via `http://growmate.local` tanpa perlu mengingat IP. |
| OTA Update | Upload firmware baru tanpa kabel melalui jaringan WiFi (hostname `Wemos-Growmate`). |

---

## Konsep dan Arsitektur

Firmware berjalan menggunakan penjadwalan fungsi modular (BlynkTimer, serta pengecekan kondisi berbasis `millis()` non-blocking) untuk menjamin stabilitas server HTTP lokal tanpa terinterupsi oleh kegagalan jaringan luar.

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
| Pompa    |   |  |   (Relay/PWM)    |   | EEPROM / RTC  |  |   |   Service   |
+----------+   |  +------------------+   +---------------+  |   +-------------+
               +--------------------------------------------+
```

---

## Struktur Repository

```
Growmate/
├── growmate/
│   └── growmate.ino      Firmware utama ESP8266 (sketch folder, dipakai Arduino IDE untuk compile/upload).
├── growmate.ino           Duplikat berkas firmware di root repository (lihat catatan pada Keamanan).
├── asset/
│   ├── growmate.png       Logo proyek (ikon).
│   └── growmate1.png      Logo proyek (full).
├── index.html              Halaman link proyek (portal publik / QR pamflet).
├── .vscode/                Konfigurasi editor (intellisense, build task), tidak memengaruhi firmware.
└── README.md
```

Catatan: Arduino IDE mensyaratkan nama folder sketch sama dengan nama berkas `.ino` di dalamnya, sehingga `growmate/growmate.ino` adalah berkas yang dikompilasi dan diupload. Berkas `growmate.ino` di root repository merupakan duplikat dan tidak digunakan langsung oleh Arduino IDE kecuali folder repository sendiri dibuka sebagai sketch.

---

## Prasyarat

### Perangkat Keras (Hardware)

| Komponen | Pin |
| --- | --- |
| WEMOS D1 Mini (ESP8266) | - |
| Sensor Kelembaban Tanah Kapasitif / Resistif | `A0` |
| Modul Relay 5V + Pompa Air DC | `D2` / GPIO4, aktif HIGH |
| LCD I2C 16x2 | SDA (`D4`) - SCL (`D5`) |
| Kabel Jumper & Breadboard | - |

ESP8266 hanya mendukung WiFi 2.4 GHz. Pastikan tidak menggunakan jaringan 5 GHz.

### Perangkat Lunak dan Pustaka

- Arduino IDE 1.8+ atau 2.x
- Board package: **ESP8266 by ESP8266 Community**, ditambahkan melalui Additional Boards Manager URL `https://arduino.esp8266.com/stable/package_esp8266com_index.json`

| Nama Pustaka | Sumber / Versi | Fungsi |
| --- | --- | --- |
| `ESP8266WiFi` | Bawaan board | Manajemen konektivitas WiFi |
| `ESP8266WebServer` | Bawaan board | Penyedia layanan REST API dan Web Dashboard |
| `ESP8266mDNS` | Bawaan board | Akses dashboard via `growmate.local` |
| `ArduinoOTA` | Bawaan board | Update firmware over-the-air |
| `EEPROM` | Bawaan board | Persistensi konfigurasi |
| `Wire` | Built-in | Komunikasi I2C ke LCD |
| `Blynk` | Library Manager (Volodymyr Shymanskyy), terbaru | Komunikasi data duplex ke server cloud Blynk |
| `LiquidCrystal I2C` | Library Manager (Frank de Brabander), terbaru | Driver LCD 16x2 |
| `ArduinoJson` | v6.x / v7.x | Hanya diperlukan apabila menambah parsing JSON kustom; firmware saat ini membangun string JSON secara manual tanpa pustaka ini. |

- Akun [Blynk](https://blynk.io) (gratis)

---

## Instalasi

### 1. Persiapan Arduino IDE

1. Buka Arduino IDE -> **File -> Preferences**, tambahkan URL board manager di atas pada kolom **Additional Boards Manager URLs**.
2. Buka **Tools -> Board -> Boards Manager**, cari `esp8266`, instal **ESP8266 by ESP8266 Community**.
3. Pilih board: **Tools -> Board -> ESP8266 Boards -> LOLIN(WEMOS) D1 R2 & Mini**.
4. Instal pustaka `Blynk` dan `LiquidCrystal I2C` melalui **Sketch -> Include Library -> Manage Libraries**.

### 2. Setup Template Blynk

1. Buat akun di [blynk.io](https://blynk.io), buat **Template** baru (Hardware: ESP8266, Connection: WiFi).
2. Buat **Datastream** berikut pada template:

   | Virtual Pin | Nama | Tipe | Keterangan |
   | :---: | --- | :---: | --- |
   | V0 | Soil ADC | Integer | Nilai ADC mentah (0-1024) |
   | V1 | Mode | String | Auto / Manual ON / Manual OFF |
   | V2 | Kelembaban % | Integer | Persentase kelembaban (0-100%) |
   | V3 | Kontrol Pompa | Integer | 1 = ON, 0 = OFF |

3. Buat **Device** dari template, salin **Template ID**, **Template Name**, dan **Auth Token**.

### 3. Clone Repository

```bash
git clone https://github.com/hilmyah/Growmate.git
```

Buka folder `growmate/` (bukan berkas `growmate.ino` di root) sebagai sketch di Arduino IDE.

---

## Konfigurasi Firmware

Buka `growmate/growmate.ino`, sesuaikan parameter berikut sebelum flashing. Jangan commit nilai asli kredensial ke repository, gunakan placeholder seperti contoh di bawah lalu isi nilai sebenarnya hanya secara lokal:

```cpp
// Kredensial Blynk
#define BLYNK_TEMPLATE_ID   "BLYNK_TEMPLATE_ID_ANDA"
#define BLYNK_TEMPLATE_NAME "BLYNK_TEMPLATE_NAME_ANDA"
#define BLYNK_AUTH_TOKEN    "BLYNK_AUTH_TOKEN_ANDA"

// Kredensial WiFi
const char* ssid     = "NAMA_WIFI_ANDA";
const char* password = "PASSWORD_WIFI_ANDA";

// Pin dan LCD
const int soilPin  = A0;
const int relayPin = D2;                // GPIO4, aktif HIGH
LiquidCrystal_I2C lcd(0x27, 16, 2);    // Coba 0x3F jika LCD tidak tampil

// Threshold default (hanya berlaku saat EEPROM kosong / pertama kali boot)
int threshold = 700;   // rentang 200-1024; rendah = basah, tinggi = kering
```

---

## Referensi API Endpoint

ESP8266 menjalankan server HTTP lokal pada port 80. Endpoint berikut dapat diakses oleh browser atau dihubungkan ke server perantara (Growbot Gateway). Tidak ada endpoint yang memerlukan autentikasi, lihat bagian Keamanan.

| Method | Endpoint | Fungsi |
| --- | --- | --- |
| GET | `/` | Memuat antarmuka web dashboard berbasis HTML grafis. |
| GET | `/api/data` | Status terkini sistem (ADC, status pompa, mode aktif, threshold, jadwal) dalam format JSON. |
| GET | `/api/threshold?val=[nilai]` | Mengubah nilai threshold otomatis dan menyimpannya ke EEPROM. |
| GET | `/api/presets` | Daftar preset kustom tersimpan di EEPROM. Format setiap item: `{"n": "nama", "t": nilaiThreshold}`. |
| POST | `/api/presets` | Menyimpan array JSON preset kustom ke EEPROM, format sama seperti respons GET, maksimum 10 slot. |
| GET | `/api/history` | Array 5 nilai ADC terakhir dari buffer internal (`moistureHistory`), dipakai Growbot untuk perintah riwayat. Terpisah dari grafik 40 titik pada dashboard web yang diakumulasi di sisi browser. |
| GET | `/api/schedule?min=[total_menit]&en=[0/1]` | Mengonfigurasi jadwal siram terjadwal. `min` dalam satuan menit (maksimum 10080), `en=0` menonaktifkan jadwal. |
| GET | `/on` | Memaksa pompa aktif secara manual, mode berubah ke MANUAL ON, otomatis kembali ke AUTO setelah 60 detik. |
| GET | `/off` | Memaksa pompa mati secara manual, mode berubah ke MANUAL OFF. |
| GET | `/auto` | Mengembalikan kontrol pompa ke logika otomatis berbasis sensor. |

Contoh respons `/api/data`:

```json
{
  "adc": 732,
  "kondisi": "KERING",
  "pump": "ON",
  "mode": "AUTO",
  "threshold": 700,
  "lastWatered": 143,
  "count": 3,
  "schedEnabled": true,
  "schedIntervalMin": 720,
  "schedElapsedMin": 45
}
```

---

## Peta Memori EEPROM

Penyimpanan parameter konfigurasi menggunakan tata letak alokasi byte berikut, total ukuran EEPROM yang diinisialisasi 512 byte:

| Alokasi Byte | Parameter | Ukuran Data |
| --- | --- | --- |
| `0-1` | Threshold otomatis (int) | 2 byte |
| `2` | Jumlah slot preset kustom terisi (byte) | 1 byte |
| `3-152` | Blok data preset kustom (maksimum 10 slot x 15 byte: 13 byte nama + 2 byte threshold) | 150 byte |
| `163` | Status jadwal aktif (`schedEnabled`, 1 = aktif) | 1 byte |
| `164-165` | Interval jadwal dalam menit (`uint16`, maksimum 10080) | 2 byte |

---

## Manajemen dan Operasional

1. Sambungkan WEMOS D1 Mini ke komputer via kabel USB Micro-B.
2. Di Arduino IDE: `Tools -> Board -> ESP8266 Boards -> LOLIN(WEMOS) D1 R2 & Mini`.
3. Set `Upload Speed` ke `115200`, pilih port COM yang sesuai pada `Tools -> Port`.
4. Tekan **Verify** untuk memastikan kompilasi berhasil, lalu **Upload**.
5. Buka **Serial Monitor** pada baud rate `115200` untuk memantau proses koneksi:
   ```
   WiFi connected - 192.168.1.X
   mDNS started - http://growmate.local
   System ready - http://192.168.1.X
   ```
6. Akses dashboard via `http://growmate.local` atau IP yang tercatat, dari perangkat pada jaringan WiFi yang sama.

### Remote Access untuk Growbot

Agar [Growbot](https://github.com/hilmyah/Growbot) yang berjalan di cloud dapat menjangkau ESP8266 di jaringan lokal, jalankan salah satu tunnel berikut di komputer yang satu jaringan WiFi dengan ESP8266:

```bash
# Cloudflare Tunnel (direkomendasikan)
cloudflared tunnel --url http://192.168.1.X

# Tailscale (alternatif, IP lokal tetap dipakai sebagai ESP_URL)
sudo tailscale up --advertise-routes=192.168.1.0/24
```

URL atau IP hasil tunnel disalin ke variabel `ESP_URL` pada konfigurasi Growbot.

---

## Penjelasan Kode

### LCD paging

LCD diperbarui tiap 1 detik secara non-blocking. Tiga halaman bergantian setiap 3 detik:

| Halaman | Baris 1 | Baris 2 |
| :---: | --- | --- |
| 1 | `Tanah: BASAH` | `ADC: 412   60%` |
| 2 | `Mode: AUTO` | `Pompa: OFF` |
| 3 | `Batas: 700` | `Disiram:   0 kali` |

### Mode otomatis vs status tampilan

Dua ambang batas berbeda dipakai untuk dua tujuan berbeda:

```cpp
// Kontrol aktual pompa pada mode AUTO (hysteresis +/-20)
if (raw > threshold + 20 && !isPumpOn) { pumpOn(); }
if (raw < threshold - 20 &&  isPumpOn) { pumpOff(); }

// Label status BASAH/NORMAL/KERING pada LCD dan dashboard (band +/-50)
if      (sensorValue < threshold - 50) kondisi = "BASAH";
else if (sensorValue < threshold + 50) kondisi = "NORMAL";
else                                    kondisi = "KERING";
```

Hysteresis +/-20 pada kontrol pompa mencegah relay chattering. Band +/-50 pada label status lebih lebar karena hanya untuk keperluan tampilan, tidak menggerakkan aktuator.

### Jadwal penyiraman

```cpp
// Contoh: jadwal setiap 720 menit (12 jam)
// GET /api/schedule?min=720&en=1
if (schedEnabled && millis() - lastSchedWater >= schedIntervalMs) {
  // aktifkan pompa selama 60 detik, lalu kembali ke mode Auto
}
```

Interval disimpan dalam satuan menit (`uint16`, maksimum 10.080 = 1 minggu). Saat jadwal terpicu, sistem dipaksa ke mode MANUAL ON selama 60 detik melalui mekanisme timeout manual yang sama, kemudian otomatis kembali ke mode AUTO.

### Timeout manual

```cpp
const long manualTimeout = 60000;   // pompa mati otomatis setelah 60 detik
```

### Penyimpanan EEPROM

Lihat tabel lengkap pada bagian Peta Memori EEPROM.

### Blynk non-blocking

Jika koneksi ke server Blynk terputus, sistem mencoba reconnect setiap 10 detik tanpa menghentikan web server, pembacaan sensor, atau tampilan LCD.

---

## Troubleshooting

**LCD menampilkan karakter acak atau tidak tampil sama sekali**

Pastikan VCC LCD terhubung ke pin 5V WEMOS, bukan 3.3V. Jika tidak tampil sama sekali, coba ubah alamat I2C dari `0x27` ke `0x3F` pada baris `LiquidCrystal_I2C lcd(...)`. Gunakan sketch `i2c_scanner` terpisah untuk menemukan alamat yang benar apabila kedua alamat tersebut tidak berhasil.

**ESP8266 tidak terhubung ke WiFi**

ESP8266 hanya mendukung jaringan 2.4 GHz. Periksa apakah SSID yang dimasukkan adalah jaringan 2.4 GHz, terutama pada router dual-band yang memisahkan SSID 2.4 GHz dan 5 GHz, atau yang menggunakan SSID sama untuk keduanya.

---

## Keamanan

- Tidak ada autentikasi pada endpoint HTTP apa pun (`/api/*`, `/on`, `/off`, `/auto`). Siapa pun yang dapat menjangkau IP lokal ESP8266, atau URL tunnel publik yang dipakai Growbot, dapat mengontrol pompa dan mengubah konfigurasi secara langsung tanpa melalui WhatsApp/Telegram/Blynk.
- Kredensial WiFi dan Blynk Auth Token didefinisikan langsung sebagai konstanta di dalam `.ino`. Repository ini tidak memiliki `.gitignore`, sehingga tidak ada mekanisme bawaan yang mencegah berkas berisi kredensial asli ikut ter-commit. Disarankan memisahkan kredensial ke header terpisah (misalnya `secrets.h`) yang dimasukkan ke `.gitignore`, dan memverifikasi riwayat commit repository untuk memastikan tidak ada token atau password asli yang pernah terpublikasi. Apabila pernah terpublikasi, token Blynk dan password WiFi yang bersangkutan sebaiknya dirotasi/diganti.
- Endpoint `/api/presets` (POST) menerima dan mem-parsing body JSON secara manual tanpa pustaka parser, dengan asumsi format input mengikuti pola yang sama seperti respons GET. Input di luar format tersebut akan diabaikan oleh parser, bukan menyebabkan crash, namun tetap disarankan untuk tidak mengekspos endpoint ini ke jaringan publik tanpa lapisan proteksi tambahan (misalnya autentikasi di sisi tunnel/proxy).

---

## Lisensi

Repository ini tidak memiliki berkas `LICENSE`. Status lisensi belum dideklarasikan secara resmi.

---

<div align="center">
  <sub>Growmate - Tugas PKK &nbsp;|&nbsp; <a href="https://github.com/hilmyah/Growbot">Growbot - WhatsApp & Telegram Gateway</a></sub>
</div>
