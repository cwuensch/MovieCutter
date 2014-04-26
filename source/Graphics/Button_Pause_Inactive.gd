#include "tap.h"

byte _Button_Pause_Inactive_Cpm[] =
{
  0x00, 0x08, 0x1B, 0x90, 0xFF, 0xFF, 0x00, 0x01, 0x00, 0x01, 
  0x00, 0xC1, 0x43, 0x68, 0xFF, 0xFF, 0x06, 0xC0, 
  0x00, 0x8F, 0x54, 0x56, 0xD9, 0xA5, 0x5B, 0xBC, 0x07, 0x72, 0x32, 0x03, 0x01, 0xA7, 0x95, 0x65, 
  0x58, 0x16, 0xF0, 0x0A, 0x79, 0x09, 0x15, 0x17, 0x0D, 0x85, 0x51, 0x1F, 0x02, 0x67, 0xC3, 0x9F, 
  0xDF, 0xAE, 0xB1, 0x18, 0x07, 0x00, 0xF7, 0x6B, 0xC0, 0x2B, 0xC0, 0x08, 0x0B, 0xBA, 0x80, 0xC0, 
  0x31, 0x42, 0x81, 0xD3, 0xC3, 0x4F, 0x65, 0x96, 0x45, 0xFA, 0x83, 0x78, 0xE3, 0x89, 0x61, 0x86, 
  0x1F, 0xE8, 0x36, 0x94, 0xA7, 0x7A, 0x84, 0x23, 0x7B, 0x7D, 0xF7, 0xED, 0x73, 0xCF, 0xBF, 0x8A, 
  0xDC, 0x7A, 0xDB, 0xAE, 0xBB, 0x6A, 0xA1, 0x6D, 0xC7, 0xAD, 0xED, 0x65, 0x85, 0x6D, 0xB6, 0xB2, 
  0xA8, 0x5B, 0x71, 0xAE, 0x83, 0x7D, 0x6B, 0xAC, 0xA0, 0x82, 0x06, 0x55, 0x0B, 0x6E, 0x35, 0xD0, 
  0x6F, 0xA5, 0x55, 0x13, 0xCF, 0x3C, 0xCA, 0xA1, 0x6D, 0xC6, 0xBA, 0x0D, 0xD5, 0x4D, 0x24, 0xFB, 
  0xEF, 0xB2, 0xA8, 0x5B, 0x71, 0xAE, 0x83, 0x74, 0xD1, 0x43, 0x8F, 0x83, 0x73, 0xCF, 0x3B, 0x8F, 
  0x83, 0x7C, 0xE6, 0x99, 0xC7, 0xC1, 0xB9, 0xA5, 0x97, 0xD7, 0xCF, 0xEE, 0x83, 0x72, 0xC9, 0x23, 
  0x8F, 0x83, 0x72, 0x47, 0x1B, 0x8F, 0x83, 0x7C, 0x8E, 0x72, 0x31, 0x8D, 0xF4, 0xD8, 0x26, 0x0D, 
  0xC5, 0x14, 0x44, 0x7D, 0x3B, 0xE3, 0x0C, 0x3F, 0x29, 0xFC, 0x9B, 
};

TYPE_GrData _Button_Pause_Inactive_Gd =
{
  1,                              //version
  0,                              //reserved
  OSD_8888,                       //data format
  COMPRESS_Tfp,                   //compressed method
  _Button_Pause_Inactive_Cpm,
  1728,                           //size
    24,                           //width
    18                            //height
};

