#ifndef WizMote_h
#define WizMote_h

#include <ESP8266WiFi.h>
#include <Arduino.h>
#include <EEPROM.h>
#include <Wire.h>

#include <configuration.h>
#include <user_interface.h>
#include <espnow.h>

class WizMoteClass {

public:

    WizMoteClass();

    void begin();

    void initializeEspNow();

    uint8_t readButtonPress();

    uint32_t nextSequenceNumber();

    void powerOff();

    void setChannel(uint8_t ch);

    void registerSendCallback(esp_now_send_cb_t cb);

    void broadcast(uint8_t *data, size_t data_size);

private:

    static uint8_t shiftIn165(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder);

    void printException(const char *message);

    bool initialized = false;

    uint32_t sequenceNumber;

    static uint8_t broadcastAddress[];
};

#endif