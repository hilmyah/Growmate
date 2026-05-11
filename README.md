# Growmate — Smart Irrigation System

Growmate adalah sistem irigasi cerdas berbasis ESP8266 yang memungkinkan pemantauan dan kontrol kelembaban tanah melalui Web Dashboard, aplikasi Blynk, dan bot WhatsApp. Sistem ini dirancang untuk otomasi penyiraman tanaman dengan fitur akses jarak jauh menggunakan Cloudflare Tunnel.

## Deskripsi Kode dalam Repository

Repository ini berisi komponen utama sebagai berikut:

1. 
**`growmate_with_lightmode.ino`**: Firmware untuk ESP8266 yang menangani pembacaan sensor kelembaban (ADC), kontrol relay pompa, logika mode otomatis, integrasi Blynk, mDNS (`growmate.local`), dan fitur Over-the-Air (OTA) update.


2. **`portal.html`**: Halaman landing statis yang berfungsi sebagai direktori tautan proyek (Instagram, TikTok, YouTube, GitHub, dan Live Dashboard) untuk memudahkan akses pengguna melalui QR Code.
3. **`panduan_lengkap.md`**: Dokumentasi teknis mengenai cara kerja sistem secara keseluruhan, termasuk setup bot WhatsApp dan konfigurasi akses jarak jauh.

## Persyaratan Perangkat Keras

* NodeMCU / ESP8266.
* Sensor Kelembaban Tanah (Soil Moisture Sensor) pada pin A0.


* Relay Module pada pin D4 (GPIO 2) untuk kontrol pompa.


* Pompa Air Mini DC.

## Panduan Instalasi dan Penggunaan

### 1. Persiapan Firmware

1. Buka file `growmate_with_lightmode.ino` menggunakan Arduino IDE.
2. Pastikan library `Blynk`, `ESP8266WiFi`, dan `ESP8266mDNS` sudah terinstal.
3. Sesuaikan kredensial WiFi (`ssid`, `password`) dan `BLYNK_AUTH_TOKEN` pada kode.


4. Lakukan upload ke board ESP8266.

### 2. Konfigurasi Remote Access (Cloudflare Tunnel)

Untuk mengakses dashboard dari luar jaringan lokal tanpa port forwarding:

1. Instal `cloudflared` pada perangkat yang berada di jaringan yang sama dengan ESP8266.
2. Jalankan perintah: `cloudflared tunnel --url http://[IP_LOKAL_ESP8266]`.
3. Gunakan URL `.trycloudflare.com` yang dihasilkan untuk mengakses dashboard secara publik.

### 3. Kontrol melalui WhatsApp Bot

Sistem ini menggunakan server Node.js sebagai perantara antara Fonnte (WhatsApp API) dan ESP8266.

1. Daftar di Fonnte dan dapatkan API Token.
2. Deploy kode bot (Node.js) ke platform seperti Railway atau Render.
3. Atur environment variable `FONNTE_TOKEN` dan `ESP_URL` (URL dari Cloudflare Tunnel).
4. Hubungkan Webhook Fonnte ke URL server bot yang telah di-deploy.

---