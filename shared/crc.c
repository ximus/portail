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

/* This got removed from cpu header, but seems to work anyways? needs investigation*/
#define CRCDIRB_              0x0152    /* CRC data in reverse byte Register */
sfrb(CRCDIRB_L , CRCDIRB_);
sfrb(CRCDIRB_H , CRCDIRB_+1);
sfrw(CRCDIRB, CRCDIRB_);

// msp430 CRC module uses CRC-CCITT-BR (bit reversed)
// however some variants included registers with the bits reversed
// CRCDIRB vs CRCDI
// shout out to http://e2e.ti.com/support/microcontrollers/msp430/f/166/t/323420
uint16_t crc16(uint8_t* data, uint8_t length)
{
  CRCINIRES = 0xFFFF;
  uint8_t i = 0;
  for(; i<length; i++)
  {
    CRCDIRB_L = data[i];
  }
  return CRCINIRES;
}