#include "tap.h"

byte _Button_Up_small_Cpm[] =
{
  0x00, 0x08, 0x1B, 0x90, 0xFF, 0xFF, 0x00, 0x01, 0x00, 0x01, 
  0x00, 0x83, 0xEE, 0x16, 0xFF, 0xFF, 0x03, 0x20, 
  0x00, 0x61, 0x53, 0x4E, 0xB5, 0x47, 0xA6, 0xEA, 0x09, 0x68, 0x25, 0xA0, 0x74, 0x04, 0x96, 0x81, 
  0x13, 0x53, 0xD0, 0x15, 0x55, 0x8A, 0xC8, 0xD1, 0x9F, 0xB7, 0xD9, 0x92, 0x74, 0x9B, 0xA4, 0xEE, 
  0x93, 0x01, 0x88, 0xC0, 0xE1, 0xF8, 0x4D, 0x49, 0x7A, 0x60, 0x50, 0x08, 0x66, 0xDD, 0xB1, 0x8C, 
  0x60, 0xEA, 0xED, 0xDF, 0x16, 0xB5, 0x8F, 0xD4, 0x87, 0xA9, 0x4A, 0x1E, 0xAD, 0x29, 0x41, 0xEF, 
  0x8E, 0x43, 0xD2, 0x94, 0x84, 0x21, 0x1B, 0xDB, 0xDE, 0xE2, 0x10, 0x80, 0x7B, 0xDF, 0xCF, 0x1C, 
  0x83, 0x90, 0xF3, 0x9C, 0xE0, 0xC6, 0x37, 0x8B, 0x29, 0x48, 0x4E, 0x73, 0x18, 0xC6, 0x33, 0xB9, 
  0x5F, 0x3C, 0xB2, 0x0F, 0xC0, 0xF8, 0xC6, 0x39, 0xFC, 0xF5, 0xAD, 0x43, 0x9C, 0xEC, 0xF9, 0xF6, 
  0xB5, 0xA2, 0xD6, 0xB7, 0xD1, 0xFF, 0x2F, 0xF1, 0xFA, 0x27, 0xD5, 0xD0, 0xB0, 
};

TYPE_GrData _Button_Up_small_Gd =
{
  1,                              //version
  0,                              //reserved
  OSD_8888,                       //data format
  COMPRESS_Tfp,                   //compressed method
  _Button_Up_small_Cpm,     
   800,                           //size
    20,                           //width
    10                            //height
};

