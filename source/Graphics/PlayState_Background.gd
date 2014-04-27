#include "tap.h"

byte _PlayState_Background_Cpm[] =
{
  0x00, 0x08, 0x1B, 0x90, 0xFF, 0xFF, 0x00, 0x01, 0x00, 0x01, 
  0x03, 0x74, 0xDF, 0x73, 0xFF, 0xFF, 0x43, 0x90, 
  0x02, 0xF4, 0x6A, 0x96, 0xD8, 0x54, 0xAC, 0xFF, 0x66, 0xD5, 0x64, 0xD8, 0x91, 0x60, 0x54, 0x55, 
  0x05, 0x41, 0x50, 0x55, 0x05, 0x50, 0x54, 0xB3, 0xDE, 0xAB, 0xD5, 0x55, 0x55, 0x55, 0x55, 0x55, 
  0x55, 0x55, 0x5A, 0x76, 0x8A, 0xB0, 0x15, 0x6A, 0x96, 0xA6, 0xAA, 0x33, 0xEE, 0x0F, 0x3F, 0xBF, 
  0xFB, 0xC5, 0xD0, 0xB0, 0x37, 0x80, 0x7D, 0xE0, 0x2E, 0xD0, 0xDC, 0x0F, 0x21, 0xB8, 0x1B, 0x62, 
  0x58, 0x3A, 0x6A, 0x6F, 0xBC, 0x6F, 0x09, 0x5B, 0x87, 0x70, 0x25, 0xFF, 0x00, 0x46, 0xE0, 0x39, 
  0x49, 0x2F, 0x9F, 0x9F, 0x99, 0x79, 0x79, 0x79, 0x17, 0x8F, 0x8F, 0x89, 0x78, 0x78, 0x78, 0x07, 
  0xBF, 0xBF, 0xBC, 0xBB, 0xBB, 0xBB, 0x8A, 0xED, 0xDB, 0xA1, 0xED, 0xED, 0xED, 0x2E, 0xCE, 0xCE, 
  0xC0, 0xF5, 0xF5, 0xF5, 0x95, 0xCB, 0x97, 0x0A, 0xDD, 0xBB, 0x61, 0xEA, 0xEA, 0xEA, 0x2E, 0x9E, 
  0x93, 0x7A, 0x4A, 0xBD, 0x7A, 0xE5, 0xCD, 0xCD, 0xCC, 0x5C, 0x7C, 0x7C, 0x65, 0xC5, 0xC5, 0xC4, 
  0x55, 0xAB, 0x7E, 0x9E, 0x8D, 0x62, 0xE1, 0xE1, 0xE1, 0x0F, 0x07, 0x07, 0x00, 0x6A, 0xD5, 0xAA, 
  0x55, 0x2A, 0x54, 0x0D, 0x3A, 0x74, 0xC3, 0x4A, 0x95, 0x22, 0xA3, 0x44, 0xDA, 0x25, 0x42, 0x85, 
  0x00, 0xCF, 0x9F, 0x3C, 0x33, 0xA7, 0x4E, 0x29, 0xB3, 0x4D, 0x9A, 0x53, 0x26, 0x1B, 0x30, 0xA5, 
  0xCB, 0x96, 0x19, 0x52, 0xA5, 0x06, 0x4C, 0x99, 0x21, 0x91, 0x22, 0x40, 0x63, 0xC7, 0x8E, 0x51, 
  0xA3, 0x1B, 0x18, 0xA2, 0xC5, 0x36, 0x29, 0x44, 0x89, 0x10, 0x30, 0xE1, 0x9B, 0x0C, 0xA1, 0x42, 
  0x84, 0x18, 0x30, 0x4D, 0x82, 0x50, 0x20, 0x40, 0x5D, 0x7E, 0xFD, 0xF9, 0x3E, 0x7C, 0x6B, 0xE0, 
  0xBD, 0x7A, 0xF4, 0x2F, 0x1E, 0x3C, 0x0B, 0xB7, 0x6E, 0xC2, 0xE9, 0xD1, 0xAE, 0x89, 0xCB, 0x93, 
  0x5C, 0x85, 0xC3, 0x87, 0x01, 0x6E, 0xDD, 0xBA, 0xEB, 0x66, 0xCD, 0x87, 0xFA, 0x67, 0x35, 0xD9, 
  0xB3, 0x64, 0xAD, 0x5A, 0xB4, 0x5C, 0xFC, 0xFC, 0xE8, 0xDE, 0x6C, 0x58, 0xB0, 0x31, 0x0E, 0xB2, 
  0xCD, 0x86, 0x35, 0xCF, 0xE6, 0x8F, 0xF4, 0xF2, 0x72, 0x72, 0x0C, 0xF9, 0xAC, 0xB2, 0xC3, 0x43, 
  0xFF, 0x5C, 0xF7, 0xA3, 0xFC, 0xA7, 0xA9, 0xF9, 0xEA, 0x66, 0xB2, 0xCD, 0x46, 0xB5, 0x67, 0xA8, 
  0xF9, 0xBE, 0xBA, 0x97, 0xA3, 0xFC, 0x27, 0xB3, 0x79, 0xEC, 0xB6, 0x78, 0xDF, 0x39, 0xBC, 0xF6, 
  0x53, 0x34, 0xF6, 0x63, 0x47, 0xEE, 0x9E, 0xF4, 0x67, 0xBC, 0xBB, 0x56, 0xAD, 0x7D, 0x38, 0x43, 
  0xF7, 0x8D, 0xDF, 0xCF, 0x78, 0xFF, 0x6D, 0xE7, 0xD1, 0xF7, 0x4F, 0x86, 0xB9, 0xF0, 0xCB, 0x39, 
  0xBE, 0xED, 0xDE, 0xE1, 0xC2, 0xEC, 0xF8, 0x63, 0x9F, 0x0D, 0x74, 0x7E, 0xE9, 0xF2, 0x0F, 0x3D, 
  0x12, 0xDA, 0x34, 0x68, 0x39, 0xAF, 0xAA, 0xAA, 0xA1, 0x66, 0xCD, 0x9A, 0x37, 0x95, 0x14, 0x50, 
  0x99, 0x32, 0x64, 0xBC, 0xDF, 0xBD, 0xBD, 0xBC, 0x16, 0x0C, 0x18, 0x23, 0x79, 0xDD, 0xDD, 0xDD, 
  0x2D, 0xCD, 0xCD, 0xC1, 0x37, 0x9F, 0x63, 0xBD, 0xAD, 0xAD, 0xA0, 0xED, 0xED, 0xED, 0xA3, 0x79, 
  0xD9, 0xD9, 0xD9, 0x2D, 0x8D, 0x8D, 0x8F, 0x4C, 0x75, 0xB4, 0x1D, 0xE8, 0xF5, 0xB5, 0xB5, 0x83, 
  0xAF, 0xAF, 0xAE, 0x8D, 0xE7, 0x57, 0x57, 0x54, 0xB5, 0x35, 0x35, 0x3D, 0x35, 0xB3, 0x66, 0x3C, 
  0x21, 0xFA, 0x7A, 0x7A, 0x68, 0xFC, 0x3A, 0x5A, 0x5A, 0x45, 0xA3, 0xA3, 0xA2, 0x35, 0xCB, 0x39, 
  0xBF, 0x43, 0x43, 0x41, 0x1F, 0x87, 0x3F, 0x3F, 0x3C, 0xB3, 0xB3, 0xB3, 0x84, 0xDF, 0x72, 0xE7, 
  0x59, 0x66, 0x66, 0x66, 0x07, 0x37, 0x37, 0x35, 0x1B, 0xCE, 0x5E, 0x5E, 0x59, 0x65, 0x65, 0x65, 
  0x0E, 0x40, 0x76, 0xED, 0xDC, 0x2C, 0x8C, 0x8C, 0x80, 0xE4, 0xE4, 0xE4, 0xA3, 0x79, 0xC7, 0xC7, 
  0xC7, 0x0A, 0xAA, 0xA8, 0x3C, 0xD8, 0x78, 0xD8, 0xD8, 0xC8, 0xFC, 0xBF, 0x6C, 0x6F, 0xDB, 0x2C, 
  0x5C, 0x5C, 0x54, 0x7E, 0x55, 0x14, 0x54, 0x7A, 0x67, 0xF1, 0x31, 0x31, 0x11, 0xF9, 0x4F, 0xB3, 
  0x39, 0xFA, 0xC1, 0xF0, 0x70, 0x70, 0x4B, 0x03, 0x03, 0x01, 0x1F, 0x87, 0x0B, 0x0B, 0x08, 0x4D, 
  0xFD, 0x1D, 0x1D, 0x09, 0x37, 0xA4, 0xDF, 0xF5, 0x37, 0x9F, 0x8A, 0xCD, 0x26, 0xFF, 0xC4, 0xDE, 
  0x7E, 0xE3, 0xF4, 0x9B, 0xFF, 0xC9, 0xBE, 0xD5, 0xAB, 0x49, 0x37, 0xA4, 0xDF, 0xF5, 0x8D, 0xFD, 
  0xFD, 0xFD, 0xF2, 0xB3, 0x66, 0xCA, 0x4D, 0xF7, 0xE6, 0xFB, 0x16, 0x2B, 0xA4, 0xDF, 0xFE, 0xCD, 
  0xF6, 0x12, 0x6F, 0xBF, 0x37, 0xD7, 0xAE, 0x93, 0x7F, 0xF7, 0x37, 0xB1, 0x62, 0xC4, 0xB9, 0x79, 
  0x79, 0x52, 0x6F, 0xBF, 0x37, 0xD6, 0xAD, 0xC2, 0x93, 0x7D, 0xF9, 0xBE, 0xAD, 0x5A, 0xA9, 0x05, 
  0xA4, 0xDF, 0xF5, 0x8D, 0xF6, 0x0C, 0x18, 0x15, 0x4A, 0x95, 0x12, 0x0B, 0xBF, 0x37, 0xD3, 0xA7, 
  0x4D, 0x20, 0xB4, 0xC6, 0xFA, 0x63, 0x7D, 0x1F, 0xDE, 0x6F, 0xA5, 0x4A, 0x92, 0x41, 0x77, 0xE6, 
  0xFA, 0x34, 0x68, 0xA4, 0x17, 0xFF, 0x63, 0x7D, 0x3A, 0x77, 0xA6, 0x37, 0xFF, 0xF4, 0xFB, 0xE1, 
  0x9C, 0xDF, 0x42, 0x85, 0x04, 0x82, 0xEF, 0xCD, 0xF3, 0xE7, 0xCF, 0x48, 0x2F, 0xF3, 0x37, 0xFE, 
  0xBE, 0x2A, 0xF3, 0xFB, 0xA3, 0xEE, 0xFB, 0x6B, 0x7E, 0x73, 0x79, 0xFC, 0xFD, 0xBF, 0x79, 0xFD, 
  0xD1, 0xF7, 0x4E, 0x53, 0xC3, 0xC3, 0xC3, 0x13, 0x79, 0xFB, 0x2F, 0xCE, 0x73, 0x9D, 0x3A, 0x71, 
  0x4D, 0x36, 0x6C, 0xD2, 0x99, 0x30, 0xD9, 0x85, 0x2E, 0x5C, 0xB0, 0xCA, 0x95, 0x28, 0x32, 0x64, 
  0xC9, 0x0C, 0x89, 0x12, 0x03, 0x1E, 0x3C, 0x70, 0xC6, 0x8D, 0x18, 0xA2, 0xC5, 0x36, 0x28, 0x62, 
  0x44, 0x88, 0x50, 0xE1, 0x9B, 0x0C, 0xA1, 0x42, 0x36, 0x11, 0x41, 0x82, 0x6C, 0x10, 0xC0, 0x81, 
  0x00, 0x2F, 0xDF, 0xBF, 0x0B, 0xE7, 0xCF, 0x82, 0xF5, 0xEB, 0xD0, 0xBC, 0x78, 0xF0, 0x2E, 0xDD, 
  0x9A, 0xEC, 0x2E, 0x9D, 0x3A, 0x0B, 0x97, 0x2E, 0x42, 0xE1, 0xC3, 0x85, 0xD6, 0xED, 0xDB, 0x85, 
  0xB3, 0x63, 0x5B, 0x05, 0x65, 0x96, 0xFB, 0x2D, 0x56, 0x26, 0xAD, 0x5A, 0x85, 0xA3, 0x46, 0x8B, 
  0xAC, 0xD9, 0xB3, 0x5D, 0x55, 0x55, 0x57, 0x54, 0x51, 0x45, 0xD6, 0x4C, 0x94, 0x1E, 0xF5, 0x3F, 
  0x54, 0xFD, 0x33, 0xFE, 0xAB, 0x16, 0x3B, 0xE3, 0xC7, 0xDF, 0xFB, 0x27, 0x21, 0x80, 
};

TYPE_GrData _PlayState_Background_Gd =
{
  1,                              //version
  0,                              //reserved
  OSD_8888,                       //data format
  COMPRESS_Tfp,                   //compressed method
  _PlayState_Background_Cpm,
  17296,                           //size
    92,                           //width
    47                            //height
};

