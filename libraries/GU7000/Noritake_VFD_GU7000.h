#ifndef NORITAKE_VFD_GU7000_H
#define NORITAKE_VFD_GU7000_H

#include <stdint.h>
#include <stddef.h>
#include <util/delay.h>

enum ImageMemoryArea {
    FlashImageArea = 1,
    ScreenImageArea = 2
};

enum ScrollMode {
    WrappingMode =    1,
    VertScrollMode =  2,
    HorizScrollMode = 3
};

enum CompositionMode {
    NormalCompositionMode = 0,
    OrCompositionMode =     1,
    AndCompositionMode =    2,
    XorCompositionMode =    3
};

enum ScreenSaver {
    AllDotsOffSaver = 2,
    AllDotsOnSaver =  3,
    InvertSaver =   4
};

enum LEDColor {
    NoLight =       0x000,
    BlueLight =     0x00f,
    GreenLight =    0x0f0,
    CyanLight =     0x0ff,
    RedLight =      0xf00,
    MagentaLight =  0xf0f,
    SmokeLight =    0xfff
};

enum FontFormat {
     GU70005x7Format =0,
     GU70007x8Format =1,
     CUUFormat =    2,
     LCDFormat =    CUUFormat     
};



class Noritake_VFD_GU7000 {

    void initialState();
    void printNumber(unsigned long number, uint8_t base);
    void printNumber(unsigned x, uint8_t y, unsigned long number, uint8_t base);
    void command(uint8_t data);
    void us_command(uint8_t group, uint8_t cmd);
    void command(uint8_t prefix, uint8_t group, uint8_t cmd);
    void command_xy(unsigned x, unsigned y);
    void command_xy1(unsigned x, unsigned y);
    void crlf();
    
    GU7000_Interface *io;    

public:
    uint8_t width;
    uint8_t height;
    uint8_t lines;
    unsigned modelClass;
    bool	generation; // GU-7***B = true
   

    Noritake_VFD_GU7000() {
        this->modelClass = 7000;
        this->generation = false;
    }

    void interface(GU7000_Interface &interface) {
        io = &interface;
		//io->init();
    }
    
    void begin(uint8_t width, uint8_t height) {
        this->width = width;
        this->height = height;
        this->lines = this->height / 8;
    }
    
    void isModelClass(unsigned modelClass) {
        this->modelClass = modelClass;
		io->getModelClass = modelClass;
    }

    
    void isGeneration(char c) {
        this->generation = toupper(c) == 'B';
    }

    void GU7000_back();
    void GU7000_forward();
    void GU7000_lineFeed();
    void GU7000_home();
    void GU7000_carriageReturn();
    void GU7000_setCursor(unsigned x, unsigned y);
    void GU7000_clearScreen();
    void GU7000_cursorOn();
    void GU7000_cursorOff();
    void GU7000_init();
    void GU7000_reset();
    void GU7000_useMultibyteChars(bool enable);
    void GU7000_setMultibyteCharset(uint8_t code);
    void GU7000_useCustomChars(bool enable);
    void GU7000_defineCustomChar(uint8_t code, FontFormat format, const uint8_t *data);
    void GU7000_deleteCustomChar(uint8_t code);
    void GU7000_setAsciiVariant(uint8_t code);
    void GU7000_setCharset(uint8_t code);
    void GU7000_setScrollMode(ScrollMode mode);
    void GU7000_setHorizScrollSpeed(uint8_t speed);
    void GU7000_invertOn();
    void GU7000_invertOff();
    void GU7000_setCompositionMode(CompositionMode mode);
    void GU7000_setScreenBrightness(unsigned level);
    void GU7000_wait(uint8_t time);
    void GU7000_scrollScreen(unsigned x, unsigned y, unsigned count, uint8_t speed);
    void GU7000_blinkScreen();
    void GU7000_blinkScreen(bool enable, bool reverse, uint8_t on, uint8_t off, uint8_t times);
    void GU7000_displayOn();
    void GU7000_displayOff();
    void GU7000_screenSaver(ScreenSaver mode);
    void GU7000_drawImage(unsigned width, uint8_t height, const uint8_t *data);
    void GU7000_drawFROMImage(unsigned long address, uint8_t srcHeight, unsigned width, uint8_t height);
    void GU7000_setFontStyle(bool proportional, bool evenSpacing);
    void GU7000_setFontSize(uint8_t x, uint8_t y, bool tall);
    void GU7000_selectWindow(uint8_t window);
    void GU7000_defineWindow(uint8_t window, unsigned x, unsigned y, unsigned width, unsigned height);
    void GU7000_deleteWindow(uint8_t window);
    void GU7000_joinScreens();
    void GU7000_separateScreens();
    void GU7000_setBacklightColor(uint8_t r, uint8_t g, uint8_t b);
    void GU7000_setBacklightColor(unsigned rgb);

    void clear();
    void setCursor(unsigned x, unsigned y);

    void print(char c);
    void print(const char *str);
    void print(const char *buffer, size_t size);
    void print(int number, uint8_t base);
    void print(unsigned number, uint8_t base);
    void print(long number, uint8_t base);
    void print(unsigned long number, uint8_t base);
    void println(char c);
    void println(const char *str);
    void println(const char *buffer, size_t size);
    void println(int number, uint8_t base);
    void println(unsigned number, uint8_t base);
    void println(long number, uint8_t base);
    void println(unsigned long number, uint8_t base);

    void GU7000_fillRect(unsigned x0, unsigned y0, unsigned x1, unsigned y1, bool on=true);
    
    void print(unsigned x, uint8_t y, char c);
    void print(unsigned x, uint8_t y, const char *str);
    void print(unsigned x, uint8_t y, const char *buffer, uint8_t len);
    void print(unsigned x, uint8_t y, int number, uint8_t base);
    void print(unsigned x, uint8_t y, unsigned number, uint8_t base);
    void GU7000_drawImage(unsigned x, uint8_t y, unsigned width, uint8_t height, const uint8_t *data);
    void GU7000_drawImage(unsigned x, uint8_t y, ImageMemoryArea area, unsigned long address, uint8_t srcHeight, unsigned width, uint8_t height, unsigned offsetx, unsigned offsety);
    void GU7000_drawImage(unsigned x, uint8_t y, ImageMemoryArea area, unsigned long address, unsigned width, uint8_t height);
    
    void print_p(const char *str);
    void print_p(unsigned x, uint8_t y, const char *str);
    void print_p(unsigned x, uint8_t y, const char *buffer, uint8_t len);
    void GU7000_drawImage_p(unsigned width, uint8_t height, const uint8_t *data);
    void GU7000_drawImage_p(unsigned x, uint8_t y, unsigned width, uint8_t height, const uint8_t *data);
};
#endif
