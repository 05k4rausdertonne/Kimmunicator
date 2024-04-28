#include "Arduino.h"
#include <bb_spi_lcd.h>
#include <AnimatedGIF.h>
#include <SD.h>
#include "FS.h"

#define LCD DISPLAY_CYD

AnimatedGIF gif;
BB_SPI_LCD lcd;
int iOffX, iOffY;

// file for AnimatedGIF lib
File f;

uint8_t *pFrameBuffer;

// Draw callback from GIF decoder
//
// called once for each line of the current frame
// MCUs with very little RAM would have to test for disposal methods, transparent pixels
// and translate the 8-bit pixels through the palette to generate the final output.
// The code for MCUs with enough RAM is much simpler because the AnimatedGIF library can
// generate "cooked" pixels that are ready to send to the display

void GIFDraw(GIFDRAW *pDraw)
{
  if (pDraw->y == 0) { // set the memory window when the first line is rendered
    lcd.setAddrWindow(iOffX + pDraw->iX, iOffY + pDraw->iY, pDraw->iWidth, pDraw->iHeight);
  }
  // For all other lines, just push the pixels to the display
  lcd.pushPixels((uint16_t *)pDraw->pPixels, pDraw->iWidth, DRAW_TO_LCD | DRAW_WITH_DMA);
} /* GIFDraw() */

static void * GIFOpenFile(const char *fname, int32_t *pSize)
{
  //log_d("GIFOpenFile( %s )\n", fname );
  f = SD.open(fname);
  if (f) {
    *pSize = f.size();
    return (void *)&f;
  }
  return NULL;
} /* GIFOpenFile() */

static void GIFCloseFile(void *pHandle)
{
  File *f = static_cast<File *>(pHandle);
  if (f != NULL)
     f->close();
}

int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen)
{
    int32_t iBytesRead;
    iBytesRead = iLen;
    File *f = static_cast<File *>(pFile->fHandle);
    // Note: If you read a file all the way to the last byte, seek() stops working
    if ((pFile->iSize - pFile->iPos) < iLen)
       iBytesRead = pFile->iSize - pFile->iPos - 1; // <-- ugly work-around
    if (iBytesRead <= 0)
       return 0;
    iBytesRead = (int32_t)f->read(pBuf, iBytesRead);
    pFile->iPos = f->position();
    return iBytesRead;
} /* GIFReadFile() */

int32_t GIFSeekFile(GIFFILE *pFile, int32_t iPosition)
{ 
  int i = micros();
  File *f = static_cast<File *>(pFile->fHandle);
  f->seek(iPosition);
  pFile->iPos = (int32_t)f->position();
  i = micros() - i;
//  Serial.printf("Seek time = %d us\n", i);
  return pFile->iPos;
} /* GIFSeekFile() */

bool openGIF(const char *fileName)
{
    int w, h;

    if (gif.open(fileName, GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw)) {
        w = gif.getCanvasWidth();
        h = gif.getCanvasHeight();
        Serial.printf("Canvas size = %d x %d\n", w, h);
        pFrameBuffer = (uint8_t *)heap_caps_malloc(w*(h+2), MALLOC_CAP_8BIT);
        gif.setDrawType(GIF_DRAW_COOKED); // we want the library to generate ready-made pixels
        gif.setFrameBuf(pFrameBuffer);
        iOffX = (lcd.width() - w)/2;
        iOffY = (lcd.height() - h)/2;
        return true;
    } 
    else
    {
        return false;
    }
}

bool initGIF()
{
    gif.begin(BIG_ENDIAN_PIXELS);
    lcd.begin(LCD);
    lcd.fillScreen(TFT_BLACK);
    return true;
}

bool loopGIF()
{
    return gif.playFrame(true, NULL);
}

bool resetGIF()
{
    gif.reset();
    // lcd.fillScreen(TFT_BLACK);
    heap_caps_free(pFrameBuffer);
    gif.close();
    return true;
}
