#include "tap.h"

static byte _Button_Rwd_Inactive_Cpm[] =
{
  0x00, 0x08, 0x1B, 0x90, 0xFF, 0xFF, 0x00, 0x01, 0x00, 0x01, 
  0x00, 0xD4, 0x52, 0x96, 0xFF, 0xFF, 0x06, 0xC0, 
  0x00, 0x98, 0x53, 0x52, 0xDA, 0xA5, 0x5B, 0xB8, 0x0E, 0xA8, 0xC8, 0x0C, 0x07, 0xA7, 0x2F, 0x99, 
  0x56, 0x05, 0xB8, 0x05, 0x39, 0x0B, 0xC2, 0xA2, 0xA3, 0x22, 0xA8, 0x8F, 0x02, 0x67, 0x87, 0x3F, 
  0xBF, 0x6D, 0x78, 0x04, 0x06, 0x01, 0x01, 0x02, 0x01, 0xDA, 0x76, 0x40, 0x40, 0x40, 0xAE, 0x01, 
  0xC0, 0x5B, 0x50, 0x08, 0x0C, 0x50, 0xA8, 0x70, 0xEE, 0xAF, 0xD4, 0xD3, 0x49, 0x7E, 0xA0, 0xDD, 
  0x14, 0x50, 0x53, 0xCF, 0x3F, 0xFA, 0x0D, 0xCD, 0x34, 0xDB, 0x59, 0x65, 0x97, 0x6B, 0x24, 0x92, 
  0x63, 0x6A, 0xAB, 0xAF, 0xBD, 0x68, 0x42, 0x30, 0xA3, 0xBE, 0x2E, 0x9A, 0xC5, 0xE9, 0x14, 0x45, 
  0x1C, 0x71, 0xDF, 0x53, 0xDF, 0x17, 0x3D, 0x62, 0x83, 0x7C, 0xE1, 0x84, 0x97, 0x5D, 0x7B, 0xAD, 
  0x9D, 0xF1, 0x72, 0xD6, 0x9A, 0x0D, 0xF2, 0x82, 0x02, 0x51, 0x45, 0x3B, 0xDB, 0x7B, 0xE2, 0xF9, 
  0xD7, 0x65, 0x06, 0xE0, 0x7D, 0xF2, 0x59, 0x65, 0xAE, 0xB6, 0x83, 0xCB, 0x5A, 0x68, 0x36, 0xFB, 
  0xCF, 0x5E, 0x7E, 0xD0, 0xF9, 0xEB, 0x14, 0x1B, 0x75, 0xD7, 0x7C, 0x1F, 0xB0, 0x3E, 0x9A, 0xC1, 
  0xBE, 0x2E, 0x39, 0x81, 0xF4, 0x87, 0xD3, 0x58, 0x36, 0xE1, 0xCF, 0x89, 0xFF, 0x54, 0x1B, 0x3B, 
  0x6D, 0xEA, 0x7C, 0x1B, 0x6D, 0xA6, 0xB5, 0x3E, 0x0D, 0xF0, 0x31, 0x89, 0x55, 0x55, 0xFA, 0x6C, 
  0x13, 0x06, 0xD9, 0x65, 0x92, 0x35, 0x7F, 0xF7, 0xB0, 0xC7, 0xCA, 0x6E, 0xC6, 0xC0, 
};

static TYPE_GrData _Button_Rwd_Inactive_Gd =
{
  1,                              //version
  0,                              //reserved
  OSD_8888,                       //data format
  COMPRESS_Tfp,                   //compressed method
  _Button_Rwd_Inactive_Cpm, 
  1728,                           //size
    24,                           //width
    18                            //height
};

