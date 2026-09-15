#ifndef LM6029ACW_595_H
#define LM6029ACW_595_H

#include <stdint.h>
#include <unistd.h>
#include <string>
#include "Adafruit_GFX.h" // Header Adafruit_GFX C++

#define LCD_WIDTH   128
#define LCD_HEIGHT  64
#define LCD_PAGES   8

// Control bit mask 74HC595 ke-2
#define PIN_RD  (1 << 7)
#define PIN_WR  (1 << 6)
#define PIN_RS  (1 << 5)
#define PIN_RES (1 << 4)
#define PIN_CS  (1 << 3)
#define PIN_LED (1 << 2)

// Command LCD
#define LCD_DISPLAY_ON            0xAF
#define LCD_DISPLAY_ALL_POINT_OFF 0xA4
#define LCD_BIAS_1_9              0xA2
#define LCD_START_LINE            0x40
#define LCD_SET_PAGE              0xB0
#define LCD_SET_COL_MSB           0x10
#define LCD_SET_COL_LSB           0x00
#define LCD_ADC_NORMAL            0xA0
#define LCD_COM_NORMAL            0xC8
#define LCD_POWER_CTRL            0x2F
#define LCD_SET_EVR               0x81
#define LCD_DISPLAY_OFF           0xAE
#define LCD_DISPLAY_NORMAL        0xA6
#define LCD_DISPLAY_INVERSE       0xA7

class LM6029ACW_595 : public Adafruit_GFX {
public:
    LM6029ACW_595();
    ~LM6029ACW_595();

    bool begin(uint32_t speedHz = 10000000);
    void clearDisplay();
    void display();
    void drawPixel(int16_t x, int16_t y, uint16_t color) override;
    
    // Fungsi untuk cetak string sederhana
    void printText(const char* str, int16_t x, int16_t y, uint8_t size = 1);

    // --- Kontrol tampilan ---
    // Kontras (Electronic Volume Register), range 0..63. Default driver: 0x1F (31).
    void setContrast(uint8_t level);
    uint8_t getContrast() const { return _contrastLevel; }

    // Backlight (pin LED pada 74HC595). Untuk hemat daya / meredupkan layar.
    void setBacklight(bool on);
    bool getBacklight() const { return _backlightOn; }

    // Inversi seluruh tampilan (piksel nyala <-> mati)
    void setDisplayInvert(bool invert);
    bool getDisplayInvert() const { return _displayInverted; }

    // Display on/off (0xAF / 0xAE). Isi buffer tetap aman saat display dimatikan.
    void setDisplayOn(bool on);
    bool isDisplayOn() const { return _displayOn; }

    // --- Kecepatan SPI ---
    // Di Raspberry Pi (bcm2835), clock SPI = core clock / 2^CDIV, jadi kecepatan
    // efektif hanya bisa melompat dalam pangkat dua. Dengan core 250 MHz pilihannya:
    //   8 MHz  -> 7.8125 MHz real
    //  16 MHz  -> 15.625  MHz real
    //  32 MHz  -> 31.25   MHz real
    //  64 MHz  -> 62.5    MHz real
    // Jadi minta 32 MHz, bukan 25 MHz, kalau mau dapat 31.25 MHz.
    void setSpiSpeed(uint32_t hz);
    uint32_t getSpiSpeed() const { return _spiSpeedHz; }

    // --- Bus SPI (portabilitas antar-SBC) ---
    // Path spidev TIDAK sama antar board:
    //   Raspberry Pi  -> /dev/spidev0.0
    //   Orange Pi     -> /dev/spidev3.0
    //   board lain    -> /dev/spidev1.0, /dev/spidev2.0, dst.
    // Default "" = auto-deteksi saat begin(): coba preferensi -> /dev/spidev0.0
    // -> sisa /dev/spidev* (urut lexicographic) dan pakai yang pertama bisa
    // dibuka. Jadi satu binary yang sama jalan di Pi maupun Orange Pi.
    void setSpiDevice(const char* path);
    /** Bus yang benar-benar dipakai begin() ("" sebelum begin sukses). */
    const char* getSpiDevicePath() const { return _spiDevicePath.c_str(); }
    /** Rekap percobaan open terakhir (mis. "/dev/spidev0.0 gagal, /dev/spidev3.0 ok"). */
    const char* getSpiProbeLog() const { return _spiProbeLog.c_str(); }

private:
    int _spiFd;
    uint16_t _shiftRegState;
    uint8_t _displayBuffer[LCD_WIDTH * LCD_PAGES];
    uint8_t _contrastLevel;
    bool _backlightOn;
    bool _displayInverted;
    bool _displayOn;
    uint32_t _spiSpeedHz;
    std::string _spiDevicePref; // diminta user/env ("" = auto)
    std::string _spiDevicePath; // hasil resolusi saat begin()
    std::string _spiProbeLog;   // jejak percobaan open untuk log/error

    int openSpiBus();
    void shiftOutDual74HC595(uint16_t value);
    void sendCommand(uint8_t cmd);
    void sendData(uint8_t data);
    void setControlPins(uint8_t pins);
};

#endif
