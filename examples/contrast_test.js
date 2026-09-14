/*
 * contrast_test.js - Tuning kontras LCD LM6029ACW (EVR 6-bit, 0..63)
 *
 * Cara pakai:
 *   node contrast_test.js              -> sweep kontras 0..63 otomatis
 *   node contrast_test.js 40           -> set kontras 40, lalu tampilkan pola uji
 *   node contrast_test.js 40 --invert  -> plus inversi tampilan
 *   node contrast_test.js 40 --no-bl   -> plus matikan backlight
 *
 * Pola uji memakai dithering Bayer 4x4 supaya terbentuk 8 tingkat "abu-abu"
 * pada layar 1-bit. Kalau antar band masih kelihatan jelas batasnya di
 * kontras tertentu, berarti rentang itu yang paling nyaman untuk matamu.
 *
 * Catatan: EVR di LM6029/ST7565 hanya 6 bit. Banyak nilai di bawah ~20
 * membuat tulisan nyaris tak terbaca, dan di atas ~55 band dithering mulai
 * menyatu jadi putih semua.
 */

const { LM6029LCD } = require('../build/Release/lcd_lm6029.node');

const args = process.argv.slice(2);
const flags = args.filter((a) => a.startsWith('--'));
const numericArg = args.find((a) => !a.startsWith('--'));

const lcd = new LM6029LCD();
if (!lcd.begin()) {
    console.error('Gagal membuka /dev/spidev0.0. Pastikan SPI sudah di-enable.');
    process.exit(1);
}

const W = lcd.getWidth();
const H = lcd.getHeight();

// Opsi dari command line
if (flags.includes('--no-bl')) lcd.setBacklight(false);
if (flags.includes('--invert')) lcd.setDisplayInvert(true);

// Matriks Bayer 4x4 untuk mensimulasikan gradasi abu-abu di layar 1-bit
const BAYER4 = [
    [0, 8, 2, 10],
    [12, 4, 14, 6],
    [3, 11, 1, 9],
    [15, 7, 13, 5],
];

const TEXT_H = 10; // ruang untuk label kontras di bagian bawah

// 8 band vertikal, tiap band satu tingkat "keabuan" (2,4,...,16 dari 16)
function drawGrayRamp() {
    const bandW = Math.ceil(W / 8);
    for (let b = 0; b < 8; b++) {
        const level = (b + 1) * 2; // 2..16
        const x0 = b * bandW;
        const x1 = Math.min(x0 + bandW, W);
        for (let x = x0; x < x1; x++) {
            for (let y = 0; y < H - TEXT_H; y++) {
                if (BAYER4[y & 3][x & 3] < level) lcd.drawPixel(x, y, 1);
            }
        }
    }
}

// Elemen bantu untuk cek ghosting, ketajaman, dan kontras garis
function drawTestElements() {
    // Bingkai tepi layar
    lcd.drawRect(0, 0, W - 1, H - 1, 1);
    // Garis-garis tipis 1 px untuk cek ketajaman
    for (let y = 4; y < 20; y += 4) {
        lcd.drawLine(4, y, 43, y, 1);
    }
    // Blok hitam solid di kanan atas
    lcd.fillRect(W - 26, 4, 22, 18, 1);
}

function drawPattern(level) {
    lcd.clear();
    drawGrayRamp();
    drawTestElements();

    // Label
    lcd.drawLine(0, H - TEXT_H, W - 1, H - TEXT_H, 1);
    lcd.printText(`CONTRAST ${level}`, 2, H - 8, 1);
    lcd.printText(`${level}/63`, W - 26, H - 8, 1);

    lcd.display();
}

function sleep(ms) {
    const end = Date.now() + ms;
    while (Date.now() < end);
}

// ---------------------------------------------------------------------------
// Mode 1: set satu nilai kontras saja
// ---------------------------------------------------------------------------
if (numericArg !== undefined && !isNaN(Number(numericArg))) {
    const level = lcd.setContrast(Number(numericArg));
    drawPattern(level);
    console.log(`Kontras diset ke ${level}/63. Pola uji ditampilkan di layar.`);
    console.log('Kalau sudah pas, pakai di kodemu: lcd.setContrast(' + level + ');');
    process.exit(0);
}

// ---------------------------------------------------------------------------
// Mode 2: sweep otomatis
// ---------------------------------------------------------------------------
const STEPS = [0, 4, 8, 12, 16, 20, 24, 28, 31, 34, 38, 42, 46, 50, 54, 58, 63];
const DEFAULT_LEVEL = 31;

console.log(`Sweep kontras 0..63 (default pabrik: ${DEFAULT_LEVEL})`);
console.log('Perhatikan layar, lalu pilih nilai yang paling nyaman.\n');

for (const step of STEPS) {
    const level = lcd.setContrast(step);
    drawPattern(level);
    const marker = level === DEFAULT_LEVEL ? '   <-- default' : '';
    console.log(`  EVR ${String(level).padStart(2)}/63  ${'#'.repeat(Math.round(level / 2))}${marker}`);
    sleep(600);
}

// Kembalikan ke nilai default supaya aman kalau ditinggal
const finalLevel = lcd.setContrast(DEFAULT_LEVEL);
drawPattern(finalLevel);

console.log(`\nSelesai. Kontras dikembalikan ke default ${finalLevel}/63.`);
console.log('Untuk mengunci nilai pilihanmu:');
console.log('  node contrast_test.js <0..63>');
console.log('  atau tambahkan lcd.setContrast(n) setelah lcd.begin() di kode utama.\n');
