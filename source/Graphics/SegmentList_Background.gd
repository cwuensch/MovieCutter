#include "tap.h"

static byte _SegmentList_Background_Cpm[] =
{
  0x00, 0x08, 0x19, 0x10, 0xFF, 0xFF, 0x00, 0x01, 0x00, 0x07, 
  0x00, 0xA2, 0x47, 0x5B, 0xFF, 0xFF, 0x7F, 0xF8, 
  0x00, 0x94, 0x4B, 0x68, 0x60, 0x36, 0xFF, 0xC0, 0x12, 0xB2, 0xA1, 0x26, 0xA5, 0x43, 0x5F, 0x6D, 
  0x5E, 0xE6, 0x7C, 0x05, 0xF0, 0x39, 0xC0, 0x1E, 0x4B, 0x00, 0xBC, 0x0E, 0xE6, 0x00, 0x40, 0x80, 
  0x08, 0x9D, 0xDD, 0xF8, 0x49, 0x27, 0xE6, 0x22, 0x09, 0xDA, 0x93, 0xE5, 0x27, 0xC9, 0x49, 0xF2, 
  0x93, 0xE4, 0xA4, 0xF9, 0x3E, 0xC9, 0xFA, 0xAA, 0x49, 0xF2, 0x93, 0xE4, 0xA4, 0xF9, 0x49, 0xF2, 
  0x52, 0x7C, 0x94, 0x9F, 0x29, 0x3E, 0x4A, 0x4F, 0x94, 0x9F, 0x49, 0xF2, 0x93, 0xE4, 0xA4, 0xF9, 
  0x49, 0xF2, 0x52, 0x7C, 0x9D, 0x49, 0xFC, 0x5E, 0x1E, 0x59, 0x3F, 0x25, 0x27, 0xCA, 0x4F, 0x92, 
  0x93, 0xE4, 0xA4, 0xF9, 0x49, 0xF2, 0x52, 0x7C, 0xA4, 0xFE, 0x75, 0xCC, 0xC9, 0x3F, 0xA9, 0x3E, 
  0x52, 0x7C, 0x94, 0x9F, 0x29, 0x3E, 0x93, 0xE5, 0x27, 0xC9, 0x49, 0xF2, 0x93, 0xE4, 0xA4, 0xF9, 
  0x29, 0x3E, 0x7B, 0x27, 0xF1, 0x49, 0xF2, 0x93, 0xE4, 0xB7, 0xBE, 0x52, 0x7C, 0x94, 0x9F, 0x25, 
  0x27, 0xCA, 0x4F, 0x92, 0x93, 0xE5, 0x27, 0xD2, 0x7C, 0xA4, 0xF9, 0xE2, 0x00, 0xBB, 0x04, 0x25, 
  0xFF, 0xFF, 0x7F, 0xF8, 0x00, 0x9B, 0x4B, 0x48, 0xA0, 0x58, 0xBF, 0xC8, 0x13, 0x82, 0xFA, 0x49, 
  0xFE, 0xDC, 0x9A, 0xED, 0x40, 0x5B, 0x7D, 0x03, 0xC2, 0x48, 0xFF, 0x80, 0x80, 0x80, 0x80, 0x8C, 
  0x90, 0x1C, 0x04, 0x83, 0xE4, 0x0F, 0xED, 0x00, 0xD1, 0x00, 0xA1, 0x49, 0x11, 0x19, 0x74, 0xDA, 
  0xD6, 0xD5, 0xBD, 0xEF, 0x94, 0xCC, 0xE9, 0xEF, 0x36, 0xB2, 0xFB, 0xFB, 0xB3, 0xFA, 0x93, 0xE5, 
  0x27, 0xC9, 0x49, 0xF2, 0x93, 0xE9, 0x3E, 0x52, 0x7C, 0x9D, 0xE4, 0xFD, 0x35, 0x96, 0x4E, 0x14, 
  0x9F, 0x3E, 0x49, 0xF6, 0xFB, 0xE5, 0xC9, 0xFC, 0x94, 0x9F, 0x29, 0x3E, 0x4A, 0x4F, 0x92, 0x93, 
  0xE5, 0x27, 0xC9, 0x49, 0xF2, 0x93, 0xE9, 0x3E, 0x52, 0x7C, 0x94, 0x9F, 0x29, 0x3E, 0x4A, 0x4F, 
  0x92, 0x93, 0xE7, 0xE9, 0x3F, 0x4A, 0x4F, 0x94, 0x9F, 0x25, 0xBE, 0xF9, 0x49, 0xF2, 0x52, 0x7C, 
  0x94, 0x9F, 0x29, 0x3E, 0x4A, 0x4F, 0x94, 0x9F, 0x49, 0xF2, 0x93, 0xE4, 0xA4, 0xF9, 0x49, 0xF2, 
  0x79, 0x93, 0xE4, 0xE1, 0x49, 0xF3, 0xEC, 0x9F, 0x6C, 0xCE, 0x5C, 0x9F, 0xC9, 0xDA, 0x4F, 0xD2, 
  0x93, 0xE5, 0x27, 0xC9, 0x6F, 0xBE, 0x52, 0x7C, 0x94, 0x9F, 0x25, 0x27, 0xCA, 0x4F, 0x92, 0x93, 
  0xE5, 0x27, 0xD2, 0x7C, 0xA4, 0xF9, 0x29, 0x3F, 0xC2, 0x00, 0xB6, 0x0B, 0x28, 0xFF, 0xFF, 0x7F, 
  0xF8, 0x00, 0x9A, 0x4B, 0x68, 0x80, 0x46, 0xBF, 0xC0, 0x13, 0xE9, 0x78, 0x49, 0xD2, 0x60, 0xD7, 
  0xA2, 0xC8, 0xB2, 0x78, 0x17, 0x41, 0xBE, 0xA8, 0x80, 0x76, 0xC6, 0xA9, 0x81, 0x1B, 0x63, 0xB4, 
  0x03, 0x84, 0x00, 0x64, 0xEC, 0xCC, 0xE0, 0xF8, 0xC4, 0x47, 0x6D, 0xEF, 0x7C, 0x5A, 0xD6, 0xE7, 
  0xEF, 0x86, 0xBA, 0xDF, 0x93, 0x94, 0x9F, 0x25, 0x27, 0xCA, 0x4F, 0x92, 0x93, 0xE5, 0x27, 0xFA, 
  0x93, 0xF4, 0xA4, 0xF9, 0x49, 0xF2, 0x5B, 0xDF, 0x29, 0x3E, 0x4A, 0x4F, 0x92, 0x93, 0xE5, 0x27, 
  0xC9, 0x49, 0xF2, 0x93, 0xF5, 0xD6, 0x4E, 0x54, 0x9F, 0x3B, 0x93, 0xED, 0x33, 0x97, 0x93, 0xF9, 
  0x29, 0x3E, 0x52, 0x7C, 0x94, 0x9F, 0x29, 0x3E, 0x93, 0xE7, 0xE9, 0x3F, 0x4A, 0x4F, 0x94, 0x9F, 
  0x25, 0xBD, 0xF2, 0x93, 0xE4, 0xA4, 0xF9, 0x29, 0x3E, 0x52, 0x7C, 0x94, 0x9F, 0x29, 0x3E, 0x93, 
  0xE5, 0x27, 0xC9, 0x49, 0xF2, 0x93, 0xE4, 0xA4, 0xF9, 0x29, 0x3E, 0x52, 0x7C, 0x94, 0x9F, 0x3F, 
  0xC9, 0xFA, 0x52, 0x7C, 0xA4, 0xF9, 0x2D, 0xEF, 0x94, 0x9F, 0x27, 0xD9, 0x3E, 0x4E, 0x54, 0x9F, 
  0x3B, 0x93, 0xED, 0x33, 0x97, 0x93, 0xF9, 0x29, 0x3E, 0x52, 0x7C, 0x94, 0x9F, 0x29, 0x3F, 0xA9, 
  0x3E, 0x00, 0xAF, 0xEE, 0x6F, 0xFF, 0xFF, 0x7F, 0xF8, 0x00, 0x98, 0x4B, 0x68, 0x80, 0x38, 0xBF, 
  0xC0, 0x13, 0x52, 0xC1, 0x27, 0x49, 0x83, 0x5F, 0xC5, 0xA0, 0xC9, 0x82, 0x3B, 0x71, 0xB0, 0x80, 
  0xB4, 0x03, 0xA0, 0x2D, 0x97, 0xDB, 0x1D, 0xA0, 0x1A, 0x20, 0x02, 0xA9, 0x26, 0x67, 0x47, 0xE6, 
  0x22, 0x38, 0x6E, 0xEF, 0x55, 0x55, 0xBF, 0xBD, 0x9C, 0xCB, 0x7C, 0x9C, 0xA4, 0xF9, 0x29, 0x3E, 
  0x52, 0x7C, 0x9E, 0x64, 0xFE, 0x29, 0x3E, 0x52, 0x7C, 0x96, 0xFB, 0xE5, 0x27, 0xC9, 0x49, 0xF2, 
  0x52, 0x7C, 0xA4, 0xF9, 0x29, 0x3E, 0x52, 0x7D, 0x27, 0xCA, 0x4F, 0x92, 0x93, 0xE5, 0x27, 0xC9, 
  0x49, 0xF2, 0x52, 0x7C, 0xA4, 0xF9, 0x29, 0x3E, 0x7F, 0x93, 0xF8, 0xE6, 0x59, 0x3D, 0xA9, 0x3E, 
  0x7C, 0x93, 0xED, 0xF7, 0xCB, 0x93, 0xF9, 0x29, 0x3E, 0x52, 0x7C, 0x94, 0x9F, 0x25, 0x27, 0xCA, 
  0x4F, 0x92, 0x93, 0xE5, 0x27, 0xD2, 0x7C, 0xA4, 0xF9, 0x29, 0x3E, 0x52, 0x7C, 0x94, 0x9F, 0x25, 
  0x27, 0xCF, 0xB2, 0x7F, 0x14, 0x9F, 0x29, 0x3E, 0x4B, 0x7D, 0xF2, 0x93, 0xE4, 0xA4, 0xF9, 0x29, 
  0x3E, 0x52, 0x7C, 0x94, 0x9F, 0x29, 0x3E, 0x93, 0xE5, 0x27, 0xC9, 0x49, 0xF2, 0x93, 0xE4, 0xF5, 
  0x27, 0xC0, 0x00, 0xBD, 0x1B, 0x9F, 0xFF, 0xFF, 0x7F, 0xF8, 0x00, 0x9F, 0x4C, 0x4C, 0xA0, 0x54, 
  0xBF, 0xD8, 0x08, 0x84, 0x82, 0xFF, 0x10, 0xDE, 0xE7, 0x1A, 0xDE, 0x44, 0x41, 0xB0, 0xEA, 0x32, 
  0x06, 0x2E, 0x16, 0x32, 0x1E, 0x12, 0x02, 0x12, 0xC0, 0xFB, 0x02, 0xFD, 0xA0, 0x10, 0x00, 0x02, 
  0x27, 0x2D, 0x6B, 0x5F, 0x0C, 0xA5, 0x22, 0x79, 0x71, 0x24, 0xEB, 0x5A, 0xDD, 0x27, 0xCF, 0xB2, 
  0x7E, 0x73, 0x99, 0x3B, 0xA9, 0x3E, 0x7C, 0x93, 0xF9, 0x52, 0x7C, 0xA4, 0xF9, 0x2D, 0xEF, 0x94, 
  0x9F, 0x27, 0xF9, 0x3F, 0x11, 0x47, 0x3F, 0x96, 0x4F, 0xCA, 0x4F, 0x92, 0x93, 0xE4, 0xA4, 0xF9, 
  0x49, 0xF2, 0x52, 0x7C, 0xA4, 0xFA, 0x4F, 0x94, 0x9F, 0x25, 0x27, 0xCA, 0x4F, 0x92, 0x93, 0xE4, 
  0xE9, 0x27, 0xF2, 0xA4, 0xF9, 0x49, 0xF2, 0x5B, 0xDF, 0x29, 0x3E, 0x4A, 0x4F, 0x92, 0x93, 0xE5, 
  0x27, 0xC9, 0x49, 0xF2, 0x93, 0xF9, 0x93, 0xE4, 0xF0, 0xA4, 0xF9, 0xFA, 0x4F, 0xB4, 0xCE, 0x5E, 
  0x9F, 0xC9, 0x49, 0xF2, 0x93, 0xE4, 0xA4, 0xF9, 0x49, 0xF4, 0x9F, 0x3D, 0xC9, 0xFC, 0xBA, 0xC9, 
  0xF8, 0x8A, 0x39, 0xFC, 0xF5, 0x27, 0xC9, 0x6F, 0x7C, 0xA4, 0xF9, 0x29, 0x3E, 0x4A, 0x4F, 0x94, 
  0x9F, 0x25, 0x27, 0xCA, 0x4F, 0xA4, 0xF9, 0x49, 0xF2, 0x52, 0x7C, 0xA4, 0xF9, 0x29, 0x3E, 0x76, 
  0x40, 0x00, 0xBD, 0xDF, 0xB2, 0xFF, 0xFF, 0x7F, 0xF8, 0x00, 0x9B, 0x4B, 0x48, 0xA0, 0x58, 0xBF, 
  0xC8, 0x13, 0x82, 0xFA, 0x49, 0xFE, 0xDC, 0x9A, 0xED, 0x40, 0x5B, 0x7D, 0x03, 0xC4, 0x48, 0x1E, 
  0x43, 0xFC, 0x24, 0x04, 0x04, 0x64, 0x80, 0xE4, 0x27, 0x19, 0x11, 0xED, 0x00, 0xD1, 0x00, 0xB5, 
  0x4B, 0x11, 0x19, 0x74, 0xDA, 0xD6, 0xD5, 0xBD, 0xEF, 0x94, 0xCC, 0xE9, 0xCF, 0x36, 0xB2, 0xFB, 
  0xEB, 0xB3, 0xF2, 0x93, 0xE5, 0x27, 0xC9, 0xE2, 0x4F, 0xD2, 0x93, 0xE5, 0x27, 0xC9, 0x6F, 0xBE, 
  0x52, 0x7C, 0x9E, 0x64, 0xF9, 0x38, 0x52, 0x7C, 0xFB, 0x27, 0xDB, 0x33, 0x97, 0xD3, 0xF9, 0x29, 
  0x3E, 0x52, 0x7C, 0x94, 0x9F, 0x29, 0x3E, 0x93, 0xE5, 0x27, 0xC9, 0x49, 0xF2, 0x93, 0xEF, 0x13, 
  0x94, 0x9F, 0x27, 0x69, 0x3F, 0x4A, 0x4F, 0x94, 0x9F, 0x25, 0xBE, 0xF9, 0x49, 0xF2, 0x52, 0x7C, 
  0x94, 0x9F, 0x29, 0x3E, 0x4A, 0x4F, 0x94, 0x9F, 0x49, 0xF2, 0x93, 0xE4, 0xA4, 0xF9, 0x49, 0xF2, 
  0x52, 0x7C, 0x94, 0x9F, 0x29, 0x3E, 0x4A, 0x4F, 0x9F, 0xE4, 0xFD, 0x35, 0x96, 0x4E, 0x14, 0x9F, 
  0x3E, 0x49, 0xF6, 0xFB, 0xE5, 0xF4, 0xFE, 0x4A, 0x4F, 0x94, 0x9F, 0x25, 0x27, 0xC9, 0x49, 0xF2, 
  0x93, 0xE4, 0xA4, 0xF9, 0x49, 0xF4, 0x9F, 0x29, 0x3E, 0x4A, 0x4F, 0x94, 0x9F, 0x3B, 0xF9, 0x8C, 
  0x00, 0x8C, 0x8A, 0x35, 0xFF, 0xFF, 0x62, 0x70, 0x00, 0x7A, 0x4A, 0x88, 0x80, 0x36, 0xFF, 0xC8, 
  0x12, 0x71, 0x44, 0x93, 0x02, 0x26, 0xBA, 0x6A, 0x1B, 0x05, 0x40, 0xFB, 0xB2, 0xA3, 0x7B, 0xC1, 
  0xB8, 0x32, 0x5F, 0xFC, 0x00, 0xC1, 0x00, 0x11, 0x49, 0x99, 0xC1, 0xDB, 0x11, 0x1E, 0x37, 0x77, 
  0x8A, 0xAA, 0xCE, 0x7D, 0x3A, 0xDB, 0x71, 0x39, 0x49, 0xF2, 0x7D, 0x93, 0xFA, 0x52, 0x7C, 0xA4, 
  0xF9, 0x2F, 0xF7, 0xCA, 0x4F, 0x92, 0x93, 0xE4, 0xA4, 0xF9, 0x49, 0xF2, 0x52, 0x7C, 0xA4, 0xFA, 
  0x4F, 0x94, 0x9F, 0x25, 0x27, 0xCA, 0x4F, 0x93, 0xFC, 0x9F, 0x27, 0x8A, 0x4F, 0x94, 0x9F, 0x25, 
  0x27, 0xCA, 0x4F, 0x92, 0x93, 0xE4, 0xA4, 0xF9, 0x49, 0xF2, 0x52, 0x7C, 0x9E, 0xE4, 0xBA, 0x4F, 
  0x94, 0x9F, 0x25, 0x27, 0xCA, 0x4F, 0x92, 0x93, 0xE4, 0xA4, 0xF9, 0x49, 0xF2, 0x52, 0x7C, 0xA4, 
  0xFA, 0x4F, 0x94, 0x9F, 0x25, 0x27, 0xCA, 0x4F, 0x93, 0xF4, 0x9F, 0x24, 0x93, 0xB4, 
};

static TYPE_GrData _SegmentList_Background_Gd =
{
  1,                              //version
  0,                              //reserved
  OSD_8888,                       //data format
  COMPRESS_Tfp,                   //compressed method
  _SegmentList_Background_Cpm,
  221760,                           //size
   168,                           //width
   330                            //height
};

