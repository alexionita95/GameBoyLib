#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include<hardware/HardwareSetupConfig.h>

#ifndef TAB_COLOUR
  #define TAB_COLOUR 0
#endif

// If the frequency is not defined, set a default
#ifndef SPI_FREQUENCY
  #define SPI_FREQUENCY  20000000
#endif

// If the frequency is not defined, set a default
#ifndef SPI_READ_FREQUENCY
  #define SPI_READ_FREQUENCY SPI_FREQUENCY
#endif

#ifdef ST7789_DRIVER
  #define TFT_SPI_MODE SPI_MODE3
#else
  #define TFT_SPI_MODE SPI_MODE0
#endif


// Only load the fonts defined in User_Setup.h (to save space)
// Set flag so RLE rendering code is optionally compiled
#ifdef LOAD_GLCD
  #include <Fonts/glcdfont.c>
#endif

#ifdef LOAD_FONT2
  #include <Fonts/Font16.h>
#endif

#ifdef LOAD_FONT4
  #include <Fonts/Font32rle.h>
  #define LOAD_RLE
#endif

#ifdef LOAD_FONT6
  #include <Fonts/Font64rle.h>
  #ifndef LOAD_RLE
    #define LOAD_RLE
  #endif
#endif

#ifdef LOAD_FONT7
  #include <Fonts/Font7srle.h>
  #ifndef LOAD_RLE
    #define LOAD_RLE
  #endif
#endif

#ifdef LOAD_FONT8
  #include <Fonts/Font72rle.h>
  #ifndef LOAD_RLE
    #define LOAD_RLE
  #endif
#elif defined LOAD_FONT8N
  #define LOAD_FONT8
  #include <Fonts/Font72x53rle.h>
  #ifndef LOAD_RLE
    #define LOAD_RLE
  #endif
#endif

#include <Arduino.h>
//#include <Print.h>

#include <pgmspace.h>

#include <SPI.h>


#ifndef TFT_DC
  #define DC_C // No macro allocated so it generates no code
  #define DC_D // No macro allocated so it generates no code
#else
  #if defined (ESP8266) && (TFT_DC == 16)
    #define DC_C digitalWrite(TFT_DC, LOW)
    #define DC_D digitalWrite(TFT_DC, HIGH)
  #else
    #define DC_C GPOC=dcpinmask
    #define DC_D GPOS=dcpinmask
  #endif
#endif

#if defined (TFT_SPI_OVERLAP)
  #undef TFT_CS
  #define SPI1U_WRITE (SPIUMOSI | SPIUSSE | SPIUCSSETUP | SPIUCSHOLD)
  #define SPI1U_READ  (SPIUMOSI | SPIUSSE | SPIUCSSETUP | SPIUCSHOLD | SPIUDUPLEX)
#else
  #ifdef ESP8266
    #define SPI1U_WRITE (SPIUMOSI | SPIUSSE)
    #define SPI1U_READ  (SPIUMOSI | SPIUSSE | SPIUDUPLEX)
  #endif
#endif


#ifndef TFT_CS
  #define CS_L // No macro allocated so it generates no code
  #define CS_H // No macro allocated so it generates no code
#else
  #if defined (ESP8266) && (TFT_CS == 16)
    #define CS_L digitalWrite(TFT_CS, LOW)
    #define CS_H digitalWrite(TFT_CS, HIGH)
  #else
    #define CS_L GPOC=cspinmask
    #define CS_H GPOS=cspinmask
  #endif
#endif

#define CS_L_DC_C CS_L; DC_C

#ifdef ESP8266
  // Concatenate two 16 bit values for the SPI 32 bit register write
  #define SPI_32(H,L) ( (H)<<16 | (L) )
  #define COL_32(H,L) ( (H)<<16 | (L) )
#endif

#if defined ESP8266

  #define tft_Write_8(C)   spi.write(C)
  #define tft_Write_16(C)  spi.write16(C)
  #define tft_Write_32(C)  spi.write32(C)

  #endif

  #if defined (ESP8266) && defined (TFT_SDA_READ)
    // Use a bit banged function call for ESP8266 and bi-directional SDA pin
    #define SCLK_L GPOC=sclkpinmask
    #define SCLK_H GPOS=sclkpinmask
  #else
    // Use a SPI read transfer
    #define tft_Read_8() spi.transfer(0)
  #endif

  // Make sure TFT_MISO is defined if not used to avoid an error message
  #ifndef TFT_MISO
    #define TFT_MISO -1
  #endif

//These enumerate the text plotting alignment (reference datum point)
#define TL_DATUM 0 // Top left (default)
#define TC_DATUM 1 // Top centre
#define TR_DATUM 2 // Top right
#define ML_DATUM 3 // Middle left
#define CL_DATUM 3 // Centre left, same as above
#define MC_DATUM 4 // Middle centre
#define CC_DATUM 4 // Centre centre, same as above
#define MR_DATUM 5 // Middle right
#define CR_DATUM 5 // Centre right, same as above
#define BL_DATUM 6 // Bottom left
#define BC_DATUM 7 // Bottom centre
#define BR_DATUM 8 // Bottom right
#define L_BASELINE  9 // Left character baseline (Line the 'A' character would sit on)
#define C_BASELINE 10 // Centre character baseline
#define R_BASELINE 11 // Right character baseline


// New color definitions use for all my libraries
#define TFT_BLACK       0x0000      /*   0,   0,   0 */
#define TFT_NAVY        0x000F      /*   0,   0, 128 */
#define TFT_DARKGREEN   0x03E0      /*   0, 128,   0 */
#define TFT_DARKCYAN    0x03EF      /*   0, 128, 128 */
#define TFT_MAROON      0x7800      /* 128,   0,   0 */
#define TFT_PURPLE      0x780F      /* 128,   0, 128 */
#define TFT_OLIVE       0x7BE0      /* 128, 128,   0 */
#define TFT_LIGHTGREY   0xC618      /* 192, 192, 192 */
#define TFT_DARKGREY    0x7BEF      /* 128, 128, 128 */
#define TFT_BLUE        0x001F      /*   0,   0, 255 */
#define TFT_GREEN       0x07E0      /*   0, 255,   0 */
#define TFT_CYAN        0x07FF      /*   0, 255, 255 */
#define TFT_RED         0xF800      /* 255,   0,   0 */
#define TFT_MAGENTA     0xF81F      /* 255,   0, 255 */
#define TFT_YELLOW      0xFFE0      /* 255, 255,   0 */
#define TFT_WHITE       0xFFFF      /* 255, 255, 255 */
#define TFT_ORANGE      0xFDA0      /* 255, 180,   0 */
#define TFT_GREENYELLOW 0xB7E0      /* 180, 255,   0 */
#define TFT_PINK        0xFC9F

// Next is a special 16 bit colour value that encodes to 8 bits
// and will then decode back to the same 16 bit value.
// Convenient for 8 bit and 16 bit transparent sprites.
#define TFT_TRANSPARENT 0x0120

// Swap any type
template <typename T> static inline void
swap_coord(T& a, T& b) { T t = a; a = b; b = t; }

#ifndef min
  // Return minimum of two numbers, may already be defined
  #define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

// Create a null set in case some fonts not used (to prevent crash)
const  uint8_t widtbl_null[1] = {0};
PROGMEM const uint8_t chr_null[1] = {0};
PROGMEM const uint8_t* const chrtbl_null[1] = {chr_null};

typedef struct {
    const uint8_t *chartbl;
    const uint8_t *widthtbl;
    uint8_t height;
    uint8_t baseline;
    } fontinfo;

// Now fill the structure
const PROGMEM fontinfo fontdata [] = {
  #ifdef LOAD_GLCD
   { (const uint8_t *)font, widtbl_null, 0, 0 },
  #else
   { (const uint8_t *)chrtbl_null, widtbl_null, 0, 0 },
  #endif
   // GLCD font (Font 1) does not have all parameters
   { (const uint8_t *)chrtbl_null, widtbl_null, 8, 7 },

  #ifdef LOAD_FONT2
   { (const uint8_t *)chrtbl_f16, widtbl_f16, chr_hgt_f16, baseline_f16},
  #else
   { (const uint8_t *)chrtbl_null, widtbl_null, 0, 0 },
  #endif

   // Font 3 current unused
   { (const uint8_t *)chrtbl_null, widtbl_null, 0, 0 },

  #ifdef LOAD_FONT4
   { (const uint8_t *)chrtbl_f32, widtbl_f32, chr_hgt_f32, baseline_f32},
  #else
   { (const uint8_t *)chrtbl_null, widtbl_null, 0, 0 },
  #endif

   // Font 5 current unused
   { (const uint8_t *)chrtbl_null, widtbl_null, 0, 0 },

  #ifdef LOAD_FONT6
   { (const uint8_t *)chrtbl_f64, widtbl_f64, chr_hgt_f64, baseline_f64},
  #else
   { (const uint8_t *)chrtbl_null, widtbl_null, 0, 0 },
  #endif

  #ifdef LOAD_FONT7
   { (const uint8_t *)chrtbl_f7s, widtbl_f7s, chr_hgt_f7s, baseline_f7s},
  #else
   { (const uint8_t *)chrtbl_null, widtbl_null, 0, 0 },
  #endif

  #ifdef LOAD_FONT8
   { (const uint8_t *)chrtbl_f72, widtbl_f72, chr_hgt_f72, baseline_f72}
  #else
   { (const uint8_t *)chrtbl_null, widtbl_null, 0, 0 }
  #endif
};


class DisplayDriver /*: public Print*/ {

public:

DisplayDriver(int16_t _W = TFT_WIDTH, int16_t _H = TFT_HEIGHT);
void init();

int16_t width(void)
{
  return _width;
}

int16_t height(void)
{
  return _height;
}



void setTextColor(uint16_t c);
void setTextColor(uint16_t c, uint16_t b);
void setTextSize(uint8_t s);
int16_t drawString(const String& string, int32_t poX, int32_t poY);
int16_t drawString(const String& string, int32_t poX, int32_t poY, uint8_t font);
int16_t drawString(const char *string, int32_t poX, int32_t poY);
int16_t drawString(const char *string, int32_t poX, int32_t poY, uint8_t font);
int16_t drawCentreString(const String& string, int32_t dX, int32_t poY, uint8_t font);
int16_t drawCentreString(const char *string, int32_t dX, int32_t poY, uint8_t font);
int16_t drawRightString(const String& string, int32_t dX, int32_t poY, uint8_t font);
int16_t drawRightString(const char *string, int32_t dX, int32_t poY, uint8_t font);
int16_t drawNumber(long long_num, int32_t poX, int32_t poY);
int16_t drawNumber(long long_num, int32_t poX, int32_t poY, uint8_t font);
int16_t drawFloat(float floatNumber, uint8_t dp, int32_t poX, int32_t poY);
int16_t drawFloat(float floatNumber, uint8_t dp, int32_t poX, int32_t poY, uint8_t font);
int16_t textWidth(const String& string);
int16_t textWidth(const String& string, uint8_t font);
int16_t textWidth(const char *string);
int16_t textWidth(const char *string, uint8_t font);
int16_t fontHeight(int16_t font);
int16_t fontHeight(void);
void drawChar(int32_t x, int32_t y, uint16_t c, uint32_t color, uint32_t bg, uint8_t size);
int16_t drawChar(uint16_t uniCode, int32_t x, int32_t y);
int16_t drawChar(uint16_t uniCode, int32_t x, int32_t y, uint8_t font);
uint16_t decodeUTF8(uint8_t c);
uint16_t decodeUTF8(uint8_t *buf, uint16_t *index, uint16_t remaining);
void  
   spiwrite(uint8_t),
   writecommand(uint8_t c),
   writedata(uint8_t d),
   setRotation(uint8_t r);
   
   void fillScreen(uint32_t color);
   void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
   void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
   void drawPixel(int32_t x, int32_t y, uint32_t color);
   void drawFastVLine(int32_t x, int32_t y, int32_t h, uint32_t color);
   void drawFastHLine(int32_t x, int32_t y, int32_t w, uint32_t color);

   void drawMatrix(uint8_t** matrix, int rows, int cols, int size,int x, int y);
   
   
   void setWindow(int32_t xs, int32_t ys, int32_t xe, int32_t ye);
  

int32_t  cursor_x, cursor_y, padX;
  uint32_t textcolor, textbgcolor;


  uint8_t  textfont,  // Current selected font
           textsize,  // Current font size multiplier
           textdatum, // Text reference datum
           rotation;  // Display rotation (0-3)

  int16_t _xpivot;   // x pivot point coordinate
  int16_t _ypivot;   // x pivot point coordinate

  uint8_t  decoderState = 0;   // UTF8 decoder state
  uint16_t decoderBuffer;      // Unicode code-point buffer

 private:

  inline void spi_begin() __attribute__((always_inline));
  inline void spi_end()   __attribute__((always_inline));

  inline void spi_begin_read() __attribute__((always_inline));
  inline void spi_end_read()   __attribute__((always_inline));

  void     readAddrWindow(int32_t xs, int32_t ys, int32_t w, int32_t h);

  uint8_t colstart = 0, rowstart = 0; // some ST7735 displays need this changed

  volatile uint32_t *dcport, *csport;
  
  uint32_t cspinmask, dcpinmask, sclkpinmask;




 protected:
  int32_t  _init_width, _init_height; // Display w/h as input, used by setRotation()
  int32_t  _width, _height;           // Display w/h as modified by current rotation
  int32_t  addr_row, addr_col;

  uint32_t fontsloaded;

  bool     isDigits;   // adjust bounding box for numbers to reduce visual jiggling
  bool     textwrapX, textwrapY;   // If set, 'wrap' text at right and optionally bottom edge of display
  bool     locked, inTransaction; // Transaction and mutex lock flags for ESP32

  bool     _booted;    // init() or begin() has already run once

 
 
};
#endif