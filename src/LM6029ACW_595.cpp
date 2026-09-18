#include "LM6029ACW_595.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <glob.h>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

LM6029ACW_595::LM6029ACW_595() : Adafruit_GFX(LCD_WIDTH, LCD_HEIGHT) {
    _spiFd = -1;
    _shiftRegState = 0;
    _contrastLevel = 0x1F; // default ~31 dari 63
    _backlightOn = true;
    _displayInverted = false;
    _displayOn = true;
    _spiSpeedHz = 10000000; // 10 MHz

    // Bus SPI berbeda antar board (Pi: /dev/spidev0.0, Orange Pi:
    // /dev/spidev3.0). Bisa dipaksa lewat env LM6029_SPI_DEV; dibiarkan kosong
    // berarti begin() yang mengauto-deteksi dari /dev/spidev*.
    const char* envDev = getenv("LM6029_SPI_DEV");
    _spiDevicePref = envDev ? envDev : "";
}

LM6029ACW_595::~LM6029ACW_595() {
    if (_spiFd >= 0) close(_spiFd);
}

bool LM6029ACW_595::begin(uint32_t speedHz) {
    // Nilai di bawah 100 kHz dianggap tidak masuk akal (mis. sisa pemanggilan
    // begin() versi lama dengan argumen pin) -> abaikan.
    if (speedHz >= 100000) _spiSpeedHz = speedHz;

    // begin() boleh dipanggil ulang (hotplug) — jangan bocorkan FD lama.
    if (_spiFd >= 0) {
        close(_spiFd);
        _spiFd = -1;
    }

    // Buka bus SPI yang tersedia; path-nya beda antar SBC (Pi vs Orange Pi).
    _spiFd = openSpiBus();
    if (_spiFd < 0) return false;

    uint8_t mode = SPI_MODE_0;
    uint32_t speed = _spiSpeedHz;

    ioctl(_spiFd, SPI_IOC_WR_MODE, &mode);
    ioctl(_spiFd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);

    // Inisialisasi LCD
    setControlPins(PIN_RD | PIN_WR | PIN_RS | PIN_LED);
    usleep(50000);
    setControlPins(PIN_RD | PIN_WR | PIN_RS | PIN_RES | PIN_LED);
    usleep(50000);

    sendCommand(LCD_START_LINE);
    sendCommand(LCD_ADC_NORMAL);
    sendCommand(LCD_DISPLAY_ALL_POINT_OFF);
    sendCommand(LCD_BIAS_1_9);
    sendCommand(LCD_COM_NORMAL);
    sendCommand(LCD_POWER_CTRL);
    sendCommand(LCD_SET_EVR);
    sendCommand(_contrastLevel);
    sendCommand(_displayInverted ? LCD_DISPLAY_INVERSE : LCD_DISPLAY_NORMAL);
    sendCommand(LCD_DISPLAY_ON);
    _displayOn = true;

    clearDisplay();
    display();
    return true;
}

// ---------------------------------------------------------------------------
// Pemilihan bus SPI (portabilitas antar-SBC)
// ---------------------------------------------------------------------------

// Paksa bus tertentu, mis. "/dev/spidev3.0" (Orange Pi). String kosong =
// kembali ke auto-deteksi. Berlaku pada begin() berikutnya.
void LM6029ACW_595::setSpiDevice(const char* path) {
    if (path == NULL) {
        _spiDevicePref.clear();
        return;
    }

    std::string p(path);
    // Buang spasi/baris baru yang ikut terbawa (env var, argumen CLI).
    while (!p.empty() && isspace(static_cast<unsigned char>(p.front()))) p.erase(p.begin());
    while (!p.empty() && isspace(static_cast<unsigned char>(p.back()))) p.pop_back();
    _spiDevicePref = p;
}

// Cari bus SPI yang bisa dibuka. Urutan prioritas:
//   1. preferensi eksplisit — setSpiDevice() atau env LM6029_SPI_DEV
//   2. /dev/spidev0.0 — default Raspberry Pi
//   3. sisa /dev/spidev* — urut lexicographic (Orange Pi: /dev/spidev3.0, dst.)
// Path pertama yang berhasil dibuka dipakai (disimpan di _spiDevicePath).
// Kalau bukan pilihan pertama, tulis alasannya ke stderr sekali — supaya
// kelihatan saat pindah board. Set LM6029_SPI_DEV untuk memaksa bus tertentu.
int LM6029ACW_595::openSpiBus() {
    std::vector<std::string> candidates;
    auto add = [&candidates](const std::string& p) {
        if (p.empty()) return;
        if (std::find(candidates.begin(), candidates.end(), p) == candidates.end())
            candidates.push_back(p);
    };

    add(_spiDevicePref);
    add("/dev/spidev0.0"); // Raspberry Pi

    // Apa pun yang tersedia (Orange Pi 3.0, board lain 1.0/2.0, ...).
    glob_t g;
    if (glob("/dev/spidev*", 0, NULL, &g) == 0) {
        for (size_t i = 0; i < g.gl_pathc; i++) add(g.gl_pathv[i]);
        globfree(&g);
    }

    _spiProbeLog.clear();
    for (size_t i = 0; i < candidates.size(); i++) {
        int fd = open(candidates[i].c_str(), O_RDWR);
        if (!_spiProbeLog.empty()) _spiProbeLog += ", ";
        _spiProbeLog += candidates[i] + (fd >= 0 ? " ok" : " gagal");

        if (fd >= 0) {
            _spiDevicePath = candidates[i];
            if (i > 0) {
                std::cerr << "[lm6029acw] " << candidates[0]
                          << " tidak tersedia - memakai " << _spiDevicePath
                          << " (set LM6029_SPI_DEV untuk memaksa)" << std::endl;
            }
            return fd;
        }
    }

    std::cerr << "[lm6029acw] tidak ada bus SPI yang bisa dibuka. Dicoba: "
              << (_spiProbeLog.empty() ? "(tidak ada /dev/spidev*)" : _spiProbeLog)
              << " — SPI/overlay sudah diaktifkan?" << std::endl;
    return -1;
}

void LM6029ACW_595::shiftOutDual74HC595(uint16_t value) {
    if (_backlightOn) {
        value &= ~(PIN_LED << 8);
    } else {
        value |= (PIN_LED << 8);
    }

    // Kirim 16-bit data (2 byte) via SPI hardware sekaligus
    uint8_t data[2];
    data[0] = (value >> 8) & 0xFF; // Byte kontrol (595 ke-2)
    data[1] = value & 0xFF;        // Byte data (595 ke-1)

    struct spi_ioc_transfer tr = {};
    tr.tx_buf = (unsigned long)data;
    tr.len = 2;
    tr.speed_hz = _spiSpeedHz; // per-transfer, INI yang menentukan kecepatan nyata

    ioctl(_spiFd, SPI_IOC_MESSAGE(1), &tr);
    _shiftRegState = value;
}

// Low level command & data
void LM6029ACW_595::setControlPins(uint8_t pins) {
    uint16_t newState = (_shiftRegState & 0x00FF) | (pins << 8);
    shiftOutDual74HC595(newState);
}

void LM6029ACW_595::sendCommand(uint8_t cmd) {
    setControlPins(PIN_RD | PIN_WR | PIN_RES | PIN_LED);
    shiftOutDual74HC595((_shiftRegState & 0xFF00) | cmd);
    setControlPins(PIN_RD | PIN_RES | PIN_LED);
    setControlPins(PIN_RD | PIN_WR | PIN_RES | PIN_LED);
}

void LM6029ACW_595::sendData(uint8_t data) {
    setControlPins(PIN_RD | PIN_WR | PIN_RS | PIN_RES | PIN_LED);
    shiftOutDual74HC595((_shiftRegState & 0xFF00) | data);
    setControlPins(PIN_RD | PIN_RS | PIN_RES | PIN_LED);
    setControlPins(PIN_RD | PIN_WR | PIN_RS | PIN_RES | PIN_LED);
}

void LM6029ACW_595::clearDisplay() {
    memset(_displayBuffer, 0, sizeof(_displayBuffer));
}

void LM6029ACW_595::display() {
    for (uint8_t page = 0; page < LCD_PAGES; page++) {
        sendCommand(LCD_SET_PAGE | page);
        sendCommand(LCD_SET_COL_MSB);
        sendCommand(LCD_SET_COL_LSB);
        for (uint8_t col = 0; col < LCD_WIDTH; col++) {
            sendData(_displayBuffer[(page * LCD_WIDTH) + col]);
        }
    }
}

void LM6029ACW_595::drawPixel(int16_t x, int16_t y, uint16_t color) {
    if ((x < 0) || (x >= LCD_WIDTH) || (y < 0) || (y >= LCD_HEIGHT)) return;
    uint8_t page = y / 8;
    uint8_t bit = y % 8;
    uint16_t index = (page * LCD_WIDTH) + x;

    if (color) _displayBuffer[index] |= (1 << bit);
    else _displayBuffer[index] &= ~(1 << bit);
}

// Cetak string pada posisi (x, y) memakai font yang sedang aktif.
// Untuk font default 5x7, y = baris atas teks.
// Untuk font Adafruit kustom, y = baseline teks.
//
// PENTING: warna teks/latar TIDAK dipaksa di sini — pakai setTextColor() yang
// sedang aktif, sama seperti print(). Sebelumnya ada baris `setTextColor(1)`
// yang menimpa pilihan pemanggil, sehingga teks inversi `setTextColor(0, 1)`
// (glyph 0 di atas latar 1) ikut digambar 1 di atas latar 1 → tidak terlihat
// di panel. Bug ini tidak muncul di examples/test_gfx.js karena contoh itu
// memakai setCursor()+print(), bukan printText().
void LM6029ACW_595::printText(const char* str, int16_t x, int16_t y, uint8_t size) {
    if (str == NULL) return;

    setTextSize(size);
    setCursor(x, y);

    for (const char* p = str; *p != '\0'; ++p) {
        write(static_cast<uint8_t>(*p));
    }
}

// ---------------------------------------------------------------------------
// Kontrol tampilan
// ---------------------------------------------------------------------------

// Atur kontras lewat Electronic Volume Register (0x81) pada LM6029 / ST7565.
// Nilainya 6-bit: 0 (paling pudar) .. 63 (paling tajam).
// Boleh dipanggil sebelum begin(): nilainya disimpan dan dikirim saat begin().
void LM6029ACW_595::setContrast(uint8_t level) {
    if (level > 0x3F) level = 0x3F; // EVR hanya 6 bit
    _contrastLevel = level;

    if (_spiFd < 0) return; // belum begin()

    sendCommand(LCD_SET_EVR);
    sendCommand(_contrastLevel);
}

void LM6029ACW_595::setBacklight(bool on) {
    _backlightOn = on;

    if (_spiFd < 0) return; // belum begin()

    // Kirim ulang state shift register yang sama, bit LED akan di-set ulang
    // oleh shiftOutDual74HC595() sesuai _backlightOn.
    shiftOutDual74HC595(_shiftRegState);
}

void LM6029ACW_595::setDisplayInvert(bool invert) {
    _displayInverted = invert;

    if (_spiFd < 0) return; // belum begin()

    sendCommand(invert ? LCD_DISPLAY_INVERSE : LCD_DISPLAY_NORMAL);
}

void LM6029ACW_595::setDisplayOn(bool on) {
    _displayOn = on;

    if (_spiFd < 0) return; // belum begin()

    sendCommand(on ? LCD_DISPLAY_ON : LCD_DISPLAY_OFF);
}

// Ubah kecepatan clock SPI saat program sedang jalan.
// Karena tiap transfer memakai tr.speed_hz = _spiSpeedHz, perubahan ini
// langsung berlaku pada transfer berikutnya (tidak perlu begin() ulang).
void LM6029ACW_595::setSpiSpeed(uint32_t hz) {
    if (hz < 100000) return; // abaikan nilai tidak masuk akal
    _spiSpeedHz = hz;

    if (_spiFd < 0) return; // belum begin()

    uint32_t speed = _spiSpeedHz;
    ioctl(_spiFd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
}
