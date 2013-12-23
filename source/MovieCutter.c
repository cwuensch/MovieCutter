#define _FILE_OFFSET_BITS  64
#define __USE_LARGEFILE64  1

#include                <stdio.h>
#include                <stdlib.h>
#include                <string.h>
#include                "tap.h"
#include                "libFireBird.h"
#include                "MovieCutter.h"
#include                "MovieCutterLib.h"
#include                "MovieCutter_TAPCOM.h"
#include                "Graphics/SegmentList_Background.gd"
#include                "Graphics/Selection_Blue.gd"
//#include                "Graphics/Selection_Blue_small.gd"
//#include                "Graphics/Selection_Red.gd"
#include                "Graphics/SegmentList_ScrollBar.gd"
#include                "Graphics/SegmentList_ScrollButton.gd"
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
#include                "Graphics/Button_Menu.gd"
#include                "Graphics/Button_Ok.gd"
#include                "Graphics/Button_Exit.gd"
#include                "Graphics/Info_Background.gd"
#include                "Graphics/Info_Progressbar.gd"
#include                "Graphics/BookmarkMarker.gd"
#include                "Graphics/BookmarkMarker_gray.gd"
#include                "Graphics/SegmentMarker.gd"
#include                "Graphics/SegmentMarker_gray.gd"
#include                "Graphics/ActionMenu9.gd"
#include                "Graphics/ActionMenu_Bar.gd"
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

typedef struct
{
  dword                 Block;  //Block nr
  dword                 Timems; //Time in ms
  float                 Percent;
  bool                  Selected;
} tSegmentMarker;

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
  MI_SelectFunction,
  MI_SaveSegment,
  MI_DeleteSegment,
  MI_SelectOddSegments,
  MI_SelectEvenSegments,
  MI_ClearAll,
  MI_ImportBookmarks,
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
  LS_MissingNav,     // error message
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
  LS_NrStrings
} tLngStrings;


// MovieCutter state variables
tState                  State = ST_Init;
bool                    MCShowMessageBox = FALSE;
bool                    AutoOSDPolicy = TRUE;
bool                    BookmarkMode;
TYPE_TrickMode          TrickMode;
byte                    TrickModeSpeed;
dword                   MinuteJump;                           //Seconds or 0 if deactivated
dword                   MinuteJumpBlocks;                     //Number of blocks, which shall be added
bool                    NoPlaybackCheck = FALSE;              //Used to circumvent a race condition during the cutting process
int                     NrSelectedSegments;
word                    JumpRequestedSegment = 0xFFFF;        //Is set, when the user presses up/down to jump to another segment
dword                   JumpRequestedTime = 0;
dword                   JumpPerformedTime = 0;

// Playback information
TYPE_PlayInfo           PlayInfo;
char                    PlaybackName[MAX_FILE_NAME_SIZE + 1];
char                    AbsPlaybackDir[512];
char                   *PlaybackDir;
__off64_t               RecFileSize;
dword                   LastTotalBlocks = 0;
dword                   BlocksOneSecond;
dword                   BlockNrLastSecond;
dword                   BlockNrLast10Seconds;

// Video parameters
int                     NrSegmentMarker;
int                     ActiveSegment;
tSegmentMarker          SegmentMarker[NRSEGMENTMARKER];       //[0]=Start of file, [x]=End of file
int                     NrBookmarks;
dword                   Bookmarks[NRBOOKMARKS];
dword                   NrTimeStamps = 0;
tTimeStamp             *TimeStamps = NULL;
tTimeStamp             *LastTimeStamp = NULL;
bool                    HDVideo;

// OSD object variables
tFontDataUC             Calibri_10_FontDataUC;
tFontDataUC             Calibri_12_FontDataUC;
tFontDataUC             Calibri_14_FontDataUC;
word                    rgnSegmentList = 0;
word                    rgnInfo = 0;
word                    rgnActionMenu = 0;
int                     ActionMenuItem;
dword                   BookmarkMode_x;

char                    LogString[512];

// ============================================================================
//                              IMPLEMENTIERUNG
// ============================================================================

int TAP_Main(void)
{
  #if STACKTRACE == TRUE
    CallTraceInit();
    CallTraceEnable(TRUE);
    CallTraceEnter("TAP_Main");
  #endif

  CreateSettingsDir();
  LoadINI();
  KeyTranslate(TRUE, &TAP_EventHandler);

  if (!(FMUC_LoadFontFile("Calibri_10.ufnt", &Calibri_10_FontDataUC)
     && FMUC_LoadFontFile("Calibri_12.ufnt", &Calibri_12_FontDataUC)
     && FMUC_LoadFontFile("Calibri_14.ufnt", &Calibri_14_FontDataUC)))
  {
    FMUC_FreeFontFile(&Calibri_10_FontDataUC);
    FMUC_FreeFontFile(&Calibri_12_FontDataUC);
    FMUC_FreeFontFile(&Calibri_14_FontDataUC);
    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return 0;
  }

  if(!LangLoadStrings(LNGFILENAME, LS_NrStrings, LAN_English, PROGRAM_NAME))
  {
    WriteLogMC(PROGRAM_NAME, "Language file is missing!");
    TAP_SPrint(LogString, "Language file '%s' not found!", LNGFILENAME);
    OSDMenuInfoBoxShow(PROGRAM_NAME " " VERSION, LogString, 500);
    do
    {
      OSDMenuEvent(NULL, NULL, NULL);
    } while(OSDMenuInfoBoxIsVisible());
    OSDMenuInfoBoxDestroy();

    FMUC_FreeFontFile(&Calibri_10_FontDataUC);
    FMUC_FreeFontFile(&Calibri_12_FontDataUC);
    FMUC_FreeFontFile(&Calibri_14_FontDataUC);
    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return 0;
  }

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
  return 1;
}

// ----------------------------------------------------------------------------
//                            TAP EventHandler
// ----------------------------------------------------------------------------
dword TAP_EventHandler(word event, dword param1, dword param2)
{
  dword                 SysState, SysSubState;
  static bool           DoNotReenter = FALSE;
  static bool           OldRepeatMode = FALSE;
  static bool           RebootMessage = FALSE;
  static bool           NavLengthMessage = FALSE;
  static char           NavLengthWrongStr[128];
  static dword          LastMinuteKey = 0;
  static dword          LastDraw = 0;

  (void) param2;

  if(DoNotReenter) return param1;
  DoNotReenter = TRUE;

  #if STACKTRACE == TRUE
    CallTraceEnter("TAP_EventHandler");
  #endif

  if(event == EVT_TMSCommander)
  {
    dword ret = TMSCommander_handler(param1);

    DoNotReenter = FALSE;
    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return ret;
  }

  if(event == EVT_TAPCOM)
  {
    TAPCOM_Channel  Channel;
    dword           ServiceID;

    Channel = TAPCOM_GetChannel(param1, NULL, &ServiceID, NULL, NULL);
    if(ServiceID == TAPCOM_SERVICE_ISOSDVISIBLE)
      TAPCOM_Finish(Channel, rgnSegmentList ? 1 : 0);
    else
      TAPCOM_Reject(Channel);
  }

  if(event == EVT_STOP)
  {
    State = ST_Exit;
  }

  // Behandlung von offenen MesssageBoxen
  // ------------------------------------
  if(OSDMenuMessageBoxIsVisible())
  {
    if(MCShowMessageBox)
    {
      if(event == EVT_KEY && (param1 == RKEY_Ok || param1 == RKEY_Exit))
      {
        // ST_ActionMenu: Confirmation-Dialog
        if (State == ST_ActionMenu)
        {
          OSDMenuMessageBoxDestroyNoOSDUpdate();
          MCShowMessageBox = FALSE;
          TAP_Osd_Sync();
          OSDRedrawEverything();
          if ((param1 == RKEY_Ok) && (OSDMenuMessageBoxLastButton() == 0))
          {
            ActionMenuRemove();
            State = ST_ActiveOSD;
            switch(ActionMenuItem)
            {
              case MI_SaveSegment:        MovieCutterSaveSegments(); break;
              case MI_DeleteSegment:      MovieCutterDeleteSegments(); break;
              case MI_ClearAll:
              {
                (BookmarkMode) ? DeleteAllBookmarks() : DeleteAllSegmentMarkers(); 
                OSDSegmentListDrawList();
                OSDInfoDrawProgressbar(TRUE);
                break;
              }
              case MI_DeleteFile:         MovieCutterDeleteFile(); break;
            }
          }
          else
            ActionMenuDraw();
        }
        // ST_WaitForPlayback: NavPatched-Meldung UND Reboot- und NavLengthMismatch-Dialog
        else if (State == ST_WaitForPlayback)
        {
          OSDMenuMessageBoxDestroyNoOSDUpdate();
          MCShowMessageBox = FALSE;
          TAP_Osd_Sync();

          if (RebootMessage || NavLengthMessage)
          {
            if ((param1 == RKEY_Ok) && (OSDMenuMessageBoxLastButton() == 0))
            {
              if (RebootMessage && NavLengthMessage)
              {
                RebootMessage = FALSE;
                TAP_SPrint(NavLengthWrongStr, LangGetString(LS_NavLengthWrong), (int)((TimeStamps[NrTimeStamps-1].Timems/1000) - (60*PlayInfo.duration + PlayInfo.durationSec)));
                ShowConfirmationDialog(NavLengthWrongStr);
              }
              else
              {
                RebootMessage = FALSE;
                NavLengthMessage = FALSE;
                State = ST_ActiveOSD;
                OSDRedrawEverything();
              }
            }
            else
            {
              RebootMessage = FALSE;
              NavLengthMessage = FALSE;
              if (AutoOSDPolicy)
                State = ST_UnacceptedFile;
              else
              {
                Cleanup(FALSE);
                State = ST_InactiveMode;
              }
              PlaybackRepeatSet(OldRepeatMode);
              ClearOSD(TRUE);
            }
          }
        }
        // ST_ActiveOSD: SegmentListIsFull-Message
        else if (State == ST_ActiveOSD)
        {
          OSDMenuMessageBoxDestroyNoOSDUpdate();
          MCShowMessageBox = FALSE;
          TAP_Osd_Sync();
          OSDRedrawEverything();
        }
        // ST_UnacceptedFile: alle möglichen Fehlermeldungen
        else
        {
          OSDMenuMessageBoxDestroy();
          MCShowMessageBox = FALSE;
          PlaybackRepeatSet(OldRepeatMode);
          ClearOSD(TRUE);
        }
        param1 = 0;
      }
      else
        OSDMenuEvent(&event, &param1, &param2);  // Event-Handling der MessageBox (Button wählen, etc.)
    }
    DoNotReenter = FALSE;
    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return param1;
  }

  switch(State)
  {
    // Initialisierung bei TAP-Start
    // -----------------------------
    case ST_Init:
    {
      CleanupCut();
/*
      LastTotalBlocks = 0;
      MCShowMessageBox = FALSE;
      NrTimeStamps = 0;
      TimeStamps = NULL;
      LastTimeStamp = NULL;
      JumpRequestedSegment = 0xFFFF;
      JumpRequestedTime = 0;
      JumpPerformedTime = 0;
*/
      State = AutoOSDPolicy ? ST_WaitForPlayback : ST_InactiveMode;
      break;
    }

    // Warten / Aufnahme analysieren
    // -----------------------------
    case ST_WaitForPlayback:    // Idle loop while there is no playback active, shows OSD as soon as playback starts
    {
      if(isPlaybackRunning())
      {
        char *p;

        if((int)PlayInfo.totalBlock > 0)
        {
          TAP_GetState(&SysState, &SysSubState);
          if(SysState != STATE_Normal) break;

          BookmarkMode = FALSE;
          NrSegmentMarker = 0;
          ActiveSegment = 0;
          MinuteJump = 0;
//          MinuteJumpBlocks = 0;  // nicht unbedingt nötig
          RebootMessage = FALSE;
          NavLengthMessage = FALSE;
          NoPlaybackCheck = FALSE;
          LastTotalBlocks = PlayInfo.totalBlock;

          OldRepeatMode = PlaybackRepeatGet();

          //"Calculate" the file name (.rec or .mpg)
          if(!PlayInfo.file || !PlayInfo.file->name[0]) PlaybackName[0] = '\0';
          strcpy(PlaybackName, PlayInfo.file->name);
          PlaybackName[strlen(PlaybackName) - 4] = '\0';

          //Extract the absolute path to the rec file and change into that dir
          HDD_GetAbsolutePathByTypeFile(PlayInfo.file, AbsPlaybackDir);
          p = strstr(AbsPlaybackDir, PlaybackName);
          if(p) *(p-1) = '\0';
          PlaybackDir = &AbsPlaybackDir[strlen(TAPFSROOT)];
//          TAP_Hdd_ChangeDir(&PlaybackDirectory[strlen(TAPFSROOT)]);
          HDD_ChangeDir(PlaybackDir);

          WriteLogMC(PROGRAM_NAME, "========================================\n");
          TAP_SPrint(LogString, "Attaching to %s/%s", AbsPlaybackDir, PlaybackName);
          WriteLogMC(PROGRAM_NAME, LogString);
          WriteLogMC("MovieCutterLib", "----------------------------------------");

          // Detect size of rec file
//          RecFileSize = HDD_GetFileSize(PlaybackName);
//          if(RecFileSize <= 0)
          if(!HDD_GetFileSizeAndInode(PlaybackDir, PlaybackName, NULL, &RecFileSize))
          {
            WriteLogMC(PROGRAM_NAME, ".rec size could not be detected!");
            ShowErrorMessage(LangGetString(LS_NoRecSize));
            State = ST_UnacceptedFile;
            break;
          }

          TAP_SPrint(LogString, "File size = %llu Bytes (%u blocks)", RecFileSize, (dword)(RecFileSize / BLOCKSIZE));
          WriteLogMC(PROGRAM_NAME, LogString);
          TAP_SPrint(LogString, "Reported total blocks: %u", PlayInfo.totalBlock);
          WriteLogMC(PROGRAM_NAME, LogString);

          //Check if a nav is available
          if(!isNavAvailable(PlaybackName))
          {
            WriteLogMC(PROGRAM_NAME, ".nav is missing!");
            ShowErrorMessage(LangGetString(LS_MissingNav));
            State = ST_UnacceptedFile;
            break;
          }

          //Check if it is crypted
          if(isCrypted(PlaybackName))
          {
            WriteLogMC(PROGRAM_NAME, "File is crypted!");
            ShowErrorMessage(LangGetString(LS_IsCrypted));
            State = ST_UnacceptedFile;
            break;
          }
          
          // Detect if video stream is in HD
          if (!isHDVideo(PlaybackName, &HDVideo))
          {
            WriteLogMC(PROGRAM_NAME, "Could not detect type of video stream!");
            ShowErrorMessage(LangGetString(LS_HDDetectionFailed));
            State = ST_UnacceptedFile;
            break;
          }
          WriteLogMC(PROGRAM_NAME, (HDVideo) ? "Type of recording: HD" : "Type of recording: SD");

          //Free the old timing array, so that it is empty (NULL pointer) if something goes wrong
          if(TimeStamps)
          {
            TAP_MemFree(TimeStamps);
            TimeStamps = NULL;
          }
//          NrTimeStamps = 0;
//          LastTimeStamp = NULL;

          // Try to load the nav
          TimeStamps = NavLoad(PlaybackName, &NrTimeStamps, HDVideo);
          if (TimeStamps)
          {
            // Write duration to log file
            char TimeStr[16];
            TAP_SPrint(LogString, ".nav-file loaded: %u different TimeStamps found.", NrTimeStamps);
            WriteLogMC(PROGRAM_NAME, LogString);
            MSecToTimeString(TimeStamps[0].Timems, TimeStr);
            TAP_SPrint(LogString, "First Timestamp: Block=%u, Time=%s", TimeStamps[0].BlockNr, TimeStr);
            WriteLogMC(PROGRAM_NAME, LogString);
            MSecToTimeString(TimeStamps[NrTimeStamps-1].Timems, TimeStr);
            TAP_SPrint(LogString, "Playback Duration (from nav): %s", TimeStr);
            WriteLogMC(PROGRAM_NAME, LogString);
            SecToTimeString(60*PlayInfo.duration + PlayInfo.durationSec, TimeStr);
            TAP_SPrint(LogString, "Playback Duration (from inf): %s", TimeStr);
            WriteLogMC(PROGRAM_NAME, LogString);
          }
          else
          {
            WriteLogMC(PROGRAM_NAME, "Error loading the .nav file!");
            ShowErrorMessage(LangGetString(LS_NavLoadFailed));
            State = ST_UnacceptedFile;
            break;
          }
          LastTimeStamp = &TimeStamps[0];

          // Check if receiver has been rebooted since the recording
          dword RecDateTime;
          if (GetRecDateFromInf(PlaybackName, &RecDateTime))
          {
            dword TimeSinceRec = TimeDiff(RecDateTime, Now(NULL));
            dword UpTime = GetUptime() / 6000;
            if (TimeSinceRec <= UpTime + 1)
              RebootMessage = TRUE;

            #ifdef FULLDEBUG
              if (RecDateTime > 0xd0790000)
                RecDateTime = TF2UnixTime(RecDateTime);
              TAP_PrintNet("Reboot-Check (%s): TimeSinceRec=%u, UpTime=%u, RecDateTime=%s", (RebootMessage) ? "TRUE" : "FALSE", TimeSinceRec, UpTime, asctime(localtime(&RecDateTime)));
            #endif
          }

          // Check if nav has correct length!
          if(labs(TimeStamps[NrTimeStamps-1].Timems - (1000 * (60*PlayInfo.duration + PlayInfo.durationSec))) > 3000)
          {
            WriteLogMC(PROGRAM_NAME, ".nav file length not matching duration!");

            // [COMPATIBILITY LAYER - fill holes in old nav file]
            if (PatchOldNavFile(PlaybackName, HDVideo))
            {
              WriteLogMC(PROGRAM_NAME, ".nav file patched by Compatibility Layer.");
              ShowErrorMessage(LangGetString(LS_NavPatched));
              LastTotalBlocks = 0;
              break;
            }
            else
            {
              NavLengthMessage = TRUE;
            }
          }

//          CreateOSD();
//          Playback_Normal();
          CalcLastSeconds();
          ReadBookmarks();
          if(!CutFileLoad())
          {
            if(!AddDefaultSegmentMarker())
            {
              WriteLogMC(PROGRAM_NAME, "Error adding default segment markers!");
              ShowErrorMessage(LangGetString(LS_NavLoadFailed));
              
              State = ST_UnacceptedFile;
              RebootMessage = FALSE;
              NavLengthMessage = FALSE;
              break;
            }
          }

          PlaybackRepeatSet(TRUE);

          if (RebootMessage)
            ShowConfirmationDialog(LangGetString(LS_RebootMessage));
          else if (NavLengthMessage)
          {
            TAP_SPrint(NavLengthWrongStr, LangGetString(LS_NavLengthWrong), (TimeStamps[NrTimeStamps-1].Timems/1000) - (60*PlayInfo.duration + PlayInfo.durationSec));
            ShowConfirmationDialog(NavLengthWrongStr);
          }
          else
          {
            State = ST_ActiveOSD;
            OSDRedrawEverything();
          }
        }
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
      if (!isPlaybackRunning() || (LastTotalBlocks != PlayInfo.totalBlock)) {
        Cleanup(FALSE);
        State = (AutoOSDPolicy) ? ST_WaitForPlayback : ST_InactiveMode;
        break;
      }

      if (State == ST_UnacceptedFile) break;
      // else continue with ST_InactiveMode
    }
    case ST_InactiveMode:    // OSD is hidden and has to be manually activated
    {
      // if cut-key is pressed -> show MovieCutter OSD (ST_WaitForPlayback)
      if((event == EVT_KEY) && ((param1 == RKEY_Ab) || (param1 == RKEY_Option)))
      {
        if (!isPlaybackRunning()) break;
        if (LastTotalBlocks != PlayInfo.totalBlock)
          State = ST_WaitForPlayback;
        else {
          // beim erneuten Einblenden kann man sich das Neu-Berechnen aller Werte sparen (AUCH wenn 2 Aufnahmen gleiche Blockzahl haben!!)
          NoPlaybackCheck = FALSE;
          BookmarkMode = FALSE;
          MinuteJump = 0;
//          MinuteJumpBlocks = 0;
//          CreateOSD();
//          Playback_Normal();
          ReadBookmarks();
          OldRepeatMode = PlaybackRepeatGet();
          PlaybackRepeatSet(TRUE);
          OSDRedrawEverything();
          State = ST_ActiveOSD;
        }
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
#ifdef FULLDEBUG
  WriteLogMC(PROGRAM_NAME, "TAP_EventHandler(): State=ST_ActiveOSD, !isPlaybackRunning --> Aufruf von CutFileSave()");
#endif
        CutFileSave();
        Cleanup(TRUE);
        State = AutoOSDPolicy ? ST_WaitForPlayback : ST_InactiveMode;
        break;
      }

      TAP_GetState(&SysState, &SysSubState);
      if(SysSubState == SUBSTATE_Normal) TAP_ExitNormal();

      if(event == EVT_KEY)
      {
        switch(param1)
        {
          case RKEY_Ab:
          case RKEY_Option:
          {
            BookmarkMode = !BookmarkMode;
//            OSDRedrawEverything();
            OSDInfoDrawProgressbar(TRUE);
            OSDInfoDrawBookmarkMode();
            OSDInfoDrawMinuteJump();
            break;
          }
        
          case RKEY_Record:
          {
            break;
          }

          case RKEY_Exit:
          {
#ifdef FULLDEBUG
  WriteLogMC(PROGRAM_NAME, "TAP_EventHandler(): State=ST_ActiveOSD, Key=RKEY_Exit --> Aufruf von CutFileSave()");
#endif
            CutFileSave();
            ClearOSD(TRUE);
            PlaybackRepeatSet(OldRepeatMode);
            State = ST_InactiveModePlaying;

            //Exit immediately so that other functions can not interfere with the cleanup
            DoNotReenter = FALSE;
            #if STACKTRACE == TRUE
              CallTraceExit(NULL);
            #endif
            return 0;
          }

          case RKEY_Down:
          {
            if(NrSegmentMarker > 2)
            {
              if (JumpRequestedSegment == 0xFFFF)
                JumpRequestedSegment = ActiveSegment;
              if (JumpRequestedSegment < (NrSegmentMarker - 2))
                JumpRequestedSegment++;
              else
                JumpRequestedSegment = 0;
              OSDSegmentListDrawList();
              OSDInfoDrawProgressbar(TRUE);
//              OSDInfoDrawCurrentPosition(FALSE);
//              TAP_Osd_Sync();
              JumpRequestedTime = TAP_GetTick();
              if (!JumpRequestedTime) JumpRequestedTime = 1;
//              if(TrickMode == TRICKMODE_Pause) Playback_Normal();
//              TAP_Hdd_ChangePlaybackPos(SegmentMarker[ActiveSegment].Block);
            }
            break;
          }

          case RKEY_Up:
          {
            if(NrSegmentMarker > 2)
            {
              if (JumpRequestedSegment == 0xFFFF)
                JumpRequestedSegment = ActiveSegment;
              if (JumpRequestedSegment > 0)
                JumpRequestedSegment--;
              else
                JumpRequestedSegment = NrSegmentMarker - 2;
              OSDSegmentListDrawList();
              OSDInfoDrawProgressbar(TRUE);
//              OSDInfoDrawCurrentPosition(TRUE);
//              TAP_Osd_Sync();
              JumpRequestedTime = TAP_GetTick();
              if (!JumpRequestedTime) JumpRequestedTime = 1;
//              if(TrickMode == TRICKMODE_Pause) Playback_Normal();
//              TAP_Hdd_ChangePlaybackPos(SegmentMarker[ActiveSegment].Block);
            }
            break;
          }

          case RKEY_Prev:
          {
            if(MinuteJump)
              Playback_JumpBackward();
            else if(BookmarkMode)
              Playback_JumpPrevBookmark();
            else
              Playback_JumpPrevSegment();  
            break;
          }

          case RKEY_Next:
          {
            if(MinuteJump)
              Playback_JumpForward();
            else if(BookmarkMode)
              Playback_JumpNextBookmark();
            else
              Playback_JumpNextSegment();
            break;
          }

          case RKEY_Red:
          {
            if (BookmarkMode) {  
              int NearestBookmarkIndex = FindNearestBookmark();
              if(NearestBookmarkIndex != -1)
              {
                if (DeleteBookmark(NearestBookmarkIndex))
                  OSDInfoDrawProgressbar(TRUE);
//                  OSDRedrawEverything();
              }
            } else {
              int NearestMarkerIndex = FindNearestSegmentMarker();
              if(NearestMarkerIndex != -1)
              {
                if (DeleteSegmentMarker(NearestMarkerIndex))
                {
                  OSDSegmentListDrawList();
                  OSDInfoDrawProgressbar(TRUE);
//                  OSDRedrawEverything();
                }
              }
            } 
            break;
          }

          case RKEY_Green:
          {
            if (BookmarkMode) {
              if(AddBookmark(PlayInfo.currentBlock)) OSDInfoDrawProgressbar(TRUE); // OSDRedrawEverything();
            } else {
              if(AddSegmentMarker(PlayInfo.currentBlock, TRUE)) {OSDSegmentListDrawList(); OSDInfoDrawProgressbar(TRUE);}; // OSDRedrawEverything();
            }
            break;
          }

          case RKEY_Yellow:
          {
            if (BookmarkMode) {
              if(MoveBookmark(PlayInfo.currentBlock)) OSDInfoDrawProgressbar(TRUE);
            } else {
              if(MoveSegmentMarker(PlayInfo.currentBlock)) {OSDSegmentListDrawList(); OSDInfoDrawProgressbar(TRUE);};
            }
//            OSDRedrawEverything();
            break;
          }

          case RKEY_Blue:
          {
            SelectSegmentMarker();
            OSDSegmentListDrawList();
            OSDInfoDrawProgressbar(TRUE);
//            OSDRedrawEverything();
            break;
          }

          case RKEY_Menu:
          {
            if(TrickMode != TRICKMODE_Pause)
            {
              Playback_Pause();
            }
            ActionMenuDraw();
            State = ST_ActionMenu;
            break;
          }

          case RKEY_Ok:
          {
            if(TrickMode == TRICKMODE_Normal)
            {
              Playback_Pause();
            }
            else
            {
              Playback_Normal();
//              ActionMenuDraw();
//              State = ST_ActionMenu;
            }
            break;
          }

          case RKEY_Play:
          {
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

          case RKEY_Stop:
          {
            TAP_Hdd_StopTs();
            break;
          }

          case RKEY_Right:
          {
            Playback_Faster();
            break;
          }

          case RKEY_Left:
          {
            Playback_Slower();
            break;
          }

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
            if ((MinuteJump > 0) && (MinuteJump < 10) && (labs(TAP_GetTick() - LastMinuteKey) < 200))
              // We are already in minute jump mode, but only one digit already entered
              MinuteJump = 10 * MinuteJump + (param1 & 0x0f);
            else
              MinuteJump = (param1 & 0x0f);
            MinuteJumpBlocks = (PlayInfo.totalBlock / (60*PlayInfo.duration + PlayInfo.durationSec)) * MinuteJump*60;
            LastMinuteKey = TAP_GetTick();
            OSDInfoDrawMinuteJump();
            break;
          }
        }
        param1 = 0;
      }

      // VORSICHT!!! Das hier wird interaktiv ausgeführt
      if(labs(TAP_GetTick() - LastDraw) > 10) {
        if (JumpRequestedTime && (labs(TAP_GetTick() - JumpRequestedTime) >= 100))
        {
          if (JumpRequestedSegment != 0xFFFF)
          {
            if(TrickMode == TRICKMODE_Pause) Playback_Normal();
            TAP_Hdd_ChangePlaybackPos(SegmentMarker[JumpRequestedSegment].Block);
            ActiveSegment = JumpRequestedSegment;
            JumpPerformedTime = TAP_GetTick();
          }
          JumpRequestedSegment = 0xFFFF;
          JumpRequestedTime = 0;
        }
        CheckLastSeconds();
        OSDInfoDrawProgressbar(FALSE);
        OSDInfoDrawPlayIcons(FALSE);
        SetCurrentSegment();
        OSDInfoDrawCurrentPosition(FALSE);
        OSDInfoDrawClock(FALSE);
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
        switch(param1)
        {
          case RKEY_Ok:
          {
            if(ActionMenuItem != MI_SelectFunction)
            {
              // Deaktivierte Aktionen
              if ((ActionMenuItem==MI_SaveSegment || ActionMenuItem==MI_DeleteSegment || ActionMenuItem==MI_SelectOddSegments || ActionMenuItem==MI_SelectEvenSegments) && NrSegmentMarker<=2)
                break;

              // Aktionen mit Confirmation-Dialog
              if (ActionMenuItem==MI_SaveSegment || ActionMenuItem==MI_DeleteSegment || (ActionMenuItem==MI_ClearAll && NrSelectedSegments==0) || ActionMenuItem==MI_DeleteFile)
              {
                ShowConfirmationDialog(LangGetString(LS_AskConfirmation));
                // Beim Speichern/Löschen einer Szene aus der Mitte, wird häufig das Ende der Original-Aufnahme korrumpiert!!
                // Wenn Szenen zum Speichern ausgewählt sind, ist davon auszugehen, dass der Rest nicht mehr benötigt wird. Da die letzte Szene zuerst gespeichert wird, passiert also nichts.
                // Wenn Szenen zum Löschen ausgewählt sind, und das Ende soll nicht gelöscht werden -> Beschädigung möglich
//                if (ActionMenuItem==MI_SaveSegment || !SegmentMarker[NrSegmentMarker-2].Selected)
//                  OSDMenuMessageBoxModifyText("Vorsicht!! Beschädigung des Endes möglich...");
                break;
              }

              ActionMenuRemove();
              State = ST_ActiveOSD;
              switch(ActionMenuItem)
              {
//                case MI_SaveSegment:        MovieCutterSaveSegments(); break;
//                case MI_DeleteSegment:      MovieCutterDeleteSegments(); break;
                case MI_SelectOddSegments:  MovieCutterSelectOddSegments();  ActionMenuDraw(); State = ST_ActionMenu; break;
                case MI_SelectEvenSegments: MovieCutterSelectEvenSegments(); ActionMenuDraw(); State = ST_ActionMenu; break;
                case MI_ClearAll:           MovieCutterUnselectAll(); break;
//                case MI_DeleteFile:         MovieCutterDeleteFile(); break;
                case MI_ImportBookmarks:    (BookmarkMode) ? ExportSegmentsToBookmarks() : AddBookmarksToSegmentList(); break;
                case MI_ExitMC:             State = ST_Exit; break;
                case MI_NrMenuItems:        break;
              }
            }
            break;
          }

          case RKEY_Down:
          {
            ActionMenuDown();
            break;
          }

          case RKEY_Up:
          {
            ActionMenuUp();
            break;
          }

          case RKEY_Ab:
          case RKEY_Option:
          {
            BookmarkMode = !BookmarkMode;
            OSDInfoDrawProgressbar(TRUE);
            OSDInfoDrawBookmarkMode();
            OSDInfoDrawMinuteJump();
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
      if (strlen(PlaybackName) > 0)
      {
#ifdef FULLDEBUG
  WriteLogMC(PROGRAM_NAME, "TAP_EventHandler(): State=ST_Exit --> Aufruf von CutFileSave()");
#endif
        CutFileSave();
      }
      PlaybackRepeatSet(OldRepeatMode);
      Cleanup(TRUE);
      LangUnloadStrings();
      FMUC_FreeFontFile(&Calibri_10_FontDataUC);
      FMUC_FreeFontFile(&Calibri_12_FontDataUC);
      FMUC_FreeFontFile(&Calibri_14_FontDataUC);
      TAP_Exit();
      break;
    }
  }

  DoNotReenter = FALSE;

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
  return param1;
}

// ----------------------------------------------------------------------------
//                           MessageBox-Funktionen
// ----------------------------------------------------------------------------
void ShowConfirmationDialog(char* MessageStr)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("ShowConfirmationDialog");
  #endif

  OSDMenuMessageBoxInitialize(PROGRAM_NAME, MessageStr);
  OSDMenuMessageBoxDoNotEnterNormalMode(TRUE);
  OSDMenuMessageBoxAllowScrollOver();
  OSDMenuMessageBoxButtonAdd(LangGetString(LS_Yes));
  OSDMenuMessageBoxButtonAdd(LangGetString(LS_No));
  OSDMenuMessageBoxButtonSelect(1);
  OSDMenuMessageBoxShow();
  MCShowMessageBox = TRUE;

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void ShowErrorMessage(char* MessageStr)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("ShowErrorMessage");
  #endif

  OSDMenuMessageBoxInitialize(PROGRAM_NAME, MessageStr);
  OSDMenuMessageBoxButtonAdd(LangGetString(LS_OK));
  OSDMenuMessageBoxShow();
  MCShowMessageBox = TRUE;
  NoPlaybackCheck = FALSE;

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

dword TMSCommander_handler(dword param1)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("TMSCommander_handler");
  #endif

  switch (param1)
  {
    case TMSCMDR_Capabilities:
    {
      #if STACKTRACE == TRUE
        CallTraceExit(NULL);
      #endif
      return (dword)(TMSCMDR_CanBeStopped | TMSCMDR_HaveUserEvent);
    }

    case TMSCMDR_UserEvent:
    {
      if(State == ST_InactiveMode || State == ST_InactiveModePlaying)
        State = ST_WaitForPlayback;

      #if STACKTRACE == TRUE
        CallTraceExit(NULL);
      #endif
      return TMSCMDR_OK;
    }

    case TMSCMDR_Menu:
    {
      #if STACKTRACE == TRUE
        CallTraceExit(NULL);
      #endif
      return TMSCMDR_NotOK;
    }

    case TMSCMDR_Stop:
    {
      State = ST_Exit;

      #if STACKTRACE == TRUE
        CallTraceExit(NULL);
      #endif
      return TMSCMDR_OK;
    }
  }

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
  return TMSCMDR_UnknownFunction;
}

// ----------------------------------------------------------------------------
//                            Reset-Funktionen
// ----------------------------------------------------------------------------
void ClearOSD(bool EnterNormal)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("ClearOSD");
  #endif

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

  if(rgnInfo)
  {
    TAP_Osd_Delete(rgnInfo);
    rgnInfo = 0;
  }

  TAP_Osd_Sync();
  if (EnterNormal) TAP_EnterNormal();

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void Cleanup(bool DoClearOSD)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("Cleanup");
  #endif
  LastTotalBlocks = 0;
  JumpRequestedSegment = 0xFFFF;
  JumpRequestedTime = 0;
  JumpPerformedTime = 0;
  if(TimeStamps) {
    TAP_MemFree(TimeStamps);
    TimeStamps = NULL;
  }
//  lastTimeStamp = NULL;
//  NrTimeStamps = 0;
  if (DoClearOSD) ClearOSD(TRUE);

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void CleanupCut(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("CleanupCut");
  #endif

  int                   NrFiles, i;
  TYPE_FolderEntry      FolderEntry;
  char                  RecFileName[MAX_FILE_NAME_SIZE + 1];

  HDD_TAP_PushDir();
  HDD_ChangeDir("/DataFiles");
  
  // Lösche verweiste .cut-Dateien in /DataFiles (nicht in Unterverzeichnissen)
  NrFiles = TAP_Hdd_FindFirst(&FolderEntry, "cut");
  for (i = 0; i < NrFiles; i++)
  {
    if(FolderEntry.attr == ATTR_NORMAL)
    {
      strcpy(RecFileName, FolderEntry.name);
      RecFileName[strlen(RecFileName) - 4] = '\0';
      strcat(RecFileName, ".rec");
      if(!TAP_Hdd_Exist(RecFileName)) TAP_Hdd_Delete(FolderEntry.name);
    }
    TAP_Hdd_FindNext(&FolderEntry);
  }

  // Lösche verweiste .cut.bak-Dateien in /DataFiles
  NrFiles = TAP_Hdd_FindFirst(&FolderEntry, "bak");
  for (i = 0; i < NrFiles; i++)
  {
    if(FolderEntry.attr == ATTR_NORMAL)
    {
      strcpy(RecFileName, FolderEntry.name);
      if (strncmp(RecFileName + strlen(RecFileName) - 8, ".cut.bak", 8) == 0)
      {
        RecFileName[strlen(RecFileName) - 8] = '\0';
        strcat(RecFileName, ".rec");
        if(!TAP_Hdd_Exist(RecFileName)) TAP_Hdd_Delete(FolderEntry.name);
      }
    }
    TAP_Hdd_FindNext(&FolderEntry);
  }
  HDD_TAP_PopDir();

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

// ----------------------------------------------------------------------------
//                             INI-Funktionen
// ----------------------------------------------------------------------------
void CreateSettingsDir(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("CreateSettingsDir");
  #endif

  HDD_TAP_PushDir();
  HDD_ChangeDir("/ProgramFiles");
  if(!TAP_Hdd_Exist("Settings")) TAP_Hdd_Create("Settings", ATTR_FOLDER);
  HDD_ChangeDir("Settings");
  if(!TAP_Hdd_Exist("MovieCutter")) TAP_Hdd_Create("MovieCutter", ATTR_FOLDER);
  HDD_TAP_PopDir();

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void LoadINI(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("LoadINI");
  #endif

  INILOCATION IniFileState;

  HDD_TAP_PushDir();
  HDD_ChangeDir(LOGDIR);
  IniFileState = INIOpenFile(INIFILENAME, PROGRAM_NAME);
  if((IniFileState != INILOCATION_NotFound) && (IniFileState != INILOCATION_NewFile))
  {
    AutoOSDPolicy = INIGetInt("AutoOSDPolicy", 1, 0, 1) != 0;
  }
  INICloseFile();
  if(IniFileState == INILOCATION_NewFile)
    SaveINI();
  HDD_TAP_PopDir();

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void SaveINI(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("SaveINI");
  #endif

  HDD_TAP_PushDir();
  HDD_ChangeDir(LOGDIR);
  INIOpenFile(INIFILENAME, PROGRAM_NAME);
  INISetInt("AutoOSDPolicy", AutoOSDPolicy ? 1 : 0);
  INISaveFile(INIFILENAME, INILOCATION_AtCurrentDir, NULL);
  INICloseFile();
  HDD_TAP_PopDir();

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}


// ----------------------------------------------------------------------------
//                         SegmentMarker-Funktionen
// ----------------------------------------------------------------------------
void AddBookmarksToSegmentList(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("AddBookmarksToSegmentList");
  #endif

  int i;

  if (NrBookmarks > 0)
  {
    // first, delete all present segment markers (*CW*)
    DeleteAllSegmentMarkers();

    // second, add a segment marker for each bookmark
    TAP_SPrint(LogString, "Importing %d of %d bookmarks", min(NrBookmarks, NRSEGMENTMARKER-2), NrBookmarks);
    WriteLogMC(PROGRAM_NAME, LogString);

    for(i = 0; i < min(NrBookmarks, NRSEGMENTMARKER-2); i++)
    { 
      TAP_SPrint(LogString, "Bookmark %d @ %u", i + 1, Bookmarks[i]);
      WriteLogMC(PROGRAM_NAME, LogString);
      AddSegmentMarker(Bookmarks[i], FALSE);
    }

    OSDSegmentListDrawList();
    OSDInfoDrawProgressbar(TRUE);
//    OSDRedrawEverything();
  }
  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

bool AddDefaultSegmentMarker(void)
{
  bool ret;
  #if STACKTRACE == TRUE
    CallTraceEnter("AddDefaultSegmentMarker");
  #endif

  ret = AddSegmentMarker(0, FALSE);
  ret = ret && AddSegmentMarker(PlayInfo.totalBlock, FALSE);

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
  return ret;
}

bool AddSegmentMarker(dword Block, bool RejectSmallSegments)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("AddSegmentMarker");
  #endif

  int                   i, j;
  dword                 newTime;
  char                  StartTime[16];

  if(NrSegmentMarker >= NRSEGMENTMARKER)
  {
    WriteLogMC(PROGRAM_NAME, "AddSegmentMarker: SegmentMarker list is full!");
    ShowErrorMessage(LangGetString(LS_ListIsFull));
//    OSDMenuMessageBoxDoNotEnterNormalMode(TRUE);

    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return FALSE;
  }

  newTime = NavGetBlockTimeStamp(Block);
  if((newTime == 0) && (Block > 3 * BlocksOneSecond))
  {
    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return FALSE;
  }

#ifdef FULLDEBUG
  printf("----------- vorher ---------------\n");
  printf(" PlayInfo.currentBlock: %u,  PlayInfo.totalBlock: %u\n", PlayInfo.currentBlock, PlayInfo.totalBlock);
  char TimeStr[16];
  MSecToTimeString(TimeStamps[0].Timems, TimeStr);
  printf(" erster  TimeStamp[    0]:  Block=%8u,  Time= %s\n", TimeStamps[0].BlockNr, TimeStr);
  MSecToTimeString(TimeStamps[NrTimeStamps-1].Timems, TimeStr);
  printf(" letzter TimeStamp[%5u]:  Block=%8u,  Time= %s\n", NrTimeStamps-1, TimeStamps[NrTimeStamps-1].BlockNr, TimeStr);
  printf(" Anzahl SegmentMarker: %d\n", NrSegmentMarker);
  for(i=0; i<NrSegmentMarker; i++) {
    MSecToTimeString(SegmentMarker[i].Timems, TimeStr);
    printf("  %2d. Segment:\tBlock=%8u,  Time= %s,  Sel=%s,  Perct=%2.1f\n", i, SegmentMarker[i].Block, TimeStr, ((SegmentMarker[i].Selected)?"y":"n"), SegmentMarker[i].Percent);
  }
  MSecToTimeString(newTime, TimeStr);
  printf(" NEU Segment:\tBlock=%8u,  Time= %s,  Percent=%2.1f\n", Block, TimeStr, ((float)Block / PlayInfo.totalBlock)*100);
#endif

  //Find the point where to insert the new marker so that the list stays sorted
  if(NrSegmentMarker < 2)
  {
    //If less than 2 markers present, then set marker for start and end of file (called from AddDefaultSegmentMarker)
    SegmentMarker[NrSegmentMarker].Block = Block;
    SegmentMarker[NrSegmentMarker].Timems  = newTime;
    SegmentMarker[NrSegmentMarker].Percent = ((float)Block / PlayInfo.totalBlock) * 100;
    SegmentMarker[NrSegmentMarker].Selected = FALSE;
    NrSegmentMarker++;
  }
  else
  {
    for(i = 1; i < NrSegmentMarker; i++)
    {
      if(SegmentMarker[i].Block > Block)
      {
        // Erlaube kein Segment mit weniger als 3 Sekunden
        if (RejectSmallSegments && ((SegmentMarker[i-1].Block + 3*BlocksOneSecond >= Block) || (Block + 3*BlocksOneSecond >= SegmentMarker[i].Block)))
        {
          #if STACKTRACE == TRUE
            CallTraceExit(NULL);
          #endif
          return FALSE;
        }

        for(j = NrSegmentMarker; j > i; j--)
          memcpy(&SegmentMarker[j], &SegmentMarker[j - 1], sizeof(tSegmentMarker));

        SegmentMarker[i].Block = Block;
        SegmentMarker[i].Timems  = newTime;
        SegmentMarker[i].Percent = ((float)Block / PlayInfo.totalBlock) * 100;
        SegmentMarker[i].Selected = FALSE;

        MSecToTimeString(SegmentMarker[i].Timems, StartTime);
        TAP_SPrint(LogString, "New marker @ block = %u, time = %s, percent = %1.1f%%", Block, StartTime, SegmentMarker[i].Percent);
        WriteLogMC(PROGRAM_NAME, LogString);
        break;
      }
    }
    NrSegmentMarker++;
  }

#ifdef FULLDEBUG
  printf("----------- nachher ---------------\n");
  printf(" PlayInfo.currentBlock: %u,  PlayInfo.totalBlock: %u\n", PlayInfo.currentBlock, PlayInfo.totalBlock);
  MSecToTimeString(TimeStamps[0].Timems, TimeStr);
  printf(" erster  TimeStamp[    0]:  Block=%8u,  Time= %s\n", TimeStamps[0].BlockNr, TimeStr);
  MSecToTimeString(TimeStamps[NrTimeStamps-1].Timems, TimeStr);
  printf(" letzter TimeStamp[%5u]:  Block=%8u,  Time= %s\n", NrTimeStamps-1, TimeStamps[NrTimeStamps-1].BlockNr, TimeStr);
  printf(" Anzahl SegmentMarker: %d\n", NrSegmentMarker);
  for(i=0; i<NrSegmentMarker; i++) {
    MSecToTimeString(SegmentMarker[i].Timems, TimeStr);
    printf("  %2d. Segment:\tBlock=%8u,  Time= %s,  Sel=%s,  Perct=%2.1f\n", i, SegmentMarker[i].Block, TimeStr, ((SegmentMarker[i].Selected)?"y":"n"), SegmentMarker[i].Percent);
  }
#endif

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
  return TRUE;
}

int FindNearestSegmentMarker(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("FindNearestSegmentMarker");
  #endif

  int                   NearestMarkerIndex;
  long                  MinDelta;
  int                   i;

  NearestMarkerIndex = -1;
  if(NrSegmentMarker > 2)   // at least one segment marker present
  {
    MinDelta = 0x7fffffff;
    for(i = 1; i < NrSegmentMarker - 1; i++)
    {
      if(labs(SegmentMarker[i].Block - PlayInfo.currentBlock) < MinDelta)
      {
        MinDelta = labs(SegmentMarker[i].Block - PlayInfo.currentBlock);
        NearestMarkerIndex = i;
      }
    }
  }

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
  return NearestMarkerIndex;
}

bool MoveSegmentMarker(dword newBlock)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("MoveSegmentMarker");
  #endif

  dword newTime;
  int NearestMarkerIndex = FindNearestSegmentMarker();

  if(NearestMarkerIndex != -1)
  {
    // Erlaube kein Segment mit weniger als 3 Sekunden
    if ((SegmentMarker[NearestMarkerIndex-1].Block + 3*BlocksOneSecond < newBlock) && (newBlock + 3*BlocksOneSecond < SegmentMarker[NearestMarkerIndex+1].Block))
    {
      // neue Zeit ermitteln
      newTime = NavGetBlockTimeStamp(newBlock);
      if((newTime != 0) || (newBlock <= 3 * BlocksOneSecond))
      {
        SegmentMarker[NearestMarkerIndex].Block = newBlock;
        SegmentMarker[NearestMarkerIndex].Timems = newTime;
        SegmentMarker[NearestMarkerIndex].Percent = (float)PlayInfo.currentBlock * 100 / PlayInfo.totalBlock;

        #if STACKTRACE == TRUE
          CallTraceExit(NULL);
        #endif
        return TRUE;
      }
    }
  }

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
  return FALSE;
}

bool DeleteSegmentMarker(word MarkerIndex)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("DeleteSegmentMarker");
  #endif

  int i;

  if((MarkerIndex <= 0) || (MarkerIndex >= (NrSegmentMarker - 1)))
  {
    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return FALSE;
  }

  for(i = MarkerIndex; i < NrSegmentMarker - 1; i++)
    memcpy(&SegmentMarker[i], &SegmentMarker[i + 1], sizeof(tSegmentMarker));

//  memset(&SegmentMarker[NrSegmentMarker - 1], 0, sizeof(tSegmentMarker));
  NrSegmentMarker--;

  SegmentMarker[NrSegmentMarker - 1].Selected = FALSE;        // the very last marker (no segment)
  if(NrSegmentMarker < 3) SegmentMarker[0].Selected = FALSE;  // there is only one segment (from start to end)
  if(ActiveSegment >= NrSegmentMarker - 1) ActiveSegment = NrSegmentMarker - 2;

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
  return TRUE;
}

void DeleteAllSegmentMarkers(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("DeleteAllSegmentMarkers");
  #endif

  if (NrSegmentMarker > 2) {
    memcpy(&SegmentMarker[1], &SegmentMarker[NrSegmentMarker-1], sizeof(tSegmentMarker));
//    memset(&SegmentMarker[2], 0, (NrSegmentMarker-2) * sizeof(tSegmentMarker));
    NrSegmentMarker = 2;
  }
  SegmentMarker[0].Selected = FALSE;
  ActiveSegment = 0;

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void SelectSegmentMarker(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("SelectSegmentMarker");
  #endif

  if (NrSegmentMarker > 2)
    SegmentMarker[ActiveSegment].Selected = !SegmentMarker[ActiveSegment].Selected;

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}


// ----------------------------------------------------------------------------
//                           Bookmark-Funktionen
// ----------------------------------------------------------------------------
//Bookmarks werden jetzt aus dem inf-Cache der Firmware aus dem Speicher ausgelesen.
//Das geschieht bei jedem Einblenden des MC-OSDs, da sie sonst nicht benötigt werden
bool ReadBookmarks(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("ReadBookmarks");
  #endif

  dword                *PlayInfoBookmarkStruct;
  byte                 *TempRecSlot;

  PlayInfoBookmarkStruct = NULL;
  TempRecSlot = (byte*)FIS_vTempRecSlot();
  if(TempRecSlot) PlayInfoBookmarkStruct = (dword*)HDD_GetPvrRecTsPlayInfoPointer(*TempRecSlot);

  if(PlayInfoBookmarkStruct)
  {
    NrBookmarks = PlayInfoBookmarkStruct[0];
    memset(Bookmarks, 0, sizeof(Bookmarks));
    memcpy(Bookmarks, &PlayInfoBookmarkStruct[1], NrBookmarks * sizeof(dword));
  }
  else
  {
    NrBookmarks = 0;
    WriteLogMC(PROGRAM_NAME, "ReadBookmarks: Fatal error - inf cache entry point not found!");

    char s[512];
    TAP_SPrint(s, "TempRecSlot=%p", TempRecSlot);
    if(TempRecSlot) TAP_SPrint(&s[strlen(s)], ", *TempRecSlot=%d, HDD_NumberOfRECSlots()=%d", *TempRecSlot, HDD_NumberOfRECSlots());
    WriteLogMC(PROGRAM_NAME, s);
  }

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
  return(PlayInfoBookmarkStruct != NULL);
}

//Experimentelle Methode, um Bookmarks direkt in der Firmware abzuspeichern.
bool SaveBookmarks(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("SaveBookmarks");
  #endif

  dword                *PlayInfoBookmarkStruct;
  byte                 *TempRecSlot;

  PlayInfoBookmarkStruct = NULL;
  TempRecSlot = (byte*)FIS_vTempRecSlot();
  if(TempRecSlot) PlayInfoBookmarkStruct = (dword*)HDD_GetPvrRecTsPlayInfoPointer(*TempRecSlot);

  if(PlayInfoBookmarkStruct)
  {
    PlayInfoBookmarkStruct[0] = NrBookmarks;
    memset(&PlayInfoBookmarkStruct[1], 0, NRBOOKMARKS * sizeof(dword));
    memcpy(&PlayInfoBookmarkStruct[1], Bookmarks, sizeof(Bookmarks));
  }
  else
  {
    NrBookmarks = 0;
    WriteLogMC(PROGRAM_NAME, "SaveBookmarks: Fatal error - inf cache entry point not found!");
  }

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
  return(PlayInfoBookmarkStruct != NULL);
}

bool SaveBookmarksToInf(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("SaveBookmarksToInf");
  #endif

  char                  InfFileName[MAX_FILE_NAME_SIZE + 1];
  tRECHeaderInfo        RECHeaderInfo;
  byte                 *Buffer;
  TYPE_File            *fInf;
  dword                 FileSize, BufferSize;
  int                   i;

#ifdef FULLDEBUG
  TAP_PrintNet("SaveBookmarksToInf()\n");
  for (i = 0; i < NrBookmarks; i++) {
    TAP_PrintNet("%u\n", Bookmarks[i]);
  }
#endif

  //Read inf
  HDD_ChangeDir(PlaybackDir);
  TAP_SPrint(InfFileName, "%s.inf", PlaybackName);
  fInf = TAP_Hdd_Fopen(InfFileName);
  if(!fInf)
  {
    WriteLogMC(PROGRAM_NAME, "SaveBookmarksToInf: failed to open the inf file!");

    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return FALSE;
  }

  FileSize = TAP_Hdd_Flen(fInf);
  BufferSize = (FileSize < 8192 ? 8192 : FileSize);
  Buffer = (byte*) TAP_MemAlloc(BufferSize);
  memset(Buffer, 0, BufferSize);
  TAP_Hdd_Fread(Buffer, FileSize, 1, fInf);

  //decode inf
  HDD_DecodeRECHeader(Buffer, &RECHeaderInfo, ST_UNKNOWN);

  //add new Bookmarks
  for (i = 0; i < NrBookmarks; i++) {
    RECHeaderInfo.Bookmark[i] = Bookmarks[i];
  }
  for (i = NrBookmarks; i < NRBOOKMARKS; i++) {
    RECHeaderInfo.Bookmark[i] = 0;
  }
  RECHeaderInfo.NrBookmarks = NrBookmarks;

  //enconde inf
  HDD_EncodeRECHeader(Buffer, &RECHeaderInfo, ST_UNKNOWN);

  //write inf
  TAP_Hdd_Fseek(fInf, 0, SEEK_SET);
  TAP_Hdd_Fwrite(Buffer, FileSize, 1, fInf);
  TAP_Hdd_Fclose(fInf);
  TAP_MemFree(Buffer);

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
  return TRUE;
}

void ExportSegmentsToBookmarks(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("ExportSegmentsToBookmarks");
  #endif

  int i;

  if (NrSegmentMarker > 2)
  {
    // first, delete all present bookmarks
    DeleteAllBookmarks();

    // second, add a bookmark for each SegmentMarker
    for(i = 1; i <= min(NRBOOKMARKS, NrSegmentMarker-2); i++)
    { 
      Bookmarks[NrBookmarks] = SegmentMarker[i].Block;
      NrBookmarks++;
    }

    SaveBookmarks();
    OSDInfoDrawProgressbar(TRUE);
//    OSDRedrawEverything();
  }
  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

bool AddBookmark(dword Block)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("AddBookmark");
  #endif

  int i, j;

  if(NrBookmarks >= NRBOOKMARKS)
  {
    WriteLogMC(PROGRAM_NAME, "AddBookmark: Bookmark list is full!");

    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return FALSE;
  }

  //Find the point where to insert the new marker so that the list stays sorted
  if (NrBookmarks == 0)
    Bookmarks[0] = Block;
  else if (Block > Bookmarks[NrBookmarks - 1])
  {
    // Erlaube keinen Abschnitt mit weniger als 3 Sekunden
    if (Bookmarks[NrBookmarks-1] + 3*BlocksOneSecond >= Block)
    {
      #if STACKTRACE == TRUE
        CallTraceExit(NULL);
      #endif
      return FALSE;
    }
    Bookmarks[NrBookmarks] = Block;
  }
  else
  {
    for(i = 0; i < NrBookmarks; i++)
    {
      if(Bookmarks[i] > Block)
      {
        // Erlaube keinen Abschnitt mit weniger als 3 Sekunden
        if ((Bookmarks[i-1] + 3*BlocksOneSecond >= Block) || (Block + 3*BlocksOneSecond >= Bookmarks[i]))
        {
          #if STACKTRACE == TRUE
            CallTraceExit(NULL);
          #endif
          return FALSE;
        }

        for(j = NrBookmarks; j > i; j--)
          Bookmarks[j] = Bookmarks[j - 1];

        Bookmarks[i] = Block;
        break;
      }
    }
  }
  NrBookmarks++;
  SaveBookmarks();

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
  return TRUE;
}

int FindNearestBookmark(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("FindNearestBookmark");
  #endif

  int                   NearestBookmarkIndex;
  long                  MinDelta;
  int                   i;

  NearestBookmarkIndex = -1;
  if(NrBookmarks > 0)
  {
    MinDelta = 0x7fffffff;
    for(i = 0; i < NrBookmarks; i++)
    {
      if(labs(Bookmarks[i] - PlayInfo.currentBlock) < MinDelta)
      {
        MinDelta = labs(Bookmarks[i] - PlayInfo.currentBlock);
        NearestBookmarkIndex = i;
      }
    }
  }

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
  return NearestBookmarkIndex;
}

bool MoveBookmark(dword newBlock)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("MoveBookmark");
  #endif

  int NearestBookmarkIndex = FindNearestBookmark();
  if(NearestBookmarkIndex != -1)
  {
    // Erlaube keinen Abschnitt mit weniger als 3 Sekunden
    if ((Bookmarks[NearestBookmarkIndex-1] + 3*BlocksOneSecond < newBlock) && (newBlock + 3*BlocksOneSecond < Bookmarks[NearestBookmarkIndex+1]))
    {
      Bookmarks[NearestBookmarkIndex] = newBlock;
      SaveBookmarks();

      #if STACKTRACE == TRUE
        CallTraceExit(NULL);
      #endif
      return TRUE;
    }
  }

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
  return FALSE;
}

bool DeleteBookmark(word BookmarkIndex)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("DeleteBookmark");
  #endif

  bool ret = FALSE;
  int i;

  if (BookmarkIndex < NrBookmarks)
  {
    for(i = BookmarkIndex; i < NrBookmarks - 1; i++)
      Bookmarks[i] = Bookmarks[i + 1];
    Bookmarks[NrBookmarks - 1] = 0;

    NrBookmarks--;
    SaveBookmarks();
    ret = TRUE;
  }

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
  return ret;
}

void DeleteAllBookmarks(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("DeleteAllBookmarks");
  #endif

  int i;

  for(i = 0; i < NrBookmarks; i++)
    Bookmarks[i] = 0;

  NrBookmarks = 0;
  SaveBookmarks();

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

// ----------------------------------------------------------------------------
//                           CutFile-Funktionen
// ----------------------------------------------------------------------------
bool CutFileLoad(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("CutFileLoad");
  #endif

  char                  Name[MAX_FILE_NAME_SIZE + 1];
  word                  Version = 0;
  word                  Padding;
  TYPE_File            *fCut;
  __off64_t             SavedSize;
  int                   i;
  tTimeStamp           *CurTimeStamp;

  // Create name of cut-file
  strcpy(Name, PlaybackName);
  Name[strlen(Name) - 4] = '\0';
  strcat(Name, ".cut");

  // Check, if cut-File exists
  HDD_ChangeDir(PlaybackDir);
  if(!TAP_Hdd_Exist(Name))
  {
    WriteLogMC(PROGRAM_NAME, "CutFileLoad: .cut doesn't exist!");

    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return FALSE;
  }

  // Try to open cut-File
  fCut = TAP_Hdd_Fopen(Name);
  if(!fCut)
  {
    WriteLogMC(PROGRAM_NAME, "CutFileLoad: failed to open .cut!");

    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return FALSE;
  }

  // Check correct version of cut-file
  TAP_Hdd_Fread(&Version, sizeof(byte), 1, fCut);    // read only one byte for compatibility with V.1  [COMPATIBILITY LAYER]
  if(Version > CUTFILEVERSION)
  {
    WriteLogMC(PROGRAM_NAME, "CutFileLoad: .cut version mismatch!");
    TAP_Hdd_Fclose(fCut);

    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return FALSE;
  }
  if (Version > 1)
    TAP_Hdd_Fread(&Padding, sizeof(byte), 1, fCut);  // read the second byte of Version (if not V.1)  [COMPATIBILITY LAYER]

  // Check, if size of rec-File has been changed
  TAP_Hdd_Fread(&SavedSize, sizeof(__off64_t), 1, fCut);
  if(RecFileSize != SavedSize)
  {
    WriteLogMC(PROGRAM_NAME, "CutFileLoad: .cut file size mismatch!");
/*    TAP_Hdd_Fclose(fCut);

    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return FALSE; */
  }

  // Read data from .cut-File
  NrSegmentMarker = 0;
  ActiveSegment = 0;
  TAP_Hdd_Fread(&NrSegmentMarker, sizeof(word), 1, fCut);
  if (NrSegmentMarker > NRSEGMENTMARKER) NrSegmentMarker = NRSEGMENTMARKER;
  if (Version == 1)
    TAP_Hdd_Fread(&Padding, sizeof(word), 1, fCut);  // read the second word of NrSegmentMarker (if V.1)  [COMPATIBILITY LAYER]
  TAP_Hdd_Fread(&ActiveSegment, sizeof(word), 1, fCut);
  TAP_Hdd_Fread(&Padding, sizeof(word), 1, fCut);
  TAP_Hdd_Fread(SegmentMarker, sizeof(tSegmentMarker), NrSegmentMarker, fCut);
  TAP_Hdd_Fclose(fCut);

  // Sonderfunktion: Import von Cut-Files mit
  if (RecFileSize != SavedSize)
  {
    if ((NrSegmentMarker > 2) && (TimeStamps != NULL))
    {
      char curTimeStr[16];
      WriteLogMC(PROGRAM_NAME, "CutFileLoad: Importing timestamps only, recalculating block numbers..."); 

      SegmentMarker[0].Block = 0;
      SegmentMarker[0].Timems = NavGetBlockTimeStamp(0);
      SegmentMarker[0].Selected = FALSE;
      SegmentMarker[NrSegmentMarker - 1].Block = 0xFFFFFFFF;

      CurTimeStamp = TimeStamps;
      for (i = 1; i <= NrSegmentMarker-2; i++)
      {
        if (Version == 1)
          SegmentMarker[i].Timems = SegmentMarker[i].Timems * 1000;

        MSecToTimeString(SegmentMarker[i].Timems, curTimeStr);
        TAP_SPrint(LogString, "%2u.)  oldTimeStamp=%s   oldBlock=%u   ", i+1, curTimeStr, SegmentMarker[i].Block);

        if ((SegmentMarker[i].Timems <= CurTimeStamp->Timems) || (CurTimeStamp >= TimeStamps + NrTimeStamps-1))
        {
          if (DeleteSegmentMarker(i)) i--;
          TAP_SPrint(&LogString[strlen(LogString)], "--> Smaller than last TimeStamp or end of nav reached. Deleted!");
        }
        else
        {
          while ((SegmentMarker[i].Timems > CurTimeStamp->Timems) && (CurTimeStamp < TimeStamps + NrTimeStamps-1))
            CurTimeStamp++;
          if (SegmentMarker[i].Timems < CurTimeStamp->Timems)
            CurTimeStamp--;
        
          if (CurTimeStamp->BlockNr < PlayInfo.totalBlock)
          {
            SegmentMarker[i].Block = CurTimeStamp->BlockNr;
            SegmentMarker[i].Timems = NavGetBlockTimeStamp(SegmentMarker[i].Block);
            SegmentMarker[i].Selected = FALSE;
            MSecToTimeString(SegmentMarker[i].Timems, curTimeStr);
            TAP_SPrint(&LogString[strlen(LogString)], "--> newBlock=%u   newTimeStamp=%s", SegmentMarker[i].Block, curTimeStr);
          }
          else
          {
            if (DeleteSegmentMarker(i)) i--;
            TAP_SPrint(&LogString[strlen(LogString)], "--> TotalBlocks exceeded. Deleted!", SegmentMarker[i].Block, curTimeStr);        
          }
        }
        WriteLogMC(PROGRAM_NAME, LogString);
      }
    }
    else
    {
      WriteLogMC(PROGRAM_NAME, "CutFileLoad: Less than 2 timestamps imported, or NavTimeStamps=NULL!"); 
      NrSegmentMarker = 0;
    }
  }

  // Wenn zu wenig Segmente -> auf Standard zurücksetzen
  if (NrSegmentMarker <= 2)
  {
    NrSegmentMarker = 0;
    ActiveSegment = 0;
    WriteLogMC(PROGRAM_NAME, "CutFileLoad: Less than 2 timestamps imported, resetting!"); 
    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return FALSE;
  }

  // Prozent-Angaben neu berechnen (müssen künftig nicht mehr in der .cut gespeichert werden)
  for (i = 0; i < NrSegmentMarker; i++)
  {
    SegmentMarker[i].Percent = ((float)SegmentMarker[i].Block / PlayInfo.totalBlock) * 100;
  }

  // Wenn letzter Segment-Marker ungleich TotalBlock ist -> anpassen
  if (SegmentMarker[NrSegmentMarker - 1].Block != PlayInfo.totalBlock) {
#ifdef FULLDEBUG
  TAP_SPrint(LogString, "CutFileLoad: Letzter Segment-Marker %u ist ungleich TotalBlock %u!", SegmentMarker[NrSegmentMarker - 1].Block, PlayInfo.totalBlock);
  WriteLogMC(PROGRAM_NAME, LogString);
#endif
    SegmentMarker[NrSegmentMarker - 1].Block = PlayInfo.totalBlock;
    dword newTime = NavGetBlockTimeStamp(SegmentMarker[NrSegmentMarker - 1].Block);
    SegmentMarker[NrSegmentMarker - 1].Timems = (newTime) ? newTime : (dword)(1000 * (60*PlayInfo.duration + PlayInfo.durationSec));
    SegmentMarker[NrSegmentMarker - 1].Percent = 100;
  }

  // Markierungen und ActiveSegment prüfen, ggf. korrigieren
  SegmentMarker[NrSegmentMarker - 1].Selected = FALSE;        // the very last marker (no segment)
  if(NrSegmentMarker <= 2) SegmentMarker[0].Selected = FALSE;  // there is only one segment (from start to end)
  if(ActiveSegment >= NrSegmentMarker - 1) ActiveSegment = NrSegmentMarker - 2;

  // Wenn cut-File Version 1 hat, dann ermittle neue TimeStamps und vergleiche diese mit den alten (DEBUG)  [COMPATIBILITY LAYER]
  if ((Version == 1) && (RecFileSize == SavedSize)) {
    int i;
    dword oldTime, newTime;
    char oldTimeStr[12], newTimeStr[16];

    WriteLogMC(PROGRAM_NAME, "Import of old cut file version (compare old and new TimeStamps!)");
    for (i = 0; i < NrSegmentMarker; i++) {
      oldTime = SegmentMarker[i].Timems;
      newTime = NavGetBlockTimeStamp(SegmentMarker[i].Block);
      SegmentMarker[i].Timems = (newTime) ? newTime : oldTime;
      SecToTimeString(oldTime, oldTimeStr);
      MSecToTimeString(newTime, newTimeStr);
      TAP_SPrint(LogString, " %s%2u.)  BlockNr=%8u   oldTimeStamp=%s   newTimeStamp=%s", (labs(oldTime*1000-newTime) > 1000) ? "!!" : "  ", i+1, SegmentMarker[i].Block, oldTimeStr, newTimeStr);
      WriteLogMC(PROGRAM_NAME, LogString);
    }
  }

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
  return TRUE;
}

void CutFileSave(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("CutFileSave");
  #endif

  char                  Name[MAX_FILE_NAME_SIZE + 1];
  word                  Version;
  TYPE_File            *fCut;

/*  //Save the file size to check if the file didn't change
  fRec = TAP_Hdd_Fopen(PlaybackName);
  if(!fRec)
  {
    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return;
  }

  FileSize = fRec->size;
  TAP_Hdd_Fclose(fRec); */

//  RecFileSize = HDD_GetFileSize(PlaybackName);
//  if(RecFileSize <= 0)

  if(NrSegmentMarker <= 2)
  {
    CutFileDelete();
    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return;
  }

  if(!HDD_GetFileSizeAndInode(PlaybackDir, PlaybackName, NULL, &RecFileSize))
  {
    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return;
  }

  Version = CUTFILEVERSION;
  strcpy(Name, PlaybackName);
  Name[strlen(Name) - 4] = '\0';
  strcat(Name, ".cut");

#ifdef FULLDEBUG
  char CurDir[512];
  HDD_TAP_GetCurrentDir(CurDir);
  TAP_SPrint(LogString, "CutFileSave()! CurrentDir: %s, PlaybackName: %s, CutFileName: %s", CurDir, PlaybackName, Name);
  WriteLogMC(PROGRAM_NAME, LogString);
#endif

  HDD_ChangeDir(PlaybackDir);
  TAP_Hdd_Delete(Name);
  TAP_Hdd_Create(Name, ATTR_NORMAL);
  fCut = TAP_Hdd_Fopen(Name);
  if(!fCut)
  {
    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return;
  }

  SegmentMarker[NrSegmentMarker - 1].Selected = FALSE;
  if(NrSegmentMarker < 3)SegmentMarker[0].Selected = FALSE;

  TAP_Hdd_Fwrite(&Version, sizeof(word), 1, fCut);
  TAP_Hdd_Fwrite(&RecFileSize, sizeof(__off64_t), 1, fCut);
  TAP_Hdd_Fwrite(&NrSegmentMarker, sizeof(word), 1, fCut);
  TAP_Hdd_Fwrite(&ActiveSegment, sizeof(word), 1, fCut);
word Padding=0;
  TAP_Hdd_Fwrite(&Padding, sizeof(word), 1, fCut);
  TAP_Hdd_Fwrite(SegmentMarker, sizeof(tSegmentMarker), NrSegmentMarker, fCut);
  TAP_Hdd_Fclose(fCut);

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void CutFileDelete(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("CutFileDelete");
  #endif

  char                  Name[MAX_FILE_NAME_SIZE + 1];

  HDD_ChangeDir(PlaybackDir);
  strcpy(Name, PlaybackName);
  Name[strlen(Name) - 4] = '\0';
  strcat(Name, ".cut");
  TAP_Hdd_Delete(Name);

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void CutDumpList(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("CutDumpList");
  #endif

  char                  TimeString[16];
  int                   i;

  WriteLogMC(PROGRAM_NAME, "Seg      Block Time        Pct Sel Act");
  for(i = 0; i < NrSegmentMarker; i++)
  {
    MSecToTimeString(SegmentMarker[i].Timems, TimeString);
    TAP_SPrint(LogString, "%02d: %010d %s %03d %3s %3s", i, SegmentMarker[i].Block, TimeString, (int)SegmentMarker[i].Percent, SegmentMarker[i].Selected ? "yes" : "no", (i == ActiveSegment ? "*" : ""));
    WriteLogMC(PROGRAM_NAME, LogString);
  }

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}


// ----------------------------------------------------------------------------
//                              OSD-Funktionen
// ----------------------------------------------------------------------------
void CreateOSD(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("CreateOSD");
  #endif

  if(!rgnSegmentList)
  {
    TAP_ExitNormal();
    TAP_EnterNormalNoInfo();
    TAP_ExitNormal();
    rgnSegmentList = TAP_Osd_Create(28 +22 /***CW***/, 85 -3, _SegmentList_Background_Gd.width, _SegmentList_Background_Gd.height, 0, 0);
  }
  if(!rgnInfo) rgnInfo = TAP_Osd_Create(0, 576 - _Info_Background_Gd.height, _Info_Background_Gd.width, _Info_Background_Gd.height, 0, 0);

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void OSDRedrawEverything(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("OSDRedrawEverything");
  #endif

  CreateOSD();
  OSDSegmentListDrawList();
  OSDInfoDrawBackground();
  OSDInfoDrawRecName();
  OSDInfoDrawProgressbar(TRUE);
  OSDInfoDrawPlayIcons(TRUE);
  SetCurrentSegment();
  OSDInfoDrawCurrentPosition(TRUE);
  OSDInfoDrawClock(TRUE);
  OSDInfoDrawBookmarkMode();
  OSDInfoDrawMinuteJump();

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

// Segment-Liste
// -------------
void OSDSegmentListDrawList(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("OSDSegmentListDrawList");
  #endif

  const int             RegionWidth    = 164,  BelowTextArea_Y = 307;
  const int             StartTextField_X = 5,  StartTextField_Y = 29,  EndTextField_X = 148;
  const int             TextFieldHeight = 26,  TextFieldDist    =  2;
  const int             Scrollbar_X = 150,     Scrollbar_Y = 40,     /*ScrollbarWidth = 10,*/    ScrollbarHeight = 256;
  const int             NrWidth     = FMUC_GetStringWidth("99.", &Calibri_12_FontDataUC);
  const int             DashWidth   = FMUC_GetStringWidth(" - ", &Calibri_12_FontDataUC);
  const int             TimeWidth   = FMUC_GetStringWidth("99:99:99", &Calibri_12_FontDataUC);
  const dword           C1 = COLOR_Yellow;
  const dword           C2 = COLOR_White;

  word                  CurrentSegment;
  word                  ScrollButtonHeight, ScrollButtonPos;
  char                  StartTime[12], EndTime[12], OutStr[5];
  char                  PageNrStr[8], *PageStr;
  int                   p, NrPages;
  int                   Start;
  dword                 PosX, PosY, UseColor;
  word                  i;

  if(rgnSegmentList)
  {
    if (JumpRequestedSegment != 0xFFFF)
      CurrentSegment = JumpRequestedSegment;
    else
      CurrentSegment = (word)ActiveSegment;
    if(CurrentSegment >= NrSegmentMarker - 1)
      CurrentSegment = NrSegmentMarker - 2;

    // Hintergrund, Überschrift, Buttons
    TAP_Osd_PutGd(rgnSegmentList, 0, 0, &_SegmentList_Background_Gd, FALSE);
    FMUC_PutString(rgnSegmentList, 0, 2, RegionWidth, LangGetString(LS_SegmentList), COLOR_White, COLOR_None, &Calibri_14_FontDataUC, TRUE, ALIGN_CENTER);
    TAP_Osd_PutGd(rgnSegmentList, EndTextField_X - 2*19 - 3, BelowTextArea_Y + 6, &_Button_Up_small_Gd, TRUE);
    TAP_Osd_PutGd(rgnSegmentList, EndTextField_X - 19,       BelowTextArea_Y + 6, &_Button_Down_small_Gd, TRUE);

    // Seitenzahl
    NrPages = ((NrSegmentMarker - 2) / 10) + 1;
    p       = (CurrentSegment / 10) + 1;
    PageStr = LangGetString(LS_PageStr);
    TAP_SPrint(PageNrStr, "%d/%d", p, NrPages);

    PosX = StartTextField_X + 11;
    PosY = BelowTextArea_Y + 3;
    FMUC_PutString(rgnSegmentList, PosX, PosY, EndTextField_X - 2*19 - 3, PageStr,   COLOR_White, COLOR_None, &Calibri_10_FontDataUC, FALSE, ALIGN_LEFT);
    PosX += FMUC_GetStringWidth(PageStr, &Calibri_10_FontDataUC);

    FMUC_PutString(rgnSegmentList, PosX, PosY, EndTextField_X - 2*19 - 3, PageNrStr, COLOR_White, COLOR_None, &Calibri_10_FontDataUC, FALSE, ALIGN_LEFT);

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
//            TAP_Osd_PutGd(rgnSegmentList, 3, 44 + 28*i, &_Selection_Red_Gd, FALSE);
//          else
          TAP_Osd_PutGd(rgnSegmentList, StartTextField_X, StartTextField_Y + i*(TextFieldHeight+TextFieldDist), &_Selection_Blue_Gd, FALSE);
        }

        SecToTimeString((SegmentMarker[Start + i].Timems) / 1000, StartTime);
        SecToTimeString((SegmentMarker[Start + i + 1].Timems) / 1000, EndTime);

        PosX = StartTextField_X;
        PosY = StartTextField_Y + i*(TextFieldHeight+TextFieldDist) + 3;
        UseColor = (SegmentMarker[Start + i].Selected) ? C1 : C2;

        TAP_SPrint(OutStr, "%d.", Start + i + 1);
        if (Start + i + 1 >= 100) TAP_SPrint(OutStr, "00.");
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
      TAP_Osd_PutGd(rgnSegmentList, EndTextField_X, StartTextField_Y, &_SegmentList_ScrollBar_Gd, FALSE);
      ScrollButtonHeight = (ScrollbarHeight / NrPages);
      ScrollButtonPos    = Scrollbar_Y + (ScrollbarHeight * (p - 1) / NrPages);
//      TAP_Osd_Draw3dBoxFill(rgnSegmentList, Scrollbar_X+2, ScrollButtonPos, ScrollbarWidth-4, ScrollButtonHeight, COLOR_Yellow, COLOR_Red, COLOR_DarkGray);
      for (i = 0; i < ScrollButtonHeight; i++)
        TAP_Osd_PutGd(rgnSegmentList, Scrollbar_X, ScrollButtonPos + i, &_SegmentList_ScrollButton_Gd, FALSE);
    }
  }

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

// SetCurrentSegment
// -----------------
void SetCurrentSegment(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("SetCurrentSegment");
  #endif

  int                   i;

  if(NrSegmentMarker <= 2)
  {
    ActiveSegment = 0;
    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return;
  }
  if (JumpRequestedTime || (JumpPerformedTime && (labs(TAP_GetTick() - JumpPerformedTime) < 75)))
  {
    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return;
  }
  JumpRequestedSegment = 0xFFFF;
  JumpPerformedTime = 0;

  for(i = 1; i < NrSegmentMarker; i++)
  {
    if(SegmentMarker[i].Block > PlayInfo.currentBlock)
    {
      if(ActiveSegment != (i - 1))
      {
        ActiveSegment = i - 1;
        OSDSegmentListDrawList();
        OSDInfoDrawProgressbar(TRUE);
      }
      break;
    }
  }

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

// Fortschrittsbalken
// ------------------
void OSDInfoDrawProgressbar(bool Force)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("OSDInfoDrawProgressbar");
  #endif

  int                   y, i;
  dword                 pos = 0;
  dword                 x1, x2;
  dword                 totalBlock;
  static dword          LastDraw = 0;
  static dword          LastPos = 999;

  totalBlock = PlayInfo.totalBlock;
  if(rgnInfo)
  {
    if((labs(TAP_GetTick() - LastDraw) > 20) || Force)
    {
      if(totalBlock) pos = (dword)((float)PlayInfo.currentBlock * 653 / totalBlock);

      if(Force || (pos != LastPos))
      {
        LastPos = pos;

        //The background
        TAP_Osd_PutGd(rgnInfo, 28, 90, &_Info_Progressbar_Gd, FALSE);

        //Fill the active segment
        if (NrSegmentMarker >= 3)
        {
          x1 = 34 + (int)((float)653 * SegmentMarker[ActiveSegment].Percent / 100);
          x2 = 34 + (int)((float)653 * SegmentMarker[ActiveSegment + 1].Percent / 100);

//          if((SegmentMarker[ActiveSegment + 1].Timems - SegmentMarker[ActiveSegment].Timems) < 60001)
//            TAP_Osd_FillBox(rgnInfo, x1, 102, x2 - x1, 10, RGB(238, 63, 63));
//          else
            TAP_Osd_FillBox(rgnInfo, x1, 102, x2 - x1, 10, RGB(73, 206, 239));
        }

        //SegmentMarker: 0% = 31/93,  100% = 683/93
        //Draw the selection for segment 0
        if(SegmentMarker[0].Selected)
        {
          x1 = 34 + (int)((float)653 * SegmentMarker[0].Percent / 100);
          x2 = 34 + (int)((float)653 * SegmentMarker[1].Percent / 100);
          TAP_Osd_DrawRectangle(rgnInfo, x1, 102, x2 - x1, 10, 2, COLOR_Blue);
        }

        for(i = 1; i < NrSegmentMarker - 1; i++)
        {
          //Draw the selection
          if(SegmentMarker[i].Selected)
          {
            x1 = 34 + (int)((float)653 * SegmentMarker[i].Percent / 100);
            x2 = 34 + (int)((float)653 * SegmentMarker[i + 1].Percent / 100);
            TAP_Osd_DrawRectangle(rgnInfo, x1, 102, x2 - x1, 10, 2, COLOR_Blue);
          }

          //Draw the segment marker
          if(totalBlock && SegmentMarker[i].Block <= totalBlock)
          {
            pos = (dword)((float)SegmentMarker[i].Block * 653 / totalBlock);
            TAP_Osd_PutGd(rgnInfo, 31 + pos, 93, ((BookmarkMode) ? &_SegmentMarker_gray_Gd : &_SegmentMarker_Gd), TRUE);
          }
        }

        // Draw requested jump
        if (JumpRequestedSegment != 0xFFFF)
        {
          x1 = 34 + (int)((float)653 * SegmentMarker[JumpRequestedSegment].Percent / 100);
          x2 = 34 + (int)((float)653 * SegmentMarker[JumpRequestedSegment + 1].Percent / 100);
          TAP_Osd_DrawRectangle(rgnInfo, x1, 102, x2 - x1, 10, 2, RGB(73, 206, 239));
        }

        //Bookmarks: 0% = 31/112, 100% = 683/112
        for(i = 0; i < NrBookmarks; i++)
        {
          if(totalBlock && (Bookmarks[i] <= totalBlock))
          {
            pos = (dword)((float)Bookmarks[i] * 653 / totalBlock);
            TAP_Osd_PutGd(rgnInfo, 31 + pos, 112, ((BookmarkMode) ? &_BookmarkMarker_Gd : &_BookmarkMarker_gray_Gd), TRUE);
          }
        }

        //Draw the current position
        //0% = X34, 100% = X686
        for(y = 102; y < 112; y++)
          TAP_Osd_PutPixel(rgnInfo, 34 + LastPos, y, COLOR_DarkRed);

        TAP_Osd_Sync();
      }

      LastDraw = TAP_GetTick();
    }
  }

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void OSDInfoDrawCurrentPosition(bool Force)
{
  dword                 Time;
  float                 Percent;
  char                  TimeString[12];
  char                  PercentString[10];
  dword                 fw;
  static byte           LastSec = 99;
//  static dword          LastDraw = 0;  // TODO: ich nehme an, es kann in 2 verschiedenen Funktionen je eine lokale statische Variable mit gleichem Namen aber unterschiedlichem Inhalt geben...
//  static dword          maxBlock = 0;

  #if STACKTRACE == TRUE
    CallTraceEnter("OSDInfoDrawCurrentPosition");
  #endif

  // Experiment: Stabilisierung der vor- und zurückspringenden Zeit-Anzeige (noch linear)
//  if (PlayInfo.currentBlock > maxBlock) maxBlock = PlayInfo.currentBlock;
    
//  if((labs(TAP_GetTick() - LastDraw) > 10) || Force)
//  {
    if(rgnInfo && (PlayInfo.totalBlock > 0))
    {
      Time = NavGetBlockTimeStamp(PlayInfo.currentBlock) / 1000;
      if(((Time % 60) != LastSec) || Force)
      {
        SecToTimeString(Time, TimeString);
        Percent = (float)PlayInfo.currentBlock / PlayInfo.totalBlock;
        TAP_SPrint(PercentString, " (%1.1f%%)", Percent * 100);
        strcat(TimeString, PercentString);
        TAP_Osd_FillBox(rgnInfo, 60, 48, 283, 31, RGB(30, 30, 30));
        fw = FMUC_GetStringWidth(TimeString, &Calibri_14_FontDataUC);
        FMUC_PutString(rgnInfo, max(0, 200 - (int)(fw >> 1)), 52, 660, TimeString, COLOR_White, COLOR_None, &Calibri_14_FontDataUC, TRUE, ALIGN_LEFT);
        LastSec = Time % 60;
        TAP_Osd_Sync();
      }
    }
//    maxBlock = 0;
//    LastDraw = TAP_GetTick();
//  }

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}


// Info-Bereich
// ------------
void OSDInfoDrawBackground(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("OSDInfoDrawBackground");
  #endif

  int                   x;
  char                 *s;

  if(rgnInfo)
  {
    TAP_Osd_PutGd(rgnInfo, 0, 0, &_Info_Background_Gd, FALSE);

    x = 350;
    TAP_Osd_PutGd(rgnInfo, x, 48, &_Button_Red_Gd, TRUE);
    x += _Button_Red_Gd.width;
    s = LangGetString(LS_Delete);
    FMUC_PutString(rgnInfo, x, 47, 720, s, COLOR_White, COLOR_None, &Calibri_12_FontDataUC, TRUE, ALIGN_LEFT);
    x += FMUC_GetStringWidth(s, &Calibri_12_FontDataUC);

    TAP_Osd_PutGd(rgnInfo, x, 48, &_Button_Green_Gd, TRUE);
    x += _Button_Green_Gd.width;
    s = LangGetString(LS_Add);
    FMUC_PutString(rgnInfo, x, 47, 720, s, COLOR_White, COLOR_None, &Calibri_12_FontDataUC, TRUE, ALIGN_LEFT);
    x += FMUC_GetStringWidth(s, &Calibri_12_FontDataUC);

    TAP_Osd_PutGd(rgnInfo, x, 48, &_Button_Yellow_Gd, TRUE);
    x += _Button_Yellow_Gd.width;
    s = LangGetString(LS_Move);
    FMUC_PutString(rgnInfo, x, 47, 720, s, COLOR_White, COLOR_None, &Calibri_12_FontDataUC, TRUE, ALIGN_LEFT);
    x += FMUC_GetStringWidth(s, &Calibri_12_FontDataUC);

    TAP_Osd_PutGd(rgnInfo, x, 48, &_Button_Blue_Gd, TRUE);
    x += _Button_Blue_Gd.width;
    s = LangGetString(LS_Select);
    FMUC_PutString(rgnInfo, x, 47, 720, s, COLOR_White, COLOR_None, &Calibri_12_FontDataUC, TRUE, ALIGN_LEFT);

    x = 350;
    TAP_Osd_PutGd(rgnInfo, x, 68, &_Button_Menu_Gd, TRUE);
    x += _Button_Menu_Gd.width;
    s = LangGetString(LS_PauseMenu);
    FMUC_PutString(rgnInfo, x, 67, 720, s, COLOR_White, COLOR_None, &Calibri_12_FontDataUC, TRUE, ALIGN_LEFT);
    x += FMUC_GetStringWidth(s, &Calibri_12_FontDataUC);

    TAP_Osd_PutGd(rgnInfo, x, 68, &_Button_Exit_Gd, TRUE);
    x += _Button_Exit_Gd.width;
    s = LangGetString(LS_Exit);
    FMUC_PutString(rgnInfo, x, 67, 720, s, COLOR_White, COLOR_None, &Calibri_12_FontDataUC, TRUE, ALIGN_LEFT);
    x += FMUC_GetStringWidth(s, &Calibri_12_FontDataUC);
    
    BookmarkMode_x = x;
  }

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void OSDInfoDrawRecName(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("OSDInfoDrawRecName");
  #endif

  const dword           TimeWidth = FMUC_GetStringWidth("99:99:99", &Calibri_12_FontDataUC);
  char                  NameStr[MAX_FILE_NAME_SIZE + 1];
  char                  TimeStr[12], *pTimeStr;
  char                 *LastSpace = NULL;
  char                 *EndOfName = NULL;
  int                   EndOfNameWidth;

  if(rgnInfo)
  {
    strcpy(NameStr, PlaybackName);
    NameStr[strlen(NameStr) - 4] = '\0';

    if (FMUC_GetStringWidth(NameStr, &Calibri_14_FontDataUC) > 500 - TimeWidth - 65)
    {
      LastSpace = strrchr(NameStr, ' ');
      if (LastSpace)
      {
        LastSpace[0] = '*';
        EndOfName = strrchr(NameStr, ' ');
        LastSpace[0] = ' ';
      }

      if (!EndOfName)
      {
        if (LastSpace)
          EndOfName = LastSpace;
        else
          EndOfName = &NameStr[strlen(NameStr) - 10];
      }
      EndOfNameWidth = FMUC_GetStringWidth(EndOfName, &Calibri_14_FontDataUC);
      FMUC_PutString(rgnInfo, 65, 11, 500-TimeWidth-EndOfNameWidth, NameStr, COLOR_White, COLOR_None, &Calibri_14_FontDataUC, TRUE, ALIGN_LEFT);
      FMUC_PutString(rgnInfo, 500-TimeWidth-EndOfNameWidth, 11, 500-TimeWidth, EndOfName, COLOR_White, COLOR_None, &Calibri_14_FontDataUC, FALSE, ALIGN_LEFT);
    }
    else
      FMUC_PutString(rgnInfo, 65, 11, 500-TimeWidth, NameStr, COLOR_White, COLOR_None, &Calibri_14_FontDataUC, FALSE, ALIGN_LEFT);

    SecToTimeString(PlayInfo.duration * 60 + PlayInfo.durationSec, TimeStr);
    pTimeStr = TimeStr;
    if (PlayInfo.duration >= 60)
      TimeStr[strlen(TimeStr) - 3] = '\0';
    else
      pTimeStr = &TimeStr[2];
    FMUC_PutString(rgnInfo, 500-TimeWidth, 14, 500, pTimeStr, COLOR_White, COLOR_None, &Calibri_12_FontDataUC, FALSE, ALIGN_RIGHT);
  }

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void OSDInfoDrawPlayIcons(bool Force)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("OSDInfoDrawPlayIcons");
  #endif

  TYPE_GrData           *Btn_Play, *Btn_Pause, *Btn_Fwd, *Btn_Rwd, *Btn_Slow;
  static TYPE_TrickMode  LastTrickMode = TRICKMODE_Slow;
  static byte            LastTrickModeSwitch = 0;
  byte                   TrickModeSwitch;

  if(rgnInfo)
  {
    TrickModeSwitch = 0;
/*    switch(TrickMode)
    {
      case TRICKMODE_Normal:
      case TRICKMODE_Pause: break;

      case TRICKMODE_Forward:  TrickModeSwitch = 0x10; break;
      case TRICKMODE_Rewind:   TrickModeSwitch = 0x20; break;
      case TRICKMODE_Slow:     TrickModeSwitch = 0x30; break;
    }
    TrickModeSwitch += TrickModeSpeed; */
    TrickModeSwitch = (TrickMode << 4) + TrickModeSpeed;

    if(Force || (TrickMode != LastTrickMode) || (TrickModeSwitch != LastTrickModeSwitch))
    {
      Btn_Play  = &_Button_Play_Inactive_Gd;
      Btn_Pause = &_Button_Pause_Inactive_Gd;
      Btn_Fwd   = &_Button_Ffwd_Inactive_Gd;
      Btn_Rwd   = &_Button_Rwd_Inactive_Gd;
      Btn_Slow  = &_Button_Slow_Inactive_Gd;

      switch(TrickMode)
      {
        case TRICKMODE_Normal:   Btn_Play  = &_Button_Play_Active_Gd; break;
        case TRICKMODE_Forward:  Btn_Fwd   = &_Button_Ffwd_Active_Gd; break;
        case TRICKMODE_Rewind:   Btn_Rwd   = &_Button_Rwd_Active_Gd; break;
        case TRICKMODE_Slow:     Btn_Slow  = &_Button_Slow_Active_Gd; break;
        case TRICKMODE_Pause:    Btn_Pause = &_Button_Pause_Active_Gd; break;
      }

      TAP_Osd_PutGd(rgnInfo, 557, 8, Btn_Rwd, FALSE);
      TAP_Osd_PutGd(rgnInfo, 584, 8, Btn_Pause, FALSE);
      TAP_Osd_PutGd(rgnInfo, 611, 8, Btn_Slow, FALSE);
      TAP_Osd_PutGd(rgnInfo, 638, 8, Btn_Play, FALSE);
      TAP_Osd_PutGd(rgnInfo, 665, 8, Btn_Fwd, FALSE);

      LastTrickMode = TrickMode;

      TAP_Osd_FillBox(rgnInfo, 552, 26, 145, 19, RGB(51, 51, 51));
      switch(TrickModeSwitch)
      {
        case 0x11: FMUC_PutString(rgnInfo, 657, 26, 697, "2x",    COLOR_White, COLOR_None, &Calibri_12_FontDataUC, TRUE, ALIGN_CENTER); break;
        case 0x12: FMUC_PutString(rgnInfo, 657, 26, 697, "4x",    COLOR_White, COLOR_None, &Calibri_12_FontDataUC, TRUE, ALIGN_CENTER); break;
        case 0x13: FMUC_PutString(rgnInfo, 657, 26, 697, "8x",    COLOR_White, COLOR_None, &Calibri_12_FontDataUC, TRUE, ALIGN_CENTER); break;
        case 0x14: FMUC_PutString(rgnInfo, 657, 26, 697, "16x",   COLOR_White, COLOR_None, &Calibri_12_FontDataUC, TRUE, ALIGN_CENTER); break;
        case 0x15: FMUC_PutString(rgnInfo, 657, 26, 697, "32x",   COLOR_White, COLOR_None, &Calibri_12_FontDataUC, TRUE, ALIGN_CENTER); break;
        case 0x16: FMUC_PutString(rgnInfo, 657, 26, 697, "64x",   COLOR_White, COLOR_None, &Calibri_12_FontDataUC, TRUE, ALIGN_CENTER); break;
        case 0x21: FMUC_PutString(rgnInfo, 549, 26, 589, "2x",    COLOR_White, COLOR_None, &Calibri_12_FontDataUC, TRUE, ALIGN_CENTER); break;
        case 0x22: FMUC_PutString(rgnInfo, 549, 26, 589, "4x",    COLOR_White, COLOR_None, &Calibri_12_FontDataUC, TRUE, ALIGN_CENTER); break;
        case 0x23: FMUC_PutString(rgnInfo, 549, 26, 589, "8x",    COLOR_White, COLOR_None, &Calibri_12_FontDataUC, TRUE, ALIGN_CENTER); break;
        case 0x24: FMUC_PutString(rgnInfo, 549, 26, 589, "16x",   COLOR_White, COLOR_None, &Calibri_12_FontDataUC, TRUE, ALIGN_CENTER); break;
        case 0x25: FMUC_PutString(rgnInfo, 549, 26, 589, "32x",   COLOR_White, COLOR_None, &Calibri_12_FontDataUC, TRUE, ALIGN_CENTER); break;
        case 0x26: FMUC_PutString(rgnInfo, 549, 26, 589, "64x",   COLOR_White, COLOR_None, &Calibri_12_FontDataUC, TRUE, ALIGN_CENTER); break;
        case 0x31: FMUC_PutString(rgnInfo, 602, 26, 642, "1/2x",  COLOR_White, COLOR_None, &Calibri_12_FontDataUC, TRUE, ALIGN_CENTER); break;
        case 0x32: FMUC_PutString(rgnInfo, 602, 26, 642, "1/4x",  COLOR_White, COLOR_None, &Calibri_12_FontDataUC, TRUE, ALIGN_CENTER); break;
        case 0x33: FMUC_PutString(rgnInfo, 602, 26, 642, "1/8x",  COLOR_White, COLOR_None, &Calibri_12_FontDataUC, TRUE, ALIGN_CENTER); break;
        case 0x34: FMUC_PutString(rgnInfo, 602, 26, 642, "1/16x", COLOR_White, COLOR_None, &Calibri_12_FontDataUC, TRUE, ALIGN_CENTER); break;
      }
      LastTrickModeSwitch = TrickModeSwitch;

      TAP_Osd_Sync();
    }
  }

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void OSDInfoDrawClock(bool Force)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("OSDInfoDrawClock");
  #endif

  word                  mjd;
  byte                  hour, min, sec;
  static byte           LastMin = 99;
  char                  Time[8];

  if(rgnInfo)
  {
    TAP_GetTime(&mjd, &hour, &min, &sec);
    if((min != LastMin) || Force)
    {
      TAP_SPrint(Time, "%2.2d:%2.2d", hour, min);
      TAP_Osd_FillBox(rgnInfo, 638, 65, 60, 25, RGB(16, 16, 16));
      FMUC_PutString(rgnInfo, 638, 65, 710, Time, COLOR_White, COLOR_None, &Calibri_14_FontDataUC, TRUE, ALIGN_LEFT);
      LastMin = min;
      TAP_Osd_Sync();
    }
  }

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void OSDInfoDrawBookmarkMode(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("OSDInfoDrawBookmarkMode");
  #endif

  if(rgnInfo)
  {
    TAP_Osd_FillBox(rgnInfo, BookmarkMode_x, 68, 60, 19, RGB(51, 51, 51));
    FMUC_PutString(rgnInfo, BookmarkMode_x, 70, BookmarkMode_x+60, ((BookmarkMode ? LangGetString(LS_Bookmarks) : LangGetString(LS_Segments))), ((BookmarkMode) ? RGB(60,255,60) : RGB(250,139,18)), COLOR_None, &Calibri_10_FontDataUC, FALSE, ALIGN_CENTER);
    TAP_Osd_Sync();
  }

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void OSDInfoDrawMinuteJump(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("OSDInfoDrawMinuteJump");
  #endif

  char InfoStr[5];

  if(rgnInfo)
  {
    TAP_Osd_FillBox(rgnInfo, 507, 8, 50, 35, RGB(51, 51, 51));
    if(BookmarkMode || MinuteJump)
    {
      TAP_Osd_PutGd(rgnInfo, 507, 8, &_Button_SkipLeft_Gd, TRUE);
      TAP_Osd_PutGd(rgnInfo, 507 + 1 + _Button_SkipLeft_Gd.width, 8, &_Button_SkipRight_Gd, TRUE);

      if (MinuteJump)
        TAP_SPrint(InfoStr, "%u'", MinuteJump);
      else
        TAP_SPrint(InfoStr, "BM");
      FMUC_PutString(rgnInfo, 508, 26, 555, InfoStr, COLOR_White, COLOR_None, &Calibri_10_FontDataUC, TRUE, ALIGN_CENTER);
    }
    TAP_Osd_Sync();
  }

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}


// ----------------------------------------------------------------------------
//                            Playback-Funktionen
// ----------------------------------------------------------------------------
void Playback_Faster(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("Playback_Faster");
  #endif

  switch(TrickMode)
  {
    case TRICKMODE_Pause:
    {
      // 1/16xFWD
//      Playback_Slow();
      TrickModeSpeed = 3;
      Appl_SetPlaybackSpeed(TRICKMODE_Slow, TrickModeSpeed, TRUE);
      TrickMode = TRICKMODE_Slow;
      break;
    }

    case TRICKMODE_Rewind:
    {
      if(TrickModeSpeed > 2)
      {
        // 64xRWD down to 2xRWD
        TrickModeSpeed -= 2;
        Appl_SetPlaybackSpeed(TRICKMODE_Rewind, TrickModeSpeed, TRUE);
      }
      else
      {
        Playback_Normal();
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
        Appl_SetPlaybackSpeed(TRICKMODE_Slow, TrickModeSpeed, TRUE);
      }
      else
        Playback_Normal();
      break;
    }

    case TRICKMODE_Normal:
    {
      // 2xFWD
      Playback_FFWD();
      break;
    }

    case TRICKMODE_Forward:
    {
      // 2xFWD to 64xFWD
      Playback_FFWD();
      break;
    }
  }

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void Playback_Slower(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("Playback_Slower");
  #endif

  switch(TrickMode)
  {
    case TRICKMODE_Forward:
    {
      if(TrickModeSpeed > 2)
      {
        // 64xFWD down to 2xFWD
        TrickModeSpeed -= 2;
        Appl_SetPlaybackSpeed(TRICKMODE_Forward, TrickModeSpeed, TRUE);
      }
      else
      {
        // 1xFWD
        Playback_Normal();
      }
      break;
    }

    case TRICKMODE_Normal:
    {
      Playback_RWD();
      // 1/2xFWD
//      Playback_Slow();
      break;
    }

    case TRICKMODE_Slow:
    {
      // 1/2xFWD to 1/16xFWD
      if(TrickModeSpeed < 3)
      {
        TrickModeSpeed++;
        Appl_SetPlaybackSpeed(TRICKMODE_Slow, TrickModeSpeed, TRUE);
      }
      else
      {
        // 1xRWD
//        TrickModeSpeed = 0;
//        Playback_RWD();
      }
      break;
    }

    case TRICKMODE_Pause:
    {
      // 2xRWD
      Playback_RWD();
      break;
    }

    case TRICKMODE_Rewind:
    {
      // 2xRWD down to 64xRWD
      Playback_RWD();
      break;
    }
  }

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void Playback_Normal(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("Playback_Normal");
  #endif

  Appl_SetPlaybackSpeed(TRICKMODE_Normal, 1, TRUE);
  TrickMode = TRICKMODE_Normal;
  TrickModeSpeed = 1;

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void Playback_Pause(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("Playback_Pause");
  #endif

  Appl_SetPlaybackSpeed(TRICKMODE_Pause, 0, TRUE);
  TrickMode = TRICKMODE_Pause;
  TrickModeSpeed = 0;

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void Playback_FFWD(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("Playback_FFWD");
  #endif

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
    TrickModeSpeed++;
    if (TrickModeSpeed < 6) TrickModeSpeed++;
    Appl_SetPlaybackSpeed(TRICKMODE_Forward, TrickModeSpeed, TRUE);
    TrickMode = TRICKMODE_Forward;
  }
  else
    Playback_Normal();

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void Playback_RWD(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("Playback_RWD");
  #endif

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
    TrickModeSpeed++;
    if (TrickModeSpeed < 6) TrickModeSpeed++;
    Appl_SetPlaybackSpeed(TRICKMODE_Rewind, TrickModeSpeed, TRUE);
    TrickMode = TRICKMODE_Rewind;
  }
  else
    Playback_Normal();

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void Playback_Slow(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("Playback_Slow");
  #endif

  //Appl_SetPlaybackSpeed(3, 1, true) 1/2x Slow
  //Appl_SetPlaybackSpeed(3, 2, true) 1/4x Slow
  //Appl_SetPlaybackSpeed(3, 3, true) 1/8x Slow
  //Appl_SetPlaybackSpeed(3, 4, true) 1/16x Slow

  if (TrickMode != TRICKMODE_Slow)
    TrickModeSpeed = 1;
  else
    if(TrickModeSpeed < 3) TrickModeSpeed++;
    else TrickModeSpeed = 1;
  Appl_SetPlaybackSpeed(TRICKMODE_Slow, TrickModeSpeed, TRUE);
  TrickMode = TRICKMODE_Slow;

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void Playback_JumpForward(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("Playback_JumpForward");
  #endif

  dword                 JumpBlock;
  JumpBlock = min(PlayInfo.currentBlock + MinuteJumpBlocks, BlockNrLastSecond);

  if(TrickMode == TRICKMODE_Pause) Playback_Normal();
  TAP_Hdd_ChangePlaybackPos(JumpBlock);

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void Playback_JumpBackward(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("Playback_JumpBackward");
  #endif

  dword                 JumpBlock;
  JumpBlock = max(PlayInfo.currentBlock - MinuteJumpBlocks, 0);

  if(TrickMode == TRICKMODE_Pause) Playback_Normal();
  TAP_Hdd_ChangePlaybackPos(JumpBlock);

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void Playback_JumpNextSegment(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("Playback_JumpNextSegment");
  #endif

  if(NrSegmentMarker > 2)
  {
    if (ActiveSegment < NrSegmentMarker - 2)
      ActiveSegment++;
    else
      ActiveSegment = 0;
    if(TrickMode == TRICKMODE_Pause) Playback_Normal();
    TAP_Hdd_ChangePlaybackPos(SegmentMarker[ActiveSegment].Block);
    JumpRequestedTime = 0;
    JumpPerformedTime = TAP_GetTick();
    OSDSegmentListDrawList();
  }

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void Playback_JumpPrevSegment(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("Playback_JumpPrevSegment");
  #endif
  const dword FiveSeconds = PlayInfo.totalBlock * 5 / (60*PlayInfo.duration + PlayInfo.durationSec);

  if(NrSegmentMarker > 2)
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
    JumpRequestedTime = 0;
    JumpPerformedTime = TAP_GetTick();
    OSDSegmentListDrawList();
  }

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void Playback_JumpNextBookmark(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("Playback_JumpNextBookmark");
  #endif

  int i;

  if ((NrBookmarks > 0) && (PlayInfo.currentBlock > Bookmarks[NrBookmarks-1]))
  {
    if(TrickMode == TRICKMODE_Pause) Playback_Normal();
    TAP_Hdd_ChangePlaybackPos(Bookmarks[0]);

    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return;
  }
  
  for(i = 0; i < NrBookmarks; i++)
  {
    if(PlayInfo.currentBlock < Bookmarks[i])
    {
      if(TrickMode == TRICKMODE_Pause) Playback_Normal();
      TAP_Hdd_ChangePlaybackPos(Bookmarks[i]);

      #if STACKTRACE == TRUE
        CallTraceExit(NULL);
      #endif
      return;
    }
  }

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void Playback_JumpPrevBookmark(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("Playback_JumpPrevBookmark");
  #endif

  const dword           ThirtySeconds = PlayInfo.totalBlock * 30 / (60*PlayInfo.duration + PlayInfo.durationSec);
  int                   i;

  if ((NrBookmarks > 0) && (PlayInfo.currentBlock < Bookmarks[0]))
  {
    if(TrickMode == TRICKMODE_Pause) Playback_Normal();
    TAP_Hdd_ChangePlaybackPos(Bookmarks[NrBookmarks-1]);

    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return;
  }
  
  for(i = NrBookmarks - 1; i >= 0; i--)
  {
    if((Bookmarks[i] + ThirtySeconds) < PlayInfo.currentBlock)
    {
      if(TrickMode == TRICKMODE_Pause) Playback_Normal();
      TAP_Hdd_ChangePlaybackPos(Bookmarks[i]);

      #if STACKTRACE == TRUE
        CallTraceExit(NULL);
      #endif
      return;
    }
  }

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}


// ----------------------------------------------------------------------------
//                              Hilfsfunktionen
// ----------------------------------------------------------------------------
bool isPlaybackRunning(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("isPlaybackRunning");
  #endif

  TAP_Hdd_GetPlayInfo(&PlayInfo);
  if((int)PlayInfo.currentBlock < 0) PlayInfo.currentBlock = 0;

  if ((PlayInfo.playMode == PLAYMODE_Playing) && !NoPlaybackCheck)
  {
    TrickMode = (TYPE_TrickMode)PlayInfo.trickMode;
    TrickModeSpeed = PlayInfo.speed;
  }

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
  return ((PlayInfo.playMode == PLAYMODE_Playing) || NoPlaybackCheck);
}

void CalcLastSeconds(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("CalcLastSecond");
  #endif

  BlocksOneSecond      = PlayInfo.totalBlock / (60*PlayInfo.duration + PlayInfo.durationSec);
  BlockNrLastSecond    = PlayInfo.totalBlock - BlocksOneSecond;
  BlockNrLast10Seconds = PlayInfo.totalBlock - (10 * PlayInfo.totalBlock / (60*PlayInfo.duration + PlayInfo.durationSec));

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void CheckLastSeconds(void)
{
  static bool LastSecondsPaused = FALSE;

  #if STACKTRACE == TRUE
    CallTraceEnter("CheckLastSecond");
  #endif

  if((PlayInfo.currentBlock > BlockNrLastSecond) && (TrickMode != TRICKMODE_Pause))
  {
    if (!LastSecondsPaused) Playback_Pause();
    LastSecondsPaused = TRUE;
  }
  else if((PlayInfo.currentBlock > BlockNrLast10Seconds) && (TrickMode == TRICKMODE_Forward)) 
    Playback_Normal();
  else
    LastSecondsPaused = FALSE;

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}


// ----------------------------------------------------------------------------
//                          ActionMenu-Funktionen
// ----------------------------------------------------------------------------
void ActionMenuDraw(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("ActionMenuDraw");
  #endif

  dword                 C1, C2, C3, C4, DisplayColor;
  int                   x, y, i;
  static char           TempStr[128];
  char                 *DisplayStr;

  NrSelectedSegments = 0;
  for(i = 0; i < NrSegmentMarker - 1; i++)
  {
    if(SegmentMarker[i].Selected) NrSelectedSegments++;
  }

  if(!rgnActionMenu)
  {
    rgnActionMenu = TAP_Osd_Create(((720 - _ActionMenu9_Gd.width) >> 1) +25, 70, _ActionMenu9_Gd.width, _ActionMenu9_Gd.height, 0, 0);
    ActionMenuItem = 0;
  }

  TAP_Osd_PutGd(rgnActionMenu, 0, 0, &_ActionMenu9_Gd, FALSE);
  TAP_Osd_PutGd(rgnActionMenu, 7, 4 + 28 * ActionMenuItem, &_ActionMenu_Bar_Gd, FALSE);

  x = 20;
  y = MI_NrMenuItems * 28 + 6;
  TAP_Osd_PutGd(rgnActionMenu, x, y, &_Button_Down_Gd, TRUE);
  x += (_Button_Down_Gd.width + 5);

  TAP_Osd_PutGd(rgnActionMenu, x, y, &_Button_Up_Gd, TRUE);
  x += (_Button_Up_Gd.width + 5);

  TAP_Osd_PutGd(rgnActionMenu, x, y, &_Button_Ok_Gd, TRUE);
  x += (_Button_Ok_Gd.width + 5);

  TAP_Osd_PutGd(rgnActionMenu, x, y, &_Button_Exit_Gd, TRUE);

  C1 = COLOR_Black;
  C2 = COLOR_White;
  C3 = RGB(255, 100, 100);
  C4 = RGB(120, 120, 120);

  for(i = 0; i < MI_NrMenuItems; i++)
  {
    DisplayStr = NULL;
    DisplayColor = (ActionMenuItem == i ? C1 : C2);
    switch(i)
    {
      case MI_SelectFunction:     DisplayStr = LangGetString(LS_SelectFunction); break;
      case MI_SaveSegment:
      {
        if (NrSelectedSegments == 0)
        {
          DisplayStr = LangGetString(LS_SaveCurSegment);
          if (NrSegmentMarker <= 2) DisplayColor = C4;
        }
        else if (NrSelectedSegments == 1)
          DisplayStr = LangGetString(LS_Save1Segment);
        else
        {
          TAP_SPrint(TempStr, LangGetString(LS_SaveNrSegments), NrSelectedSegments);
          DisplayStr = TempStr;
        }
        break;
      }
      case MI_DeleteSegment:
      {
        if (NrSelectedSegments == 0)
        {
          DisplayStr = LangGetString(LS_DeleteCurSegment);
          if (NrSegmentMarker <= 2) DisplayColor = C4;
        }
        else if (NrSelectedSegments == 1)
          DisplayStr = LangGetString(LS_Delete1Segment);
        else
        {
          TAP_SPrint(TempStr, LangGetString(LS_DeleteNrSegments), NrSelectedSegments);
          DisplayStr = TempStr;
        }
        break;
      }
      case MI_SelectOddSegments:
      {
        DisplayStr = (NrSegmentMarker == 4) ? LangGetString(LS_SelectPadding) : LangGetString(LS_SelectOddSegments);
        if (NrSegmentMarker <= 2) DisplayColor = C4;
        break;
      }
      case MI_SelectEvenSegments:
      {
        DisplayStr = (NrSegmentMarker == 4) ? LangGetString(LS_SelectMiddle) : LangGetString(LS_SelectEvenSegments);
        if (NrSegmentMarker <= 2) DisplayColor = C4;
        break;
      }
      case MI_ClearAll:
      {
        if (NrSelectedSegments > 0)
          DisplayStr = LangGetString(LS_UnselectAll);
        else
        {
          if (BookmarkMode)
          {
            DisplayStr = LangGetString(LS_DeleteAllBookmarks);
            if (NrBookmarks <= 0) DisplayColor = C4;
          }
          else
          {
            DisplayStr = LangGetString(LS_ClearSegmentList);
            if (NrSegmentMarker <= 2) DisplayColor = C4;
          }
        }
        break;
      }
      case MI_DeleteFile:         DisplayStr = LangGetString(LS_DeleteFile); DisplayColor = C3; break;
      case MI_ImportBookmarks:
      {
        if (BookmarkMode)
        {
          DisplayStr = LangGetString(LS_ExportToBM);
          if (NrSegmentMarker <= 2) DisplayColor = C4;
        }
        else
        {
          DisplayStr = LangGetString(LS_ImportBM);
          if (NrBookmarks <= 0) DisplayColor = C4;
        }
        break;
      }
      case MI_ExitMC:             DisplayStr = LangGetString(LS_ExitMC); break;
      case MI_NrMenuItems:        break;
    }
    if (DisplayStr && (i < MI_NrMenuItems))
      FMUC_PutString(rgnActionMenu, 18, 4 + 28 * i, 300, DisplayStr, DisplayColor, COLOR_None, &Calibri_14_FontDataUC, TRUE, (i==0) ? ALIGN_CENTER : ALIGN_LEFT);
  }

  TAP_Osd_Sync();

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void ActionMenuDown(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("ActionMenuDown");
  #endif

  do
  {
    if(ActionMenuItem >= (MI_NrMenuItems - 1))
      ActionMenuItem = 1;
    else
      ActionMenuItem++;
  } while ((ActionMenuItem<=4 && NrSegmentMarker<=2) || (ActionMenuItem==MI_ClearAll && !(NrSelectedSegments>0 || (BookmarkMode && NrBookmarks>0) || (!BookmarkMode && NrSegmentMarker>2))) || (ActionMenuItem==MI_ImportBookmarks && !((BookmarkMode && NrSegmentMarker>2) || (!BookmarkMode && NrBookmarks>0))));

  ActionMenuDraw();

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void ActionMenuUp(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("ActionMenuUp");
  #endif

  do
  {
    if(ActionMenuItem > 1)
      ActionMenuItem--;
    else
      ActionMenuItem = MI_NrMenuItems - 1;
  } while ((ActionMenuItem<=4 && NrSegmentMarker<=2) || (ActionMenuItem==MI_ClearAll && !(NrSelectedSegments>0 || (BookmarkMode && NrBookmarks>0) || (!BookmarkMode && NrSegmentMarker>2))) || (ActionMenuItem==MI_ImportBookmarks && !((BookmarkMode && NrSegmentMarker>2) || (!BookmarkMode && NrBookmarks>0))));

  ActionMenuDraw();

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void ActionMenuRemove(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("ActionMenuRemove");
  #endif

  TAP_Osd_Delete(rgnActionMenu);
  rgnActionMenu = 0;
  OSDRedrawEverything();
//  TAP_Osd_Sync();

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}


// ----------------------------------------------------------------------------
//                        MovieCutter-Funktionalität
// ----------------------------------------------------------------------------
void MovieCutterSaveSegments(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("MovieCutterSaveSegments");
  #endif

  if (NrSegmentMarker > 2)
    MovieCutterProcess(TRUE);

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void MovieCutterDeleteSegments(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("MovieCutterDeleteSegments");
  #endif

  if (NrSegmentMarker > 2)
    MovieCutterProcess(FALSE);

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void MovieCutterSelectOddSegments(void)
{
  if (NrSegmentMarker <= 2) return;
  #if STACKTRACE == TRUE
    CallTraceEnter("MovieCutterSelectOddSegments");
  #endif

  int i;

  for(i = 0; i < NrSegmentMarker-1; i++)
    SegmentMarker[i].Selected = ((i & 1) == 0);

  OSDSegmentListDrawList();
  OSDInfoDrawProgressbar(TRUE);
//  OSDRedrawEverything();
//  MovieCutterProcess(TRUE, FALSE);

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void MovieCutterSelectEvenSegments(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("MovieCutterSelectEvenSegments");
  #endif

  int i;

  for(i = 0; i < NrSegmentMarker-1; i++)
    SegmentMarker[i].Selected = ((i & 1) == 1);

  OSDSegmentListDrawList();
  OSDInfoDrawProgressbar(TRUE);
//  OSDRedrawEverything();
//  MovieCutterProcess(TRUE, FALSE);

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void MovieCutterUnselectAll(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("MovieCutterUnselectAll");
  #endif

  int i;

  for(i = 0; i < NrSegmentMarker-1; i++)
    SegmentMarker[i].Selected = FALSE;

  OSDSegmentListDrawList();
  OSDInfoDrawProgressbar(TRUE);
//  OSDRedrawEverything();
//  MovieCutterProcess(TRUE, FALSE);

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void MovieCutterDeleteFile(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("MovieCutterDeleteFile");
  #endif

  NoPlaybackCheck = TRUE;
  HDD_ChangeDir(PlaybackDir);
  TAP_Hdd_StopTs();
  CutFileDelete();
  HDD_Delete(PlaybackName);
  NoPlaybackCheck = FALSE;

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void MovieCutterProcess(bool KeepCut)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("MovieCutterProcess");
  #endif

//  int                   NrSelectedSegments;
  bool                  isMultiSelect, CutEnding;
  word                  WorkingSegment;
  char                  CutFileName[MAX_FILE_NAME_SIZE + 1];
  char                  TempFileName[MAX_FILE_NAME_SIZE + 1], TempString[MAX_FILE_NAME_SIZE + 1];
  tTimeStamp            CutStartPoint, BehindCutPoint;
  dword                 DeltaTime, DeltaBlock;
  int                   i, j;
  tResultCode           ret;

  NoPlaybackCheck = TRUE;
  HDD_ChangeDir(PlaybackDir);

  // *CW* FRAGE: Werden die Bookmarks von der Firmware sowieso vor dem Schneiden in die inf gespeichert?
  // -> sonst könnte man der Schnittroutine auch das Bookmark-Array übergeben
  SaveBookmarksToInf();
  CutDumpList();

  // Lege ein Backup der .cut-Datei an
  CutFileSave();
  char BackupCutName[MAX_FILE_NAME_SIZE + 1];
  strcpy(BackupCutName, PlaybackName);
  BackupCutName[strlen(BackupCutName) - 4] = '\0';
  TAP_SPrint(LogString, "cp \"%s/%s.cut\" \"%s/%s.cut.bak\"", AbsPlaybackDir, BackupCutName, AbsPlaybackDir, BackupCutName);
  system(LogString);

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

  // NEU: Wenn Segmente zum Löschen ausgewählt sind, aber das Ende nicht gelöscht werden soll -> versuche dieses durch Löschen vor Beschädigung zu schützen
/*  if (!KeepCut && !SegmentMarker[NrSegmentMarker - 2].Selected)
  {
    if (AddSegmentMarker(PlayInfo.totalBlock - 3, FALSE))
    {
      SegmentMarker[NrSegmentMarker - 2].Selected = TRUE;
      NrSelectedSegments++;
      if (!isMultiSelect)
      {
        SegmentMarker[ActiveSegment].Selected = TRUE;
        isMultiSelect = TRUE;
      }
      WriteLogMC(PROGRAM_NAME, "Inserting new segment marker, to crop the ending -> prevents disruption of ending!");
      CutDumpList();
    }
  } */

//  ClearOSD(FALSE);

  for(i = NrSegmentMarker - 2; i >= 0 /*-1*/; i--)
  {
    //OSDMenuProgressBarShow(PROGRAM_NAME, LangGetString(LS_Cutting), NrSegmentMarker - i - 1, NrSegmentMarker, NULL);

    if(isMultiSelect)
    {
      //If one or more segments have been selected, work with them.
      WorkingSegment = i;

      // Process the ending at last (*experimental!*)
/*      if (i == NrSegmentMarker - 2) {
        NrSelectedSegments--;
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
      NoPlaybackCheck = FALSE;
      if (isPlaybackRunning())
        TAP_Hdd_StopTs();
      NoPlaybackCheck = TRUE;
      TAP_SPrint(LogString, "Processing segment %u", WorkingSegment);
      WriteLogMC(PROGRAM_NAME, LogString);

      // Ermittlung der Schnittpositionen
      CutStartPoint.BlockNr = SegmentMarker[WorkingSegment].Block;
      CutStartPoint.Timems = SegmentMarker[WorkingSegment].Timems;
      BehindCutPoint.BlockNr = SegmentMarker[WorkingSegment + 1].Block;
      BehindCutPoint.Timems = SegmentMarker[WorkingSegment + 1].Timems;

      // NEUE Spezial-Behandlung beim Schneiden des Endes
      CutEnding = FALSE;
      if (WorkingSegment == NrSegmentMarker - 2)
      {
        if (KeepCut)
          //letztes Segment soll gespeichert werden -> versuche bis zum tatsächlichen Ende zu gehen
          BehindCutPoint.BlockNr = 0xFFFFFFFF;
        else
        {
          //letztes Segment soll geschnitten werden -> speichere stattdessen den vorderen Teil der Aufnahme und tausche mit dem Original
          CutEnding = TRUE;
          CutStartPoint.BlockNr  = SegmentMarker[0].Block;
          CutStartPoint.Timems   = SegmentMarker[0].Timems;
          BehindCutPoint.BlockNr = SegmentMarker[NrSegmentMarker-2].Block;
          BehindCutPoint.Timems  = SegmentMarker[NrSegmentMarker-2].Timems;
          WriteLogMC(PROGRAM_NAME, "(* special mode for cut ending *)");
        }
      }

      // Ermittlung des Dateinamens für das CutFile
      GetNextFreeCutName(PlaybackName, CutFileName, NrSelectedSegments - 1);
      if (CutEnding)
      {
        strncpy(TempString, PlaybackName, strlen(PlaybackName) - 4);
        TAP_SPrint(TempFileName, "%s_temp.rec", TempString);
        HDD_Delete(TempFileName);
      }

      // Schnittoperation
      ret = MovieCutter(PlaybackName, ((CutEnding) ? TempFileName : CutFileName), &CutStartPoint, &BehindCutPoint, (KeepCut || CutEnding), HDVideo);

      // Das erzeugte CutFile wird zum neuen SourceFile
      if (CutEnding)
      {
        if (KeepCut)
        {
          TAP_SPrint(LogString, "Renaming original file '%s' to '%s'", PlaybackName, CutFileName);
          WriteLogMC(PROGRAM_NAME, LogString);
          HDD_Rename(PlaybackName, CutFileName);
        }
        else 
          HDD_Delete(PlaybackName);
        TAP_SPrint(LogString, "Renaming cutfile '%s' to '%s'", TempFileName, PlaybackName);
        WriteLogMC(PROGRAM_NAME, LogString);
        HDD_Rename(TempFileName, PlaybackName);
      }

      // Wiedergabe wird neu gestartet
      TAP_Hdd_PlayTs(PlaybackName);
      do
      {
        isPlaybackRunning();
      } while ((int)PlayInfo.totalBlock == -1);
      TAP_SPrint(LogString, "Reported new totalBlock = %u", PlayInfo.totalBlock);
      WriteLogMC(PROGRAM_NAME, LogString);

      //Bail out if the cut failed
      if(ret == RC_Error)
      {
        ShowErrorMessage(LangGetString(LS_CutHasFailed));
        State = ST_UnacceptedFile;
        break;
      }

      // Anpassung der verbleibenden Segmente
      SegmentMarker[WorkingSegment].Selected = FALSE;
      DeleteSegmentMarker(WorkingSegment);  // das 0. Segment darf nicht gelöscht werden
      NrSelectedSegments--;

      if (CutEnding) {
        DeltaBlock = CutStartPoint.BlockNr;
        DeltaTime = CutStartPoint.Timems;
      } else {
        DeltaBlock = BehindCutPoint.BlockNr - CutStartPoint.BlockNr;
        DeltaTime = BehindCutPoint.Timems - CutStartPoint.Timems;
      }

      for(j = NrSegmentMarker - 1; j >= 1; j--)
      {
        if(SegmentMarker[j].Block > CutStartPoint.BlockNr + 1)
        {
          SegmentMarker[j].Block -= min(DeltaBlock, SegmentMarker[j].Block);
          SegmentMarker[j].Timems -= min(DeltaTime, SegmentMarker[j].Timems);
        }

        // das letzte Segment auf den gemeldeten TotalBlock-Wert setzen
        if((j == NrSegmentMarker - 1) && (SegmentMarker[j].Block != PlayInfo.totalBlock))
        {
          #ifdef FULLDEBUG
            TAP_SPrint(LogString, "MovieCutterProcess: Letzter Segment-Marker %u ist ungleich TotalBlock %u!", SegmentMarker[NrSegmentMarker - 1].Block, PlayInfo.totalBlock);
            WriteLogMC(PROGRAM_NAME, LogString);
          #endif
          SegmentMarker[j].Block = PlayInfo.totalBlock;
          dword newTime = NavGetBlockTimeStamp(SegmentMarker[j].Block);
          SegmentMarker[j].Timems = (newTime) ? newTime : (dword)(1000 * (60*PlayInfo.duration + PlayInfo.durationSec));
        }
        SegmentMarker[j].Percent = (float)SegmentMarker[j].Block * 100 / SegmentMarker[NrSegmentMarker - 1].Block;

        //If the first marker has moved to block 0, delete it
        if(SegmentMarker[j].Block <= 1) DeleteSegmentMarker(j);  // Annahme: ermittelte DeltaBlocks weichen nur um höchstens 1 ab
      }
    
      // Wenn Spezial-Crop-Modus, nochmal testen, ob auch mit der richtigen rec weitergemacht wird
      if(CutEnding)
      {
        if((ret==RC_Warning) || !TAP_Hdd_Exist(PlaybackName) || TAP_Hdd_Exist(TempFileName))
        {
          WriteLogMC(PROGRAM_NAME, "Error processing the last segment: Renaming or nav-generation failed!");
          ShowErrorMessage(LangGetString(LS_CutHasFailed));
          State = ST_UnacceptedFile;
          break;
        }
      }
      OSDSegmentListDrawList();
      OSDInfoDrawProgressbar(TRUE);
    }
    if ((NrSelectedSegments <= 0) /* && !SegmentMarker[NrSegmentMarker-2].Selected*/ )
      break;
  }
  CutFileSave();
  CutDumpList();
/*  if(TimeStamps)
  {
    TAP_MemFree(TimeStamps);
    TimeStamps = NULL;
  }
  TimeStamps = NavLoad(PlaybackName, &NrTimeStamps, HDVideo);
  LastTimeStamp = &TimeStamps[0];
  ReadBookmarks();
  CalcLastSeconds();
  LastTotalBlocks = PlayInfo.totalBlock;

  //OSDMenuProgressBarShow(PROGRAM_NAME, LangGetString(LS_Cutting), 1, 1, NULL);
  //OSDMenuProgressBarDestroy();
  Playback_Normal(); */

  LastTotalBlocks = PlayInfo.totalBlock;
  TAP_Hdd_ChangePlaybackPos(SegmentMarker[min(ActiveSegment, NrSegmentMarker-2)].Block);

//  TAP_Osd_Sync();
//  ClearOSD(FALSE);

//  NrBookmarks = 0;
//  OSDRedrawEverything();

  if (State != ST_UnacceptedFile)
    State = ST_WaitForPlayback;
  NoPlaybackCheck = FALSE;

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}


// ----------------------------------------------------------------------------
//                           System-Funktionen
// ----------------------------------------------------------------------------
// Sets the Playback Repeat Mode: 0=NoRepeat, 1=RepeatFromTo, 2=RepeatAll.
// If RepeatMode != 1, the other parameters are without function.
// Returns the old value if success, 0xFF if failure
byte PlaybackRepeatMode(bool ChangeMode, byte RepeatMode, dword RepeatStartBlock, dword RepeatEndBlock)
{
  static byte        *_RepeatMode = NULL;
  static int         *_RepeatStart = NULL;
  static int         *_RepeatEnd = NULL;
  byte               OldValue;

  if(_RepeatMode == NULL)
  {
    _RepeatMode = (byte*)TryResolve("_playbackRepeatMode");
    if(_RepeatMode == NULL) return 0xFF;
  }

  if(_RepeatStart == NULL)
  {
    _RepeatStart = (int*)TryResolve("_playbackRepeatRegionStart");
    if(_RepeatStart == NULL) return 0xFF;
  }

  if(_RepeatEnd == NULL)
  {
    _RepeatEnd = (int*)TryResolve("_playbackRepeatRegionEnd");
    if(_RepeatEnd == NULL) return 0xFF;
  }

  OldValue = *_RepeatMode;
  if (ChangeMode)
  {
    *_RepeatMode = RepeatMode;
    *_RepeatStart = (RepeatMode == 1) ? (int)RepeatStartBlock : -1;
    *_RepeatEnd = (RepeatMode == 1) ? (int)RepeatEndBlock : -1;
  }
  return OldValue;
}
bool PlaybackRepeatSet(bool EnableRepeatAll)
{
  return (PlaybackRepeatMode(TRUE, ((EnableRepeatAll) ? 2 : 0), 0, 0) != 0xFF);
}
bool PlaybackRepeatGet()
{
  return (PlaybackRepeatMode(FALSE, 0, 0, 0) == 2);
}


// ----------------------------------------------------------------------------
//                            NAV-Funktionen
// ----------------------------------------------------------------------------
dword NavGetBlockTimeStamp(dword PlaybackBlockNr)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("NavGetBlockTimeStamp");
  #endif

  if(TimeStamps == NULL)
  {
    WriteLogMC(PROGRAM_NAME, "Someone is trying to get a timestamp while the array is empty!");

    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
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
    if(LastTimeStamp->BlockNr > PlaybackBlockNr) {
      #if STACKTRACE == TRUE
        CallTraceExit(NULL);
      #endif
      return 0;
    }
  }

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
  return LastTimeStamp->Timems;
}

// ----------------------------------------------------------------------------
//                           Compatibility-Layer
// ----------------------------------------------------------------------------
// Die Implementierung berücksichtigt (aus Versehen) auch negative Sprünge und Überlauf.
// SOLLTE eine nav-Datei einen Überlauf beinhalten, wird dieser durch PatchOldNavFile korrigiert.
bool PatchOldNavFileSD(char *SourceFileName)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("PatchOldNavFileSD");
  #endif

  FILE                 *fSourceNav;
  TYPE_File            *fNewNav;
  char                  FileName[MAX_FILE_NAME_SIZE + 1];
  char                  BakFileName[MAX_FILE_NAME_SIZE + 1];
  tnavSD               *navRecs;
  size_t                navsRead, i;
  char                  AbsFileName[512];

  WriteLogMC(PROGRAM_NAME, "Checking source nav file (possibly older version with incorrect Times)...");

  HDD_ChangeDir(PlaybackDir);
  TAP_SPrint(FileName, "%s.nav", SourceFileName);
  TAP_SPrint(BakFileName, "%s.nav.bak", SourceFileName);

  //If nav already patched -> exit function
  if (TAP_Hdd_Exist(BakFileName))
  {
    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return FALSE;
  }

  //Rename the original nav file to bak
  TAP_Hdd_Rename(FileName, BakFileName);

  //Open the original nav
  TAP_SPrint(AbsFileName, "%s%s/%s.nav.bak", TAPFSROOT, PlaybackDir, SourceFileName);
  fSourceNav = fopen(AbsFileName, "rb");
  if(!fSourceNav)
  {
    WriteLogMC(PROGRAM_NAME, "PatchOldNavFile() E0d01.");

    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return FALSE;
  }

  //Create and open the new source nav
  TAP_SPrint(FileName, "%s.nav", SourceFileName);
  TAP_Hdd_Delete(FileName);
  TAP_Hdd_Create(FileName, ATTR_NORMAL);
  fNewNav = TAP_Hdd_Fopen(FileName);
  if(!fNewNav)
  {
    fclose(fSourceNav);
    WriteLogMC(PROGRAM_NAME, "PatchOldNavFile() E0d02.");

    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return FALSE;
  }

  //Loop through the nav
  dword Difference = 0;
  dword LastTime = 0;
  bool FirstRun = TRUE;

  navRecs = (tnavSD*) TAP_MemAlloc(NAVRECS_SD * sizeof(tnavSD));
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
      if (navRecs[i].Timems - LastTime > 1000)
      {
        Difference += (navRecs[i].Timems - LastTime) - 1000;

        TAP_SPrint(LogString, "  - Gap found at nav record nr. %u:  Offset=%llu, TimeStamp(before)=%u, TimeStamp(after)=%u, GapSize=%u", ftell(fSourceNav)/sizeof(tnavSD) - navsRead + i, ((off_t)(navRecs[i].PHOffsetHigh) << 32) | navRecs[i].PHOffset, navRecs[i].Timems, navRecs[i].Timems-Difference, navRecs[i].Timems-LastTime);
        WriteLogMC(PROGRAM_NAME, LogString);
      }
      LastTime = navRecs[i].Timems;
      navRecs[i].Timems -= Difference;
    }
    TAP_Hdd_Fwrite(navRecs, sizeof(tnavSD), navsRead, fNewNav);
  }

  fclose(fSourceNav);
  TAP_Hdd_Fclose(fNewNav);
  TAP_MemFree(navRecs);

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
  return (Difference > 0);
}

bool PatchOldNavFileHD(char *SourceFileName)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("PatchOldNavFileHD");
  #endif

  FILE                 *fSourceNav;
  TYPE_File            *fNewNav;
  char                  FileName[MAX_FILE_NAME_SIZE + 1];
  char                  BakFileName[MAX_FILE_NAME_SIZE + 1];
  tnavHD               *navRecs;
  size_t                navsRead, i;
  char                  AbsFileName[512];

  WriteLogMC(PROGRAM_NAME, "Checking source nav file (possibly older version with incorrect Times)...");

  HDD_ChangeDir(PlaybackDir);
  TAP_SPrint(FileName, "%s.nav", SourceFileName);
  TAP_SPrint(BakFileName, "%s.nav.bak", SourceFileName);

  //If nav already patched -> exit function
  if (TAP_Hdd_Exist(BakFileName))
  {
    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return FALSE;
  }

  //Rename the original nav file to bak
  TAP_Hdd_Rename(FileName, BakFileName);

  //Open the original nav
  TAP_SPrint(AbsFileName, "%s%s/%s.nav.bak", TAPFSROOT, PlaybackDir, SourceFileName);
  fSourceNav = fopen(AbsFileName, "rb");
  if(!fSourceNav)
  {
    WriteLogMC(PROGRAM_NAME, "PatchOldNavFile() E0e01.");

    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return FALSE;
  }

  //Create and open the new source nav
  TAP_SPrint(FileName, "%s.nav", SourceFileName);
  TAP_Hdd_Delete(FileName);
  TAP_Hdd_Create(FileName, ATTR_NORMAL);
  fNewNav = TAP_Hdd_Fopen(FileName);
  if(!fNewNav)
  {
    fclose(fSourceNav);
    WriteLogMC(PROGRAM_NAME, "PatchOldNavFile() E0e02.");

    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return FALSE;
  }

  //Loop through the nav
  dword Difference = 0;
  dword LastTime = 0;
  bool FirstRun = TRUE;

  navRecs = (tnavHD*) TAP_MemAlloc(NAVRECS_HD * sizeof(tnavHD));
  while(TRUE)
  {
    navsRead = fread(navRecs, sizeof(tnavHD), NAVRECS_HD, fSourceNav);
    if(navsRead == 0) break;

    // Versuche, nav-Dateien aus Timeshift-Aufnahmen zu unterstützen ***experimentell***
    if(FirstRun)
    {
      FirstRun = FALSE;
      if(navRecs[0].LastPPS == 0x72767062)  // 'bpvr'
      {
        fseek(fSourceNav, 1056, SEEK_SET);
        continue;
      }
    }

    for(i = 0; i < navsRead; i++)
    {
      if (navRecs[i].Timems - LastTime > 1000)
      {
        Difference += (navRecs[i].Timems - LastTime) - 1000;

        TAP_SPrint(LogString, "  - Gap found at nav record nr. %u:  Offset=%llu, TimeStamp(before)=%u, TimeStamp(after)=%u, GapSize=%u", ftell(fSourceNav)/sizeof(tnavHD) - navsRead + i, ((off_t)(navRecs[i].SEIOffsetHigh) << 32) | navRecs[i].SEIOffsetLow, navRecs[i].Timems, navRecs[i].Timems-Difference, navRecs[i].Timems-LastTime);
        WriteLogMC(PROGRAM_NAME, LogString);
      }
      LastTime = navRecs[i].Timems;
      navRecs[i].Timems -= Difference;
    }
    TAP_Hdd_Fwrite(navRecs, sizeof(tnavHD), navsRead, fNewNav);
  }

  fclose(fSourceNav);
  TAP_Hdd_Fclose(fNewNav);
  TAP_MemFree(navRecs);

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
  return (Difference > 0);
}

bool PatchOldNavFile(char *SourceFileName, bool isHD)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("PatchOldNavFile");
  #endif

  bool ret;

  if(isHD)
    ret = PatchOldNavFileHD(SourceFileName);
  else
    ret = PatchOldNavFileSD(SourceFileName);

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
  return ret;
}
