#include "tap.h"

static byte _Button_Play_Inactive_Cpm[] =
{
  0x00, 0x08, 0x1B, 0x90, 0xFF, 0xFF, 0x00, 0x01, 0x00, 0x01, 
  0x00, 0xCA, 0xC8, 0xA0, 0xFF, 0xFF, 0x06, 0xC0, 
  0x00, 0x95, 0x53, 0x56, 0xDA, 0x25, 0x5B, 0xBC, 0x07, 0x79, 0x19, 0x01, 0x80, 0xE9, 0xCA, 0xF4, 
  0xAB, 0x02, 0xDE, 0x01, 0x4E, 0x42, 0x45, 0x45, 0x46, 0xE1, 0x54, 0x47, 0xC0, 0x99, 0xF0, 0xE7, 
  0xF7, 0xED, 0xA4, 0x04, 0x04, 0xCB, 0xB6, 0xE7, 0x65, 0xDA, 0x80, 0x7C, 0x07, 0x80, 0xB6, 0xA0, 
  0x10, 0x18, 0x81, 0x40, 0x19, 0xE1, 0x77, 0x59, 0x65, 0x85, 0xFA, 0x83, 0x75, 0xD7, 0x59, 0x55, 
  0x55, 0x5F, 0xE8, 0x36, 0x94, 0xA7, 0x7B, 0x4D, 0x34, 0xEF, 0x68, 0xA2, 0x8C, 0xAD, 0xB6, 0xF7, 
  0xD3, 0x2F, 0x0B, 0x3C, 0xF3, 0xE5, 0x41, 0xF1, 0x73, 0xCF, 0xBE, 0xF6, 0x96, 0x52, 0x9A, 0x69, 
  0x95, 0x6F, 0x3E, 0x2A, 0xF3, 0xC5, 0x06, 0xFA, 0xC9, 0x21, 0x34, 0xD3, 0x4A, 0xB8, 0x1F, 0x15, 
  0x7A, 0x60, 0xDF, 0x48, 0xE3, 0x26, 0x18, 0x61, 0x57, 0x13, 0xE2, 0xF9, 0xD3, 0x06, 0xE3, 0x8A, 
  0x22, 0x65, 0x96, 0x55, 0x71, 0x36, 0xBD, 0x30, 0x6E, 0x28, 0x61, 0xD4, 0xFA, 0xF3, 0xC5, 0x06, 
  0xE0, 0x82, 0x0F, 0x67, 0xF3, 0x36, 0x28, 0x37, 0xCD, 0x08, 0xD0, 0xFF, 0xA3, 0x62, 0x83, 0x68, 
  0x7D, 0xFD, 0x75, 0xF3, 0xA0, 0xDB, 0xEF, 0x3D, 0xB1, 0xF0, 0x6D, 0xE7, 0x5D, 0xD8, 0xF8, 0x37, 
  0xC8, 0xE7, 0x23, 0x18, 0xDF, 0x4D, 0x82, 0x60, 0xDB, 0x8E, 0x38, 0x47, 0xBB, 0xFE, 0x2D, 0xB7, 
  0xF2, 0x9F, 0xC1, 0xB0, 
};

static TYPE_GrData _Button_Play_Inactive_Gd =
{
  1,                              //version
  0,                              //reserved
  OSD_8888,                       //data format
  COMPRESS_Tfp,                   //compressed method
  _Button_Play_Inactive_Cpm,
  1728,                           //size
    24,                           //width
    18                            //height
};

