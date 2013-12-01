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
#include                "Graphics/Selection_Red.gd"
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
#include                "Graphics/Button_Left.gd"
#include                "Graphics/Button_Right.gd"
#include                "Graphics/Button_Up.gd"
#include                "Graphics/Button_Up2.gd"
#include                "Graphics/Button_Down.gd"
#include                "Graphics/Button_Down2.gd"
#include                "Graphics/Button_Red.gd"
#include                "Graphics/Button_Green.gd"
#include                "Graphics/Button_Yellow.gd"
#include                "Graphics/Button_Blue.gd"
#include                "Graphics/Button_White.gd"
#include                "Graphics/Button_Ok.gd"
#include                "Graphics/Button_Exit.gd"
#include                "Graphics/Info_Background.gd"
#include                "Graphics/Info_Progressbar.gd"
#include                "Graphics/BookmarkMarker.gd"
#include                "Graphics/SegmentMarker.gd"
#include                "Graphics/ActionMenu9.gd"
#include                "Graphics/ActionMenu_Bar.gd"
#include                "TMSCommander.h"

TAP_ID                  (TAPID);
TAP_PROGRAM_NAME        (PROGRAM_NAME" "VERSION);
TAP_PROGRAM_VERSION     (VERSION);
TAP_AUTHOR_NAME         (AUTHOR);
TAP_DESCRIPTION         (DESCRIPTION);
TAP_ETCINFO             (__DATE__);

typedef struct
{
  dword                 Block;  //Block nr
  dword                 Timems; //Time in ms
  float                 Percent;
  bool                  Selected;
} tSegmentMarker;

typedef enum
{
  ST_Init,               // TAP start (executed only once)
  ST_IdleNoPlayback,     // TAP inactive, no file is being played back
  ST_Idle,               // OSD is active (playback running)
  ST_IdleInvisible,      // OSD is currently not active, but can be entered (playback running)
  ST_IdleUnacceptedFile, // TAP is not active and cannot be entered (playback of unsupported file)
  ST_ActionDialog,       // Action Menu is open, navigation only within menu
  ST_CutFailDialog,      // Failure Dialog is open, no other actions possible
  ST_Exit                // Preparing to exit TAP
} tState;

typedef enum
{
  TM_Play,
  TM_Pause,
  TM_Fwd,
  TM_Rwd,
  TM_Slow
} tTrickMode;

typedef enum
{
  MI_SelectFunction,
  MI_SaveSegment,
  MI_DeleteSegment,
  MI_SelectOddSegments,
  MI_SelectEvenSegments,
  MI_UnselectAll,
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
  LS_Pause,
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
  LS_Segments,
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
  LS_PageStr,
  LS_AskConfirmation,
  LS_Yes,
  LS_No,
  LS_ListIsFull,
  LS_NoRecSize,
  LS_HDDetectionFailed,
  LS_NavLoadFailed,
  LS_WrongNavLength,
  LS_NrStrings
} tLngStrings;


// MovieCutter state variables
tState                  State = ST_Init;
bool                    AutoOSDPolicy = TRUE;
bool                    BookmarkMode;
tTrickMode              TrickMode;
dword                   TrickModeSpeed;
dword                   MinuteJump;                           //Seconds or 0 if deactivated
dword                   MinuteJumpBlocks;                     //Number of blocks, which shall be added
bool                    NoPlaybackCheck;                      //Used to circumvent a race condition during the cutting process

// Playback information
TYPE_PlayInfo           PlayInfo;
char                    PlaybackName[MAX_FILE_NAME_SIZE + 1];
char                    PlaybackDirectory[512];
__off64_t               RecFileSize;
dword                   LastTotalBlocks = 0;
dword                   BlockNrLastSecond;
dword                   BlockNrLast10Seconds;

// Video parameters
word                    NrSegmentMarker;
word                    ActiveSegment;
tSegmentMarker          SegmentMarker[NRSEGMENTMARKER];       //[0]=Start of file, [x]=End of file
word                    NrBookmarks;
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

char                    LogString[512];


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

  FMUC_LoadFontFile("Calibri_10.ufnt", &Calibri_10_FontDataUC);
  FMUC_LoadFontFile("Calibri_12.ufnt", &Calibri_12_FontDataUC);
  FMUC_LoadFontFile("Calibri_14.ufnt", &Calibri_14_FontDataUC);

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

dword TAP_EventHandler(word event, dword param1, dword param2)
{
  dword                 SysState, SysSubState;
  static bool           DoNotReenter = FALSE;
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

  if(OSDMenuMessageBoxIsVisible())
  {
    OSDMenuEvent(&event, &param1, &param2);
    if(event == EVT_KEY && param1 == RKEY_Ok) OSDMenuMessageBoxDestroy();

/*    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return 0;
*/  }

  TAP_GetState(&SysState, &SysSubState);

  switch(State)
  {
    case ST_Init:             //Executed once upon TAP start
    {
      CleanupCut();
/*
      LastTotalBlocks = 0;
      NrTimeStamps = 0;
      TimeStamps = NULL;
      LastTimeStamp = NULL;
*/
      State = AutoOSDPolicy ? ST_IdleNoPlayback : ST_IdleInvisible;
      break;
    }

    case ST_IdleNoPlayback:   //Idle loop while there is no playback active
    {
      if(isPlaybackRunning())
      {
        char *p;

        if((int)PlayInfo.totalBlock > 0)
        {
          if(SysState != STATE_Normal) break;
          LastTotalBlocks = PlayInfo.totalBlock;  // *CW*

          //"Calculate" the file name (.rec or .mpg)
          if(!PlayInfo.file || !PlayInfo.file->name[0]) PlaybackName[0] = '\0';
          strcpy(PlaybackName, PlayInfo.file->name);
          PlaybackName[strlen(PlaybackName) - 4] = '\0';

          //Extract the absolute path to the rec file and change into that dir
          HDD_GetAbsolutePathByTypeFile(PlayInfo.file, PlaybackDirectory);
          p = strstr(PlaybackDirectory, PlaybackName);
          if(p) *p = '\0';
//          TAP_Hdd_ChangeDir(&PlaybackDirectory[strlen(TAPFSROOT)]);
          HDD_ChangeDir(&PlaybackDirectory[strlen(TAPFSROOT)]);

          WriteLogMC(PROGRAM_NAME, "========================================\n");
          TAP_SPrint(LogString, "Attaching to %s%s", PlaybackDirectory, PlaybackName);
          WriteLogMC(PROGRAM_NAME, LogString);
          WriteLogMC("MovieCutterLib", "----------------------------------------");

          // Detect size of rec file
//          RecFileSize = HDD_GetFileSize(PlaybackName);
//          if(RecFileSize <= 0)
          if(!HDD_GetFileSizeAndInode(&PlaybackDirectory[strlen(TAPFSROOT)], PlaybackName, NULL, &RecFileSize))
          {
            WriteLogMC(PROGRAM_NAME, ".rec size could not be detected");
            OSDMenuMessageBoxInitialize(PROGRAM_NAME, LangGetString(LS_NoRecSize));
            OSDMenuMessageBoxButtonAdd(LangGetString(LS_OK));
            OSDMenuMessageBoxShow();
            State = ST_IdleUnacceptedFile;
            break;
          }

          TAP_SPrint(LogString, "File size = %llu Bytes (%u blocks)", RecFileSize, (dword)(RecFileSize / BLOCKSIZE));
          WriteLogMC(PROGRAM_NAME, LogString);
          TAP_SPrint(LogString, "Reported total blocks: %u", PlayInfo.totalBlock);
          WriteLogMC(PROGRAM_NAME, LogString);

          //Check if a nav is available
          if(!isNavAvailable(PlaybackName))
          {
            WriteLogMC(PROGRAM_NAME, ".nav is missing");
            OSDMenuMessageBoxInitialize(PROGRAM_NAME, LangGetString(LS_MissingNav));
            OSDMenuMessageBoxButtonAdd(LangGetString(LS_OK));
            OSDMenuMessageBoxShow();
            State = ST_IdleUnacceptedFile;
            break;
          }

          // Detect if video stream is in HD
          if (!isHDVideo(PlaybackName, &HDVideo))
          {
            WriteLogMC(PROGRAM_NAME, "could not detect type of video stream.");
            OSDMenuMessageBoxInitialize(PROGRAM_NAME, LangGetString(LS_HDDetectionFailed));
            OSDMenuMessageBoxButtonAdd(LangGetString(LS_OK));
            OSDMenuMessageBoxShow();
            State = ST_IdleUnacceptedFile;
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
#ifdef FULLDEBUG
  TAP_PrintNet("Size of used types:\nlong: %d, int: %d, short: %d, char: %d, byte: %d, word: %d, dword: %d, bool: %d, float: %d\n", sizeof(long), sizeof(int), sizeof(short), sizeof(char), sizeof(byte), sizeof(word), sizeof(dword), sizeof(bool), sizeof(float));
#endif
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
            WriteLogMC(PROGRAM_NAME, "error loading the .nav file");
            OSDMenuMessageBoxInitialize(PROGRAM_NAME, LangGetString(LS_NavLoadFailed));
            OSDMenuMessageBoxButtonAdd(LangGetString(LS_OK));
            OSDMenuMessageBoxShow();
            State = ST_IdleUnacceptedFile;
            break;
          }
          LastTimeStamp = &TimeStamps[0];

          // Check if nav has correct length! ***CW***
          if(labs(TimeStamps[NrTimeStamps-1].Timems - (1000 * (60*PlayInfo.duration + PlayInfo.durationSec))) > 3000)
          {
            WriteLogMC(PROGRAM_NAME, ".nav file length not matching duration");

            // [COMPATIBILITY LAYER - fill holes in old nav file]
            if (PatchOldNavFile(PlaybackName, HDVideo))
              OSDMenuMessageBoxInitialize(PROGRAM_NAME, ".nav duration mismatch. Patched!\nPlease try again.");
            else
              OSDMenuMessageBoxInitialize(PROGRAM_NAME, LangGetString(LS_WrongNavLength));
            OSDMenuMessageBoxButtonAdd(LangGetString(LS_OK));
            OSDMenuMessageBoxShow();
            State = ST_IdleUnacceptedFile;
            break;
          }

          //Check if it is crypted
          if(isCrypted(PlaybackName))
          {
            WriteLogMC(PROGRAM_NAME, "file is crypted");
            OSDMenuMessageBoxInitialize(PROGRAM_NAME, LangGetString(LS_IsCrypted));
            OSDMenuMessageBoxButtonAdd(LangGetString(LS_OK));
            OSDMenuMessageBoxShow();
            State = ST_IdleUnacceptedFile;
            break;
          }
          
          BookmarkMode = FALSE;
          NrSegmentMarker = 0;
          ActiveSegment = 0;
          MinuteJump = 0;
//          MinuteJumpBlocks = 0;  // nicht unbedingt nötig
          NoPlaybackCheck = FALSE;
          CreateOSD();
          Playback_Normal();
          CalcLastSeconds();
          ReadBookmarks();
          if(!CutFileLoad()) AddDefaultSegmentMarker();
          OSDRedrawEverything();
//          LastTotalBlocks = PlayInfo.totalBlock;  // *CW*
          State = ST_Idle;
        }
      }
      break;
    }

    case ST_IdleInvisible:    //Playback is active but OSD is hidden
    {
      // if playback stopped
      if (!isPlaybackRunning()) {
        Cleanup(FALSE);
        if(AutoOSDPolicy) State = ST_IdleNoPlayback;
        break;
      }

      // if progress-bar is enabled and cut-key is pressed -> show MovieCutter OSD (ST_IdleNoPlayback)
      if(/*SysSubState == SUBSTATE_PvrPlayingSearch &&*/ (event == EVT_KEY) && ((param1 == RKEY_Ab) || (param1 == RKEY_Option)))
      {
        if (LastTotalBlocks != PlayInfo.totalBlock)
          State = ST_IdleNoPlayback;
        else {
          // beim erneuten Einblenden kann man sich das Neu-Berechnen aller Werte sparen (AUCH wenn 2 Aufnahmen gleiche Blockzahl haben!!)
          BookmarkMode = FALSE;
          MinuteJump = 0;
//          MinuteJumpBlocks = 0;
//          NoPlaybackCheck = FALSE;
          CreateOSD();
//          Playback_Normal();
          ReadBookmarks();
          OSDRedrawEverything();
          State = ST_Idle;
        }
        param1 = 0;
      }

      // if playback-file changed -> show MovieCutter as soon as next playback is started (ST_IdleNoPlayback)
      if(AutoOSDPolicy && (LastTotalBlocks != PlayInfo.totalBlock)) State = ST_IdleNoPlayback;
      break;
    }

    case ST_Idle:             //Playback is active and OSD is visible
    {
      if(!isPlaybackRunning())
      {
        CutFileSave();
        Cleanup(TRUE);
        State = AutoOSDPolicy ? ST_IdleNoPlayback : ST_IdleInvisible;
        break;
      }

      if(SysSubState == SUBSTATE_Normal) TAP_ExitNormal();

      if(event == EVT_KEY)
      {
        switch(param1)
        {
          case RKEY_Ab:
          case RKEY_Option:
          {
            BookmarkMode = !BookmarkMode;
            OSDRedrawEverything();
            break;
          }
        
          case RKEY_Record:
          {
            break;
          }

          case RKEY_Exit:
          {
            CutFileSave();
            ClearOSD();
            State = ST_IdleInvisible;

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
              if (ActiveSegment < (NrSegmentMarker - 2))
                ActiveSegment++;
              else
                ActiveSegment = 0;
              OSDSegmentListDrawList();
              if(TrickMode == TM_Pause) Playback_Normal();
              TAP_Hdd_ChangePlaybackPos(SegmentMarker[ActiveSegment].Block);
            }
            break;
          }

          case RKEY_Up:
          {
            if(NrSegmentMarker > 2)
            {
              if (ActiveSegment > 0)
                ActiveSegment--;
              else
                ActiveSegment = NrSegmentMarker - 2;
              OSDSegmentListDrawList();
              if(TrickMode == TM_Pause) Playback_Normal();
              TAP_Hdd_ChangePlaybackPos(SegmentMarker[ActiveSegment].Block);
            }
            break;
          }

          case RKEY_Red:
          {
            if (BookmarkMode) {  
              int NearestBookmarkIndex = FindNearestBookmark();
              if(NearestBookmarkIndex != -1)
              {
                DeleteBookmark(NearestBookmarkIndex);
                OSDInfoDrawProgressbar(TRUE);
//                OSDRedrawEverything();
              }
            } else {
              int NearestMarkerIndex = FindNearestSegmentMarker();
              if(NearestMarkerIndex != -1)
              {
                DeleteSegmentMarker(NearestMarkerIndex);
                OSDSegmentListDrawList();
                OSDInfoDrawProgressbar(TRUE);
//                OSDRedrawEverything();
              }
            } 
            break;
          }

          case RKEY_Green:
          {
            if (BookmarkMode) {
              if(AddBookmark(PlayInfo.currentBlock)) OSDInfoDrawProgressbar(TRUE); // OSDRedrawEverything();
            } else {
              if(AddSegmentMarker(PlayInfo.currentBlock)) {OSDSegmentListDrawList(); OSDInfoDrawProgressbar(TRUE);}; // OSDRedrawEverything();
            }
            break;
          }

          case RKEY_Yellow:
          {
            if (BookmarkMode) {
              MoveBookmark(PlayInfo.currentBlock);
            } else {
              MoveSegmentMarker();
            }
            OSDSegmentListDrawList();
            OSDInfoDrawProgressbar(TRUE);
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

          case RKEY_Ok:
          {
            if(TrickMode != TM_Pause)
            {
              Playback_Pause();
            }
            else
            {
              ActionMenuDraw();
              State = ST_ActionDialog;
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
            if(MinuteJump)
              Playback_JumpForward();
            else if(BookmarkMode)
              Playback_JumpNextBookmark();
            else
              Playback_Faster();
            break;
          }

          case RKEY_Left:
          {
            if(MinuteJump)
              Playback_JumpBackward();
            else if(BookmarkMode)
              Playback_JumpPrevBookmark();
            else
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

    case ST_IdleUnacceptedFile: //Playback is active but MC can't use that file and is inactive
    {
      if(!isPlaybackRunning() || LastTotalBlocks != PlayInfo.totalBlock)
      { 
        Cleanup(TRUE);
        State = AutoOSDPolicy ? ST_IdleNoPlayback : ST_IdleInvisible;
      }
      break;
    }

    case ST_ActionDialog:     //Action dialog is visible
    {
      if(event == EVT_KEY)
      {
        switch(param1)
        {
          case RKEY_Ok:
          {
            if(ActionMenuItem != MI_SelectFunction)
            {
              if (ActionMenuItem==MI_SaveSegment || ActionMenuItem==MI_DeleteSegment || ActionMenuItem==MI_DeleteFile)
                if (!GetUserConfirmation())
                  break;

              ActionMenuRemove();
              State = ST_Idle;
              switch(ActionMenuItem)
              {
                case MI_SaveSegment:        MovieCutterSaveSegments(); break;
                case MI_DeleteSegment:      MovieCutterDeleteSegments(); break;
                case MI_SelectOddSegments:  MovieCutterSelectOddSegments();  ActionMenuDraw(); State = ST_ActionDialog; break;
                case MI_SelectEvenSegments: MovieCutterSelectEvenSegments(); ActionMenuDraw(); State = ST_ActionDialog; break;
                case MI_UnselectAll:        MovieCutterUnselectAll(); break;
                case MI_DeleteFile:         MovieCutterDeleteFile(); break;
                case MI_ImportBookmarks:    AddBookmarksToSegmentList(); break;
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

          case RKEY_Exit:
          {
            ActionMenuRemove();
            State = ST_Idle;
            break;
          }

        param1 = 0;
        }
      }
      break;
    }

    case ST_CutFailDialog:    //Cut fail dialog is visible
    {
      OSDMenuMessageBoxInitialize(PROGRAM_NAME, LangGetString(LS_CutHasFailed));
      OSDMenuMessageBoxButtonAdd(LangGetString(LS_OK));
      OSDMenuMessageBoxShow();
      NoPlaybackCheck = FALSE;
      State = ST_IdleUnacceptedFile;
      break;
    }

    case ST_Exit:             //Preparing to terminate the TAP
    {
      if (strlen(PlaybackName) > 0)
        CutFileSave();
      Cleanup(TRUE);
      LangUnloadStrings();
      FMUC_FreeFontFile(&Calibri_10_FontDataUC);
      FMUC_FreeFontFile(&Calibri_12_FontDataUC);
      FMUC_FreeFontFile(&Calibri_14_FontDataUC);
      #if STACKTRACE == TRUE
        CallTraceExit(NULL);
      #endif
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

bool GetUserConfirmation(void)
{
  word event; dword param1, param2;
return TRUE;
  OSDMenuMessageBoxInitialize(PROGRAM_NAME, LangGetString(LS_AskConfirmation));
  OSDMenuMessageBoxButtonAdd(LangGetString(LS_No));
  OSDMenuMessageBoxButtonAdd(LangGetString(LS_Yes));
  OSDMenuMessageBoxShow();
  do
  {
    OSDMenuEvent(&event, &param1, &param2);
    if((event == EVT_KEY) && ((param1 == RKEY_Left) || (param1 == RKEY_Right)))
      OSDMenuMessageBoxButtonSelect((OSDMenuMessageBoxLastButton() + 1) % 2);
    if((event == EVT_KEY) && (param1 == RKEY_Ok)) 
      OSDMenuMessageBoxDestroy();
  } while(OSDMenuMessageBoxIsVisible());
  return (OSDMenuMessageBoxLastButton() == 1);
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
      if(State == ST_IdleInvisible) State = ST_IdleNoPlayback;

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

void ClearOSD(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("ClearOSD");
  #endif

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
  TAP_EnterNormal();

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
  if(TimeStamps) {
    TAP_MemFree(TimeStamps);
    TimeStamps = NULL;
  }
//  lastTimeStamp = NULL;
//  NrTimeStamps = 0;
  if (DoClearOSD) ClearOSD();

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
  HDD_TAP_PopDir();

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

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


//Segment marker functions
void AddBookmarksToSegmentList(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("AddBookmarksToSegmentList");
  #endif

  word i;

  // first, delete all present segment markers (*CW*)
  DeleteAllSegmentMarkers();

  // second, add a segment marker for each bookmark
  TAP_SPrint(LogString, "Importing %u of %u bookmarks", min(NrBookmarks, NRSEGMENTMARKER-2), NrBookmarks);
  WriteLogMC(PROGRAM_NAME, LogString);

  for(i = 0; i < min(NrBookmarks, NRSEGMENTMARKER-2); i++)
  { 
    TAP_SPrint(LogString, "Bookmark %u @ %u", i + 1, Bookmarks[i]);
    WriteLogMC(PROGRAM_NAME, LogString);
    AddSegmentMarker(Bookmarks[i]);
  }

  OSDRedrawEverything();

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void AddDefaultSegmentMarker(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("AddDefaultSegmentMarker");
  #endif

  AddSegmentMarker(0);
  AddSegmentMarker(PlayInfo.totalBlock);

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

bool AddSegmentMarker(dword Block)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("AddSegmentMarker");
  #endif

  word                  i, j;
  dword                 newTime;
  char                  StartTime[16];

  if(NrSegmentMarker >= NRSEGMENTMARKER)
  {
    WriteLogMC(PROGRAM_NAME, "AddSegmentMarker: SegmentMarker list is full");
    OSDMenuMessageBoxInitialize(PROGRAM_NAME, LangGetString(LS_ListIsFull));
    OSDMenuMessageBoxButtonAdd(LangGetString(LS_OK));
    OSDMenuMessageBoxShow();

    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return FALSE;
  }

  if (Block == 0)
    newTime = 0;
  else
  {
    newTime = NavGetBlockTimeStamp(Block);
    if(newTime == 0)
    {
      #if STACKTRACE == TRUE
        CallTraceExit(NULL);
      #endif
      return FALSE;
    }
  }

#ifdef FULLDEBUG
  printf("----------- vorher ---------------\n");
  printf(" PlayInfo.currentBlock: %u,  PlayInfo.totalBlock: %u\n", PlayInfo.currentBlock, PlayInfo.totalBlock);
  char TimeStr[16];
  MSecToTimeString(TimeStamps[0].Timems, TimeStr);
  printf(" erster  TimeStamp[    0]:  Block=%8u,  Time= %s\n", TimeStamps[0].BlockNr, TimeStr);
  MSecToTimeString(TimeStamps[NrTimeStamps-1].Timems, TimeStr);
  printf(" letzter TimeStamp[%5u]:  Block=%8u,  Time= %s\n", NrTimeStamps-1, TimeStamps[NrTimeStamps-1].BlockNr, TimeStr);
  printf(" Anzahl SegmentMarker: %u\n", NrSegmentMarker);
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
  printf(" Anzahl SegmentMarker: %u\n", NrSegmentMarker);
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

void MoveSegmentMarker(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("MoveSegmentMarker");
  #endif

  dword newTime;
  dword newBlock = PlayInfo.currentBlock;
  int NearestMarkerIndex = FindNearestSegmentMarker();

  if(NearestMarkerIndex != -1)
  {
    newTime = NavGetBlockTimeStamp(newBlock);
    if((PlayInfo.currentBlock > 0) && (newTime == 0))
    {
      #if STACKTRACE == TRUE
        CallTraceExit(NULL);
      #endif
      return;
    }

    SegmentMarker[NearestMarkerIndex].Block = newBlock;
    SegmentMarker[NearestMarkerIndex].Timems = newTime;
    SegmentMarker[NearestMarkerIndex].Percent = (float)PlayInfo.currentBlock * 100 / PlayInfo.totalBlock;
  }

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void DeleteSegmentMarker(word MarkerIndex)
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
    return;
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

void SetCurrentSegment(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("SetCurrentSegment");
  #endif

  int                   i;

  if(NrSegmentMarker < 3)
  {
    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return;
  }

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

void SelectSegmentMarker(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("SelectSegmentMarker");
  #endif

  SegmentMarker[ActiveSegment].Selected = !SegmentMarker[ActiveSegment].Selected;

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}


//Bookmarks werden jetzt aus dem inf-Cache der Firmware aus dem Speicher ausgelesen.
//Das geschieht bei jedem Einblenden des MC-OSDs, da sie sonst nicht benötigt werden
void ReadBookmarks(void)
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
  }

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

//Experimentelle Methode, um Bookmarks direkt in der Firmware abzuspeichern.
void SaveBookmarks(void)
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
}

void SaveBookmarksToInf(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("SaveBookmarksToInf");
  #endif

  char                  InfFileName[MAX_FILE_NAME_SIZE + 1];
  tRECHeaderInfo        RECHeaderInfo;
  byte                 *Buffer;
  TYPE_File            *fInf;
  dword                 FileSize, BufferSize;
  word                  i;

#ifdef FULLDEBUG
  TAP_PrintNet("SaveBookmarksToInf()\n");
  for (i = 0; i < NrBookmarks; i++) {
    TAP_PrintNet("%u\n", Bookmarks[i]);
  }
#endif

  //Read inf
  TAP_SPrint(InfFileName, "%s.inf", PlaybackName);
  fInf = TAP_Hdd_Fopen(InfFileName);
  if(!fInf)
  {
    WriteLogMC(PROGRAM_NAME, "SaveBookmarksToInf: failed to open the inf file");

    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return;
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
}

bool AddBookmark(dword Block)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("AddBookmark");
  #endif

  word i, j;

  if(NrBookmarks >= NRBOOKMARKS)
  {
    WriteLogMC(PROGRAM_NAME, "AddBookmark: Bookmark list is full");

    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return FALSE;
  }

  //Find the point where to insert the new marker so that the list stays sorted
  if (Block > Bookmarks[NrBookmarks - 1])
  {
    Bookmarks[NrBookmarks] = Block;
  }
  else
  {
    for(i = 0; i < NrBookmarks; i++)
    {
      if(Bookmarks[i] > Block)
      {
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
  word                  i;

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

void MoveBookmark(dword Block)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("MoveBookmark");
  #endif

  int NearestBookmarkIndex = FindNearestBookmark();

  if(NearestBookmarkIndex != -1)
    Bookmarks[NearestBookmarkIndex] = Block;
  SaveBookmarks();

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void DeleteBookmark(word BookmarkIndex)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("DeleteBookmark");
  #endif

  word i;

  for(i = BookmarkIndex; i < NrBookmarks - 1; i++)
    Bookmarks[i] = Bookmarks[i + 1];
  Bookmarks[NrBookmarks - 1] = 0;

  NrBookmarks--;
  SaveBookmarks();

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}


//Cut file functions
bool CutFileLoad(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("CutFileLoad");
  #endif

  char                  Name[MAX_FILE_NAME_SIZE + 1];
  word                  Version = 0;
  word                  Padding;
  TYPE_File            *fCut;
  __off64_t             FileSize;

  // Create name of cut-file
  strcpy(Name, PlaybackName);
  Name[strlen(Name) - 4] = '\0';
  strcat(Name, ".cut");

  // Check, if cut-File exists
  if(!TAP_Hdd_Exist(Name))
  {
    WriteLogMC(PROGRAM_NAME, "CutFileLoad: .cut doesn't exist");

    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return FALSE;
  }

  // Try to open cut-File
  fCut = TAP_Hdd_Fopen(Name);
  if(!fCut)
  {
    WriteLogMC(PROGRAM_NAME, "CutFileLoad: failed to open .cut");

    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return FALSE;
  }

  // Check correct version of cut-file
  TAP_Hdd_Fread(&Version, sizeof(byte), 1, fCut);    // read only one byte for compatibility with V.1  [COMPATIBILITY LAYER]
  if(Version > CUTFILEVERSION)
  {
    WriteLogMC(PROGRAM_NAME, "CutFileLoad: .cut version mismatch");
    TAP_Hdd_Fclose(fCut);

    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return FALSE;
  }
  if (Version > 1)
    TAP_Hdd_Fread(&Padding, sizeof(byte), 1, fCut);  // read the second byte of Version (if not V.1)  [COMPATIBILITY LAYER]

  // Check, if size of rec-File has been changed
  TAP_Hdd_Fread(&FileSize, sizeof(__off64_t), 1, fCut);
  if(RecFileSize != FileSize)
  {
    WriteLogMC(PROGRAM_NAME, "CutFileLoad: .cut file size mismatch");
    TAP_Hdd_Fclose(fCut);

    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return FALSE;
  }

  TAP_Hdd_Fread(&NrSegmentMarker, sizeof(word), 1, fCut);
  if (NrSegmentMarker > NRSEGMENTMARKER) NrSegmentMarker = NRSEGMENTMARKER;
  if (Version == 1)
    TAP_Hdd_Fread(&Padding, sizeof(word), 1, fCut);  // read the second word of NrSegmentMarker (if V.1)  [COMPATIBILITY LAYER]
  TAP_Hdd_Fread(&ActiveSegment, sizeof(word), 1, fCut);
  TAP_Hdd_Fread(&Padding, sizeof(word), 1, fCut);
  TAP_Hdd_Fread(SegmentMarker, sizeof(tSegmentMarker), NrSegmentMarker, fCut);
  TAP_Hdd_Fclose(fCut);
 
  if (NrSegmentMarker < 2) {
    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    NrSegmentMarker = 0;
    return FALSE;
  }

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
  SegmentMarker[NrSegmentMarker - 1].Selected = FALSE;        // the very last marker (no segment)
  if(NrSegmentMarker < 3) SegmentMarker[0].Selected = FALSE;  // there is only one segment (from start to end)
  if(ActiveSegment >= NrSegmentMarker - 1) ActiveSegment = NrSegmentMarker - 2;

  // Wenn cut-File Version 1 hat, dann ermittle neue TimeStamps und vergleiche diese mit den alten (DEBUG)  [COMPATIBILITY LAYER]
  if (Version == 1) {
    word i;
    dword oldTime, newTime;
    char oldTimeStr[12], newTimeStr[16];

    WriteLogMC(PROGRAM_NAME, "Import of old cut file version (compare old and new TimeStamps!)");
    for (i = 0; i < NrSegmentMarker; i++) {
      oldTime = SegmentMarker[i].Timems;
      newTime = NavGetBlockTimeStamp(SegmentMarker[i].Block);
      SegmentMarker[i].Timems = newTime;

      SecToTimeString(oldTime, oldTimeStr);
      MSecToTimeString(newTime, newTimeStr);
      TAP_SPrint(LogString, " %s%2u.)  BlockNr=%8u   oldTimeStamp=%s   newTimeStamp=%s", (labs(oldTime*1000-newTime) > 1000) ? "!!" : "  ", i, SegmentMarker[i].Block, oldTimeStr, newTimeStr);
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

  if(!HDD_GetFileSizeAndInode(&PlaybackDirectory[strlen(TAPFSROOT)], PlaybackName, NULL, &RecFileSize))
  {
    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return;
  }

  strcpy(Name, PlaybackName);
  Name[strlen(Name) - 4] = '\0';
  strcat(Name, ".cut");

#ifdef FULLDEBUG
  char CurDir[512];
  HDD_TAP_GetCurrentDir(CurDir);
  TAP_SPrint(LogString, "CutFileSave()! CurrentDir: %s, PlaybackName: %s, CutFileName: %s", CurDir, PlaybackName, Name);
  WriteLogMC(PROGRAM_NAME, LogString);
#endif

  Version = CUTFILEVERSION;
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
  word                   i;

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


//OSD functions
void CreateOSD(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("CreateOSD");
  #endif

  if(!rgnSegmentList)
  {
    TAP_ExitNormal();
    TAP_EnterNormal();
    TAP_ExitNormal();
    rgnSegmentList = TAP_Osd_Create(28, 85, _SegmentList_Background_Gd.width, _SegmentList_Background_Gd.height, 0, 0);
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
  OSDInfoDrawMinuteJump();

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void OSDSegmentListDrawList(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("OSDSegmentListDrawList");
  #endif

  word                  i, p;
  char                  StartTime[12], EndTime[12], TimeStr[28];
  char                  PageStr[15];
  dword                 fw;
  dword                 C1, C2;

  C1 = COLOR_Yellow;
  C2 = COLOR_White;

  if(rgnSegmentList)
  {
    TAP_Osd_PutGd(rgnSegmentList, 0, 0, &_SegmentList_Background_Gd, FALSE);
    FMUC_PutString(rgnSegmentList, 5, 3, 80, LangGetString(LS_Segments), COLOR_White, COLOR_None, &Calibri_12_FontDataUC, TRUE, ALIGN_LEFT);
    TAP_Osd_PutGd(rgnSegmentList, 62, 23, &_Button_Up_Gd, TRUE);
    TAP_Osd_PutGd(rgnSegmentList, 88, 23, &_Button_Down_Gd, TRUE);

    if(NrSegmentMarker > 2)
    {
      if(ActiveSegment >= NrSegmentMarker-1)           ActiveSegment = NrSegmentMarker - 2;
      if(ActiveSegment >= 10)                          TAP_Osd_PutGd(rgnSegmentList, 62, 23, &_Button_Up2_Gd, TRUE);
      if((ActiveSegment < 10) && (NrSegmentMarker>11)) TAP_Osd_PutGd(rgnSegmentList, 88, 23, &_Button_Down2_Gd, TRUE);

      TAP_SPrint(PageStr, "%s%u/%u", LangGetString(LS_PageStr), (ActiveSegment/10)+1, ((NrSegmentMarker-2)/10)+1);
      FMUC_PutString(rgnSegmentList, 60, 3, 114, PageStr, COLOR_White, COLOR_None, &Calibri_12_FontDataUC, FALSE, ALIGN_RIGHT);

      p = (ActiveSegment / 10) * 10;
      for(i = 0; i < min(10, (NrSegmentMarker - p) - 1); i++)
      {
        if((p+i) == ActiveSegment)
        {
          if((SegmentMarker[p+i + 1].Timems - SegmentMarker[p+i].Timems) < 60001)
            TAP_Osd_PutGd(rgnSegmentList, 3, 44 + 28*i, &_Selection_Red_Gd, FALSE);
          else
            TAP_Osd_PutGd(rgnSegmentList, 3, 44 + 28*i, &_Selection_Blue_Gd, FALSE);
        }

        SecToTimeString((SegmentMarker[p+i].Timems) / 1000, StartTime);
        SecToTimeString((SegmentMarker[p+i + 1].Timems) / 1000, EndTime);
        TAP_SPrint(TimeStr, "%2d. %s-%s", (p+i + 1), StartTime, EndTime);
        fw = FMUC_GetStringWidth(TimeStr, &Calibri_10_FontDataUC);            // 250
        FMUC_PutString(rgnSegmentList, max(0, 58 - (int)(fw >> 1)), 45 + 28*i, 116, TimeStr, (SegmentMarker[p+i].Selected ? C1 : C2), COLOR_None, &Calibri_10_FontDataUC, FALSE, ALIGN_LEFT);
      }
    }
  }

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void OSDInfoDrawBackground(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("OSDInfoDrawBackground");
  #endif

  int                   x;
  char                  s[50];

  if(rgnInfo)
  {
    TAP_Osd_PutGd(rgnInfo, 0, 0, &_Info_Background_Gd, FALSE);

    x = 350;
    TAP_Osd_PutGd(rgnInfo, x, 48, &_Button_Red_Gd, TRUE);
    x += _Button_Red_Gd.width;
    TAP_SPrint(s, LangGetString(LS_Delete));
    FMUC_PutString(rgnInfo, x, 47, 720, s, COLOR_White, COLOR_None, &Calibri_12_FontDataUC, TRUE, ALIGN_LEFT);
    x += FMUC_GetStringWidth(s, &Calibri_12_FontDataUC);

    TAP_Osd_PutGd(rgnInfo, x, 48, &_Button_Green_Gd, TRUE);
    x += _Button_Green_Gd.width;
    TAP_SPrint(s, LangGetString(LS_Add));
    FMUC_PutString(rgnInfo, x, 47, 720, s, COLOR_White, COLOR_None, &Calibri_12_FontDataUC, TRUE, ALIGN_LEFT);
    x += FMUC_GetStringWidth(s, &Calibri_12_FontDataUC);

    TAP_Osd_PutGd(rgnInfo, x, 48, &_Button_Yellow_Gd, TRUE);
    x += _Button_Yellow_Gd.width;
    TAP_SPrint(s, LangGetString(LS_Move));
    FMUC_PutString(rgnInfo, x, 47, 720, s, COLOR_White, COLOR_None, &Calibri_12_FontDataUC, TRUE, ALIGN_LEFT);
    x += FMUC_GetStringWidth(s, &Calibri_12_FontDataUC);

    TAP_Osd_PutGd(rgnInfo, x, 48, &_Button_Blue_Gd, TRUE);
    x += _Button_Blue_Gd.width;
    TAP_SPrint(s, LangGetString(LS_Select));
    FMUC_PutString(rgnInfo, x, 47, 720, s, COLOR_White, COLOR_None, &Calibri_12_FontDataUC, TRUE, ALIGN_LEFT);

    x = 350;
    TAP_Osd_PutGd(rgnInfo, x, 68, &_Button_Ok_Gd, TRUE);
    x += _Button_Ok_Gd.width;
    TAP_SPrint(s, LangGetString(LS_Pause));
    FMUC_PutString(rgnInfo, x, 67, 720, s, COLOR_White, COLOR_None, &Calibri_12_FontDataUC, TRUE, ALIGN_LEFT);
    x += FMUC_GetStringWidth(s, &Calibri_12_FontDataUC);

    TAP_Osd_PutGd(rgnInfo, x, 68, &_Button_Exit_Gd, TRUE);
    x += _Button_Exit_Gd.width;
    TAP_SPrint(s, LangGetString(LS_Exit));
    FMUC_PutString(rgnInfo, x, 67, 720, s, COLOR_White, COLOR_None, &Calibri_12_FontDataUC, TRUE, ALIGN_LEFT);
    x += FMUC_GetStringWidth(s, &Calibri_12_FontDataUC);
    
    if (BookmarkMode)
      TAP_SPrint(s, "Bookmarks");
    else
      TAP_SPrint(s, "Segments");
    FMUC_PutString(rgnInfo, x, 67, 720, s, COLOR_White, COLOR_None, &Calibri_10_FontDataUC, TRUE, ALIGN_LEFT);
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

  char                  Name[MAX_FILE_NAME_SIZE + 1];

  if(rgnInfo)
  {
    strcpy(Name, PlaybackName);
    Name[strlen(Name) - 4] = '\0';
    FMUC_PutString(rgnInfo, 65, 11, 490, Name, COLOR_White, COLOR_None, &Calibri_14_FontDataUC, TRUE, ALIGN_LEFT);
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

  TYPE_GrData          *Btn_Play, *Btn_Pause, *Btn_Fwd, *Btn_Rwd, *Btn_Slow;
  static tTrickMode     LastTrickMode = TM_Slow;
  static byte           LastTrickModeSwitch = 0;
  byte                  TrickModeSwitch;

  if(rgnInfo)
  {
    TrickModeSwitch = 0;
    switch(TrickMode)
    {
      case TM_Play:
      case TM_Pause: break;

      case TM_Fwd:  TrickModeSwitch = 0x10; break;
      case TM_Rwd:  TrickModeSwitch = 0x20; break;
      case TM_Slow: TrickModeSwitch = 0x30; break;
    }
    TrickModeSwitch += TrickModeSpeed;

    if(Force || (TrickMode != LastTrickMode) || (TrickModeSwitch != LastTrickModeSwitch))
    {
      Btn_Play  = &_Button_Play_Inactive_Gd;
      Btn_Pause = &_Button_Pause_Inactive_Gd;
      Btn_Fwd   = &_Button_Ffwd_Inactive_Gd;
      Btn_Rwd   = &_Button_Rwd_Inactive_Gd;
      Btn_Slow  = &_Button_Slow_Inactive_Gd;

      switch(TrickMode)
      {
        case TM_Play:  Btn_Play  = &_Button_Play_Active_Gd; break;
        case TM_Fwd:   Btn_Fwd   = &_Button_Ffwd_Active_Gd; break;
        case TM_Rwd:   Btn_Rwd   = &_Button_Rwd_Active_Gd; break;
        case TM_Slow:  Btn_Slow  = &_Button_Slow_Active_Gd; break;
        case TM_Pause: Btn_Pause = &_Button_Pause_Active_Gd; break;
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

        x1 = 34 + (int)((float)653 * SegmentMarker[ActiveSegment].Percent / 100);
        x2 = 34 + (int)((float)653 * SegmentMarker[ActiveSegment + 1].Percent / 100);

        if((SegmentMarker[ActiveSegment + 1].Timems - SegmentMarker[ActiveSegment].Timems) < 60001)
          TAP_Osd_FillBox(rgnInfo, x1, 102, x2 - x1, 10, RGB(238, 63, 63));
        else
          TAP_Osd_FillBox(rgnInfo, x1, 102, x2 - x1, 10, RGB(73, 206, 239));

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
          if(totalBlock)
            pos = (dword)((float)SegmentMarker[i].Block * 653 / totalBlock);
          else
            pos = 0;
          TAP_Osd_PutGd(rgnInfo, 31 + pos, 93, &_SegmentMarker_Gd, TRUE);
        }

        //Bookmarks: 0% = 31/112, 100% = 683/112
        for(i = 0; i < NrBookmarks; i++)
        {
          if(totalBlock)
            pos = (dword)((float)Bookmarks[i] * 653 / totalBlock);
          else
            pos = 0;
          TAP_Osd_PutGd(rgnInfo, 31 + pos, 112, &_BookmarkMarker_Gd, TRUE);
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

void OSDInfoDrawMinuteJump(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("OSDInfoDrawMinuteJump");
  #endif

  char                  Time[5];

  if(rgnInfo)
  {
    TAP_Osd_FillBox(rgnInfo, 507, 8, 50, 35, RGB(51, 51, 51));
    if(MinuteJump)
    {
      TAP_Osd_PutGd(rgnInfo, 507, 8, &_Button_Left_Gd, TRUE);
      TAP_Osd_PutGd(rgnInfo, 507 + 1 + _Button_Left_Gd.width, 8, &_Button_Right_Gd, TRUE);

      TAP_SPrint(Time, "%u'", MinuteJump);

      FMUC_PutString(rgnInfo, 508, 26, 555, Time, COLOR_White, COLOR_None, &Calibri_12_FontDataUC, TRUE, ALIGN_CENTER);
    }
    TAP_Osd_Sync();
  }

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}


//Playback functions
void Playback_Faster(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("Playback_Faster");
  #endif

  switch(TrickMode)
  {
    case TM_Pause:
    {
      // 1/16xFWD
      TrickModeSpeed = 4;
      Appl_SetPlaybackSpeed(3, TrickModeSpeed, TRUE);
      TrickMode = TM_Slow;
      break;
    }

    case TM_Rwd:
    {
      if(TrickModeSpeed > 1)
      {
        // 64xRWD down to 2xRWD
        TrickModeSpeed--;
        Appl_SetPlaybackSpeed(2, TrickModeSpeed, TRUE);
      }
      else
      {
        // 1/16xFWD
        TrickModeSpeed = 4;
        Appl_SetPlaybackSpeed(3, TrickModeSpeed, TRUE);
        TrickMode = TM_Slow;
      }

      break;
    }

    case TM_Slow:
    {
      if(TrickModeSpeed > 1)
      {
        // 1/16xFWD up to 1/2xFWD
        TrickModeSpeed--;
        Appl_SetPlaybackSpeed(3, TrickModeSpeed, TRUE);
      }
      else
      {
        // 1xFWD
        Playback_Normal();
      }

      break;
    }

    case TM_Play:
    {
      // 2xFWD
      Playback_FFWD();
      break;
    }

    case TM_Fwd:
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
    case TM_Fwd:
    {
      if(TrickModeSpeed > 1)
      {
        // 64xFWD down to 2xFWD
        TrickModeSpeed--;
        Appl_SetPlaybackSpeed(1, TrickModeSpeed, TRUE);
      }
      else
      {
        // 1xFWD
        Playback_Normal();
      }
      break;
    }

    case TM_Play:
    {
      // 1/2xFWD
      Playback_Slow();
      break;
    }

    case TM_Slow:
    {
      if(TrickModeSpeed < 4)
      {
        // 1/2xFWD to 1/16xFWD
        Playback_Slow();
      }
      else
      {
        // 1xRWD
        TrickModeSpeed = 0;
        Playback_RWD();
      }
      break;
    }

    case TM_Pause:
    {
      // 2xRWD
      Playback_RWD();
      break;
    }

    case TM_Rwd:
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

  Appl_SetPlaybackSpeed(0, 1, TRUE);
  TrickMode = TM_Play;
  TrickModeSpeed = 0;

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void Playback_Pause(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("Playback_Pause");
  #endif

  Appl_SetPlaybackSpeed(4, 0, TRUE);
  TrickMode = TM_Pause;
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

  if(TrickModeSpeed < 6) TrickModeSpeed++;
  Appl_SetPlaybackSpeed(1, TrickModeSpeed, TRUE);
  TrickMode = TM_Fwd;

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

  if(TrickModeSpeed < 6) TrickModeSpeed++;
  Appl_SetPlaybackSpeed(2, TrickModeSpeed, TRUE);
  TrickMode = TM_Rwd;

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

  if(TrickModeSpeed < 4) TrickModeSpeed++;
  Appl_SetPlaybackSpeed(3, TrickModeSpeed, TRUE);
  TrickMode = TM_Slow;

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

  if(TrickMode == TM_Pause) Playback_Normal();
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

  if(TrickMode == TM_Pause) Playback_Normal();
  TAP_Hdd_ChangePlaybackPos(JumpBlock);

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
    if(TrickMode != TM_Play) Playback_Normal();
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
      if(TrickMode != TM_Play) Playback_Normal();
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

  int                   i;
  dword                 ThirtySeconds;

  if ((NrBookmarks > 0) && (PlayInfo.currentBlock < Bookmarks[0]))
  {
    if(TrickMode != TM_Play) Playback_Normal();
    TAP_Hdd_ChangePlaybackPos(Bookmarks[NrBookmarks-1]);

    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return;
  }
  
  ThirtySeconds = PlayInfo.totalBlock * 30 / (60*PlayInfo.duration + PlayInfo.durationSec);
  for(i = NrBookmarks - 1; i >= 0; i--)
  {
    if(Bookmarks[i] < (PlayInfo.currentBlock - ThirtySeconds))
    {
      if(TrickMode != TM_Play) Playback_Normal();
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


//Some generic functions
bool isPlaybackRunning(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("isPlaybackRunning");
  #endif

  TAP_Hdd_GetPlayInfo(&PlayInfo);
  if((int)PlayInfo.currentBlock < 0) PlayInfo.currentBlock = 0;

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

  BlockNrLastSecond    = PlayInfo.totalBlock -      (PlayInfo.totalBlock / (60*PlayInfo.duration + PlayInfo.durationSec));
  BlockNrLast10Seconds = PlayInfo.totalBlock - (10 * PlayInfo.totalBlock / (60*PlayInfo.duration + PlayInfo.durationSec));

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

void CheckLastSeconds(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("CheckLastSecond");
  #endif

  if((PlayInfo.currentBlock > BlockNrLastSecond) && (TrickMode != TM_Pause))
    Playback_Pause();
  else if((PlayInfo.currentBlock > BlockNrLast10Seconds) && (TrickMode == TM_Fwd)) 
    Playback_Normal();

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}

//Action Menu
void ActionMenuDraw(void)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("ActionMenuDraw");
  #endif

  dword  C1, C2, C3, C4, DisplayColor;
  int    x, y, i;
  char   TempStr[128];
  char*  DisplayStr;
  word   NrSelectedSegments = 0;

  for(i = 0; i < NrSegmentMarker - 1; i++)
  {
    if(SegmentMarker[i].Selected) NrSelectedSegments++;
  }

  if(!rgnActionMenu)
  {
    rgnActionMenu = TAP_Osd_Create((720 - _ActionMenu9_Gd.width) >> 1, 70, _ActionMenu9_Gd.width, _ActionMenu9_Gd.height, 0, 0);
    ActionMenuItem = 0;
  }

  TAP_Osd_PutGd(rgnActionMenu, 0, 0, &_ActionMenu9_Gd, FALSE);
  TAP_Osd_PutGd(rgnActionMenu, 8, 4 + 28 * ActionMenuItem, &_ActionMenu_Bar_Gd, FALSE);

  x = 20;
  y = MI_NrMenuItems * 30 - 12;
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
      case MI_UnselectAll:
      {
        DisplayStr = LangGetString(LS_UnselectAll);
        if (NrSelectedSegments == 0) DisplayColor = C4;
        break;
      }
      case MI_DeleteFile:         DisplayStr = LangGetString(LS_DeleteFile); DisplayColor = C3; break;
      case MI_ImportBookmarks:    DisplayStr = LangGetString(LS_ImportBM); break;
      case MI_ExitMC:             DisplayStr = LangGetString(LS_ExitMC); break;
      case MI_NrMenuItems:        break;
    }
    if (DisplayStr && (i < MI_NrMenuItems))
      FMUC_PutString(rgnActionMenu, 20, 4 + 28 * i, 300, DisplayStr, DisplayColor, COLOR_None, &Calibri_14_FontDataUC, TRUE, (i==0) ? ALIGN_CENTER : ALIGN_LEFT);
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

  if(ActionMenuItem >= (MI_NrMenuItems - 1))
    ActionMenuItem = 1;
  else
    ActionMenuItem++;

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

  if(ActionMenuItem > 1)
    ActionMenuItem--;
  else
    ActionMenuItem = MI_NrMenuItems - 1;

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
  TAP_Osd_Sync();

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}


//MovieCutter functions
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
  #if STACKTRACE == TRUE
    CallTraceEnter("MovieCutterSelectOddSegments");
  #endif

  word i;

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

  word i;

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

  word i;

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

  int                   NrSelectedSegments;
  bool                  isMultiSelect, CutEnding;
  word                  WorkingSegment;
  char                  CutFileName[MAX_FILE_NAME_SIZE + 1];
  char                  TempFileName[MAX_FILE_NAME_SIZE + 1], TempString[MAX_FILE_NAME_SIZE + 1];
  tTimeStamp            CutStartPoint, BehindCutPoint;
  dword                 DeltaTime, DeltaBlock;
  int                   i, j;
  tResultCode           ret;

  NoPlaybackCheck = TRUE;

  // *CW* FRAGE: Werden die Bookmarks von der Firmware sowieso vor dem Schneiden in die inf gespeichert?
  // -> sonst könnte man der Schnittroutine auch das Bookmark-Array übergeben
  SaveBookmarksToInf();
  CutDumpList();

  // Lege ein Backup der .cut-Datei an
  CutFileSave();
  char BackupName[MAX_FILE_NAME_SIZE + 1];
  strcpy(BackupName, PlaybackName);
  BackupName[strlen(BackupName) - 4] = '\0';
  TAP_SPrint(LogString, "cp \"%s/%s.cut\" \"%s/%s.cut.bak\"", PlaybackDirectory, BackupName, PlaybackDirectory, BackupName);
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

  //ClearOSD();

  for(i = NrSegmentMarker - 2; i >= -1; i--)
  {
    //OSDMenuProgressBarShow(PROGRAM_NAME, LangGetString(LS_Cutting), NrSegmentMarker - i - 1, NrSegmentMarker, NULL);

    if(isMultiSelect)
    {
      //If one or more segments have been selected, work with them.
      WorkingSegment = i;

      // Process the ending at last (*experimental!*)
      if (i == NrSegmentMarker-2) continue;
      if (i == -1) WorkingSegment = NrSegmentMarker-2;
    }
    else
      //If no segment has been selected, use the active segment and break the loop
      WorkingSegment = ActiveSegment;

    if(!isMultiSelect || SegmentMarker[i].Selected)
    {
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
//        if (KeepCut)
          //letztes Segment soll gespeichert werden -> versuche bis zum tatsächlichen Ende zu gehen
//          BehindCutPoint.BlockNr = 0xFFFFFFFF;
//        else
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
          HDD_Rename(PlaybackName, CutFileName);
        }
        else 
          HDD_Delete(PlaybackName);
        TAP_SPrint(LogString, "Renaming cutfile '%s' to '%s'", TempFileName, PlaybackName);
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
        State = ST_CutFailDialog;
        break;
      }

      // Anpassung der verbleibenden Segmente
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
        if(SegmentMarker[j].Block-1 > CutStartPoint.BlockNr)
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
          WriteLogMC(PROGRAM_NAME, "Error processing the last segment: Renaming or nav-generation failed.");
          State = ST_CutFailDialog;
          break;
        }
      }
    }
    if (NrSelectedSegments <= 0) break;
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
  TAP_Hdd_ChangePlaybackPos(SegmentMarker[min(ActiveSegment, NrSegmentMarker-2)].Block);
//  ClearOSD();  // unnötig?
//  OSDRedrawEverything();

  State = ST_IdleNoPlayback;
  NoPlaybackCheck = FALSE;

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
}


dword NavGetBlockTimeStamp(dword PlaybackBlockNr)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("NavGetBlockTimeStamp");
  #endif

  if(TimeStamps == NULL)
  {
    WriteLogMC(PROGRAM_NAME, "Someone is trying to get a timestamp while the array is empty");

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
  }

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
  return LastTimeStamp->Timems;
}


// ------ [COMPATIBILITY LAYER] ------
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
  char                  CurrentDir[512];
  char                  AbsFileName[512];

  WriteLogMC(PROGRAM_NAME, "Patching source nav file (possibly older version with incorrect Times)...");

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
  HDD_TAP_GetCurrentDir(CurrentDir);
  TAP_SPrint(AbsFileName, "%s%s/%s.nav.bak", TAPFSROOT, CurrentDir, SourceFileName);
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

  navRecs = (tnavSD*) TAP_MemAlloc(NAVRECS_SD * sizeof(tnavSD));
  while(TRUE)
  {
    navsRead = fread(navRecs, sizeof(tnavSD), NAVRECS_SD, fSourceNav);
    if(navsRead == 0) break;

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
  char                  CurrentDir[512];
  char                  AbsFileName[512];

  WriteLogMC(PROGRAM_NAME, "Patching source nav file (possibly older version with incorrect Times)...");

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
  HDD_TAP_GetCurrentDir(CurrentDir);
  TAP_SPrint(AbsFileName, "%s%s/%s.nav.bak", TAPFSROOT, CurrentDir, SourceFileName);
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

  navRecs = (tnavHD*) TAP_MemAlloc(NAVRECS_HD * sizeof(tnavHD));
  while(TRUE)
  {
    navsRead = fread(navRecs, sizeof(tnavHD), NAVRECS_HD, fSourceNav);
    if(navsRead == 0) break;

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
