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

String sub_dir = "/wade_messages";

String spash_screen_path = "/spash_screen";

// define SD pins
#define SD_CS_PIN 5
#define MOSI_PIN 23
#define SCK_PIN 18
#define MISO_PIN 19

#define UP_PIN 22
#define DOWN_PIN 27

#define VIDEO_FORMAT ".gif"
#define AUDIO_FORMAT ".wav"

// TODO: Add config stuff

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
    if (String(file.name()).endsWith(VIDEO_FORMAT)) {
      files.push_back(String(file.name()));
    }
    file = root.openNextFile();
  }
  return files;
}

String selectRandomVideo(String path)
{
  auto gifFiles = listGIFFiles(path.c_str());
  if (gifFiles.empty()) {
    Serial.println("No "+(String)VIDEO_FORMAT+" files found");
    return "";
  }
  
  Serial.println("found " + String(gifFiles.size()) + " files");

  // Shuffle the list to randomize which file is played
  int randomIndex = random(gifFiles.size());

  String selectedGIF = gifFiles.at(randomIndex);
  String selected_message = gifFiles.at(randomIndex).substring(0, selectedGIF.lastIndexOf('.'));
  Serial.println("randomly selected message: " + selected_message);
  return selected_message;
} 

void play_video(String path)
{
  Serial.println("Playing Video " + path);
  if(!openGIF((path + VIDEO_FORMAT).c_str()))
  {
    Serial.println("Video file open failed");
  }
  Serial.println("Video file open succeeded");

  if(!audioConnecttoFS(SD, (path + AUDIO_FORMAT).c_str()))
  {
    Serial.println("Audio file open failed");
  }
  Serial.println("Audio file open succeeded");

  while(loopGIF())
  {
    if(digitalRead(DOWN_PIN) == LOW)
    {
      esp_deep_sleep_start();
    }
    yield();
  }
}
void reset_playback()
{
  // audioStopSong();
  resetGIF();
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

  String selected_path_one = sub_dir + "/" + selectRandomVideo(sub_dir);

  initGIF();

  // Setup audio
  audioInit();

  play_video(spash_screen_path);

  while(digitalRead(UP_PIN) == HIGH)
  {
    yield();
  }

  reset_playback();
  play_video(selected_path_one);
  reset_playback();
  Serial.println("Done playing, going to sleep.");
  esp_deep_sleep_start();
}

// TODO: look into new example in AnimatedGIF
// TODO: write issue about not being able to run more than two videos in sequence without crash

// do nothing in loop()
void loop() {  
  yield();
  // TODO: Add random timer for reboot 
}