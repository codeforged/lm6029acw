# lm6029acw

Native Node.js addon (**N-API / node-addon-api**) untuk LCD monokrom
**128×64 LM6029ACW** yang dikendalikan lewat **SPI0 + 2× 74HC595**.
Semua render memakai **[Adafruit_GFX](https://github.com/adafruit/Adafruit-GFX-Library)**
(primitif bentuk, font kustom, bitmap) yang di-port ke C++ di repo ini.

Addon ini adalah *backend* dari driver TSIX `/dev/lcd` — lihat
`src/kernel/devices/aux-devices/LM6029Device.ts` di repo TSIX. Aplikasi TSIX
tidak pernah menyentuh addon ini langsung; mereka memakai
`import { lcd } from "@tsix/lcdLib"`.

## Kebutuhan

| | |
|---|---|
| OS | **Linux** (memakai `spidev`; bus di-auto-deteksi dari `/dev/spidev*`) |
| Hardware | Raspberry Pi / Orange Pi / SBC lain dengan spidev + LCD LM6029ACW + 2× 74HC595 |
| Build | Node.js ≥ 18, `python3`, `make`, `g++` → `sudo apt install build-essential python3` |
| SPI | harus diaktifkan (`sudo raspi-config` → Interface Options → SPI → Enable → reboot; di Orange Pi: `sudo orangepi-config` → Hardware → SPI) |

Cek bus SPI yang tersedia:

```bash
ls /dev/spidev*
# Raspberry Pi : /dev/spidev0.0  /dev/spidev0.1
# Orange Pi    : /dev/spidev3.0  (nomor bus berbeda!)
```

## Instalasi

Sebagai dependency proyek lain (dari npm):

```bash
npm install lm6029acw
```

Untuk mengembangkan addon ini sendiri (dari source):

```bash
git clone https://github.com/codeforged/lm6029acw.git
cd lm6029acw
npm install           # menjalankan node-gyp rebuild
```

Catatan: nama folder/repo boleh berbeda dari nama paket — yang dipakai npm
adalah kolom `name` di `package.json`.

## Pemakaian singkat

```js
const { LM6029LCD } = require('lm6029acw');

const lcd = new LM6029LCD();
if (!lcd.begin()) {
  console.error('Gagal membuka bus SPI — SPI sudah di-enable?');
  process.exit(1);
}
console.log('SPI:', lcd.getSpiDevicePath());   // mis. /dev/spidev0.0 atau /dev/spidev3.0

lcd.setContrast(40);          // 0 (pudar) .. 63 (paling tajam)
lcd.setBacklight(true);
lcd.clear();
lcd.setTextColor(1);
lcd.setTextSize(2);
lcd.setCursor(0, 16);
lcd.print('Halo!');
lcd.drawRect(0, 0, lcd.getWidth(), lcd.getHeight(), 1);
lcd.display();                // flush buffer → panel
```

## API

### Lifecycle
| Fungsi | Keterangan |
|---|---|
| `begin(speedHz?, devicePath?)` | Buka bus SPI (auto-deteksi bila `devicePath` kosong) + init controller. `speedHz` default 10 MHz; urutan argumen bebas (`begin('/dev/spidev3.0')` juga boleh). Return `boolean`. |
| `setSpiDevice(path)` | Paksa bus tertentu untuk `begin()` berikutnya; `''` = kembali auto-deteksi. |
| `getSpiDevicePath()` | Bus yang benar-benar dipakai (`''` bila belum `begin()`). |
| `getSpiProbeLog()` | Jejak percobaan open, mis. `/dev/spidev0.0 gagal, /dev/spidev3.0 ok`. |
| `clear()` / `clearDisplay()` | Bersihkan buffer (belum tampil). |
| `display()` | Kirim buffer ke panel. |
| `getWidth()` / `getHeight()` | 128 / 64 (ikut rotasi). |

### Primitif GFX
`drawPixel(x,y,color)` · `fillScreen(color)` · `drawLine(x0,y0,x1,y1,color)` ·
`drawRect(x,y,w,h,color)` · `fillRect(...)` · `drawCircle(x,y,r,color)` ·
`fillCircle(...)` · `drawTriangle(x0,y0,x1,y1,x2,y2,color)` · `fillTriangle(...)` ·
`drawRoundRect(x,y,w,h,r,color)` · `fillRoundRect(...)` ·
`drawBitmap(x,y,<Buffer>,w,h,color)`

### Teks
| Fungsi | Keterangan |
|---|---|
| `setFont(id)` | `0` = default 5×7, `1` = FreeSans9pt7b, `2` = FreeSansBold12pt7b, `3` = FreeMono9pt7b |
| `setTextColor(color, bg?)` | `bg` diisi → mode opaque |
| `setTextSize(n)` | perbesaran (1 = normal) |
| `setTextWrap(bool)` | word-wrap otomatis |
| `setCursor(x,y)` | posisi basis-kiri-atas |
| `print(str)` | cetak di cursor |
| `printText(str, x, y, size?)` | cetak sekali di posisi tertentu |

`color` di panel ini biner: `1` = piksel nyala, `0` = mati.

### Kontrol tampilan
| Fungsi | Keterangan |
|---|---|
| `setContrast(0..63)` / `getContrast()` | EVR; default driver `31`, nyaman 28–38 |
| `setBacklight(bool)` / `getBacklight()` | pin LED pada 74HC595 |
| `setDisplayInvert(bool)` / `getDisplayInvert()` | tukar piksel nyala ⇄ mati |
| `setDisplayOn(bool)` / `isDisplayOn()` | isi buffer tetap aman saat OFF |
| `setSpiSpeed(hz)` / `getSpiSpeed()` | kecepatan SPI efektif |
| `setRotation(0..3)` | rotasi tampilan |

## Bus SPI: Raspberry Pi vs Orange Pi

Nomor bus SPI **berbeda antar board** dan tidak bisa diasumsikan:

| Board | Path tipikal |
|---|---|
| Raspberry Pi (SPI0) | `/dev/spidev0.0` |
| Orange Pi (SPI3) | `/dev/spidev3.0` |
| Board lain | `/dev/spidev1.0`, `/dev/spidev2.0`, … |

Karena itu addon **tidak meng-hardcode** path. Saat `begin()` ia mencoba
berurutan dan memakai yang pertama bisa dibuka:

1. bus eksplisit — `setSpiDevice()` / `begin(…, path)` / env `LM6029_SPI_DEV`
2. `/dev/spidev0.0` (default Raspberry Pi)
3. sisa `/dev/spidev*` urut lexicographic (menemukan `/dev/spidev3.0` di Orange Pi)

Jadi **satu binary yang sama jalan di Pi maupun Orange Pi tanpa konfigurasi**.
Kalau jatuh ke pilihan 2/3, addon menulis satu baris peringatan ke `stderr`
supaya kelihatan board mana yang sedang dipakai.

Memaksa bus tertentu (kalau auto-deteksi memilih bus yang salah — mis. board
dengan beberapa spidev dan panel Anda di bus kedua):

```bash
# 1. lewat env (paling praktis untuk service/daemon)
LM6029_SPI_DEV=/dev/spidev3.0 npm run demo

# 2. lewat API
```

```js
const lcd = new LM6029LCD();
lcd.setSpiDevice('/dev/spidev3.0');   // atau: lcd.begin(10000000, '/dev/spidev3.0')
lcd.begin();
console.log(lcd.getSpiDevicePath());  // /dev/spidev3.0
```

> Auto-deteksi hanya memilih bus yang **bisa dibuka** — ia tidak bisa tahu di
> bus mana panel benar-benar terpasang. Kalau layar tetap kosong padahal
> `begin()` sukses, cek urutan bus dengan `getSpiProbeLog()` lalu paksa path
> yang benar.

## Contoh

Jalankan dari root project:

```bash
npm run demo          # contoh paling dasar
npm run gfx           # semua primitif Adafruit_GFX
npm run ti84          # grafik kalkulator (butuh mathjs)
npm run fps           # benchmark FPS 3 fase
npm run spi           # cek integritas sinyal vs kecepatan clock
npm run contrast      # tuning kontras (EVR)
```

Detail tiap script: [`examples/README.md`](examples/README.md).

## Catatan hardware

**Clock SPI.** Di Raspberry Pi (driver bcm2835) clock SPI = core clock / 2^CDIV
dan CDIV harus pangkat dua. Dengan core 250 MHz: minta 32 MHz → dapat 31.25 MHz.
Cek core clock asli: `vcgencmd measure_clock core`.

**Performa.** Satu `display()` mengirim 8384 byte lewat 4192 `ioctl()`, jadi
bottleneck-nya biasanya overhead syscall, bukan SPI.

## Arsitektur singkat

```
Node.js (JS)              C++ (addon)                    Hardware
─────────────             ───────────                    ────────
LM6029LCD  ──binding──▶   LM6029ACW_595 : Adafruit_GFX
                             │
                             ├─ /dev/spidev* ──────────▶ SPI (data piksel)
                             │   (auto: 0.0 Pi / 3.0 Orange Pi)
                             └─ bit kontrol 74HC595 ───▶ RD / WR / RS / RES / CS / LED
```

Pin kontrol (`RD`, `WR`, `RS`, `RES`, `CS`, `LED`) di-*shift* ke 74HC595,
bukan lewat GPIO JavaScript — lihat bit mask di `src/LM6029ACW_595.h`.

## Publikasi (maintainer)

```bash
npm login
npm view lm6029acw        # pastikan nama masih kosong
npm publish --dry-run     # cek isi paket: index.js, binding.gyp, src/, examples/
npm publish               # publikasikan
```

Yang **tidak** ikut terkirim (memang tidak boleh): `node_modules/` dan `build/`.
Binari `.node` dibuat ulang oleh `node-gyp` di mesin pemasang, sesuai
arsitektur mesin itu (ARM32/ARM64).

## Lisensi

ISC — lihat `package.json`.
