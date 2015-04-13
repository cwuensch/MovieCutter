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


  void  OSDMenuMessageBoxAllowScrollOver();
  void  OSDMenuMessageBoxDoScrollOver(word *event, dword *param1);

  void  OSDMenuProgressBarDestroyNoOSDUpdate(void);


  int Appl_StartPlayback(char *FileName, unsigned int p2, bool p3, bool ScaleInPip);

  inline dword FIS_vTempRecSlot(void);
  byte  *HDD_GetPvrRecTsPlayInfoPointer(byte Slot);

  inline dword FIS_fwTimeToLinux(void);
  dword  PvrTimeToLinux(dword PVRTime);

  bool  HDD_TAP_CheckCollision(void);

  char *ansicstr (char *string, int len, int flags, int *sawc, int *rlen);

#endif
