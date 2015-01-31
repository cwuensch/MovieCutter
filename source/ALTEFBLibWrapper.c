#define _FILE_OFFSET_BITS  64
#define __USE_LARGEFILE64  1
#ifdef _MSC_VER
  #define __const const
#endif


#include                <stdio.h>
#include                <stdlib.h>
#include                <string.h>
#include                <unistd.h>
#include                "libFireBird.h"


inline dword FIS_fwAppl_StartPlayback(void)
{
  static dword          fwAppl_StartPlayback = 0;

  if(!fwAppl_StartPlayback)
    fwAppl_StartPlayback = TryResolve("_Z18Appl_StartPlaybackPKcjb");

  return fwAppl_StartPlayback;
}
int Appl_StartPlayback(char *FileName, unsigned int p2, bool p3, bool ScaleInPip)
{
  int (*__Appl_StartPlayback)(char const*, unsigned int, bool, bool);
  int  ret = -1;

  __Appl_StartPlayback = (void*)FIS_fwAppl_StartPlayback();
  if(__Appl_StartPlayback) ret = __Appl_StartPlayback(FileName, p2, p3, ScaleInPip);
  return ret;
}


bool FMUC_LoadFontFile(char *FontFileName, tFontData *FontData)
{
//  return FM_LoadFontFile(FontFileName, FontData);
  char *myFontFileName = FontFileName; myFontFileName++;
  tFontData *myFontData = FontData; myFontData++;
  return TRUE;
}

void  FMUC_FreeFontFile(tFontData *FontData)
{
//  FM_FreeFontFile(FontData);
  tFontData *myFontData = FontData; myFontData++;
}

dword FMUC_GetStringWidth(const char *Text, tFontData *FontData)
{
  return FM_GetStringWidth(Text, FontData);
}

dword FMUC_GetStringHeight(const char *Text, tFontData *FontData)
{
  return FM_GetStringHeight(Text, FontData);
}

void FMUC_PutString(word rgn, dword x, dword y, dword maxX, const char * str, dword fcolor, dword bcolor, tFontData *FontData, byte bDot, byte align)
{
  FM_PutString(rgn, x, y, maxX, str, fcolor, bcolor, FontData, bDot, align);
}

void FMUC_PutStringAA(word rgn, dword x, dword y, dword maxX, const char * str, dword fcolor, dword bcolor, tFontData *FontData, byte bDot, byte align, float AntiAliasFactor)
{
  FM_PutStringAA(rgn, x, y, maxX, str, fcolor, bcolor, FontData, bDot, align, AntiAliasFactor);
}


extern tMessageBox      MessageBox;
bool                    MessageBoxAllowScrollOver = FALSE;

void  OSDMenuMessageBoxAllowScrollOver()
{
  MessageBoxAllowScrollOver = TRUE;
}

void OSDMenuMessageBoxDoScrollOver(word *event, dword *param1)
{
  if(MessageBoxAllowScrollOver && MessageBox.NrButtons == 2)
  {
    if ((*event == EVT_KEY) && (*param1 == RKEY_Left))
    {
      if(MessageBox.CurrentButton == 0)
        *param1 = RKEY_Right;
    }
    if ((*event == EVT_KEY) && (*param1 == RKEY_Right))
    {  
      if(MessageBox.CurrentButton >= (MessageBox.NrButtons - 1))
        *param1 = RKEY_Left;
    }
  }
}


extern word ProgressBarOSDRgn;
extern word ProgressBarFullRgn;
extern word ProgressBarLastValue;

void OSDMenuProgressBarDestroyNoOSDUpdate(void)
{
  if(ProgressBarOSDRgn)
  {
    TAP_Osd_Delete(ProgressBarOSDRgn);
    TAP_Osd_Delete(ProgressBarFullRgn);
    ProgressBarOSDRgn = 0;
    ProgressBarFullRgn = 0;
  }
  OSDMenuInfoBoxDestroyNoOSDUpdate();
  ProgressBarLastValue =  0xfff;
}

void  OSDMenuFreeStdFonts(void) {}


inline dword FIS_vTempRecSlot(void)
{
  static dword          vTempRecSlot = 0;
  if(!vTempRecSlot)
    vTempRecSlot = TryResolve("_tempRecSlot");
  return vTempRecSlot;
}

inline dword FIS_vPvrRecTempInfo(void)
{
  static dword          vpvrRecTempInfo = 0;
  if(!vpvrRecTempInfo)
    vpvrRecTempInfo = TryResolve("_pvrRecTempInfo");
  return vpvrRecTempInfo;
}

byte *HDD_GetPvrRecTsPlayInfoPointer(byte Slot)
{
  byte                 *__pvrRecTsPlayInfo;
  byte                 *__pvrRecTempInfo;
  int                   StructSize = 0x774;
  byte                 *ret;

  if(Slot > HDD_NumberOfRECSlots())
    return NULL;

  __pvrRecTempInfo = (byte*)FIS_vPvrRecTempInfo();
  if(!__pvrRecTempInfo)
    return NULL;

  __pvrRecTsPlayInfo = (byte*)FIS_vPvrRecTsPlayInfo();
  if(!__pvrRecTsPlayInfo)
    return NULL;
  StructSize = ((dword)__pvrRecTempInfo - (dword)__pvrRecTsPlayInfo) / (HDD_NumberOfRECSlots() + 1);
  ret = &__pvrRecTsPlayInfo[StructSize * Slot];
  return ret;
}


bool HDD_TAP_CheckCollision(void)
{
  char                 *myTAPFileName, *TAPFileName;
  dword                *pCurrentTAPIndex, myTAPIndex;
  dword                 i;
  bool                  TAPCollision;

  TAPCollision = FALSE;

  pCurrentTAPIndex = (dword*)FIS_vCurTapTask();
  if(pCurrentTAPIndex)
  {
    //Get the path to myself
    myTAPIndex = *pCurrentTAPIndex;
    if(HDD_TAP_GetFileNameByIndex(myTAPIndex, &myTAPFileName) && myTAPFileName)
    {
      //Check all other TAPs
      for(i = 0; i < 16; i++)
        if((i != myTAPIndex) && HDD_TAP_GetFileNameByIndex(i, &TAPFileName) && TAPFileName && (strcmp(TAPFileName, myTAPFileName) == 0))
        {
          TAPCollision = TRUE;
          break;
        }
    }
  }
  return TAPCollision;
}
