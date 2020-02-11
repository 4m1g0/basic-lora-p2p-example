#include <Arduino.h>
#include <RadioLib.h>

#define PRINT_BUFF(BUFF, LEN) { \
  for(size_t i = 0; i < LEN; i++) { \
    Serial.print(F("0x")); \
    Serial.print(BUFF[i], HEX); \
    Serial.print('\t'); \
    Serial.write(BUFF[i]); \
    Serial.println(); \
  } }

// pin definition
#define LORA_NSS D8
#define LORA_DI00 D1
#define LORA_DI01 D2
#define LORA_RST D4
// modem configuration
#define LORA_CARRIER_FREQUENCY                          868.0f  // MHz
#define LORA_BANDWIDTH                                  125.0f  // kHz dual sideband
#define LORA_SPREADING_FACTOR                           7
#define LORA_CODING_RATE                                8       // 4/8, Extended Hamming
#define LORA_OUTPUT_POWER                               14      // dBm
#define LORA_CURRENT_LIMIT                              120     // mA

SX1276 lora = new Module(LORA_NSS, LORA_DI00, LORA_RST, LORA_DI01);

bool receivedFlag = false;
bool enableInterrupt = true;

void ICACHE_RAM_ATTR setFlag(void) {
  // check if the interrupt is enabled
  if(!enableInterrupt) {
    return;
  }

  // we got a packet, set the flag
  receivedFlag = true;
}

void handleReceivedPacket();
void sendHello();

void setup() {
  Serial.begin(115200);
  Serial.print(F("[SX12xx] Initializing ... "));
  int state = lora.begin(LORA_CARRIER_FREQUENCY,
                          LORA_BANDWIDTH,
                          LORA_SPREADING_FACTOR,
                          LORA_CODING_RATE,
                          SX127X_SYNC_WORD,
                          17,
                          (uint8_t)LORA_CURRENT_LIMIT);
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }
  //lora.setDio1Action(setFlag);
  lora.setDio0Action(setFlag);
  
  // start listening for LoRa packets
  Serial.print(F("[SX12x8] Starting to listen ... "));
  state = lora.startReceive();
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }
}

void loop() {
  if(receivedFlag) {
    enableInterrupt = false;
    receivedFlag = false;
    handleReceivedPacket();
    enableInterrupt = true;
    lora.startReceive();
  }

  if (Serial.available()) {
    enableInterrupt = false;
    char c = Serial.read();
    Serial.print(c);

    switch (c) {
      case 'p':
        sendHello();
        break;
      case 's':
        //sendSysInfo();
        break;
      case 'm':
        //sendSysInfo(true);
        break;
    }

    delay(20);
    // dump the serial buffer
    while(Serial.available()) {
      Serial.read();
      delay(1);
    }

    enableInterrupt = true;
    lora.startReceive();
  }
}

void handleReceivedPacket() {
  // read received data
  size_t respLen = lora.getPacketLength();
  uint8_t* respFrame = new uint8_t[respLen];
  int state = lora.readData(respFrame, respLen);

  if (state == ERR_NONE) {
    PRINT_BUFF(respFrame, respLen);
    Serial.println();
  } else if (state == ERR_CRC_MISMATCH) {
    // packet was received, but is malformed
    Serial.println(F("[SX12x8] CRC error!"));
  } else {
    // some other error occurred
    Serial.print(F("[SX12x8] Failed, code "));
    Serial.println(state);
  }
}

void sendHello() {
  Serial.print(F("Sending hello frame ... "));

  uint8_t frame[12] = "hello world";
  int state = lora.transmit(frame, 12);

  if (state == ERR_NONE) {
    Serial.println(F("sent successfully!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
  }

  Serial.println();
}