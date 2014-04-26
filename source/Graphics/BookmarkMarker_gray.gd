#include "tap.h"

byte _BookmarkMarker_gray_Cpm[] =
{
  0x00, 0x08, 0x1B, 0x90, 0xFF, 0xFF, 0x00, 0x01, 0x00, 0x01, 
  0x00, 0x28, 0xF1, 0x7F, 0xFF, 0xFF, 0x01, 0x20, 
  0x00, 0x10, 0x3B, 0x4E, 0x4A, 0x94, 0xF3, 0x3B, 0xCD, 0xD6, 0x80, 0x83, 0x91, 0x1D, 0xAB, 0x82, 
  0xE3, 0x27, 0x62, 0x04, 0x60, 0xBC, 0x49, 0x81, 0x9B, 0x9F, 0x4B, 0x7E, 0x3D, 0x7A, 0x1F, 0xDE, 
  0x66, 0xFA, 
};

TYPE_GrData _BookmarkMarker_gray_Gd =
{
  1,                              //version
  0,                              //reserved
  OSD_8888,                       //data format
  COMPRESS_Tfp,                   //compressed method
  _BookmarkMarker_gray_Cpm, 
   288,                           //size
     8,                           //width
     9                            //height
};

