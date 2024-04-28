//
// CYD (Cheap Yellow Display) GIF example
//

// #include <bb_captouch.h>
#include <bb_spi_lcd.h>
#include <AnimatedGIF.h>

#include <SPI.h>
#include <SD.h>
#include "FS.h"


#include <vector>
#include <algorithm> // For std::shuffle
#include <random>

#include "audioTask.h"
#include "gifUtils.h"

char *subDir = "/wade_messages";

char *spash_screen_gif = "/spash_screen.gif";
char *spash_screen_wav = "/spash_screen.wav";


// define SD pins
#define SD_CS_PIN 5
#define MOSI_PIN 23
#define SCK_PIN 18
#define MISO_PIN 19

#define UP_PIN 22
#define DOWN_PIN 27

bool audioSucceeded = false;

bool buttonPressed = false;

std::vector<String> listGIFFiles(const char* directory) {
  File root = SD.open(directory);
  std::vector<String> files;
  if (!root) {
    Serial.println("Failed to open directory");
    return files;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return files;
  }
  File file = root.openNextFile();
  while (file) {
    if (String(file.name()).endsWith(".gif")) {
      files.push_back(String(file.name()));
    }
    file = root.openNextFile();
  }
  return files;
}

void setup() {
  Serial.begin(115200);
  // randomSeed(analogRead(0));

  pinMode(DOWN_PIN, INPUT_PULLUP);
  pinMode(UP_PIN, INPUT_PULLUP);

  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SD_CS_PIN);
  if (!SD.begin(SD_CS_PIN, SPI, 80000000)) {
    Serial.println("SD card init failed!");
    while (1); // SD initialisation failed so wait here
  }
  Serial.println("SD Card init succeeded!");

  auto gifFiles = listGIFFiles(subDir);
  if (gifFiles.empty()) {
    Serial.println("No .gif files found");
    return;
  }
  
  Serial.println("found " + String(gifFiles.size()) + " files");

  // Shuffle the list to randomize which file is played
  int randomIndex = random(gifFiles.size());

  // Assuming the first file is now a randomly selected one
  String selectedGIF = gifFiles.at(randomIndex);
  String selectedWAV = selectedGIF.substring(0, selectedGIF.lastIndexOf('.')) + ".wav";
  Serial.println("randomly selected message: " + selectedGIF);

  initGIF();
  // lcd.setTextColor(TFT_GREEN, TFT_BLACK);
  // lcd.setFont(FONT_12x16);
  // lcd.setCursor(0, 0);
  // lcd.println("Test");

  // Setup audio
  audioInit();

  if(!openGIF(spash_screen_gif))
  {
    Serial.println("GIF file open failed");
  }
  Serial.println("GIF file open succeeded");

  if(!audioConnecttoFS(SD, spash_screen_wav))
  {
    Serial.println("Audio file open failed");
  }
  Serial.println("Audio file open succeeded");

  while(loopGIF())
  {
    yield();
  }

  while(digitalRead(UP_PIN) == HIGH)
  {
    yield();
  }

  resetGIF();
  
  if(!openGIF((String(subDir) + "/" + selectedGIF).c_str()))
  {
    Serial.println("GIF file open failed");
  }
  Serial.println("GIF file open succeeded");

  if(!audioConnecttoFS(SD, (String(subDir) + "/" + selectedWAV).c_str()))
  {
    Serial.println("Audio file open failed");
  }
  Serial.println("Audio file open succeeded");
}

// TODO: look into new example in AnimatedGIF

void loop() {  
  if (loopGIF())
  {
    // do stuff while gif is playing
  }
  else
  {
    Serial.println("Done playing, entering deep sleep");
    esp_deep_sleep_start();
  }
  yield();
}