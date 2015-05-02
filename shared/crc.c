/*
MSP430F5 CRC Registers for HW computation of CRC16
Register                        Short Form      Type            Access  Offset  Initial
CRC Data In                     CRCDI           Read/write      Word    0000h   0000h
                                CRCDI_L         Read/write      Byte    0000h   00h
                                CRCDI_H         Read/write      Byte    0001h   00h
CRC Data In Reverse Byte(1)     CRCDIRB         Read/write      Word    0002h   0000h
                                CRCDIRB_L       Read/write      Byte    0002h   00h
                                CRCDIRB_H       Read/write      Byte    0003h   00h

CRC Initialization and Result   CRCINIRES       Read/write      Word    0004h   FFFFh
                                CRCINIRES_L     Read/write      Byte    0004h   FFh
                                CRCINIRES_H     Read/write      Byte    0005h   FFh
CRC Result Reverse(1)           CRCRESR         Read only       Word    0006h   FFFFh
                                CRCRESR_L       Read/write      Byte    0006h   FFh
                                CRCRESR_H       Read/write      Byte    0007h   FFh
*/
#include "stdint.h"
#include "msp430.h"

static inline void crc_ccitt_update(uint16_t *crc, uint8_t x)
{
     uint16_t crc_new = (uint8_t)(*crc >> 8) | (*crc << 8);
     crc_new ^= x;
     crc_new ^= (uint8_t)(crc_new & 0xff) >> 4;
     crc_new ^= crc_new << 12;
     crc_new ^= (crc_new & 0xff) << 5;
     *crc = crc_new;
}

uint16_t crc_calculate_sw(uint8_t* data, uint8_t length)
{
  uint16_t crc = 0xffff;
  uint8_t i = 0;

  for(; i<length; i++)
  {
    crc_ccitt_update(&crc, data[i]);
  }
  return crc;
}

uint16_t crc_calculate_hw(uint8_t* data, uint8_t length)
{
  CRCINIRES = 0xFFFF;
  uint8_t i = 0;
  for(; i<length; i++)
  {
    CRCDI_L = data[i];
    //CRCDIRB_L = data[i];
  }
  uint16_t crc = CRCINIRES;
  uint16_t crcMSB = (crc << 8) | (crc >> 8);
  return crcMSB;
}