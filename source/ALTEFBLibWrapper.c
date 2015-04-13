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
/* void OSDMenuMessageBoxDoScrollOver(word *event, dword *param1)
{
  if(MessageBoxAllowScrollOver && (MessageBox.NrButtons > 1))
  {
    if ((*event == EVT_KEY) && (*param1 == RKEY_Left))
    {
      if(MessageBox.CurrentButton == 0)
      {
        MessageBox.CurrentButton = MessageBox.NrButtons - 1;
        OSDMenuMessageBoxShow();
        *param1 = 0;
      }
    }
    if ((*event == EVT_KEY) && (*param1 == RKEY_Right))
    {  
      if(MessageBox.CurrentButton >= (MessageBox.NrButtons - 1))
      {
        MessageBox.CurrentButton = 0;
        OSDMenuMessageBoxShow();
        *param1 = 0;
      }
    }
  }
} */


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


// ----------------------------------------------------------------------------
//                        ansicstr (FireBirdLib)
// ----------------------------------------------------------------------------
#include                <ctype.h>

#define CTLESC          '\001'
#define CTLNUL          '\177'
#define ESC             '\033'
#define TOUPPER(c)      (islower(c) ? toupper(c) : (c))
#define TOCTRL(x)       (TOUPPER(x) & 037)
#define ISOCTAL(c)      ((c) >= '0' && (c) <= '7')
#define OCTVALUE(c)     ((c) - '0')
#define ISDIGIT(c)      ((c) >= '0' && (c) <= '9')
#define ISXDIGIT(c)     (ISDIGIT((c)) || ((c) >= 'a' && (c) <= 'f') || ((c) >= 'A' && (c) <= 'F'))

#define HEXVALUE(c) \
  (((c) >= 'a' && (c) <= 'f') \
    ? (c)-'a'+10 \
    : (c) >= 'A' && (c) <= 'F' ? (c)-'A'+10 : (c)-'0')


char *ansicstr (char *string, int len, int flags, int *sawc, int *rlen)
{
  TRACEENTER();

  int                   c, temp;
  char                 *ret, *r, *s;
//  unsigned long         v;

  if(rlen) *rlen = 0;
  if(sawc) *sawc = 0;

  if((string == 0) || (len == 0) || (*string == '\0'))
  {
    TRACEEXIT();
    return NULL;
  }

  ret = (char*)TAP_MemAlloc(4*len + 1);
  if(ret == NULL)
  {
    TRACEEXIT();
    return NULL;
  }

  for(r = ret, s = string; s && *s;)
  {
    c = *s++;
    if (c != '\\' || *s == '\0')
      *r++ = c;
    else
    {
      switch (c = *s++)
      {
        case 'a': c = '\a'; break;
        case 'v': c = '\v'; break;
        case 'b': c = '\b'; break;
        case 'e':
        case 'E': c = ESC; break;
        case 'f': c = '\f'; break;
        case 'n': c = '\n'; break;
        case 'r': c = '\r'; break;
        case 't': c = '\t'; break;
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        {
          if(flags & 1)
          {
            *r++ = '\\';
            break;
          }
        }
        /* no break */

        case '0':
        {
          /* If (FLAGS & 1), we're translating a string for echo -e (or
               the equivalent xpg_echo option), so we obey the SUSv3/
               POSIX-2001 requirement and accept 0-3 octal digits after
               a leading `0'. */
          temp = 2 + ((flags & 1) && (c == '0'));
          for (c -= '0'; ISOCTAL (*s) && temp--; s++)
            c = (c * 8) + OCTVALUE (*s);
          c &= 0xFF;
          break;
        }

        case 'x':     /* Hex digit -- non-ANSI */
        {
          if ((flags & 2) && *s == '{')
          {
            flags |= 16;    /* internal flag value */
            s++;
          }
          /* Consume at least two hex characters */
          for (temp = 2, c = 0; ISXDIGIT ((unsigned char)*s) && temp--; s++)
            c = (c * 16) + HEXVALUE (*s);

          /* DGK says that after a `\x{' ksh93 consumes ISXDIGIT chars
               until a non-xdigit or `}', so potentially more than two
               chars are consumed. */

          if (flags & 16)
          {
            for ( ; ISXDIGIT ((unsigned char)*s); s++)
              c = (c * 16) + HEXVALUE (*s);
            flags &= ~16;
            if (*s == '}')
              s++;
          }
          /* \x followed by non-hex digits is passed through unchanged */
          else if (temp == 2)
          {
            *r++ = '\\';
            c = 'x';
          }
          c &= 0xFF;
          break;
        }

        case '\\':
          break;

        case '\'':
        case '"':
        case '?':
        {
          if(flags & 1)
            *r++ = '\\';
          break;
        }

        case 'c':
        {
          if(sawc)
          {
            *sawc = 1;
            *r = '\0';
            if(rlen) *rlen = r - ret;
            return ret;
          }
          else if((flags & 1) == 0 && *s == 0)
            ;   /* pass \c through */
          else if((flags & 1) == 0 && (c = *s))
          {
            s++;
            if((flags & 2) && c == '\\' && c == *s)
              s++;  /* Posix requires $'\c\\' do backslash escaping */
            c = TOCTRL(c);
            break;
          }
        }
        /* no break */

        default:
        {
          if((flags & 4) == 0)
            *r++ = '\\';
          break;
        }
      }

      if ((flags & 2) && (c == CTLESC || c == CTLNUL))
        *r++ = CTLESC;
      *r++ = c;
    }
  }

  *r = '\0';
  if(rlen) *rlen = r - ret;

  TRACEEXIT();
  return ret;
}
