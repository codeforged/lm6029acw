# Examples

Semua script di folder ini dijalankan dari **root project** (bukan dari dalam
`examples/`), karena path-nya relatif ke `build/Release/lcd_lm6029.node`.

```bash
npm run install     # build native binding dulu (sekali saja / tiap ganti kode C++)
```

| Script | npm | Fungsi |
|---|---|---|
| `basic_demo.js` | `npm run demo` | Contoh paling dasar: bingkai, teks, lingkaran, kotak. Pakai `require('../index.js')` jadi sekaligus contoh integrasi. |
| `test_gfx.js` | `npm run gfx` | Uji semua primitive Adafruit_GFX: shapes, teks default, inversi, 3 font kustom. |
| `ti84.js` | `npm run ti84` | Grafik kalkulator TI-84 (`sin(x)`, `x^2-3`, dst) pakai `mathjs`. Rumusnya ganti di variabel `rumus`. |
| `contrast_test.js` | `npm run contrast` | Tuning kontras (EVR 0..63) dengan pola gradasi dithering Bayer 4x4. |
| `spi_speed_test.js` | `npm run spi` | Uji kecepatan clock SPI + cek integritas sinyal (pola papan catur & garis tipis). |
| `fps_test.js` | `npm run fps` | Benchmark FPS 3 fase: render-only, flush-only, dan full frame. |

## Cara pakai tiap script

```bash
# Contoh dasar
node examples/basic_demo.js

# Semua primitive GFX (butuh ~12 detik)
node examples/test_gfx.js

# Kalkulator grafik
node examples/ti84.js

# Kontras: sweep 0..63, atau set satu nilai
node examples/contrast_test.js
node examples/contrast_test.js 42

# Kecepatan SPI: sweep, atau set satu nilai
node examples/spi_speed_test.js
node examples/spi_speed_test.js 32000000

# FPS: [durasi detik] [mode: ball|wave|bars] [--speed=Hz]
node examples/fps_test.js
node examples/fps_test.js 10 wave
node examples/fps_test.js 10 ball --speed=32000000
```

## Catatan hardware

**SPI clock.** Di Raspberry Pi (driver bcm2835), clock SPI = core clock / 2^CDIV
dan CDIV harus pangkat dua. Dengan core 250 MHz:

| Diminta | Didapat |
|---|---|
| 10 MHz (default) | 7.8125 MHz |
| 16 MHz | 15.625 MHz |
| 32 MHz | 31.25 MHz |
| 64 MHz | 62.5 MHz (74HC595 hampir pasti error) |

Cek core clock asli: `vcgencmd measure_clock core`.

**Kontras.** EVR cuma 6 bit (0..63), default driver `31`. Nyaman biasanya 28-38.
Di bawah ~20 tulisan 1 px mulai hilang; di atas ~55 gradasi dithering menyatu
jadi putih semua.

**Performa.** Satu `display()` mengirim 8384 byte lewat 4192 kali `ioctl()`,
jadi bottleneck-nya biasanya overhead syscall, bukan SPI-nya. Kalau
`spi_speed_test.js` menunjukkan "sisanya" (non-SPI) mirip di semua kecepatan,
naikkan clock tidak akan banyak menolong.

**SPI belum aktif?**

```bash
ls /dev/spidev0.0     # kalau tidak ada:
sudo raspi-config     # Interface Options -> SPI -> Enable -> reboot
```
