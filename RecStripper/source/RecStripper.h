#ifndef __RECSTRIPPERH__
#define __RECSTRIPPERH__

//#define RS_UNICODE      TRUE
#define RS_MULTILANG    TRUE

#define PROGRAM_NAME          "RecStripper"
#ifdef FULLDEBUG
  #define VERSION             "V0.1 (debug)"
#else
  #define VERSION             "V0.1"
#endif
#define TAPID                 0x2A0A0004
#define AUTHOR                "chris86"
#define DESCRIPTION           "Shrinks recordings by removal of filler NALUs and Zero-byte-padding."
#define LOGFILE               PROGRAM_NAME ".log"
#define LNGFILENAME           PROGRAM_NAME ".lng"
#define INIFILENAME           PROGRAM_NAME ".ini"
#define LOGFILEDIR            TAPFSROOT "/ProgramFiles/Settings/" PROGRAM_NAME
#define RECSTRIPPATH          TAPFSROOT "/ProgramFiles"
#define ABSRECDIR             TAPFSROOT "/DataFiles/RecStrip"
#define ABSOUTDIR             TAPFSROOT "/DataFiles/RecStrip_out"


  int TAP_Main(void);
  dword TAP_EventHandler(word event, dword param1, dword param2);

  static bool ShowConfirmationDialog(char *MessageStr);
  static void ShowErrorMessage(char *MessageStr, char *TitleStr);

  static void LoadINI(void);
  static void SaveINI(void);

  static void CreateRootDir(void);
  static void CreateRecStripDirs(void);

  static inline dword CalcBlockSize(off_t Size);
  static void SecToTimeString(dword Time, char *const OutTimeString);  // needs max. 4 + 1 + 2 + 1 + 2 + 1 = 11 chars
  static void AbortRecStrip(void);

#endif
