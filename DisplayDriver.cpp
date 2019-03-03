#include <DisplayDriver.h>
  
  
SPIClass spi = SPIClass();

#define CMD_BITS 8-1

void writeBlock(uint16_t color, uint32_t repeat);

inline void DisplayDriver::spi_begin(void){
#if defined (SPI_HAS_TRANSACTION) && defined (SUPPORT_TRANSACTIONS) && !defined(ESP32_PARALLEL)
  if (locked) {locked = false; spi.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, TFT_SPI_MODE)); CS_L;}
#else
  CS_L;
#endif
#ifdef ESP8266
  SPI1U = SPI1U_WRITE;
#endif
}

inline void DisplayDriver::spi_end(void){
#if defined (SPI_HAS_TRANSACTION) && defined (SUPPORT_TRANSACTIONS) && !defined(ESP32_PARALLEL)
  if(!inTransaction) {if (!locked) {locked = true; CS_H; spi.endTransaction();}}
  #ifdef ESP8266
    SPI1U = SPI1U_READ;
  #endif
#else
  if(!inTransaction) CS_H;
#endif
}

inline void DisplayDriver::spi_begin_read(void){
#if defined (SPI_HAS_TRANSACTION) && defined (SUPPORT_TRANSACTIONS) && !defined(ESP32_PARALLEL)
  if (locked) {locked = false; spi.beginTransaction(SPISettings(SPI_READ_FREQUENCY, MSBFIRST, TFT_SPI_MODE)); CS_L;}
#else
  #if !defined(ESP32_PARALLEL)
    spi.setFrequency(SPI_READ_FREQUENCY);
  #endif
   CS_L;
#endif
#ifdef ESP8266
  SPI1U = SPI1U_READ;
#endif
}

inline void DisplayDriver::spi_end_read(void){
#if defined (SPI_HAS_TRANSACTION) && defined (SUPPORT_TRANSACTIONS) && !defined(ESP32_PARALLEL)
  if(!inTransaction) {if (!locked) {locked = true; CS_H; spi.endTransaction();}}
#else
  #if !defined(ESP32_PARALLEL)
    spi.setFrequency(SPI_FREQUENCY);
  #endif
   if(!inTransaction) CS_H;
#endif
#ifdef ESP8266
  SPI1U = SPI1U_WRITE;
#endif
}

void DisplayDriver::writecommand(uint8_t c)
{
  spi_begin(); // CS_L;

  DC_C;

  tft_Write_8(c);

  DC_D;

  spi_end();  // CS_H;

}


/***************************************************************************************
** Function name:           writedata
** Description:             Send a 8 bit data value to the TFT
***************************************************************************************/
void DisplayDriver::writedata(uint8_t d)
{
  spi_begin(); // CS_L;

  DC_D;        // Play safe, but should already be in data mode

  tft_Write_8(d);

  CS_L;        // Allow more hold time for low VDI rail

  spi_end();   // CS_H;
}


DisplayDriver::DisplayDriver(int16_t w, int16_t h)
{
	// The control pins are deliberately set to the inactive state (CS high) as setup()
// might call and initialise other SPI peripherals which would could cause conflicts
// if CS is floating or undefined.
#ifdef TFT_CS
  digitalWrite(TFT_CS, HIGH); // Chip select high (inactive)
  pinMode(TFT_CS, OUTPUT);
#endif

#ifdef TFT_DC
  digitalWrite(TFT_DC, HIGH); // Data/Command high = data mode
  pinMode(TFT_DC, OUTPUT);
#endif

#ifdef TFT_RST
  if (TFT_RST >= 0) {
    digitalWrite(TFT_RST, HIGH); // Set high, do not share pin with another SPI device
    pinMode(TFT_RST, OUTPUT);
  }
#endif

  _init_width  = _width  = w; // Set by specific xxxxx_Defines.h file or by users sketch
  _init_height = _height = h; // Set by specific xxxxx_Defines.h file or by users sketch
  rotation  = 0;
  cursor_y  = cursor_x  = 0;
  textfont  = 1;
  textsize  = 1;
  textcolor   = 0xFFFF; // White
  textbgcolor = 0x0000; // Black
  padX = 0;             // No padding
  isDigits   = false;   // No bounding box adjustment
  textwrapX  = true;    // Wrap text at end of line when using print stream
  textwrapY  = false;   // Wrap text at bottom of screen when using print stream
  textdatum = TL_DATUM; // Top Left text alignment is default
  fontsloaded = 0;



  locked = true;        // ESP32 transaction mutex lock flags
  inTransaction = false;

  _booted   = true;

  addr_row = 0xFFFF;
  addr_col = 0xFFFF;

  _xpivot = 0;
  _ypivot = 0;

#ifdef LOAD_GLCD
  fontsloaded  = 0x0002; // Bit 1 set
#endif

#ifdef LOAD_FONT2
  fontsloaded |= 0x0004; // Bit 2 set
#endif

#ifdef LOAD_FONT4
  fontsloaded |= 0x0010; // Bit 4 set
#endif

#ifdef LOAD_FONT6
  fontsloaded |= 0x0040; // Bit 6 set
#endif

#ifdef LOAD_FONT7
  fontsloaded |= 0x0080; // Bit 7 set
#endif

#ifdef LOAD_FONT8
  fontsloaded |= 0x0100; // Bit 8 set
#endif

#ifdef LOAD_FONT8N
  fontsloaded |= 0x0200; // Bit 9 set
#endif


}

void DisplayDriver::init()
{
  if (_booted)
  {
#if !defined (ESP32)
  #ifdef TFT_CS
    cspinmask = (uint32_t) digitalPinToBitMask(TFT_CS);
  #endif

  #ifdef TFT_DC
    dcpinmask = (uint32_t) digitalPinToBitMask(TFT_DC);
  #endif
  
  
  #ifdef TFT_SCLK
    sclkpinmask = (uint32_t) digitalPinToBitMask(TFT_SCLK);
  #endif
  
  #ifdef TFT_SPI_OVERLAP
    // Overlap mode SD0=MISO, SD1=MOSI, CLK=SCLK must use D3 as CS
    //    pins(int8_t sck, int8_t miso, int8_t mosi, int8_t ss);
    //spi.pins(        6,          7,           8,          0);
    spi.pins(6, 7, 8, 0);
  #endif

  spi.begin(); // This will set HMISO to input

#else
  #if !defined(ESP32_PARALLEL)
    #if defined (TFT_MOSI) && !defined (TFT_SPI_OVERLAP)
      spi.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, -1);
    #else
      spi.begin();
    #endif
  #endif
#endif

    inTransaction = false;
    locked = true;

  // SUPPORT_TRANSACTIONS is mandatory for ESP32 so the hal mutex is toggled
  // so the code here is for ESP8266 only
#if !defined (SUPPORT_TRANSACTIONS) && defined (ESP8266)
    spi.setBitOrder(MSBFIRST);
    spi.setDataMode(TFT_SPI_MODE);
    spi.setFrequency(SPI_FREQUENCY);
#endif

  #ifdef TFT_CS
    // Set to output once again in case D6 (MISO) is used for CS
    digitalWrite(TFT_CS, HIGH); // Chip select high (inactive)
    pinMode(TFT_CS, OUTPUT);
  #else
    spi.setHwCs(1); // Use hardware SS toggling
  #endif

  // Set to output once again in case D6 (MISO) is used for DC
#ifdef TFT_DC
    digitalWrite(TFT_DC, HIGH); // Data/Command high = data mode
    pinMode(TFT_DC, OUTPUT);
#endif
    _booted = false;
    spi_end();
  } // end of: if just _booted

  // Toggle RST low to reset
  spi_begin();

#ifdef TFT_RST
  if (TFT_RST >= 0) {
    digitalWrite(TFT_RST, HIGH);
    delay(5);
    digitalWrite(TFT_RST, LOW);
    delay(20);
    digitalWrite(TFT_RST, HIGH);
  }
  else writecommand(TFT_SWRST); // Software reset
#else
  writecommand(TFT_SWRST); // Software reset
#endif

  spi_end();

  delay(150); // Wait for reset to complete

  spi_begin();
  
  // This loads the driver specific initialisation code  <<<<<<<<<<<<<<<<<<<<< ADD NEW DRIVERS TO THE LIST HERE <<<<<<<<<<<<<<<<<<<<<<<
#if   defined (ILI9341_DRIVER)
    #include "hardware/ILI9341_Init.h"
#endif

#ifdef TFT_INVERSION_ON
  writecommand(TFT_INVON);
#endif

#ifdef TFT_INVERSION_OFF
  writecommand(TFT_INVOFF);
#endif

  spi_end();

  setRotation(rotation);

}

void DisplayDriver::setRotation(uint8_t m)
{

  spi_begin();

    // This loads the driver specific rotation code  <<<<<<<<<<<<<<<<<<<<< ADD NEW DRIVERS TO THE LIST HERE <<<<<<<<<<<<<<<<<<<<<<<
#if   defined (ILI9341_DRIVER)
    #include "hardware/ILI9341_Rotation.h"

#endif

  delayMicroseconds(10);

  spi_end();

  addr_row = 0xFFFF;
  addr_col = 0xFFFF;
}

void DisplayDriver::drawPixel(int32_t x, int32_t y, uint32_t color)
{
  // Range checking
  if ((x < 0) || (y < 0) ||(x >= _width) || (y >= _height)) return;

#ifdef CGRAM_OFFSET
  x+=colstart;
  y+=rowstart;
#endif

  spi_begin();

  // No need to send x if it has not changed (speeds things up)
  if (addr_col != x) {

    DC_C;

    SPI1U1 = (CMD_BITS << SPILMOSI) | (CMD_BITS << SPILMISO);
    SPI1W0 = TFT_CASET<<(CMD_BITS + 1 - 8);
    SPI1CMD |= SPIBUSY;
    while(SPI1CMD & SPIBUSY) {}

    DC_D;
    SPI1U1 = (31 << SPILMOSI) | (31 << SPILMISO);
    // Load the two coords as a 32 bit value and shift in one go
    uint32_t xswap = (x >> 8) | (uint16_t)(x << 8);
    SPI1W0 = xswap | (xswap << 16);
    SPI1CMD |= SPIBUSY;
    while(SPI1CMD & SPIBUSY) {}


    addr_col = x;
  }

  // No need to send y if it has not changed (speeds things up)
  if (addr_row != y) {

    DC_C;

    SPI1U1 = (CMD_BITS << SPILMOSI) | (CMD_BITS << SPILMISO);

    SPI1W0 = TFT_PASET<<(CMD_BITS + 1 - 8);
    SPI1CMD |= SPIBUSY;
    while(SPI1CMD & SPIBUSY) {}

    DC_D;

    SPI1U1 = (31 << SPILMOSI) | (31 << SPILMISO);
    // Load the two coords as a 32 bit value and shift in one go
    uint32_t yswap = (y >> 8) | (uint16_t)(y << 8);
    SPI1W0 = yswap | (yswap << 16);
    SPI1CMD |= SPIBUSY;
    while(SPI1CMD & SPIBUSY) {}

    addr_row = y;
  }

  DC_C;

  SPI1U1 = (CMD_BITS << SPILMOSI) | (CMD_BITS << SPILMISO);

  SPI1W0 = TFT_RAMWR<<(CMD_BITS + 1 - 8);
  SPI1CMD |= SPIBUSY;
  while(SPI1CMD & SPIBUSY) {}

  DC_D;


  SPI1U1 = (15 << SPILMOSI) | (15 << SPILMISO);

  SPI1W0 = (color >> 8) | (color << 8);
  SPI1CMD |= SPIBUSY;
  while(SPI1CMD & SPIBUSY) {}

  spi_end();
}


void DisplayDriver::fillScreen(uint32_t color)
{
  fillRect(0, 0, _width, _height, color);
}

void DisplayDriver::drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color)
{
  spi_begin();
  inTransaction = true;

  drawFastHLine(x, y, w, color);
  drawFastHLine(x, y + h - 1, w, color);
  // Avoid drawing corner pixels twice
  drawFastVLine(x, y+1, h-2, color);
  drawFastVLine(x + w - 1, y+1, h-2, color);

  inTransaction = false;
  spi_end();
}

void DisplayDriver::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color)
{
  // Clipping
  if ((x >= _width) || (y >= _height)) return;
  
  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }

  if ((x + w) > _width)  w = _width  - x;
  if ((y + h) > _height) h = _height - y;

  if ((w < 1) || (h < 1)) return;

  spi_begin();

  setWindow(x, y, x + w - 1, y + h - 1);

  writeBlock(color, w * h);
  
  spi_end();
}

void DisplayDriver::drawFastVLine(int32_t x, int32_t y, int32_t h, uint32_t color)
{
  // Clipping
  if ((x < 0) || (x >= _width) || (y >= _height)) return;

  if (y < 0) { h += y; y = 0; }

  if ((y + h) > _height) h = _height - y;

  if (h < 1) return;

  spi_begin();

  setWindow(x, y, x, y + h - 1);

  writeBlock(color, h);
  
  spi_end();
}

void DisplayDriver::drawFastHLine(int32_t x, int32_t y, int32_t w, uint32_t color)
{
  // Clipping
  if ((y < 0) || (x >= _width) || (y >= _height)) return;

  if (x < 0) { w += x; x = 0; }

  if ((x + w) > _width)  w = _width  - x;

  if (w < 1) return;

  spi_begin();

  setWindow(x, y, x + w - 1, y);

  writeBlock(color, w);
  
  spi_end();
}

#if defined (ESP8266) && !defined (RPI_WRITE_STROBE) && !defined (RPI_ILI9486_DRIVER)
void DisplayDriver::setWindow(int32_t xs, int32_t ys, int32_t xe, int32_t ye)
{
  //spi_begin(); // Must be called before setWimdow

#ifdef CGRAM_OFFSET
  xs+=colstartcolstart;
  xe+=colstart;
  ys+=rowstart;
  ye+=rowstart;
#endif

  // Column addr set
  DC_C;

  SPI1U1 = (CMD_BITS << SPILMOSI) | (CMD_BITS << SPILMISO);

  SPI1W0 = TFT_CASET;
  SPI1CMD |= SPIBUSY;

  addr_col = 0xFFFF;
  addr_row = 0xFFFF;
  
  while(SPI1CMD & SPIBUSY) {}

  DC_D;

  SPI1U1 = (31 << SPILMOSI) | (31 << SPILMISO);
  // Load the two coords as a 32 bit value and shift in one go
  SPI1W0 = (xs >> 8) | (uint16_t)(xs << 8) | ((uint8_t)(xe >> 8)<<16 | (xe << 24));
  SPI1CMD |= SPIBUSY;
  while(SPI1CMD & SPIBUSY) {}

  // Row addr set
  DC_C;

  SPI1U1 = (CMD_BITS << SPILMOSI) | (CMD_BITS << SPILMISO);

  SPI1W0 = TFT_PASET;
  SPI1CMD |= SPIBUSY;
  while(SPI1CMD & SPIBUSY) {}

  DC_D;

  SPI1U1 = (31 << SPILMOSI) | (31 << SPILMISO);
  // Load the two coords as a 32 bit value and shift in one go
  SPI1W0 = (ys >> 8) | (uint16_t)(ys << 8) | ((uint8_t)(ye >> 8)<<16 | (ye << 24));
  SPI1CMD |= SPIBUSY;
  while(SPI1CMD & SPIBUSY) {}

  // write to RAM
  DC_C;

  SPI1U1 = (CMD_BITS << SPILMOSI) | (CMD_BITS << SPILMISO);
  SPI1W0 = TFT_RAMWR;
  SPI1CMD |= SPIBUSY;
  while(SPI1CMD & SPIBUSY) {}

  DC_D;

  SPI1U1 = (15 << SPILMOSI) | (15 << SPILMISO);
  //spi_end();
}
#endif


void writeBlock(uint16_t color, uint32_t repeat)
{
  uint16_t color16 = (color >> 8) | (color << 8);
  uint32_t color32 = color16 | color16 << 16;

  SPI1W0 = color32;
  SPI1W1 = color32;
  SPI1W2 = color32;
  SPI1W3 = color32;
  if (repeat > 8)
  {
    SPI1W4 = color32;
    SPI1W5 = color32;
    SPI1W6 = color32;
    SPI1W7 = color32;
  }
  if (repeat > 16)
  {
    SPI1W8 = color32;
    SPI1W9 = color32;
    SPI1W10 = color32;
    SPI1W11 = color32;
  }
  if (repeat > 24)
  {
    SPI1W12 = color32;
    SPI1W13 = color32;
    SPI1W14 = color32;
    SPI1W15 = color32;
  }
  if (repeat > 31)
  {
    SPI1U1 = (511 << SPILMOSI);
    while(repeat>31)
    {
#if defined SPI_FREQUENCY && (SPI_FREQUENCY == 80000000)
      if(SPI1CMD & SPIBUSY) // added to sync with flag change
#endif
      while(SPI1CMD & SPIBUSY) {}
      SPI1CMD |= SPIBUSY;
      repeat -= 32;
    }
    while(SPI1CMD & SPIBUSY) {}
  }

  if (repeat)
  {
    repeat = (repeat << 4) - 1;
    SPI1U1 = (repeat << SPILMOSI);
    SPI1CMD |= SPIBUSY;
    while(SPI1CMD & SPIBUSY) {}
  }

}

void DisplayDriver::setTextColor(uint16_t c)
{
  // For 'transparent' background, we'll set the bg
  // to the same as fg instead of using a flag
  textcolor = textbgcolor = c;
}


/***************************************************************************************
** Function name:           setTextColor
** Description:             Set the font foreground and background colour
***************************************************************************************/
void DisplayDriver::setTextColor(uint16_t c, uint16_t b)
{
  textcolor   = c;
  textbgcolor = b;
}

void DisplayDriver::setTextSize(uint8_t s)
{
  if (s>7) s = 7; // Limit the maximum size multiplier so byte variables can be used for rendering
  textsize = (s > 0) ? s : 1; // Don't allow font size 0
}

int16_t DisplayDriver::drawString(const String& string, int32_t poX, int32_t poY)
{
  int16_t len = string.length() + 2;
  char buffer[len];
  string.toCharArray(buffer, len);
  return drawString(buffer, poX, poY, textfont);
}
// With font number
int16_t DisplayDriver::drawString(const String& string, int32_t poX, int32_t poY, uint8_t font)
{
  int16_t len = string.length() + 2;
  char buffer[len];
  string.toCharArray(buffer, len);
  return drawString(buffer, poX, poY, font);
}

// Without font number, uses font set by setTextFont()
int16_t DisplayDriver::drawString(const char *string, int32_t poX, int32_t poY)
{
  return drawString(string, poX, poY, textfont);
}


int16_t DisplayDriver::drawString(const char *string, int32_t poX, int32_t poY, uint8_t font)
{
  int16_t sumX = 0;
  uint8_t padding = 1, baseline = 0;
  uint16_t cwidth = textWidth(string, font); // Find the pixel width of the string in the font
  uint16_t cheight = 8 * textsize;

  if (font!=1) {
    baseline = pgm_read_byte( &fontdata[font].baseline ) * textsize;
    cheight = fontHeight(font);
  }

  if (textdatum || padX)
  {

    switch(textdatum) {
      case TC_DATUM:
        poX -= cwidth/2;
        padding += 1;
        break;
      case TR_DATUM:
        poX -= cwidth;
        padding += 2;
        break;
      case ML_DATUM:
        poY -= cheight/2;
        //padding += 0;
        break;
      case MC_DATUM:
        poX -= cwidth/2;
        poY -= cheight/2;
        padding += 1;
        break;
      case MR_DATUM:
        poX -= cwidth;
        poY -= cheight/2;
        padding += 2;
        break;
      case BL_DATUM:
        poY -= cheight;
        //padding += 0;
        break;
      case BC_DATUM:
        poX -= cwidth/2;
        poY -= cheight;
        padding += 1;
        break;
      case BR_DATUM:
        poX -= cwidth;
        poY -= cheight;
        padding += 2;
        break;
      case L_BASELINE:
        poY -= baseline;
        //padding += 0;
        break;
      case C_BASELINE:
        poX -= cwidth/2;
        poY -= baseline;
        padding += 1;
        break;
      case R_BASELINE:
        poX -= cwidth;
        poY -= baseline;
        padding += 2;
        break;
    }
    // Check coordinates are OK, adjust if not
    if (poX < 0) poX = 0;
    if (poX+cwidth > width())   poX = width() - cwidth;
    if (poY < 0) poY = 0;
    if (poY+cheight-baseline> height()) poY = height() - cheight;
  }


  int8_t xo = 0;

  uint16_t len = strlen(string);
  uint16_t n = 0;

  {
    while (n < len)
    {
      uint16_t uniCode = decodeUTF8((uint8_t*)string, &n, len - n);
      sumX += drawChar(uniCode, poX+sumX, poY, font);
    }
  }

//vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv DEBUG vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
// Switch on debugging for the padding areas
//#define PADDING_DEBUG

#ifndef PADDING_DEBUG
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ DEBUG ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  if((padX>cwidth) && (textcolor!=textbgcolor))
  {
    int16_t padXc = poX+cwidth+xo;
    switch(padding) {
      case 1:
        fillRect(padXc,poY,padX-cwidth,cheight, textbgcolor);
        break;
      case 2:
        fillRect(padXc,poY,(padX-cwidth)>>1,cheight, textbgcolor);
        padXc = (padX-cwidth)>>1;
        if (padXc>poX) padXc = poX;
        fillRect(poX - padXc,poY,(padX-cwidth)>>1,cheight, textbgcolor);
        break;
      case 3:
        if (padXc>padX) padXc = padX;
        fillRect(poX + cwidth - padXc,poY,padXc-cwidth,cheight, textbgcolor);
        break;
    }
  }


#else

//vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv DEBUG vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
// This is debug code to show text (green box) and blanked (white box) areas
// It shows that the padding areas are being correctly sized and positioned

  if((padX>sumX) && (textcolor!=textbgcolor))
  {
    int16_t padXc = poX+sumX; // Maximum left side padding
    drawRect(poX,poY,sumX,cheight, TFT_GREEN);
    switch(padding) {
      case 1:
        drawRect(padXc,poY,padX-sumX,cheight, TFT_WHITE);
        break;
      case 2:
        drawRect(padXc,poY,(padX-sumX)>>1, cheight, TFT_WHITE);
        padXc = (padX-sumX)>>1;
        if (padXc>poX) padXc = poX;
        drawRect(poX - padXc,poY,(padX-sumX)>>1,cheight, TFT_WHITE);
        break;
      case 3:
        if (padXc>padX) padXc = padX;
        drawRect(poX + sumX - padXc,poY,padXc-sumX,cheight, TFT_WHITE);
        break;
    }
  }
#endif
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ DEBUG ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

return sumX;
}



int16_t DisplayDriver::drawCentreString(const String& string, int32_t dX, int32_t poY, uint8_t font)
{
  int16_t len = string.length() + 2;
  char buffer[len];
  string.toCharArray(buffer, len);
  return drawCentreString(buffer, dX, poY, font);
}

int16_t DisplayDriver::drawCentreString(const char *string, int32_t dX, int32_t poY, uint8_t font)
{
  uint8_t tempdatum = textdatum;
  int32_t sumX = 0;
  textdatum = TC_DATUM;
  sumX = drawString(string, dX, poY, font);
  textdatum = tempdatum;
  return sumX;
}


int16_t DisplayDriver::drawRightString(const String& string, int32_t dX, int32_t poY, uint8_t font)
{
  int16_t len = string.length() + 2;
  char buffer[len];
  string.toCharArray(buffer, len);
  return drawRightString(buffer, dX, poY, font);
}

int16_t DisplayDriver::drawRightString(const char *string, int32_t dX, int32_t poY, uint8_t font)
{
  uint8_t tempdatum = textdatum;
  int16_t sumX = 0;
  textdatum = TR_DATUM;
  sumX = drawString(string, dX, poY, font);
  textdatum = tempdatum;
  return sumX;
}


/***************************************************************************************
** Function name:           drawNumber
** Description:             draw a long integer
***************************************************************************************/
int16_t DisplayDriver::drawNumber(long long_num, int32_t poX, int32_t poY)
{
  isDigits = true; // Eliminate jiggle in monospaced fonts
  char str[12];
  ltoa(long_num, str, 10);
  return drawString(str, poX, poY, textfont);
}

int16_t DisplayDriver::drawNumber(long long_num, int32_t poX, int32_t poY, uint8_t font)
{
  isDigits = true; // Eliminate jiggle in monospaced fonts
  char str[12];
  ltoa(long_num, str, 10);
  return drawString(str, poX, poY, font);
}


// Assemble and print a string, this permits alignment relative to a datum
// looks complicated but much more compact and actually faster than using print class
int16_t DisplayDriver::drawFloat(float floatNumber, uint8_t dp, int32_t poX, int32_t poY)
{
  return drawFloat(floatNumber, dp, poX, poY, textfont);
}

int16_t DisplayDriver::drawFloat(float floatNumber, uint8_t dp, int32_t poX, int32_t poY, uint8_t font)
{
  isDigits = true;
  char str[14];               // Array to contain decimal string
  uint8_t ptr = 0;            // Initialise pointer for array
  int8_t  digits = 1;         // Count the digits to avoid array overflow
  float rounding = 0.5;       // Round up down delta

  if (dp > 7) dp = 7; // Limit the size of decimal portion

  // Adjust the rounding value
  for (uint8_t i = 0; i < dp; ++i) rounding /= 10.0;

  if (floatNumber < -rounding)    // add sign, avoid adding - sign to 0.0!
  {
    str[ptr++] = '-'; // Negative number
    str[ptr] = 0; // Put a null in the array as a precaution
    digits = 0;   // Set digits to 0 to compensate so pointer value can be used later
    floatNumber = -floatNumber; // Make positive
  }

  floatNumber += rounding; // Round up or down

  // For error put ... in string and return (all TFT_eSPI library fonts contain . character)
  if (floatNumber >= 2147483647) {
    strcpy(str, "...");
    return drawString(str, poX, poY, font);
  }
  // No chance of overflow from here on

  // Get integer part
  uint32_t temp = (uint32_t)floatNumber;

  // Put integer part into array
  ltoa(temp, str + ptr, 10);

  // Find out where the null is to get the digit count loaded
  while ((uint8_t)str[ptr] != 0) ptr++; // Move the pointer along
  digits += ptr;                  // Count the digits

  str[ptr++] = '.'; // Add decimal point
  str[ptr] = '0';   // Add a dummy zero
  str[ptr + 1] = 0; // Add a null but don't increment pointer so it can be overwritten

  // Get the decimal portion
  floatNumber = floatNumber - temp;

  // Get decimal digits one by one and put in array
  // Limit digit count so we don't get a false sense of resolution
  uint8_t i = 0;
  while ((i < dp) && (digits < 9)) // while (i < dp) for no limit but array size must be increased
  {
    i++;
    floatNumber *= 10;       // for the next decimal
    temp = floatNumber;      // get the decimal
    ltoa(temp, str + ptr, 10);
    ptr++; digits++;         // Increment pointer and digits count
    floatNumber -= temp;     // Remove that digit
  }

  // Finally we can plot the string and return pixel length
  return drawString(str, poX, poY, font);
}


int16_t DisplayDriver::textWidth(const String& string)
{
  int16_t len = string.length() + 2;
  char buffer[len];
  string.toCharArray(buffer, len);
  return textWidth(buffer, textfont);
}

int16_t DisplayDriver::textWidth(const String& string, uint8_t font)
{
  int16_t len = string.length() + 2;
  char buffer[len];
  string.toCharArray(buffer, len);
  return textWidth(buffer, font);
}

int16_t DisplayDriver::textWidth(const char *string)
{
  return textWidth(string, textfont);
}

int16_t DisplayDriver::textWidth(const char *string, uint8_t font)
{
  int32_t str_width = 0;
  uint16_t uniCode  = 0;

  if (font>1 && font<9)
  {
    char *widthtable = (char *)pgm_read_dword( &(fontdata[font].widthtbl ) ) - 32; //subtract the 32 outside the loop

    while (*string)
    {
      uniCode = *(string++);
      if (uniCode > 31 && uniCode < 128)
      str_width += pgm_read_byte( widthtable + uniCode); // Normally we need to subtract 32 from uniCode
      else str_width += pgm_read_byte( widthtable + 32); // Set illegal character = space width
    }
  }
  else
  {

#ifdef LOAD_GLCD
      while (*string++) str_width += 6;
#endif
  }
  isDigits = false;
  return str_width * textsize;
}

int16_t DisplayDriver::fontHeight(int16_t font)
{

  return pgm_read_byte( &fontdata[font].height ) * textsize;
}

int16_t DisplayDriver::fontHeight(void)
{
  return fontHeight(textfont);
}

void DisplayDriver::drawChar(int32_t x, int32_t y, uint16_t c, uint32_t color, uint32_t bg, uint8_t size)
{
  if ((x >= _width)            || // Clip right
      (y >= _height)           || // Clip bottom
      ((x + 6 * size - 1) < 0) || // Clip left
      ((y + 8 * size - 1) < 0))   // Clip top
    return;

  if (c < 32) return;
#ifdef LOAD_GLCD
//>>>>>>>>>>>>>>>>>>

  boolean fillbg = (bg != color);

  if ((size==1) && fillbg)
  {
    uint8_t column[6];
    uint8_t mask = 0x1;
    spi_begin();

    setWindow(x, y, x+5, y+8);

    for (int8_t i = 0; i < 5; i++ ) column[i] = pgm_read_byte(font + (c * 5) + i);
    column[5] = 0;

#if defined (ESP8266) && !defined (ILI9488_DRIVER)
    color = (color >> 8) | (color << 8);
    bg = (bg >> 8) | (bg << 8);

    for (int8_t j = 0; j < 8; j++) {
      for (int8_t k = 0; k < 5; k++ ) {
        if (column[k] & mask) {
          SPI1W0 = color;
        }
        else {
          SPI1W0 = bg;
        }
        SPI1CMD |= SPIBUSY;
        while(SPI1CMD & SPIBUSY) {}
      }

      mask <<= 1;

      SPI1W0 = bg;
      SPI1CMD |= SPIBUSY;
      while(SPI1CMD & SPIBUSY) {}
    }

#endif

    spi_end();
  }
  else
  {
    spi_begin();
    inTransaction = true;
    for (int8_t i = 0; i < 6; i++ ) {
      uint8_t line;
      if (i == 5)
        line = 0x0;
      else
        line = pgm_read_byte(font + (c * 5) + i);

      if (size == 1) // default size
      {
        for (int8_t j = 0; j < 8; j++) {
          if (line & 0x1) drawPixel(x + i, y + j, color);
          line >>= 1;
        }
      }
      else {  // big size
        for (int8_t j = 0; j < 8; j++) {
          if (line & 0x1) fillRect(x + (i * size), y + (j * size), size, size, color);
          else if (fillbg) fillRect(x + i * size, y + j * size, size, size, bg);
          line >>= 1;
        }
      }
    }
    inTransaction = false;
    spi_end();
  }




#endif

}
int16_t DisplayDriver::drawChar(uint16_t uniCode, int32_t x, int32_t y)
{
  return drawChar(uniCode, x, y, textfont);
}

  // Any UTF-8 decoding must be done before calling drawChar()
int16_t DisplayDriver::drawChar(uint16_t uniCode, int32_t x, int32_t y, uint8_t font)
{
  if (!uniCode) return 0;

  if (font==1)
  {
#ifdef LOAD_GLCD
    drawChar(x, y, uniCode, textcolor, textbgcolor, textsize);
    return 6 * textsize;
#else
    return 0;
#endif

  }

  if ((font>1) && (font<9) && ((uniCode < 32) || (uniCode > 127))) return 0;

  int32_t width  = 0;
  int32_t height = 0;
  uint32_t flash_address = 0;
  uniCode -= 32;

#ifdef LOAD_FONT2
  if (font == 2)
  {
    flash_address = pgm_read_dword(&chrtbl_f16[uniCode]);
    width = pgm_read_byte(widtbl_f16 + uniCode);
    height = chr_hgt_f16;
  }
  #ifdef LOAD_RLE
  else
  #endif
#endif

#ifdef LOAD_RLE
  {
    if ((font>2) && (font<9))
    {
      flash_address = pgm_read_dword( pgm_read_dword( &(fontdata[font].chartbl ) ) + uniCode*sizeof(void *) );
      width = pgm_read_byte( (uint8_t *)pgm_read_dword( &(fontdata[font].widthtbl ) ) + uniCode );
      height= pgm_read_byte( &fontdata[font].height );
    }
  }
#endif

  int32_t w = width;
  int32_t pX      = 0;
  int32_t pY      = y;
  uint8_t line = 0;

#ifdef LOAD_FONT2 // chop out code if we do not need it
  if (font == 2) {
    w = w + 6; // Should be + 7 but we need to compensate for width increment
    w = w / 8;
    if (x + width * textsize >= (int16_t)_width) return width * textsize ;

    if (textcolor == textbgcolor || textsize != 1) {
      spi_begin();
      inTransaction = true;

      for (int32_t i = 0; i < height; i++)
      {
        if (textcolor != textbgcolor) fillRect(x, pY, width * textsize, textsize, textbgcolor);

        for (int32_t k = 0; k < w; k++)
        {
          line = pgm_read_byte((uint8_t *)flash_address + w * i + k);
          if (line) {
            if (textsize == 1) {
              pX = x + k * 8;
              if (line & 0x80) drawPixel(pX, pY, textcolor);
              if (line & 0x40) drawPixel(pX + 1, pY, textcolor);
              if (line & 0x20) drawPixel(pX + 2, pY, textcolor);
              if (line & 0x10) drawPixel(pX + 3, pY, textcolor);
              if (line & 0x08) drawPixel(pX + 4, pY, textcolor);
              if (line & 0x04) drawPixel(pX + 5, pY, textcolor);
              if (line & 0x02) drawPixel(pX + 6, pY, textcolor);
              if (line & 0x01) drawPixel(pX + 7, pY, textcolor);
            }
            else {
              pX = x + k * 8 * textsize;
              if (line & 0x80) fillRect(pX, pY, textsize, textsize, textcolor);
              if (line & 0x40) fillRect(pX + textsize, pY, textsize, textsize, textcolor);
              if (line & 0x20) fillRect(pX + 2 * textsize, pY, textsize, textsize, textcolor);
              if (line & 0x10) fillRect(pX + 3 * textsize, pY, textsize, textsize, textcolor);
              if (line & 0x08) fillRect(pX + 4 * textsize, pY, textsize, textsize, textcolor);
              if (line & 0x04) fillRect(pX + 5 * textsize, pY, textsize, textsize, textcolor);
              if (line & 0x02) fillRect(pX + 6 * textsize, pY, textsize, textsize, textcolor);
              if (line & 0x01) fillRect(pX + 7 * textsize, pY, textsize, textsize, textcolor);
            }
          }
        }
        pY += textsize;
      }

      inTransaction = false;
      spi_end();
    }
    else
      // Faster drawing of characters and background using block write
    {
      spi_begin();

      setWindow(x, y, (x + w * 8) - 1, y + height - 1);

      uint8_t mask;
      for (int32_t i = 0; i < height; i++)
      {
        for (int32_t k = 0; k < w; k++)
        {
          line = pgm_read_byte((uint8_t *)flash_address + w * i + k);
          pX = x + k * 8;
          mask = 0x80;
          while (mask) {
            if (line & mask) {tft_Write_16(textcolor);}
            else {tft_Write_16(textbgcolor);}
            mask = mask >> 1;
          }
        }
        pY += textsize;
      }

      spi_end();
    }
  }

  #ifdef LOAD_RLE
  else
  #endif
#endif  //FONT2

#ifdef LOAD_RLE  //674 bytes of code
  // Font is not 2 and hence is RLE encoded
  {
    spi_begin();
    inTransaction = true;

    w *= height; // Now w is total number of pixels in the character
    if ((textsize != 1) || (textcolor == textbgcolor)) {
      if (textcolor != textbgcolor) fillRect(x, pY, width * textsize, textsize * height, textbgcolor);
      int32_t px = 0, py = pY; // To hold character block start and end column and row values
      int32_t pc = 0; // Pixel count
      uint8_t np = textsize * textsize; // Number of pixels in a drawn pixel

      uint8_t tnp = 0; // Temporary copy of np for while loop
      uint8_t ts = textsize - 1; // Temporary copy of textsize
      // 16 bit pixel count so maximum font size is equivalent to 180x180 pixels in area
      // w is total number of pixels to plot to fill character block
      while (pc < w)
      {
        line = pgm_read_byte((uint8_t *)flash_address);
        flash_address++;
        if (line & 0x80) {
          line &= 0x7F;
          line++;
          if (ts) {
            px = x + textsize * (pc % width); // Keep these px and py calculations outside the loop as they are slow
            py = y + textsize * (pc / width);
          }
          else {
            px = x + pc % width; // Keep these px and py calculations outside the loop as they are slow
            py = y + pc / width;
          }
          while (line--) { // In this case the while(line--) is faster
            pc++; // This is faster than putting pc+=line before while()?
            setWindow(px, py, px + ts, py + ts);

            if (ts) {
              tnp = np;
              while (tnp--) {tft_Write_16(textcolor);}
            }
            else {tft_Write_16(textcolor);}
            px += textsize;

            if (px >= (x + width * textsize))
            {
              px = x;
              py += textsize;
            }
          }
        }
        else {
          line++;
          pc += line;
        }
      }
    }
    else // Text colour != background && textsize = 1
         // so use faster drawing of characters and background using block write
    {
      setWindow(x, y, x + width - 1, y + height - 1);

      // Maximum font size is equivalent to 180x180 pixels in area
      while (w > 0)
      {
        line = pgm_read_byte((uint8_t *)flash_address++); // 8 bytes smaller when incrementing here
        if (line & 0x80) {
          line &= 0x7F;
          line++; w -= line;
            writeBlock(textcolor,line);

        }
        else {
          line++; w -= line;
            writeBlock(textbgcolor,line);

        }
      }
    }
    inTransaction = false;
    spi_end();
  }
  // End of RLE font rendering
#endif
  return width * textsize;    // x +
}

#define DECODE_UTF8 // Test only, comment out to stop decoding
uint16_t DisplayDriver::decodeUTF8(uint8_t c)
{
#ifdef DECODE_UTF8
  // 7 bit Unicode Code Point
  if ((c & 0x80) == 0x00) {
    decoderState = 0;
    return (uint16_t)c;
  }

  if (decoderState == 0)
  {
    // 11 bit Unicode Code Point
    if ((c & 0xE0) == 0xC0)
    {
      decoderBuffer = ((c & 0x1F)<<6);
      decoderState = 1;
      return 0;
    }

    // 16 bit Unicode Code Point
    if ((c & 0xF0) == 0xE0)
    {
      decoderBuffer = ((c & 0x0F)<<12);
      decoderState = 2;
      return 0;
    }
    // 21 bit Unicode  Code Point not supported so fall-back to extended ASCII
    if ((c & 0xF8) == 0xF0) return (uint16_t)c;
  }
  else
  {
    if (decoderState == 2)
    {
      decoderBuffer |= ((c & 0x3F)<<6);
      decoderState--;
      return 0;
    }
    else
    {
      decoderBuffer |= (c & 0x3F);
      decoderState = 0;
      return decoderBuffer;
    }
  }

  decoderState = 0;
#endif

  return (uint16_t)c; // fall-back to extended ASCII
}


uint16_t DisplayDriver::decodeUTF8(uint8_t *buf, uint16_t *index, uint16_t remaining)
{
  uint16_t c = buf[(*index)++];
  //Serial.print("Byte from string = 0x"); Serial.println(c, HEX);

#ifdef DECODE_UTF8
  // 7 bit Unicode
  if ((c & 0x80) == 0x00) return c;

  // 11 bit Unicode
  if (((c & 0xE0) == 0xC0) && (remaining > 1))
    return ((c & 0x1F)<<6) | (buf[(*index)++]&0x3F);

  // 16 bit Unicode
  if (((c & 0xF0) == 0xE0) && (remaining > 2))
  {
    c = ((c & 0x0F)<<12) | ((buf[(*index)++]&0x3F)<<6);
    return  c | ((buf[(*index)++]&0x3F));
  }

  // 21 bit Unicode not supported so fall-back to extended ASCII
  // if ((c & 0xF8) == 0xF0) return c;
#endif

  return c; // fall-back to extended ASCII
}

void DisplayDriver::drawMatrix( uint8_t** matrix, int rows, int cols, int size,int x, int y)
{
	

	for (int i = 0; i < rows; ++i)
	{
		for (int j = 0; j < cols; ++j)
		{
			if (matrix[i][j] > 0)
			{
				fillRect(x+j*size,y+i*size,size,size,TFT_BLUE);
				drawRect(x+j*size,y+i*size,size+1,size+1,TFT_WHITE);
				
			}
			else
			{
				//drawRect(x+j*size,y+i*size,size+1,size+1,TFT_WHITE);
			}
		}
	}
}

