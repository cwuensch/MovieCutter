#include "tap.h"

byte _Icon_Ffwd_Cpm[] =
{
  0x00, 0x08, 0x1B, 0x90, 0xFF, 0xFF, 0x00, 0x01, 0x00, 0x01, 
  0x00, 0x48, 0xE9, 0x2B, 0xFF, 0xFF, 0x06, 0x40, 
  0x00, 0x19, 0x44, 0x4A, 0x8D, 0x7F, 0xA4, 0xA9, 0x21, 0xC9, 0x28, 0x41, 0x40, 0x60, 0x30, 0x1E, 
  0x04, 0x1C, 0x14, 0x1E, 0x03, 0x01, 0x01, 0x71, 0xFE, 0x04, 0x87, 0xE4, 0xCF, 0xF6, 0x84, 0x01, 
  0xC5, 0x1C, 0xA7, 0x8B, 0x56, 0xFB, 0x87, 0x09, 0xEF, 0xBF, 0x10, 0xE5, 0x3D, 0x94, 0xC4, 0x3E, 
  0x27, 0xE1, 0x4C, 0x43, 0xEA, 0x7E, 0x73, 0xFE, 0x3E, 0x69, 0xF9, 0xBB, 0xE7, 0xCF, 0xE8, 0x42, 
  0x34, 0x00, 
};

TYPE_GrData _Icon_Ffwd_Gd =
{
  1,                              //version
  0,                              //reserved
  OSD_8888,                       //data format
  COMPRESS_Tfp,                   //compressed method
  _Icon_Ffwd_Cpm,           
  1600,                           //size
    20,                           //width
    20                            //height
};

