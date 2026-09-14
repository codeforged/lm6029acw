const { LM6029LCD } = require('../build/Release/lcd_lm6029.node');
const math = require('mathjs');

const lcd = new LM6029LCD();

if (!lcd.begin()) {
    console.error("Gagal menginisialisasi LCD!");
    process.exit(1);
}

// Konfigurasi Layar & Grafik
const WIDTH = 128;
const HEIGHT = 64;

// Rentang Sumbu Cartesian (Viewport TI-84)
const xMin = -10, xMax = 10;
const yMin = -5,  yMax = 5;

// Konversi Koordinat Matematika (x, y) ke Koordinat Piksel Layar (px, py)
function toPixelX(x) {
    return Math.round(((x - xMin) / (xMax - xMin)) * (WIDTH - 1));
}

function toPixelY(y) {
    return Math.round(((yMax - y) / (yMax - yMin)) * (HEIGHT - 1));
}

function drawTI84Graph(expressionStr) {
    lcd.clearDisplay();

    // 1. Gambar Sumbu X dan Y (Cartesian Grid)
    const originX = toPixelX(0);
    const originY = toPixelY(0);

    // Sumbu X
    if (originY >= 0 && originY < HEIGHT) {
        lcd.drawLine(0, originY, WIDTH - 1, originY, 1);
    }
    // Sumbu Y
    if (originX >= 0 && originX < WIDTH) {
        lcd.drawLine(originX, 0, originX, HEIGHT - 1, 1);
    }

    // 2. Parse Rumus Matematika
    const compiledExpr = math.compile(expressionStr);

    // 3. Plot Grafik (Koneksikan titik-titik koordinat dengan garis)
    let prevPx = null;
    let prevPy = null;

    for (let px = 0; px < WIDTH; px++) {
        // Hitung nilai x matematika berdasarkan piksel
        const xVal = xMin + (px / (WIDTH - 1)) * (xMax - xMin);

        try {
            const yVal = compiledExpr.evaluate({ x: xVal });

            // Hanya gambar jika hasil y adalah angka valid
            if (typeof yVal === 'number' && !isNaN(yVal) && isFinite(yVal)) {
                const py = toPixelY(yVal);

                // Hubungkan titik sebelumnya ke titik sekarang agar garis tidak putus-putus
                if (prevPx !== null && prevPy !== null) {
                    // Filter agar garis ekstrem tidak merusak tampilan (out of bounds)
                    if (py >= -HEIGHT && py < HEIGHT * 2) {
                        lcd.drawLine(prevPx, prevPy, px, py, 1);
                    }
                }

                prevPx = px;
                prevPy = py;
            } else {
                prevPx = null;
                prevPy = null;
            }
        } catch (e) {
            prevPx = null;
            prevPy = null;
        }
    }

    // 4. Tampilkan Nama Fungsi di Pojok Kiri Atas (Khas TI-84)
    lcd.printText(`y=${expressionStr}`, 2, 2, 1);

    // Flush ke LCD
    lcd.display();
    console.log(`Berhasil menggambar grafik: y = ${expressionStr}`);
}

// --- TES GAMBAR RUMUS ---
// BISA DIGANTI-GANTI RUMUSNYA SESUKA HATI:
// Contoh: "sin(x)", "x^2 - 3", "0.5 * x^3", "cos(x) * 2"
const rumus = "sin(x)"; 
drawTI84Graph(rumus);
