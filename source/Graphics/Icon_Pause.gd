#include "tap.h"

static byte _Icon_Pause_Cpm[] =
{
  0x00, 0x08, 0x1B, 0x90, 0xFF, 0xFF, 0x00, 0x01, 0x00, 0x01, 
  0x00, 0x28, 0x60, 0x5A, 0xFF, 0xFF, 0x06, 0x40, 
  0x00, 0x0E, 0x38, 0x68, 0x60, 0x1F, 0xF2, 0x2A, 0x20, 0xE2, 0x28, 0x33, 0xA0, 0x32, 0x0B, 0xA1, 
  0x12, 0x30, 0x78, 0x41, 0x80, 0xC1, 0x85, 0xD5, 0x4D, 0xE6, 0xEF, 0xB0, 0xF0, 0xF0, 0xF0, 0xFD, 
  0x1F, 0xF0, 
};

static TYPE_GrData _Icon_Pause_Gd =
{
  1,                              //version
  0,                              //reserved
  OSD_8888,                       //data format
  COMPRESS_Tfp,                   //compressed method
  _Icon_Pause_Cpm,          
  1600,                           //size
    20,                           //width
    20                            //height
};

