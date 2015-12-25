#define _FILE_OFFSET_BITS  64
#define __USE_LARGEFILE64  1
#ifdef _MSC_VER
  #define __const const
#endif
#define __RecStripper_C__
//#define STACKTRACE      TRUE

#define _GNU_SOURCE
#include                <stdio.h>
#include                <stdlib.h>
#include                <unistd.h>
#include                <fcntl.h>
#include                <string.h>
#include                <stdarg.h>
#include                <sys/stat.h>
#include                <tap.h>
#include                "libFireBird.h"   // <libFireBird.h>
#include                "CWTapApiLib.h"
#include                "RecStripper.h"

TAP_ID                  (TAPID);
TAP_PROGRAM_NAME        (PROGRAM_NAME" "VERSION);
TAP_PROGRAM_VERSION     (VERSION);
TAP_AUTHOR_NAME         (AUTHOR);
TAP_DESCRIPTION         (DESCRIPTION);
TAP_ETCINFO             (__DATE__);


// ============================================================================
//                              Definitionen
// ============================================================================

typedef enum
{
  LS_AskConfirmation,
  LS_NoFiles,
  LS_FinishedStr,
  LS_StatusText,
  LS_Yes,
  LS_No,
  LS_OK,
  LS_NrStrings
} tLngStrings;

static char* DefaultStrings[LS_NrStrings] =
{
  "%d Aufnahme(n) jetzt schrumpfen?",
  "Keine Dateien zu verarbeiten.",
  "RecStripper beendet.\n%i von %i Dateien erfolgreich verarbeitet.",
  "Verarbeite [%d/%d] . . .\n%s\n\nZeit vergangen: %lu s, Zeit verbleibend: %lu s\n\n",
  "Ja",
  "Nein",
  "Ok"
};
#ifdef RS_MULTILANG
  #define LangGetString(x)  LangGetStringDefault(x, DefaultStrings[x])
#else
  #define LangGetString(x)  DefaultStrings[x]  
#endif


// Globale Variablen
static char             CurRecName[MAX_FILE_NAME_SIZE + 1], OutRecName[MAX_FILE_NAME_SIZE + 1];
//static char             AbsRecDir[FBLIB_DIR_SIZE], AbsOutDir[FBLIB_DIR_SIZE];
static __off64_t        RecFileSize = 0, CurOutFileSize = 0;
static int              NrRecFiles = 0, CurRecNr = 0, NrSuccessful = 0;
static dword            RecStrip_Pid = 0, RecStrip_Return = 0;
static bool             RecStrip_Cancelled = FALSE;
static dword            StartTime = 0;
static dword            LastDraw = 0;


// ============================================================================
//                              IMPLEMENTIERUNG
// ============================================================================

int TAP_Main(void)
{
  char CommandLine[2*FBLIB_DIR_SIZE], MessageStr[128];

  #if STACKTRACE == TRUE
    CallTraceInit();
    CallTraceEnable(TRUE);
  #endif
  TRACEENTER();

  if(HDD_TAP_CheckCollision())
  {
    TAP_PrintNet("RecStripper: Duplicate instance of the same TAP already started!\n");
    TRACEEXIT();
    return 0;
  }

  CreateRootDir();
  CreateRecStripDirs();
  KeyTranslate(TRUE, &TAP_EventHandler);

  WriteLogMC (PROGRAM_NAME, "***  RecStripper " VERSION " started! (FBLib " __FBLIB_VERSION__ ") ***");
  WriteLogMC (PROGRAM_NAME, "=======================================================");
  WriteLogMCf(PROGRAM_NAME, "Receiver Model: %s (%u), System Type: TMS-%c (%d)", GetToppyString(GetSysID()), GetSysID(), SysTypeToStr(), GetSystemType());
  WriteLogMCf(PROGRAM_NAME, "Firmware: %s", GetApplVer());
  WriteLogMCf(PROGRAM_NAME, "RecStripDir: %s, OutDir: %s", ABSRECDIR, ABSOUTDIR);
  if (HDD_Exist2("RecStrip", RECSTRIPPATH))
    chmod(RECSTRIPPATH "/RecStrip", 0777);
  else
  {
    WriteLogMC(PROGRAM_NAME, "ERROR! '" RECSTRIPPATH "/RecStrip' not found.");
    CloseLogMC();
    TRACEEXIT();
    return 0;
  }

  // Load Fonts
  #ifdef RS_UNICODE
    if (!(FMUC_LoadFontFile("Calibri_10.ufnt", &Calibri_10_FontData)
       && FMUC_LoadFontFile("Calibri_12.ufnt", &Calibri_12_FontData)
       && FMUC_LoadFontFile("Calibri_14.ufnt", &Calibri_14_FontData)
       && FMUC_LoadFontFile("Courier_New_13.ufnt", &Courier_New_13_FontData)))
    {
      WriteLogMC(PROGRAM_NAME, "Loading fonts failed!\r\n");
      FMUC_FreeFontFile(&Calibri_10_FontData);
      FMUC_FreeFontFile(&Calibri_12_FontData);
      FMUC_FreeFontFile(&Calibri_14_FontData);
      FMUC_FreeFontFile(&Courier_New_13_FontData);

      CloseLogMC();
      TRACEEXIT();
      return 0;
    }
  #endif

  // Load Language Strings
  #ifdef RS_MULTILANG
    if (TAP_GetSystemVar(SYSVAR_OsdLan) != LAN_German)
      if(!LangLoadStrings(LNGFILENAME, LS_NrStrings, LAN_English, PROGRAM_NAME))
      {
/*        char LogString[512];
        TAP_SPrint(LogString, sizeof(LogString), "Language file '%s' not found!", LNGFILENAME);
        WriteLogMCf(PROGRAM_NAME, "%s\r\n", LogString);
        OSDMenuInfoBoxShow(PROGRAM_NAME " " VERSION, LogString, 500);
        do
        {
          OSDMenuEvent(NULL, NULL, NULL);
        } while(OSDMenuInfoBoxIsVisible());

        #ifdef RS_UNICODE
          FMUC_FreeFontFile(&Calibri_10_FontData);
          FMUC_FreeFontFile(&Calibri_12_FontData);
          FMUC_FreeFontFile(&Calibri_14_FontData);
        #endif
        CloseLogMC();
        TRACEEXIT();
        return 0; */
      }
  #endif


  // hier der Inhalt
//  TAP_SPrint(AbsRecDir, sizeof(AbsRecDir), TAPFSROOT "/DataFiles/RecStrip");
//  TAP_SPrint(AbsOutDir, sizeof(AbsRecDir), TAPFSROOT "/DataFiles/RecStrip_out");

  TYPE_FolderEntry FolderEntry;

  TAP_Hdd_ChangeDir("/DataFiles/RecStrip");
  NrRecFiles = TAP_Hdd_FindFirst(&FolderEntry, "rec");
  WriteLogMCf(PROGRAM_NAME, "Nr of files in folder: %i", NrRecFiles);

  if (NrRecFiles > 0)
  {
    TAP_SPrint(MessageStr, sizeof(MessageStr), LangGetString(LS_AskConfirmation), NrRecFiles);
    if (ShowConfirmationDialog(MessageStr))
    {
      for (CurRecNr = 0; CurRecNr < NrRecFiles; CurRecNr++)
      {
        if(FolderEntry.attr == ATTR_NORMAL)
        {
          byte sec;
          TAP_SPrint(CurRecName, sizeof(CurRecName), FolderEntry.name);
          strcpy(OutRecName, CurRecName);
          HDD_GetFileSizeAndInode2(CurRecName, ABSRECDIR, NULL, &RecFileSize);

          WriteLogMCf(PROGRAM_NAME, "Processing file '%s' (%llu bytes)... ", CurRecName, RecFileSize);
          StartTime = PvrTimeToLinux(Now(&sec)) + sec;

          // Start RecStrip
          RecStrip_Cancelled = FALSE;
          TAP_SPrint(CommandLine, sizeof(CommandLine), "( rm /tmp/RecStrip.* ; " RECSTRIPPATH "/RecStrip \"%s/%s\" \"%s/%s\" &> /tmp/RecStrip.log ; echo $? > /tmp/RecStrip.out ) & echo $!", ABSRECDIR, CurRecName, ABSOUTDIR, OutRecName);
          FILE* fPidFile = popen(CommandLine, "r");
          if(fPidFile)
          {
//            if (fgets(PidStr, 13, fPidFile))
//              fsck_Pid = atoi(PidStr);
            fscanf(fPidFile, "%ld", &RecStrip_Pid);
            pclose(fPidFile);
          }

          //Wait for termination of fsck
          TAP_SPrint(CommandLine, sizeof(CommandLine), "/proc/%lu", RecStrip_Pid);
          while (access(CommandLine, F_OK) != -1)
          {
//            BytesRead += fread(&LogBuffer[BytesRead], 1, BufSize-BytesRead-1, fLogFile);
//            LogBuffer[BytesRead] = '\0';
//            TAP_PrintNet(LogBuffer);
            TAP_Sleep(100);
            TAP_SystemProc();
          }
            
          // Get exit code of RecStrip
          RecStrip_Return = -1;
          if (RecStrip_Pid)
          {
            FILE* fRetFile = fopen("/tmp/RecStrip.out", "rb");
            if(fRetFile)
            {
              fscanf(fPidFile, "%ld", &RecStrip_Return);
TAP_PrintNet("Return fopen: %lu\n", RecStrip_Return);
              fclose(fPidFile);
            }
/*            int fRetFile2 = open("/tmp/RecStrip.out", O_RDONLY);
            if (fRetFile2 > 0)
            {
              char tmp[10];
              if (read(fRetFile2, tmp, 10) > 0)
                RecStrip_Return = atoi(tmp);
TAP_PrintNet("Return open: %lu\n", RecStrip_Return);
              close(fRetFile2);
            } */
            system("cat /tmp/RecStrip.log >> " LOGFILEDIR "/RecStrip.log");
          }
          RecStrip_Pid = 0;

          // Output RecStrip return
          HDD_GetFileSizeAndInode2(OutRecName, ABSOUTDIR, NULL, &CurOutFileSize);
          dword CurTime = PvrTimeToLinux(Now(&sec)) + sec;
          if (RecStrip_Return == 0)
          {
            WriteLogMCf(PROGRAM_NAME, "Success! Output size %llu (%.2f %% reduced). Elapsed time: %lu s.", CurOutFileSize, ((double)CalcBlockSize(CurOutFileSize)/CalcBlockSize(RecFileSize))*100, CurTime-StartTime);
            NrSuccessful++;
          }
          else if (RecStrip_Cancelled)
            WriteLogMCf(PROGRAM_NAME, "RecStrip aborted! Elapsed time: %lu s.", CurTime-StartTime);
          else
          {
            WriteLogMCf(PROGRAM_NAME, "RecStrip returned error code %lu! Elapsed time: %lu s.", RecStrip_Return, CurTime-StartTime);
//            HDD_Delete2(OutRecName, ABSOUTDIR, TRUE);
          }
        }
        TAP_Hdd_FindNext(&FolderEntry);
      }
      OSDMenuProgressBarDestroyNoOSDUpdate();
      WriteLogMCf(PROGRAM_NAME, "%d of %d files successfully processed.", NrSuccessful, NrRecFiles);
      TAP_SPrint(MessageStr, sizeof(MessageStr), LangGetString(LS_FinishedStr), NrSuccessful, NrRecFiles);
      ShowErrorMessage(MessageStr, PROGRAM_NAME);
    }
  }
  else
    ShowErrorMessage(LangGetString(LS_NoFiles), PROGRAM_NAME);

  #ifdef RS_MULTILANG
    LangUnloadStrings();
  #endif
  #ifdef RS_UNICODE
    FMUC_FreeFontFile(&Calibri_10_FontData);
    FMUC_FreeFontFile(&Calibri_12_FontData);
    FMUC_FreeFontFile(&Calibri_14_FontData);
    OSDMenuFreeStdFonts();
  #endif
  WriteLogMC(PROGRAM_NAME, "RecStripper Exit.\r\n");
  CloseLogMC();
  TRACEEXIT();
  return 0;
}

// ----------------------------------------------------------------------------
//                            TAP EventHandler
// ----------------------------------------------------------------------------
dword TAP_EventHandler(word event, dword param1, dword param2)
{
  dword                 SysState, SysSubState;
  static bool           DoNotReenter = FALSE;

  TRACEENTER();
  #if STACKTRACE == TRUE
    TAP_PrintNet("Status = %u\n", State);
  #endif

  // Behandlung offener MessageBoxen (rekursiver Aufruf, auch bei DoNotReenter)
  if(OSDMenuMessageBoxIsVisible())
  {
    OSDMenuEvent(&event, &param1, &param2);
    param1 = 0;
  }

  // Abbruch von RecStrip ermöglichen (selbst bei DoNotReenter)
  if(OSDMenuProgressBarIsVisible())
  {
    if(event == EVT_KEY && (param1 == RKEY_Exit || param1 == FKEY_Exit || param1 == RKEY_Sleep))
      AbortRecStrip();
    param1 = 0;
  }

  if(DoNotReenter)
  {
    TRACEEXIT();
    return param1;
  }
  DoNotReenter = TRUE;

  if (event == EVT_IDLE)
  {
    if(labs(TAP_GetTick() - LastDraw) > 100)
    {
      sync();
      if (RecStrip_Pid && HDD_GetFileSizeAndInode2(OutRecName, ABSOUTDIR, NULL, &CurOutFileSize))
      {
        byte sec;
        char ProgressStr[MAX_FILE_NAME_SIZE + 128], ISORecName[MAX_FILE_NAME_SIZE + 1];
        dword CurTime = PvrTimeToLinux(Now(&sec)) + sec;
        dword percent = CalcBlockSize(CurOutFileSize)/(CalcBlockSize(RecFileSize)/100);

        StrToISO(CurRecName, ISORecName);
//        TAP_SPrint(TitleStr, sizeof(ProgressStr), LangGetString(LS_StatusTitle), CurRecNr+1, NrRecFiles);
        TAP_SPrint(ProgressStr, sizeof(ProgressStr), LangGetString(LS_StatusText), CurRecNr+1, NrRecFiles, ISORecName, CurTime-StartTime, (percent ? ((100*(CurTime-StartTime))/percent)-(CurTime-StartTime) : 0));
        OSDMenuProgressBarShow(PROGRAM_NAME, ProgressStr, percent, 100, NULL);
      }
      else
        OSDMenuProgressBarDestroyNoOSDUpdate();
      LastDraw = TAP_GetTick();
    }
  }

/*  if (!isPlaybackRunning())
  {
    sync();
    if (wget_Pid && TotalRecFileSize && HDD_GetFileSizeAndInode2(DOWNLOADNAME, DOWNLOADPATH, NULL, &CurrentRecFileSize))
    {
      if (CurrentRecFileSize >= min(4*1024*1024, TotalRecFileSize/3))
      {
        param1 = 0;
        HDD_StartPlayback2(DOWNLOADNAME, DOWNLOADPATH, TRUE);
        break;
      }
      else
        if (rgnSplashScreen)
        {
          TAP_Osd_FillBox(rgnSplashScreen, 30, 190, (_Logo_Gd.width-60) * (dword)CurrentRecFileSize / (4*1024*1024), 10, COLOR_Red);
          TAP_Osd_Sync();
        }
    }
        
    if (!wget_Pid || !TotalRecFileSize || (event == EVT_KEY && param1 == RKEY_Exit) || (labs(TAP_GetTick() - DownloadStarted) >= 3000))
    {
      WriteLogMCf(PROGRAM_NAME, "Download aborted! wget_Pid=%lu, TotalRecFileSize=%lld, ExitKey=%s, TimeOut=%lu", wget_Pid, TotalRecFileSize, (event==EVT_KEY&&param1==RKEY_Exit) ? "true" : "false", labs(TAP_GetTick() - DownloadStarted));
      ShowErrorMessage("Medien-Download fehlgeschlagen!", PROGRAM_NAME);
      AbortDownload();
      ClearOSD(FALSE);
      param1 = 0;
      State = ST_InactiveMode;
    }
  }
  else
  {
    param1 = 0;

    if(!PlayInfo.file || !PlayInfo.file->name[0]) break;
    if(((int)PlayInfo.totalBlock <= 0) || ((int)PlayInfo.currentBlock < 0)) break;

    TAP_GetState(&SysState, &SysSubState);
    if(SysState != STATE_Normal || (SysSubState != SUBSTATE_Normal && SysSubState != SUBSTATE_PvrPlayingSearch && SysSubState != 0)) break;

//        PlaybackRepeatSet(TRUE);
    LastTotalBlocks = PlayInfo.totalBlock;

    WriteLogMC(PROGRAM_NAME, "========================================\r\n");

    //Identify the file name (.rec or .mpg)
    strncpy(PlaybackName, PlayInfo.file->name, MAX_FILE_NAME_SIZE);
    PlaybackName[MAX_FILE_NAME_SIZE] = '\0';

    //Find out the absolute path to the rec file and check if it is DOWNLOADNAME
    HDD_GetAbsolutePathByTypeFile2(PlayInfo.file, AbsPlaybackDir);
    if(strcmp(AbsPlaybackDir, DOWNLOADPATH "/" DOWNLOADNAME) != 0)
    {
      State = ST_InactiveMode;
      WriteLogMC(PROGRAM_NAME, "Playback file name is not the downloaded file!");
//          ClearOSD(TRUE);
      break;
    }

    //Save only the absolute path to rec folder
    char *p;
    p = strstr(AbsPlaybackDir, PlaybackName);
    if(p) *(p-1) = '\0';

    if (strncmp(AbsPlaybackDir, TAPFSROOT, strlen(TAPFSROOT)) == 0)
      HDD_ChangeDir(&AbsPlaybackDir[strlen(TAPFSROOT)]);

    WriteLogMCf(PROGRAM_NAME, "Attaching to %s/%s", AbsPlaybackDir, PlaybackName);
    WriteLogMC (PROGRAM_NAME, "----------------------------------------");
    WriteLogMCf(PROGRAM_NAME, "Playback Description: %s", PlaybackDesc);
    WriteLogMCf(PROGRAM_NAME, "Total rec file size   = %llu Bytes (%lu blocks)", TotalRecFileSize, (dword)(TotalRecFileSize / BLOCKSIZE));
    WriteLogMCf(PROGRAM_NAME, "Current rec file size = %llu Bytes (%lu blocks)", CurrentRecFileSize, (dword)(CurrentRecFileSize / BLOCKSIZE));
    WriteLogMCf(PROGRAM_NAME, "Reported total blocks:  %lu", PlayInfo.totalBlock);

    ClearOSD(FALSE);

    State = ST_ActiveOSD;
    OSDMode = TRUE;
    OSDRedrawEverything();
  }  */

  DoNotReenter = FALSE;
  TRACEEXIT();
  return param1;
}


// ----------------------------------------------------------------------------
//                           MessageBox-Funktionen
// ----------------------------------------------------------------------------
// Die Funktionen zeigt eine Bestätigungsfrage (Ja/Nein) an, und wartet auf die Bestätigung des Benutzers.
// Nach Beendigung der Message kehrt das TAP in den Normal-Mode zurück, FALLS dieser zuvor aktiv war.
// Beim Beenden wird das entsprechende OSDRange gelöscht. Überdeckte Bereiche anderer OSDs werden NICHT wiederhergestellt.
bool ShowConfirmationDialog(char *MessageStr)
{
  dword OldSysState, OldSysSubState;
  bool ret;

  TRACEENTER();
  HDD_TAP_PushDir();
  TAP_GetState(&OldSysState, &OldSysSubState);

  OSDMenuMessageBoxInitialize(PROGRAM_NAME, MessageStr);
  OSDMenuMessageBoxDoNotEnterNormalMode(TRUE);
  OSDMenuMessageBoxAllowScrollOver();
  OSDMenuMessageBoxButtonAdd(LangGetString(LS_Yes));
  OSDMenuMessageBoxButtonAdd(LangGetString(LS_No));
  OSDMenuMessageBoxButtonSelect(1);
  OSDMenuMessageBoxShow();
  while (OSDMenuMessageBoxIsVisible())
  {
    TAP_SystemProc();
    TAP_Sleep(1);
  }
  ret = (OSDMenuMessageBoxLastButton() == 0);

  TAP_Osd_Sync();
  if(OldSysSubState != 0) TAP_EnterNormalNoInfo();
//  OSDMenuFreeStdFonts();

  HDD_TAP_PopDir();
  TRACEEXIT();
  return ret;
}

// Die Funktionen zeigt einen Informationsdialog (OK) an, und wartet auf die Bestätigung des Benutzers.
void ShowErrorMessage(char *MessageStr, char *TitleStr)
{
  dword OldSysState, OldSysSubState;

  TRACEENTER();
  HDD_TAP_PushDir();
  TAP_GetState(&OldSysState, &OldSysSubState);

  OSDMenuMessageBoxInitialize((TitleStr) ? TitleStr : PROGRAM_NAME, MessageStr);
  OSDMenuMessageBoxDoNotEnterNormalMode(TRUE);
  OSDMenuMessageBoxButtonAdd(LangGetString(LS_OK));
  OSDMenuMessageBoxShow();
  while (OSDMenuMessageBoxIsVisible())
  {
    TAP_SystemProc();
    TAP_Sleep(1);
  }

  TAP_Osd_Sync();
  if(OldSysSubState != 0) TAP_EnterNormalNoInfo();
//  OSDMenuFreeStdFonts();

  HDD_TAP_PopDir();
  TRACEEXIT();
}


// ----------------------------------------------------------------------------
//                             INI-Funktionen
// ----------------------------------------------------------------------------
void LoadINI(void)
{
  TRACEENTER();

  INILOCATION IniFileState;

  HDD_TAP_PushDir();
  HDD_ChangeDir(LOGDIR);
  IniFileState = INIOpenFile(INIFILENAME, PROGRAM_NAME);
  if((IniFileState != INILOCATION_NotFound) && (IniFileState != INILOCATION_NewFile))
  {
//    Overscan_X        =            INIGetInt("Overscan_X",                50,   0,  100);
//    Overscan_Y        =            INIGetInt("Overscan_Y",                25,   0,  100);
  }
  INICloseFile();

  if(IniFileState == INILOCATION_NewFile)
    SaveINI();
  HDD_TAP_PopDir();

  TRACEEXIT();
}

void SaveINI(void)
{
  TRACEENTER();

  HDD_TAP_PushDir();
  HDD_ChangeDir(LOGDIR);
  INIOpenFile(INIFILENAME, PROGRAM_NAME);
//  INISetInt ("Overscan_X",          Overscan_X);
//  INISetInt ("Overscan_Y",          Overscan_Y);
  INISaveFile(INIFILENAME, INILOCATION_AtCurrentDir, NULL);
  INICloseFile();
  HDD_TAP_PopDir();

  TRACEEXIT();
}


// ----------------------------------------------------------------------------
//                              Hilfsfunktionen
// ----------------------------------------------------------------------------

void CreateRootDir(void)
{
  //Check & Create Folders
  HDD_TAP_PushDir();
  HDD_ChangeDir("/ProgramFiles");
  if(!TAP_Hdd_Exist("Settings")) TAP_Hdd_Create("Settings", ATTR_FOLDER);
  HDD_ChangeDir("Settings");
  if(!TAP_Hdd_Exist(PROGRAM_NAME)) TAP_Hdd_Create(PROGRAM_NAME, ATTR_FOLDER);
  HDD_TAP_PopDir();
}
static void CreateRecStripDirs(void)
{
  //Check & Create Folders
  HDD_TAP_PushDir();
  HDD_ChangeDir("/DataFiles");
  if(!TAP_Hdd_Exist("RecStrip")) TAP_Hdd_Create("RecStrip", ATTR_FOLDER);
  if(!TAP_Hdd_Exist("RecStrip_out")) TAP_Hdd_Create("RecStrip_out", ATTR_FOLDER);
  HDD_TAP_PopDir();
}

static inline dword CalcBlockSize(off_t Size)
{
  // Workaround für die Division durch BLOCKSIZE (9024)
  // Primfaktorenzerlegung: 9024 = 2^6 * 3 * 47
  // max. Dateigröße: 256 GB (dürfte reichen...)
  return (dword)(Size >> 6) / 141;
}

void SecToTimeString(dword Time, char *const OutTimeString)  // needs max. 4 + 1 + 2 + 1 + 2 + 1 = 11 chars
{
  dword                 Hour, Min, Sec;

  if(OutTimeString)
  {
    Hour = (Time / 3600);
    Min  = (Time / 60) % 60;
    Sec  = Time % 60;
    if (Hour >= 10000) Hour = 9999;
    TAP_SPrint(OutTimeString, 11, "%lu:%02lu:%02lu", Hour, Min, Sec);
  }
}

void AbortRecStrip(void)
{
  char KillCommand[16];
  if (RecStrip_Pid)
  {
    RecStrip_Cancelled = TRUE;
    TAP_SPrint(KillCommand, sizeof(KillCommand), "kill %lu", RecStrip_Pid);
    system(KillCommand);
//    RecStrip_Pid = 0;
  }
}
