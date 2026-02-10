# LED Testing Information Laboratory

Proyek ini merupakan sistem **running text LED Matrix P5** berbasis **ESP32**, dilengkapi antarmuka web untuk mengatur teks, warna, kecerahan, dan kecepatan secara nirkabel (WiFi).

## Gambaran Umum
Sistem memungkinkan pengguna:
* Mengubah teks running text melalui browser
* Mengatur warna setiap baris teks
* Mengatur tingkat kecerahan
* Mengatur kecepatan animasi teks
  Semua pengaturan dilakukan melalui **web interface** yang di-host langsung oleh ESP32.

## Cara Mengubah Teks Running Text
1. Hubungkan perangkat (HP/Laptop) ke WiFi running text:
   * **SSID**: (sesuai yang diset pada ESP32)
   * **Password**: (sesuai konfigurasi)
2. Setelah terhubung, biasanya browser akan otomatis terbuka menuju halaman pengaturan.
3. Jika tidak otomatis:
   * Buka browser
   * Masukkan alamat berikut pada address bar:

     ```
     192.168.4.1
     ```

4. Antarmuka web akan menampilkan beberapa kolom pengaturan:
   * Isi teks running text
   * Warna teks
   * Kecerahan layer
   * Kecepatan running text
5. Klik tombol **Submit** untuk menerapkan perubahan.

## Komponen Utama dan Fungsinya
### P5 Matrix Panel Module
Digunakan sebagai media display teks running.
### ESP32 DevKit V1
Berfungsi sebagai pengendali utama LED Matrix sekaligus web server.
### Power Supply 5V 10A
Menyuplai daya untuk seluruh sistem.

## Skematik dan Koneksi
* Skematik tersedia pada (./Schematics/HUB75-to-ESP32.pdf)
* Modul P5 menggunakan konektor **HUB75**
  Pastikan ESP32 dihubungkan ke **port input** (sebelah kiri, berlawanan arah panah pada modul).
* Konfigurasi pin mengikuti library:
  **ESP32-HUB75-MatrixPanel-DMA**

### Pin Configuration
(https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA?tab=readme-ov-file#2-wiring-the-esp32-to-an-led-matrix-panel)
```cpp
#define R1_PIN 25
#define G1_PIN 26
#define B1_PIN 27
#define R2_PIN 14
#define G2_PIN 12
#define B2_PIN 13

#define A_PIN 23
#define B_PIN 19
#define C_PIN 5
#define D_PIN 17
#define E_PIN -1   // Digunakan untuk panel 1/32 scan (mis. 64x64), bisa menggunakan pin bebas

#define LAT_PIN 4
#define OE_PIN 15
#define CLK_PIN 16
```

## Upload dan Update Program ke ESP32
Digunakan untuk update firmware atau troubleshooting.
1. Pastikan salah satu sudah terpasang:
   * **Arduino IDE**, atau
   * **VS Code + PlatformIO**
2. Hubungkan ESP32 ke komputer menggunakan kabel **USB Type-C**.
3. Saat proses upload berjalan dan muncul pesan:
   ```
   connecting...
   ```
   tekan dan tahan tombol **BOOT** pada ESP32 selama beberapa detik, lalu lepaskan.
4. Tunggu hingga proses upload selesai.


## Catatan Teknis
* Gunakan power supply yang memadai untuk menghindari flicker atau restart ESP32.
* Pastikan koneksi HUB75 tidak terbalik (input vs output).
* Library ESP32-HUB75-MatrixPanel-DMA wajib sesuai dengan konfigurasi panel yang digunakan.
