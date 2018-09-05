#include <Wire.h>
//#include <SoftwareSerial.h>
//SoftwareSerial softSerial(8, 9);


//
// Code down here is from Paweł Spychalski (DzikuVx)
// https://github.com/DzikuVx/ltm_telemetry_reader/
//

enum ltmStates {
  IDLE,
  HEADER_START1,
  HEADER_START2,
  HEADER_MSGTYPE,
  HEADER_DATA
};

#define LONGEST_FRAME_LENGTH 18

/*
 * LTM based on https://github.com/KipK/Ghettostation/blob/master/GhettoStation/LightTelemetry.cpp implementation
 */

#define GFRAMELENGTH 18
#define AFRAMELENGTH 10
#define SFRAMELENGTH 11
#define OFRAMELENGTH 18
#define NFRAMELENGTH 10
#define XFRAMELENGTH 10


typedef struct remoteData_s {
  int pitch;
  int roll;
  int heading;
  uint16_t voltage;
  byte rssi;
  bool armed;
  bool failsafe;
  byte flightmode;

  int32_t latitude;
  int32_t longitude;
  int32_t altitude;
  uint8_t groundSpeed; 
  int16_t hdop;
  uint8_t gpsFix;
  uint8_t gpsSats;

  int32_t homeLatitude;
  int32_t homeLongitude;

  uint8_t sensorStatus;
} remoteData_t;

remoteData_t remoteData;

uint8_t serialBuffer[LONGEST_FRAME_LENGTH];
uint8_t state = IDLE;
char frameType;
byte frameLength;
byte receiverIndex;

byte readByte(uint8_t offset) {
  return serialBuffer[offset];
}

int readInt(uint8_t offset) {
  return (int) serialBuffer[offset] + ((int) serialBuffer[offset + 1] << 8);
}

int32_t readInt32(uint8_t offset) {
  return (int32_t) serialBuffer[offset] + ((int32_t) serialBuffer[offset + 1] << 8) + ((int32_t) serialBuffer[offset + 2] << 16) + ((int32_t) serialBuffer[offset + 3] << 24);
}

uint32_t nextDisplay = 0;


//
// Code above is from Paweł Spychalski (DzikuVx)
// https://github.com/DzikuVx/ltm_telemetry_reader/
//



void setup() {
  Serial.begin(100000,SERIAL_8E2); // Unconventional setting for S.BUS. 
                                   // same UART on FC used for Rx (RC commands) and LTM
  Wire.begin();
  //softSerial.begin(4800);          // Use softserial for radio downlink 
                                   // because the ATmega328 only has 1 UART
}

int16_t alt=0;

void loop() {
  uint32_t currentMillis, prevMillis, diffMillis;
  uint16_t currentAlt, prevAlt, vario;
  currentMillis=millis();
  currentAlt=remoteData.altitude;
  diffMillis=currentMillis-prevMillis;
  vario=(100*(currentAlt-prevAlt))/diffMillis;
  prevMillis=currentMillis;
  prevAlt=currentAlt;

  if (millis() >= nextDisplay) {
    alt=remoteData.altitude/100;
    sendTemp(3,alt);
    sendVario(4,vario);
    sendAlt(5, alt);
    nextDisplay = millis() + 500;
  }


//
// Code down here is from Paweł Spychalski (DzikuVx)
// https://github.com/DzikuVx/ltm_telemetry_reader/
//

  if (Serial.available()) {

    char data = Serial.read();

    if (state == IDLE) {
      if (data == '$') {
        state = HEADER_START1;
      }
    } else if (state == HEADER_START1) {
      if (data == 'T') {
        state = HEADER_START2;
      } else {
        state = IDLE;
      }
    } else if (state == HEADER_START2) {
      frameType = data;
      state = HEADER_MSGTYPE;
      receiverIndex = 0;

      switch (data) {

        case 'G':
          frameLength = GFRAMELENGTH;
          break;
        case 'A':
          frameLength = AFRAMELENGTH;
          break;
        case 'S':
          frameLength = SFRAMELENGTH;
          break;
        case 'O':
          frameLength = OFRAMELENGTH;
          break;
        case 'N':
          frameLength = NFRAMELENGTH;
          break;
        case 'X':
          frameLength = XFRAMELENGTH;
          break;
        default:
          state = IDLE;
      }

    } else if (state == HEADER_MSGTYPE) {

      /*
       * Check if last payload byte has been received.
       */
      if (receiverIndex == frameLength - 4) {
        /*
         * If YES, check checksum and execute data processing
         */

        if (frameType == 'A') {
            remoteData.pitch = readInt(0);
            remoteData.roll = readInt(2);
            remoteData.heading = readInt(4);
        }

        if (frameType == 'S') {
            remoteData.voltage = readInt(0);
            remoteData.rssi = readByte(4);

            byte raw = readByte(6);
            remoteData.flightmode = raw >> 2;
        }

        if (frameType == 'G') {
            remoteData.latitude = readInt32(0);
            remoteData.longitude = readInt32(4);
            remoteData.groundSpeed = readByte(8);
            remoteData.altitude = readInt32(9);

            uint8_t raw = readByte(13);
            remoteData.gpsSats = raw >> 2;
            remoteData.gpsFix = raw & 0x03;
        }

        if (frameType == 'X') {
            remoteData.hdop = readInt(0);
            remoteData.sensorStatus = readByte(2);
        }
        
        state = IDLE;
        memset(serialBuffer, 0, LONGEST_FRAME_LENGTH);

      } else {
        /*
         * If no, put data into buffer
         */
        serialBuffer[receiverIndex++] = data;
      }

    }

  } // endif  (Serial.available())

//
// Code above is from Paweł Spychalski (DzikuVx)
// https://github.com/DzikuVx/ltm_telemetry_reader/
//

}


void sendAlt(byte slot, int altitude) {
  byte data;
  Wire.beginTransmission(0x78);
  Wire.write(slot+32);
  data=(altitude >> 8) | B11000000;
  Wire.write(data);
  data=altitude & 255;
  Wire.write(data);
  Wire.endTransmission();
}


void sendVario(byte slot, int16_t vario) {
  byte data;
  Wire.beginTransmission(0x78);
  Wire.write(slot+32);
  data=(vario >> 8) & 255;
  Wire.write(data);
  data=vario & 255;
  Wire.write(data);
  Wire.endTransmission();
}


void sendTemp(byte slot, uint16_t temp) {
  byte data;
  Wire.beginTransmission(0x78);
  Wire.write(slot+32);
  data=(temp+100)%256;
  Wire.write(data);
  data=((temp+100)/256)+128;
  Wire.write(data);
  Wire.endTransmission();
}






