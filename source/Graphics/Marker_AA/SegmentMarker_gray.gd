#include "tap.h"

byte _SegmentMarker_gray_Cpm[] =
{
  0x00, 0x08, 0x1B, 0x90, 0xFF, 0xFF, 0x00, 0x01, 0x00, 0x01, 
  0x00, 0x98, 0x69, 0x6E, 0xFF, 0xFF, 0x01, 0x20, 
  0x00, 0x8C, 0x52, 0x72, 0xD9, 0x2B, 0x0E, 0xF7, 0x01, 0xE7, 0x04, 0x58, 0x9C, 0x08, 0x58, 0xEC, 
  0x11, 0x22, 0xA4, 0x10, 0x58, 0xEC, 0x74, 0x44, 0xAA, 0x36, 0x08, 0xAC, 0x57, 0x1E, 0x7E, 0xF7, 
  0xE8, 0x4E, 0x08, 0x29, 0x42, 0xD4, 0x84, 0x75, 0xD7, 0x58, 0xEB, 0xAE, 0xA5, 0xEE, 0xFB, 0xEE, 
  0x23, 0x18, 0x8E, 0x38, 0xE0, 0x77, 0xDF, 0x63, 0x1C, 0x71, 0x1F, 0xBF, 0x85, 0xFC, 0xA3, 0x42, 
  0x10, 0x0E, 0x73, 0xCA, 0x7E, 0x79, 0xE4, 0x55, 0x55, 0x52, 0xBE, 0xEB, 0xAE, 0x10, 0x84, 0x24, 
  0xBF, 0x1E, 0x5C, 0x5F, 0x8F, 0xB6, 0xDB, 0x43, 0x9C, 0xE9, 0xBD, 0x8C, 0x60, 0xDB, 0x6D, 0xA6, 
  0xFA, 0x28, 0xA0, 0x61, 0x86, 0x03, 0x2C, 0xB2, 0x16, 0x59, 0x60, 0x7B, 0xDE, 0x29, 0xA6, 0x99, 
  0xDE, 0x42, 0x52, 0x91, 0xA6, 0x9A, 0x05, 0xAD, 0x61, 0xAD, 0x6F, 0xB5, 0x90, 0x63, 0x18, 0x5F, 
  0x7D, 0xE3, 0x5D, 0x75, 0x9E, 0x2F, 0xF0, 0xA5, 0x2B, 0xDC, 0x9F, 0xE0, 0xBA, 0x3C, 0xF3, 0xCF, 
  0xDB, 0xC0, 
};

TYPE_GrData _SegmentMarker_gray_Gd =
{
  1,                              //version
  0,                              //reserved
  OSD_8888,                       //data format
  COMPRESS_Tfp,                   //compressed method
  _SegmentMarker_gray_Cpm,  
   288,                           //size
     8,                           //width
     9                            //height
};

