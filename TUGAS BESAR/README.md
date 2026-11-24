# AirWatch IoT — single-file dashboard + ESP32 + Vercel API

## Isi project
- `index.html`: seluruh HTML, CSS, dan JavaScript dashboard dalam satu file.
- `api/telemetry.js`: Vercel Function untuk menerima POST ESP32 dan menyediakan GET data terbaru.
- `esp32/airwatch_esp32.ino`: contoh firmware ESP32 (DHT22 + PMS5003 + MQ-135).
- `supabase-schema.sql`: tabel penyimpanan telemetry.

## Deploy ke Vercel
1. Upload/commit seluruh isi folder project ini ke root repository GitHub. Pastikan folder `api/` ikut ter-upload.
2. Import repository tersebut di Vercel. Framework Preset: `Other`; Root Directory: `./`; Build Command kosong; Output Directory: `.`.
3. Buat project Supabase, buka SQL Editor, jalankan isi `supabase-schema.sql`.
4. Di Vercel > Project > Settings > Environment Variables, tambahkan:
   - `SUPABASE_URL` = Project URL Supabase.
   - `SUPABASE_SERVICE_ROLE_KEY` = service_role key Supabase (RAHASIA, jangan dimasukkan ke HTML atau firmware).
   - `DEVICE_API_TOKEN` = token acak panjang buatan sendiri.
5. Redeploy project di Vercel setelah environment variables disimpan.
6. Uji `https://NAMA-PROJECT.vercel.app/api/telemetry` — sebelum ESP32 mengirim data, respons normalnya `{"data":null}`.

## Firmware ESP32
1. Install library Arduino: `DHT sensor library` by Adafruit, `Adafruit Unified Sensor`, dan `ArduinoJson`.
2. Buka `esp32/airwatch_esp32.ino`. Isi SSID, password Wi-Fi, API URL Vercel, dan DEVICE_TOKEN.
3. Wiring asumsi: DHT22 data GPIO4; PMS5003 TX ke GPIO16 (RX2), RX ke GPIO17 (opsional); MQ-135 AO ke GPIO34. Semua GND harus tersambung. Pastikan output analog MQ-135 tidak melebihi 3.3V pada pin ESP32.
4. Upload ke board ESP32. Buka Serial Monitor 115200 baud. Data dikirim tiap 10 detik.

## Cara kerja dashboard
- Dashboard memeriksa `/api/telemetry` setiap 5 detik. Jika data perangkat yang diterima berumur kurang dari 60 detik, data aktual ditampilkan. Jika tidak ada, dashboard kembali memakai simulasi lokal.
- `Hapus data simulasi` mengosongkan data grafik/nilai yang sedang tersimpan di memori browser. `Hapus history simulasi` mengosongkan daftar alert pada browser. Keduanya tidak menghapus data permanen di Supabase.
- Tombol mode simulasi tetap tersedia untuk pengujian tampilan.

## Catatan akurasi dan keamanan
- AQI pada contoh firmware dihitung sebagai estimasi dari PM2.5 dengan interpolasi breakpoint; bukan hasil alat AQI tersertifikasi. MQ-135 hanya dikirim sebagai nilai ADC mentah (`gas_raw`), bukan ppm. Sensor perlu kalibrasi dan pengkondisian yang benar untuk angka ilmiah.
- Firmware memakai `WiFiClientSecure::setInsecure()` untuk mempermudah prototipe. Untuk produksi, pasang verifikasi sertifikat TLS.
- Jangan commit Wi-Fi password atau token API ke repository publik. Simpan rahasia dan ganti token jika pernah terlanjur dipublikasikan.
- Endpoint GET bersifat publik pada contoh ini dan hanya mengembalikan pembacaan terbaru. Endpoint POST dilindungi `DEVICE_API_TOKEN`.
