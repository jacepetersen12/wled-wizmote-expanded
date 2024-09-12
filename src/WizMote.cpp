#include <WizMote.h>



#define DOUBLE_PRESS_TIME       600
#define TRIPLE_PRESS_TIME       600
#define LONG_PRESS_THRESHOLD    1000
#define DEBOUNCE_DELAY          50

// Button Mappings:
// - Button 0: Off Button (immediately returns)
// - Button 1: On Button (immediately returns)
// - Button 2: Button 2 on the Wiz Remote (supports multi-press and long press)
// - Button 3: Button 1 on the Wiz Remote (supports multi-press and long press)
// - Button 4: Button 4 on the Wiz Remote (supports multi-press and long press)
// - Button 5: Button 3 on the Wiz Remote (supports multi-press and long press)
// - Button 6: Brightness Up (immediately returns)
// - Button 7: Brightness Down (immediately returns)
// - Button 8: Moon Button (immediately returns, Sleep Button)

uint8_t WizMoteClass::broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

WizMoteClass::WizMoteClass() {}

// Unique integers for each button press type
uint8_t buttonPressCodes[9][7] = {
    {1, 0, 0, 0, 0, 0, 2},      // Button 0: Off Button (immediately returns)
    {3, 0, 0, 0, 0, 0, 4},      // Button 1: On Button (immediately returns)
    {5, 6, 7, 8, 9, 10, 11},    // Button 2: Button 2 on the Wiz Remote
    {12, 13, 14, 15, 16, 17, 18}, // Button 3: Button 1 on the Wiz Remote
    {19, 20, 21, 22, 23, 24, 25}, // Button 4: Button 4 on the Wiz Remote
    {26, 27, 28, 29, 30, 31, 32}, // Button 5: Button 3 on the Wiz Remote
    {33, 0, 0, 0, 0, 0, 34},    // Button 6: Brightness Up (immediately returns)
    {35, 0, 0, 0, 0, 0, 36},    // Button 7: Brightness Down (immediately returns)
    {37, 0, 0, 0, 0, 0, 38}     // Button 8: Moon Button (immediately returns, Sleep Button)
}; 

void WizMoteClass::begin() {
    // Prevent calling this method a second time
    if (initialized) {
        return;
    }

    // Initialize EEPROM
    EEPROM.begin(EEPROM_SIZE);

    // Set initialization status
    initialized = true;
}

void WizMoteClass::initializeEspNow() {
    // Set device as a Wi-Fi Station
    if (WiFi.mode(WIFI_STA) != true) {
        printException("setting Wi-Fi mode failed");
    }

    // Immediately disconnect from any networks
    if (WiFi.disconnect() != true) {
        printException("disconnecting Wi-Fi failed");
    }

    // Initialize ESP-NOW
    if (esp_now_init() != OK) {
        printException("initializing ESP-NOW failed");
    }

    // Set this device's role to CONTROLLER
    if (esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER) != OK) {
        printException("setting ESP-NOW role failed");
    }
}

void WizMoteClass::setChannel(uint8_t ch) {
    esp_now_del_peer(WizMoteClass::broadcastAddress);
    wifi_set_channel(ch);
    esp_now_add_peer(WizMoteClass::broadcastAddress, ESP_NOW_ROLE_SLAVE, ch, NULL, 0);
}

uint8_t WizMoteClass::readButtonPress() {
    // Define pins and debounce parameters
    const int load = 13;
    const int dataIn = 5;
    const int clockIn = 4;
    const int sleepButtonPin = 14;

    static uint8_t pressCounts[9] = {0};
    static unsigned long lastPressTime[9] = {0};
    static unsigned long pressStartTime[9] = {0};
    static bool buttonPressed[9] = {false};
    static int lastButtonIndex = -1; // Track the last button pressed

    // Ensure pins are set correctly
    pinMode(load, OUTPUT);
    pinMode(clockIn, OUTPUT);
    pinMode(dataIn, INPUT);
    pinMode(sleepButtonPin, INPUT);

    while (true) {  // Loop until a button press is classified
        // Read data from shift register
        digitalWrite(load, LOW);
        delayMicroseconds(5);
        digitalWrite(load, HIGH);
        delayMicroseconds(50);

        uint8_t incoming = shiftIn165(dataIn, clockIn, MSBFIRST);
        bool sleepButtonPressed = digitalRead(sleepButtonPin) == LOW;

        // Loop through button states and classify presses
        for (int i = 0; i < 9; i++) {  // Loop includes all buttons including the Moon button (index 8)
            bool isPressed = (i < 8) ? !(incoming & (1 << i)) : sleepButtonPressed; // Handle the Moon button separately
            unsigned long currentTime = millis();

            // Handle button state change
            if (isPressed && !buttonPressed[i]) {
                // If a different button is pressed, reset the count
                if (lastButtonIndex != i && lastButtonIndex >= 0 && lastButtonIndex < 9) {
                    pressCounts[lastButtonIndex] = 0; // Reset count of the previous button
                }

                buttonPressed[i] = true;
                pressStartTime[i] = currentTime;
                lastPressTime[i] = currentTime;
                lastButtonIndex = i; // Update the last pressed button

                // Immediate return for buttons 0, 1, 6, 7, 8
                if (i == 0) {
                    Serial.println("Off Button pressed."); // Button 0: Off Button
                    return buttonPressCodes[i][0]; // Single press code
                } else if (i == 1) {
                    Serial.println("On Button pressed."); // Button 1: On Button
                    return buttonPressCodes[i][0]; // Single press code
                } else if (i == 6) {
                    Serial.println("Brightness Up pressed."); // Button 6: Brightness Up
                    return buttonPressCodes[i][0]; // Single press code
                } else if (i == 7) {
                    Serial.println("Brightness Down pressed."); // Button 7: Brightness Down
                    return buttonPressCodes[i][0]; // Single press code
                } else if (i == 8) {
                    Serial.println("Moon Button pressed."); // Button 8: Moon Button
                    return buttonPressCodes[i][0]; // Single press code
                }
            } else if (!isPressed && buttonPressed[i]) {
                buttonPressed[i] = false;
                unsigned long pressDuration = currentTime - pressStartTime[i];

                // Immediately handle long press for buttons 2-5
                if (pressDuration >= LONG_PRESS_THRESHOLD && (i >= 2 && i <= 5)) {
                    Serial.printf("Button %d long pressed.\n", i);
                    return buttonPressCodes[i][6]; // Long press code
                }

                if (pressDuration > DEBOUNCE_DELAY && (i >= 2 && i <= 5)) {
                    pressCounts[i]++;
                }

                lastPressTime[i] = currentTime;
            }

            // Check for multi-press types after a press has been completed for buttons 2-5
            if (!buttonPressed[i] && pressCounts[i] > 0 && (i >= 2 && i <= 5)) {
                unsigned long timeSinceLastPress = currentTime - lastPressTime[i];

                if (pressCounts[i] == 1 && timeSinceLastPress > DOUBLE_PRESS_TIME) {
                    Serial.printf("Button %d single pressed.\n", i);
                    pressCounts[i] = 0;
                    return buttonPressCodes[i][0]; // Single press code
                } else if (pressCounts[i] == 2 && timeSinceLastPress > TRIPLE_PRESS_TIME) {
                    Serial.printf("Button %d double pressed.\n", i);
                    pressCounts[i] = 0;
                    return buttonPressCodes[i][1]; // Double press code
                } else if (pressCounts[i] == 3 && timeSinceLastPress > TRIPLE_PRESS_TIME) {
                    Serial.printf("Button %d triple pressed.\n", i);
                    pressCounts[i] = 0;
                    return buttonPressCodes[i][2]; // Triple press code
                } else if (pressCounts[i] == 4 && timeSinceLastPress > TRIPLE_PRESS_TIME) {
                    Serial.printf("Button %d quadruple pressed.\n", i);
                    pressCounts[i] = 0;
                    return buttonPressCodes[i][3]; // Quadruple press code
                } else if (pressCounts[i] == 5 && timeSinceLastPress > TRIPLE_PRESS_TIME) {
                    Serial.printf("Button %d quintuple pressed.\n", i);
                    pressCounts[i] = 0;
                    return buttonPressCodes[i][4]; // Quintuple press code
                } else if (pressCounts[i] >= 6 && timeSinceLastPress > TRIPLE_PRESS_TIME) {
                    Serial.printf("Button %d sextuple pressed.\n", i);
                    pressCounts[i] = 0;
                    return buttonPressCodes[i][5]; // Sextuple press code
                }
            }
        }

        // Add a small delay to avoid overwhelming the CPU
        delay(10);
    }
}

uint32_t WizMoteClass::nextSequenceNumber() {
    // Read sequence number from EEPROM
    EEPROM.get(EEPROM_SEQUENCE_OFFSET, sequenceNumber);

    // Increment sequence number
    sequenceNumber++;

    // Write back sequence number into EEPROM
    EEPROM.put(EEPROM_SEQUENCE_OFFSET, sequenceNumber);
    EEPROM.commit();

    return sequenceNumber;
}

void WizMoteClass::broadcast(uint8_t *data, size_t data_size) {
    if (esp_now_send(WizMoteClass::broadcastAddress, data, data_size) != OK) {
        printException("sending ESP-NOW message failed");
    } else {
        Serial.print("success!");
    }
}

void WizMoteClass::powerOff() {
    // Disable the voltage regulator, so the remote turns off
    digitalWrite(VOLTAGE_REGULATOR_PIN, LOW);
}

void WizMoteClass::registerSendCallback(esp_now_send_cb_t cb) {
    if (esp_now_register_send_cb(cb) != OK) {
        printException("registering ESP-NOW send callback failed");
    }
}

void WizMoteClass::printException(const char *message) {
    Serial.println();
    Serial.println("========================");
    Serial.println("  An unexpected error occurred.");
    Serial.printf("  message: %s\n", message);
    Serial.println("========================");
    Serial.println();
    Serial.println("System will restart in 5 seconds...");

    delay(5000);
    system_restart();
}

uint8_t WizMoteClass::shiftIn165(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder) {
    uint8_t value = 0;
    for (uint8_t i = 0; i < 8; ++i) {
        digitalWrite(clockPin, LOW);
        if (bitOrder == LSBFIRST) {
            value |= digitalRead(dataPin) << i;
        } else {
            value |= digitalRead(dataPin) << (7 - i);
        }
        digitalWrite(clockPin, HIGH);
    }
    return value;
}
