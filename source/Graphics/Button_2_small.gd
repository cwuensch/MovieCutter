#include "tap.h"

static byte _Button_2_small_Cpm[] =
{
  0x00, 0x08, 0x1B, 0x90, 0xFF, 0xFF, 0x00, 0x01, 0x00, 0x01, 
  0x01, 0x86, 0xFB, 0x3F, 0xFF, 0xFF, 0x05, 0xA0, 
  0x01, 0x46, 0x62, 0x7A, 0xEE, 0xEC, 0x67, 0x52, 0xFF, 0xF8, 0x09, 0x51, 0x11, 0x08, 0x7A, 0x00, 
  0x61, 0x4C, 0x1E, 0xD7, 0xB5, 0xED, 0xED, 0x30, 0x30, 0xB1, 0x30, 0x25, 0x28, 0x94, 0x24, 0x24, 
  0x92, 0x30, 0x31, 0xA6, 0x14, 0x60, 0x49, 0x06, 0x45, 0x26, 0x04, 0x28, 0xF0, 0x55, 0xF3, 0xFF, 
  0xDF, 0xB8, 0x3B, 0x07, 0x60, 0xEA, 0x7E, 0xC1, 0xFB, 0x02, 0xBB, 0x27, 0x78, 0x03, 0xEC, 0x1D, 
  0x85, 0x5D, 0xD9, 0xC1, 0x5E, 0x8D, 0x47, 0x37, 0xFB, 0xFF, 0xFB, 0x26, 0x4C, 0x83, 0xF0, 0x92, 
  0x5E, 0x2C, 0x58, 0x87, 0xE9, 0xBD, 0x7A, 0xF7, 0xF1, 0xB1, 0x62, 0xC2, 0x4D, 0xBB, 0x76, 0xCE, 
  0x0E, 0xFD, 0xFB, 0xE7, 0xEE, 0xCD, 0x9B, 0x3E, 0x63, 0xC8, 0xD3, 0xA7, 0x4D, 0x07, 0x06, 0x0C, 
  0x03, 0x3E, 0x7C, 0xE3, 0x5E, 0xBD, 0x63, 0x76, 0xED, 0xC7, 0xEF, 0x2E, 0x5C, 0xA2, 0xE5, 0xCB, 
  0x89, 0x93, 0x23, 0x3E, 0x7C, 0xF4, 0x12, 0x9B, 0xE1, 0xC3, 0x81, 0xA3, 0xEA, 0xD5, 0xA8, 0x74, 
  0xE9, 0xD0, 0x75, 0xEB, 0xD4, 0x68, 0xD1, 0xA0, 0x52, 0xA5, 0x49, 0x32, 0xF3, 0x66, 0xCD, 0x41, 
  0xAD, 0x5A, 0xB0, 0xAF, 0x5E, 0xB8, 0xA3, 0x46, 0x89, 0x9C, 0x78, 0xF1, 0x8E, 0xFD, 0xFB, 0x8D, 
  0xFB, 0xF7, 0x8A, 0xB5, 0x6A, 0xA6, 0x5E, 0x54, 0xA9, 0x41, 0xCB, 0x97, 0x3F, 0x05, 0xDB, 0xB7, 
  0x61, 0xD3, 0xA7, 0x5E, 0xDF, 0xFC, 0xE9, 0xD3, 0x8D, 0x3F, 0x6E, 0xDD, 0xA1, 0x65, 0x96, 0x44, 
  0x9C, 0x99, 0x32, 0x44, 0x89, 0x12, 0x03, 0x26, 0x4C, 0xBC, 0x97, 0x0E, 0x1C, 0x0D, 0x3A, 0x74, 
  0x8E, 0x7C, 0xF9, 0x9D, 0x9B, 0x68, 0xD1, 0xA2, 0x21, 0xE3, 0x46, 0x8C, 0x23, 0xC7, 0x8E, 0x94, 
  0x76, 0x6C, 0xD9, 0x9B, 0x79, 0x53, 0xFE, 0x5C, 0xB9, 0x0C, 0x38, 0x70, 0x86, 0xED, 0xDB, 0xA6, 
  0x3E, 0x2C, 0x58, 0xA6, 0xA7, 0xE3, 0x2D, 0xAB, 0x56, 0xA1, 0x75, 0xD7, 0x1B, 0x36, 0x6C, 0x1C, 
  0x78, 0xF1, 0x16, 0xAD, 0x5A, 0x0D, 0x9B, 0x36, 0xF3, 0xB1, 0xE1, 0xC3, 0x86, 0x6A, 0x7E, 0x32, 
  0xE5, 0xCB, 0x96, 0x74, 0xAF, 0xB7, 0x6E, 0xC3, 0x36, 0x6C, 0xC1, 0x86, 0x18, 0x35, 0xF5, 0x55, 
  0x54, 0x3C, 0x78, 0xF1, 0x12, 0x70, 0xA1, 0x42, 0x11, 0x22, 0x44, 0x46, 0x5D, 0xDB, 0xB7, 0x4F, 
  0x84, 0x65, 0x60, 0xCB, 0xD9, 0x53, 0xB2, 0x25, 0xC8, 0xB9, 0x93, 0x26, 0x25, 0x3C, 0xB5, 0x1F, 
  0x8C, 0xB2, 0xD1, 0xE5, 0xC9, 0xFA, 0x95, 0x2A, 0x7B, 0x9A, 0x14, 0x28, 0x26, 0xE1, 0xC1, 0x83, 
  0x04, 0xCF, 0xCD, 0xDF, 0xFA, 0x2F, 0xDF, 0xBF, 0x10, 0x20, 0x40, 0xF4, 0x9E, 0xA2, 0x8A, 0x07, 
  0xCF, 0x9F, 0x7A, 0x4F, 0x7A, 0xF5, 0xE9, 0xFC, 0x81, 0x97, 0x41, 0xFE, 0xC9, 0x69, 0x72, 0x4B, 
};

static TYPE_GrData _Button_2_small_Gd =
{
  1,                              //version
  0,                              //reserved
  OSD_8888,                       //data format
  COMPRESS_Tfp,                   //compressed method
  _Button_2_small_Cpm,      
  1440,                           //size
    20,                           //width
    18                            //height
};

