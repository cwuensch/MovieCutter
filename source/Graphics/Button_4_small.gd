#include "tap.h"

static byte _Button_4_small_Cpm[] =
{
  0x00, 0x08, 0x1B, 0x90, 0xFF, 0xFF, 0x00, 0x01, 0x00, 0x01, 
  0x01, 0x7E, 0xF1, 0x54, 0xFF, 0xFF, 0x05, 0xA0, 
  0x01, 0x3F, 0x5A, 0x7A, 0xE1, 0xD0, 0xDA, 0x5F, 0xBE, 0x04, 0x93, 0x01, 0xD0, 0x4E, 0xEE, 0x00, 
  0x2E, 0xAE, 0xBA, 0xBB, 0x07, 0x69, 0xEC, 0x0C, 0x89, 0x89, 0x18, 0x54, 0x45, 0x14, 0xC2, 0xC0, 
  0x92, 0x2E, 0xC1, 0xD8, 0xFB, 0xA0, 0x61, 0x18, 0x03, 0x02, 0x84, 0x4F, 0x85, 0x5F, 0xBD, 0xFF, 
  0xFE, 0x05, 0x3D, 0x5E, 0xEA, 0x2B, 0xCA, 0xF7, 0x60, 0x53, 0xE0, 0x3F, 0xB0, 0x7B, 0x0A, 0xBD, 
  0xEC, 0xC1, 0x0B, 0x1A, 0x72, 0x6F, 0x6F, 0x7F, 0x96, 0x59, 0x61, 0xFA, 0x48, 0x97, 0x6E, 0xDD, 
  0xB1, 0xFC, 0x6C, 0xD9, 0xB3, 0xFE, 0x6A, 0x54, 0xA9, 0xE0, 0xD6, 0xAD, 0x58, 0x58, 0xB1, 0x60, 
  0x57, 0xAF, 0x5C, 0xBE, 0x7F, 0x71, 0x74, 0x68, 0xD1, 0xF0, 0x6E, 0xDD, 0xBA, 0x30, 0x60, 0xC0, 
  0x18, 0x61, 0x81, 0x6A, 0xD5, 0xA6, 0xE2, 0xE7, 0x4E, 0x9C, 0xD9, 0xAB, 0x56, 0xA8, 0xD7, 0xAF, 
  0x58, 0xE5, 0xCB, 0x90, 0xDD, 0xBB, 0x70, 0xBF, 0x7E, 0xFB, 0x71, 0xF3, 0x26, 0x4C, 0xEE, 0x66, 
  0xCD, 0x9A, 0x30, 0xE1, 0xC2, 0x34, 0x68, 0xD0, 0x36, 0x6C, 0xD8, 0x36, 0xED, 0xDA, 0x2E, 0x5C, 
  0xB8, 0xDC, 0x9A, 0x49, 0x24, 0x1F, 0x3E, 0x7D, 0xD8, 0xD3, 0xA7, 0x4C, 0xB4, 0xDA, 0xAA, 0xAA, 
  0x33, 0xE7, 0xCE, 0x55, 0x3A, 0x54, 0xA9, 0x77, 0x8F, 0x45, 0x14, 0x44, 0xB9, 0x72, 0xC3, 0x87, 
  0x0E, 0x1A, 0x2A, 0x28, 0xA0, 0xC9, 0x93, 0x21, 0x75, 0xFC, 0xC3, 0x0C, 0x19, 0x72, 0xE5, 0x2A, 
  0x7D, 0x0A, 0x14, 0x3B, 0x47, 0x90, 0xC9, 0x93, 0x24, 0x4A, 0x95, 0x29, 0xAC, 0x78, 0x30, 0x60, 
  0x8B, 0xD7, 0xAF, 0x0C, 0x58, 0xB1, 0x04, 0x10, 0x40, 0x3F, 0x7E, 0xFF, 0xE2, 0x9F, 0x78, 0xF1, 
  0xE0, 0x72, 0xE5, 0xCB, 0x52, 0xF2, 0x24, 0x48, 0x2A, 0x9F, 0xCE, 0x39, 0x34, 0x0D, 0x96, 0x59, 
  0x1A, 0xB5, 0x6A, 0x18, 0xF1, 0xE3, 0x2A, 0xBE, 0xFD, 0xFB, 0xC7, 0x1E, 0x3C, 0x46, 0x9D, 0x3A, 
  0x4B, 0x59, 0xBB, 0x76, 0xED, 0xA9, 0x78, 0xD1, 0xA3, 0x15, 0x4F, 0xAB, 0x39, 0x38, 0xDE, 0xBA, 
  0xEB, 0xF3, 0x24, 0xDD, 0xBE, 0x1C, 0x38, 0x0C, 0xD9, 0xB3, 0x16, 0xA3, 0xEA, 0x9F, 0x16, 0x2C, 
  0x51, 0x1E, 0x3C, 0x76, 0xB1, 0xDD, 0x3A, 0x74, 0x1E, 0xBD, 0x7A, 0x20, 0x40, 0x81, 0xCC, 0xF4, 
  0xBB, 0xC9, 0xA9, 0xFD, 0x6E, 0xF2, 0x27, 0x92, 0xA3, 0xFA, 0xD5, 0xFE, 0x84, 0x9C, 0x5B, 0x9F, 
  0x3E, 0x78, 0x4D, 0x34, 0xFB, 0xC1, 0xC4, 0x89, 0x10, 0xA7, 0xCA, 0x7F, 0x93, 0x0A, 0x14, 0x21, 
  0x0E, 0x1C, 0x3F, 0x49, 0xE7, 0x9E, 0x78, 0x38, 0xE3, 0xBD, 0x27, 0x9A, 0x69, 0xA5, 0xFD, 0x03, 
  0x27, 0x17, 0xFF, 0x09, 0x29, 0x72, 0x25, 0x80, 
};

static TYPE_GrData _Button_4_small_Gd =
{
  1,                              //version
  0,                              //reserved
  OSD_8888,                       //data format
  COMPRESS_Tfp,                   //compressed method
  _Button_4_small_Cpm,      
  1440,                           //size
    20,                           //width
    18                            //height
};

