#include "tap.h"

static byte _Button_Ffwd_Active_Cpm[] =
{
  0x00, 0x08, 0x1B, 0x90, 0xFF, 0xFF, 0x00, 0x01, 0x00, 0x01, 
  0x00, 0xA5, 0xB2, 0xB9, 0xFF, 0xFF, 0x06, 0xC0, 
  0x00, 0x5E, 0x52, 0x56, 0xDA, 0x35, 0xFE, 0xDA, 0x32, 0xF7, 0x19, 0x8E, 0xF0, 0xEE, 0x2A, 0xF0, 
  0x06, 0x6F, 0x2B, 0x96, 0x94, 0xA4, 0x74, 0x13, 0x92, 0x57, 0x04, 0x6F, 0x00, 0x72, 0x7B, 0x19, 
  0x0D, 0x1F, 0xEF, 0xB0, 0x48, 0x7F, 0xC1, 0xCA, 0xFF, 0x12, 0x92, 0x1C, 0x14, 0x14, 0x85, 0x2F, 
  0x23, 0x17, 0x80, 0x35, 0x78, 0x31, 0x78, 0xBF, 0x65, 0x04, 0x07, 0x50, 0x02, 0x41, 0x6F, 0xA2, 
  0xCF, 0x41, 0x43, 0xA2, 0xA2, 0xBD, 0x35, 0x3E, 0x51, 0x16, 0x24, 0xC4, 0xDC, 0x1A, 0x6C, 0x2F, 
  0x6C, 0xDF, 0x5C, 0xDF, 0x18, 0x72, 0xED, 0x45, 0x6B, 0x5F, 0x7D, 0x6D, 0x65, 0xDE, 0x88, 0x3B, 
  0xA1, 0xFA, 0x6B, 0x6B, 0x21, 0x23, 0xBA, 0x0E, 0xE8, 0x7E, 0x5A, 0xCD, 0x8B, 0x25, 0x68, 0x9C, 
  0x18, 0x5F, 0xAC, 0xD5, 0x62, 0xD8, 0x8B, 0xA0, 0xC3, 0xC7, 0x59, 0xB7, 0xF1, 0xCE, 0x8B, 0xBF, 
  0xF7, 0xEB, 0x7B, 0xC3, 0xFF, 0xF1, 0x36, 0x70, 0x7D, 0x75, 0xF3, 0x9F, 0xED, 0xAF, 0xFB, 0xF9, 
  0x1D, 0x01, 0x47, 0x83, 0x51, 0x2B, 0x0B, 0x2D, 0x75, 0xFA, 0x58, 0x42, 0x4C, 0x74, 0xBE, 
};

static TYPE_GrData _Button_Ffwd_Active_Gd =
{
  1,                              //version
  0,                              //reserved
  OSD_8888,                       //data format
  COMPRESS_Tfp,                   //compressed method
  _Button_Ffwd_Active_Cpm,  
  1728,                           //size
    24,                           //width
    18                            //height
};

