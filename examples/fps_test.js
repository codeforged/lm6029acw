/*
 * fps_test.js - Benchmark FPS untuk LCD LM6029ACW (128x64) via SPI 10 MHz
 *
 * Cara pakai:
 *   node fps_test.js                          -> animasi bola, 5 detik per fase
 *   node fps_test.js 10                       -> 10 detik per fase
 *   node fps_test.js 10 wave                  -> animasi gelombang sinus
 *   node fps_test.js 10 bars                  -> animasi bar equalizer
 *   node fps_test.js 10 ball --speed=32000000 -> pakai SPI 32 MHz
 *
 * Catatan Pi: begin(32000000) menghasilkan 31.25 MHz nyata, karena pembagi
 * clock SPI di Raspberry Pi harus pangkat dua (lihat spi_speed_test.js).
 *
 * Tiga fase pengukuran:
 *   [1] RENDER  : hanya menggambar ke buffer RAM (tanpa kirim ke LCD)
 *   [2] FLUSH   : hanya lcd.display() (buffer diam, murni biaya SPI)
 *   [3] FULL    : render + flush (FPS nyata yang kamu rasakan di mata)
 *
 * Catatan teknis: satu lcd.display() mengirim 8 halaman x
 * (3 perintah + 128 data) = 8384 byte SPI, tapi dipecah jadi 4192 transfer
 * 2-byte. Jadi yang jadi bottleneck biasanya overhead ioctl(), bukan
 * kecepatan SPI-nya sendiri (10 MHz = ~1.25 MB/s).
 */

const { LM6029LCD } = require('../build/Release/lcd_lm6029.node');

const argv = process.argv.slice(2);
const speedArg = argv.find((a) => a.startsWith('--speed='));
const SPI_SPEED_HZ = speedArg ? Number(speedArg.split('=')[1]) : 0; // 0 = default driver (10 MHz)

const positional = argv.filter((a) => !a.startsWith('--'));
const DURATION_MS = Math.max(1, Number(positional[0] || 5)) * 1000;
const MODE = (positional[1] || 'ball').toLowerCase();

const lcd = new LM6029LCD();
if (!lcd.begin(SPI_SPEED_HZ)) {
    console.error('Gagal membuka /dev/spidev0.0. Pastikan SPI sudah di-enable.');
    process.exit(1);
}

lcd.setContrast(40);       // 0 = pudar, 63 = paling tajam
lcd.setBacklight(true);

const W = lcd.getWidth();
const H = lcd.getHeight();

// Timer resolusi tinggi (nanosecond -> millisecond, float)
const now = () => Number(process.hrtime.bigint()) / 1e6;

// Gelombang segitiga 0..1 (untuk gerak bolak-balik mulus)
function tri(phase) {
    const p = phase % 1;
    return p < 0.5 ? p * 2 : 2 - p * 2;
}

// ---------------------------------------------------------------------------
// Scene / animasi
// ---------------------------------------------------------------------------
function drawScene(frame, t) {
    lcd.clear();

    if (MODE === 'wave') {
        // Gelombang sinus yang bergerak + sumbu tengah
        const mid = (H >> 1) - 6;
        lcd.drawLine(0, mid, W - 1, mid, 1);

        let prevX = 0;
        let prevY = mid;
        for (let x = 0; x < W; x++) {
            const y = Math.round(mid + Math.sin((x + t * 0.25) * 0.15) * 18);
            lcd.drawLine(prevX, prevY, x, y, 1);
            prevX = x;
            prevY = y;
        }
    } else if (MODE === 'bars') {
        // Equalizer 16 bar yang bergerak naik-turun
        const bars = 16;
        const barW = Math.floor(W / bars);
        for (let i = 0; i < bars; i++) {
            const h = Math.round((0.5 + 0.5 * Math.sin(t * 0.006 + i * 0.7)) * (H - 16));
            lcd.fillRect(i * barW, H - 10 - h, barW - 1, h, 1);
        }
        lcd.drawLine(0, H - 10, W - 1, H - 10, 1);
    } else {
        // Mode default: bola memantul + bingkai
        const r = 9;
        const spanX = W - 2 * r - 2;
        const spanY = H - 2 * r - 2 - 10; // sisakan ruang untuk footer
        const cx = Math.round(1 + r + tri(t / 1400) * spanX);
        const cy = Math.round(1 + r + tri(t / 900) * spanY);
        lcd.drawRect(0, 0, W - 1, H - 1, 1);
        lcd.fillCircle(cx, cy, r, 1);
    }

    // --- Footer: nomor frame + progress bar durasi fase ---
    lcd.printText(`F${frame}`, 1, H - 9, 1);

    const barX = 34;
    const barW = W - barX - 1;
    lcd.drawRect(barX, H - 9, barW, 8, 1);
    const progress = ((t % DURATION_MS) / DURATION_MS) * (barW - 2);
    if (progress > 0) {
        lcd.fillRect(barX + 1, H - 8, Math.round(progress), 6, 1);
    }
}

// ---------------------------------------------------------------------------
// Fase 1: RENDER saja (buffer RAM, tanpa SPI)
// ---------------------------------------------------------------------------
function benchRender() {
    let frames = 0;
    const t0 = now();
    let elapsed = 0;
    while (elapsed < DURATION_MS) {
        drawScene(frames, elapsed);
        frames++;
        elapsed = now() - t0;
    }
    return { frames, elapsed };
}

// ---------------------------------------------------------------------------
// Fase 2: FLUSH saja (buffer tidak berubah)
// ---------------------------------------------------------------------------
function benchFlush() {
    drawScene(0, 0);
    let frames = 0;
    const t0 = now();
    let elapsed = 0;
    while (elapsed < DURATION_MS) {
        lcd.display();
        frames++;
        elapsed = now() - t0;
    }
    return { frames, elapsed };
}

// ---------------------------------------------------------------------------
// Fase 3: FULL (render + flush) -- FPS sesungguhnya
// ---------------------------------------------------------------------------
function benchFull() {
    let frames = 0;
    let minMs = Infinity;
    let maxMs = 0;
    let renderTotal = 0;
    let flushTotal = 0;

    const t0 = now();
    let elapsed = 0;

    while (elapsed < DURATION_MS) {
        const fStart = now();

        const rStart = now();
        drawScene(frames, elapsed);
        renderTotal += now() - rStart;

        const dStart = now();
        lcd.display();
        flushTotal += now() - dStart;

        const fMs = now() - fStart;
        if (fMs < minMs) minMs = fMs;
        if (fMs > maxMs) maxMs = fMs;

        frames++;
        elapsed = now() - t0;
    }

    return { frames, elapsed, minMs, maxMs, renderTotal, flushTotal };
}

// ---------------------------------------------------------------------------
// Jalankan & laporkan
// ---------------------------------------------------------------------------
function fps(frames, ms) {
    return frames / (ms / 1000);
}

function report(label, frames, ms, extra) {
    const f = fps(frames, ms);
    const perFrame = ms / frames;
    console.log(
        `${label.padEnd(34)}: ${f.toFixed(1).padStart(8)} FPS  ` +
        `(${perFrame.toFixed(3).padStart(8)} ms/frame, ${frames} frame)` +
        (extra ? `  ${extra}` : '')
    );
    return f;
}

console.log('='.repeat(78));
console.log(` Benchmark LCD LM6029ACW  ${W}x${H}  |  mode: ${MODE}  |  durasi/fase: ${DURATION_MS / 1000}s`);
console.log(` SPI clock yang dipakai: ${(lcd.getSpiSpeed() / 1e6).toFixed(3)} MHz (diminta)`);
console.log('='.repeat(78));

console.log('\n[1] Render ke buffer RAM (tanpa SPI)...');
const r1 = benchRender();
report('  GFX render only', r1.frames, r1.elapsed);

console.log('\n[2] Flush buffer -> LCD (murni biaya SPI/ioctl)...');
const r2 = benchFlush();
report('  lcd.display() only', r2.frames, r2.elapsed);

console.log('\n[3] Animasi penuh (render + flush)...');
lcd.clear();
const r3 = benchFull();
const fullFps = report('  FULL frame', r3.frames, r3.elapsed,
    `min ${r3.minMs.toFixed(2)} ms / max ${r3.maxMs.toFixed(2)} ms`);

console.log('\n' + '-'.repeat(78));
console.log(' Rincian rata-rata per frame pada fase 3:');
console.log(`   render (GFX ke RAM) : ${(r3.renderTotal / r3.frames).toFixed(3)} ms`);
console.log(`   flush  (SPI ke LCD) : ${(r3.flushTotal / r3.frames).toFixed(3)} ms`);
console.log(`   total               : ${(r3.elapsed / r3.frames).toFixed(3)} ms`);
console.log(`   -> FPS efektif      : ${fullFps.toFixed(1)}`);
console.log(`   -> SPI dipakai      : ${(8384 * fullFps / 1024).toFixed(1)} KB/s dari ~1220 KB/s (teoretis 10 MHz)`);
console.log('-'.repeat(78));

// Bersihkan layar
lcd.clear();
lcd.display();
console.log('Benchmark selesai. Layar dibersihkan.\n');
