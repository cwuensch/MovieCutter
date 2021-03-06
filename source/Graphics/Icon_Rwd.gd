#include "tap.h"

static byte _Icon_Rwd_Cpm[] =
{
  0x00, 0x08, 0x1B, 0x90, 0xFF, 0xFF, 0x00, 0x01, 0x00, 0x01, 
  0x00, 0x47, 0x9F, 0xD0, 0xFF, 0xFF, 0x06, 0x40, 
  0x00, 0x19, 0x43, 0x4E, 0x6D, 0x7F, 0x9A, 0x54, 0x68, 0x71, 0xA5, 0x04, 0x24, 0x0E, 0x07, 0x91, 
  0xC0, 0x24, 0xB2, 0x70, 0x38, 0x08, 0x0D, 0x05, 0xE2, 0x1A, 0x03, 0x69, 0xCF, 0x5A, 0x10, 0x07, 
  0x14, 0x72, 0x9E, 0x2D, 0x5B, 0xEE, 0x1C, 0x25, 0xBE, 0xEC, 0x43, 0x94, 0xB6, 0x4F, 0x10, 0xF8, 
  0x97, 0x84, 0xF1, 0x0F, 0xA9, 0x7A, 0xCF, 0xF8, 0xF9, 0xA7, 0xE6, 0xEF, 0x9F, 0x3F, 0xA1, 0x08, 
  0xC8, 
};

static TYPE_GrData _Icon_Rwd_Gd =
{
  1,                              //version
  0,                              //reserved
  OSD_8888,                       //data format
  COMPRESS_Tfp,                   //compressed method
  _Icon_Rwd_Cpm,            
  1600,                           //size
    20,                           //width
    20                            //height
};

