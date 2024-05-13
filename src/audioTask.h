#include "Arduino.h"
#include "Audio.h"
#include <SD.h>
#include "FS.h"

Audio audio(true, I2S_DAC_CHANNEL_LEFT_EN);

//****************************************************************************************
//                                   A U D I O _ T A S K                                 *
//****************************************************************************************

struct audioMessage{
    uint8_t     cmd;
    const char* txt;
    uint32_t    value;
    uint32_t    ret;
    FS*         fs;  // Pointer to the file system object
} audioTxMessage, audioRxMessage;

enum : uint8_t { SET_VOLUME, GET_VOLUME, CONNECTTOHOST, CONNECTTOFS, STOP_SONG, TERMINATE_TASK};

QueueHandle_t audioSetQueue = NULL;
QueueHandle_t audioGetQueue = NULL;

void CreateQueues(){
    audioSetQueue = xQueueCreate(10, sizeof(struct audioMessage));
    audioGetQueue = xQueueCreate(10, sizeof(struct audioMessage));
}

void audioTask(void *parameter) {
    CreateQueues();
    if(!audioSetQueue || !audioGetQueue){
        log_e("queues are not initialized");
        while(true){;}  // endless loop
    }

    struct audioMessage audioRxTaskMessage;
    struct audioMessage audioTxTaskMessage;

    // Setup audio and 
    audio.forceMono(true);
    audio.setVolume(21);

    while(true){
        if(xQueueReceive(audioSetQueue, &audioRxTaskMessage, 1) == pdPASS) {
            if(audioRxTaskMessage.cmd == SET_VOLUME){
                audioTxTaskMessage.cmd = SET_VOLUME;
                audio.setVolume(audioRxTaskMessage.value);
                audioTxTaskMessage.ret = 1;
                xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
            }
            else if(audioRxTaskMessage.cmd == CONNECTTOHOST){
                audioTxTaskMessage.cmd = CONNECTTOHOST;
                audioTxTaskMessage.ret = audio.connecttohost(audioRxTaskMessage.txt);
                xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
            }
            else if(audioRxTaskMessage.cmd == CONNECTTOFS){
                audioTxTaskMessage.cmd = CONNECTTOFS;
                // Use the file system pointer and path from the received message
                if (audioRxTaskMessage.fs != nullptr) {
                    audioTxTaskMessage.ret = audio.connecttoFS(*audioRxTaskMessage.fs, audioRxTaskMessage.txt);
                } else {
                    audioTxTaskMessage.ret = 0;  // Indicate failure if fs is nullptr
                }
                xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
            }
            else if(audioRxTaskMessage.cmd == GET_VOLUME){
                audioTxTaskMessage.cmd = GET_VOLUME;
                audioTxTaskMessage.ret = audio.getVolume();
                xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
            } 
            else if (audioRxTaskMessage.cmd == STOP_SONG) {
                audioTxTaskMessage.cmd = STOP_SONG;
                audio.stopSong(); // Stop the currently playing song
                audioTxTaskMessage.ret = 1;
                xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
            } else if (audioRxTaskMessage.cmd == TERMINATE_TASK) {
                log_i("Terminating audio task...");
                vTaskDelete(NULL); // Terminate the current task
            } 
            else 
            {
                log_i("error");
            }
        }
        audio.loop();
        if (!audio.isRunning()) {
          sleep(1);
        }
    }
}

void audioInit() {
    xTaskCreatePinnedToCore(
        audioTask,             /* Function to implement the task */
        "audioplay",           /* Name of the task */
        6144,                  /* Stack size in words */
        NULL,                  /* Task input parameter */
        2 | portPRIVILEGE_BIT, /* Priority of the task */
        NULL,                  /* Task handle. */
        1                      /* Core where the task should run */
    );
}

audioMessage transmitReceive(audioMessage msg){
    xQueueSend(audioSetQueue, &msg, portMAX_DELAY);
    if(xQueueReceive(audioGetQueue, &audioRxMessage, portMAX_DELAY) == pdPASS){
        if(msg.cmd != audioRxMessage.cmd){
            log_e("wrong reply from message queue");
        }
    }
    return audioRxMessage;
}

void audioSetVolume(uint8_t vol){
    audioTxMessage.cmd = SET_VOLUME;
    audioTxMessage.value = vol;
    audioMessage RX = transmitReceive(audioTxMessage);
}

uint8_t audioGetVolume(){
    audioTxMessage.cmd = GET_VOLUME;
    audioMessage RX = transmitReceive(audioTxMessage);
    return RX.ret;
}

bool audioConnecttohost(const char* host){
    audioTxMessage.cmd = CONNECTTOHOST;
    audioTxMessage.txt = host;
    audioMessage RX = transmitReceive(audioTxMessage);
    return RX.ret;
}

bool audioConnecttoFS(FS& fs, const char* path){
    audioTxMessage.cmd = CONNECTTOFS;
    audioTxMessage.txt = path;
    audioTxMessage.fs = &fs;  // Pass the address of the file system object
    audioMessage RX = transmitReceive(audioTxMessage);
    return RX.ret;
}

bool audioStopSong() {
    audioTxMessage.cmd = STOP_SONG;
    audioMessage RX = transmitReceive(audioTxMessage);
    return RX.ret;
}

void audioTerminateTask() {
    audioTxMessage.cmd = TERMINATE_TASK;
    transmitReceive(audioTxMessage);
}