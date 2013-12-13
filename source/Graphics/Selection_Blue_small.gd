#include "tap.h"

byte _Selection_Blue_small_Cpm[] =
{
  0x00, 0x08, 0x1B, 0x90, 0xFF, 0xFF, 0x00, 0x01, 0x00, 0x01, 
  0x00, 0xF5, 0x8E, 0x76, 0xFF, 0xFF, 0x3A, 0x80, 
  0x00, 0xB7, 0x5B, 0x55, 0xD7, 0x0A, 0x97, 0xFB, 0xF1, 0xA1, 0x90, 0x3C, 0x0C, 0x0A, 0x0A, 0x0C, 
  0x0C, 0x04, 0x1A, 0x32, 0x23, 0xB2, 0x12, 0x04, 0x64, 0x2C, 0x1D, 0xA8, 0x35, 0x01, 0x01, 0x11, 
  0x01, 0x50, 0x57, 0x5A, 0xDA, 0xB4, 0x0D, 0x05, 0x40, 0xD2, 0x02, 0x80, 0xB9, 0xD9, 0x73, 0x94, 
  0x14, 0x14, 0x04, 0x14, 0x14, 0x0B, 0x24, 0x7B, 0x75, 0x02, 0xD7, 0x00, 0x96, 0x3D, 0xEC, 0x35, 
  0xC0, 0x5B, 0xC4, 0x3F, 0xDC, 0x01, 0x16, 0x87, 0x42, 0x02, 0xEE, 0xD1, 0x46, 0x51, 0x1E, 0x0C, 
  0x92, 0x49, 0x2E, 0xEC, 0xF4, 0x04, 0x5B, 0xBE, 0x1F, 0xE6, 0x9E, 0x22, 0xD5, 0x0F, 0xFE, 0xB2, 
  0x74, 0x08, 0xB5, 0x43, 0xFD, 0xB8, 0x79, 0xC4, 0x5A, 0xA1, 0xFE, 0xCB, 0xFC, 0xC2, 0x2D, 0x50, 
  0xFF, 0x5F, 0xC4, 0xE1, 0x16, 0xA8, 0x7F, 0xAB, 0xDA, 0x68, 0x8B, 0x54, 0x3F, 0xD3, 0xEB, 0xCA, 
  0x22, 0xD5, 0x0F, 0xF4, 0x79, 0x88, 0xFF, 0x2A, 0x1F, 0xE5, 0xF0, 0x98, 0x22, 0xD5, 0x0F, 0xF0, 
  0x59, 0xE4, 0x11, 0x6A, 0x87, 0xFE, 0x5D, 0x90, 0x22, 0xD5, 0x0F, 0xEA, 0x53, 0x81, 0x16, 0xA8, 
  0x7F, 0xDD, 0x4A, 0x04, 0x5A, 0xA1, 0xFD, 0x6E, 0x98, 0x11, 0x6A, 0x87, 0xF6, 0xBA, 0xA0, 0x45, 
  0xAA, 0x1F, 0xDD, 0xEB, 0x81, 0x16, 0xA8, 0x7F, 0xE3, 0xDB, 0x02, 0x2D, 0x50, 0xFF, 0xD2, 0xAC, 
  0x08, 0xB5, 0x43, 0xFF, 0x7A, 0xF0, 0x22, 0xD5, 0x0F, 0xFE, 0x6C, 0x47, 0xDB, 0x7B, 0xF3, 0x8F, 
  0xF7, 0xAF, 0xC1, 0x34, 0xFC, 0x74, 0xFC, 0x7C, 0x4F, 0x7F, 0xE2, 0xFE, 0xDC, 0x08, 0xB5, 0x43, 
  0xFC, 0x57, 0x20, 0x45, 0xAA, 0x1F, 0xE3, 0xEF, 0x81, 0x16, 0x90, 0xFC, 0x7F, 0xE1, 0xF8, 
};

TYPE_GrData _Selection_Blue_small_Gd =
{
  1,                              //version
  0,                              //reserved
  OSD_8888,                       //data format
  COMPRESS_Tfp,                   //compressed method
  _Selection_Blue_small_Cpm,
  14976,                           //size
   144,                           //width
    26                            //height
};
