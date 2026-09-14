/*
 * spi_speed_test.js - Uji kecepatan clock SPI + cek integritas sinyal
 *
 * Cara pakai:
 *   node spi_speed_test.js             -> sweep beberapa kecepatan, ukur & tampilkan pola
 *   node spi_speed_test.js 32000000    -> set satu kecepatan, tampilkan pola uji
 *
 * PENTING soal Raspberry Pi (driver bcm2835):
 *   clock SPI = core clock / 2^CDIV   (pembagi HARUS pangkat dua)
 * Jadi angka yang enak di teori tidak semuanya bisa dicapai. Dengan core 250 MHz:
 *
 *   minta 8 MHz  -> dapat  7.8125 MHz
 *   minta 16 MHz -> dapat 15.625  MHz
 *   minta 32 MHz -> dapat 31.25   MHz   <- termanis
 *   minta 64 MHz -> dapat 62.5    MHz   (74HC595 hampir pasti sudah error)
 *
 * Cek core clock aslimu dengan:  vcgencmd measure_clock core
 *
 * CARA MEMBACA HASILNYA:
 *   - ms/flush turun  = kecepatan naik, bagus.
 *   - ms/flush mentok / malah naik = kemungkinan besar sudah kena limit ioctl(),
 *     bukan limit SPI-nya.
 *   - Pola di layar rusak (garis putus, blok jadi bergaris, teks ngawur) =
 *     kecepatan terlalu tinggi untuk 74HC595 + kabelmu. Turunkan satu tingkat.
 */

const { LM6029LCD } = require('../build/Release/lcd_lm6029.node');

// Byte yang dikirim ke LCD tiap kali display():
// 8 halaman x (3 perintah + 128 data) x 4 transfer x 2 byte = 8384 byte
const BYTES_PER_FLUSH = 8384;

const CORE_HZ = 250e6; // Pi 2/3 default 250 MHz. Pi 4/5 beda (pembaginya bukan pangkat dua).

const now = () => Number(process.hrtime.bigint()) / 1e6;
const sleep = (ms) => { const end = Date.now() + ms; while (Date.now() < end); };

function fmtHz(hz) {
    if (hz >= 1e6) return `${(hz / 1e6).toFixed(hz % 1e6 === 0 ? 0 : 3).replace(/\.?0+$/, '')} MHz`;
    return `${(hz / 1e3).toFixed(0)} kHz`;
}

// Perkiraan kecepatan nyata: core / 2^CDIV
function predictActualHz(requested) {
    let div = 2;
    while (CORE_HZ / div > requested && div < 65536) div *= 2;
    return CORE_HZ / div;
}

const lcd = new LM6029LCD();
if (!lcd.begin()) {
    console.error('Gagal membuka /dev/spidev0.0. Pastikan SPI sudah di-enable.');
    process.exit(1);
}

const W = lcd.getWidth();
const H = lcd.getHeight();

// ---------------------------------------------------------------------------
// Pola uji integritas sinyal
// ---------------------------------------------------------------------------
function drawIntegrityPattern(label) {
    lcd.clear();

    // 1. Papan catur 1 px -> paling sensitif terhadap geser/bit hilang
    for (let y = 3; y < 21; y++) {
        for (let x = 3; x < 61; x++) {
            if ((x + y) & 1) lcd.drawPixel(x, y, 1);
        }
    }

    // 2. Garis vertikal 1 px jarak 4 px -> kelihatan kalau ada kolom yang tertukar
    for (let x = 66; x < 126; x += 4) {
        lcd.drawLine(x, 3, x, 21, 1);
    }

    // 3. Blok solid -> kalau ada noise, akan muncul garis-garis di dalamnya
    lcd.fillRect(3, 25, 58, 13, 1);

    // 4. Teks
    lcd.printText('SPI', 66, 30, 1);
    lcd.printText(label, 66, 40, 1);

    // 5. Bingkai
    lcd.drawRect(0, 0, W - 1, H - 1, 1);
    lcd.drawRect(63, 1, 64, 62, 1);

    lcd.display();
}

// ---------------------------------------------------------------------------
// Ukur waktu flush
// ---------------------------------------------------------------------------
function measureFlushMs(samples) {
    lcd.display(); // warm-up
    const t0 = now();
    for (let i = 0; i < samples; i++) lcd.display();
    return (now() - t0) / samples;
}

// ---------------------------------------------------------------------------
// Mode 1: set satu kecepatan saja
// ---------------------------------------------------------------------------
const arg = process.argv[2];

if (arg !== undefined && !isNaN(Number(arg))) {
    const requested = Number(arg);
    lcd.setSpiSpeed(requested);
    const actual = lcd.getSpiSpeed();

    drawIntegrityPattern(fmtHz(actual));
    const ms = measureFlushMs(30);

    console.log(`Kecepatan SPI: minta ${fmtHz(requested)} -> driver pakai ${fmtHz(actual)}`);
    console.log(`Perkiraan clock efektif (core 250 MHz / 2^CDIV): ${fmtHz(predictActualHz(requested))}`);
    console.log(`Rata-rata flush: ${ms.toFixed(2)} ms  ->  ${(1000 / ms).toFixed(1)} FPS`);
    console.log(`Laju data: ${(BYTES_PER_FLUSH / (ms / 1000) / 1024).toFixed(1)} KB/s`);
    console.log('\nPeriksa pola di layar. Kalau rusak, kecepatan ini terlalu tinggi.');
    console.log(`Pakai di kodemu: lcd.begin(${requested});  atau  lcd.setSpiSpeed(${requested});`);
    process.exit(0);
}

// ---------------------------------------------------------------------------
// Mode 2: sweep
// ---------------------------------------------------------------------------
const SPEEDS = [8000000, 16000000, 32000000, 64000000];

console.log('='.repeat(80));
console.log(' Uji kecepatan SPI - LCD LM6029ACW 128x64');
console.log('='.repeat(80));
console.log(' Perhatikan layar setiap kali kecepatan berubah.');
console.log(' Pola papan catur & garis harus tetap rapi dan tegas.');
console.log(' Kalau ada garis putus / blok bergaris / teks ngawur -> terlalu cepat.');
console.log('-'.repeat(80));
console.log('  minta      -> dipakai        flush     FPS    laju data   estimasi clock');
console.log('-'.repeat(80));

const results = [];

for (const requested of SPEEDS) {
    lcd.setSpiSpeed(requested);
    const actual = lcd.getSpiSpeed();

    drawIntegrityPattern(fmtHz(actual));
    sleep(250); // beri waktu mata melihat sebelum diukur

    const ms = measureFlushMs(30);
    const fps = 1000 / ms;
    const kb = BYTES_PER_FLUSH / (ms / 1000) / 1024;

    results.push({ requested, ms, fps });

    console.log(
        `  ${fmtHz(requested).padEnd(9)} -> ${fmtHz(actual).padEnd(12)} ` +
        `${ms.toFixed(2).padStart(6)} ms ${fps.toFixed(1).padStart(6)} ` +
        `${kb.toFixed(0).padStart(7)} KB/s   ${fmtHz(predictActualHz(requested))}`
    );
}

console.log('-'.repeat(80));

// ---------------------------------------------------------------------------
// Analisis
// ---------------------------------------------------------------------------
const base = results[0];
const best = results.reduce((a, b) => (b.ms < a.ms ? b : a));

console.log('\nAnalisis:');
console.log(`  Titik awal : ${fmtHz(base.requested)} -> ${base.ms.toFixed(2)} ms/flush (${base.fps.toFixed(1)} FPS)`);
console.log(`  Tercepat   : ${fmtHz(best.requested)} -> ${best.ms.toFixed(2)} ms/flush (${best.fps.toFixed(1)} FPS)`);
console.log(`  Peningkatan: ${(base.ms / best.ms).toFixed(2)}x lebih cepat`);

// Waktu SPI murni (tanpa overhead syscall) untuk tiap kecepatan
const bits = BYTES_PER_FLUSH * 8;
console.log('\n  Kalau waktu flush didominasi SPI murni, angkanya akan seperti ini:');
for (const r of results) {
    const pureMs = (bits / predictActualHz(r.requested)) * 1000;
    const overhead = r.ms - pureMs;
    console.log(
        `    ${fmtHz(r.requested).padEnd(10)} SPI murni ~${pureMs.toFixed(2).padStart(6)} ms, ` +
        `sisanya ~${overhead.toFixed(2).padStart(6)} ms`
    );
}
console.log('\n  Kalau "sisanya" mirip di semua kecepatan, berarti batasnya bukan SPI');
console.log('  tapi 4192 kali ioctl() per flush. Naikkan clock tidak akan banyak');
console.log('  menolong; perlu batching transfer atau dirty-page tracking.');

// ---------------------------------------------------------------------------
// Tinggalkan di kecepatan yang masih aman (default: 16 MHz)
// ---------------------------------------------------------------------------
const SAFE_REQUEST = 16000000;
lcd.setSpiSpeed(SAFE_REQUEST);
drawIntegrityPattern(fmtHz(lcd.getSpiSpeed()));

console.log(`\nLayar ditinggalkan di ${fmtHz(lcd.getSpiSpeed())} dengan pola uji.`);
console.log('Kalau sudah mantap, set permanen di awal program:');
console.log('  const lcd = new LM6029LCD();');
console.log('  if (!lcd.begin(32000000)) { ... }   // atau lcd.setSpiSpeed(32000000)');
console.log('Kalau layar rusak, turunkan satu tingkat (16000000) atau balik ke 8000000.\n');
