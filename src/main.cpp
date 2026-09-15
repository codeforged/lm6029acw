#include <napi.h>
#include "LM6029ACW_595.h"

// Include font-font Adafruit yang mau digunakan
#include "Fonts/FreeSans9pt7b.h"
#include "Fonts/FreeSansBold12pt7b.h"
#include "Fonts/FreeMono9pt7b.h"

class LCDAddon : public Napi::ObjectWrap<LCDAddon> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports) {
        Napi::Function func = DefineClass(env, "LM6029LCD", {
            InstanceMethod("begin", &LCDAddon::Begin),
            InstanceMethod("clear", &LCDAddon::ClearDisplay),
            InstanceMethod("clearDisplay", &LCDAddon::ClearDisplay),
            InstanceMethod("display", &LCDAddon::Display),
            InstanceMethod("drawPixel", &LCDAddon::DrawPixel),
            InstanceMethod("getWidth", &LCDAddon::GetWidth),
            InstanceMethod("getHeight", &LCDAddon::GetHeight),
            
            // --- Primitive GFX ---
            InstanceMethod("fillScreen", &LCDAddon::FillScreen),
            InstanceMethod("drawLine", &LCDAddon::DrawLine),
            InstanceMethod("drawRect", &LCDAddon::DrawRect),
            InstanceMethod("fillRect", &LCDAddon::FillRect),
            InstanceMethod("drawCircle", &LCDAddon::DrawCircle),
            InstanceMethod("fillCircle", &LCDAddon::FillCircle),
            InstanceMethod("drawTriangle", &LCDAddon::DrawTriangle),
            InstanceMethod("fillTriangle", &LCDAddon::FillTriangle),
            InstanceMethod("drawRoundRect", &LCDAddon::DrawRoundRect),
            InstanceMethod("fillRoundRect", &LCDAddon::FillRoundRect),
            
            // --- Text & Custom Font Adafruit ---
            InstanceMethod("setFont", &LCDAddon::SetFont),
            InstanceMethod("setTextColor", &LCDAddon::SetTextColor),
            InstanceMethod("setTextSize", &LCDAddon::SetTextSize),
            InstanceMethod("setTextWrap", &LCDAddon::SetTextWrap),
            InstanceMethod("setCursor", &LCDAddon::SetCursor),
            InstanceMethod("print", &LCDAddon::Print),
            InstanceMethod("printText", &LCDAddon::PrintText),
            
            // --- Advanced Features ---
            InstanceMethod("setRotation", &LCDAddon::SetRotation),
            InstanceMethod("drawBitmap", &LCDAddon::DrawBitmap),

            // --- Kontrol Tampilan ---
            InstanceMethod("setContrast", &LCDAddon::SetContrast),
            InstanceMethod("getContrast", &LCDAddon::GetContrast),
            InstanceMethod("setBacklight", &LCDAddon::SetBacklight),
            InstanceMethod("getBacklight", &LCDAddon::GetBacklight),
            InstanceMethod("setDisplayInvert", &LCDAddon::SetDisplayInvert),
            InstanceMethod("getDisplayInvert", &LCDAddon::GetDisplayInvert),
            InstanceMethod("setDisplayOn", &LCDAddon::SetDisplayOn),
            InstanceMethod("isDisplayOn", &LCDAddon::IsDisplayOn),
            InstanceMethod("setSpiSpeed", &LCDAddon::SetSpiSpeed),
            InstanceMethod("getSpiSpeed", &LCDAddon::GetSpiSpeed),

            // --- Bus SPI (portabilitas antar-SBC) ---
            InstanceMethod("setSpiDevice", &LCDAddon::SetSpiDevice),
            InstanceMethod("getSpiDevicePath", &LCDAddon::GetSpiDevicePath),
            InstanceMethod("getSpiProbeLog", &LCDAddon::GetSpiProbeLog)
        });

        exports.Set("LM6029LCD", func);
        return exports;
    }

    LCDAddon(const Napi::CallbackInfo& info) : Napi::ObjectWrap<LCDAddon>(info) {
        this->driver = new LM6029ACW_595();
    }

    ~LCDAddon() {
        delete this->driver;
    }

private:
    LM6029ACW_595* driver;

    Napi::Value Begin(const Napi::CallbackInfo& info) {
        uint32_t speedHz = 0; // 0 = pakai default driver (10 MHz)

        // Argumen fleksibel, urutan bebas:
        //   begin()                          -> auto-deteksi bus + 10 MHz
        //   begin(32000000)                  -> auto-deteksi bus + 32 MHz
        //   begin('/dev/spidev3.0')          -> paksa bus (Orange Pi)
        //   begin(32000000, '/dev/spidev3.0') -> keduanya
        for (size_t i = 0; i < info.Length(); i++) {
            if (info[i].IsNumber() && speedHz == 0) {
                speedHz = info[i].As<Napi::Number>().Uint32Value();
            } else if (info[i].IsString()) {
                this->driver->setSpiDevice(info[i].As<Napi::String>().Utf8Value().c_str());
            }
        }
        return Napi::Boolean::New(info.Env(), this->driver->begin(speedHz));
    }

    // setSpiDevice(path) -> paksa bus SPI tertentu (mis. '/dev/spidev3.0').
    // Berlaku pada begin() berikutnya; '' = kembali ke auto-deteksi.
    Napi::Value SetSpiDevice(const Napi::CallbackInfo& info) {
        if (info.Length() > 0 && info[0].IsString()) {
            this->driver->setSpiDevice(info[0].As<Napi::String>().Utf8Value().c_str());
        } else {
            this->driver->setSpiDevice(NULL);
        }
        return info.Env().Undefined();
    }

    // getSpiDevicePath() -> bus yang benar-benar dipakai ('' bila belum begin).
    Napi::Value GetSpiDevicePath(const Napi::CallbackInfo& info) {
        return Napi::String::New(info.Env(), this->driver->getSpiDevicePath());
    }

    // getSpiProbeLog() -> jejak percobaan open, mis.
    // '/dev/spidev0.0 gagal, /dev/spidev3.0 ok'.
    Napi::Value GetSpiProbeLog(const Napi::CallbackInfo& info) {
        return Napi::String::New(info.Env(), this->driver->getSpiProbeLog());
    }

    Napi::Value ClearDisplay(const Napi::CallbackInfo& info) {
        this->driver->clearDisplay();
        return info.Env().Undefined();
    }

    Napi::Value Display(const Napi::CallbackInfo& info) {
        this->driver->display();
        return info.Env().Undefined();
    }

    Napi::Value DrawPixel(const Napi::CallbackInfo& info) {
        this->driver->drawPixel(info[0].As<Napi::Number>().Int32Value(),
                                info[1].As<Napi::Number>().Int32Value(),
                                info[2].As<Napi::Number>().Int32Value());
        return info.Env().Undefined();
    }

    Napi::Value GetWidth(const Napi::CallbackInfo& info) {
        return Napi::Number::New(info.Env(), this->driver->width());
    }

    Napi::Value GetHeight(const Napi::CallbackInfo& info) {
        return Napi::Number::New(info.Env(), this->driver->height());
    }

    Napi::Value FillScreen(const Napi::CallbackInfo& info) {
        this->driver->fillScreen(info[0].As<Napi::Number>().Uint32Value());
        return info.Env().Undefined();
    }

    Napi::Value DrawLine(const Napi::CallbackInfo& info) {
        this->driver->drawLine(info[0].As<Napi::Number>().Int32Value(),
                               info[1].As<Napi::Number>().Int32Value(),
                               info[2].As<Napi::Number>().Int32Value(),
                               info[3].As<Napi::Number>().Int32Value(),
                               info[4].As<Napi::Number>().Int32Value());
        return info.Env().Undefined();
    }

    Napi::Value DrawRect(const Napi::CallbackInfo& info) {
        this->driver->drawRect(info[0].As<Napi::Number>().Int32Value(),
                               info[1].As<Napi::Number>().Int32Value(),
                               info[2].As<Napi::Number>().Int32Value(),
                               info[3].As<Napi::Number>().Int32Value(),
                               info[4].As<Napi::Number>().Int32Value());
        return info.Env().Undefined();
    }

    Napi::Value FillRect(const Napi::CallbackInfo& info) {
        this->driver->fillRect(info[0].As<Napi::Number>().Int32Value(),
                               info[1].As<Napi::Number>().Int32Value(),
                               info[2].As<Napi::Number>().Int32Value(),
                               info[3].As<Napi::Number>().Int32Value(),
                               info[4].As<Napi::Number>().Int32Value());
        return info.Env().Undefined();
    }

    Napi::Value DrawCircle(const Napi::CallbackInfo& info) {
        this->driver->drawCircle(info[0].As<Napi::Number>().Int32Value(),
                                 info[1].As<Napi::Number>().Int32Value(),
                                 info[2].As<Napi::Number>().Int32Value(),
                                 info[3].As<Napi::Number>().Int32Value());
        return info.Env().Undefined();
    }

    Napi::Value FillCircle(const Napi::CallbackInfo& info) {
        this->driver->fillCircle(info[0].As<Napi::Number>().Int32Value(),
                                 info[1].As<Napi::Number>().Int32Value(),
                                 info[2].As<Napi::Number>().Int32Value(),
                                 info[3].As<Napi::Number>().Int32Value());
        return info.Env().Undefined();
    }

    Napi::Value DrawTriangle(const Napi::CallbackInfo& info) {
        this->driver->drawTriangle(info[0].As<Napi::Number>().Int32Value(), info[1].As<Napi::Number>().Int32Value(),
                                   info[2].As<Napi::Number>().Int32Value(), info[3].As<Napi::Number>().Int32Value(),
                                   info[4].As<Napi::Number>().Int32Value(), info[5].As<Napi::Number>().Int32Value(),
                                   info[6].As<Napi::Number>().Int32Value());
        return info.Env().Undefined();
    }

    Napi::Value FillTriangle(const Napi::CallbackInfo& info) {
        this->driver->fillTriangle(info[0].As<Napi::Number>().Int32Value(), info[1].As<Napi::Number>().Int32Value(),
                                   info[2].As<Napi::Number>().Int32Value(), info[3].As<Napi::Number>().Int32Value(),
                                   info[4].As<Napi::Number>().Int32Value(), info[5].As<Napi::Number>().Int32Value(),
                                   info[6].As<Napi::Number>().Int32Value());
        return info.Env().Undefined();
    }

    Napi::Value DrawRoundRect(const Napi::CallbackInfo& info) {
        this->driver->drawRoundRect(info[0].As<Napi::Number>().Int32Value(), info[1].As<Napi::Number>().Int32Value(),
                                    info[2].As<Napi::Number>().Int32Value(), info[3].As<Napi::Number>().Int32Value(),
                                    info[4].As<Napi::Number>().Int32Value(), info[5].As<Napi::Number>().Int32Value());
        return info.Env().Undefined();
    }

    Napi::Value FillRoundRect(const Napi::CallbackInfo& info) {
        this->driver->fillRoundRect(info[0].As<Napi::Number>().Int32Value(), info[1].As<Napi::Number>().Int32Value(),
                                    info[2].As<Napi::Number>().Int32Value(), info[3].As<Napi::Number>().Int32Value(),
                                    info[4].As<Napi::Number>().Int32Value(), info[5].As<Napi::Number>().Int32Value());
        return info.Env().Undefined();
    }

    // --- PILIH FONT ADAFRUIT ---
    Napi::Value SetFont(const Napi::CallbackInfo& info) {
        int fontId = info[0].As<Napi::Number>().Int32Value();
        switch (fontId) {
            case 1:  this->driver->setFont(&FreeSans9pt7b); break;
            case 2:  this->driver->setFont(&FreeSansBold12pt7b); break;
            case 3:  this->driver->setFont(&FreeMono9pt7b); break;
            default: this->driver->setFont(NULL); break; // Default 5x7 Font
        }
        return info.Env().Undefined();
    }

    Napi::Value SetTextColor(const Napi::CallbackInfo& info) {
        uint16_t color = static_cast<uint16_t>(info[0].As<Napi::Number>().Int32Value());
        if (info.Length() > 1 && info[1].IsNumber()) {
            this->driver->setTextColor(color, static_cast<uint16_t>(info[1].As<Napi::Number>().Int32Value()));
        } else {
            this->driver->setTextColor(color);
        }
        return info.Env().Undefined();
    }

    Napi::Value SetTextSize(const Napi::CallbackInfo& info) {
        this->driver->setTextSize(info[0].As<Napi::Number>().Int32Value());
        return info.Env().Undefined();
    }

    Napi::Value SetTextWrap(const Napi::CallbackInfo& info) {
        this->driver->setTextWrap(info[0].As<Napi::Boolean>().Value());
        return info.Env().Undefined();
    }

    Napi::Value SetCursor(const Napi::CallbackInfo& info) {
        this->driver->setCursor(info[0].As<Napi::Number>().Int32Value(),
                                info[1].As<Napi::Number>().Int32Value());
        return info.Env().Undefined();
    }

    Napi::Value Print(const Napi::CallbackInfo& info) {
        std::string str = info[0].As<Napi::String>().Utf8Value();
        for (char c : str) {
            this->driver->write(c);
        }
        return info.Env().Undefined();
    }

    // printText(str, x, y, size = 1) -> cetak sekali di posisi tertentu
    Napi::Value PrintText(const Napi::CallbackInfo& info) {
        std::string str = info[0].As<Napi::String>().Utf8Value();
        int16_t x = static_cast<int16_t>(info[1].As<Napi::Number>().Int32Value());
        int16_t y = static_cast<int16_t>(info[2].As<Napi::Number>().Int32Value());
        uint8_t size = (info.Length() > 3 && info[3].IsNumber())
                           ? static_cast<uint8_t>(info[3].As<Napi::Number>().Int32Value())
                           : 1;
        this->driver->printText(str.c_str(), x, y, size);
        return info.Env().Undefined();
    }

    Napi::Value SetRotation(const Napi::CallbackInfo& info) {
        this->driver->setRotation(info[0].As<Napi::Number>().Int32Value());
        return info.Env().Undefined();
    }

    Napi::Value DrawBitmap(const Napi::CallbackInfo& info) {
        int x = info[0].As<Napi::Number>().Int32Value();
        int y = info[1].As<Napi::Number>().Int32Value();
        Napi::Buffer<uint8_t> buf = info[2].As<Napi::Buffer<uint8_t>>();
        int w = info[3].As<Napi::Number>().Int32Value();
        int h = info[4].As<Napi::Number>().Int32Value();
        int color = info[5].As<Napi::Number>().Int32Value();

        this->driver->drawBitmap(x, y, buf.Data(), w, h, color);
        return info.Env().Undefined();
    }

    // --- Kontrol Tampilan ---

    // setContrast(level 0..63) -> nilai yang benar-benar dipakai
    Napi::Value SetContrast(const Napi::CallbackInfo& info) {
        int level = info[0].As<Napi::Number>().Int32Value();
        if (level < 0) level = 0;
        if (level > 63) level = 63;
        this->driver->setContrast(static_cast<uint8_t>(level));
        return Napi::Number::New(info.Env(), static_cast<int>(this->driver->getContrast()));
    }

    Napi::Value GetContrast(const Napi::CallbackInfo& info) {
        return Napi::Number::New(info.Env(), static_cast<int>(this->driver->getContrast()));
    }

    Napi::Value SetBacklight(const Napi::CallbackInfo& info) {
        this->driver->setBacklight(info[0].As<Napi::Boolean>().Value());
        return Napi::Boolean::New(info.Env(), this->driver->getBacklight());
    }

    Napi::Value GetBacklight(const Napi::CallbackInfo& info) {
        return Napi::Boolean::New(info.Env(), this->driver->getBacklight());
    }

    Napi::Value SetDisplayInvert(const Napi::CallbackInfo& info) {
        this->driver->setDisplayInvert(info[0].As<Napi::Boolean>().Value());
        return Napi::Boolean::New(info.Env(), this->driver->getDisplayInvert());
    }

    Napi::Value GetDisplayInvert(const Napi::CallbackInfo& info) {
        return Napi::Boolean::New(info.Env(), this->driver->getDisplayInvert());
    }

    Napi::Value SetDisplayOn(const Napi::CallbackInfo& info) {
        this->driver->setDisplayOn(info[0].As<Napi::Boolean>().Value());
        return Napi::Boolean::New(info.Env(), this->driver->isDisplayOn());
    }

    Napi::Value IsDisplayOn(const Napi::CallbackInfo& info) {
        return Napi::Boolean::New(info.Env(), this->driver->isDisplayOn());
    }

    // setSpiSpeed(Hz) -> nilai yang dipakai driver
    Napi::Value SetSpiSpeed(const Napi::CallbackInfo& info) {
        uint32_t hz = info[0].As<Napi::Number>().Uint32Value();
        this->driver->setSpiSpeed(hz);
        return Napi::Number::New(info.Env(), static_cast<double>(this->driver->getSpiSpeed()));
    }

    Napi::Value GetSpiSpeed(const Napi::CallbackInfo& info) {
        return Napi::Number::New(info.Env(), static_cast<double>(this->driver->getSpiSpeed()));
    }
};

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
    return LCDAddon::Init(env, exports);
}

NODE_API_MODULE(lcd_lm6029, InitAll)