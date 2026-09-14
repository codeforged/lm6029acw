/*
 * basic_demo.js - Contoh paling dasar: bingkai, teks, lingkaran, kotak.
 *
 * Script ini sengaja meng-import lewat '../index.js' (entry point paket),
 * jadi sekaligus jadi contoh cara pakai modul ini dari project lain.
 *
 * Jalankan dari root project:
 *   node examples/basic_demo.js
 *   npm run demo
 */

const { LM6029LCD } = require('../index.js');

const lcd = new LM6029LCD();

if (!lcd.begin()) {
    console.error('Gagal inisialisasi layar (cek /dev/spidev0.0).');
    process.exit(1);
}

lcd.setContrast(40);   // 0 = pudar, 63 = paling tajam (lihat examples/contrast_test.js)
lcd.setBacklight(true);
lcd.clearDisplay();

// Bingkai luar (Box)
lcd.drawRect(0, 0, 128, 64, 1);

// Cetak Teks bawaan Adafruit_GFX (setCursor + print)
lcd.setTextColor(1);
lcd.setTextSize(1);
lcd.setCursor(18, 10);
lcd.print('RASPBERRY PI 2B');

lcd.drawLine(10, 22, 118, 22, 1);

// Gambar Lingkaran dan Kotak Terisi (Solid)
lcd.fillCircle(30, 42, 10, 1);
lcd.fillRect(75, 32, 20, 20, 1);

// Flush ke LCD fisik
lcd.display();
console.log('Render grafis & teks berhasil!');
