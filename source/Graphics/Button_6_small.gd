#include "tap.h"

static byte _Button_6_small_Cpm[] =
{
  0x00, 0x08, 0x1B, 0x90, 0xFF, 0xFF, 0x00, 0x01, 0x00, 0x01, 
  0x01, 0xA9, 0xF5, 0xAC, 0xFF, 0xFF, 0x05, 0xA0, 
  0x01, 0x6F, 0x5A, 0x76, 0xEE, 0x18, 0xCA, 0x5F, 0x78, 0x12, 0x31, 0x08, 0xC0, 0x00, 0x02, 0x92, 
  0x9E, 0x00, 0x30, 0xA9, 0x22, 0x88, 0x62, 0x14, 0x41, 0x09, 0x46, 0x00, 0x52, 0x4A, 0x05, 0x29, 
  0x30, 0xA5, 0x04, 0x02, 0x9E, 0x15, 0x7E, 0xEF, 0xFF, 0xEB, 0xB0, 0xB0, 0x29, 0xF5, 0x7C, 0xAE, 
  0x6B, 0xEB, 0x7F, 0x60, 0xEC, 0x1D, 0x83, 0xBB, 0x28, 0x40, 0x36, 0xDB, 0x97, 0xDB, 0xDF, 0xD7, 
  0x2E, 0x5C, 0x1F, 0x84, 0xA1, 0x75, 0xAB, 0x56, 0x1F, 0xC6, 0x8D, 0x1A, 0x3F, 0xE6, 0x54, 0xA9, 
  0x5E, 0x8C, 0xB9, 0x72, 0xC4, 0xE9, 0xD3, 0x85, 0x2A, 0x54, 0x85, 0x0A, 0x14, 0x04, 0xC9, 0x93, 
  0x3D, 0x37, 0xC6, 0x8D, 0x1B, 0xC9, 0x93, 0x26, 0x48, 0xB5, 0x6A, 0xD0, 0xC9, 0x93, 0x20, 0xCD, 
  0x9B, 0x30, 0xCB, 0x97, 0x28, 0xBB, 0x76, 0xE9, 0xA7, 0x45, 0xCE, 0x0C, 0x18, 0x28, 0x18, 0xB1, 
  0x62, 0x8B, 0xD7, 0xAF, 0x0D, 0xFB, 0xF7, 0x8D, 0x5A, 0xB5, 0x0C, 0x38, 0x70, 0x8C, 0x78, 0xF1, 
  0x99, 0x28, 0x7A, 0x2E, 0xEF, 0x9F, 0x3E, 0x40, 0x95, 0xB7, 0xAF, 0x5E, 0xB3, 0x85, 0x95, 0x8F, 
  0x0A, 0x14, 0x21, 0x0E, 0x1C, 0x31, 0x12, 0x24, 0x40, 0xFD, 0xFB, 0xFF, 0x2E, 0xEF, 0x1E, 0x3C, 
  0x0A, 0x95, 0x2A, 0xEC, 0x55, 0xAB, 0x56, 0x2A, 0xD5, 0xAA, 0x39, 0xF3, 0xE6, 0x2D, 0xDB, 0xB6, 
  0x79, 0x0E, 0xA5, 0x4A, 0x82, 0x44, 0x89, 0x01, 0xD3, 0xA7, 0x5D, 0xDB, 0xFE, 0x0B, 0xB7, 0x6E, 
  0xC3, 0x87, 0x0E, 0x02, 0x44, 0x89, 0x3B, 0x14, 0xE9, 0xD3, 0x8B, 0x16, 0x2C, 0x0E, 0xBD, 0x7A, 
  0x8E, 0x3C, 0x78, 0x8E, 0x1C, 0x38, 0x0E, 0x5C, 0xB9, 0x0D, 0xDB, 0xB7, 0x1E, 0x3F, 0x6C, 0xD9, 
  0xB7, 0x77, 0x32, 0x24, 0x9D, 0xBB, 0x76, 0xFD, 0xD3, 0xA8, 0x50, 0xA0, 0x59, 0xB3, 0x64, 0xE1, 
  0xFB, 0x36, 0x6C, 0x0E, 0x5C, 0xB9, 0x0C, 0x99, 0x32, 0x19, 0xF3, 0xE7, 0x3C, 0x9E, 0x57, 0xFF, 
  0x88, 0x7B, 0x56, 0xAD, 0x50, 0x95, 0xA6, 0x4C, 0x98, 0xED, 0xFE, 0x9D, 0x3A, 0x0C, 0x18, 0x30, 
  0x1C, 0x7C, 0xAB, 0x3A, 0xF5, 0xEB, 0x9A, 0x3A, 0x74, 0xE9, 0xA1, 0x0F, 0x66, 0xCD, 0x99, 0xC4, 
  0xFC, 0x2B, 0x2C, 0xE7, 0xA7, 0x4E, 0x91, 0xA3, 0x46, 0x80, 0xF5, 0xEB, 0xD3, 0xCC, 0xE5, 0xC3, 
  0x7D, 0xBB, 0x76, 0x88, 0xF1, 0xE3, 0xA0, 0xE6, 0x56, 0x1B, 0x46, 0x8D, 0x10, 0x56, 0xB5, 0x6A, 
  0xD1, 0x3E, 0x7C, 0xF3, 0xBB, 0xF1, 0x62, 0xC4, 0x2F, 0xDF, 0xBE, 0x78, 0xFC, 0xAA, 0x32, 0xBF, 
  0xD1, 0x87, 0x95, 0x47, 0xE1, 0x59, 0x16, 0x2C, 0x58, 0x88, 0x10, 0x20, 0x1F, 0x44, 0xE6, 0xCD, 
  0x9A, 0x7C, 0x2F, 0x2D, 0xBE, 0x95, 0x2A, 0x5F, 0x2D, 0xE4, 0x51, 0x87, 0x91, 0x27, 0xB2, 0xC7, 
  0xEA, 0x54, 0xA9, 0x3C, 0xAF, 0xF2, 0xD6, 0xBD, 0x7A, 0xF0, 0xC1, 0x83, 0x0F, 0x4C, 0x7F, 0x45, 
  0x72, 0xE5, 0xC7, 0x89, 0xFF, 0x61, 0xEB, 0x16, 0x2C, 0x3F, 0xA0, 0x65, 0xD0, 0xFF, 0xB2, 0x54, 
  0xB9, 0x42, 0xC0, 
};

static TYPE_GrData _Button_6_small_Gd =
{
  1,                              //version
  0,                              //reserved
  OSD_8888,                       //data format
  COMPRESS_Tfp,                   //compressed method
  _Button_6_small_Cpm,      
  1440,                           //size
    20,                           //width
    18                            //height
};

