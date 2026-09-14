#include "LM6029ACW_595.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <cstring>
#include <iostream>

// Sesudah (Fix):
LM6029ACW_595::LM6029ACW_595() : Adafruit_GFX(LCD_WIDTH, LCD_HEIGHT) {
    _spiFd = -1;
    _shiftRegState = 0;
    _contrastLevel = 0x1F; // default ~31 dari 63
    _backlightOn = true;
    _displayInverted = false;
    _displayOn = true;
    _spiSpeedHz = 10000000; // 10 MHz
}

LM6029ACW_595::~LM6029ACW_595() {
    if (_spiFd >= 0) close(_spiFd);
}

bool LM6029ACW_595::begin(uint32_t speedHz) {
    // Nilai di bawah 100 kHz dianggap tidak masuk akal (mis. sisa pemanggilan
    // begin() versi lama dengan argumen pin) -> abaikan.
    if (speedHz >= 100000) _spiSpeedHz = speedHz;

    // Buka device SPI Hardware Raspi
    _spiFd = open("/dev/spidev0.0", O_RDWR);
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
void LM6029ACW_595::printText(const char* str, int16_t x, int16_t y, uint8_t size) {
    if (str == NULL) return;

    setTextSize(size);
    setTextColor(1); // piksel nyala, background transparan (bg == fg)
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
