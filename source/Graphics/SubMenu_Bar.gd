#include "tap.h"

static byte _SubMenu_Bar_Cpm[] =
{
  0x00, 0x08, 0x1B, 0x90, 0xFF, 0xFF, 0x00, 0x01, 0x00, 0x01, 
  0x00, 0xAE, 0x74, 0x8D, 0xFF, 0xFF, 0x6B, 0x40, 
  0x00, 0xC9, 0x5A, 0x78, 0xE1, 0xC3, 0x68, 0x7F, 0xC9, 0x30, 0x19, 0x1D, 0xF1, 0x3B, 0xAE, 0xE1, 
  0x31, 0xFC, 0x62, 0xEB, 0xAB, 0xAB, 0xBB, 0xB0, 0xF5, 0x76, 0x3E, 0xC6, 0xC4, 0x64, 0x65, 0x53, 
  0x4B, 0x53, 0x11, 0x9D, 0x95, 0x4C, 0x49, 0x89, 0x93, 0xA9, 0x14, 0x65, 0x64, 0x62, 0x29, 0x89, 
  0x81, 0x4E, 0x36, 0x3C, 0x02, 0xBF, 0xFB, 0xE3, 0x37, 0xF0, 0x17, 0x2F, 0xBF, 0x1F, 0xF2, 0xAF, 
  0xFF, 0x93, 0x3F, 0x02, 0xAC, 0xF1, 0x7E, 0xE5, 0x55, 0x9E, 0x1F, 0xCC, 0x6A, 0xB3, 0xBF, 0xF7, 
  0x81, 0x56, 0x72, 0xFD, 0x5D, 0x55, 0x9C, 0x9F, 0x35, 0xAA, 0xCE, 0x1F, 0x8D, 0xB5, 0x59, 0xBF, 
  0xED, 0x6D, 0x56, 0x6F, 0x7A, 0xDA, 0x55, 0x9B, 0xBE, 0x96, 0x55, 0x67, 0x7B, 0xBE, 0xA5, 0x59, 
  0xDA, 0xEB, 0xD4, 0x55, 0x9A, 0xB9, 0xF4, 0x15, 0x66, 0xC7, 0x32, 0xAD, 0x35, 0x74, 0x2A, 0xD3, 
  0xAD, 0xD2, 0xAB, 0x4E, 0xBF, 0x52, 0xAD, 0x3B, 0x1D, 0x8A, 0xB4, 0xD9, 0xEE, 0x55, 0xA7, 0x67, 
  0x36, 0x92, 0xAD, 0x3E, 0x0A, 0xB4, 0xDC, 0xF2, 0xD1, 0x55, 0x9D, 0xCF, 0x3D, 0x35, 0x59, 0xDD, 
  0xF1, 0xD5, 0x55, 0x9C, 0x5D, 0xB5, 0xAA, 0xC8, 
};

static TYPE_GrData _SubMenu_Bar_Gd =
{
  1,                              //version
  0,                              //reserved
  OSD_8888,                       //data format
  COMPRESS_Tfp,                   //compressed method
  _SubMenu_Bar_Cpm,         
  27456,                           //size
   264,                           //width
    26                            //height
};

