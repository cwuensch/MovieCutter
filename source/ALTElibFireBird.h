#ifndef __ALTEFBLIB__
  #define __ALTEFBLIB__

  #include <libFireBird.h>

  #ifdef STACKTRACE
    #undef TRACEENTER
    #define TRACEENTER()    CallTraceEnter((char*)__FUNCTION__)
    #undef TRACEEXIT
    #define TRACEEXIT()     CallTraceExit(NULL)
  #else
    #define TRACEENTER()
    #define TRACEEXIT()
  #endif


  extern tFontData        Calibri_10_FontData;
  extern tFontData        Calibri_12_FontData;
  extern tFontData        Calibri_14_FontData;

  #define Calibri_10_FontDataUC Calibri_10_FontData
  #define Calibri_12_FontDataUC Calibri_12_FontData
  #define Calibri_14_FontDataUC Calibri_14_FontData

  #define tFontDataUC tFontData


  #define STDSTRINGSIZE   256
  #define MAXMBBUTTONS    5

  typedef struct
  {
    dword                 NrButtons;
    dword                 CurrentButton;
    char                  Button[MAXMBBUTTONS][STDSTRINGSIZE];
    char                  Title[STDSTRINGSIZE];
    char                  Text[STDSTRINGSIZE];
//    tFontData            *FontColorPickerTitle;
//    tFontData            *FontColorPickerCursor;
  } tMessageBox;



  int Appl_StartPlayback(char *FileName, unsigned int p2, bool p3, bool ScaleInPip);

  bool  FMUC_LoadFontFile(char *FontFileName, tFontData *FontData);
  void  FMUC_FreeFontFile(tFontData *FontData);
  dword FMUC_GetStringWidth(const char *Text, tFontData *FontData);
  dword FMUC_GetStringHeight(const char *Text, tFontData *FontData);
  void  FMUC_PutString(word rgn, dword x, dword y, dword maxX, const char * str, dword fcolor, dword bcolor, tFontData *FontData, byte bDot, byte align);
  void  FMUC_PutStringAA(word rgn, dword x, dword y, dword maxX, const char * str, dword fcolor, dword bcolor, tFontData *FontData, byte bDot, byte align, float AntiAliasFactor);


  void  OSDMenuMessageBoxAllowScrollOver();
  void  OSDMenuMessageBoxDoScrollOver(word *event, dword *param1);

  void  OSDMenuProgressBarDestroyNoOSDUpdate(void);
  void  OSDMenuFreeStdFonts(void);


  inline dword FIS_vTempRecSlot(void);
  byte  *HDD_GetPvrRecTsPlayInfoPointer(byte Slot);

  bool  HDD_TAP_CheckCollision(void);

#endif
