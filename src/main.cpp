#include <WiFi.h>
#include <BluetoothSerial.h>
#include <esp_sleep.h>
#include <esp_wifi.h>

// #include <bb_captouch.h>
#include <bb_spi_lcd.h>
#include <AnimatedGIF.h>

#include <SPI.h>
#include <SD.h>
#include "FS.h"

#include <ArduinoJson.h>

#include <vector>
#include <algorithm> // For std::shuffle
#include <random>

#include "audioTask.h"
#include "gifUtils.h"

String messages_dir = "/wade_messages";

String spash_screen_path = "/spash_screen";

// define SD pins
#define SD_CS_PIN 5
#define MOSI_PIN 23
#define SCK_PIN 18
#define MISO_PIN 19

#define UP_PIN 22
#define DOWN_PIN 27

#define RED_PIN 4
#define BLUE_PIN 17
#define GREEN_PIN 16

#define VIDEO_FORMAT ".gif"
#define AUDIO_FORMAT ".wav"

// TODO: Add config stuff
const char* config_path = "/config.json";
int random_sleep = 0;
int spash_screen_timeout = 20;
int volume = 21;

bool audioSucceeded = false;

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
      break;
    }
    yield();
  }
}
void reset_playback()
{
  audioStopSong();
  resetGIF();
}

void go_into_standby()
{
  digitalWrite(BLUE_PIN, HIGH);
  digitalWrite(GREEN_PIN, HIGH);
  digitalWrite(RED_PIN, HIGH);
  reset_playback();
  lcd.backlight(false);
  Serial.println("Done playing.");
  setCpuFrequencyMhz(80);
  Serial.println("Going into standby.");
}

void parse_config()
{
  File config_file = SD.open(config_path);
  if (!config_file) {
      Serial.println("Failed to open config file!");
      esp_deep_sleep_start();
  }

  JsonDocument config;
  DeserializationError error = deserializeJson(config, config_file);

  if (error) {
    Serial.print("Failed to parse config file: ");
    Serial.println(error.c_str());
    esp_deep_sleep_start();
  }

  int min_sleep = config["min_sleep"];
  int max_sleep = config["max_sleep"];
  Serial.println("min and max sleep times: " + (String)min_sleep + " " + (String)max_sleep);

  if (min_sleep <= max_sleep && max_sleep != 0)
  {
    random_sleep = random(min_sleep, max_sleep + 1);
  }
  Serial.println("Random sleep time in seconds: " + (String)random_sleep);

  if(config.containsKey("messages_dir"))
  {
    messages_dir = config["messages_dir"].as<String>();
  }
  if(config.containsKey("spash_screen_path"))
  {
    spash_screen_path = config["spash_screen_path"].as<String>();
  }
  if(config.containsKey("spash_screen_timeout"))
  {
    spash_screen_timeout = config["spash_screen_timeout"];
  }
  if(config.containsKey("spash_screen_timeout"))
  {
    spash_screen_timeout = config["spash_screen_timeout"];
  }
  if(config.containsKey("volume"))
  {
    volume = config["volume"];
  }
  config_file.close();
}

// SETUP

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_OFF);         // Disable WiFi
  btStop();                    // Disable Bluetooth

  pinMode(DOWN_PIN, INPUT_PULLUP);
  pinMode(UP_PIN, INPUT_PULLUP);
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  digitalWrite(BLUE_PIN, HIGH);
  digitalWrite(GREEN_PIN, HIGH);
  digitalWrite(RED_PIN, HIGH);

  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SD_CS_PIN);
  if (!SD.begin(SD_CS_PIN, SPI, 80000000)) {
    Serial.println("SD card init failed!");
    while (1); // SD initialisation failed so wait here
  }
  Serial.println("SD Card init succeeded!");  

  parse_config();

  String selected_path = messages_dir + "/" + selectRandomVideo(messages_dir);

  initGIF();

  audioInit();

  audioSetVolume(volume);

  digitalWrite(BLUE_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
  digitalWrite(RED_PIN, HIGH);

  play_video(spash_screen_path);

  while(digitalRead(UP_PIN) == HIGH)
  {
    if(millis() >= spash_screen_timeout * 1000
      || digitalRead(DOWN_PIN) == LOW)
    {
      Serial.println("reached timeout.");
      go_into_standby();
      return;
    }
    yield();
  }

  reset_playback();
  play_video(selected_path);
  go_into_standby();
}

// TODO: write issue about not being able to run more than two videos in sequence without crash

void loop() {  
  if(millis() >= random_sleep * 1000)
  {
    esp_restart();
  }
  Serial.println("waiting for timer...");
  delay(1000);
}