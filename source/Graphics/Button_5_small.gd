#include "tap.h"

static byte _Button_5_small_Cpm[] =
{
  0x00, 0x08, 0x1B, 0x90, 0xFF, 0xFF, 0x00, 0x01, 0x00, 0x01, 
  0x01, 0x96, 0xC6, 0x98, 0xFF, 0xFF, 0x05, 0xA0, 
  0x01, 0x56, 0x62, 0x7A, 0xED, 0xD4, 0xDD, 0x4B, 0xFB, 0xC0, 0x48, 0xC0, 0x61, 0x30, 0x01, 0xC0, 
  0x24, 0x4D, 0x8C, 0x09, 0x39, 0x71, 0x84, 0x61, 0x04, 0xA4, 0x14, 0x13, 0x03, 0x09, 0x89, 0x25, 
  0x29, 0x81, 0x13, 0x28, 0x10, 0x60, 0x13, 0xC1, 0x67, 0xEE, 0xFB, 0xED, 0x73, 0x0B, 0x02, 0xB9, 
  0xFE, 0x9C, 0xC9, 0xDC, 0x7D, 0xF0, 0x07, 0xDC, 0x1D, 0xC3, 0xDA, 0xEF, 0xD9, 0xC1, 0x5E, 0x6D, 
  0xC6, 0xF7, 0xFB, 0xE3, 0xF6, 0x3C, 0x78, 0xC7, 0xE1, 0x28, 0x5E, 0x0C, 0x18, 0x07, 0xF1, 0xBD, 
  0x7A, 0xF7, 0xFC, 0xD7, 0xAF, 0x5D, 0x06, 0xCD, 0x9B, 0x22, 0xDD, 0xBB, 0x62, 0xE5, 0xCB, 0x9E, 
  0x4D, 0xAB, 0x56, 0x97, 0x73, 0x23, 0x4A, 0x95, 0x24, 0x1B, 0xB7, 0x6E, 0x8C, 0xD9, 0xB3, 0x0D, 
  0x1A, 0x34, 0x79, 0x39, 0x32, 0x64, 0x14, 0xE9, 0xD3, 0x5D, 0xDE, 0x6C, 0xD9, 0xA8, 0x37, 0xEF, 
  0xDF, 0x1C, 0xF9, 0xF3, 0x1D, 0x7A, 0xF5, 0x1C, 0x38, 0x70, 0x1B, 0xF7, 0xEF, 0x33, 0x9F, 0x3E, 
  0x71, 0x3A, 0x74, 0xE5, 0xDD, 0xE5, 0x4A, 0x94, 0x82, 0x5D, 0x6F, 0x8F, 0x1E, 0x23, 0x56, 0xAD, 
  0x42, 0xAD, 0x5A, 0xA2, 0xA5, 0x4A, 0x86, 0x67, 0xCF, 0x9E, 0x13, 0x4D, 0x35, 0xDD, 0xE3, 0x46, 
  0x8C, 0x1A, 0xB5, 0x6B, 0xE8, 0xCB, 0x97, 0x2C, 0x6D, 0xDB, 0xB4, 0x6B, 0xD7, 0xAC, 0x65, 0xCB, 
  0x94, 0x62, 0xC5, 0x88, 0x50, 0xA1, 0x40, 0x3E, 0x7C, 0xF9, 0x0D, 0xE4, 0x62, 0xC5, 0x8A, 0x21, 
  0x42, 0x84, 0x18, 0x30, 0x61, 0xE8, 0xC8, 0x91, 0x20, 0xE1, 0x7D, 0xBB, 0x76, 0x33, 0xDF, 0xBF, 
  0x71, 0xD3, 0xA7, 0x41, 0x87, 0x0E, 0x10, 0xD9, 0xB3, 0x64, 0x39, 0xC0, 0x81, 0x00, 0x41, 0x83, 
  0x05, 0x0A, 0xDE, 0x3C, 0x78, 0x72, 0xB6, 0x6C, 0xD9, 0x86, 0x4C, 0x99, 0x1D, 0x1B, 0xF1, 0xE3, 
  0xC0, 0xDD, 0xBB, 0x71, 0xF3, 0xB5, 0x9C, 0xDF, 0xBF, 0x7E, 0x71, 0x35, 0x95, 0xAB, 0x07, 0x57, 
  0xFF, 0x6E, 0xDD, 0xB9, 0xD9, 0xFE, 0x5C, 0xB9, 0x0A, 0x34, 0x68, 0xA1, 0xCD, 0xEB, 0xD7, 0xA7, 
  0x13, 0x59, 0x5C, 0x78, 0xF1, 0xCC, 0xC4, 0x89, 0x10, 0x43, 0x87, 0x0C, 0x4C, 0x99, 0x30, 0xF8, 
  0xFE, 0xCD, 0x9B, 0x04, 0x99, 0x32, 0x50, 0xE6, 0x92, 0x49, 0x1D, 0x5F, 0x59, 0x5D, 0x6A, 0xD5, 
  0x8D, 0xEF, 0x4E, 0x9D, 0x27, 0x84, 0x65, 0x50, 0xCB, 0x0D, 0xCB, 0x38, 0xCB, 0x95, 0xFB, 0x87, 
  0x97, 0x23, 0x59, 0x59, 0x5A, 0xB2, 0xC6, 0x7B, 0x16, 0x2C, 0x1E, 0x6D, 0x97, 0xB5, 0x14, 0x50, 
  0x3A, 0x74, 0xE9, 0x0D, 0xEA, 0xD7, 0x32, 0x3E, 0xD7, 0xB1, 0x62, 0xC4, 0x34, 0x68, 0xD0, 0xF0, 
  0x2C, 0xBD, 0xFB, 0x67, 0x54, 0x94, 0xAD, 0xDB, 0xB7, 0x7F, 0x2C, 0xFF, 0x45, 0xCB, 0x97, 0x21, 
  0xC3, 0x87, 0x1F, 0xB0, 0xF2, 0xAA, 0xE5, 0xF2, 0x06, 0x59, 0x3F, 0xF6, 0x4B, 0x8B, 0x94, 0x2C, 
};

static TYPE_GrData _Button_5_small_Gd =
{
  1,                              //version
  0,                              //reserved
  OSD_8888,                       //data format
  COMPRESS_Tfp,                   //compressed method
  _Button_5_small_Cpm,      
  1440,                           //size
    20,                           //width
    18                            //height
};

