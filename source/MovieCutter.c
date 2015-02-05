#define _FILE_OFFSET_BITS  64
#define __USE_LARGEFILE64  1
#ifdef _MSC_VER
  #define __const const
#endif

#include                <stdio.h>
#include                <stdlib.h>
#include                <string.h>
#include                <unistd.h>
#include                <tap.h>
#include                "libFireBird.h"   // <libFireBird.h>
#include                "CWTapApiLib.h"
#include                "HddToolsLib.h"
#include                "MovieCutterLib.h"
#include                "MovieCutter_TAPCOM.h"
#include                "MovieCutter.h"
#include                "Graphics/ActionMenu10.gd"
#include                "Graphics/ActionMenu_Bar.gd"
#include                "Graphics/SegmentList_Background.gd"
#include                "Graphics/SegmentList_ScrollBar.gd"
#include                "Graphics/SegmentList_ScrollButton.gd"
#include                "Graphics/Selection_Blue.gd"
//#include                "Graphics/Selection_Red.gd"
#include                "Graphics/Button_Ffwd_Active.gd"
#include                "Graphics/Button_Ffwd_Inactive.gd"
#include                "Graphics/Button_Pause_Active.gd"
#include                "Graphics/Button_Pause_Inactive.gd"
#include                "Graphics/Button_Play_Active.gd"
#include                "Graphics/Button_Play_Inactive.gd"
#include                "Graphics/Button_Rwd_Active.gd"
#include                "Graphics/Button_Rwd_Inactive.gd"
#include                "Graphics/Button_Slow_Active.gd"
#include                "Graphics/Button_Slow_Inactive.gd"
#include                "Graphics/Button_SkipLeft.gd"
#include                "Graphics/Button_SkipRight.gd"
#include                "Graphics/Button_Up.gd"
#include                "Graphics/Button_Up_small.gd"
#include                "Graphics/Button_Down.gd"
#include                "Graphics/Button_Down_small.gd"
#include                "Graphics/Button_Red.gd"
#include                "Graphics/Button_Green.gd"
#include                "Graphics/Button_Yellow.gd"
#include                "Graphics/Button_Blue.gd"
#include                "Graphics/Button_White.gd"
#include                "Graphics/Button_Recall.gd"
#include                "Graphics/Button_VF.gd"
#include                "Graphics/Button_Menu.gd"
#include                "Graphics/Button_ProgPlusMinus.gd"
#include                "Graphics/Button_Exit.gd"
#include                "Graphics/Button_Ok.gd"
#include                "Graphics/BookmarkMarker.gd"
#include                "Graphics/BookmarkMarker_current.gd"
#include                "Graphics/BookmarkMarker_gray.gd"
#include                "Graphics/SegmentMarker.gd"
#include                "Graphics/SegmentMarker_current.gd"
#include                "Graphics/SegmentMarker_gray.gd"
#include                "Graphics/PlayState_Background.gd"
#include                "Graphics/Icon_Ffwd.gd"
#include                "Graphics/Icon_Pause.gd"
#include                "Graphics/Icon_Playing.gd"
#include                "Graphics/Icon_Rwd.gd"
#include                "Graphics/Icon_Slow.gd"
#include                "Graphics/Button_Sleep_small.gd"
#include                "Graphics/Button_1_small.gd"
#include                "Graphics/Button_2_small.gd"
#include                "Graphics/Button_3_small.gd"
#include                "Graphics/Button_4_small.gd"
#include                "Graphics/Button_5_small.gd"
#include                "Graphics/Button_6_small.gd"
#include                "Graphics/Button_7_small.gd"
#include                "Graphics/Button_8_small.gd"
#include                "TMSCommander.h"


TAP_ID                  (TAPID);
TAP_PROGRAM_NAME        (PROGRAM_NAME" "VERSION);
TAP_PROGRAM_VERSION     (VERSION);
TAP_AUTHOR_NAME         (AUTHOR);
TAP_DESCRIPTION         (DESCRIPTION);
TAP_ETCINFO             (__DATE__);

// ============================================================================
//                              Definitionen
// ============================================================================

#define PLAYINFOVALID() (((int)PlayInfo.totalBlock > 0) && ((int)PlayInfo.currentBlock >= 0))

typedef struct
{
  dword                 Block;  //Block nr
  dword                 Timems; //Time in ms
  float                 Percent;
  bool                  Selected;
} tSegmentMarker;

typedef struct
{
  bool                  Bookmark;  // TRUE for Bookmark-Event, FALSE for Segment-Event
  dword                 PrevBlockNr;
  dword                 NewBlockNr;
  bool                  SegmentWasSelected;
} tUndoEvent;

typedef enum
{
  ST_Init,               //                      // TAP start (executed only once)
  ST_WaitForPlayback,    // [ST_IdleNoPlayback]  // TAP inactive, changes to active mode, as soon as a playback starts
  ST_ActiveOSD,          // [ST_Idle]            // OSD is active (playback running)
  ST_InactiveModePlaying,// [ST_IdleInvisible]   // OSD is hidden, checks if playback stopps or changes to start AutoOSD
  ST_InactiveMode,       //   "       "          // OSD is hidden and has to be manually activated
  ST_UnacceptedFile,     // ->[ST_Inact.Playing] // TAP is not active and cannot be entered (playback of unsupported file)
  ST_ActionMenu,         // [ST_ActionDialog]    // Action Menu is open, navigation only within menu
//  ST_CutFailDialog,    //                      // Show the failure dialog
  ST_Exit                //                      // Preparing to exit TAP
} tState;

typedef enum
{
  MD_NoOSD,
  MD_FullOSD,
  MD_NoSegmentList,
  MD_MiniOSD
} tOSDMode;

typedef enum
{
  RC_SRP2401,
  RC_SRP2410,
  RC_CRP,
  RC_TF5000,
  RC_NoVolKeys
} tRCUMode;

typedef enum
{
  MI_SelectFunction,
  MI_SaveSegments,
  MI_DeleteSegments,
  MI_SelectOddSegments,
  MI_SelectEvenSegments,
  MI_ClearAll,
  MI_ImportBookmarks,
  MI_ExportSegments,
  MI_DeleteFile,
  MI_ExitMC,
  MI_NrMenuItems
} tMenuItem;

typedef enum
{
 // Symbols in info bar
  LS_Add,
  LS_Delete,
  LS_Exit,
  LS_Move,
  LS_PauseMenu,
  LS_Select,
  LS_NoNavMessage,     // error message
 // Menu entries (1)
  LS_DeleteSegments,
  LS_DeleteFile,
  LS_ExitMC,
  LS_FailedResolve,  // error message
 // Menu entries (2)
  LS_GotoNextBM,
  LS_GotoPrevBM,
  LS_ImportBM,
  LS_OK,             // dialog button
 // Menu entries (3)
  LS_RemovePadding,
  LS_SaveSegments,
  LS_SegmentList,
  LS_SelectFunction,
  LS_CutHasFailed,   // error message
  LS_IsCrypted,      // error message
  LS_Cutting,
 // Menu entries (4)
  LS_DeleteOddSegments,
  LS_DeleteEvenSegments,
 // new language strings V2.0
  LS_SaveNrSegments,
  LS_Save1Segment,
  LS_SaveCurSegment,
  LS_DeleteNrSegments,
  LS_Delete1Segment,
  LS_DeleteCurSegment,
  LS_SelectOddSegments,
  LS_SelectPadding,
  LS_SelectEvenSegments,
  LS_SelectMiddle,
  LS_UnselectAll,
  LS_ClearSegmentList,
  LS_DeleteAllBookmarks,
  LS_ExportToBM,
  LS_Segments,
  LS_Bookmarks,
  LS_PageStr,
  LS_BeginStr,
  LS_EndStr,
  LS_AskConfirmation,
  LS_Yes,
  LS_No,
  LS_ListIsFull,
  LS_NoRecSize,
  LS_HDDetectionFailed,
  LS_NavLoadFailed,
  LS_NavLengthWrong,
  LS_RebootMessage,
  LS_NavPatched,
  LS_FileSystemCheck,
  LS_CheckingFileSystem,
  LS_CheckFSFailed,
  LS_CheckFSAborted,
  LS_CheckFSSuccess,
  LS_SuspectFilesFound,
  LS_FilesNotVerified,
  LS_FileNameTooLong,
  LS_Warning,
  LS_Undo,
  LS_ChangeMode,
  LS_FastNav,
  LS_OSD,
  LS_BM,
  LS_B,
  LS_S,
 // User notifications (10)
  LS_MovieCutterActive,
  LS_SegmentMode,
  LS_BookmarkMode,
  LS_SegMarkerCreated,
  LS_SegMarkerMoved,
  LS_SegMarkerDeleted,
  LS_BookmarkCreated,
  LS_BookmarkMoved,
  LS_BookmarkDeleted,
  LS_SegmentSelected,
  LS_SegmentUnselected,
  LS_UndoLastAction,
  LS_MinuteJumpActive,
  LS_MinuteJumpDisabled,
  LS_NotEnoughSpace,
  LS_NrStrings
} tLngStrings;

// Konstanten
const int               ScreenWidth = 720;
const int               ScreenHeight = 576;
const dword             ColorInfoBarTitle      = RGB(51, 51, 51);
const dword             ColorDarkBackground    = RGB(16, 16, 16);
const dword             ColorLightBackground   = RGB(24, 24, 24);
const dword             ColorInfoBarDarkSub    = RGB(30, 30, 30);
const dword             ColorInfoBarLightSub   = RGB(43, 43, 43);
const int               InfoBarRightAreaWidth  = 190;
const int               InfoBarModeAreaWidth   = 72;
const int               InfoBarLine1_Y         = 49,   InfoBarLine1Height = 30;
const int               InfoBarLine2_Y         = 84;
const int               InfoBarLine3_Y         = 128;  // alternativ: 135

// MovieCutter INI-Flags
bool                    AutoOSDPolicy      = FALSE;
tOSDMode                DefaultOSDMode     = MD_FullOSD;
int                     DefaultMinuteJump  = 0;
bool                    ShowRebootMessage  = TRUE;
dword                   MaxNavDiscrepancy  = 5000;
bool                    AskBeforeEdit      = TRUE;
bool                    SaveCutBak         = TRUE;
bool                    DisableSpecialEnd  = FALSE;
byte                    DoiCheckTest       = 1;   // 0 - nie, 1 - gesammelt am Ende (ro), 2 - gesammelt am Ende (fix), 3 - nach jedem Schnitt (ro), [4 - nach jedem Schnitt (fix)]
byte                    CheckFSAfterCut    = 1;   // 0 - nie, 1 - auto, 2 - immer
bool                    InodeMonitoring    = FALSE;
dword                   Overscan_X         = 50;
dword                   Overscan_Y         = 25;
dword                   SegmentList_X      = 55;
dword                   SegmentList_Y      = 78;
tRCUMode                RCUMode            = RC_SRP2401;
bool                    DirectSegmentsCut  = FALSE;
bool                    DisableSleepKey    = FALSE;


// MovieCutter state variables
tState                  State = ST_Init;
tOSDMode                OSDMode = MD_NoOSD;
bool                    BookmarkMode;
bool                    LinearTimeMode = FALSE;
bool                    MCShowMessageBox = FALSE;
bool                    OldRepeatMode = FALSE;
TYPE_TrickMode          TrickMode;
byte                    TrickModeSpeed;
int                     MinuteJump;                           //Seconds or 0 if deactivated
//bool                    NoPlaybackCheck = FALSE;              //Used to circumvent a race condition during the cutting process
int                     NrSelectedSegments;
word                    JumpRequestedSegment = 0xFFFF;        //Is set, when the user presses up/down to jump to another segment
dword                   JumpRequestedBlock = (dword) -1;      //Is set, when user presses Vol+/- to navigate in the progressbar
dword                   JumpRequestedTime = 0;                //Is set, when one of the options on top is active
dword                   JumpPerformedTime = 0;                //Is set after a segment jump has been performed to reduce flicker
dword                   LastMessageBoxKey;
//bool                    fsck_Cancelled;
tUndoEvent             *UndoStack = NULL;
int                     UndoLastItem;

// Playback information
TYPE_PlayInfo           PlayInfo;
char                    PlaybackName[MAX_FILE_NAME_SIZE + 1];
char                    AbsPlaybackDir[FBLIB_DIR_SIZE];
__off64_t               RecFileSize;
dword                   LastTotalBlocks = 0;
dword                   BlocksOneSecond;
dword                   BlockNrLastSecond;
dword                   BlockNrLast10Seconds;

// Video parameters
tSegmentMarker         *SegmentMarker = NULL;       //[0]=Start of file, [x]=End of file
dword                  *Bookmarks = NULL;
int                     NrSegmentMarker;
int                     ActiveSegment;
int                     NrBookmarks;
int                     NrTimeStamps = 0;
tTimeStamp             *TimeStamps = NULL;
tTimeStamp             *LastTimeStamp = NULL;
bool                    HDVideo;

// OSD object variables
#ifndef Calibri_10_FontDataUC
  tFontDataUC           Calibri_10_FontDataUC;
  tFontDataUC           Calibri_12_FontDataUC;
  tFontDataUC           Calibri_14_FontDataUC;
#endif
tFontDataUC             Courier_New_13_FontDataUC;
word                    rgnSegmentList = 0;
word                    rgnInfoBar = 0;
word                    rgnInfoBarMini = 0;
word                    rgnPlayState = 0;
word                    rgnTextState = 0;
word                    rgnActionMenu = 0;
int                     ActionMenuItem;
dword                   LastPlayStateChange = 0;


static inline dword currentVisibleBlock()
{
  return ((JumpRequestedBlock != (dword) -1) ? JumpRequestedBlock : PlayInfo.currentBlock);
}

// ============================================================================
//                              IMPLEMENTIERUNG
// ============================================================================

int TAP_Main(void)
{
  #if STACKTRACE == TRUE
    CallTraceInit();
    CallTraceEnable(TRUE);
    TRACEENTER();
  #endif

  if(HDD_TAP_CheckCollision())
  {
    TAP_PrintNet("MovieCutter: Duplicate instance of the same TAP already started!\n");
    TRACEEXIT();
    return 0;
  }

  char HDDModel[41], HDDSerial[21], HDDFirmware[9];

  CreateSettingsDir();
  KeyTranslate(TRUE, &TAP_EventHandler);

  WriteLogMC (PROGRAM_NAME, "***  MovieCutter " VERSION " started! (FBLib " __FBLIB_VERSION__ ") ***");
  WriteLogMC (PROGRAM_NAME, "=======================================================");
  WriteLogMCf(PROGRAM_NAME, "Receiver Model: %s (%u)", GetToppyString(GetSysID()), GetSysID());
  WriteLogMCf(PROGRAM_NAME, "Firmware: %s", GetApplVer());
  if (HDD_GetHddID(HDDModel, HDDSerial, HDDFirmware))
    WriteLogMCf(PROGRAM_NAME, "Hard disk: %s, FW %s, Serial: %s", HDDModel, HDDFirmware, HDDSerial);

  // Load Fonts
  if (!(FMUC_LoadFontFile("Calibri_10.ufnt", &Calibri_10_FontDataUC)
     && FMUC_LoadFontFile("Calibri_12.ufnt", &Calibri_12_FontDataUC)
     && FMUC_LoadFontFile("Calibri_14.ufnt", &Calibri_14_FontDataUC)
     && FMUC_LoadFontFile("Courier_New_13.ufnt", &Courier_New_13_FontDataUC)))
  {
    WriteLogMC(PROGRAM_NAME, "Loading fonts failed!");
    FMUC_FreeFontFile(&Calibri_10_FontDataUC);
    FMUC_FreeFontFile(&Calibri_12_FontDataUC);
    FMUC_FreeFontFile(&Calibri_14_FontDataUC);
    FMUC_FreeFontFile(&Courier_New_13_FontDataUC);

    TRACEEXIT();
    return 0;
  }

  // Load Language Strings
  if(!LangLoadStrings(LNGFILENAME, LS_NrStrings, LAN_English, PROGRAM_NAME))
  {
    char LogString[512];
    TAP_SPrint(LogString, sizeof(LogString), "Language file '%s' not found!", LNGFILENAME);
    WriteLogMC(PROGRAM_NAME, LogString);
    OSDMenuInfoBoxShow(PROGRAM_NAME " " VERSION, LogString, 500);
    do
    {
      OSDMenuEvent(NULL, NULL, NULL);
    } while(OSDMenuInfoBoxIsVisible());
    OSDMenuInfoBoxDestroy();

    FMUC_FreeFontFile(&Calibri_10_FontDataUC);
    FMUC_FreeFontFile(&Calibri_12_FontDataUC);
    FMUC_FreeFontFile(&Calibri_14_FontDataUC);
    FMUC_FreeFontFile(&Courier_New_13_FontDataUC);

    TRACEEXIT();
    return 0;
  }

  // Allocate Buffers
  SegmentMarker = (tSegmentMarker*) TAP_MemAlloc(NRSEGMENTMARKER * sizeof(tSegmentMarker));
  Bookmarks     = (dword*)          TAP_MemAlloc(NRBOOKMARKS     * sizeof(dword));
  UndoStack     = (tUndoEvent*)     TAP_MemAlloc(NRUNDOEVENTS    * sizeof(tUndoEvent));
  if (!SegmentMarker || !Bookmarks || !UndoStack)
  {
    WriteLogMC(PROGRAM_NAME, "Failed to allocate buffers!");

    TAP_MemFree(UndoStack);
    TAP_MemFree(Bookmarks);
    TAP_MemFree(SegmentMarker);
    LangUnloadStrings();
    FMUC_FreeFontFile(&Calibri_10_FontDataUC);
    FMUC_FreeFontFile(&Calibri_12_FontDataUC);
    FMUC_FreeFontFile(&Calibri_14_FontDataUC);
    FMUC_FreeFontFile(&Courier_New_13_FontDataUC);

    TRACEEXIT();
    return 0;
  }
  // Reset Undo-Stack
  UndoResetStack();

  TRACEEXIT();
  return 1;
}

// ----------------------------------------------------------------------------
//                            TAP EventHandler
// ----------------------------------------------------------------------------
dword TAP_EventHandler(word event, dword param1, dword param2)
{
  dword                 SysState, SysSubState;
  static bool           DoNotReenter = FALSE;
  static tOSDMode       LastOSDMode = MD_FullOSD;
  static dword          LastMinuteKey = 0;
  static dword          LastDraw = 0;
  static bool           InterceptShutdown = TRUE;     //Soll der Shutdown abgefangen werden?
  static bool           DoOwnJob = FALSE;             //Der Shutdown wurde abgefangen, eigenen "Job" beginnen

  (void) param2;

  TRACEENTER();
  #if STACKTRACE == TRUE
    TAP_PrintNet("Status = %u\n", State);
  #endif

  // Behandlung offener MessageBoxen (rekursiver Aufruf, auch bei DoNotReenter)
  if(MCShowMessageBox)
  {
    if(OSDMenuMessageBoxIsVisible())
    {
      if(event == EVT_KEY) LastMessageBoxKey = param1;
      #ifdef Calibri_10_FontDataUC
        OSDMenuMessageBoxDoScrollOver(&event, &param1);
      #endif
      OSDMenuEvent(&event, &param1, &param2);
    }
    if(!OSDMenuMessageBoxIsVisible())
      MCShowMessageBox = FALSE;
    param1 = 0;
  }

  // Abbruch von fsck ermöglichen (selbst bei DoNotReenter)
  if(DoNotReenter && OSDMenuProgressBarIsVisible())
  {
    if(event == EVT_KEY && (param1 == RKEY_Exit || param1 == RKEY_Sleep))
      HDD_CancelCheckFS();
    param1 = 0;
  }

  // Shutdown Server
  if(event == EVT_IDLE)
  {
    SDS();
  }
  if((event == EVT_STOP) && (param1 == 2))
  {
    WriteLogMC(PROGRAM_NAME, "EVT_STOP\n");
    if(InterceptShutdown)
    {
      WriteLogMC(PROGRAM_NAME, "Shutdown has been intercepted, starting own job\n");
      DoOwnJob = TRUE;
      TRACEEXIT();
      return 0;
    }
  }


  if(DoNotReenter)
  {
    TRACEEXIT();
    return param1;
  }
  DoNotReenter = TRUE;


  if(event == EVT_TMSCommander)
  {
    dword ret = TMSCommander_handler(param1);

    DoNotReenter = FALSE;
    TRACEEXIT();
    return ret;
  }

  if(event == EVT_TAPCOM)
  {
    TAPCOM_Channel  Channel;
    dword           ServiceID;

    Channel = TAPCOM_GetChannel(param1, NULL, &ServiceID, NULL, NULL);
    if(ServiceID == TAPCOM_SERVICE_ISOSDVISIBLE)
      TAPCOM_Finish(Channel, (State==ST_ActiveOSD || State==ST_ActionMenu || State==ST_Exit) ? 1 : 0);
    else
      TAPCOM_Reject(Channel);
  }

  // Shutdown-Event abfangen
  if(event == EVT_IDLE)
  {
    if(DoOwnJob)
    {
      WriteLogMC(PROGRAM_NAME, "Doing shutdown own job\n");
      CheckFileSystem(0, 1, 1, TRUE, TRUE, FALSE, FALSE, 0, NULL);

      //Abfang-Flag zurücksetzen, sonst endet man in einer Endlosschleife
      TAP_PrintNet("Own job is done, shutting Toppy down\n");

      InterceptShutdown = FALSE;
      DoOwnJob = FALSE;
      SDSTerminate();
      Shutdown(TaskPower);
    }
  }

  if((event == EVT_STOP) && (param1 != 2))
  {
    State = ST_Exit;
  }

  // Notfall-AUS
#ifdef FULLDEBUG
  if((event == EVT_KEY) && (param1 == RKEY_Sleep) && !DisableSleepKey)
  {
    if (OSDMenuMessageBoxIsVisible()) OSDMenuMessageBoxDestroy();

//    DumpInodeFixingList("/mnt/hd/FixInodes.lst");
//    DumpInodeFixingList("/tmp/FixInodes.tmp");
//    HDD_FixInodeList(((AbsPlaybackDir[0]) ? AbsPlaybackDir : TAPFSROOT), TRUE);

//    TAP_EnterNormal();
    State = ST_Exit;
    param1 = 0;
  }

/* // **** LÖSCHEN ****
if((event == EVT_KEY) && (param1 == RKEY_Sat) && (State==ST_ActiveOSD || State==ST_InactiveModePlaying || State==ST_InactiveMode || State==ST_WaitForPlayback || State==ST_UnacceptedFile) && !DisableSleepKey)
{
  CheckFileSystem(0, 1, 1, TRUE, TRUE, FALSE, FALSE, 0, NULL);
} */
#endif


  switch(State)
  {
    // Initialisierung bei TAP-Start
    // -----------------------------
    case ST_Init:
    {
/*
      LastTotalBlocks = 0;
      MCShowMessageBox = FALSE;
      NrTimeStamps = 0;
      TimeStamps = NULL;
      LastTimeStamp = NULL;
      JumpRequestedSegment = 0xFFFF;
      JumpRequestedBlock = (dword) -1;
      JumpRequestedTime = 0;
      JumpPerformedTime = 0;
*/
      // Load INI
      LoadINI();
      OSDMode = DefaultOSDMode;

      // Set the system time to current time
//      /*if (InodeMonitoring)*/ SetSystemTimeToCurrent();

      // Fix list of defect inodes
      if (HDD_Exist2("jfs_fsck", FSCKPATH)) system("chmod a+x " FSCKPATH "/jfs_fsck &");
      if(InodeMonitoring) HDD_FixInodeList(TAPFSROOT, TRUE);  // Problem: NUR für die interne HDD wird die Liste jemals zurückgesetzt!

      CleanupCut();

      State = AutoOSDPolicy ? ST_WaitForPlayback : ST_InactiveMode;
      break;
    }

    // Warten / Aufnahme analysieren
    // -----------------------------
    case ST_WaitForPlayback:    // Idle loop while there is no playback active, shows OSD as soon as playback starts
    {
      if(isPlaybackRunning())
      {
        param1 = 0;
        if(!PlayInfo.file || !PlayInfo.file->name[0]) break;
        if(((int)PlayInfo.totalBlock <= 0) || ((int)PlayInfo.currentBlock < 0)) break;

        TAP_GetState(&SysState, &SysSubState);
        if(SysState != STATE_Normal || (SysSubState != SUBSTATE_Normal && SysSubState != SUBSTATE_PvrPlayingSearch && SysSubState != 0)) break;

//        OSDMode = DefaultOSDMode;  // unnötig
        BookmarkMode = FALSE;
        NrSegmentMarker = 0;
        ActiveSegment = 0;
        MinuteJump = DefaultMinuteJump;
        JumpRequestedSegment = 0xFFFF;    // eigentlich unnötig
        JumpRequestedBlock = (dword) -1;  //   "
//        JumpRequestedTime = 0;            //   "
//        NoPlaybackCheck = FALSE;
        LastTotalBlocks = PlayInfo.totalBlock;
//        UndoResetStack();  // unnötig

        OldRepeatMode = PlaybackRepeatGet();
        PlaybackRepeatSet(TRUE);

        //Flush the caches *experimental*
        sync();
        TAP_Sleep(1);

        WriteLogMC(PROGRAM_NAME, "========================================\r\n");

        //Identify the file name (.rec or .mpg)
        strncpy(PlaybackName, PlayInfo.file->name, MAX_FILE_NAME_SIZE);
        PlaybackName[MAX_FILE_NAME_SIZE] = '\0';
        PlaybackName[strlen(PlaybackName) - 4] = '\0';

        //Find out the absolute path to the rec file and check for max length
        HDD_GetAbsolutePathByTypeFile2(PlayInfo.file, AbsPlaybackDir);
        if((strlen(PlaybackName) + 18 > MAX_FILE_NAME_SIZE) || (strlen(AbsPlaybackDir) + 14 >= FBLIB_DIR_SIZE))  // 18 = ' (Cut-123)' + '.nav.bak' | 14 = ' (Cut-123)' + '.bak'
        {
          State = ST_UnacceptedFile;
          WriteLogMC(PROGRAM_NAME, "File name or path is too long!");
          ShowErrorMessage(LangGetString(LS_FileNameTooLong), NULL);
          PlaybackRepeatSet(OldRepeatMode);
          ClearOSD(TRUE);
          break;
        }

        //Save only the absolute path to rec folder
        char *p;
        p = strstr(AbsPlaybackDir, PlaybackName);
        if(p) *(p-1) = '\0';

//        HDD_ChangeDir(PlaybackDir);
        if (strncmp(AbsPlaybackDir, TAPFSROOT, strlen(TAPFSROOT)) == 0)
          HDD_ChangeDir(&AbsPlaybackDir[strlen(TAPFSROOT)]);

        WriteLogMCf(PROGRAM_NAME, "Attaching to %s/%s", AbsPlaybackDir, PlaybackName);
        WriteLogMC ("MovieCutterLib", "----------------------------------------");

        //Free the old timing array, so that it is empty (NULL pointer) if something goes wrong
        if(TimeStamps)
        {
          TAP_MemFree(TimeStamps);
          TimeStamps = NULL;
        }
        NrTimeStamps = 0;
        LastTimeStamp = NULL;

        // Detect size of rec file
        __ino64_t InodeNr = 0;
        if(!HDD_GetFileSizeAndInode2(PlaybackName, AbsPlaybackDir, &InodeNr, &RecFileSize) || !RecFileSize)
        {
          State = ST_UnacceptedFile;
          WriteLogMC(PROGRAM_NAME, ".rec size could not be detected!");
          ShowErrorMessage(LangGetString(LS_NoRecSize), NULL);
          PlaybackRepeatSet(OldRepeatMode);
          ClearOSD(TRUE);
          break;
        }

        WriteLogMCf(PROGRAM_NAME, "Inode Nr  = %llu", InodeNr);
        WriteLogMCf(PROGRAM_NAME, "File size = %llu Bytes (%lu blocks)", RecFileSize, (dword)(RecFileSize / BLOCKSIZE));
        WriteLogMCf(PROGRAM_NAME, "Reported total blocks: %lu", PlayInfo.totalBlock);

        //Check if it is crypted
        if(isCrypted(PlaybackName, AbsPlaybackDir))
        {
          State = ST_UnacceptedFile;
          WriteLogMC(PROGRAM_NAME, "File is crypted!");
          ShowErrorMessage(LangGetString(LS_IsCrypted), NULL);
          PlaybackRepeatSet(OldRepeatMode);
          ClearOSD(TRUE);
          break;
        }

        //Check if a nav is available
        if(!isNavAvailable(PlaybackName, AbsPlaybackDir))
        {
          if (ShowConfirmationDialog(LangGetString(LS_NoNavMessage)))
          {
            WriteLogMC(PROGRAM_NAME, ".nav file not found! Using linear time mode...");
            LinearTimeMode = TRUE;
          }
          else
          {
            WriteLogMC(PROGRAM_NAME, ".nav is missing!");
            PlaybackRepeatSet(OldRepeatMode);
            ClearOSD(TRUE);
            if (AutoOSDPolicy)
              State = ST_UnacceptedFile;
            else
            {
              Cleanup(FALSE);
              State = ST_InactiveMode;
            }
            break;
          }
        }

        // Detect if video stream is in HD
        HDVideo = FALSE;
        if (!LinearTimeMode && !isHDVideo(PlaybackName, AbsPlaybackDir, &HDVideo))
        {
          State = ST_UnacceptedFile;
          WriteLogMC(PROGRAM_NAME, "Could not detect type of video stream!");
          ShowErrorMessage(LangGetString(LS_HDDetectionFailed), NULL);
          PlaybackRepeatSet(OldRepeatMode);
          ClearOSD(TRUE);
          break;
        }
        WriteLogMCf(PROGRAM_NAME, "Type of recording: %s", (HDVideo) ? "HD" : "SD");

        // Try to load the nav
        if (!LinearTimeMode)
        {
          TimeStamps = NavLoad(PlaybackName, AbsPlaybackDir, &NrTimeStamps, HDVideo);
          if (TimeStamps)
          {
            // Write duration to log file
            char TimeStr[16];
            WriteLogMCf(PROGRAM_NAME, ".nav-file loaded: %d different TimeStamps found.", NrTimeStamps);
            MSecToTimeString(TimeStamps[0].Timems, TimeStr);
            WriteLogMCf(PROGRAM_NAME, "First Timestamp: Block=%lu, Time=%s", TimeStamps[0].BlockNr, TimeStr);
            MSecToTimeString(TimeStamps[NrTimeStamps-1].Timems, TimeStr);
            WriteLogMCf(PROGRAM_NAME, "Playback Duration (from nav): %s", TimeStr);
            SecToTimeString(60*PlayInfo.duration + PlayInfo.durationSec, TimeStr);
            WriteLogMCf(PROGRAM_NAME, "Playback Duration (from inf): %s", TimeStr);
            LastTimeStamp = &TimeStamps[0];
          }
          else
          {
            State = ST_UnacceptedFile;
            WriteLogMC(PROGRAM_NAME, "Error loading the .nav file!");
            ShowErrorMessage(LangGetString(LS_NavLoadFailed), NULL);
            PlaybackRepeatSet(OldRepeatMode);
            ClearOSD(TRUE);
            break;
          }
        }

        // Check if nav has correct length!
        dword CurNavDiscrepancy = labs(TimeStamps[NrTimeStamps-1].Timems - (1000 * (60*PlayInfo.duration + PlayInfo.durationSec)));
        if(TimeStamps && (((MaxNavDiscrepancy > 0) && (CurNavDiscrepancy > MaxNavDiscrepancy)) || (CurNavDiscrepancy > 5000)))
        {
          char  NavLengthWrongStr[128];
          WriteLogMC(PROGRAM_NAME, ".nav file length not matching duration!");

          // [COMPATIBILITY LAYER - fill holes in old nav file]
          if (PatchOldNavFile(PlaybackName, AbsPlaybackDir, HDVideo))
          {
            LastTotalBlocks = 0;
            WriteLogMC(PROGRAM_NAME, ".nav file patched by Compatibility Layer.");
            ShowErrorMessage(LangGetString(LS_NavPatched), NULL);
            PlaybackRepeatSet(OldRepeatMode);
            break;
          }
          else
          {
            TAP_SPrint(NavLengthWrongStr, sizeof(NavLengthWrongStr), LangGetString(LS_NavLengthWrong), (TimeStamps[NrTimeStamps-1].Timems/1000) - (60*PlayInfo.duration + PlayInfo.durationSec));
            if ((MaxNavDiscrepancy > 0) && (CurNavDiscrepancy > MaxNavDiscrepancy))
            {
              if (!ShowConfirmationDialog(NavLengthWrongStr))
              {
                PlaybackRepeatSet(OldRepeatMode);
                ClearOSD(TRUE);
                if (AutoOSDPolicy)
                  State = ST_UnacceptedFile;
                else
                {
                  Cleanup(FALSE);
                  State = ST_InactiveMode;
                }
                break;
              }
            }
          }
        }

        // Check if receiver has been rebooted since the recording
        dword RecDateTime;
        if (GetRecDateFromInf(PlaybackName, AbsPlaybackDir, &RecDateTime))
        {
          dword TimeSinceRec = TimeDiff(RecDateTime, Now(NULL));
          dword UpTime = GetUptime() / 6000;
          #ifdef FULLDEBUG
            if (RecDateTime > 0xd0790000)
              RecDateTime = TF2UnixTime(RecDateTime);
            TAP_PrintNet("Reboot-Check (%s): TimeSinceRec=%lu, UpTime=%lu, RecDateTime=%s", (TimeSinceRec <= UpTime + 1) ? "TRUE" : "FALSE", TimeSinceRec, UpTime, ctime((time_t*) &RecDateTime));
          #endif

          if (TimeSinceRec <= UpTime + 1)
          {
            if (ShowRebootMessage && !ShowConfirmationDialog(LangGetString(LS_RebootMessage)))
            {
              PlaybackRepeatSet(OldRepeatMode);
              ClearOSD(TRUE);
              if (AutoOSDPolicy)
                State = ST_UnacceptedFile;
              else
              {
                Cleanup(FALSE);
                State = ST_InactiveMode;
              }
              break;
            }
          }
        }

        // Check if free disc space is greater than RecFileSize
        __off64_t FreeDiscSpace = 0;
        FreeDiscSpace = HDD_GetFreeDiscSpace(PlaybackName, AbsPlaybackDir);
        WriteLogMCf(PROGRAM_NAME, "HDD free space = %llu Bytes", FreeDiscSpace);
        if (FreeDiscSpace < RecFileSize)
        {
          if (!ShowConfirmationDialog(LangGetString(LS_NotEnoughSpace)))
          {
            PlaybackRepeatSet(OldRepeatMode);
            ClearOSD(TRUE);
            if (AutoOSDPolicy)
              State = ST_UnacceptedFile;
            else
            {
              Cleanup(FALSE);
              State = ST_InactiveMode;
            }
            break;
          }
        }

        CalcLastSeconds();
        if (!ReadBookmarks(Bookmarks, &NrBookmarks))
          WriteLogMC(PROGRAM_NAME, "Error: ReadBookmarks() failed!");
        if(!CutFileLoad())
        {
          if(!AddDefaultSegmentMarker())
          {
            State = ST_UnacceptedFile;
            WriteLogMC(PROGRAM_NAME, "Error adding default segment markers!");
            ShowErrorMessage(LangGetString(LS_NavLoadFailed), NULL);
            PlaybackRepeatSet(OldRepeatMode);
            ClearOSD(TRUE);
            break;
          }
        }

        State = ST_ActiveOSD;
        OSDRedrawEverything();
        OSDTextStateWindow(LS_MovieCutterActive);
      }
      break;
    }

    // Inaktiver Modus
    // ---------------
    case ST_UnacceptedFile:
    case ST_InactiveModePlaying:  // OSD is hidden, checks if playback stopps or changes to start AutoOSD
    {
/*      if (State != ST_UnacceptedFile && !AutoOSDPolicy)  // dann kein Cleanup mehr, wenn Playback stoppt
      {
        State = ST_InactiveMode;
        break;
      } 
*/
      // if playback stopped or changed -> show MovieCutter as soon as next playback is started (ST_WaitForPlayback)
      if (!isPlaybackRunning() || (LastTotalBlocks == 0) || (LastTotalBlocks != PlayInfo.totalBlock))
      {
        Cleanup(FALSE);
        if(InodeMonitoring) HDD_FixInodeList(AbsPlaybackDir, TRUE);
        State = (AutoOSDPolicy) ? ST_WaitForPlayback : ST_InactiveMode;
        break;
      }

      if (State == ST_UnacceptedFile) break;

      if((event == EVT_KEY) && (param1 == RKEY_Ab || param1 == RKEY_Option))
      {
        TAP_GetState(&SysState, &SysSubState);
        if(SysState != STATE_Normal || (SysSubState != SUBSTATE_Normal && SysSubState != SUBSTATE_PvrPlayingSearch)) break;  // (nur wenn kein OSD eingeblendet ist!)

        // beim erneuten Einblenden kann man sich das Neu-Berechnen aller Werte sparen (AUCH wenn 2 Aufnahmen gleiche Blockzahl haben!!)
        if ((int)PlayInfo.currentBlock >= 0)
        {
          OSDMode = LastOSDMode;
//          NoPlaybackCheck = FALSE;
//          BookmarkMode = FALSE;
//          MinuteJump = DefaultMinuteJump;
          JumpRequestedSegment = 0xFFFF;    // eigentlich unnötig
          JumpRequestedBlock = (dword) -1;  //   "
//          JumpRequestedTime = 0;            //   "
          if (!ReadBookmarks(Bookmarks, &NrBookmarks))
            WriteLogMC(PROGRAM_NAME, "Error: ReadBookmarks() failed!");
          OldRepeatMode = PlaybackRepeatGet();
          PlaybackRepeatSet(TRUE);
          State = ST_ActiveOSD;
          OSDRedrawEverything();
          OSDTextStateWindow(LS_MovieCutterActive);
        }
        param1 = 0;
      }
      break;
    }

    case ST_InactiveMode:    // OSD is hidden and has to be manually activated
    {
      // if cut-key is pressed -> show MovieCutter OSD (ST_WaitForPlayback)
      if((event == EVT_KEY) && (param1 == RKEY_Ab || param1 == RKEY_Option))
      {
        TAP_GetState(&SysState, &SysSubState);
        if(SysState != STATE_Normal || (SysSubState != SUBSTATE_Normal && SysSubState != SUBSTATE_PvrPlayingSearch)) break;  // (nur wenn kein OSD eingeblendet ist!)
        if (!isPlaybackRunning()) break;

        State = ST_WaitForPlayback;
        param1 = 0;
      }
      break;
    }

    // Aktiver OSD-Modus
    // -----------------
    case ST_ActiveOSD:             //Playback is active and OSD is visible
    {
      if(!isPlaybackRunning())
      {
        if (LastTotalBlocks > 0)
        {
#ifdef FULLDEBUG
  WriteLogMC(PROGRAM_NAME, "TAP_EventHandler(): State=ST_ActiveOSD, !isPlaybackRunning --> Aufruf von CutFileSave()");
#endif
          CutFileSave();
        }
        Cleanup(TRUE);
        if(InodeMonitoring) HDD_FixInodeList(AbsPlaybackDir, TRUE);
        State = AutoOSDPolicy ? ST_WaitForPlayback : ST_InactiveMode;
        break;
      }

      if((event == EVT_KEY) && ((param1==RKEY_Exit && OSDMode==MD_NoOSD && !rgnInfoBarMini) || param1==RKEY_Stop || param1==RKEY_Info || param1==RKEY_Teletext || param1==RKEY_PlayList || param1==RKEY_AudioTrk || param1==RKEY_Subt))
      {
#ifdef FULLDEBUG
  WriteLogMC(PROGRAM_NAME, "TAP_EventHandler(): State=ST_ActiveOSD, Key=RKEY_Exit --> Aufruf von CutFileSave()");
#endif
        if (OSDMode != MD_NoOSD)
          LastOSDMode = OSDMode;
        JumpRequestedSegment = 0xFFFF;
        JumpRequestedBlock = (dword) -1;
        CutFileSave();
        PlaybackRepeatSet(OldRepeatMode);
        State = ST_InactiveModePlaying;
        ClearOSD(TRUE);
        if (param1 == RKEY_Exit) param1 = 0;
//        else if (param1 == RKEY_Info) TAP_GenerateEvent(EVT_KEY, RKEY_Info, 0);

/*        //Exit immediately so that other cases can not interfere with the cleanup
        DoNotReenter = FALSE;
        TRACEEXIT();
        return ((param1 != RKEY_Exit) ? param1 : 0);  */
        break;
      }

      if ((int)PlayInfo.currentBlock < 0)
        break;  // *** kritisch ***

      TAP_GetState(&SysState, &SysSubState);
      if(SysSubState != 0) TAP_ExitNormal();

      if(event == EVT_KEY)
      {
        bool ReturnKey = FALSE;

        switch(param1)
        {
          case RKEY_Record:  // Warum das!?
            break;
        
          case RKEY_Exit:
          {
            if ((JumpRequestedSegment != 0xFFFF) || (JumpRequestedBlock != (dword) -1))
            {
              JumpRequestedSegment = 0xFFFF;
              JumpRequestedBlock = (dword) -1;
              JumpRequestedTime = 0;
              SetCurrentSegment();
              OSDSegmentListDrawList(FALSE);
              OSDInfoDrawProgressbar(TRUE, TRUE);
              break;
            }

            if (OSDMode != MD_NoOSD)
              LastOSDMode = OSDMode;
            OSDMode = MD_NoOSD;
            OSDRedrawEverything();
            OSDTextStateWindow(LS_MovieCutterActive);
            break;
          }

          case RKEY_Ab:
          case RKEY_Option:
          {
            if (OSDMode == MD_NoOSD)
              OSDMode = LastOSDMode;
            else
              OSDMode = (tOSDMode) ((OSDMode + 1) % 4);
            if(OSDMode == MD_NoOSD) OSDMode = MD_FullOSD;
            OSDRedrawEverything();
//            OSDTextStateWindow(LS_MovieCutterActive);
            break;
          }

          case RKEY_VFormat:
          case RKEY_Fav:
          case RKEY_Guide:
          {
            BookmarkMode = !BookmarkMode;
//            OSDRedrawEverything();
            OSDInfoDrawMinuteJump(FALSE);
            OSDInfoDrawBookmarkMode(FALSE);
            OSDInfoDrawProgressbar(TRUE, TRUE);
            OSDTextStateWindow((BookmarkMode) ? LS_BookmarkMode : LS_SegmentMode);
            break;
          }

          case RKEY_Up:
          {
            Playback_SetJumpNavigate(TRUE, FALSE, TRUE);
            break;
          }
          case RKEY_Down:
          {
            Playback_SetJumpNavigate(TRUE, FALSE, FALSE);
            break;
          }

          case RKEY_Prev:
          case RKEY_Next:
          case RKEY_VolDown:
          case RKEY_VolUp:
          case RKEY_ChDown:
          case RKEY_ChUp:
          {
            // Navigation nach links
            if (   ((RCUMode==RC_SRP2401 || RCUMode==RC_NoVolKeys) && param1==RKEY_ChDown) || (RCUMode==RC_SRP2410 && param1==RKEY_ChUp)   || (param1==RKEY_VolDown && RCUMode!=RC_SRP2410 && RCUMode!=RC_SRP2401 && RCUMode!=RC_NoVolKeys))
              Playback_SetJumpNavigate(FALSE, TRUE, TRUE);

            // Navigation nach rechts
            else if( ((RCUMode==RC_SRP2401 || RCUMode==RC_NoVolKeys) && param1==RKEY_ChUp) || (RCUMode==RC_SRP2410 && param1==RKEY_ChDown) || (param1==RKEY_VolUp   && RCUMode!=RC_SRP2410 && RCUMode!=RC_SRP2401 && RCUMode!=RC_NoVolKeys))
              Playback_SetJumpNavigate(FALSE, TRUE, FALSE);

            // MultiJump nach links
            else if ( (param1==RKEY_Prev) || (RCUMode==RC_SRP2401 && param1==RKEY_VolDown) || (RCUMode==RC_SRP2410 && param1==RKEY_VolUp))
            {
              if(MinuteJump)
                Playback_JumpBackward();
              else if(BookmarkMode)
                Playback_JumpPrevBookmark();
              else
                Playback_JumpPrevSegment();  
            }

            // MultiJump nach rechts
            else if ( (param1==RKEY_Next) || (RCUMode==RC_SRP2401 && param1==RKEY_VolUp)   || (RCUMode==RC_SRP2410 && param1==RKEY_VolDown))
            {
              if(MinuteJump)
                Playback_JumpForward();
              else if(BookmarkMode)
                Playback_JumpNextBookmark();
              else
                Playback_JumpNextSegment();
            }

            // bei anderen Fernbedienungen (ohne Up/Down) steuert ChUp/ChDown die Up/Down-Tase
            else if (RCUMode != RC_SRP2410 && RCUMode != RC_SRP2401 && RCUMode != RC_NoVolKeys)
            {
WriteLogMC("DEBUG-Ausgabe", "ChUp/ChDown-Event empfangen, das nicht durch Up/Down gehandelt wurde!");
              if (param1 == RKEY_ChUp)          Playback_SetJumpNavigate(TRUE, FALSE, TRUE);
              else if (param1 == RKEY_ChDown)   Playback_SetJumpNavigate(TRUE, FALSE, FALSE);
            }

            // bei RCUMode = RC_NoVolKeys die Volume-Tasten durchreichen
            else if ((RCUMode == RC_NoVolKeys) && (param1 == RKEY_VolUp || param1 == RKEY_VolDown))
              ReturnKey = TRUE;
            break;
          }

          case RKEY_Left:
          {
            Playback_Slower();
            break;
          }

          case RKEY_Right:
          {
            Playback_Faster();
            break;
          }

          case RKEY_Recall:
          {
            if (UndoLastAction())
            {
              OSDSegmentListDrawList(FALSE);
              OSDInfoDrawProgressbar(TRUE, TRUE);
              OSDTextStateWindow(LS_UndoLastAction);
            }
            break;
          }

          case RKEY_F1:
          case RKEY_Red:
          {
            if (BookmarkMode)
            {  
              int NearestIndex = FindNearestBookmark(currentVisibleBlock());
              if (NearestIndex >= 0)
              {
                dword NearestBlock = Bookmarks[NearestIndex];
                if (DeleteBookmark(NearestIndex))
                {
                  UndoAddEvent(TRUE, NearestBlock, 0, FALSE);
                  OSDTextStateWindow(LS_BookmarkDeleted);
                }
                OSDInfoDrawProgressbar(TRUE, TRUE);
//                OSDRedrawEverything();
              }
            }
            else
            {
              if (JumpRequestedSegment != 0xFFFF)
                break;
              int NearestIndex = FindNearestSegmentMarker(currentVisibleBlock());
              if (NearestIndex > 0)
              {
                dword NearestBlock = SegmentMarker[NearestIndex].Block;
                bool SegmentWasSelected = SegmentMarker[NearestIndex].Selected;
                if (DeleteSegmentMarker(NearestIndex))
                {
                  UndoAddEvent(FALSE, NearestBlock, 0, SegmentWasSelected);
                  OSDSegmentListDrawList(FALSE);
                  OSDInfoDrawProgressbar(TRUE, TRUE);
                  OSDTextStateWindow(LS_SegMarkerDeleted);
//                  OSDRedrawEverything();
                }
              }
            } 
            break;
          }

          case RKEY_Green:
          {
            dword newBlock = currentVisibleBlock();
            if (BookmarkMode)
            {
              if(AddBookmark(newBlock, TRUE) >= 0)
              {
                UndoAddEvent(TRUE, 0, newBlock, FALSE);
                OSDTextStateWindow(LS_BookmarkCreated);
              }
              OSDInfoDrawProgressbar(TRUE, TRUE);
//              OSDRedrawEverything();
            }
            else
            {
              if(AddSegmentMarker(newBlock, TRUE) >= 0)
              {
                UndoAddEvent(FALSE, 0, newBlock, FALSE);
                OSDSegmentListDrawList(FALSE);
                OSDInfoDrawProgressbar(TRUE, TRUE);
                OSDTextStateWindow(LS_SegMarkerCreated);
//                OSDRedrawEverything();
                JumpPerformedTime = TAP_GetTick();
                if(!JumpPerformedTime) JumpPerformedTime = 1;
              }
            }
            break;
          }

          case RKEY_Yellow:
          {
            dword newBlock = currentVisibleBlock();
            if (BookmarkMode)
            {
              int NearestIndex = FindNearestBookmark(newBlock);
              if (NearestIndex >= 0)
              {
                dword NearestBlock = Bookmarks[NearestIndex];
                if(MoveBookmark(NearestIndex, newBlock, TRUE))
                {
                  UndoAddEvent(TRUE, NearestBlock, newBlock, FALSE);
                  OSDTextStateWindow(LS_BookmarkMoved);
                }
                OSDInfoDrawProgressbar(TRUE, TRUE);
              }
            }
            else
            {
              int NearestIndex = FindNearestSegmentMarker(newBlock);
              if (NearestIndex > 0)
              {
                dword NearestBlock = SegmentMarker[NearestIndex].Block;
                if(MoveSegmentMarker(NearestIndex, newBlock, TRUE))
                {
                  UndoAddEvent(FALSE, NearestBlock, newBlock, FALSE);
                  if(ActiveSegment + 1 == NearestIndex) ActiveSegment++;
                  OSDSegmentListDrawList(FALSE);
                  OSDInfoDrawProgressbar(TRUE, TRUE);
                  OSDTextStateWindow(LS_SegMarkerMoved);
                  JumpPerformedTime = TAP_GetTick();
                  if(!JumpPerformedTime) JumpPerformedTime = 1;
                }
              }
            }
//            OSDRedrawEverything();
            break;
          }

          case RKEY_Blue:
          {
            bool Selected = SelectSegmentMarker();
            OSDSegmentListDrawList(FALSE);
            OSDInfoDrawProgressbar(TRUE, TRUE);
            OSDTextStateWindow((Selected) ? LS_SegmentSelected : LS_SegmentUnselected);
//            OSDRedrawEverything();
            break;
          }

          case RKEY_Menu:
          {
            if (JumpRequestedSegment != 0xFFFF)
              break;

            if(TrickMode != TRICKMODE_Pause)
            {
              Playback_Pause();
            }
            if (OSDMode != MD_FullOSD)
            {
              LastOSDMode = OSDMode;
              OSDMode = MD_FullOSD;
              OSDRedrawEverything();
            }
            State = ST_ActionMenu;
            ActionMenuItem = 0;
            ActionMenuDraw();
            break;
          }

          case RKEY_Ok:
          {
            if(TrickMode == TRICKMODE_Normal)
              Playback_Pause();
            else
              Playback_Normal();
            break;
          }

          case RKEY_Play:
          {
            if(TrickMode == TRICKMODE_Normal)
            {
/*              switch (OSDMode)
              {
                case MD_NoOSD:
                case MD_FullOSD:       OSDMode = MD_MiniOSD; break;
                case MD_MiniOSD:       OSDMode = MD_NoSegmentList; break;
                case MD_NoSegmentList: OSDMode = MD_FullOSD; break;
              }  
              OSDRedrawEverything();  */
              if (OSDMode == MD_NoOSD)
              {
                LastPlayStateChange = TAP_GetTick();
                if(!LastPlayStateChange) LastPlayStateChange = 1;
                OSDMode = MD_MiniOSD;
                OSDRedrawEverything();
                OSDMode = MD_NoOSD;
              }
//              OSDTextStateWindow(LS_MovieCutterActive);
            }
            else
              Playback_Normal();
            break;
          }

          case RKEY_Pause:
          {
            Playback_Pause();
            break;
          }

          case RKEY_Forward:
          {
            Playback_FFWD();
            break;
          }

          case RKEY_Rewind:
          {
            Playback_RWD();
            break;
          }

          case RKEY_Slow:
          {
            Playback_Slow();
            break;
          }

/*          case RKEY_Stop:
          {
            TAP_Hdd_StopTs();
            HDD_ChangeDir(PlaybackDir);
            break;
          }
*/
          case RKEY_0:
          case RKEY_1:
          case RKEY_2:
          case RKEY_3:
          case RKEY_4:
          case RKEY_5:
          case RKEY_6:
          case RKEY_7:
          case RKEY_8:
          case RKEY_9:
          {
//            if(PLAYINFOVALID())   // PlayInfo ist sicher, da bereits in Z. 804 überprüft
//            {
              if ((MinuteJump > 0) && (MinuteJump < 10) && (labs(TAP_GetTick() - LastMinuteKey) < 200))
                // We are already in minute jump mode, but only one digit already entered
                MinuteJump = 10 * MinuteJump + (param1 & 0x0f);
              else
                MinuteJump = (param1 & 0x0f);
              LastMinuteKey = TAP_GetTick();
              OSDInfoDrawMinuteJump(TRUE);
              OSDTextStateWindow((MinuteJump) ? LS_MinuteJumpActive : LS_MinuteJumpDisabled);
              break;
//            }
          }
          default:
            ReturnKey = TRUE;
        }
        if (!ReturnKey)
          param1 = 0;
      }

      // VORSICHT!!! Das hier wird interaktiv ausgeführt
      if(labs(TAP_GetTick() - LastDraw) > 10)
      {
        if (JumpRequestedTime && (labs(TAP_GetTick() - JumpRequestedTime) >= 100))
        {
          if ((JumpRequestedSegment != 0xFFFF) || (JumpRequestedBlock != (dword) -1))
          {
            if(TrickMode == TRICKMODE_Pause) Playback_Normal();
            if (JumpRequestedSegment != 0xFFFF)
            {
              TAP_Hdd_ChangePlaybackPos(SegmentMarker[JumpRequestedSegment].Block);
              ActiveSegment = JumpRequestedSegment;
              JumpRequestedBlock = (dword) -1;
            }
            else if (JumpRequestedBlock != (dword) -1)
              TAP_Hdd_ChangePlaybackPos(JumpRequestedBlock);
            JumpPerformedTime = TAP_GetTick();    if(!JumpPerformedTime) JumpPerformedTime = 1;
            LastPlayStateChange = TAP_GetTick();  if(!LastPlayStateChange) LastPlayStateChange = 1;
            JumpRequestedSegment = 0xFFFF;
          }
          JumpRequestedTime = 0;
        }
        if(LastPlayStateChange && (labs(TAP_GetTick() - LastPlayStateChange) > 300))
        {
          if(rgnPlayState)
          {
            TAP_Osd_Delete(rgnPlayState);
            rgnPlayState = 0;
          }
          if(rgnTextState)
          {
            TAP_Osd_Delete(rgnTextState);
            rgnTextState = 0;
          }
          if(rgnInfoBarMini && OSDMode != MD_MiniOSD)
          {
            TAP_Osd_Delete(rgnInfoBarMini);
            rgnInfoBarMini = 0;
          }
          LastPlayStateChange = 0;
        }
        CheckLastSeconds();
        SetCurrentSegment();
        OSDInfoDrawProgressbar(FALSE, FALSE);
        OSDInfoDrawPlayIcons(FALSE, FALSE);
        if ((JumpRequestedBlock != (dword) -1) && !JumpRequestedTime)  // nach Sprung den grauen Balken aktualisieren
          OSDInfoDrawCurrentPlayTime(TRUE);
        else
          OSDInfoDrawCurrentPlayTime(FALSE);
        OSDInfoDrawClock(FALSE);
        TAP_Osd_Sync();
        LastDraw = TAP_GetTick();
      }
      break;
    }

    // ActionMenu eingeblendet
    // -----------------------
    case ST_ActionMenu:        //Action dialog is visible
    {
      if(event == EVT_KEY)
      {
        dword SelectedMenuItem = param1 & 0x0f;
        switch(param1)
        {
          case RKEY_1:
          case RKEY_2:
          case RKEY_3:
          case RKEY_4:
          case RKEY_5:
          case RKEY_6:
          case RKEY_7:
          case RKEY_8:
            if (SelectedMenuItem == MI_DeleteFile && BookmarkMode)
              break;
            if ((SelectedMenuItem<=4 && NrSegmentMarker<=2) || (SelectedMenuItem==MI_ClearAll && !((BookmarkMode && NrBookmarks>0) || (!BookmarkMode && NrSegmentMarker>2))) || (SelectedMenuItem==MI_ImportBookmarks && NrBookmarks<=0) || (SelectedMenuItem==MI_ExportSegments && NrSegmentMarker<=2))
              break;
            ActionMenuItem = SelectedMenuItem;
            ActionMenuDraw();
            // fortsetzen mit OK-Button ...

          case RKEY_Ok:
          {
            if(ActionMenuItem != MI_SelectFunction)
            {
              // Deaktivierte Aktionen
//              if ((ActionMenuItem==MI_SaveSegments || ActionMenuItem==MI_DeleteSegments || ActionMenuItem==MI_SelectOddSegments || ActionMenuItem==MI_SelectEvenSegments) && NrSegmentMarker<=2)
              if ((ActionMenuItem<=4 && NrSegmentMarker<=2) || (ActionMenuItem==MI_ClearAll && !((BookmarkMode && NrBookmarks>0) || (!BookmarkMode && (NrSelectedSegments>0 || NrSegmentMarker>2)))) || (ActionMenuItem==MI_ImportBookmarks && NrBookmarks<=0) || (ActionMenuItem==MI_ExportSegments && NrSegmentMarker<=2))
                break;

              // Aktionen mit Confirmation-Dialog
              if (ActionMenuItem==MI_SaveSegments || ActionMenuItem==MI_DeleteSegments || (ActionMenuItem==MI_ClearAll && (BookmarkMode || NrSelectedSegments==0)) || (ActionMenuItem==MI_DeleteFile && BookmarkMode) || (ActionMenuItem==MI_ImportBookmarks && NrSegmentMarker>2) || (ActionMenuItem==MI_ExportSegments && NrBookmarks>0))
              {
                if(AskBeforeEdit && !ShowConfirmationDialog(LangGetString(LS_AskConfirmation)))
                {
//                  OSDRedrawEverything();
                  ActionMenuDraw();
                  break;
                }
              }

              // PlayInfo prüfen
              if (!(isPlaybackRunning() && PLAYINFOVALID()) && ActionMenuItem != MI_ExitMC && ActionMenuItem != MI_DeleteFile)   // PlayInfo wird nicht aktualisiert
                break;

              ActionMenuRemove();
              State = ST_ActiveOSD;

              switch(ActionMenuItem)
              {
                case MI_SaveSegments:        MovieCutterSaveSegments(); break;
                case MI_DeleteSegments:      MovieCutterDeleteSegments(); break;
                case MI_SelectOddSegments:   MovieCutterSelectOddSegments();  /*if(!DirectSegmentsCut) {ActionMenuDraw(); State = ST_ActionMenu;}*/ break;
                case MI_SelectEvenSegments:  MovieCutterSelectEvenSegments(); /*if(!DirectSegmentsCut) {ActionMenuDraw(); State = ST_ActionMenu;}*/ break;
                case MI_ClearAll:            
                {  
                  if(BookmarkMode)
                    DeleteAllBookmarks();
                  else
                  {
                    if(NrSelectedSegments > 0)
                    {
                      MovieCutterUnselectAll();
                      break;
                    }
                    else
                      DeleteAllSegmentMarkers();
                  }
                  OSDSegmentListDrawList(FALSE);
                  OSDInfoDrawProgressbar(TRUE, TRUE);
                  break;
                }
                case MI_ImportBookmarks:     ImportBookmarksToSegments(); break;
                case MI_ExportSegments:      ExportSegmentsToBookmarks(); break;
                case MI_DeleteFile:
                {
                  if (!BookmarkMode)
                    CheckFileSystem(0, 1, 1, TRUE, FALSE, FALSE, TRUE, 0, NULL);
                  else
                    MovieCutterDeleteFile();
                  break;
                }
                case MI_ExitMC:              State = ST_Exit; break;
                case MI_NrMenuItems:         break;
              }
            }
            break;
          }

          case RKEY_Down:
          case RKEY_ChDown:
          {
            ActionMenuDown();
            break;
          }

          case RKEY_Up:
          case RKEY_ChUp:
          {
            ActionMenuUp();
            break;
          }

          case RKEY_VFormat:
          case RKEY_Fav:
          case RKEY_Guide:
          {
            BookmarkMode = !BookmarkMode;
            OSDInfoDrawMinuteJump(FALSE);
            OSDInfoDrawBookmarkMode(FALSE);
            OSDInfoDrawProgressbar(TRUE, TRUE);
            ActionMenuDraw();
            break;
          }

          case RKEY_Exit:
          {
            ActionMenuRemove();
            State = ST_ActiveOSD;
            break;
          }
        }
        param1 = 0;
      }
      break;
    }

    // Beendigung des TAPs
    // -------------------
    case ST_Exit:             //Preparing to terminate the TAP
    {
      if (LastTotalBlocks > 0)
      {
#ifdef FULLDEBUG
  WriteLogMC(PROGRAM_NAME, "TAP_EventHandler(): State=ST_Exit --> Aufruf von CutFileSave()");
#endif
        CutFileSave();
      }
      if(isPlaybackRunning()) PlaybackRepeatSet(OldRepeatMode);
      Cleanup(TRUE);
      if(InodeMonitoring)
        HDD_FixInodeList(((AbsPlaybackDir[0]) ? AbsPlaybackDir : TAPFSROOT), TRUE);
      SDSTerminate();
      TAP_MemFree(UndoStack);
      TAP_MemFree(Bookmarks);
      TAP_MemFree(SegmentMarker);
      LangUnloadStrings();
      FMUC_FreeFontFile(&Calibri_10_FontDataUC);
      FMUC_FreeFontFile(&Calibri_12_FontDataUC);
      FMUC_FreeFontFile(&Calibri_14_FontDataUC);
      FMUC_FreeFontFile(&Courier_New_13_FontDataUC);
      OSDMenuFreeStdFonts();
      WriteLogMC(PROGRAM_NAME, "MovieCutter Exit.");
      TAP_Exit();
      break;
    }
  }

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
  OSDMenuSaveMyRegion(rgnSegmentList);
  OSDMenuMessageBoxDoNotEnterNormalMode(TRUE);
  OSDMenuMessageBoxAllowScrollOver();
  OSDMenuMessageBoxButtonAdd(LangGetString(LS_Yes));
  OSDMenuMessageBoxButtonAdd(LangGetString(LS_No));
  OSDMenuMessageBoxButtonSelect(1);
  OSDMenuMessageBoxShow();
  MCShowMessageBox = TRUE;
  while (MCShowMessageBox)
  {
    TAP_SystemProc();
    TAP_Sleep(1);
  }
  ret = (LastMessageBoxKey == RKEY_Ok) && (OSDMenuMessageBoxLastButton() == 0);

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
  OSDMenuSaveMyRegion(rgnSegmentList);
  OSDMenuMessageBoxDoNotEnterNormalMode(TRUE);
  OSDMenuMessageBoxButtonAdd(LangGetString(LS_OK));
  OSDMenuMessageBoxShow();
  MCShowMessageBox = TRUE;
  while (MCShowMessageBox)
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

dword TMSCommander_handler(dword param1)
{
  TRACEENTER();

  switch (param1)
  {
    case TMSCMDR_Capabilities:
    {
      TRACEEXIT();
      return (dword)(TMSCMDR_CanBeStopped | TMSCMDR_HaveUserEvent);
    }

    case TMSCMDR_UserEvent:
    {
      if(State == ST_InactiveMode || State == ST_InactiveModePlaying)
        State = ST_WaitForPlayback;

      TRACEEXIT();
      return TMSCMDR_OK;
    }

    case TMSCMDR_Menu:
    {
      TRACEEXIT();
      return TMSCMDR_NotOK;
    }

    case TMSCMDR_Stop:
    {
      State = ST_Exit;

      TRACEEXIT();
      return TMSCMDR_OK;
    }
  }

  TRACEEXIT();
  return TMSCMDR_UnknownFunction;
}

// ----------------------------------------------------------------------------
//                            Reset-Funktionen
// ----------------------------------------------------------------------------
void ClearOSD(bool EnterNormal)
{
  TRACEENTER();

  if(rgnPlayState)
  {
    TAP_Osd_Delete(rgnPlayState);
    rgnPlayState = 0;
  }
  if(rgnTextState)
  {
    TAP_Osd_Delete(rgnTextState);
    rgnTextState = 0;
  }

  if(rgnActionMenu)
  {
    TAP_Osd_Delete(rgnActionMenu);
    rgnActionMenu = 0;
  }

  if(rgnSegmentList)
  {
    TAP_Osd_Delete(rgnSegmentList);
    rgnSegmentList = 0;
  }

  if(rgnInfoBar)
  {
    TAP_Osd_Delete(rgnInfoBar);
    rgnInfoBar = 0;
  }

  if(rgnInfoBarMini)
  {
    TAP_Osd_Delete(rgnInfoBarMini);
    rgnInfoBarMini = 0;
  }

  TAP_Osd_Sync();
  if(EnterNormal)
  {
    dword SysState, SysSubState;
    TAP_GetState(&SysState, &SysSubState);
    if(SysSubState == 0) TAP_EnterNormal();
  }
  TRACEEXIT();
}

void Cleanup(bool DoClearOSD)
{
  TRACEENTER();

  LastTotalBlocks = 0;
  LinearTimeMode = FALSE;
  JumpRequestedSegment = 0xFFFF;
  JumpRequestedBlock = (dword) -1;
  JumpRequestedTime = 0;
  JumpPerformedTime = 0;
  if(TimeStamps)
  {
    TAP_MemFree(TimeStamps);
    TimeStamps = NULL;
  }
//  LastTimeStamp = NULL;
//  NrTimeStamps = 0;
  OSDMode = DefaultOSDMode;
  UndoResetStack();
  if (DoClearOSD) ClearOSD(TRUE);

  TRACEEXIT();
}

void CleanupCut(void)
{
  int                   NrFiles, i;
  TYPE_FolderEntry      FolderEntry;
  char                  RecFileName[MAX_FILE_NAME_SIZE + 1], MpgFileName[MAX_FILE_NAME_SIZE + 1];

  TRACEENTER();

  HDD_TAP_PushDir();
  HDD_ChangeDir("/DataFiles");
  
  // Lösche verweiste .cut-Dateien in /DataFiles (nicht in Unterverzeichnissen)
  NrFiles = TAP_Hdd_FindFirst(&FolderEntry, "cut");
  for (i = 0; i < NrFiles; i++)
  {
    if(FolderEntry.attr == ATTR_NORMAL)
    {
      strncpy(RecFileName, FolderEntry.name, sizeof(RecFileName));
      RecFileName[MAX_FILE_NAME_SIZE] = '\0';
      RecFileName[strlen(RecFileName) - 4] = '\0';
      strcat(RecFileName, ".rec");
      strncpy(MpgFileName, FolderEntry.name, sizeof(MpgFileName));
      MpgFileName[MAX_FILE_NAME_SIZE] = '\0';
      MpgFileName[strlen(MpgFileName) - 4] = '\0';
      strcat(MpgFileName, ".mpg");
      if(!TAP_Hdd_Exist(RecFileName) && !TAP_Hdd_Exist(MpgFileName))
        TAP_Hdd_Delete(FolderEntry.name);
    }
    TAP_Hdd_FindNext(&FolderEntry);
  }

  // Lösche verweiste .cut.bak-Dateien in /DataFiles
  NrFiles = TAP_Hdd_FindFirst(&FolderEntry, "bak");
  for (i = 0; i < NrFiles; i++)
  {
    if(FolderEntry.attr == ATTR_NORMAL)
    {
      strncpy(RecFileName, FolderEntry.name, sizeof(RecFileName));
      RecFileName[MAX_FILE_NAME_SIZE] = '\0';
      strncpy(MpgFileName, FolderEntry.name, sizeof(MpgFileName));
      MpgFileName[MAX_FILE_NAME_SIZE] = '\0';
      if (StringEndsWith(RecFileName, ".cut.bak"))
      {
        RecFileName[strlen(RecFileName) - 8] = '\0';
        MpgFileName[strlen(MpgFileName) - 8] = '\0';
        strcat(RecFileName, ".rec");
        strcat(MpgFileName, ".mpg");
        if(!TAP_Hdd_Exist(RecFileName) && !TAP_Hdd_Exist(MpgFileName))
          TAP_Hdd_Delete(FolderEntry.name);
      }
    }
    TAP_Hdd_FindNext(&FolderEntry);
  }
  HDD_TAP_PopDir();

  TRACEEXIT();
}

// ----------------------------------------------------------------------------
//                             INI-Funktionen
// ----------------------------------------------------------------------------
void CreateSettingsDir(void)
{
  TRACEENTER();

  HDD_TAP_PushDir();
  HDD_ChangeDir("/ProgramFiles");
  if(!TAP_Hdd_Exist("Settings")) TAP_Hdd_Create("Settings", ATTR_FOLDER);
  HDD_ChangeDir("Settings");
  if(!TAP_Hdd_Exist("MovieCutter")) TAP_Hdd_Create("MovieCutter", ATTR_FOLDER);
  HDD_TAP_PopDir();

  TRACEEXIT();
}

void LoadINI(void)
{
  TRACEENTER();

  INILOCATION IniFileState;

  HDD_TAP_PushDir();
  HDD_ChangeDir(LOGDIR);
  IniFileState = INIOpenFile(INIFILENAME, PROGRAM_NAME);
  if((IniFileState != INILOCATION_NotFound) && (IniFileState != INILOCATION_NewFile))
  {
    AutoOSDPolicy     =            INIGetInt("AutoOSDPolicy",              0,   0,    1)   ==   1;
    DefaultOSDMode    = (tOSDMode) INIGetInt("DefaultOSDMode",    MD_FullOSD,   0,    3);
    DefaultMinuteJump =            INIGetInt("DefaultMinuteJump",          0,   0,   99);
    ShowRebootMessage =            INIGetInt("ShowRebootMessage",          1,   0,    1)   !=   0;
    MaxNavDiscrepancy =            INIGetInt("MaxNavDiscrepancy",       5000,   0,  86400000);       // = 24 Stunden
    AskBeforeEdit     =            INIGetInt("AskBeforeEdit",              1,   0,    1)   !=   0;
    SaveCutBak        =            INIGetInt("SaveCutBak",                 1,   0,    1)   !=   0;
    DisableSpecialEnd =            INIGetInt("DisableSpecialEnd",          0,   0,    1)   ==   1;
    DoiCheckTest      =            INIGetInt("DoiCheckTest",               1,   0,    4);
    CheckFSAfterCut   =            INIGetInt("CheckFSAfterCut",            1,   0,    2);
    InodeMonitoring   =            INIGetInt("InodeMonitoring",            0,   0,    1)   ==   1;

    Overscan_X        =            INIGetInt("Overscan_X",                50,   0,  100);
    Overscan_Y        =            INIGetInt("Overscan_Y",                25,   0,  100);
    SegmentList_X     =            INIGetInt("SegmentList_X",             50,   0,  ScreenWidth - _SegmentList_Background_Gd.width);
    SegmentList_Y     =            INIGetInt("SegmentList_Y",             82,   0,  ScreenHeight - _SegmentList_Background_Gd.height);

    RCUMode           =            INIGetInt("RCUMode",           RC_SRP2401,   0,    4);
    DirectSegmentsCut =            INIGetInt("DirectSegmentsCut",          0,   0,    1)   ==   1;
    DisableSleepKey   =            INIGetInt("DisableSleepKey",            0,   0,    1)   ==   1;
  }
  INICloseFile();
  if (!AutoOSDPolicy && DefaultOSDMode == MD_NoOSD)
    DefaultOSDMode = MD_FullOSD;

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
  INISetInt ("AutoOSDPolicy",       AutoOSDPolicy       ?  1  :  0);
  INISetInt ("DefaultOSDMode",      DefaultOSDMode);
  INISetInt ("DefaultMinuteJump",   DefaultMinuteJump);
  INISetInt ("ShowRebootMessage",   ShowRebootMessage   ?  1  :  0);
  INISetInt ("MaxNavDiscrepancy",   MaxNavDiscrepancy);
  INISetInt ("AskBeforeEdit",       AskBeforeEdit       ?  1  :  0);
  INISetInt ("SaveCutBak",          SaveCutBak          ?  1  :  0);
  INISetInt ("DisableSpecialEnd",   DisableSpecialEnd   ?  1  :  0);
  INISetInt ("DoiCheckTest",        DoiCheckTest);
  INISetInt ("CheckFSAfterCut",     CheckFSAfterCut);
  INISetInt ("InodeMonitoring",     InodeMonitoring     ?  1  :  0);
  INISetInt ("Overscan_X",          Overscan_X);
  INISetInt ("Overscan_Y",          Overscan_Y);
  INISetInt ("SegmentList_X",       SegmentList_X);
  INISetInt ("SegmentList_Y",       SegmentList_Y);
  INISetInt ("RCUMode",             RCUMode);
  INISetInt ("DirectSegmentsCut",   DirectSegmentsCut   ?  1  :  0);
  INISetInt ("DisableSleepKey",     DisableSleepKey     ?  1  :  0);
  INISaveFile(INIFILENAME, INILOCATION_AtCurrentDir, NULL);
  INICloseFile();
  HDD_TAP_PopDir();

  TRACEEXIT();
}


// ----------------------------------------------------------------------------
//                         SegmentMarker-Funktionen
// ----------------------------------------------------------------------------
void ImportBookmarksToSegments(void)
{
  char LogString[512];
  int i;

  TRACEENTER();

  if (NrBookmarks > 0)
  {
    // first, delete all present segment markers (*CW*)
    UndoResetStack();
    DeleteAllSegmentMarkers();

    // second, add a segment marker for each bookmark
    WriteLogMCf(PROGRAM_NAME, "Importing %d of %d bookmarks", min(NrBookmarks, NRSEGMENTMARKER-2), NrBookmarks);

    for(i = 0; i < min(NrBookmarks, NRSEGMENTMARKER-2); i++)
    { 
      TAP_SPrint(LogString, sizeof(LogString), "Bookmark %d @ %lu", i + 1, Bookmarks[i]);
      // Erlaube keinen neuen SegmentMarker zu knapp am Anfang oder Ende oder über totalBlock
      if ((Bookmarks[i] > SegmentMarker[0].Block + 3*BlocksOneSecond) && (Bookmarks[i] + 3*BlocksOneSecond < SegmentMarker[NrSegmentMarker-1].Block))
        AddSegmentMarker(Bookmarks[i], FALSE);
      else
        TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), " -> ignored! (Too close to begin/end.)");
      WriteLogMC(PROGRAM_NAME, LogString);
    }

    SetCurrentSegment();
    OSDSegmentListDrawList(FALSE);
    OSDInfoDrawProgressbar(TRUE, TRUE);
//    OSDRedrawEverything();
  }
  TRACEEXIT();
}

bool AddDefaultSegmentMarker(void)
{
  bool ret;
  TRACEENTER();

  NrSegmentMarker = 0;
  ret = (AddSegmentMarker(0, TRUE) >= 0);
  ret = ret && (AddSegmentMarker(PlayInfo.totalBlock, TRUE) >= 0);

  TRACEEXIT();
  return ret;
}

int AddSegmentMarker(dword newBlock, bool RejectSmallSegments)
{
  char                  StartTime[16];
  dword                 newTime;
  int                   ret = -1;
  int                   i, j;

  TRACEENTER();

  if(newBlock > PlayInfo.totalBlock  /*!PLAYINFOVALID() || ((int)newBlock < 0)*/)   // PlayInfo ist sicher, da in Z.804 und im ActionMenu überprüft, Prüfung für newBlock nun restriktiver
  {
    WriteLogMC(PROGRAM_NAME, "AddSegmentMarker: newBlock > totalBlock - darf nicht auftreten!");
    TRACEEXIT();
    return -1;
  }

  if(NrSegmentMarker >= NRSEGMENTMARKER)
  {
    WriteLogMC(PROGRAM_NAME, "AddSegmentMarker: SegmentMarker list is full!");
    ShowErrorMessage(LangGetString(LS_ListIsFull), NULL);
//    OSDRedrawEverything();

    TRACEEXIT();
    return -1;
  }

  newTime = NavGetBlockTimeStamp(newBlock);
  if((newTime == 0) && (newBlock > 3 * BlocksOneSecond))
  {
    TRACEEXIT();
    return -1;
  }

  //Find the point where to insert the new marker so that the list stays sorted
  if(NrSegmentMarker < 2)
  {
    //If less than 2 markers present, then set marker for start and end of file (called from AddDefaultSegmentMarker)
    SegmentMarker[NrSegmentMarker].Block = newBlock;
    SegmentMarker[NrSegmentMarker].Timems  = newTime;
    SegmentMarker[NrSegmentMarker].Percent = ((float)newBlock / PlayInfo.totalBlock) * 100.0;
    SegmentMarker[NrSegmentMarker].Selected = FALSE;
    ret = NrSegmentMarker;
    NrSegmentMarker++;
  }
  else
  {
    for(i = 1; i < NrSegmentMarker; i++)
    {
      if(SegmentMarker[i].Block >= newBlock)
      {
        if(SegmentMarker[i].Block == newBlock)
        {
          TRACEEXIT();
          return -1;
        }
        
        // Erlaube kein Segment mit weniger als 3 Sekunden
        if (RejectSmallSegments && ((i > 1 && (newBlock <= SegmentMarker[i-1].Block + 3*BlocksOneSecond)) || (i < NrSegmentMarker-2 && (newBlock + 3*BlocksOneSecond >= SegmentMarker[i].Block))))
        {
          TRACEEXIT();
          return -1;
        }

        for(j = NrSegmentMarker; j > i; j--)
          memcpy(&SegmentMarker[j], &SegmentMarker[j - 1], sizeof(tSegmentMarker));

        SegmentMarker[i].Block = newBlock;
        SegmentMarker[i].Timems = newTime;
        SegmentMarker[i].Percent = ((float)newBlock / PlayInfo.totalBlock) * 100.0;
        SegmentMarker[i].Selected = FALSE;

        MSecToTimeString(SegmentMarker[i].Timems, StartTime);
        WriteLogMCf(PROGRAM_NAME, "New marker @ block = %lu, time = %s, percent = %1.1f%%", newBlock, StartTime, SegmentMarker[i].Percent);

        if(ActiveSegment + 1 >= i) ActiveSegment++;
        ret = i;
        break;
      }
    }
    NrSegmentMarker++;
  }

  TRACEEXIT();
  return ret;
}

int FindSegmentWithBlock(dword curBlock)
{
  int                   SegmentIndex;
  int                   i;

  TRACEENTER();
  SegmentIndex = 0;
  
  if (NrSegmentMarker > 2)
  {
    for(i = 0; i < NrSegmentMarker - 1; i++)
      if(SegmentMarker[i + 1].Block > curBlock)
      {
        SegmentIndex = i;
        break;
      }
  }
  TRACEEXIT();
  return SegmentIndex;
}

int FindNearestSegmentMarker(dword curBlock)
{
  int                   NearestMarkerIndex;
  long                  MinDelta;
  int                   i;

  TRACEENTER();

  NearestMarkerIndex = -1;
  if(NrSegmentMarker > 2)   // at least one segment marker present
  {
    MinDelta = 0x7fffffff;
    for(i = 1; i < NrSegmentMarker - 1; i++)
    {
      if(labs(SegmentMarker[i].Block - curBlock) < MinDelta)
      {
        MinDelta = labs(SegmentMarker[i].Block - curBlock);
        NearestMarkerIndex = i;
      }
    }
//    if (NearestMarkerIndex == 0) NearestMarkerIndex = 1;
//    if (NearestMarkerIndex == NrSegmentMarker-1) NearestMarkerIndex = NrSegmentMarker - 2;
  }

  TRACEEXIT();
  return NearestMarkerIndex;
}

bool MoveSegmentMarker(int MarkerIndex, dword newBlock, bool RejectSmallSegments)
{
  dword newTime;
  bool ret = FALSE;

  TRACEENTER();

  if(newBlock > PlayInfo.totalBlock  /*!PLAYINFOVALID() || ((int)newBlock < 0)*/)   // PlayInfo ist sicher, da in Z.804 überprüft, Prüfung für newBlock nun restriktiver
  {
    WriteLogMC(PROGRAM_NAME, "MoveSegmentMarker: newBlock > totalBlock - darf nicht auftreten!");
    TRACEEXIT();
    return FALSE;
  }

  if((MarkerIndex > 0) && (MarkerIndex < NrSegmentMarker - 1))
  {
    if (SegmentMarker[MarkerIndex].Block == newBlock)
    {
      TRACEEXIT();
      return FALSE;
    }
    // Erlaube kein Segment mit weniger als 3 Sekunden
    if (RejectSmallSegments && ((MarkerIndex > 1 && (newBlock <= SegmentMarker[MarkerIndex-1].Block + 3*BlocksOneSecond)) || (MarkerIndex < NrSegmentMarker-2 && (newBlock + 3*BlocksOneSecond >= SegmentMarker[MarkerIndex+1].Block))))
    {
      TRACEEXIT();
      return FALSE;
    }

    // neue Zeit ermitteln
    newTime = NavGetBlockTimeStamp(newBlock);
    if((newTime != 0) || (newBlock <= 3 * BlocksOneSecond))
    {
      SegmentMarker[MarkerIndex].Block = newBlock;
      SegmentMarker[MarkerIndex].Timems = newTime;
      SegmentMarker[MarkerIndex].Percent = ((float)newBlock / PlayInfo.totalBlock) * 100.0;
      ret = TRUE;
    }
  }

  TRACEEXIT();
  return ret;
}

bool DeleteSegmentMarker(int MarkerIndex)
{
  int i;
  bool ret = FALSE;

  TRACEENTER();

  if((MarkerIndex > 0) && (MarkerIndex < NrSegmentMarker - 1))
  {
    for(i = MarkerIndex; i < NrSegmentMarker - 1; i++)
      memcpy(&SegmentMarker[i], &SegmentMarker[i + 1], sizeof(tSegmentMarker));

//    memset(&SegmentMarker[NrSegmentMarker - 1], 0, sizeof(tSegmentMarker));
    NrSegmentMarker--;

    SegmentMarker[NrSegmentMarker - 1].Selected = FALSE;        // the very last marker (no segment)
//    if(NrSegmentMarker <= 2) SegmentMarker[0].Selected = FALSE;  // there is only one segment (from start to end)
    if(ActiveSegment >= MarkerIndex) ActiveSegment--;
    if(ActiveSegment >= NrSegmentMarker - 1) ActiveSegment = NrSegmentMarker - 2;
    JumpRequestedSegment = 0xFFFF;
    ret = TRUE;
  }

  TRACEEXIT();
  return ret;
}

void DeleteAllSegmentMarkers(void)
{
  TRACEENTER();

  if (NrSegmentMarker > 2)
  {
    UndoResetStack();
    memcpy(&SegmentMarker[1], &SegmentMarker[NrSegmentMarker-1], sizeof(tSegmentMarker));
//    memset(&SegmentMarker[2], 0, (NrSegmentMarker-2) * sizeof(tSegmentMarker));
    NrSegmentMarker = 2;
  }
  SegmentMarker[0].Selected = FALSE;
  ActiveSegment = 0;
  JumpRequestedSegment = 0xFFFF;

  TRACEEXIT();
}

bool SelectSegmentMarker(void)
{
  int                   VisibleSegment;
  bool                  ret = FALSE;
  TRACEENTER();

  if (NrSegmentMarker >= 2)
  {
    if (JumpRequestedSegment != 0xFFFF)
      VisibleSegment = JumpRequestedSegment;
    else if (JumpRequestedBlock != (dword) -1)
      VisibleSegment = FindSegmentWithBlock(JumpRequestedBlock);
    else
      VisibleSegment = ActiveSegment;

    SegmentMarker[VisibleSegment].Selected = !SegmentMarker[VisibleSegment].Selected;
    ret = SegmentMarker[VisibleSegment].Selected;
  }
  TRACEEXIT();
  return ret;
}


// ----------------------------------------------------------------------------
//                           Bookmark-Funktionen
// ----------------------------------------------------------------------------
void ExportSegmentsToBookmarks(void)
{
  int i;

  TRACEENTER();

  if (NrSegmentMarker > 2)
  {
    // first, delete all present bookmarks
    UndoResetStack();
    DeleteAllBookmarks();
    NrBookmarks = 0;

    // second, add a bookmark for each SegmentMarker
    WriteLogMCf(PROGRAM_NAME, "Exporting %d of %d segment markers", min(NrSegmentMarker-2, NRBOOKMARKS), NrSegmentMarker-2);

    for(i = 1; i <= min(NrSegmentMarker-2, NRBOOKMARKS); i++)
    { 
      Bookmarks[NrBookmarks] = SegmentMarker[i].Block;
      NrBookmarks++;
    }

    if (!SaveBookmarks(Bookmarks, NrBookmarks))
    {
      WriteLogMC(PROGRAM_NAME, "ExportSegmentsToBookmarks: SaveBookmarks() failed!");
      if (!ReadBookmarks(Bookmarks, &NrBookmarks))
        WriteLogMC(PROGRAM_NAME, "ExportSegmentsToBookmarks: -> ReadBookmarks() failed!");
    }
    OSDInfoDrawProgressbar(TRUE, TRUE);
//    OSDRedrawEverything();
  }
  TRACEEXIT();
}

int AddBookmark(dword newBlock, bool RejectSmallScenes)
{
  int ret = -1;
  int i, j;

  TRACEENTER();

  if(newBlock > PlayInfo.totalBlock  /*!PLAYINFOVALID() || ((int)newBlock < 0)*/)   // PlayInfo ist sicher, da in Z.804 überprüft, Prüfung für newBlock nun restriktiver
  {
    WriteLogMC(PROGRAM_NAME, "AddBookmark: newBlock > totalBlock - darf nicht auftreten!");
    TRACEEXIT();
    return -1;
  }

  if(NrBookmarks >= NRBOOKMARKS)
  {
    WriteLogMC(PROGRAM_NAME, "AddBookmark: Bookmark list is full!");
    TRACEEXIT();
    return -1;
  }

  //Find the point where to insert the new marker so that the list stays sorted
  if (NrBookmarks == 0)
  {
    Bookmarks[0] = newBlock;
    ret = 0;
  }
  else if (newBlock > Bookmarks[NrBookmarks - 1])
  {
    // Erlaube keinen Abschnitt mit weniger als 3 Sekunden
    if (RejectSmallScenes && (newBlock <= Bookmarks[NrBookmarks-1] + 3*BlocksOneSecond))
    {
      TRACEEXIT();
      return -1;
    }
    Bookmarks[NrBookmarks] = newBlock;
    ret = NrBookmarks;
  }
  else
  {
    for(i = 0; i < NrBookmarks; i++)
    {
      if(Bookmarks[i] >= newBlock)
      {
        if(Bookmarks[i] == newBlock)
        {
          TRACEEXIT();
          return -1;
        }

        // Erlaube keinen Abschnitt mit weniger als 3 Sekunden
        if (RejectSmallScenes && ((i > 0 && (newBlock <= Bookmarks[i-1] + 3*BlocksOneSecond)) || (newBlock + 3*BlocksOneSecond >= Bookmarks[i])))
        {
          TRACEEXIT();
          return -1;
        }

        for(j = NrBookmarks; j > i; j--)
          Bookmarks[j] = Bookmarks[j - 1];

        Bookmarks[i] = newBlock;
        ret = i;
        break;
      }
    }
  }
  NrBookmarks++;
  if (!SaveBookmarks(Bookmarks, NrBookmarks))
  {
    ret = -1;
    WriteLogMC(PROGRAM_NAME, "AddBookmark: SaveBookmarks() failed!");
    if (!ReadBookmarks(Bookmarks, &NrBookmarks))
      WriteLogMC(PROGRAM_NAME, "AddBookmark: -> ReadBookmarks() failed!");
  }

  TRACEEXIT();
  return ret;
}

int FindNearestBookmark(dword curBlock)
{
  int                   NearestBookmarkIndex;
  long                  MinDelta;
  int                   i;

  TRACEENTER();

  NearestBookmarkIndex = -1;
  if(NrBookmarks > 0)
  {
    MinDelta = 0x7fffffff;
    for(i = 0; i < NrBookmarks; i++)
    {
      if(labs(Bookmarks[i] - curBlock) < MinDelta)
      {
        MinDelta = labs(Bookmarks[i] - curBlock);
        NearestBookmarkIndex = i;
      }
    }
  }

  TRACEEXIT();
  return NearestBookmarkIndex;
}

bool MoveBookmark(int BookmarkIndex, dword newBlock, bool RejectSmallScenes)
{
  bool ret = FALSE;
  TRACEENTER();

  if(newBlock > PlayInfo.totalBlock  /*!PLAYINFOVALID() || ((int)newBlock < 0)*/)   // PlayInfo ist sicher, da in Z.804 überprüft, Prüfung für newBlock nun restriktiver
  {
    WriteLogMC(PROGRAM_NAME, "MoveBookmark: newBlock > totalBlock - darf nicht auftreten!");
    TRACEEXIT();
    return FALSE;
  }

  if ((BookmarkIndex >= 0) && (BookmarkIndex < NrBookmarks))
  {
    if (Bookmarks[BookmarkIndex] == newBlock)
    {
      TRACEEXIT();
      return FALSE;
    }
    
    // Erlaube keinen Abschnitt mit weniger als 3 Sekunden
    if (RejectSmallScenes && ((BookmarkIndex > 0 && (newBlock <= Bookmarks[BookmarkIndex-1] + 3*BlocksOneSecond)) || (BookmarkIndex < NrBookmarks-1 && (newBlock + 3*BlocksOneSecond >= Bookmarks[BookmarkIndex+1]))))
    {
      TRACEEXIT();
      return FALSE;
    }
    Bookmarks[BookmarkIndex] = newBlock;

    ret = SaveBookmarks(Bookmarks, NrBookmarks);
    if (!ret)
    {
      WriteLogMC(PROGRAM_NAME, "MoveBookmark: SaveBookmarks() failed!");
      if (!ReadBookmarks(Bookmarks, &NrBookmarks))
        WriteLogMC(PROGRAM_NAME, "MoveBookmark: -> ReadBookmarks() failed!");
    }
  }

  TRACEEXIT();
  return ret;
}

bool DeleteBookmark(int BookmarkIndex)
{
  bool ret = FALSE;
  int i;

  TRACEENTER();

  if ((BookmarkIndex >= 0) && (BookmarkIndex < NrBookmarks))
  {
    for(i = BookmarkIndex; i < NrBookmarks - 1; i++)
      Bookmarks[i] = Bookmarks[i + 1];
    Bookmarks[NrBookmarks - 1] = 0;
    NrBookmarks--;

    ret = SaveBookmarks(Bookmarks, NrBookmarks);
    if (!ret)
    {
      WriteLogMC(PROGRAM_NAME, "DeleteBookmark: SaveBookmarks() failed!");
      if (!ReadBookmarks(Bookmarks, &NrBookmarks))
        WriteLogMC(PROGRAM_NAME, "DeleteBookmark: -> ReadBookmarks() failed!");
    }
  }

  TRACEEXIT();
  return ret;
}

bool DeleteAllBookmarks(void)
{
  bool ret = FALSE;
  int i;
  TRACEENTER();

  if (NrBookmarks > 0)
  {
    UndoResetStack();
    for(i = 0; i < NrBookmarks; i++)
      Bookmarks[i] = 0;
  }
  NrBookmarks = 0;

  ret = SaveBookmarks(Bookmarks, NrBookmarks);
  if (!ret)
    WriteLogMC(PROGRAM_NAME, "SaveBookmarks: Fatal error - inf cache entry point not found!");

  TRACEEXIT();
  return ret;
}

// ----------------------------------------------------------------------------
//                          Rückgängig-Funktionen
// ----------------------------------------------------------------------------
void UndoAddEvent(bool Bookmark, dword PreviousBlock, dword NewBlock, bool SegmentWasSelected)
{
  tUndoEvent* NextAction;
  TRACEENTER();

  if (!(Bookmark == FALSE && PreviousBlock == 0 && NewBlock == 0))
  {
    UndoLastItem++;
    if (UndoLastItem >= NRUNDOEVENTS)
      UndoLastItem = 0;
    NextAction = &UndoStack[UndoLastItem];
    TAP_PrintNet("MovieCutter: UndoAddEvent %d (%s, PreviousBlock=%lu, NewBlock=%lu)\n", UndoLastItem, (Bookmark) ? "Bookmark" : "SegmentMarker", PreviousBlock, NewBlock);

    NextAction->Bookmark           = Bookmark;
    NextAction->PrevBlockNr        = PreviousBlock;
    NextAction->NewBlockNr         = NewBlock;
    NextAction->SegmentWasSelected = SegmentWasSelected;
  }
  TRACEEXIT();
}

bool UndoLastAction(void)
{
  tUndoEvent* LastAction;
  int i;

  TRACEENTER();

  if (UndoLastItem >= NRUNDOEVENTS)
  {
    TRACEEXIT();
    return FALSE;
  }
  LastAction = &UndoStack[UndoLastItem];
  TAP_PrintNet("MovieCutter: UndoLastAction %d (%s, PreviousBlock=%lu, NewBlock=%lu)\n", UndoLastItem, (LastAction->Bookmark) ? "Bookmark" : "SegmentMarker", LastAction->PrevBlockNr, LastAction->NewBlockNr);

  if (LastAction->Bookmark)
  {
    for(i = 0; i < NrBookmarks; i++)
      if(Bookmarks[i] == LastAction->NewBlockNr) break;
    if((i < NrBookmarks) && (Bookmarks[i] == LastAction->NewBlockNr))
    {
      if (LastAction->PrevBlockNr != 0)
        MoveBookmark(i, LastAction->PrevBlockNr, FALSE);
      else
        DeleteBookmark(i);
    }
    else if (LastAction->NewBlockNr == 0)
      AddBookmark(LastAction->PrevBlockNr, FALSE);
  }
  else
  {
    if (LastAction->NewBlockNr != 0)
    {
      for(i = 1; i < NrSegmentMarker - 1; i++)
        if(SegmentMarker[i].Block == LastAction->NewBlockNr) break;
      if((i < NrSegmentMarker - 1) && (SegmentMarker[i].Block == LastAction->NewBlockNr))
      {
        if (LastAction->PrevBlockNr != 0)
          MoveSegmentMarker(i, LastAction->PrevBlockNr, FALSE);
        else
          DeleteSegmentMarker(i);
      }
    }
    else if (LastAction->PrevBlockNr != 0)
    {
      i = AddSegmentMarker(LastAction->PrevBlockNr, FALSE);
      if (i >= 0)
        SegmentMarker[i].Selected = LastAction->SegmentWasSelected;
    }
    else if (UndoLastItem == 0)
    {
      TRACEEXIT();
      return FALSE;
    }
  }

  LastAction->Bookmark = FALSE;
  LastAction->PrevBlockNr = 0;
  LastAction->NewBlockNr = 0;

  if(UndoLastItem > 0)
    UndoLastItem--;
  else
    UndoLastItem = NRUNDOEVENTS - 1;

  TRACEEXIT();
  return TRUE;
}

void UndoResetStack(void)
{
  TRACEENTER();
  TAP_PrintNet("MovieCutter: UndoResetStack()\n");

  memset(UndoStack, 0, NRUNDOEVENTS * sizeof(tUndoEvent));
  UndoLastItem = 0;

  TRACEEXIT();
}

// ----------------------------------------------------------------------------
//                           CutFile-Funktionen
// ----------------------------------------------------------------------------
bool CutFileLoad(void)
{
  char                  AbsCutName[FBLIB_DIR_SIZE];
  char                  LogString[512];
  word                  Version = 0;
  word                  Padding;
  FILE                 *fCut = NULL;
  __off64_t             SavedSize;
  dword                 Offsetms;
  int                   i;
  tTimeStamp           *CurTimeStamp;

  TRACEENTER();

  // Create name of cut-file
  TAP_SPrint(AbsCutName, sizeof(AbsCutName), "%s/%s", AbsPlaybackDir, PlaybackName);
  TAP_SPrint(&AbsCutName[strlen(AbsCutName) - 4], 5, ".cut");

  // Try to open cut-File
//  HDD_ChangeDir(PlaybackDir);
  fCut = fopen(AbsCutName, "rb");
  if(!fCut)
  {
    WriteLogMC(PROGRAM_NAME, "CutFileLoad: failed to open .cut!");
    TRACEEXIT();
    return FALSE;
  }

  // Check correct version of cut-file
  fread(&Version, sizeof(byte), 1, fCut);    // read only one byte for compatibility with V.1  [COMPATIBILITY LAYER]
  if(Version > CUTFILEVERSION)
  {
    WriteLogMC(PROGRAM_NAME, "CutFileLoad: .cut version mismatch!");
    fclose(fCut);
    TRACEEXIT();
    return FALSE;
  }
  if (Version > 1)
    fread(&Padding, sizeof(byte), 1, fCut);  // read the second byte of Version (if not V.1)  [COMPATIBILITY LAYER]

  // Check, if size of rec-File has been changed
  fread(&SavedSize, sizeof(__off64_t), 1, fCut);
  if(RecFileSize != SavedSize)
  {
    WriteLogMC(PROGRAM_NAME, "CutFileLoad: .cut file size mismatch!");
/*    fclose(fCut);
    TRACEEXIT();
    return FALSE; */
  }

  // Read data from .cut-File
  NrSegmentMarker = 0;
  ActiveSegment = 0;
  fread(&NrSegmentMarker, sizeof(word), 1, fCut);
  if (NrSegmentMarker > NRSEGMENTMARKER) NrSegmentMarker = NRSEGMENTMARKER;
  if (Version == 1)
    fread(&Padding, sizeof(word), 1, fCut);  // read the second word of NrSegmentMarker (if V.1)  [COMPATIBILITY LAYER]
  fread(&ActiveSegment, sizeof(word), 1, fCut);
  fread(&Padding, sizeof(word), 1, fCut);
  fread(SegmentMarker, sizeof(tSegmentMarker), NrSegmentMarker, fCut);
  fclose(fCut);

  // Sonderfunktion: Import von Cut-Files mit unpassender Aufnahme-Größe
  if (RecFileSize != SavedSize)
  {
    if ((NrSegmentMarker > 2) && (TimeStamps != NULL))
    {
      char curTimeStr[16];
      WriteLogMC(PROGRAM_NAME, "CutFileLoad: Importing timestamps only, recalculating block numbers..."); 

      SegmentMarker[0].Block = 0;
      SegmentMarker[0].Timems = NavGetBlockTimeStamp(0);
//      SegmentMarker[0].Selected = FALSE;
      SegmentMarker[NrSegmentMarker - 1].Block = 0xFFFFFFFF;

      Offsetms = 0;
      CurTimeStamp = TimeStamps;
      for (i = 1; i <= NrSegmentMarker-2; i++)
      {
        if (Version == 1)
          SegmentMarker[i].Timems = SegmentMarker[i].Timems * 1000;

        // Wenn ein Bookmark gesetzt ist, dann verschiebe die SegmentMarker so, dass der erste auf dem Bookmark steht
        if (i == 1 && NrBookmarks > 0)
        {
          Offsetms = NavGetBlockTimeStamp(Bookmarks[0]) - SegmentMarker[1].Timems;
          MSecToTimeString(SegmentMarker[i].Timems + Offsetms, curTimeStr);
          WriteLogMCf(PROGRAM_NAME, "Bookmark found! - First segment marker will be moved to time %s. (Offset=%d ms)", curTimeStr, (int)Offsetms);
        }

        MSecToTimeString(SegmentMarker[i].Timems, curTimeStr);
        TAP_SPrint(LogString, sizeof(LogString), "%2u.)  oldTimeStamp=%s   oldBlock=%lu", i, curTimeStr, SegmentMarker[i].Block);
        if (Offsetms > 0)
        {
          SegmentMarker[i].Timems += Offsetms;
          MSecToTimeString(SegmentMarker[i].Timems, curTimeStr);
          TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), "  -  movedTimeStamp=%s", curTimeStr);
        }

        if ((SegmentMarker[i].Timems <= CurTimeStamp->Timems) || (CurTimeStamp >= TimeStamps + NrTimeStamps-1))
        {
          if (DeleteSegmentMarker(i)) i--;
          TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), "  -->  Smaller than previous TimeStamp or end of nav reached. Deleted!");
        }
        else
        {
          while ((CurTimeStamp->Timems < SegmentMarker[i].Timems) && (CurTimeStamp < TimeStamps + NrTimeStamps-1))
            CurTimeStamp++;
          if (CurTimeStamp->Timems > SegmentMarker[i].Timems)
            CurTimeStamp--;
        
          if (CurTimeStamp->BlockNr < PlayInfo.totalBlock)
          {
            SegmentMarker[i].Block = CurTimeStamp->BlockNr;
            SegmentMarker[i].Timems = NavGetBlockTimeStamp(SegmentMarker[i].Block);
//            SegmentMarker[i].Selected = FALSE;
            MSecToTimeString(SegmentMarker[i].Timems, curTimeStr);
            TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), "  -->  newBlock=%lu   newTimeStamp=%s", SegmentMarker[i].Block, curTimeStr);
          }
          else
          {
            if (DeleteSegmentMarker(i)) i--;
            TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), "  -->  TotalBlocks exceeded. Deleted!");
          }
        }
        WriteLogMC(PROGRAM_NAME, LogString);
      }
    }
    else
    {
      WriteLogMC(PROGRAM_NAME, "CutFileLoad: Two or less timestamps imported, or NavTimeStamps=NULL!"); 
      NrSegmentMarker = 0;
    }
  }

  // Prozent-Angaben neu berechnen (müssen künftig nicht mehr in der .cut gespeichert werden)
  for (i = NrSegmentMarker-1; i >= 0; i--)
  {
    if ((i < NrSegmentMarker-1) && (SegmentMarker[i].Block >= PlayInfo.totalBlock))
    {
      WriteLogMCf(PROGRAM_NAME, "SegmentMarker %d (%lu): TotalBlocks exceeded. -> Deleting!", i, SegmentMarker[i].Block);
      DeleteSegmentMarker(i);
    }
    else
      SegmentMarker[i].Percent = ((float)SegmentMarker[i].Block / PlayInfo.totalBlock) * 100.0;
  }

  // Wenn zu wenig Segmente -> auf Standard zurücksetzen
  if (NrSegmentMarker <= 2)
  {
    NrSegmentMarker = 0;
    ActiveSegment = 0;
    WriteLogMC(PROGRAM_NAME, "CutFileLoad: Two or less timestamps imported, resetting!"); 
    TRACEEXIT();
    return FALSE;
  }

  // Wenn letzter Segment-Marker ungleich TotalBlock ist -> anpassen
  if (SegmentMarker[NrSegmentMarker - 1].Block != PlayInfo.totalBlock)
  {
#ifdef FULLDEBUG
  WriteLogMCf(PROGRAM_NAME, "CutFileLoad: Letzter Segment-Marker %lu ist ungleich TotalBlock %lu!", SegmentMarker[NrSegmentMarker - 1].Block, PlayInfo.totalBlock);
#endif
    SegmentMarker[NrSegmentMarker - 1].Block = PlayInfo.totalBlock;
    dword newTime = NavGetBlockTimeStamp(SegmentMarker[NrSegmentMarker - 1].Block);
    SegmentMarker[NrSegmentMarker - 1].Timems = (newTime) ? newTime : (dword)(1000 * (60*PlayInfo.duration + PlayInfo.durationSec));
    SegmentMarker[NrSegmentMarker - 1].Percent = 100;
  }

  // Markierungen und ActiveSegment prüfen, ggf. korrigieren
  SegmentMarker[NrSegmentMarker - 1].Selected = FALSE;         // the very last marker (no segment)
  if(NrSegmentMarker <= 2) SegmentMarker[0].Selected = FALSE;  // there is only one segment (from start to end)
  if(ActiveSegment >= NrSegmentMarker - 1) ActiveSegment = NrSegmentMarker - 2;

  // Wenn cut-File Version 1 hat, dann ermittle neue TimeStamps und vergleiche diese mit den alten (DEBUG)  [COMPATIBILITY LAYER]
  if (!LinearTimeMode && (Version == 1) && (RecFileSize == SavedSize))
  {
    int i;
    dword oldTime, newTime;
    char oldTimeStr[12], newTimeStr[16];

    WriteLogMC(PROGRAM_NAME, "Import of old cut file version (compare old and new TimeStamps!)");
    for (i = 0; i < NrSegmentMarker; i++)
    {
      oldTime = SegmentMarker[i].Timems;
      newTime = NavGetBlockTimeStamp(SegmentMarker[i].Block);
      SegmentMarker[i].Timems = (newTime) ? newTime : oldTime;
      SecToTimeString(oldTime, oldTimeStr);
      MSecToTimeString(newTime, newTimeStr);
      WriteLogMCf(PROGRAM_NAME, " %s%2d.)  BlockNr=%8lu   oldTimeStamp=%s   newTimeStamp=%s", (labs(oldTime*1000-newTime) > 1000) ? "!!" : "  ", i+1, SegmentMarker[i].Block, oldTimeStr, newTimeStr);
    }
  }

  TRACEEXIT();
  return TRUE;
}

void CutFileSave(void)
{
  char                  AbsCutName[FBLIB_DIR_SIZE], *CutName = NULL;
  word                  Version;
  FILE                 *fCut = NULL;

  TRACEENTER();

/*  //Save the file size to check if the file didn't change
  fRec = TAP_Hdd_Fopen(PlaybackName);
  if(!fRec)
  {
    TRACEEXIT();
    return;
  }

  FileSize = fRec->size;
  TAP_Hdd_Fclose(fRec); */

//  RecFileSize = HDD_GetFileSize(PlaybackName);
//  if(RecFileSize <= 0)

  Version = CUTFILEVERSION;
  TAP_SPrint(AbsCutName, sizeof(AbsCutName), "%s/%s", AbsPlaybackDir, PlaybackName);
  TAP_SPrint(&AbsCutName[strlen(AbsCutName) - 4], 5, ".cut");
  CutName = &AbsCutName[strlen(AbsPlaybackDir) + 1];

#ifdef FULLDEBUG
  char CurDir[FBLIB_DIR_SIZE];
  HDD_TAP_GetCurrentDir(CurDir);
  WriteLogMCf(PROGRAM_NAME, "CutFileSave()! CurrentDir: '%s', PlaybackName: '%s', CutFileName: '%s'", CurDir, PlaybackName, CutName);
#endif
//  HDD_ChangeDir(PlaybackDir);

  if(!HDD_GetFileSizeAndInode2(PlaybackName, AbsPlaybackDir, NULL, &RecFileSize))
  {
    WriteLogMC(PROGRAM_NAME, "CutFileSave: Could not detect size of recording!"); 
    TRACEEXIT();
    return;
  }

  if(NrSegmentMarker <= 2)
  {
    HDD_Delete2(CutName, AbsPlaybackDir, FALSE);
    TRACEEXIT();
    return;
  }

  fCut = fopen(AbsCutName, "wb");
  if(!fCut)
  {
    WriteLogMC(PROGRAM_NAME, "CutFileSave: failed to open .cut!"); 
    TRACEEXIT();
    return;
  }

  SegmentMarker[NrSegmentMarker - 1].Selected = FALSE;
  if(NrSegmentMarker <= 2) SegmentMarker[0].Selected = FALSE;

  fwrite(&Version, sizeof(word), 1, fCut);
  fwrite(&RecFileSize, sizeof(__off64_t), 1, fCut);
  fwrite(&NrSegmentMarker, sizeof(word), 1, fCut);
  fwrite(&ActiveSegment, sizeof(word), 1, fCut);
word Padding=0;
  fwrite(&Padding, sizeof(word), 1, fCut);
  fwrite(SegmentMarker, sizeof(tSegmentMarker), NrSegmentMarker, fCut);
  fclose(fCut);

  TRACEEXIT();
}

void CutFileDelete(void)
{
  char                  CutName[MAX_FILE_NAME_SIZE + 1];

  TRACEENTER();

//  HDD_ChangeDir(PlaybackDir);
  strncpy(CutName, PlaybackName, sizeof(CutName));
  CutName[MAX_FILE_NAME_SIZE] = '\0';
  CutName[strlen(CutName) - 4] = '\0';
  strcat(CutName, ".cut");
  HDD_Delete2(CutName, AbsPlaybackDir, FALSE);

  TRACEEXIT();
}

void CutDumpList(void)
{
  char                  TimeString[16];
  int                   i;

  TRACEENTER();

  WriteLogMC(PROGRAM_NAME, "Seg      Block Time        Pct Sel Act");
  for(i = 0; i < NrSegmentMarker; i++)
  {
    MSecToTimeString(SegmentMarker[i].Timems, TimeString);
    WriteLogMCf(PROGRAM_NAME, "%02d: %010d %s %03d %3s %3s", i, (int)SegmentMarker[i].Block, TimeString, (int)SegmentMarker[i].Percent, SegmentMarker[i].Selected ? "yes" : "no", (i == ActiveSegment ? "*" : ""));
  }

  TRACEEXIT();
}


// ----------------------------------------------------------------------------
//                              OSD-Funktionen
// ----------------------------------------------------------------------------
void CreateOSD(void)
{
  TRACEENTER();

  if(!rgnInfoBar && !rgnInfoBarMini && OSDMode != MD_NoOSD)  // Workaround: Wenn Play-Leiste eingeblendet ist (SysSubState == SUBSTATE_PvrPlayingSearch) wird diese sonst nicht ausgeblendet
  {
//    TAP_ExitNormal();
    TAP_EnterNormalNoInfo();
    TAP_ExitNormal();
  }

  if ((OSDMode != MD_MiniOSD) && (OSDMode != MD_NoOSD))
  {
    if(rgnPlayState)
    {
      TAP_Osd_Delete(rgnPlayState);
      rgnPlayState = 0;
    }
    if(rgnTextState)
    {
      TAP_Osd_Delete(rgnTextState);
      rgnTextState = 0;
    }
  }

  if (OSDMode != MD_MiniOSD)
  {
    if(rgnInfoBarMini)
    {
      TAP_Osd_Delete(rgnInfoBarMini);
      rgnInfoBarMini = 0;
    }
  }

  if ((OSDMode != MD_FullOSD) && (OSDMode != MD_NoSegmentList))
  {
    if(rgnInfoBar)
    {
      TAP_Osd_Delete(rgnInfoBar);
      rgnInfoBar = 0;
    }
  }

  if (OSDMode != MD_FullOSD)
  {
    if(rgnSegmentList)
    {
      TAP_Osd_Delete(rgnSegmentList);
      rgnSegmentList = 0;
    }
  }

  if (OSDMode == MD_FullOSD)
    if(!rgnSegmentList)
      rgnSegmentList = TAP_Osd_Create(SegmentList_X, SegmentList_Y, _SegmentList_Background_Gd.width, _SegmentList_Background_Gd.height, 0, 0);

  if ((OSDMode == MD_FullOSD) || (OSDMode == MD_NoSegmentList))
    if(!rgnInfoBar)
      rgnInfoBar = TAP_Osd_Create(0, ScreenHeight - 160, ScreenWidth, 160, 0, 0);

  if (OSDMode == MD_MiniOSD)
    if(!rgnInfoBarMini)
      rgnInfoBarMini = TAP_Osd_Create(Overscan_X, ScreenHeight - Overscan_Y - 38 - 32, ScreenWidth - 2*Overscan_X, 38, 0, 0);

  TRACEEXIT();
}

void OSDRedrawEverything(void)
{
  TRACEENTER();

  CreateOSD();
  OSDInfoDrawBackground();
  OSDInfoDrawRecName();
  SetCurrentSegment();
  OSDSegmentListDrawList(FALSE);
  OSDInfoDrawProgressbar(TRUE, FALSE);
  OSDInfoDrawPlayIcons(TRUE, FALSE);
  OSDInfoDrawMinuteJump(FALSE);
  OSDInfoDrawBookmarkMode(FALSE);
  OSDInfoDrawCurrentPlayTime(TRUE);
  OSDInfoDrawClock(TRUE);
  TAP_Osd_Sync();

  TRACEEXIT();
}

// Segment-Liste
// -------------
void OSDSegmentListDrawList(bool DoSync)
{
  const int             RegionWidth        = _SegmentList_Background_Gd.width;
  const int             TextFieldStart_X   =    5,   EndTextField_X  = 148;
  const int             TextFieldStart_Y   =   29,   TextFieldHeight =  26,   TextFieldDist = 2;
  const int             Scrollbar_X        =  150,   Scrollbar_Y     =  40,  /*ScrollbarWidth = 10,*/    ScrollbarHeight = 256;
  const int             BelowTextArea_Y    =  307;
  const int             NrWidth     =  FMUC_GetStringWidth("99.", &Calibri_12_FontDataUC);
  const int             DashWidth   =  FMUC_GetStringWidth(" - ", &Calibri_12_FontDataUC);
  const int             TimeWidth   =  FMUC_GetStringWidth("99:99:99", &Calibri_12_FontDataUC);

  int                   CurrentSegment;
  int                   ScrollButtonHeight, ScrollButtonPos;
  char                  StartTime[12], EndTime[12], OutStr[5];
  char                  PageNrStr[8], *PageStr;
  int                   p, NrPages;
  int                   Start;
  dword                 PosX, PosY, UseColor;
  int                   i;

  TRACEENTER();

  if(rgnSegmentList)
  {
    if (JumpRequestedSegment != 0xFFFF)
      CurrentSegment = JumpRequestedSegment;
    else
      CurrentSegment = ActiveSegment;
    if(CurrentSegment >= NrSegmentMarker - 1)
      CurrentSegment = NrSegmentMarker - 2;

    // Hintergrund, Überschrift, Buttons
    TAP_Osd_PutGd(rgnSegmentList, 0, 0, &_SegmentList_Background_Gd, FALSE);
    FMUC_PutString(rgnSegmentList, 2, 2, RegionWidth-3, LangGetString(LS_SegmentList), COLOR_White, ColorLightBackground, &Calibri_14_FontDataUC, TRUE, ALIGN_CENTER);
    TAP_Osd_PutGd(rgnSegmentList, EndTextField_X - 2*19 - 3, BelowTextArea_Y + 6, &_Button_Up_small_Gd, TRUE);
    TAP_Osd_PutGd(rgnSegmentList, EndTextField_X - 19,       BelowTextArea_Y + 6, &_Button_Down_small_Gd, TRUE);

    // Seitenzahl
    NrPages = ((NrSegmentMarker - 2) / 10) + 1;
    p       = (CurrentSegment / 10) + 1;
    PageStr = LangGetString(LS_PageStr);
    TAP_SPrint(PageNrStr, sizeof(PageNrStr), "%d/%d", p, NrPages);

    PosX = TextFieldStart_X + 11;
    PosY = BelowTextArea_Y + 3;
    FMUC_PutString(rgnSegmentList, PosX, PosY, EndTextField_X - 2*19 - 3, PageStr,   COLOR_White, ColorLightBackground, &Calibri_10_FontDataUC, FALSE, ALIGN_LEFT);
    PosX += FMUC_GetStringWidth(PageStr, &Calibri_10_FontDataUC) + 5;

    FMUC_PutString(rgnSegmentList, PosX, PosY, EndTextField_X - 2*19 - 3, PageNrStr, COLOR_White, ColorLightBackground, &Calibri_10_FontDataUC, FALSE, ALIGN_LEFT);

    // Segmente
    if(NrSegmentMarker > 2)
    {
//      if(CurrentSegment >= 10)                          TAP_Osd_PutGd(rgnSegmentList, 62, 23, &_Button_Up2_Gd, TRUE);
//      if((CurrentSegment < 10) && (NrSegmentMarker>11)) TAP_Osd_PutGd(rgnSegmentList, 88, 23, &_Button_Down2_Gd, TRUE);

      Start = (CurrentSegment / 10) * 10;
      for(i = 0; i < min(10, (NrSegmentMarker - Start) - 1); i++)
      {
        if((Start + i) == CurrentSegment)
        {
//          if((SegmentMarker[p+i + 1].Timems - SegmentMarker[p+i].Timems) < 60001)
//            TAP_Osd_PutGd(rgnSegmentList, TextFieldStart_X, TextFieldStart_Y + i*(TextFieldHeight+TextFieldDist), &&_Selection_Red_Gd, FALSE);
//          else
          TAP_Osd_PutGd(rgnSegmentList, TextFieldStart_X, TextFieldStart_Y + i*(TextFieldHeight+TextFieldDist), &_Selection_Blue_Gd, FALSE);
        }

        SecToTimeString((SegmentMarker[Start + i].Timems) / 1000, StartTime);
        SecToTimeString((SegmentMarker[Start + i + 1].Timems) / 1000, EndTime);

        PosX = TextFieldStart_X;
        PosY = TextFieldStart_Y + i*(TextFieldHeight+TextFieldDist) + 3;
        UseColor = (SegmentMarker[Start + i].Selected) ? COLOR_Yellow : COLOR_White;
        if( ((Start+i < NrSegmentMarker-2 || DisableSpecialEnd)   && (SegmentMarker[Start+i + 1].Block - SegmentMarker[Start+i].Block + 22 > 475949))
         || (!DisableSpecialEnd && (Start+i == NrSegmentMarker-2) && (SegmentMarker[Start+i].Block + 22 > 475949)) )
          if (SegmentMarker[Start + i].Selected)
            UseColor = RGB(250, 139, 18);
        TAP_SPrint(OutStr, sizeof(OutStr), "%d.", Start + i + 1);
        if (Start + i + 1 >= 100) TAP_SPrint(OutStr, sizeof(OutStr), "00.");
        FMUC_PutString(rgnSegmentList, PosX, PosY, PosX + NrWidth,    OutStr,                                                               UseColor, COLOR_None, &Calibri_12_FontDataUC, FALSE, ALIGN_RIGHT);
        PosX += NrWidth;
        FMUC_PutString(rgnSegmentList, PosX, PosY, PosX + TimeWidth,  (Start+i == 0) ? LangGetString(LS_BeginStr) : StartTime,              UseColor, COLOR_None, &Calibri_12_FontDataUC, FALSE, ALIGN_RIGHT);
        PosX += TimeWidth;
        FMUC_PutString(rgnSegmentList, PosX, PosY, PosX + DashWidth,  "-",                                                                  UseColor, COLOR_None, &Calibri_12_FontDataUC, FALSE, ALIGN_CENTER);
        PosX += DashWidth;
        FMUC_PutString(rgnSegmentList, PosX, PosY, EndTextField_X+10, (Start+i == NrSegmentMarker-2) ? LangGetString(LS_EndStr) : EndTime,  UseColor, COLOR_None, &Calibri_12_FontDataUC, FALSE, ALIGN_LEFT);
      }
    }

    // Scrollbalken
    if (NrPages > 1)
    {
      TAP_Osd_PutGd(rgnSegmentList, EndTextField_X, TextFieldStart_Y, &_SegmentList_ScrollBar_Gd, FALSE);
      ScrollButtonHeight = (ScrollbarHeight / NrPages);
      ScrollButtonPos    = Scrollbar_Y + (ScrollbarHeight * (p - 1) / NrPages);
//      TAP_Osd_Draw3dBoxFill(rgnSegmentList, Scrollbar_X+2, ScrollButtonPos, ScrollbarWidth-4, ScrollButtonHeight, COLOR_Yellow, COLOR_Red, COLOR_DarkGray);
      for (i = 0; i < ScrollButtonHeight; i++)
        TAP_Osd_PutGd(rgnSegmentList, Scrollbar_X, ScrollButtonPos + i, &_SegmentList_ScrollButton_Gd, FALSE);
    }

    if(DoSync) TAP_Osd_Sync();
  }
  TRACEEXIT();
}

// SetCurrentSegment
// -----------------
void SetCurrentSegment(void)
{
  bool                  DoDraw = FALSE;
  int                   VisibleSegment = -1;

  TRACEENTER();

  if (!PLAYINFOVALID())
  {
    TRACEEXIT();
    return;
  }

  if (((JumpRequestedBlock != (dword) -1)) && !JumpRequestedTime)
  {
    VisibleSegment = FindSegmentWithBlock(JumpRequestedBlock);
    if (ActiveSegment != VisibleSegment)
      DoDraw = TRUE;
    ActiveSegment = VisibleSegment;
  }

  if (!JumpPerformedTime || (labs(TAP_GetTick() - JumpPerformedTime) >= 150))
  {
    if (JumpPerformedTime) DoDraw = TRUE;
    JumpPerformedTime = 0;

    if (!JumpRequestedTime)
    {
      JumpRequestedBlock = (dword) -1;
      JumpRequestedSegment = 0xFFFF;
    }

    if (VisibleSegment < 0)
    {
      if(NrSegmentMarker > 2)
      {
        VisibleSegment = FindSegmentWithBlock(PlayInfo.currentBlock);
        if(ActiveSegment != VisibleSegment)
          DoDraw = TRUE;
        ActiveSegment = VisibleSegment;
      }
      else
        ActiveSegment = 0;
    }
  }

  if(DoDraw)
  {
    OSDSegmentListDrawList(FALSE);
    OSDInfoDrawProgressbar(TRUE, FALSE);
  }
  TRACEEXIT();
}

// Fortschrittsbalken
// ------------------
void OSDInfoDrawProgressbar(bool Force, bool DoSync)
{
  const dword           ColorProgressBar      = RGB(250, 139,  18);
  const dword           ColorWarnedSegment    = RGB(255, 100,  60);
  const dword           ColorActiveSegment    = RGB( 73, 206, 239);
  const dword           ColorActiveSegWarning = RGB(180, 175, 235);
  const dword           ColorSegmentSelection = COLOR_Blue;
  const dword           ColorSelectionWarning = COLOR_DarkMagenta;
  const dword           ColorCurrentPos       = RGB(157,   8,  13);
  const dword           ColorCurrentPosMark   = RGB(255, 111, 114);
  dword                 UseSelectionColor;

  word                  OSDRegion = 0;
  int                   FrameWidth, FrameHeight, FrameLeft, FrameTop;
  int                   ProgBarWidth, ProgBarHeight, ProgBarLeft, ProgBarTop;

  static dword          LastDraw = 0;
  static dword          LastPos = 999;
  dword                 pos;
  dword                 curPos, curWidth;
  int                   NearestMarker;
  int                   i, j;

  TRACEENTER();

  if((labs(TAP_GetTick() - LastDraw) > 20) || Force)
  {
    if((int)PlayInfo.totalBlock <= 0)  // für die Anzeige der Progressbar reicht es, wenn totalBlock gesetzt ist, currentBlock wird später geprüft!
    {
      TRACEEXIT();
      return;
    }

    if(rgnInfoBar)
    {
      OSDRegion     = rgnInfoBar;
      FrameWidth    = ScreenWidth - 2*Overscan_X - 2;                      FrameHeight   = 32;
      FrameLeft     = Overscan_X + 1;                                      FrameTop      = InfoBarLine2_Y + 1;
      ProgBarWidth  = FrameWidth - 15;                                     ProgBarHeight = 10;
      ProgBarLeft   = FrameLeft + 10;                                      ProgBarTop    = FrameTop + (FrameHeight-ProgBarHeight)/2;
    }
    else if (rgnInfoBarMini)
    {
      OSDRegion     = rgnInfoBarMini;
      int TimeWidth = FMUC_GetStringWidth("9:99:99", &Calibri_12_FontDataUC) + 1;
      FrameWidth    = GetOSDRegionWidth(rgnInfoBarMini) - 4 - TimeWidth;   FrameHeight   = GetOSDRegionHeight(rgnInfoBarMini) - 6;
      FrameLeft     = 2;                                                   FrameTop      =  3;
      ProgBarWidth  = FrameWidth - 15;                                     ProgBarHeight = 10;
      ProgBarLeft   = FrameLeft + 10;                                      ProgBarTop    = FrameTop + (FrameHeight-ProgBarHeight)/2;
    }
    else
    {
      TRACEEXIT();
      return;
    }

//    if (!JumpRequestedTime && !JumpPerformedTime)
//      JumpRequestedBlock = (dword) -1;

    pos = (dword)((float)currentVisibleBlock() * ProgBarWidth / PlayInfo.totalBlock);
    if(Force || (pos != LastPos))
    {
      // Draw the background
      TAP_Osd_FillBox(OSDRegion, FrameLeft, FrameTop, ProgBarWidth + 15, FrameHeight, ColorLightBackground);
      TAP_Osd_FillBox(OSDRegion, ProgBarLeft, ProgBarTop, ProgBarWidth, ProgBarHeight, ColorProgressBar);
      for (i = 0; i <= 10; i++) {
        for (j = 1; j <= 4; j++) {
          TAP_Osd_PutPixel(OSDRegion, ProgBarLeft + ((i * (ProgBarWidth-1))/10), FrameTop + FrameHeight - j, COLOR_White);
        }
      }
      FMUC_PutString (OSDRegion, FrameLeft + 1, FrameTop - 2,                                                                                   ProgBarLeft, LangGetString(LS_S), ((BookmarkMode) ? COLOR_Gray : RGB(255,180,30)), COLOR_None, &Calibri_10_FontDataUC, FALSE, ALIGN_LEFT);
      FMUC_PutString (OSDRegion, FrameLeft,     FrameTop + FrameHeight + 1 - FMUC_GetStringHeight(LangGetString(LS_B), &Calibri_10_FontDataUC), ProgBarLeft, LangGetString(LS_B), ((BookmarkMode) ? RGB(60,255,60) : COLOR_Gray),  COLOR_None, &Calibri_10_FontDataUC, FALSE, ALIGN_LEFT);

      // Fill the active segment
      if ((NrSegmentMarker > 2) && (ActiveSegment < NrSegmentMarker-1))
      {
//        if ((SegmentMarker[ActiveSegment].Block <= SegmentMarker[ActiveSegment+1].Block) && (SegmentMarker[ActiveSegment+1].Block <= PlayInfo.totalBlock))
//        {
          curPos     = min((dword)((float)SegmentMarker[ActiveSegment].Block          * ProgBarWidth / PlayInfo.totalBlock), (dword)ProgBarWidth);
          curWidth   = min((dword)((float)SegmentMarker[ActiveSegment + 1].Block      * ProgBarWidth / PlayInfo.totalBlock) - curPos, (dword)ProgBarWidth + 1 - curPos);
          TAP_Osd_FillBox(OSDRegion, ProgBarLeft + curPos, ProgBarTop, curWidth, ProgBarHeight, ColorActiveSegment);
//        }
      }

      NearestMarker = (BookmarkMode) ? FindNearestBookmark(currentVisibleBlock()) : FindNearestSegmentMarker(currentVisibleBlock());

      // For each Segment
      for(i = 0; i < NrSegmentMarker - 1; i++)
      {
        // Fill too large segements with warning color
        UseSelectionColor = ColorSegmentSelection;
        if (NrSegmentMarker > 2)
        {
          if( ((i < NrSegmentMarker-2 || DisableSpecialEnd)   && (SegmentMarker[i+1].Block - SegmentMarker[i].Block + 22 > 475949))
           || (!DisableSpecialEnd && (i == NrSegmentMarker-2) && (SegmentMarker[i].Block + 22 > 475949)) )
          {
//            if ((SegmentMarker[i].Block <= SegmentMarker[i+1].Block) && (SegmentMarker[i+1].Block <= PlayInfo.totalBlock))
//            {
              curPos   = min((dword)((float)SegmentMarker[i].Block                    * ProgBarWidth / PlayInfo.totalBlock), (dword)ProgBarWidth);
              curWidth = min((dword)((float)SegmentMarker[i + 1].Block                * ProgBarWidth / PlayInfo.totalBlock) - curPos, (dword)ProgBarWidth + 1 - curPos);
              TAP_Osd_FillBox(OSDRegion, ProgBarLeft + curPos, ProgBarTop, curWidth, ProgBarHeight, ((i == ActiveSegment) ? ColorActiveSegWarning : ColorWarnedSegment));
//            }
            UseSelectionColor = ColorSelectionWarning;
          }
        }

        // Draw the selection
        if(SegmentMarker[i].Selected)
        {
//          if ((SegmentMarker[i].Block <= SegmentMarker[i+1].Block) && (SegmentMarker[i+1].Block <= PlayInfo.totalBlock))
//          {
            curPos   = min((dword)((float)SegmentMarker[i].Block                      * ProgBarWidth / PlayInfo.totalBlock), (dword)ProgBarWidth);
            curWidth = min((dword)((float)SegmentMarker[i+1].Block                    * ProgBarWidth / PlayInfo.totalBlock) - curPos, (dword)ProgBarWidth + 1 - curPos);
            TAP_Osd_DrawRectangle(OSDRegion, ProgBarLeft + curPos, ProgBarTop, curWidth, ProgBarHeight, 2, UseSelectionColor);
//          }
        }

        // Draw the segment marker
        if((i >= 1) /* && (SegmentMarker[i].Block <= PlayInfo.totalBlock) */)
        {
          curPos     = min((dword)((float)SegmentMarker[i].Block                      * ProgBarWidth / PlayInfo.totalBlock), (dword)ProgBarWidth + 1);
          if (!BookmarkMode)
          {
            if (i == NearestMarker)
              TAP_Osd_PutGd(OSDRegion, ProgBarLeft + curPos - _SegmentMarker_current_Gd.width/2, ProgBarTop - _SegmentMarker_current_Gd.height, &_SegmentMarker_current_Gd, TRUE);
            else
              TAP_Osd_PutGd(OSDRegion, ProgBarLeft + curPos - _SegmentMarker_Gd.width/2,         ProgBarTop - _SegmentMarker_Gd.height, &_SegmentMarker_Gd, TRUE);
          }
          else
            TAP_Osd_PutGd(OSDRegion,   ProgBarLeft + curPos - _SegmentMarker_gray_Gd.width/2,    ProgBarTop - _SegmentMarker_gray_Gd.height, &_SegmentMarker_gray_Gd, TRUE);
        }
      }

      // Draw requested jump
      if (JumpRequestedSegment != 0xFFFF)
      {
//        if ((SegmentMarker[JumpRequestedSegment].Block <= SegmentMarker[JumpRequestedSegment+1].Block) && (SegmentMarker[JumpRequestedSegment+1].Block <= PlayInfo.totalBlock))
//        {
          curPos     = min((dword)((float)SegmentMarker[JumpRequestedSegment].Block   * ProgBarWidth / PlayInfo.totalBlock), (dword)ProgBarWidth);
          curWidth   = min((dword)((float)SegmentMarker[JumpRequestedSegment+1].Block * ProgBarWidth / PlayInfo.totalBlock) - curPos, (dword)ProgBarWidth + 1 - curPos);
          if (SegmentMarker[JumpRequestedSegment].Selected)
            TAP_Osd_DrawRectangle(OSDRegion, ProgBarLeft + curPos, ProgBarTop, curWidth, ProgBarHeight, 1, ColorActiveSegment);
          else
          {
            TAP_Osd_DrawRectangle(OSDRegion, ProgBarLeft + curPos, ProgBarTop, curWidth, ProgBarHeight, 2, ((JumpRequestedSegment!=ActiveSegment) ? ColorActiveSegment : RGB(50, 150, 190)));
//            if ((JumpRequestedSegment == ActiveSegment) && (curWidth > 2))
//              TAP_Osd_DrawRectangle(OSDRegion, ProgBarLeft + curPos + 1, ProgBarTop + 1, curWidth - 2, ProgBarHeight - 2, 1, COLOR_Gray);
          }
//        }
      }

      // Draw the Bookmarks
      for(i = 0; i < NrBookmarks; i++)
      {
//        if(Bookmarks[i] <= PlayInfo.totalBlock)
//        {
          curPos     = min((dword)((float)Bookmarks[i] * ProgBarWidth / PlayInfo.totalBlock), (dword)ProgBarWidth + 1);
          if (BookmarkMode)
          {
            if (i == NearestMarker)
              TAP_Osd_PutGd(OSDRegion, ProgBarLeft + curPos - _BookmarkMarker_current_Gd.width/2, ProgBarTop + ProgBarHeight, &_BookmarkMarker_current_Gd, TRUE);
            else
              TAP_Osd_PutGd(OSDRegion, ProgBarLeft + curPos - _BookmarkMarker_Gd.width/2,         ProgBarTop + ProgBarHeight, &_BookmarkMarker_Gd, TRUE);
          }
          else
            TAP_Osd_PutGd(OSDRegion,   ProgBarLeft + curPos - _BookmarkMarker_gray_Gd.width/2,    ProgBarTop + ProgBarHeight, &_BookmarkMarker_gray_Gd, TRUE);
//        }
      }

      // Draw the current position
      if((int)PlayInfo.currentBlock >= 0)
      {
        curPos = ProgBarLeft + min(pos, (dword)ProgBarWidth + 1);
        for(j = 0; j <= ProgBarHeight; j++)
          TAP_Osd_PutPixel(OSDRegion, curPos,     ProgBarTop + j,                 ColorCurrentPos);
        TAP_Osd_PutPixel  (OSDRegion, curPos,     ProgBarTop + ProgBarHeight + 1, ColorCurrentPosMark);
        for(j = -1; j <= 1; j++)
          TAP_Osd_PutPixel(OSDRegion, curPos + j, ProgBarTop + ProgBarHeight + 2, ColorCurrentPosMark);
        for(j = -2; j <= 2; j++)
          TAP_Osd_PutPixel(OSDRegion, curPos + j, ProgBarTop + ProgBarHeight + 3, ColorCurrentPosMark);
      }
      LastPos = pos;
      if(DoSync) TAP_Osd_Sync();
    }
    LastDraw = TAP_GetTick();
  }
  TRACEEXIT();
}


// Info-Bereich
// ------------
void OSDInfoDrawBackground(void)
{
  TYPE_GrData*          ColorButtons[]       = {&_Button_Red_Gd,          &_Button_Green_Gd,     &_Button_Yellow_Gd,     &_Button_Blue_Gd};
  char*                 ColorButtonStrings[] = {LangGetString(LS_Delete), LangGetString(LS_Add), LangGetString(LS_Move), LangGetString(LS_Select)};
  int                   ColorButtonLengths[4];
  TYPE_GrData*          BelowButtons[]       = {&_Button_Recall_Gd,     &_Button_VF_Gd,               &_Button_ProgPlusMinus_Gd,  &_Button_Menu_Gd,            &_Button_Exit_Gd,       &_Button_White_Gd};
  char*                 BelowButtonStrings[] = {LangGetString(LS_Undo), LangGetString(LS_ChangeMode), LangGetString(LS_FastNav),  LangGetString(LS_PauseMenu), LangGetString(LS_Exit), LangGetString(LS_OSD)};
  int                   BelowButtonLengths[6];

  int                   PosX, PosY, ButtonDist;
  int                   i;

  TRACEENTER();

  if(rgnInfoBar)
  {
    // Draw the background
    TAP_Osd_FillBox(rgnInfoBar, 0,  0, ScreenWidth, 45, ColorInfoBarTitle);
    TAP_Osd_FillBox(rgnInfoBar, 0, 45, ScreenWidth, GetOSDRegionHeight(rgnInfoBar) - 45, ColorDarkBackground);

    // Draw the title area
    TAP_Osd_PutGd(rgnInfoBar, Overscan_X + 12, 11, &_Icon_Playing_Gd, TRUE);
    TAP_Osd_DrawRectangle(rgnInfoBar, ScreenWidth - Overscan_X - InfoBarRightAreaWidth - 2, 12, 1, 22, 1, RGB(92,93,93));

    // Draw the color buttons area
    const int FrameWidth = ScreenWidth - 2*Overscan_X - InfoBarRightAreaWidth - InfoBarModeAreaWidth - 4;

    TAP_Osd_FillBox(rgnInfoBar, Overscan_X, InfoBarLine1_Y, FrameWidth, InfoBarLine1Height, ColorInfoBarDarkSub);

    ButtonDist = 6;
    for (i = 0; i < 4; i++)
    {
      ColorButtonLengths[i] = FMUC_GetStringWidth(ColorButtonStrings[i], &Calibri_12_FontDataUC);
      ButtonDist += ColorButtons[i]->width + 3 + ColorButtonLengths[i];
    }
    ButtonDist = (FrameWidth - ButtonDist - 4) / 4;
    ButtonDist = min(ButtonDist, 20);

    PosY = InfoBarLine1_Y + 5;
    PosX = Overscan_X + 6;
    for (i = 0; i < 4; i++)
    {
      TAP_Osd_PutGd(rgnInfoBar, PosX, PosY + 1, ColorButtons[i], TRUE);
      PosX += _Button_Red_Gd.width + 3;
      FMUC_PutString(rgnInfoBar, PosX, PosY, PosX + max(ColorButtonLengths[i] + ButtonDist, 0), ColorButtonStrings[i], COLOR_White, ColorInfoBarDarkSub, &Calibri_12_FontDataUC, TRUE, ALIGN_LEFT);
      PosX += max(ColorButtonLengths[i] + ButtonDist, 0);
    }

    // Draw Sub-backgrounds
//    TAP_Osd_FillBox(rgnInfoBar, Overscan_X + FrameWidth + 2, InfoBarLine1_Y, InfoBarModeAreaWidth, InfoBarLine1Height, ColorInfoBarLightSub);
//    TAP_Osd_FillBox(rgnInfoBar, ScreenWidth - Overscan_X - InfoBarRightAreaWidth, InfoBarLine1_Y, InfoBarRightAreaWidth, InfoBarLine1Height, ColorInfoBarDarkSub);

    // Draw border of ProgressBar
    TAP_Osd_DrawRectangle(rgnInfoBar, Overscan_X, InfoBarLine2_Y, ScreenWidth - 2 * Overscan_X, 34, 1, COLOR_Gray);

    // Draw the button usage info
    ButtonDist = 6;
    for (i = 0; i < 6; i++)
    {
      BelowButtonLengths[i] = FMUC_GetStringWidth(BelowButtonStrings[i], &Calibri_10_FontDataUC);
      ButtonDist += BelowButtons[i]->width + 2 + BelowButtonLengths[i];
    }
    ButtonDist = (ScreenWidth - (int)(2*Overscan_X) - ButtonDist - 74) / 6;
    ButtonDist = min(ButtonDist, 20);

    PosY = InfoBarLine3_Y;
    PosX = Overscan_X + 6;
    for (i = 0; i < 5 /*6*/; i++)
    {
      TAP_Osd_PutGd(rgnInfoBar, PosX, PosY, BelowButtons[i], TRUE);
//      if (BelowButtons[i] == &_Button_Exit_Gd)
//        PosX += BelowButtons[i]->width + 3;
//      else
        PosX += BelowButtons[i]->width + 2;
      FMUC_PutString(rgnInfoBar, PosX, PosY + 1, PosX + max(BelowButtonLengths[i] + ButtonDist, 0), BelowButtonStrings[i], COLOR_White, ColorDarkBackground, &Calibri_10_FontDataUC, TRUE, ALIGN_LEFT);
      PosX += BelowButtonLengths[i] + ButtonDist;
    }
//TAP_Osd_DrawRectangle(rgnInfoBar, Overscan_X+6, InfoBarLine3_Y, PosX - Overscan_X-6, 20, 2, COLOR_Gray);
  }

  if(rgnInfoBarMini)
  {
    // Draw border of ProgressBar
    TAP_Osd_FillBox(rgnInfoBarMini, 0, 0, GetOSDRegionWidth(rgnInfoBarMini), GetOSDRegionHeight(rgnInfoBarMini), ColorLightBackground);
    TAP_Osd_DrawRectangle(rgnInfoBarMini, 0, 0, GetOSDRegionWidth(rgnInfoBarMini), GetOSDRegionHeight(rgnInfoBarMini), 2, COLOR_Gray);
  }

  TRACEEXIT();
}

void OSDInfoDrawRecName(void)
{
  const int             FrameLeft    = Overscan_X + 45,                                                    FrameTop    =  0;  // 10
  const int             FrameWidth   = ScreenWidth - Overscan_X - InfoBarRightAreaWidth - FrameLeft - 2,   FrameHeight = 45;  // 26
  const int             TimeWidth    = FMUC_GetStringWidth("99:99 h", &Calibri_12_FontDataUC) + 1,         TimeLeft    = FrameLeft + FrameWidth - TimeWidth - 2;
  int                   TimeTop      = FrameTop + 13,                                                      TitleLeft   = FrameLeft + 2;
  int                   TitleWidth   = FrameWidth - TimeWidth - (TitleLeft - FrameLeft) - 12;
  int                   TitleHeight, TitleTop;
  char                  TitleStr[MAX_FILE_NAME_SIZE + 1];
  char                  TimeStr[8];
  dword                 TimeVal, Hours, Minutes, Seconds;

  tFontDataUC          *UseTitleFont = NULL;
  char                 *LastSpace = NULL;
  char                 *EndOfName = NULL;
  int                   EndOfNameWidth;

  TRACEENTER();

  if(rgnInfoBar)
  {
//    TAP_Osd_FillBox(rgnInfoBar, FrameLeft, FrameTop, FrameWidth, FrameHeight, COLOR_Blue);

    // Dateiname in neuen String kopieren und .rec entfernen
    strncpy(TitleStr, PlaybackName, sizeof(TitleStr));
    #ifdef Calibri_10_FontDataUC
      if(isUTFToppy()) StrToISO(PlaybackName, TitleStr);
    #endif
    TitleStr[MAX_FILE_NAME_SIZE] = '\0';
//    NameStr[strlen(NameStr) - 4] = '\0';

    // Passende Schriftgröße ermitteln
    UseTitleFont = &Calibri_14_FontDataUC;
    if(FMUC_GetStringWidth(TitleStr, UseTitleFont) + 10 > (dword)TitleWidth)
    {
      UseTitleFont = &Calibri_12_FontDataUC;
      TimeTop--;
      TitleLeft -= 2;
      TitleWidth += 2;
    }

    // Vertikale Schriftposition berechnen
    TitleHeight = FMUC_GetStringHeight(TitleStr, UseTitleFont);
    TitleTop = FrameTop + (FrameHeight - TitleHeight) / 2 - 1;

    // Titel ausgeben
    if(FMUC_GetStringWidth(TitleStr, UseTitleFont) > (dword)TitleWidth)
    {
      // Falls immernoch zu lang, Titel in der Mitte kürzen
      LastSpace = strrchr(TitleStr, ' ');
      if (LastSpace)
      {
        LastSpace[0] = '*';
        EndOfName = strrchr(TitleStr, ' ');
        LastSpace[0] = ' ';
      }
      if (EndOfName)
        EndOfName++;
      else
      {
        if (LastSpace)
          EndOfName = LastSpace + 1;
        else
          EndOfName = &TitleStr[strlen(TitleStr) - 12];
      }
      EndOfNameWidth = FMUC_GetStringWidth(EndOfName, UseTitleFont);
      FMUC_PutString(rgnInfoBar, TitleLeft,                                 TitleTop, TitleLeft + TitleWidth - EndOfNameWidth, TitleStr,  COLOR_White, ColorInfoBarTitle, UseTitleFont, TRUE, ALIGN_LEFT);
      FMUC_PutString(rgnInfoBar, TitleLeft + TitleWidth - EndOfNameWidth-1, TitleTop, TitleLeft + TitleWidth,                  EndOfName, COLOR_White, ColorInfoBarTitle, UseTitleFont, FALSE, ALIGN_LEFT);
    }
    else
      FMUC_PutString(rgnInfoBar, TitleLeft,                                 TitleTop, TitleLeft + TitleWidth,                  TitleStr,  COLOR_White, ColorInfoBarTitle, UseTitleFont, TRUE, ALIGN_LEFT);

    TAP_Osd_DrawRectangle(rgnInfoBar, TimeLeft - 4, 12, 1, 22, 1, RGB(92,93,93));

//    if(PLAYINFOVALID())  // bei ungültiger PlayInfo lautet schlimmstenfalls die Zeitanzeige 99:99
//    {
      TimeVal = PlayInfo.duration * 60 + PlayInfo.durationSec;
      if (TimeVal >= 3600)
      {
        Hours    = (int)(TimeVal / 3600);
        Minutes  = (int)(TimeVal / 60) % 60;
        if (Hours >= 100) {Hours = 99; Minutes = 99;}
        TAP_SPrint(TimeStr, 8, "%lu:%02lu h", Hours, Minutes);
      }
      else if (TimeVal >= 60)
      {
        Minutes  = (int)(TimeVal / 60) % 60;
        Seconds  = TimeVal % 60;
        if (Minutes < 10)
          TAP_SPrint(TimeStr, 7, "%lu:%02lu m", Minutes, Seconds);
        else
          TAP_SPrint(TimeStr, 7, "%lu min", Minutes);
      }
      else
      {
        Seconds  = TimeVal % 60;
        TAP_SPrint(TimeStr, 7, "%lu sek", Seconds);
      }
      FMUC_PutString(rgnInfoBar, TimeLeft, TimeTop, TimeLeft + TimeWidth - 1, TimeStr, COLOR_White, ColorInfoBarTitle, &Calibri_12_FontDataUC, FALSE, ALIGN_CENTER);
//    }
  }
  TRACEEXIT();
}

void OSDInfoDrawPlayIcons(bool Force, bool DoSync)
{
  static TYPE_TrickMode  LastTrickMode = TRICKMODE_Slow;
  static byte            LastTrickModeSpeed = 99;
//  static bool            LastNoOSDMode = TRUE;

  TRACEENTER();

  if(rgnInfoBar)
  {
    const int            ButtonWidth = _Button_Play_Inactive_Gd.width,         ButtonDist = 3;
    const int            FrameWidth  = 5 * ButtonWidth + 4 * ButtonDist;
    const int            FrameLeft   = ScreenWidth - Overscan_X - FrameWidth,  FrameTop = 1;
    const int            ButtonTop   = FrameTop + 20;
    char                 SpeedText[6];
    int                  PosX, TextPosX=0;

    if(Force || /*((OSDMode == MD_NoOSD) != LastNoOSDMode) ||*/ (TrickMode != LastTrickMode) || (TrickModeSpeed != LastTrickModeSpeed))
    {
      TAP_Osd_FillBox(rgnInfoBar, FrameLeft - 6, FrameTop, min(FrameWidth + 13, ScreenWidth-FrameLeft+6), ButtonTop - FrameTop, ColorInfoBarTitle);  // müsste eigentlich 7 nach links gehen...

      PosX = FrameLeft;
      TAP_Osd_PutGd(rgnInfoBar, PosX, ButtonTop, ((TrickMode == TRICKMODE_Rewind)  ? &_Button_Rwd_Active_Gd   : &_Button_Rwd_Inactive_Gd),   TRUE);
      if (TrickMode == TRICKMODE_Rewind) TextPosX = PosX; 

      PosX += ButtonWidth + ButtonDist;
      TAP_Osd_PutGd(rgnInfoBar, PosX, ButtonTop, ((TrickMode == TRICKMODE_Pause)   ? &_Button_Pause_Active_Gd : &_Button_Pause_Inactive_Gd), TRUE);

      PosX += ButtonWidth + ButtonDist;
      TAP_Osd_PutGd(rgnInfoBar, PosX, ButtonTop, ((TrickMode == TRICKMODE_Normal)  ? &_Button_Play_Active_Gd  : &_Button_Play_Inactive_Gd),  TRUE);

      PosX += ButtonWidth + ButtonDist;
      TAP_Osd_PutGd(rgnInfoBar, PosX, ButtonTop, ((TrickMode == TRICKMODE_Slow)    ? &_Button_Slow_Active_Gd  : &_Button_Slow_Inactive_Gd),  TRUE);
      if (TrickMode == TRICKMODE_Slow) TextPosX = PosX; 

      PosX += ButtonWidth + ButtonDist;
      TAP_Osd_PutGd(rgnInfoBar, PosX, ButtonTop, ((TrickMode == TRICKMODE_Forward) ? &_Button_Ffwd_Active_Gd  : &_Button_Ffwd_Inactive_Gd),  TRUE);
      if (TrickMode == TRICKMODE_Forward) TextPosX = PosX;

      if (TrickMode == TRICKMODE_Rewind || TrickMode == TRICKMODE_Forward || TrickMode == TRICKMODE_Slow)
      {
        TAP_SPrint(SpeedText, sizeof(SpeedText), ((TrickMode == TRICKMODE_Slow) ? "1/%dx" : "%dx"), (1 << TrickModeSpeed));
        FMUC_PutString(rgnInfoBar, TextPosX - 7, FrameTop, TextPosX + ButtonWidth + 6 , SpeedText, COLOR_White, COLOR_None, &Calibri_12_FontDataUC, FALSE, ALIGN_CENTER);
      }

      LastTrickMode = TrickMode;
      LastTrickModeSpeed = TrickModeSpeed;
      if(DoSync) TAP_Osd_Sync();
    }
  }
  else
  {
    const int            RegionWidth  = _PlayState_Background_Gd.width;
    const int            RegionHeight = _PlayState_Background_Gd.height;
    const int            IconLeft = 6,   IconTop = 6;

    char                 SpeedText[6];
    TYPE_GrData         *ActiveSymbol = NULL;
    int                  PosX, PosX2;

    if((TrickMode != LastTrickMode) || (TrickModeSpeed != LastTrickModeSpeed))
    {
      if(rgnTextState)
      {
        TAP_Osd_Delete(rgnTextState);
        rgnTextState = 0;
      }
      if(!rgnPlayState)
        rgnPlayState = TAP_Osd_Create(ScreenWidth - Overscan_X - RegionWidth - 20, Overscan_Y + 20, RegionWidth, RegionHeight, 0, 0);
      LastPlayStateChange = TAP_GetTick();
      if(LastPlayStateChange == 0) LastPlayStateChange = 1;

      TAP_Osd_PutGd(rgnPlayState, 0, 0, &_PlayState_Background_Gd, FALSE);
      switch(TrickMode)
      {
        case TRICKMODE_Normal:   ActiveSymbol = &_Icon_Playing_Gd;  break;
        case TRICKMODE_Forward:  ActiveSymbol = &_Icon_Ffwd_Gd;     break;
        case TRICKMODE_Rewind:   ActiveSymbol = &_Icon_Rwd_Gd;      break;
        case TRICKMODE_Slow:     ActiveSymbol = &_Icon_Slow_Gd;     break;
        case TRICKMODE_Pause:    ActiveSymbol = &_Icon_Pause_Gd;    break;
        default:                 return;
      }
      TAP_Osd_PutGd(rgnPlayState, IconLeft, IconTop, ActiveSymbol, TRUE);

      if (TrickMode == TRICKMODE_Rewind || TrickMode == TRICKMODE_Forward || TrickMode == TRICKMODE_Slow)
      {
        TAP_SPrint(SpeedText, sizeof(SpeedText), "%dx", (1 << TrickModeSpeed));
        if (TrickMode == TRICKMODE_Slow)
        {
          PosX  = IconLeft + 6;
          PosX2 = PosX + FMUC_GetStringWidth("1", &Calibri_10_FontDataUC);
          FMUC_PutString(rgnPlayState, PosX, IconTop + 5, PosX2, "1",       COLOR_White, COLOR_None, &Calibri_10_FontDataUC, FALSE, ALIGN_LEFT);
          PosX  = PosX2 - 1;
          PosX2 = PosX + FMUC_GetStringWidth("/", &Calibri_10_FontDataUC);
          FMUC_PutString(rgnPlayState, PosX, IconTop + 6, PosX2, "/",       COLOR_White, COLOR_None, &Calibri_10_FontDataUC, FALSE, ALIGN_LEFT);
          PosX  = PosX2;
          PosX2 = PosX + FMUC_GetStringWidth(SpeedText, &Calibri_12_FontDataUC);
          FMUC_PutString(rgnPlayState, PosX, IconTop + 7, PosX2, SpeedText, COLOR_White, COLOR_None, &Calibri_12_FontDataUC, FALSE, ALIGN_LEFT);
        }
        else
          FMUC_PutString(rgnPlayState, IconLeft + 7, IconTop + 8, RegionWidth - 1, SpeedText,  COLOR_White, COLOR_None, &Calibri_12_FontDataUC, FALSE, ALIGN_LEFT);
      }

      LastTrickMode = TrickMode;
      LastTrickModeSpeed = TrickModeSpeed;
//      TAP_Osd_Sync();
      OSDInfoDrawCurrentPlayTime(TRUE);
      if(DoSync) TAP_Osd_Sync();
    }
  }

//  LastNoOSDMode = (OSDMode == MD_NoOSD);
  TRACEEXIT();
}

void OSDInfoDrawCurrentPlayTime(bool Force)
{
  static byte           LastSec = 99;
  static dword          LastDraw = 0;
  static dword          maxBlock = 0;

  TRACEENTER();
  if (((int)PlayInfo.totalBlock <= 0) /*|| ((int)PlayInfo.currentBlock < 0)*/)   // wenn currentBlock nicht gesetzt, schlimmstenfalls zu hohe Prozentzahl
  {
    TRACEEXIT();
    return;
  }

  // Experiment: Stabilisierung der vor- und zurückspringenden Zeit-Anzeige (noch linear)
  dword curBlock = currentVisibleBlock();
  if ((TrickMode != TRICKMODE_Normal) || (curBlock > maxBlock)) maxBlock = curBlock;

  if ((TrickMode != TRICKMODE_Normal) || (labs(TAP_GetTick() - LastDraw) > 15) || Force)
  {
    dword             Time;
    float             Percent;
    dword             PercentWidth;
    char              TimeString[12];
    char              PercentString[12];

    Time = NavGetBlockTimeStamp(maxBlock) / 1000;
    if(((Time % 60) != LastSec) || Force)
    {
      SecToTimeString(Time, TimeString);
      Percent = ((float)maxBlock / PlayInfo.totalBlock) * 100.0;
      if (abs(Percent) >= 999) Percent = 999.0;
      TAP_SPrint(PercentString, sizeof(PercentString), "%1.1f%%", Percent);

      if(rgnInfoBar)
      {
        const int Frame1Width = 78,                                                          Frame2Width = InfoBarRightAreaWidth - Frame1Width - 1;
        const int Frame1Left  = ScreenWidth - Overscan_X - InfoBarRightAreaWidth,            Frame2Left  = Frame1Left + Frame1Width + 1;
        PercentWidth = (dword)((float)PlayInfo.currentBlock * InfoBarRightAreaWidth / PlayInfo.totalBlock);
        PercentWidth = min(PercentWidth, InfoBarRightAreaWidth + 1);

        TAP_Osd_FillBox      (rgnInfoBar, Frame1Left,               InfoBarLine1_Y, InfoBarRightAreaWidth, InfoBarLine1Height, ColorInfoBarDarkSub);
        TAP_Osd_DrawRectangle(rgnInfoBar, Frame1Left + Frame1Width, InfoBarLine1_Y + 6, 1, 17, 1, RGB(92,93,93));

        TAP_Osd_FillBox(rgnInfoBar, Frame1Left,   InfoBarLine1_Y + InfoBarLine1Height - 2, PercentWidth, 2, COLOR_Gray);
        FMUC_PutString (rgnInfoBar, Frame1Left+1, InfoBarLine1_Y + 5, Frame1Left + Frame1Width - 1, PercentString, COLOR_White, ColorInfoBarDarkSub, &Calibri_12_FontDataUC, FALSE, ALIGN_CENTER);
        #ifndef Calibri_10_FontDataUC
          FMUC_PutString (rgnInfoBar, Frame2Left,   InfoBarLine1_Y + 5, Frame2Left + Frame2Width - 1, TimeString,    COLOR_White, ColorInfoBarDarkSub, &Courier_New_13_FontDataUC, FALSE, ALIGN_CENTER);
        #else
          FMUC_PutString (rgnInfoBar, Frame2Left,   InfoBarLine1_Y + 5, Frame2Left + Frame2Width - 1, TimeString,    COLOR_White, ColorInfoBarDarkSub, &Calibri_12_FontDataUC,     FALSE, ALIGN_CENTER);
        #endif
      }
      if(rgnInfoBarMini)
      {
        const int FrameWidth = FMUC_GetStringWidth("9:99:99", &Calibri_12_FontDataUC) + 1,   FrameHeight = GetOSDRegionHeight(rgnInfoBarMini) - 6;
        const int FrameLeft  = GetOSDRegionWidth(rgnInfoBarMini) - 2 - FrameWidth,           FrameTop = 3;

        TAP_Osd_FillBox(rgnInfoBarMini, FrameLeft, FrameTop, FrameWidth, FrameHeight, ColorLightBackground);
        FMUC_PutString(rgnInfoBarMini, FrameLeft, FrameTop + 6, FrameLeft + FrameWidth - 1, TimeString, COLOR_White, ColorLightBackground, &Calibri_12_FontDataUC, FALSE, ALIGN_RIGHT);
      }
      if(rgnPlayState)
      {
        const int ProgBarWidth  = _PlayState_Background_Gd.width - 10,   ProgBarHeight = 3;
        const int ProgBarLeft   = 5,                                     ProgBarTop    = _PlayState_Background_Gd.height - ProgBarHeight - 5;

        const int PlayTimeWidth = FMUC_GetStringWidth("9:99:99", &Calibri_10_FontDataUC) + 1,        PlayTimeHeight = FMUC_GetStringHeight("9:99:99", &Calibri_10_FontDataUC);
        const int PlayTimeLeft  = _PlayState_Background_Gd.width - PlayTimeWidth - ProgBarLeft + 1,  PlayTimeTop    = ProgBarTop - PlayTimeHeight - 1;

        FMUC_PutString(rgnPlayState, PlayTimeLeft, PlayTimeTop, PlayTimeLeft + PlayTimeWidth - 1, TimeString, COLOR_White, ColorDarkBackground, &Calibri_10_FontDataUC, FALSE, ALIGN_RIGHT);

        PercentWidth = (dword)((float)PlayInfo.currentBlock * ProgBarWidth / PlayInfo.totalBlock);
        PercentWidth = min(PercentWidth, (dword)ProgBarWidth + 1);
        TAP_Osd_FillBox(rgnPlayState, ProgBarLeft, ProgBarTop, ProgBarWidth, ProgBarHeight, COLOR_Gray);
        TAP_Osd_FillBox(rgnPlayState, ProgBarLeft, ProgBarTop, PercentWidth, ProgBarHeight, RGB(250,0,0));
      }

      LastSec = Time % 60;
    }
    maxBlock = 0;
    LastDraw = TAP_GetTick();
  }

  TRACEEXIT();
}

void OSDInfoDrawBookmarkMode(bool DoSync)
{
  const int             FrameWidth = InfoBarModeAreaWidth;
  const int             FrameLeft  = ScreenWidth - Overscan_X - InfoBarRightAreaWidth - FrameWidth - 2;
  TRACEENTER();

  if(rgnInfoBar)
  {
    TAP_Osd_FillBox(rgnInfoBar, FrameLeft, InfoBarLine1_Y, FrameWidth, InfoBarLine1Height, ColorInfoBarLightSub);
    FMUC_PutString (rgnInfoBar, FrameLeft, InfoBarLine1_Y + 7, FrameLeft + FrameWidth-1, ((BookmarkMode ? LangGetString(LS_Bookmarks) : LangGetString(LS_Segments))), ((BookmarkMode) ? RGB(60,255,60) : RGB(250,139,18)), ColorInfoBarLightSub, &Calibri_10_FontDataUC, FALSE, ALIGN_CENTER);
    if(DoSync) TAP_Osd_Sync();
  }

  TRACEEXIT();
}

void OSDInfoDrawClock(bool Force)
{
  const int             FrameWidth  = FMUC_GetStringWidth("99:99", &Calibri_14_FontDataUC) + 1;
  const int             FrameLeft   = ScreenWidth - FrameWidth - Overscan_X + 1 - 6;
  const int             FrameTop    = InfoBarLine3_Y - 3;

  word                  mjd;
  byte                  hour, min, sec;
  static byte           LastMin = 99;
  char                  Time[8];

  TRACEENTER();

  if(rgnInfoBar)
  {
    TAP_GetTime(&mjd, &hour, &min, &sec);
    if((min != LastMin) || Force)
    {
      TAP_SPrint(Time, sizeof(Time), "%2.2d:%2.2d", hour, min);
      FMUC_PutString (rgnInfoBar, FrameLeft, FrameTop, FrameLeft + FrameWidth - 1, Time, COLOR_White, ColorDarkBackground, &Calibri_14_FontDataUC, FALSE, ALIGN_RIGHT);
      LastMin = min;
    }
  }

  TRACEEXIT();
}

void OSDInfoDrawMinuteJump(bool DoSync)
{
  const int             FrameLeft  = ScreenWidth - Overscan_X - InfoBarRightAreaWidth + 4;
  const int             FrameTop   = 4,                                                            ButtonTop   = FrameTop + 17;
  const int             FrameWidth = _Button_SkipLeft_Gd.width + _Button_SkipRight_Gd.width + 1,   FrameHeight = ButtonTop - FrameTop + _Button_SkipLeft_Gd.height;
  char InfoStr[5];

  TRACEENTER();

  if(rgnInfoBar)
  {
    TAP_Osd_FillBox(rgnInfoBar, FrameLeft, FrameTop, FrameWidth, FrameHeight, ColorInfoBarTitle);
    if(BookmarkMode || MinuteJump)
    {
      TAP_Osd_PutGd(rgnInfoBar, FrameLeft,                                 ButtonTop, &_Button_SkipLeft_Gd,  TRUE);
      TAP_Osd_PutGd(rgnInfoBar, FrameLeft + 1 + _Button_SkipLeft_Gd.width, ButtonTop, &_Button_SkipRight_Gd, TRUE);

      if (MinuteJump)
        TAP_SPrint(InfoStr, sizeof(InfoStr), "%d'", MinuteJump);
      else
        TAP_SPrint(InfoStr, sizeof(InfoStr), LangGetString(LS_BM));
      FMUC_PutString(rgnInfoBar, FrameLeft + 1, FrameTop, FrameLeft + FrameWidth - 1, InfoStr, COLOR_White, COLOR_None, &Calibri_10_FontDataUC, FALSE, ALIGN_CENTER);
    }
    if(DoSync) TAP_Osd_Sync();
  }

  TRACEEXIT();
}

void OSDTextStateWindow(int MessageID)
{
  const int RegionWidth = 200;
  const int RegionHeight = 30;
  const int TextPositionY = 5;

  char  MessageStr[512];
  char *pMessageStr;
  bool  NoMiniOSD;

  TRACEENTER();

  if((OSDMode == MD_NoOSD) || (OSDMode == MD_MiniOSD && (MessageID==LS_MinuteJumpActive || MessageID==LS_MinuteJumpDisabled)))
  {
    if(rgnPlayState)
    {
      TAP_Osd_Delete(rgnPlayState);
      rgnPlayState = 0;
    }
    if(!rgnTextState)
      rgnTextState = TAP_Osd_Create(ScreenWidth - Overscan_X - RegionWidth - 20, Overscan_Y + 20, RegionWidth, RegionHeight, 0, 0);
    LastPlayStateChange = TAP_GetTick();
    if(LastPlayStateChange == 0) LastPlayStateChange = 1;

    TAP_Osd_FillBox(rgnTextState, 0, 0, RegionWidth, RegionHeight, ColorLightBackground);
    TAP_Osd_DrawRectangle(rgnTextState, 0, 0, RegionWidth, RegionHeight, 2, COLOR_Gray);

    pMessageStr = LangGetString(MessageID);
    NoMiniOSD = FALSE;
    switch(MessageID)
    {
      case LS_MovieCutterActive:
      {
        TAP_SPrint(MessageStr, sizeof(MessageStr), "%s (%s)", pMessageStr, LangGetString((BookmarkMode) ? LS_BM : LS_S));
        pMessageStr = MessageStr;
        NoMiniOSD = TRUE;
        break;
      }
      case LS_MinuteJumpActive:
      {
        TAP_SPrint(MessageStr, sizeof(MessageStr), pMessageStr, MinuteJump);
        pMessageStr = MessageStr;
        NoMiniOSD = TRUE;
        break;
      }
      case LS_MinuteJumpDisabled:
      {
        NoMiniOSD = TRUE;
        break;
      }  
    }
    FMUC_PutString(rgnTextState, 2, TextPositionY, RegionWidth-3, pMessageStr, RGB(255, 255, 224), ColorLightBackground, &Calibri_12_FontDataUC, TRUE, ALIGN_CENTER);
    TAP_Osd_Sync();

    if (OSDMode == MD_NoOSD && !NoMiniOSD)
    {
      OSDMode = MD_MiniOSD;
      OSDRedrawEverything();
      OSDMode = MD_NoOSD;
    }
  }
  TRACEEXIT();
}


// ----------------------------------------------------------------------------
//                          ActionMenu-Funktionen
// ----------------------------------------------------------------------------
void ActionMenuDraw(void)
{
  const int             TextFieldStart_X  =   7,   TextFieldStart_Y  = 4;
  const int             TextFieldHeight   =  26,   TextFieldDist     = 2;
  const int             ShortButtonLeft   =  _ActionMenu10_Gd.width - TextFieldStart_X - _Button_Sleep_small_Gd.width - 5;
  const dword           Color_Inactive    =  RGB(120, 120, 120);
  TYPE_GrData*          ShortButtons[]    =  {&_Button_1_small_Gd, &_Button_2_small_Gd, &_Button_3_small_Gd, &_Button_4_small_Gd, &_Button_5_small_Gd, &_Button_6_small_Gd, &_Button_7_small_Gd, &_Button_8_small_Gd, &_Button_Sleep_small_Gd};
  TYPE_GrData*          LowerButtons[]    =  {&_Button_Down_Gd, &_Button_Up_Gd, &_Button_VF_Gd, &_Button_Ok_Gd, &_Button_Exit_Gd};

  char                  TempStr[128];
  char                 *DisplayStr;
  dword                 DisplayColor;
  dword                 PosX, PosY;
  int                   i;

  TRACEENTER();

  NrSelectedSegments = 0;
  for(i = 0; i < NrSegmentMarker - 1; i++)
  {
    if(SegmentMarker[i].Selected) NrSelectedSegments++;
  }

  // Region erzeugen und Hintergrund zeichnen
  if(!rgnActionMenu)
  {
    rgnActionMenu = TAP_Osd_Create(((ScreenWidth - _ActionMenu10_Gd.width) / 2) +25, 70, _ActionMenu10_Gd.width, _ActionMenu10_Gd.height, 0, 0);
//    ActionMenuItem = 0;
  }
  TAP_Osd_PutGd(rgnActionMenu, 0, 0, &_ActionMenu10_Gd, FALSE);

  // Buttons unterhalb der Liste zeichnen
  PosX = TextFieldStart_X + 12;
  PosY = TextFieldStart_Y + (TextFieldHeight + TextFieldDist) * MI_NrMenuItems + 2;
  for(i = 0; i < (int)(sizeof(LowerButtons) / sizeof(TYPE_GrData*)) ; i++)
  {
    TAP_Osd_PutGd(rgnActionMenu, PosX, PosY, LowerButtons[i], TRUE);
    PosX += (LowerButtons[i]->width + 5);
  }

  // ShortCut-Buttons zeichnen
  for (i = 1; i <= 9; i++)
  {
    if (!(i == MI_DeleteFile && BookmarkMode))
      TAP_Osd_PutGd(rgnActionMenu, ShortButtonLeft, TextFieldStart_Y + (TextFieldHeight + TextFieldDist) * i + 4, ShortButtons[i-1], TRUE);
  }

  // Grünen Auswahlrahmen zeichnen
  TAP_Osd_PutGd(rgnActionMenu, TextFieldStart_X, TextFieldStart_Y + (TextFieldHeight + TextFieldDist) * ActionMenuItem, &_ActionMenu_Bar_Gd, FALSE);

  // Menü-Einträge zeichnen
  for (i = 0; i < MI_NrMenuItems; i++)
  {
    DisplayStr = NULL;
    DisplayColor = (ActionMenuItem == i ? COLOR_Black : COLOR_White);
    switch(i)
    {
      case MI_SelectFunction:
      {
        DisplayStr = LangGetString(LS_SelectFunction);
        #ifdef Calibri_10_FontDataUC
          char *p = DisplayStr;
          while (p && *p)
          {
            if (p[0] == 'â' || p[0] == '€' || p[0] == '•')  p[0] = '-';
            p++;
          }
        #endif
        break;
      }
      case MI_SaveSegments:
      {
        if (NrSelectedSegments == 0)
          DisplayStr = LangGetString(LS_SaveCurSegment);
        else if (NrSelectedSegments == 1)
          DisplayStr = LangGetString(LS_Save1Segment);
        else
        {
          TAP_SPrint(TempStr, sizeof(TempStr), LangGetString(LS_SaveNrSegments), NrSelectedSegments);
          DisplayStr = TempStr;
        }
        if (NrSegmentMarker <= 2) DisplayColor = Color_Inactive;
        break;
      }
      case MI_DeleteSegments:
      {
        if (NrSelectedSegments == 0)
          DisplayStr = LangGetString(LS_DeleteCurSegment);
        else if (NrSelectedSegments == 1)
          DisplayStr = LangGetString(LS_Delete1Segment);
        else
        {
          TAP_SPrint(TempStr, sizeof(TempStr), LangGetString(LS_DeleteNrSegments), NrSelectedSegments);
          DisplayStr = TempStr;
        }
        if (NrSegmentMarker <= 2) DisplayColor = Color_Inactive;
        break;
      }
      case MI_SelectOddSegments:
      {
        DisplayStr = (NrSegmentMarker == 4) ? LangGetString(LS_SelectPadding) : LangGetString(LS_SelectOddSegments);
        if (DirectSegmentsCut)
          DisplayStr = (NrSegmentMarker == 4) ? LangGetString(LS_RemovePadding) : LangGetString(LS_DeleteOddSegments);
        if (NrSegmentMarker <= 2) DisplayColor = Color_Inactive;
        break;
      }
      case MI_SelectEvenSegments:
      {
        DisplayStr = (NrSegmentMarker == 4) ? LangGetString(LS_SelectMiddle) : LangGetString(LS_SelectEvenSegments);
        if (DirectSegmentsCut)
          DisplayStr = LangGetString(LS_DeleteEvenSegments);
        if (NrSegmentMarker <= 2) DisplayColor = Color_Inactive;
        break;
      }
      case MI_ClearAll:
      {
        if (BookmarkMode)
        {
          DisplayStr = LangGetString(LS_DeleteAllBookmarks);
          if (NrBookmarks <= 0) DisplayColor = Color_Inactive;
        }
        else
        {
          if (NrSelectedSegments > 0)
            DisplayStr = LangGetString(LS_UnselectAll);
          else
          {
            DisplayStr = LangGetString(LS_ClearSegmentList);
            if (NrSegmentMarker <= 2) DisplayColor = Color_Inactive;
          }
        }
        break;
      }
      case MI_DeleteFile:
      {
        if (!BookmarkMode)
        {
          DisplayStr = LangGetString(LS_FileSystemCheck);
          DisplayColor = RGB(80, 240, 80);
        }
        else
        {
          DisplayStr = LangGetString(LS_DeleteFile);
          DisplayColor = RGB(255, 100, 100);
        }
        break;
      }
      case MI_ImportBookmarks:
      {
        DisplayStr = LangGetString(LS_ImportBM);
        if (NrBookmarks <= 0) DisplayColor = Color_Inactive;
        break;
      }
      case MI_ExportSegments:
      {
        DisplayStr = LangGetString(LS_ExportToBM);
        if (NrSegmentMarker <= 2) DisplayColor = Color_Inactive;
        break;
      }
      case MI_ExitMC:
        DisplayStr = LangGetString(LS_ExitMC);
        break;
      case MI_NrMenuItems:
        break;
    }

    if (DisplayColor == Color_Inactive)
      TAP_Osd_FillBox(rgnActionMenu, ShortButtonLeft,      TextFieldStart_Y + (TextFieldHeight + TextFieldDist) * i + 4, _Button_Sleep_small_Gd.width, _Button_Sleep_small_Gd.height, ColorInfoBarDarkSub);
    if (DisplayStr && (i < MI_NrMenuItems))
      FMUC_PutString(rgnActionMenu, TextFieldStart_X + 10, TextFieldStart_Y + (TextFieldHeight + TextFieldDist) * i + 1, ShortButtonLeft, DisplayStr, DisplayColor, COLOR_None, &Calibri_14_FontDataUC, TRUE, ((i==0) ? ALIGN_CENTER : ALIGN_LEFT));
  }

  TAP_Osd_Sync();
  TRACEEXIT();
}

void ActionMenuDown(void)
{
  TRACEENTER();

  do
  {
    if(ActionMenuItem >= (MI_NrMenuItems - 1))
      ActionMenuItem = 1;
    else
      ActionMenuItem++;
  } while ((ActionMenuItem<=4 && NrSegmentMarker<=2) || (ActionMenuItem==MI_ClearAll && !((BookmarkMode && NrBookmarks>0) || (!BookmarkMode && (NrSelectedSegments>0 || NrSegmentMarker>2)))) || (ActionMenuItem==MI_ImportBookmarks && NrBookmarks<=0) || (ActionMenuItem==MI_ExportSegments && NrSegmentMarker<=2));

  ActionMenuDraw();

  TRACEEXIT();
}

void ActionMenuUp(void)
{
  TRACEENTER();

  do
  {
    if(ActionMenuItem > 1)
      ActionMenuItem--;
    else
      ActionMenuItem = MI_NrMenuItems - 1;
  } while ((ActionMenuItem<=4 && NrSegmentMarker<=2) || (ActionMenuItem==MI_ClearAll && !((BookmarkMode && NrBookmarks>0) || (!BookmarkMode && (NrSelectedSegments>0 || NrSegmentMarker>2)))) || (ActionMenuItem==MI_ImportBookmarks && NrBookmarks<=0) || (ActionMenuItem==MI_ExportSegments && NrSegmentMarker<=2));

  ActionMenuDraw();

  TRACEEXIT();
}

void ActionMenuRemove(void)
{
  TRACEENTER();

  if(rgnActionMenu)
  {
    TAP_Osd_Delete(rgnActionMenu);
    rgnActionMenu = 0;
  }
  OSDRedrawEverything();
//  TAP_Osd_Sync();

  TRACEEXIT();
}


// ----------------------------------------------------------------------------
//                            Playback-Funktionen
// ----------------------------------------------------------------------------
void Playback_Faster(void)
{
  TRACEENTER();

  switch(TrickMode)
  {
    case TRICKMODE_Pause:
    {
      // 1/16xFWD
//      Playback_Slow();
      TrickModeSpeed = 3;
      TrickMode = TRICKMODE_Slow;
//      Appl_SetPlaybackSpeed(TRICKMODE_Slow, TrickModeSpeed, TRUE);
      break;
    }

    case TRICKMODE_Rewind:
    {
      if(TrickModeSpeed > 2)
      {
        // 64xRWD down to 2xRWD
        TrickModeSpeed -= 2;
//        Appl_SetPlaybackSpeed(TRICKMODE_Rewind, TrickModeSpeed, TRUE);
      }
      else
      {
        Playback_Normal();
        TRACEEXIT();
        return;
/*        // 1/16xFWD
        TrickModeSpeed = 4;
        Appl_SetPlaybackSpeed(TRICKMODE_Slow, TrickModeSpeed, TRUE);
        TrickMode = TRICKMODE_Slow; */
      }
      break;
    }

    case TRICKMODE_Slow:
    {
      if (TrickModeSpeed > 1)
      {
        // 1/16xFWD up to 1/2xFWD
        TrickModeSpeed--;
//        Appl_SetPlaybackSpeed(TRICKMODE_Slow, TrickModeSpeed, TRUE);
      }
      else
      {
        Playback_Normal();
        TRACEEXIT();
        return;
      }
      break;
    }

    case TRICKMODE_Normal:
    {
      // 2xFWD
      Playback_FFWD();
      TRACEEXIT();
      return;
    }

    case TRICKMODE_Forward:
    {
      // 2xFWD to 64xFWD
      Playback_FFWD();
      TRACEEXIT();
      return;
    }
  }
  Appl_SetPlaybackSpeed(TrickMode, TrickModeSpeed, TRUE);
  isPlaybackRunning();
  OSDInfoDrawPlayIcons(FALSE, TRUE);

  TRACEEXIT();
}

void Playback_Slower(void)
{
  TRACEENTER();

  switch(TrickMode)
  {
    case TRICKMODE_Forward:
    {
      if(TrickModeSpeed > 2)
      {
        // 64xFWD down to 2xFWD
        TrickModeSpeed -= 2;
//        Appl_SetPlaybackSpeed(TRICKMODE_Forward, TrickModeSpeed, TRUE);
      }
      else
      {
        // 1xFWD
        Playback_Normal();
        TRACEEXIT();
        return;
      }
      break;
    }

    case TRICKMODE_Normal:
    {
      Playback_RWD();
      // 1/2xFWD
//      Playback_Slow();
      TRACEEXIT();
      return;
    }

    case TRICKMODE_Slow:
    {
      // 1/2xFWD to 1/16xFWD
      if(TrickModeSpeed < 3)
      {
        TrickModeSpeed++;
//        Appl_SetPlaybackSpeed(TRICKMODE_Slow, TrickModeSpeed, TRUE);
      }
      else
      {
        // 1xRWD
//        TrickModeSpeed = 0;
//        Playback_RWD();
        TRACEEXIT();
        return;
      }
      break;
    }

    case TRICKMODE_Pause:
    {
      // 2xRWD
      Playback_RWD();
      TRACEEXIT();
      return;
    }

    case TRICKMODE_Rewind:
    {
      // 2xRWD down to 64xRWD
      Playback_RWD();
      TRACEEXIT();
      return;
    }
  }
  Appl_SetPlaybackSpeed(TrickMode, TrickModeSpeed, TRUE);
  isPlaybackRunning();
  OSDInfoDrawPlayIcons(FALSE, TRUE);

  TRACEEXIT();
}

void Playback_Normal(void)
{
  TRACEENTER();

  TrickMode = TRICKMODE_Normal;
  TrickModeSpeed = 1;
  Appl_SetPlaybackSpeed(TrickMode, TrickModeSpeed, TRUE);
  isPlaybackRunning();
  OSDInfoDrawPlayIcons(FALSE, TRUE);

  TRACEEXIT();
}

void Playback_Pause(void)
{
  TRACEENTER();

  TrickMode = TRICKMODE_Pause;
  TrickModeSpeed = 0;
  Appl_SetPlaybackSpeed(TrickMode, TrickModeSpeed, TRUE);
  isPlaybackRunning();
  OSDInfoDrawPlayIcons(FALSE, TRUE);

  TRACEEXIT();
}

void Playback_FFWD(void)
{
  TRACEENTER();

  //Appl_SetPlaybackSpeed(1, 1, true) 2x FFWD
  //Appl_SetPlaybackSpeed(1, 2, true) 4x FFWD
  //Appl_SetPlaybackSpeed(1, 3, true) 8x FFWD
  //Appl_SetPlaybackSpeed(1, 4, true) 16x FFWD
  //Appl_SetPlaybackSpeed(1, 5, true) 32x FFWD
  //Appl_SetPlaybackSpeed(1, 6, true) 64x FFWD

  if (TrickMode != TRICKMODE_Forward) 
    TrickModeSpeed = 0;

  if(TrickModeSpeed < 6)
  {
    TrickMode = TRICKMODE_Forward;
    TrickModeSpeed++;
    if (TrickModeSpeed < 6) TrickModeSpeed++;
    Appl_SetPlaybackSpeed(TrickMode, TrickModeSpeed, TRUE);
    isPlaybackRunning();
    OSDInfoDrawPlayIcons(FALSE, TRUE);
  }
  else
    Playback_Normal();

  TRACEEXIT();
}

void Playback_RWD(void)
{
  TRACEENTER();

  //Appl_SetPlaybackSpeed(2, 1, true) 2x RWD
  //Appl_SetPlaybackSpeed(2, 2, true) 4x RWD
  //Appl_SetPlaybackSpeed(2, 3, true) 8x RWD
  //Appl_SetPlaybackSpeed(2, 4, true) 16x RWD
  //Appl_SetPlaybackSpeed(2, 5, true) 32x RWD
  //Appl_SetPlaybackSpeed(2, 6, true) 64x RWD

  if (TrickMode != TRICKMODE_Rewind)
    TrickModeSpeed = 0;

  if(TrickModeSpeed < 6)
  {
    TrickMode = TRICKMODE_Rewind;
    TrickModeSpeed++;
    if (TrickModeSpeed < 6) TrickModeSpeed++;
    Appl_SetPlaybackSpeed(TrickMode, TrickModeSpeed, TRUE);
    isPlaybackRunning();
    OSDInfoDrawPlayIcons(FALSE, TRUE);
  }
  else
    Playback_Normal();

  TRACEEXIT();
}

void Playback_Slow(void)
{
  TRACEENTER();

  //Appl_SetPlaybackSpeed(3, 1, true) 1/2x Slow
  //Appl_SetPlaybackSpeed(3, 2, true) 1/4x Slow
  //Appl_SetPlaybackSpeed(3, 3, true) 1/8x Slow
  //Appl_SetPlaybackSpeed(3, 4, true) 1/16x Slow

  if (TrickMode != TRICKMODE_Slow)
    TrickModeSpeed = 1;
  else
    if(TrickModeSpeed < 3) TrickModeSpeed++;
    else TrickModeSpeed = 1;
  TrickMode = TRICKMODE_Slow;
  Appl_SetPlaybackSpeed(TrickMode, TrickModeSpeed, TRUE);
  isPlaybackRunning();
  OSDInfoDrawPlayIcons(FALSE, TRUE);

  TRACEEXIT();
}

void Playback_SetJumpNavigate(bool pJumpRequest, bool pNavRequest, bool pBackwards)
{
  static const dword    FastNavSteps[] = {12, 8, 4, 2, 1, 0, 1, 2, 4, 8, 12};
  static const int      NrFastNavSteps = 5;
  static int            LastDirection = 0;
  static dword          LastNavTime = 0;

  TRACEENTER();

  if (pJumpRequest && (NrSegmentMarker <= 2))
  {
    TRACEEXIT();
    return;
  }

  // Die jeweils andere Anforderung zurücksetzen
  if (!pNavRequest)
    JumpRequestedBlock = (dword) -1;

  if (!pJumpRequest)
  {
    if (JumpRequestedSegment != 0xFFFF)
    {
      JumpRequestedSegment = 0xFFFF;
      OSDSegmentListDrawList(FALSE);
    }
  }

  // neues Jump-Segment setzen
  if (pJumpRequest)
  {
    if (JumpRequestedSegment == 0xFFFF)
      JumpRequestedSegment = ActiveSegment;

    if (pBackwards)
    {
      if (JumpRequestedSegment > 0)
        JumpRequestedSegment--;
      else
        JumpRequestedSegment = NrSegmentMarker - 2;
    }
    else
    {
      if (JumpRequestedSegment < (NrSegmentMarker - 2))
        JumpRequestedSegment++;
      else
        JumpRequestedSegment = 0;
    }
    OSDSegmentListDrawList(FALSE);
  }
  
  // neue Navigations-Position setzen
  if (pNavRequest)
  {
    dword BlockJumpWidth;

    if (JumpRequestedBlock == (dword) -1)
      JumpRequestedBlock = PlayInfo.currentBlock;

    if (labs(TAP_GetTick() - LastNavTime) >= 500)
      LastDirection = 0;

    if (pBackwards)
    {
      if (LastDirection <= 0)
        LastDirection = max(LastDirection - 1, -NrFastNavSteps);
      else
        LastDirection = min(-LastDirection + 1, -1);
      BlockJumpWidth = (PlayInfo.totalBlock) / 100 * FastNavSteps[LastDirection + NrFastNavSteps];

      if (JumpRequestedBlock >= BlockJumpWidth)
        JumpRequestedBlock = JumpRequestedBlock - BlockJumpWidth;
      else
        JumpRequestedBlock = 0;
    }
    else
    {
      if (LastDirection >= 0)
        LastDirection = min(LastDirection + 1, NrFastNavSteps);
      else
        LastDirection = max(-LastDirection - 1, 1);
      BlockJumpWidth = (PlayInfo.totalBlock) / 100 * FastNavSteps[LastDirection + NrFastNavSteps];

      if (JumpRequestedBlock + BlockJumpWidth <= BlockNrLast10Seconds)
        JumpRequestedBlock = JumpRequestedBlock + BlockJumpWidth;
      else
        JumpRequestedBlock = (PlayInfo.currentBlock < BlockNrLast10Seconds) ? BlockNrLast10Seconds : (dword) -1;
    }
    LastNavTime = TAP_GetTick();
  }

  // Zeit der Aktion erfassen
  if (pJumpRequest || JumpRequestedBlock != (dword) -1)
  {
    JumpRequestedTime = TAP_GetTick();
    if (!JumpRequestedTime) JumpRequestedTime = 1;
  }

  // OSD aktualisieren
  OSDInfoDrawProgressbar(TRUE, TRUE);
  if (OSDMode == MD_NoOSD)
  {
    LastPlayStateChange = TAP_GetTick();
    if(!LastPlayStateChange) LastPlayStateChange = 1;
    OSDMode = MD_MiniOSD;
    OSDRedrawEverything();
    OSDMode = MD_NoOSD;
  }
  TRACEEXIT();
}

void Playback_JumpForward(void)
{
  dword                 NrJumpBlocks, JumpToBlock;

  TRACEENTER();
//  if(PLAYINFOVALID())  // Prüfung von currentBlock nun restriktiver
  if (PlayInfo.currentBlock < BlockNrLastSecond)
  {
    NrJumpBlocks = (PlayInfo.totalBlock / (60*PlayInfo.duration + PlayInfo.durationSec)) * MinuteJump*60;
    JumpToBlock  = min(PlayInfo.currentBlock + NrJumpBlocks, BlockNrLastSecond);

    if(TrickMode == TRICKMODE_Pause) Playback_Normal();
    TAP_Hdd_ChangePlaybackPos(JumpToBlock);
    JumpRequestedSegment = 0xFFFF;
    JumpRequestedBlock = (dword) -1;
  }
  TRACEEXIT();
}

void Playback_JumpBackward(void)
{
  dword                 NrJumpBlocks, JumpToBlock;

  TRACEENTER();
//  if(PLAYINFOVALID())  // Prüfung von currentBlock nun restriktiver
  NrJumpBlocks = (PlayInfo.totalBlock / (60*PlayInfo.duration + PlayInfo.durationSec)) * MinuteJump*60;
  if (((int)PlayInfo.currentBlock >= 0) && (PlayInfo.currentBlock >= NrJumpBlocks))
  {
    JumpToBlock = PlayInfo.currentBlock - NrJumpBlocks;

    if(TrickMode == TRICKMODE_Pause) Playback_Normal();
    TAP_Hdd_ChangePlaybackPos(JumpToBlock);
    JumpRequestedSegment = 0xFFFF;
    JumpRequestedBlock = (dword) -1;
  }
  TRACEEXIT();
}

void Playback_JumpNextSegment(void)
{
  TRACEENTER();

  if(NrSegmentMarker > 2)
  {
    if (ActiveSegment < NrSegmentMarker - 2)
      ActiveSegment++;
    else
      ActiveSegment = 0;

    if(TrickMode == TRICKMODE_Pause) Playback_Normal();
    TAP_Hdd_ChangePlaybackPos(SegmentMarker[ActiveSegment].Block);
    JumpRequestedSegment = 0xFFFF;
    JumpRequestedBlock = (dword) -1;
//    JumpRequestedTime = 0;
    JumpPerformedTime = TAP_GetTick();
    if(!JumpPerformedTime) JumpPerformedTime = 1;
    OSDSegmentListDrawList(TRUE);
  }

  TRACEEXIT();
}

void Playback_JumpPrevSegment(void)
{
  TRACEENTER();

  const dword FiveSeconds = PlayInfo.totalBlock * 5 / (60*PlayInfo.duration + PlayInfo.durationSec);

  if(NrSegmentMarker >= 2)
  {
    if (PlayInfo.currentBlock < (SegmentMarker[ActiveSegment].Block + FiveSeconds))
    {
      if (ActiveSegment > 0)
        ActiveSegment--;
      else
        ActiveSegment = NrSegmentMarker - 2;
    }

    if(TrickMode == TRICKMODE_Pause) Playback_Normal();
    TAP_Hdd_ChangePlaybackPos(SegmentMarker[ActiveSegment].Block);
    JumpRequestedSegment = 0xFFFF;
    JumpRequestedBlock = (dword) -1;
//    JumpRequestedTime = 0;
    JumpPerformedTime = TAP_GetTick();
    if(!JumpPerformedTime) JumpPerformedTime = 1;
    OSDSegmentListDrawList(TRUE);
  }

  TRACEEXIT();
}

void Playback_JumpNextBookmark(void)
{
  int i;

  TRACEENTER();

  if ((NrBookmarks > 0) && (PlayInfo.currentBlock > Bookmarks[NrBookmarks-1]))
  {
    if(TrickMode == TRICKMODE_Pause) Playback_Normal();
    TAP_Hdd_ChangePlaybackPos(Bookmarks[0]);
    JumpRequestedSegment = 0xFFFF;
    JumpRequestedBlock = (dword) -1;
    TRACEEXIT();
    return;
  }
  
  for(i = 0; i < NrBookmarks; i++)
  {
    if(Bookmarks[i] > PlayInfo.currentBlock)
    {
      if(TrickMode == TRICKMODE_Pause) Playback_Normal();
      TAP_Hdd_ChangePlaybackPos(Bookmarks[i]);
      JumpRequestedSegment = 0xFFFF;
      JumpRequestedBlock = (dword) -1;
      TRACEEXIT();
      return;
    }
  }

  TRACEEXIT();
}

void Playback_JumpPrevBookmark(void)
{
  TRACEENTER();

  const dword           FiveSeconds = PlayInfo.totalBlock * 5 / (60*PlayInfo.duration + PlayInfo.durationSec);
  dword                 JumpToBlock = PlayInfo.currentBlock;
  int                   i;

  if (NrBookmarks == 0)
    JumpToBlock = 0;
  else if (PlayInfo.currentBlock < Bookmarks[0] + FiveSeconds)
  {
//    if (PlayInfo.currentBlock >= FiveSeconds)
      JumpToBlock = 0;
//    else
//      JumpToBlock = Bookmarks[NrBookmarks - 1];
  }
  else
    for(i = NrBookmarks - 1; i >= 0; i--)
    {
      if((Bookmarks[i] + FiveSeconds) <= PlayInfo.currentBlock)
      {
        JumpToBlock = Bookmarks[i];
        break;
      }
    }
  if(TrickMode == TRICKMODE_Pause) Playback_Normal();
  TAP_Hdd_ChangePlaybackPos(JumpToBlock);
  JumpRequestedSegment = 0xFFFF;
  JumpRequestedBlock = (dword) -1;

  TRACEEXIT();
}


// ----------------------------------------------------------------------------
//                        MovieCutter-Funktionalität
// ----------------------------------------------------------------------------
void MovieCutterSaveSegments(void)
{
  TRACEENTER();

  if (NrSegmentMarker > 2)
  {
    WriteLogMC(PROGRAM_NAME, "[Action 'Save segments' started...]");
    MovieCutterProcess(TRUE);
  }

  TRACEEXIT();
}

void MovieCutterDeleteSegments(void)
{
  TRACEENTER();

  if (NrSegmentMarker > 2)
  {
    WriteLogMC(PROGRAM_NAME, "[Action 'Delete segments' started...]");
    MovieCutterProcess(FALSE);
  }

  TRACEEXIT();
}

void MovieCutterSelectOddSegments(void)
{
  if (NrSegmentMarker <= 2) return;

  TRACEENTER();

  int i;
  for(i = 0; i < NrSegmentMarker-1; i++)
    SegmentMarker[i].Selected = ((i & 1) == 0);

  OSDSegmentListDrawList(FALSE);
  OSDInfoDrawProgressbar(TRUE, TRUE);
//  OSDRedrawEverything();

  if (DirectSegmentsCut)
  {
    WriteLogMC(PROGRAM_NAME, "[Action 'Delete odd segments' selected...]");
    ActionMenuDraw();
    if (!AskBeforeEdit || ShowConfirmationDialog(LangGetString(LS_AskConfirmation)))
    {
      ActionMenuRemove();
      MovieCutterDeleteSegments();
    }
    else
    {
      State = ST_ActionMenu;
      ActionMenuDraw();
    }
  }
  else
  {
    State = ST_ActionMenu;
    ActionMenuDraw();
  }
  TRACEEXIT();
}

void MovieCutterSelectEvenSegments(void)
{
  TRACEENTER();

  int i;
  for(i = 0; i < NrSegmentMarker-1; i++)
    SegmentMarker[i].Selected = ((i & 1) == 1);

  OSDSegmentListDrawList(FALSE);
  OSDInfoDrawProgressbar(TRUE, TRUE);
//  OSDRedrawEverything();

  if (DirectSegmentsCut)
  {
    WriteLogMC(PROGRAM_NAME, "[Action 'Delete even segments' selected...]");
    ActionMenuDraw();
    if (!AskBeforeEdit || ShowConfirmationDialog(LangGetString(LS_AskConfirmation)))
    {
      ActionMenuRemove();
      MovieCutterDeleteSegments();
    }
    else
    {
      State = ST_ActionMenu;
      ActionMenuDraw();
    }
  }
  else
  {
    State = ST_ActionMenu;
    ActionMenuDraw();
  }
  TRACEEXIT();
}

void MovieCutterUnselectAll(void)
{
  TRACEENTER();

  int i;
  for(i = 0; i < NrSegmentMarker-1; i++)
    SegmentMarker[i].Selected = FALSE;

  OSDSegmentListDrawList(FALSE);
  OSDInfoDrawProgressbar(TRUE, TRUE);
//  OSDRedrawEverything();

  if (NrSegmentMarker > 2)
  {
    State = ST_ActionMenu;
    ActionMenuDraw();
  }
  TRACEEXIT();
}

void MovieCutterDeleteFile(void)
{
  TRACEENTER();

//  NoPlaybackCheck = FALSE;
  if (isPlaybackRunning())
  {
//    NoPlaybackCheck = TRUE;
    TAP_Hdd_StopTs();
  }
//  HDD_ChangeDir(PlaybackDir);
//  NoPlaybackCheck = TRUE;
  CutFileDelete();
  HDD_Delete2(PlaybackName, AbsPlaybackDir, TRUE);
//  NoPlaybackCheck = FALSE;

  Cleanup(TRUE);
  State = AutoOSDPolicy ? ST_WaitForPlayback : ST_InactiveMode;

  TRACEEXIT();
}

void MovieCutterProcess(bool KeepCut)
{
//  int                   NrSelectedSegments;
  bool                  isMultiSelect, CutEnding;
  int                   WorkingSegment;
  int                   maxProgress; 
  char                  CutFileName[MAX_FILE_NAME_SIZE + 1];
  char                  TempFileName[MAX_FILE_NAME_SIZE + 1];
  char                  MessageString[512];
  tTimeStamp            CutStartPoint, BehindCutPoint;
  dword                 DeltaBlock; //, DeltaTime;
  dword                 CurPlayPosition;
  char                  DeviceNode[20], CommandLine[512], InodeNrs[768];
  __ino64_t             InodeNr = 0;
  __off64_t             FileSize = 0;
  int                   icheckErrors = -2;
  int                   i, j;
  tResultCode           ret = RC_Error;

  TRACEENTER();

//  NoPlaybackCheck = TRUE;
#ifdef FULLDEBUG
  char CurDir[FBLIB_DIR_SIZE];
  HDD_TAP_GetCurrentDir(CurDir);
  WriteLogMCf(PROGRAM_NAME, "MovieCutterProcess(): CurrentDir='%s'", CurDir);
#endif
//  HDD_ChangeDir(PlaybackDir);
  UndoResetStack();

  // *CW* FRAGE: Werden die Bookmarks von der Firmware sowieso vor dem Schneiden in die inf gespeichert?
  // -> sonst könnte man der Schnittroutine auch das Bookmark-Array übergeben
  SaveBookmarksToInf(PlaybackName, AbsPlaybackDir, Bookmarks, NrBookmarks);
  CutDumpList();

  // Lege ein Backup der .cut-Datei an
  if (SaveCutBak)
  {
    CutFileSave();
    char CutName[MAX_FILE_NAME_SIZE + 1], BackupCutName[MAX_FILE_NAME_SIZE + 1];
    TAP_SPrint(CutName, sizeof(CutName), "%s", PlaybackName);
    TAP_SPrint(&CutName[strlen(CutName) - 4], 5, ".cut");
    TAP_SPrint(BackupCutName, sizeof(BackupCutName), "%s.bak", CutName);
    if (HDD_Exist2(BackupCutName, AbsPlaybackDir)) HDD_Delete2(BackupCutName, AbsPlaybackDir, FALSE);
    HDD_Rename2(CutName, BackupCutName, AbsPlaybackDir, FALSE);
  }
  CutFileSave();
//  TAP_SPrint(LogString, sizeof(LogString), "cp \"%s/%s.cut\" \"%s/%s.cut.bak\"", AbsPlaybackDir, BackupCutName, AbsPlaybackDir, BackupCutName);
//  system(LogString);

  // Zähle die ausgewählten Segmente
  isMultiSelect = FALSE;
  NrSelectedSegments = 0;
  for(i = 0; i < NrSegmentMarker - 1; i++)
  {
    if(SegmentMarker[i].Selected)
      NrSelectedSegments++;
  }
  isMultiSelect = (NrSelectedSegments > 0);
  if (!NrSelectedSegments) NrSelectedSegments = 1;

  maxProgress = NrSelectedSegments;
  OSDMenuSaveMyRegion(rgnSegmentList);
  TAP_SPrint(MessageString, sizeof(MessageString), LangGetString(LS_Cutting), 0, maxProgress);
  OSDMenuProgressBarShow(PROGRAM_NAME, MessageString, 0, maxProgress + ((CheckFSAfterCut==2) ? 1 : 0), NULL);
  CurPlayPosition = PlayInfo.currentBlock;
WriteLogMCf(PROGRAM_NAME, "Debug: Speichere CurPlayPosition = %lu", CurPlayPosition);

// Aufnahmenfresser-Test und Ausgabe
HDD_FindMountPointDev2(AbsPlaybackDir, NULL, DeviceNode);
//TAP_SPrint(CommandLine, sizeof(CommandLine), "mount -o remount,rw,integrity %s", DeviceNode);
//system(CommandLine);
WriteDebugLog("======================\n");
WriteDebugLog("MC-Version: %s, FBLib: %s, SpecialEnd %s, DoiCheckTest=%d, CheckFSAfterCut=%d, InodeMonitoring=%d", VERSION, __FBLIB_VERSION__, (DisableSpecialEnd) ? "nicht aktiv" : "aktiv", DoiCheckTest, CheckFSAfterCut, InodeMonitoring);
FILE *MountState;
TAP_SPrint(CommandLine, sizeof(CommandLine), "mount | grep '%s on'", DeviceNode);
MountState = popen(CommandLine, "r");
if(MountState)
{
  fgets(CommandLine, sizeof(CommandLine), MountState);
  WriteDebugLog("Device: %s, gemountet als: %s. (%s)", DeviceNode, ((CommandLine[0] && !strstr(CommandLine, "nointegrity")) ? "integrity" : ((CommandLine[0]) ? "nointegrity" : "unbekannt")), RemoveEndLineBreak(CommandLine));
}
if (HDD_GetFileSizeAndInode2(PlaybackName, AbsPlaybackDir, &InodeNr, &FileSize))
  WriteDebugLog("Source:   '%s/%s', inode=%llu, size=%llu", AbsPlaybackDir, PlaybackName, InodeNr, FileSize);
if (DoiCheckTest >= 3)
{
  icheckErrors = 0;
  if (!HDD_CheckInode(PlaybackName, AbsPlaybackDir, FALSE, InodeMonitoring /*, "Original" */))
    icheckErrors++;
}
WriteDebugLog("%d Schnittoperation(en) vorgesehen.", NrSelectedSegments);
WriteDebugLog("---------------");

  if (HDD_GetFileSizeAndInode2(PlaybackName, AbsPlaybackDir, &InodeNr, NULL))
    TAP_SPrint(InodeNrs, sizeof(InodeNrs), "%llu", InodeNr);

  for(i = NrSegmentMarker - 2; i >= 0 /*-1*/; i--)
  {
    if(isMultiSelect)
    {
      //If one or more segments have been selected, work with them.
      WorkingSegment = i;

      // Process the ending at last (*experimental!*)
/*      if (i == NrSegmentMarker - 2) {
        if (SegmentMarker[i].Selected) NrSelectedSegments--;
        continue;
      }
      if (i == -1)
      {
        WorkingSegment = NrSegmentMarker - 2;
        NrSelectedSegments = 1;
      }  */
    }
    else
      //If no segment has been selected, use the active segment and break the loop
      WorkingSegment = ActiveSegment;

    if(!isMultiSelect || SegmentMarker[WorkingSegment].Selected)
    {
//      NoPlaybackCheck = FALSE;
      if (isPlaybackRunning())
      {
//        NoPlaybackCheck = TRUE;
        TAP_Hdd_StopTs();
      }
//      NoPlaybackCheck = TRUE;
//      HDD_ChangeDir(PlaybackDir);

      WriteLogMCf(PROGRAM_NAME, "Processing segment %d", WorkingSegment);

      // Ermittlung der Schnittpositionen
      CutStartPoint.BlockNr = SegmentMarker[WorkingSegment].Block;
      CutStartPoint.Timems = SegmentMarker[WorkingSegment].Timems;
      BehindCutPoint.BlockNr = SegmentMarker[WorkingSegment + 1].Block;
      BehindCutPoint.Timems = SegmentMarker[WorkingSegment + 1].Timems;

      // NEUE Spezial-Behandlung beim Schneiden des Endes
      CutEnding = FALSE;
      if (WorkingSegment == NrSegmentMarker - 2)
      {
        if (!DisableSpecialEnd)
        {
//        if (KeepCut)
          //letztes Segment soll gespeichert werden -> versuche bis zum tatsächlichen Ende zu gehen
//          BehindCutPoint.BlockNr = 0xFFFFFFFF;
//        else
          //letztes Segment soll geschnitten werden -> speichere stattdessen den vorderen Teil der Aufnahme und tausche mit dem Original
          CutEnding = TRUE;
          CutStartPoint.BlockNr  = SegmentMarker[0].Block;
          CutStartPoint.Timems   = SegmentMarker[0].Timems;
          BehindCutPoint.BlockNr = SegmentMarker[NrSegmentMarker-2].Block;
          BehindCutPoint.Timems  = SegmentMarker[NrSegmentMarker-2].Timems;
          WriteLogMC(PROGRAM_NAME, "(* first special mode for cut ending *)");
          WriteDebugLog("EndSchnitt - special mode:");
        }
        else if (KeepCut)
          BehindCutPoint.BlockNr = 0xFFFFFFFF;  //letztes Segment soll gespeichert werden -> versuche bis zum tatsächlichen Ende zu gehen
      }

      // Ermittlung des Dateinamens für das CutFile
      GetNextFreeCutName(PlaybackName, CutFileName, AbsPlaybackDir, NrSelectedSegments - 1);
      if (CutEnding)
      {
        strncpy(TempFileName, PlaybackName, sizeof(TempFileName));
        TAP_SPrint(&TempFileName[strlen(PlaybackName) - 4], 10, "_temp%s", &PlaybackName[strlen(PlaybackName) - 4]);
        HDD_Delete2(TempFileName, AbsPlaybackDir, TRUE);
      }

      //Flush the caches *experimental*
      sync();
      TAP_Sleep(1);
//      HDD_ChangeDir(PlaybackDir);

      // Schnittoperation
      ret = MovieCutter(PlaybackName, ((CutEnding) ? TempFileName : CutFileName), AbsPlaybackDir, &CutStartPoint, &BehindCutPoint, (TRUE || KeepCut || CutEnding), HDVideo);
WriteLogMCf(PROGRAM_NAME, "Debug: MovieCutter() returned: %d", ret);

      if (HDD_GetFileSizeAndInode2(((CutEnding) ? TempFileName : CutFileName), AbsPlaybackDir, &InodeNr, NULL))
        TAP_SPrint(&InodeNrs[strlen(InodeNrs)], sizeof(InodeNrs)-strlen(InodeNrs), " %llu", InodeNr);

// Aufnahmenfresser-Test und Ausgabe
WriteLogMC(PROGRAM_NAME, "Debug: vor icheck-Test");
if (HDD_GetFileSizeAndInode2(((CutEnding) ? TempFileName : CutFileName), AbsPlaybackDir, &InodeNr, &FileSize))
  WriteDebugLog("Cut file: '%s/%s', inode=%llu, size=%llu", AbsPlaybackDir, ((CutEnding) ? TempFileName : CutFileName), InodeNr, FileSize);
if (HDD_GetFileSizeAndInode2(PlaybackName, AbsPlaybackDir, &InodeNr, &FileSize))
  WriteDebugLog("RestFile: '%s/%s', inode=%llu, size=%llu", AbsPlaybackDir, PlaybackName, InodeNr, FileSize);
if (DoiCheckTest >= 3)
{
  if (!HDD_CheckInode(((CutEnding) ? TempFileName : CutFileName), AbsPlaybackDir, (DoiCheckTest==4), InodeMonitoring /*, "CutFile" */))
    icheckErrors++;
  if (!HDD_CheckInode(PlaybackName, AbsPlaybackDir, (DoiCheckTest==4), InodeMonitoring /*, "RestFile" */))
    icheckErrors++;
}
if (!KeepCut && !CutEnding)
  HDD_Delete2(CutFileName, AbsPlaybackDir, TRUE);  // wenn diese Zeile gelöscht wird, stattdessen das TRUE im MovieCutter-Aufruf wieder rausnehmen
WriteLogMC(PROGRAM_NAME, "Debug: nach icheck-Test");

// INFplus
if (CutEnding || KeepCut)
{
  __ino64_t OldInodeNr, NewInodeNr;
  char InfFileName[MAX_FILE_NAME_SIZE + 1];
  TAP_SPrint(InfFileName, sizeof(InfFileName), "%s.inf", PlaybackName);
  if (HDD_GetFileSizeAndInode2(InfFileName, AbsPlaybackDir, &OldInodeNr, NULL))
  {
    TAP_SPrint(InfFileName, sizeof(InfFileName), "%010llu.INF+", OldInodeNr);
    if (HDD_Exist2(InfFileName, TAPFSROOT "/ProgramFiles/Settings/INFplus"))
    {
      TAP_SPrint(InfFileName, sizeof(InfFileName), "%s.inf", (CutEnding) ? TempFileName : CutFileName);
      if (HDD_GetFileSizeAndInode2(InfFileName, AbsPlaybackDir, &NewInodeNr, NULL))
      {
        TAP_SPrint(CommandLine, sizeof(CommandLine), "%s %s/ProgramFiles/Settings/INFplus/%010llu.INF+ %s/ProgramFiles/Settings/INFplus/%010llu.INF+", (KeepCut) ? "cp" : "mv", TAPFSROOT, OldInodeNr, TAPFSROOT, NewInodeNr);
        system(CommandLine);
      }
    }
  }
}

WriteLogMC(PROGRAM_NAME, "Debug: vor Renaming");
      // Das erzeugte CutFile wird zum neuen SourceFile
      if (CutEnding)
      {
        RecFileSize = 0;
        if(HDD_Exist2(TempFileName, AbsPlaybackDir))
          if(!HDD_GetFileSizeAndInode2(TempFileName, AbsPlaybackDir, NULL, &RecFileSize))
            RecFileSize = 0;

        if (ret && RecFileSize > 0)
        {
          if (KeepCut)
          {
            WriteLogMCf(PROGRAM_NAME, "Renaming the end-segment file '%s' to '%s'", PlaybackName, CutFileName);
            HDD_Rename2(PlaybackName, CutFileName, AbsPlaybackDir, TRUE);
          }
          else
            HDD_Delete2(PlaybackName, AbsPlaybackDir, TRUE);
          if (!HDD_Exist2(PlaybackName, AbsPlaybackDir))
          {
            WriteLogMCf(PROGRAM_NAME, "Renaming original recording '%s' back to '%s'", TempFileName, PlaybackName);
            HDD_Rename2(TempFileName, PlaybackName, AbsPlaybackDir, TRUE);
          }
        }
      }
WriteLogMC(PROGRAM_NAME, "Debug: nach Renaming");

      // Überprüfung von Existenz und Größe der geschnittenen Aufnahme
      RecFileSize = 0;
      if(HDD_Exist2(PlaybackName, AbsPlaybackDir))
        if(!HDD_GetFileSizeAndInode2(PlaybackName, AbsPlaybackDir, NULL, &RecFileSize))
          RecFileSize = 0;
      WriteLogMCf(PROGRAM_NAME, "Size of the new playback file (after cut): %llu", RecFileSize);

      // Wiedergabe wird neu gestartet
      if (RecFileSize > 0)
      {
//        TAP_Hdd_PlayTs(PlaybackName);
WriteLogMCf(PROGRAM_NAME, "Debug: HDD_StartPlayback2('%s', '%s')", PlaybackName, AbsPlaybackDir);
        HDD_StartPlayback2(PlaybackName, AbsPlaybackDir);
        PlayInfo.totalBlock = 0;
        j = 0;
        while ((j < 2000) && (!isPlaybackRunning() || (int)PlayInfo.totalBlock <= 0 || (int)PlayInfo.currentBlock < 0))  // 2000 ~ 30 sek. (750 ~ 10 sek.)
        {
          TAP_SystemProc();
          j++;
        }
WriteLogMCf(PROGRAM_NAME, "Debug: Playback gestartet (j=%d, isPlaybackRunning=%s, TotalBlock=%lu, CurrentBlock=%lu)", j, ((isPlaybackRunning()) ? "true" : "false"), PlayInfo.totalBlock, PlayInfo.currentBlock);
        if (PlayInfo.playMode == PLAYMODE_Playing)
          PlaybackRepeatSet(TRUE);
//        HDD_ChangeDir(PlaybackDir);

        WriteLogMCf(PROGRAM_NAME, "Reported new totalBlock = %lu", PlayInfo.totalBlock);
      }

      //Bail out if the cut failed
      if((ret == RC_Error) || (RecFileSize == 0) || ((int)PlayInfo.totalBlock <= 0))
      {
        WriteLogMC(PROGRAM_NAME, "MovieCutterProcess: Cutting process failed or totalBlock is 0!");
        State = ST_UnacceptedFile;
        OSDMenuProgressBarDestroyNoOSDUpdate();
        ShowErrorMessage(LangGetString(LS_CutHasFailed), NULL);
        break;
      }

      // Anpassung der verbleibenden Segmente
      SegmentMarker[WorkingSegment].Selected = FALSE;
      if (WorkingSegment + 1 < NrSegmentMarker - 1)
        DeleteSegmentMarker(WorkingSegment + 1);  // das letzte Segment darf nicht gelöscht werden
      else
      {
        NrSegmentMarker--;
        if(ActiveSegment >= NrSegmentMarker - 1) ActiveSegment = NrSegmentMarker - 2;
      }
      NrSelectedSegments--;

//      if (CutEnding) {
//        DeltaBlock = CalcBlockSize(CutFileSize);
//        DeltaTime = (!LinearTimeMode) ? (TimeStamps[NrTimeStamps-1].Timems - BehindCutPoint.Timems) : NavGetBlockTimeStamp(DeltaBlock);
//      } else {
//        DeltaBlock = BehindCutPoint.BlockNr - CutStartPoint.BlockNr;
//        DeltaTime = BehindCutPoint.Timems - CutStartPoint.Timems;
//      }

      DeltaBlock = BehindCutPoint.BlockNr - CutStartPoint.BlockNr;
      for(j = NrSegmentMarker - 1; j > 0; j--)
      {
        if(j == WorkingSegment)
        {
          // das aktuelle Segment auf die tatsächliche Schnittposition setzen
          SegmentMarker[WorkingSegment].Block =  ((!CutEnding) ? CutStartPoint : BehindCutPoint).BlockNr;
          SegmentMarker[WorkingSegment].Timems = ((!CutEnding) ? CutStartPoint : BehindCutPoint).Timems;
        }
        else if(SegmentMarker[j].Block + 11 >= BehindCutPoint.BlockNr)
        {
          // nachfolgende Semente verschieben
          SegmentMarker[j].Block -= min(DeltaBlock, SegmentMarker[j].Block);
          SegmentMarker[j].Timems = NavGetBlockTimeStamp(SegmentMarker[j].Block);  // -= min(DeltaTime, SegmentMarker[j].Timems);
        }
        SegmentMarker[j].Percent = ((float)SegmentMarker[j].Block / PlayInfo.totalBlock) * 100.0;

        //If the first marker has moved to block 0, delete it
//        if(SegmentMarker[j].Block <= 1) DeleteSegmentMarker(j);  // Annahme: ermittelte DeltaBlocks weichen nur um höchstens 1 ab
      }
    
      // das letzte Segment auf den gemeldeten TotalBlock-Wert setzen
      if(SegmentMarker[NrSegmentMarker-1].Block != PlayInfo.totalBlock)
      {
        #ifdef FULLDEBUG
          WriteLogMCf(PROGRAM_NAME, "MovieCutterProcess: Letzter Segment-Marker %lu ist ungleich TotalBlock %lu!", SegmentMarker[NrSegmentMarker - 1].Block, PlayInfo.totalBlock);
        #endif
        SegmentMarker[NrSegmentMarker-1].Block = PlayInfo.totalBlock;
        dword newTime = NavGetBlockTimeStamp(SegmentMarker[NrSegmentMarker-1].Block);
        SegmentMarker[NrSegmentMarker-1].Timems = (newTime) ? newTime : (dword)(1000 * (60*PlayInfo.duration + PlayInfo.durationSec));
        SegmentMarker[NrSegmentMarker-1].Percent = 100;
      }

      // Letzte Playback-Position anpassen
      if(CurPlayPosition)
      {
        if (!CutEnding)
        {
          if (CurPlayPosition >= BehindCutPoint.BlockNr)
            CurPlayPosition -= min(DeltaBlock, CurPlayPosition);
          else if (CurPlayPosition >= CutStartPoint.BlockNr)
            CurPlayPosition = /*(WorkingSegment < NrSegmentMarker - 1) ?*/ CutStartPoint.BlockNr;
          CalcLastSeconds();
          if (CurPlayPosition > BlockNrLastSecond)
            CurPlayPosition = 0;
        }
        else
          if (CurPlayPosition >= BehindCutPoint.BlockNr)
            CurPlayPosition = 0;
      }
WriteLogMCf(PROGRAM_NAME, "Debug: Ändere CurPlayPosition = %lu", CurPlayPosition);

      // Wenn Spezial-Crop-Modus, nochmal testen, ob auch mit der richtigen rec weitergemacht wird
      if(CutEnding)
      {
        if(/*(ret==RC_Warning) || !HDD_Exist2(PlaybackName, PlaybackDir) ||*/ HDD_Exist2(TempFileName, AbsPlaybackDir))
        {
          State = ST_UnacceptedFile;
          WriteLogMC(PROGRAM_NAME, "Error processing the last segment: Renaming failed!");
          OSDMenuProgressBarDestroyNoOSDUpdate();
          ShowErrorMessage(LangGetString(LS_CutHasFailed), NULL);
          break;
        }
      }

      //Bail out if the currentBlock could not be detected
      if((int)PlayInfo.currentBlock < 0)
      {
        WriteLogMC(PROGRAM_NAME, "MovieCutterProcess: Restarting playback failed! CurrentBlock not detected...");
        State = ST_UnacceptedFile;
        OSDMenuProgressBarDestroyNoOSDUpdate();
        ShowErrorMessage(LangGetString(LS_CutHasFailed), NULL);
        break;
      }

      JumpRequestedSegment = 0xFFFF;
      JumpRequestedBlock = (dword) -1;
      NrBookmarks = 0;
      OSDSegmentListDrawList(FALSE);
      OSDInfoDrawProgressbar(TRUE, TRUE);

TAP_PrintNet("Aktueller Prozentstand: %d von %d\n", maxProgress - NrSelectedSegments, maxProgress);
      if (OSDMenuProgressBarIsVisible())
      {
        OSDMenuSaveMyRegion(rgnSegmentList);
        TAP_SPrint(MessageString, sizeof(MessageString), LangGetString(LS_Cutting), maxProgress - NrSelectedSegments, maxProgress);
        OSDMenuProgressBarShow(PROGRAM_NAME, MessageString, maxProgress - NrSelectedSegments, maxProgress + ((CheckFSAfterCut==2) ? 1 : 0), NULL);
      }
    }
    if ((NrSelectedSegments <= 0 /* && !SegmentMarker[NrSegmentMarker-2].Selected*/) || (NrSegmentMarker <= 2))
      break;
  }
  CutFileSave();
  CutDumpList();

//  TAP_Osd_Sync();
//  ClearOSD(FALSE);
//  OSDRedrawEverything();

  //Flush the caches *experimental*
  sync();
  TAP_Sleep(1);

  // Check the modified Inodes with jfs_icheck
  if (DoiCheckTest == 1 || DoiCheckTest == 2)
  {
    if (OSDMenuProgressBarIsVisible())
      OSDMenuProgressBarShow(PROGRAM_NAME, LangGetString(LS_CheckingFileSystem), maxProgress - NrSelectedSegments, maxProgress + ((CheckFSAfterCut==2) ? 1 : 0), NULL);
    icheckErrors = HDD_CheckInodes(InodeNrs, AbsPlaybackDir, (DoiCheckTest==2), InodeMonitoring);
  }
WriteLogMCf(PROGRAM_NAME, "Debug: DoiCheckTest=%d, CheckFSAfterCut=%d, InodeMonitoring=%d, icheckErrors=%d", DoiCheckTest, CheckFSAfterCut, InodeMonitoring, icheckErrors);

  //Check file system consistency and show a warning
  if ((CheckFSAfterCut == 2) || (CheckFSAfterCut != 0 && icheckErrors))
  {
    WriteLogMCf(PROGRAM_NAME, "Inodes-Check mit fsck: %s", InodeNrs);
    WriteDebugLog(            "Inodes-Check mit fsck: %s", InodeNrs);
    if (CheckFSAfterCut == 2)
      CheckFileSystem(maxProgress, maxProgress + 1, maxProgress + 1, TRUE, TRUE, TRUE, TRUE, icheckErrors, InodeNrs);
    else
      CheckFileSystem(maxProgress, maxProgress, maxProgress, TRUE, TRUE, TRUE, TRUE, icheckErrors, InodeNrs);
//    if (CurPlayPosition > 0)
//      TAP_Hdd_ChangePlaybackPos(CurPlayPosition);
  }
  else if (DoiCheckTest == 0)
  {
    WriteLogMC(PROGRAM_NAME, "Cut files have not been verified!");
    if(OSDMenuProgressBarIsVisible()) OSDMenuProgressBarDestroyNoOSDUpdate();
    ShowErrorMessage(LangGetString(LS_FilesNotVerified), LangGetString(LS_Warning));
  }
  else if (icheckErrors != 0)
  {
    char SuspectFilesStr[12];
    if (icheckErrors >= 0)
      TAP_SPrint(SuspectFilesStr, sizeof(SuspectFilesStr), "%d", icheckErrors);
    else if (icheckErrors == -1)
      TAP_SPrint(SuspectFilesStr, sizeof(SuspectFilesStr), "1 or more");
    else
      TAP_SPrint(SuspectFilesStr, sizeof(SuspectFilesStr), "?");

    WriteLogMCf(PROGRAM_NAME, "%s suspect files found with icheck-Test!", SuspectFilesStr);
    if(OSDMenuProgressBarIsVisible()) OSDMenuProgressBarDestroyNoOSDUpdate();
    TAP_SPrint(MessageString, sizeof(MessageString), LangGetString(LS_SuspectFilesFound), SuspectFilesStr);
    ShowErrorMessage(MessageString, LangGetString(LS_Warning));
  }
//  HDD_ChangeDir(PlaybackDir);

  if (OSDMenuProgressBarIsVisible())
  {
    TAP_SPrint(MessageString, sizeof(MessageString), LangGetString(LS_Cutting), maxProgress - NrSelectedSegments, maxProgress);
    OSDMenuProgressBarShow(PROGRAM_NAME, MessageString, maxProgress, maxProgress, NULL);
    OSDMenuProgressBarDestroyNoOSDUpdate();
//    TAP_Osd_Sync();
  }

  if (isPlaybackRunning())
  {
    if(TrickMode == TRICKMODE_Pause) Playback_Normal();
WriteLogMCf(PROGRAM_NAME, "Debug: Springe jetzt zu CurPlayPosition = %lu", CurPlayPosition);
    if (CurPlayPosition > 0)
      TAP_Hdd_ChangePlaybackPos(CurPlayPosition);
    PlaybackRepeatSet(OldRepeatMode);
  }

  if (State == ST_UnacceptedFile)
  {
    LastTotalBlocks = PlayInfo.totalBlock;
    ClearOSD(TRUE);
    WriteLogMC(PROGRAM_NAME, "MovieCutterProcess() finished!");
  }
  else
  {
    State = ST_WaitForPlayback;
//    ClearOSD(TRUE);
    WriteLogMC(PROGRAM_NAME, "MovieCutterProcess() successfully completed!");
  }
//  NoPlaybackCheck = FALSE;

  TRACEEXIT();
}


// ----------------------------------------------------------------------------
//                              Hilfsfunktionen
// ----------------------------------------------------------------------------

// altes Verhalten von isPlaybackRunning():
// - holt IMMER die PlayInfo, liefert TRUE wenn Playing, bei ungültigem currentBlock wird dieser 0 gesetzt und TRUE zurückgegeben, bei NoPlaybackCheck wird TRUE zurückgegeben
// neues Verhalten von isPlaybackRunning():
// - bei NoPlaybackCheck wird TRUE zurückgegeben und KEINE PlayInfo gelesen, liefert TRUE wenn Playing, bei ungültigem currentBlock wird TRUE zurückgegeben und NICHT 0 gesetzt
// Grund: currentBlock kann ungültig werden, wenn man z.B. zum Anfang springt, das ist aber trotzdem laufende Wiedergabe
bool isPlaybackRunning(void)
{
  TRACEENTER();

  TAP_Hdd_GetPlayInfo(&PlayInfo);

//  if(NoPlaybackCheck) {
//    WriteLogMC(PROGRAM_NAME, "************NoPlaybackCheck");
//    TRACEEXIT();
//    return TRUE;  // WARUM!?
//  }

//  if((int)PlayInfo.currentBlock < 0) PlayInfo.currentBlock = 0;   *** kritisch ***

  if (PlayInfo.playMode == PLAYMODE_Playing)
  {
    TrickMode = (TYPE_TrickMode)PlayInfo.trickMode;
    TrickModeSpeed = PlayInfo.speed;
  }

  TRACEEXIT();
  return (PlayInfo.playMode == PLAYMODE_Playing);
}

void CalcLastSeconds(void)
{
  TRACEENTER();
  if((int)PlayInfo.totalBlock > 0  /*PLAYINFOVALID()*/)  // wenn nur totalBlock gesetzt ist, kann (und muss!) die Berechnung stattfinden
  {
    BlocksOneSecond      = PlayInfo.totalBlock / (60*PlayInfo.duration + PlayInfo.durationSec);
    BlockNrLastSecond    = PlayInfo.totalBlock - BlocksOneSecond;
    BlockNrLast10Seconds = PlayInfo.totalBlock - (10 * PlayInfo.totalBlock / (60*PlayInfo.duration + PlayInfo.durationSec));
  }
  TRACEEXIT();
}

void CheckLastSeconds(void)
{
  static bool LastSecondsPaused = FALSE;

  TRACEENTER();

  if((PlayInfo.currentBlock > BlockNrLastSecond) && (TrickMode != TRICKMODE_Pause))
  {
    if (!LastSecondsPaused) Playback_Pause();
    LastSecondsPaused = TRUE;
  }
  else if((PlayInfo.currentBlock > BlockNrLast10Seconds) && (TrickMode == TRICKMODE_Forward)) 
    Playback_Normal();
  else
    LastSecondsPaused = FALSE;

  TRACEEXIT();
}


// ----------------------------------------------------------------------------
//                            NAV-Funktionen
// ----------------------------------------------------------------------------
dword NavGetBlockTimeStamp(dword PlaybackBlockNr)
{
  TRACEENTER();

  if(LinearTimeMode)
  {
    TRACEEXIT();
    return ((dword) (((float)PlaybackBlockNr / PlayInfo.totalBlock) * (60*PlayInfo.duration + PlayInfo.durationSec)) * 1000);
  }

  if(TimeStamps == NULL)
  {
    WriteLogMC(PROGRAM_NAME, "Someone is trying to get a timestamp while the array is empty!");
    TRACEEXIT();
    return 0;
  }

  if(LastTimeStamp->BlockNr < PlaybackBlockNr)
  {
    // Search the TimeStamp-Array in forward direction
    while((LastTimeStamp->BlockNr < PlaybackBlockNr) && (LastTimeStamp < TimeStamps + NrTimeStamps-1))
      LastTimeStamp++;
    if(LastTimeStamp->BlockNr > PlaybackBlockNr)
      LastTimeStamp--;
  }
  else if(LastTimeStamp->BlockNr > PlaybackBlockNr)
  {
    // Search the TimeStamp-Array in reverse direction
    while((LastTimeStamp->BlockNr > PlaybackBlockNr) && (LastTimeStamp > TimeStamps))
      LastTimeStamp--;
    if(LastTimeStamp->BlockNr > PlaybackBlockNr)
    {
      TRACEEXIT();
      return 0;
    }
  }

  TRACEEXIT();
  return LastTimeStamp->Timems;
}

// ----------------------------------------------------------------------------
//                           Compatibility-Layer
// ----------------------------------------------------------------------------
// Die Implementierung berücksichtigt (aus Versehen) auch negative Sprünge und Überlauf.
// SOLLTE eine nav-Datei einen Überlauf beinhalten, wird dieser durch PatchOldNavFile korrigiert.
bool PatchOldNavFile(const char *RecFileName, const char *AbsDirectory, bool isHD)
{
  FILE                 *fSourceNav = NULL;
  FILE                 *fNewNav = NULL;
  char                  NavFileName[MAX_FILE_NAME_SIZE + 1];
  char                  BakFileName[MAX_FILE_NAME_SIZE + 1];
  tnavSD               *navRecs = NULL;
  size_t                navsRead, i;
  char                  AbsFileName[FBLIB_DIR_SIZE];

  TRACEENTER();

//  HDD_ChangeDir(PlaybackDir);
  TAP_SPrint(NavFileName, sizeof(NavFileName), "%s.nav", RecFileName);
  TAP_SPrint(BakFileName, sizeof(BakFileName), "%s.nav.bak", RecFileName);

  //If nav already patched -> exit function
  if (HDD_Exist2(BakFileName, AbsDirectory))
  {
    TRACEEXIT();
    return FALSE;
  }

  WriteLogMC(PROGRAM_NAME, "Checking source nav file (possibly older version with incorrect Times)...");

  // Allocate the buffer
  navRecs = (tnavSD*) TAP_MemAlloc(NAVRECS_SD * sizeof(tnavSD));
  if (!navRecs)
  {
    WriteLogMC(PROGRAM_NAME, "PatchOldNavFile() E1a01.");
    TRACEEXIT();
    return FALSE;
  }

  //Rename the original nav file to bak
  HDD_Rename2(NavFileName, BakFileName, AbsDirectory, FALSE);

  //Open the original nav
  TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s/%s", AbsDirectory, BakFileName);
  fSourceNav = fopen(AbsFileName, "rb");
  if(!fSourceNav)
  {
    WriteLogMC(PROGRAM_NAME, "PatchOldNavFile() E1a02.");
    TAP_MemFree(navRecs);
    TRACEEXIT();
    return FALSE;
  }

  //Create and open the new source nav
  TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s/%s", AbsDirectory, NavFileName);
  fNewNav = fopen(AbsFileName, "wb");
  if(!fNewNav)
  {
    fclose(fSourceNav);
    WriteLogMC(PROGRAM_NAME, "PatchOldNavFile() E1a03.");
    TAP_MemFree(navRecs);
    TRACEEXIT();
    return FALSE;
  }

  //Loop through the nav
  dword Difference = 0;
  dword LastTime = 0;
  size_t navsCount = 0;
  bool FirstRun = TRUE;

  while(TRUE)
  {
    navsRead = fread(navRecs, sizeof(tnavSD), NAVRECS_SD, fSourceNav);
    if(navsRead == 0) break;

    // Versuche, nav-Dateien aus Timeshift-Aufnahmen zu unterstützen ***experimentell***
    if(FirstRun)
    {
      FirstRun = FALSE;
      if(navRecs[0].SHOffset == 0x72767062)  // 'bpvr'
      {
        fseek(fSourceNav, 1056, SEEK_SET);
        continue;
      }
    }

    for(i = 0; i < navsRead; i++)
    {
      // Falls HD: Betrachte jeden tnavHD-Record als 2 tnavSD-Records, verwende den ersten und überspringe den zweiten
      if (isHD && (i % 2 != 0)) continue;

      if (navRecs[i].Timems - LastTime >= 3000)
      {
        Difference += (navRecs[i].Timems - LastTime) - 1000;
        WriteLogMCf(PROGRAM_NAME, "  - Gap found at nav record nr. %u:  Offset=%llu, TimeStamp(before)=%lu, TimeStamp(after)=%lu, GapSize=%lu", navsCount /*(ftell(fSourceNav)/sizeof(tnavSD) - navsRead + i) / ((isHD) ? 2 : 1)*/, ((off_t)(navRecs[i].PHOffsetHigh) << 32) | navRecs[i].PHOffset, navRecs[i].Timems, navRecs[i].Timems-Difference, navRecs[i].Timems-LastTime);
      }
      LastTime = navRecs[i].Timems;
      navRecs[i].Timems -= Difference;
      navsCount++;
    }
    fwrite(navRecs, sizeof(tnavSD), navsRead, fNewNav);
  }

  fclose(fSourceNav);
  fclose(fNewNav);
  TAP_MemFree(navRecs);

  TRACEEXIT();
  return (Difference > 0);
}


// ----------------------------------------------------------------------------
//                           System-Funktionen
// ----------------------------------------------------------------------------
bool CheckFileSystem(dword ProgressStart, dword ProgressEnd, dword ProgressMax, bool DoFix, bool Quick, bool NoOkInfo, bool ErrorMessage, int SuspectFiles, char *InodeNrs)
{
  char                  ErrorStrFmt[512], SuspectFilesStr[12];
  char                 *MountPath = NULL;
  dword                 OldSysState, OldSysSubState;
  bool                  ret = FALSE;
  
  TRACEENTER();
  TAP_GetState(&OldSysState, &OldSysSubState);

  // ProgressBar initialisieren
  OSDMenuSaveMyRegion(rgnSegmentList);
//  ClearOSD(FALSE);
  HDDCheck_InitProgBar(ProgressStart, ProgressEnd, ProgressMax, rgnSegmentList, LangGetString(LS_Warning), LangGetString(LS_CheckingFileSystem));

  // Suspekte Dateien in den ErrorString einsetzen
//  TAP_SPrint(ErrorStrFmt, sizeof(ErrorStrFmt), "Suspekt: %d", SuspectFiles);
//  strncat(ErrorStrFmt, LangGetString(LS_CheckFSFailed), sizeof(ErrorStrFmt) - strlen(ErrorStrFmt) - 1);
//  ErrorStrFmt[sizeof(ErrorStrFmt) - 1] = '\0';
  if (SuspectFiles >= 0)
    TAP_SPrint(SuspectFilesStr, sizeof(SuspectFilesStr), "%d", SuspectFiles);
  else
    TAP_SPrint(SuspectFilesStr, sizeof(SuspectFilesStr), "?");
  TAP_SPrint(ErrorStrFmt, sizeof(ErrorStrFmt), LangGetString(LS_CheckFSFailed), SuspectFilesStr);

  MountPath = (AbsPlaybackDir[0]) ? AbsPlaybackDir : TAPFSROOT;
  ret = HDD_CheckFileSystem(MountPath, NULL, ((ErrorMessage) ? &ShowErrorMessage : NULL), DoFix, Quick, InodeMonitoring, NoOkInfo, InodeNrs, LangGetString(LS_CheckFSSuccess), ErrorStrFmt, LangGetString(LS_CheckFSAborted));

  // Prüfen, ob das Playback wieder gestartet wurde
  if (DoFix && (State==ST_ActiveOSD || State==ST_ActionMenu) && (LastTotalBlocks > 0) && (RecFileSize > 0))
  {
    if((int)PlayInfo.totalBlock <= 0)
    {
      WriteLogMC(PROGRAM_NAME, "CheckFileSystem: Error restarting the playback!");
      State = ST_UnacceptedFile;
      LastTotalBlocks = PlayInfo.totalBlock;
      ClearOSD(TRUE);
    }
    else
    {
      if (SegmentMarker[NrSegmentMarker - 1].Block != PlayInfo.totalBlock)
      {
#ifdef FULLDEBUG
  WriteLogMCf(PROGRAM_NAME, "CheckFileSystem: Nach Playback-Restart neues TotalBlock %lu (vorher %lu)!", SegmentMarker[NrSegmentMarker - 1].Block, PlayInfo.totalBlock);
#endif
        SegmentMarker[NrSegmentMarker - 1].Block = PlayInfo.totalBlock;
      }
//      OSDRedrawEverything();
    }
  }
  else if (OldSysSubState != 0) 
    TAP_EnterNormalNoInfo();

  TRACEEXIT();
  return ret;
}
