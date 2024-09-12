#include <Arduino.h>
#include <WizMote.h>
#include <configuration.h>

// This is kind of an esoteric structure because it's pulled from the "Wizmote"
// product spec. That remote is used as the baseline for behavior and availability
// since it's broadly commercially available and works out of the box as a drop-in
typedef struct WizMoteMessageStructure {
  uint8_t program;  // 0x91 for ON button, 0x81 for all others
  uint8_t seq[4];   // Incremental sequence number 32-bit unsigned integer LSB first
  uint8_t byte5;    // Unknown (seen 0x20)
  uint8_t button;   // Identifies which button is being pressed
  uint8_t byte8;    // Unknown, but always 0x01
  uint8_t byte9;    // Unknown, but always 0x64
  uint8_t byte10;   // Unknown, maybe checksum
  uint8_t byte11;   // Unknown, maybe checksum
  uint8_t byte12;   // Unknown, maybe checksum
  uint8_t byte13;   // Unknown, maybe checksum
} message_structure_t;

message_structure_t broadcast_data;
WizMoteClass WizMote;

uint8_t ch = 1;
uint8_t repeat = 10;

void on_data_sent(uint8_t *mac_addr, uint8_t sendStatus) {
  delayMicroseconds(5000);
  ch++;
  if (ch <= 14) {
    WizMote.setChannel(ch);
    WizMote.broadcast((uint8_t *) &broadcast_data, sizeof(message_structure_t));
  } else {
    if (repeat > 0) {
      repeat--;
      ch = 1;
      WizMote.setChannel(ch);
      WizMote.broadcast((uint8_t *) &broadcast_data, sizeof(message_structure_t));
    } else {
      WizMote.powerOff();
    }
  }
}

void setup() {
  

   // Perform an initial read attempt to catch any button press immediately
  broadcast_data.button = WizMote.readButtonPress();

// Initialize Serial communication
  Serial.begin(BAUD_RATE);

  // Initialize the WizMote
  WizMote.begin();

  // Initialize the broadcast data structure with default values
  broadcast_data.program = 0x81;
  broadcast_data.byte5 = 0x20;
  broadcast_data.byte8 = 0x01;
  broadcast_data.byte9 = 0x64;

 

  // Set sequence number for the broadcast
  uint32_t seq = WizMote.nextSequenceNumber();
  memcpy(broadcast_data.seq, &seq, sizeof(seq));

  // Initialize ESP-NOW and set the initial channel
  WizMote.initializeEspNow();
  WizMote.setChannel(ch);

  // Register send callback
  WizMote.registerSendCallback(on_data_sent);

  // Broadcast the message if a button was pressed initially
  if (broadcast_data.button != 0) {
    WizMote.broadcast((uint8_t *) &broadcast_data, sizeof(message_structure_t));
  }
}

void loop() {
  // Read button press in the loop to capture subsequent presses
  uint8_t buttonPressed = WizMote.readButtonPress();

  // Check if a valid button was pressed
  if (buttonPressed != 0) {
    // Update the button in the broadcast data structure
    broadcast_data.button = buttonPressed;

    // Update the sequence number
    uint32_t seq = WizMote.nextSequenceNumber();
    memcpy(broadcast_data.seq, &seq, sizeof(seq));

    // Broadcast message to ESP-NOW receivers
    WizMote.broadcast((uint8_t *) &broadcast_data, sizeof(message_structure_t));

    // Reset channel and repeat counters if necessary
    ch = 1;
    repeat = 10;
    WizMote.setChannel(ch);
  }

  // Add a small delay to avoid overwhelming the CPU
  delay(10);
}
