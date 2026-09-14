const { LM6029LCD } = require('../build/Release/lcd_lm6029.node');

// 1. Inisialisasi Layar / Hardware
// Penting: driver ini memakai SPI hardware (/dev/spidev0.0) + 2x 74HC595,
// jadi pin TIDAK dikonfigurasi dari JavaScript. begin() tanpa argumen.
const lcd = new LM6029LCD();

if (!lcd.begin()) {
    console.error('Gagal membuka /dev/spidev0.0. Pastikan SPI sudah di-enable (raspi-config).');
    process.exit(1);
}

lcd.setContrast(40);       // 0 = pudar, 63 = paling tajam
lcd.setBacklight(false);

console.log('--- Memulai Pengujian Adafruit GFX ---');
console.log(`Ukuran layar: ${lcd.getWidth()}x${lcd.getHeight()}`);

// Skenario 1: Dasar Bentuk (Shapes)
console.log('1. Menggambar Bentuk Geometri Dasar...');
lcd.clear();

// Kotak & Kotak Terisi
lcd.drawRect(5, 5, 30, 20, 1);       // drawRect(x, y, w, h, color)
lcd.fillRect(40, 5, 30, 20, 1);      // fillRect(x, y, w, h, color)

// Lingkaran & Lingkaran Terisi
lcd.drawCircle(85, 15, 10, 1);       // drawCircle(x, y, radius, color)
lcd.fillCircle(110, 15, 10, 1);      // fillCircle(x, y, radius, color)

// Segitiga & Garis
lcd.drawLine(0, 32, 128, 32, 1);     // drawLine(x0, y0, x1, y1, color)
lcd.drawTriangle(10, 60, 25, 40, 40, 60, 1); // drawTriangle(x0,y0, x1,y1, x2,y2, color)

lcd.display(); // Kirim data buffer ke layar
sleep(2000);

// Skenario 2: Teks Default (System Font 5x7)
console.log('2. Menguji Teks Default (glcdfont)...');
lcd.clear();
lcd.setTextColor(1);   // piksel nyala (mode opaque: bg = text = nyala)
lcd.setTextSize(1);
lcd.setCursor(0, 0);
lcd.print('Adafruit_GFX Node!');

lcd.setTextSize(2);
lcd.setCursor(0, 15);
lcd.print("Size 2x");

lcd.display();
sleep(2000);

// Skenario 3: Inversi Warna & Piksel Individual
console.log('3. Menguji Inversi Warna & Modifikasi Piksel...');
lcd.clear();
// Latar putih penuh (semua piksel nyala)
lcd.fillScreen(1);

// Teks "gelap" di atas latar nyala: teks = 0 (mati), background = 1 (nyala)
lcd.setTextColor(0, 1);
lcd.setTextSize(1);
lcd.setCursor(10, 25);
lcd.print('INVERTED TEXT');

// Gambar piksel acak / pola silang
for (let i = 0; i < 128; i += 4) {
    lcd.drawPixel(i, 0, 0);
    lcd.drawPixel(i, 63, 0);
}

lcd.display();
sleep(2000);

// Skenario 4: Font Adafruit kustom
console.log('4. Menguji Font Adafruit kustom...');
lcd.clear();
lcd.setFont(1);        // 1 = FreeSans9pt7b, 2 = FreeSansBold12pt7b, 3 = FreeMono9pt7b
lcd.setTextColor(1);
lcd.setCursor(0, 12);
lcd.print('FreeSans 9pt');
lcd.setFont(2);
lcd.setCursor(0, 40);
lcd.print('Bold 12pt');
lcd.display();
sleep(2000);

// Skenario 5: Selesai
console.log('5. Pengujian Selesai. Bersihkan layar.');
lcd.setFont(0); // kembali ke font default
lcd.clear();
lcd.display();

// Helper delay sederhana
function sleep(ms) {
    const date = Date.now();
    let currentDate = null;
    do {
        currentDate = Date.now();
    } while (currentDate - date < ms);
}
