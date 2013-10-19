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
#include                "Graphics/ActionMenu11.gd"
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
  dword                 Time;   //Time in s
  float                 Percent;
  bool                  Selected;
}tSegmentMarker;

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
}tState;

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
  MI_SelectPadding,
  MI_SaveSegment,
  MI_DeleteSegment,
  MI_SelectOddSegments,
  MI_SelectEvenSegments,
  MI_ImportBookmarks,
  MI_GotoNextBM,
  MI_GotoPrevBM,
  MI_DeleteFile,
  MI_ExitMC,
  MI_NrMenuItems
}tMenuItem;

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
  // new language strings V1.5
  LS_SelectOddSegments,
  LS_SelectEvenSegments,
  LS_SelectPadding,
  LS_InvertSelection,
  LS_PageStr,
  LS_ListIsFull,
  LS_ErrorPcrPid,
  LS_ErrorPcr,
  LS_WrongNavLength,
  LS_NrStrings
}tLngStrings;

tState                  State = ST_Init;
int                     NrSegmentMarker;
int                     ActiveSegment;
tSegmentMarker          SegmentMarker[NRSEGMENTMARKER];       //[0]=Start of file, [x]=End of file
int                     NrBookmarks;
dword                  *Bookmarks;

TYPE_PlayInfo           PlayInfo;
char                    PlaybackName[MAX_FILE_NAME_SIZE + 1];
char                    PlaybackDirectory[512];
tTrickMode              TrickMode;
dword                   TrickModeSpeed;
dword                   BlockNrLast10Seconds;
dword                   MinuteJump;                           //Seconds or 0 if deactivated
dword                   MinuteJumpBlocks;                     //Number of blocks, which shall be added
int                     PacketSize;
word                    PCRPID;
dword                   FirstPCR;
bool                    NoPlaybackCheck;                      //Used to circumvent a race condition during the cutting process

extern tFontData        Calibri_14_FontData;
extern tFontData        Calibri_12_FontData;
word                    rgnSegmentList = 0;
word                    rgnInfo = 0;
word                    rgnActionMenu = 0;
int                     ActionMenuItem;
bool                    AutoOSDPolicy = TRUE;

dword                  *pCurTAPTask;
byte                    OurTAPTask;

char                    LogString[512];

int fseeko64 (FILE *__stream, __off64_t __off, int __whence);

int TAP_Main(void)
{
  CreateSettingsDir();
  LoadINI();
  KeyTranslate(TRUE, &TAP_EventHandler);

  if(!LangLoadStrings(LNGFILENAME, LS_NrStrings, LAN_English, PROGRAM_NAME))
  {
    WriteLogMC(PROGRAM_NAME, "Language file is missing!");
    TAP_SPrint(LogString, "Language file '%s' not found!", LNGFILENAME);
    OSDMenuInfoBoxShow(PROGRAM_NAME " " VERSION, LogString, 500);
    do
    {
      OSDMenuEvent(NULL, NULL, NULL);
    }while(OSDMenuInfoBoxIsVisible());
    OSDMenuInfoBoxDestroy();
    return 0;
  }

  return 1;
}

dword TAP_EventHandler(word event, dword param1, dword param2)
{
  dword                 SysState, SysSubState;
  static bool           DoNotReenter = FALSE;
  static dword          LastTotalBlocks = 0;

  (void) param2;

  if(event == EVT_TMSCommander) return TMSCommander_handler(param1);

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
    HookBookmarkFunction(FALSE);
    CutFileSave();
    Cleanup();
    LangUnloadStrings();
    TAP_Exit();
    return param1;
  }

  if(DoNotReenter) return param1;
  DoNotReenter = TRUE;

  TAP_GetState(&SysState, &SysSubState);

  switch(State)
  {
    case ST_Init:             //Executed once upon TAP start
    {
      CleanupCut();

      //Remember the current TAP ID. We need it for the bookmark hook
      pCurTAPTask = (dword*)FIS_vCurTapTask();
      OurTAPTask = *pCurTAPTask;

      HookBookmarkFunction(TRUE);
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

          //"Calculate" the file name (.rec or .mpg)
          if(!PlayInfo.file || !PlayInfo.file->name[0]) PlaybackName[0] = '\0';
          strcpy(PlaybackName, PlayInfo.file->name);
          PlaybackName[strlen(PlaybackName) - 4] = '\0';

          //Extract the absolute path to the rec file and change into that dir
          HDD_GetAbsolutePathByTypeFile(PlayInfo.file, PlaybackDirectory);
          p = strstr(PlaybackDirectory, PlaybackName);
          if(p) *p = '\0';
          TAP_Hdd_ChangeDir(&PlaybackDirectory[strlen(TAPFSROOT)]);

          TAP_SPrint(LogString, "Attaching to %s%s", PlaybackDirectory, PlaybackName);
          WriteLogMC(PROGRAM_NAME, LogString);

          //Check if a nav is available
          if(!isNavAvailable())
          {
            WriteLogMC(PROGRAM_NAME, ".nav is missing");
            OSDMenuMessageBoxInitialize(PROGRAM_NAME, LangGetString(LS_MissingNav));
            OSDMenuMessageBoxButtonAdd(LangGetString(LS_OK));
            OSDMenuMessageBoxShow();
            State = ST_IdleUnacceptedFile;
            break;
          }

          // TODO: Check if nav has correct length! ***CW***
          if(FALSE)
          {
            WriteLogMC(PROGRAM_NAME, ".nav file length not matching duration");
            OSDMenuMessageBoxInitialize(PROGRAM_NAME, LangGetString(LS_WrongNavLength));
            OSDMenuMessageBoxButtonAdd(LangGetString(LS_OK));
            OSDMenuMessageBoxShow();
            State = ST_IdleUnacceptedFile;
            break;
          }

          //Check if it is crypted
          if(isCrypted())
          {
            WriteLogMC(PROGRAM_NAME, "file is crpted");
            OSDMenuMessageBoxInitialize(PROGRAM_NAME, LangGetString(LS_IsCrypted));
            OSDMenuMessageBoxButtonAdd(LangGetString(LS_OK));
            OSDMenuMessageBoxShow();
            State = ST_IdleUnacceptedFile;
            break;
          }

          // detect first PCR (for precise detection of marker time)
          PacketSize = is192ByteTS(PlaybackName) ? 192 : 188;
          if (!GetPCRPID(PlaybackName, &PCRPID))
          {
            WriteLogMC(PROGRAM_NAME, "Could not detect PCR-PID");
            OSDMenuMessageBoxInitialize(PROGRAM_NAME, LangGetString(LS_ErrorPcrPid));  //***CW***
            OSDMenuMessageBoxButtonAdd(LangGetString(LS_OK));
            OSDMenuMessageBoxShow();
            State = ST_IdleUnacceptedFile;
            break;
          }
          if (!GetPCR(PlaybackName, 0, PacketSize, PCRPID, &FirstPCR))
          {
            WriteLogMC(PROGRAM_NAME, "Could not detect first PCR");
            OSDMenuMessageBoxInitialize(PROGRAM_NAME, LangGetString(LS_ErrorPcr));  //***CW***
            OSDMenuMessageBoxButtonAdd(LangGetString(LS_OK));
            OSDMenuMessageBoxShow();
            State = ST_IdleUnacceptedFile;
            break;
          }
#ifdef FULLDEBUG
  TAP_PrintNet("First PCR neu ermittelt: %d\n", FirstPCR);
  TAP_SPrint(LogString, "First PCR neu ermittelt: %d", FirstPCR);
  WriteLogMC(PROGRAM_NAME, LogString);
#endif
          
          NrSegmentMarker = 0;
          ActiveSegment = 0;
          MinuteJump = 0;
          MinuteJumpBlocks = 0;  // nicht unbedingt nötig
          NoPlaybackCheck = FALSE;
          CreateOSD();
          Playback_Normal();
          Calc10Seconds();
          //ReadBookmarks();
          ReadBookmarks_old();
          if(!CutFileLoad()) AddDefaultSegmentMarker();
          OSDRedrawEverything();
          State = ST_Idle;
        }
      }

      break;
    }

    case ST_IdleInvisible:    //Playback is active but OSD is hidden
    {
      // if progress-bar is enabled and cut-key is pressed -> show MovieCutter OSD (ST_IdleNoPlayback)
      if(SysSubState == SUBSTATE_PvrPlayingSearch && (event == EVT_KEY) && ((param1 == RKEY_Ab) || (param1 == RKEY_Option))) State = ST_IdleNoPlayback;

      // if playback-file changed or playback stopped -> show MovieCutter as soon as next playback is started (ST_IdleNoPlayback)
      if(AutoOSDPolicy && (LastTotalBlocks != PlayInfo.totalBlock)) State = ST_IdleNoPlayback;
      if(AutoOSDPolicy && !isPlaybackRunning()) State = ST_IdleNoPlayback;

      break;
    }

    case ST_Idle:             //Playback is active and OSD is visible
    {
      if(!isPlaybackRunning())
      {
        CutFileSave();
        Cleanup();
        State = AutoOSDPolicy ? ST_IdleNoPlayback : ST_IdleInvisible;
        break;
      }

      if(SysSubState == SUBSTATE_Normal) TAP_ExitNormal();

      if(event == EVT_KEY)
      {
        switch(param1)
        {
          case RKEY_Record:
          {
            break;
          }

          case RKEY_Exit:
          {
            CutFileSave();
            Cleanup();
            State = ST_IdleInvisible;

            //Exit immediately so that other functions can not interfere with the cleanup
            DoNotReenter = FALSE;
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
            int NearestMarkerIndex = FindNearestSegmentMarker();
            if(NearestMarkerIndex != -1)
            {
              DeleteSegmentMarker(NearestMarkerIndex);
              OSDRedrawEverything();
            }
            break;
          }

          case RKEY_Green:
          {
            if(AddSegmentMarker(PlayInfo.currentBlock)) OSDRedrawEverything();
            break;
          }

          case RKEY_Yellow:
          {
            MoveSegmentMarker();
            OSDRedrawEverything();
            break;
          }

          case RKEY_Blue:
          {
            SelectSegmentMarker();
            OSDRedrawEverything();
            break;
          }

          case RKEY_Ok:
          {
            if(MinuteJump)
            {
              MinuteJump = 0;
              MinuteJumpBlocks = 0;
              OSDInfoDrawMinuteJump();
            }
            else if(TrickMode != TM_Pause)
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
            else
              Playback_Faster();
            break;
          }

          case RKEY_Left:
          {
            if(MinuteJump)
              Playback_JumpBackward();
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
            MinuteJump = (param1 & 0x0f);
            if(MinuteJump == 0) MinuteJump = 10;
            MinuteJumpBlocks = (PlayInfo.totalBlock / (60 * PlayInfo.duration + PlayInfo.durationSec)) * MinuteJump*60;
            OSDInfoDrawMinuteJump();
            break;
          }
        }
        param1 = 0;
      }

      CheckLast10Seconds();
      OSDInfoDrawProgressbar(FALSE);
      OSDInfoDrawPlayIcons(FALSE);
      SetCurrentSegment();
      OSDInfoDrawCurrentPosition(FALSE);
      OSDInfoDrawClock(FALSE);
      break;
    }

    case ST_IdleUnacceptedFile: //Playback is active but MC can't use that file and is inactive
    {
      if(OSDMenuMessageBoxIsVisible())
      {
        OSDMenuEvent(&event, &param1, &param2);
        if(event == EVT_KEY && param1 == RKEY_Ok) OSDMenuMessageBoxDestroy();
      }
      else
      {
        if(!isPlaybackRunning() || LastTotalBlocks != PlayInfo.totalBlock) State = AutoOSDPolicy ? ST_IdleNoPlayback : ST_IdleInvisible;
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
              ActionMenuRemove();
              State = ST_Idle;
              switch(ActionMenuItem)
              {
                case MI_SaveSegment:        MovieCutterSaveSegments(); break;
                case MI_DeleteSegment:      MovieCutterDeleteSegments(); break;
                case MI_SelectOddSegments:  MovieCutterSelectOddSegments(); break;
                case MI_SelectEvenSegments: MovieCutterSelectEvenSegments(); break;
                case MI_SelectPadding:      MovieCutterSelectPadding(); break;
                case MI_DeleteFile:         MovieCutterDeleteFile(); break;
                case MI_ImportBookmarks:    AddBookmarksToSegmentList(); OSDRedrawEverything(); break;
                case MI_GotoNextBM:         Playback_JumpNextBookmark(); break;
                case MI_GotoPrevBM:         Playback_JumpPrevBookmark(); break;
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
        }
      }
      break;
    }

    case ST_CutFailDialog:    //Cut fail dialog is visible
    {
      OSDMenuMessageBoxInitialize(PROGRAM_NAME, LangGetString(LS_CutHasFailed));
      OSDMenuMessageBoxButtonAdd(LangGetString(LS_OK));
      OSDMenuMessageBoxShow();
      State = ST_IdleUnacceptedFile;
      break;
    }

    case ST_Exit:             //Preparing to terminate the TAP
    {
      HookBookmarkFunction(FALSE);
      CutFileSave();
      Cleanup();
      LangUnloadStrings();
      TAP_Exit();
      break;
    }
  }

  DoNotReenter = FALSE;

  LastTotalBlocks = PlayInfo.totalBlock;

  return param1;
}

dword TMSCommander_handler(dword param1)
{
  switch (param1)
  {
    case TMSCMDR_Capabilities:
    {
      return (dword)(TMSCMDR_CanBeStopped | TMSCMDR_HaveUserEvent);
    }

    case TMSCMDR_UserEvent:
    {
      if(State == ST_IdleInvisible) State = ST_IdleNoPlayback;
      return TMSCMDR_OK;
    }

    case TMSCMDR_Menu:
    {
      return TMSCMDR_NotOK;
    }

    case TMSCMDR_Stop:
    {
      State = ST_Exit;
      return TMSCMDR_OK;
    }
  }

  return TMSCMDR_UnknownFunction;
}

void ClearOSD(void)
{
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
}

void Cleanup(void)
{
  ClearOSD();

  if(Bookmarks)
  {
    TAP_MemFree(Bookmarks);
    Bookmarks = NULL;
  }
}

void CleanupCut(void)
{
  int                   NrFiles, i;
  TYPE_FolderEntry      FolderEntry;
  char                  RecFileName[MAX_FILE_NAME_SIZE + 1];

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
}

void CreateSettingsDir(void)
{
  HDD_TAP_PushDir();
  HDD_ChangeDir("/ProgramFiles");
  if(!TAP_Hdd_Exist("Settings")) TAP_Hdd_Create("Settings", ATTR_FOLDER);
  HDD_ChangeDir("Settings");
  if(!TAP_Hdd_Exist("MovieCutter")) TAP_Hdd_Create("MovieCutter", ATTR_FOLDER);
  HDD_TAP_PopDir();
}

void LoadINI(void)
{
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
}

void SaveINI(void)
{
  HDD_TAP_PushDir();
  HDD_ChangeDir(LOGDIR);
  INIOpenFile(INIFILENAME, PROGRAM_NAME);
  INISetInt("AutoOSDPolicy", AutoOSDPolicy ? 1 : 0);
  INISaveFile(INIFILENAME, INILOCATION_AtCurrentDir, NULL);
  INICloseFile();
  HDD_TAP_PopDir();
}


//Segment marker functions
void AddBookmarksToSegmentList(void)
{
  int i;

  // first, delete all present segment markers (*CW*)
  DeleteAllSegmentMarkers();

  // second, add a segment marker for each bookmark
  TAP_SPrint(LogString, "Importing %d of %d bookmarks", min(NrBookmarks, NRSEGMENTMARKER-2), NrBookmarks);
  WriteLogMC(PROGRAM_NAME, LogString);

  for(i = 0; i < min(NrBookmarks, NRSEGMENTMARKER-2); i++)
  { 
    TAP_SPrint(LogString, "Bookmark %d @ %d", i + 1, Bookmarks[i]);
    WriteLogMC(PROGRAM_NAME, LogString);
    AddSegmentMarker(Bookmarks[i]);
  }
}

void AddDefaultSegmentMarker(void)
{
  AddSegmentMarker(0);
  AddSegmentMarker(PlayInfo.totalBlock);
}

bool AddSegmentMarker(dword Block)
{
  int                   i, j;
  dword                 CurPCR;
  char                  StartTime[10];

  if(NrSegmentMarker >= NRSEGMENTMARKER)
  {
    WriteLogMC(PROGRAM_NAME, "SegmentMarker list is full");
    OSDMenuMessageBoxInitialize(PROGRAM_NAME, LangGetString(LS_ListIsFull));  //***CW***
    OSDMenuMessageBoxButtonAdd(LangGetString(LS_OK));
    OSDMenuMessageBoxShow();
    return FALSE;
  }

  if(!GetPCR(PlaybackName, Block, PacketSize, PCRPID, &CurPCR)) return FALSE;

  //Find the point where to insert the new marker so that the list stays sorted
  if(NrSegmentMarker < 2)
  {
    //If less than 2 markers present, then set marker for start and end of file (called from AddDefaultSegmentMarker)
    SegmentMarker[NrSegmentMarker].Block = Block;
    SegmentMarker[NrSegmentMarker].Time  = (dword)(DeltaPCR(FirstPCR, CurPCR) / 1000);
    SegmentMarker[NrSegmentMarker].Percent = (float)Block * 100 / PlayInfo.totalBlock;
    SegmentMarker[NrSegmentMarker].Selected = FALSE;
    NrSegmentMarker++;
  }
  else
  {
    for(i = 1; i < NrSegmentMarker; i++)
    {
      if(Block < SegmentMarker[i].Block)
      {
        for(j = NrSegmentMarker; j > i; j--)
          memcpy(&SegmentMarker[j], &SegmentMarker[j - 1], sizeof(tSegmentMarker));

        SegmentMarker[i].Block = Block;
        SegmentMarker[i].Time  = (dword)(DeltaPCR(FirstPCR, CurPCR) / 1000);
        SegmentMarker[i].Percent = (float)Block * 100 / PlayInfo.totalBlock;
        SegmentMarker[i].Selected = FALSE;

        SecToTimeString(SegmentMarker[i].Time - SegmentMarker[0].Time, StartTime);
        TAP_SPrint(LogString, "New marker @ block = %d, time = %s, percent = %1.1f%%", Block, StartTime, SegmentMarker[i].Percent);
        WriteLogMC(PROGRAM_NAME, LogString);
        break;
      }
    }
    NrSegmentMarker++;
  }
  return TRUE;
}

int FindNearestSegmentMarker(void)
{
  int                   NearestMarkerIndex;
  int                   MinDelta;
  int                   i;

  NearestMarkerIndex = -1;
  if(NrSegmentMarker > 2)   // at least one segment marker present
  {
    MinDelta = 0x7fffffff;
    for(i = 1; i < NrSegmentMarker - 1; i++)
    {
      if(abs(SegmentMarker[i].Block - PlayInfo.currentBlock) < MinDelta)
      {
        MinDelta = abs(SegmentMarker[i].Block - PlayInfo.currentBlock);
        NearestMarkerIndex = i;
      }
    }
  }
  return NearestMarkerIndex;
}

void MoveSegmentMarker(void)
{
  dword CurPCR;
  int NearestMarkerIndex = FindNearestSegmentMarker();

  if(NearestMarkerIndex != -1)
  {
    if(!GetPCR(PlaybackName, PlayInfo.currentBlock, PacketSize, PCRPID, &CurPCR)) return;
    SegmentMarker[NearestMarkerIndex].Block = PlayInfo.currentBlock;
//    SegmentMarker[NearestMarkerIndex].Time  = (dword)((float)PlayInfo.currentBlock * (PlayInfo.duration * 60 + PlayInfo.durationSec) / PlayInfo.totalBlock);
    SegmentMarker[NearestMarkerIndex].Time  = (dword)(DeltaPCR(FirstPCR, CurPCR) / 1000);
    SegmentMarker[NearestMarkerIndex].Percent = (float)PlayInfo.currentBlock * 100 / PlayInfo.totalBlock;
  }
}

void DeleteSegmentMarker(int MarkerIndex)
{
  int i;

  if((MarkerIndex <= 0) || (MarkerIndex >= (NrSegmentMarker - 1))) return;

  for(i = MarkerIndex; i < NrSegmentMarker - 1; i++)
    memcpy(&SegmentMarker[i], &SegmentMarker[i + 1], sizeof(tSegmentMarker));

  memset(&SegmentMarker[NrSegmentMarker - 1], 0, sizeof(tSegmentMarker));
  NrSegmentMarker--;

  SegmentMarker[NrSegmentMarker - 1].Selected = FALSE;        // the very last marker (no segment)
  if(NrSegmentMarker < 3) SegmentMarker[0].Selected = FALSE;  // there is only one segment (from start to end)
  if(ActiveSegment >= NrSegmentMarker - 1) ActiveSegment = NrSegmentMarker - 2;
}

void DeleteAllSegmentMarkers(void)
{
  int i;

  if (NrSegmentMarker > 2) {
    memcpy(&SegmentMarker[1], &SegmentMarker[NrSegmentMarker-1], sizeof(tSegmentMarker));
    for(i = 2; i < NrSegmentMarker; i++) {
      memset(&SegmentMarker[i], 0, sizeof(tSegmentMarker));
    }
    NrSegmentMarker = 2;
  }
  SegmentMarker[0].Selected = FALSE;
  ActiveSegment = 0;
}

void SetCurrentSegment(void)
{
  int                   i;

  if(NrSegmentMarker < 3) return;

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
}

void SelectSegmentMarker(void)
{
  SegmentMarker[ActiveSegment].Selected = !SegmentMarker[ActiveSegment].Selected;
}

void ReadBookmarks_old(void)
{
  tRECHeaderInfo        RECHeaderInfo;
  byte                 *Buffer;
  TYPE_File            *f;
  dword                 fs;
  int                   i;

  NrBookmarks = 0;
  Bookmarks = NULL;

  f = TAP_Hdd_Fopen(PlayInfo.file->name);
  if(!f)
  {
    WriteLogMC(PROGRAM_NAME, "ReadBookmarks_old: failed to open the inf file");
    return;
  }

  fs = TAP_Hdd_Flen(f);
  Buffer = TAP_MemAlloc(fs);
  TAP_Hdd_Fread(Buffer, fs, 1, f);
  TAP_Hdd_Fclose(f);
  HDD_DecodeRECHeader(Buffer, &RECHeaderInfo, ST_UNKNOWN);
  TAP_MemFree(Buffer);

  //Count the number of bookmarks (not all systems are using the NrBookmarks field)
  for(i = 0; i < 177; i++)
  {
    if(RECHeaderInfo.Bookmark[i])
      NrBookmarks++;
    else
      break;
  }
  Bookmarks = TAP_MemAlloc(NrBookmarks * sizeof(dword));
  if(!Bookmarks) NrBookmarks = 0;
  for(i = 0; i < NrBookmarks; i++)
    Bookmarks[i] = RECHeaderInfo.Bookmark[i];
}

// *CW* TODO: Bugfix - Beim Setzen eines Bookmarks wird die Ordnung nicht beachtet - beim Springen zum nächsten wird sie vorausgesetzt.
//            d.h. wenn man ein Bookmark zwischen 2 bereits vorhandene setzt, und sich vor dem neuen BM befindet, und zum nächsten springt, springt MC zum übernächsten
// *CW* TODO: Bugfix - Das Setzen eines Bookmarks kriegt der Topf nicht mit, da er die INF nicht neu ausliest
// *CW* Idee: Bookmarks im Speicher halten, bei Hook alle neu auslesen (alternativ bei grüner Taste im Idle-Modus)
//            AddBookmark leitet grüne Taste an den Topf weiter (alternativ setzt BM im Speicher, mit Ordnung, Speicher wird beim Beenden in INF geschrieben)
void AddBookmark(dword Block)
{
  //Read inf
  //decode inf
  //add new Bookmarks
  //enconde inf
  //write inf

  tRECHeaderInfo        RECHeaderInfo;
  byte                 *Buffer;
  TYPE_File            *f;
  dword                 fs, BufferSize;
  int                   i;

  if(PlayInfo.file == NULL) return;

  f = TAP_Hdd_Fopen(PlayInfo.file->name);
  if(!f)
  {
    WriteLogMC(PROGRAM_NAME, "AddBookmark: failed to open the inf file");
    return;
  }

  fs = TAP_Hdd_Flen(f);
  BufferSize = (fs < 8192 ? 8192 : fs);
  Buffer = TAP_MemAlloc(BufferSize);
  memset(Buffer, 0, BufferSize);
  TAP_Hdd_Fread(Buffer, fs, 1, f);

  HDD_DecodeRECHeader(Buffer, &RECHeaderInfo, ST_UNKNOWN);

  //Count the number of bookmarks (not all systems are using the NrBookmarks field)
  i = 0;
  while(RECHeaderInfo.Bookmark[i]) i++;
  RECHeaderInfo.Bookmark[i] = Block;
  RECHeaderInfo.NrBookmarks = i;

  HDD_EncodeRECHeader(Buffer, &RECHeaderInfo, ST_UNKNOWN);

  TAP_Hdd_Fseek(f, 0, SEEK_SET);
  TAP_Hdd_Fwrite(Buffer, fs, 1, f);
  TAP_Hdd_Fclose(f);
  TAP_MemFree(Buffer);
}

void ReadBookmarks(void)
{
  int                   i;
  static dword         *___bookmarkTime = NULL;

  if(!___bookmarkTime)
  {
    ___bookmarkTime = (dword*)FIS_vbookmarkTime();
    if(!___bookmarkTime)
    {
      WriteLogMC(PROGRAM_NAME, LangGetString(LS_FailedResolve));
      return;
    }
  }

  NrBookmarks = 0;
  Bookmarks = NULL;

  //Count the number of bookmarks
  //The firmware has 0x240 bytes reserved which equals 144 bookmarks
  for(i = 0; i < 144; i++)
  {
    if(___bookmarkTime[i])
      NrBookmarks++;
  //  else
    //  break;
  }
#ifdef FULLDEBUG
  TAP_PrintNet("Bookmark-Import from Firmware: %d\n", NrBookmarks);
  TAP_SPrint(LogString, "Bookmark-Import from Firmware: %d", NrBookmarks);
  WriteLogMC(PROGRAM_NAME, LogString);
#endif
  Bookmarks = TAP_MemAlloc(NrBookmarks * sizeof(dword));
  if(Bookmarks)
  {
    int p = 0;
    for(i = 0; i < 144; i++)
    {
      if(___bookmarkTime[i]) {
        Bookmarks[p] = ___bookmarkTime[i];
        p++;
        if (p > NrBookmarks-1) break;
      }
    }
  }
  else
    NrBookmarks = 0;
}

bool (*__Appl_SetBookmark)(void) = NULL;

bool Hooked_Appl_SetBookmark(void)
{
  bool ret = __Appl_SetBookmark();

  if(ret)
  {
    switch(State)
    {
      case ST_Init:
      case ST_IdleNoPlayback:
      case ST_IdleUnacceptedFile:
      case ST_ActionDialog:
      case ST_CutFailDialog:
      case ST_Exit:
        break;

      case ST_Idle:
      case ST_IdleInvisible:
      {
        dword           OrigTAPTask;

        OrigTAPTask = *pCurTAPTask;
        *pCurTAPTask = OurTAPTask;
        //AddSegmentMarker(PlayInfo.currentBlock);
        AddBookmark(PlayInfo.currentBlock);
#ifdef FULLDEBUG
  TAP_PrintNet("Bookmark-Hook event catched!\n", FirstPCR);
  TAP_SPrint(LogString, "Bookmark-Hook event catched!", FirstPCR);
  WriteLogMC(PROGRAM_NAME, LogString);
#endif
        *pCurTAPTask = OrigTAPTask;
        break;
      }
    }
  }

  return ret;
}

void HookBookmarkFunction(bool SetHook)
{
  if(SetHook && !__Appl_SetBookmark)
  {
    if(!HookFirmware("_Z16Appl_SetBookmarkv", Hooked_Appl_SetBookmark, (void*)&__Appl_SetBookmark))
    {
      WriteLogMC(PROGRAM_NAME, "Failed to hook Appl_SetBookmark()");
    }
  }

  if(!SetHook && __Appl_SetBookmark)
  {
    if(!UnhookFirmware("_Z16Appl_SetBookmarkv", Hooked_Appl_SetBookmark, (void*)&__Appl_SetBookmark))
    {
      WriteLogMC(PROGRAM_NAME, "Failed to unhook Appl_SetBookmark()");
    }
    else
      __Appl_SetBookmark = NULL;
  }
}


//Cut file functions
bool CutFileLoad(void)
{
  char                  Name[MAX_FILE_NAME_SIZE + 1];
  byte                  Version;
  TYPE_File            *fCut, *fRec;
  long64                FileSize;

  //Load the file size to check if the file didn't change
  fRec = TAP_Hdd_Fopen(PlaybackName);
  if(!fRec)
  {
    WriteLogMC(PROGRAM_NAME, "CutFileLoad: couldn't open .rec for file size verification");
    HDD_TAP_PopDir();
    return FALSE;
  }
  TAP_Hdd_Fclose(fRec);

  strcpy(Name, PlaybackName);
  Name[strlen(Name) - 4] = '\0';
  strcat(Name, ".cut");

  // Check, if cut-File exists
  if(!TAP_Hdd_Exist(Name))
  {
    WriteLogMC(PROGRAM_NAME, "CutFileLoad: .cut doesn't exist");
#ifdef FULLDEBUG
  TAP_PrintNet("CutFileLoad: HDD_TAP_PopDir() ohne HDD_TAP_PushDir()!\nvorher:");
  HDD_TAP_GetCurrentDir(LogString);
  TAP_PrintNet(LogString);
#endif
    HDD_TAP_PopDir();
#ifdef FULLDEBUG
  TAP_PrintNet("\nnachher:");
  HDD_TAP_GetCurrentDir(LogString);
  TAP_PrintNet(LogString);
  TAP_PrintNet("\n");
#endif
    return FALSE;
  }

  // Try to open cut-File
  fCut = TAP_Hdd_Fopen(Name);
  if(!fCut)
  {
    WriteLogMC(PROGRAM_NAME, "CutFileLoad: failed to open .cut");
    HDD_TAP_PopDir();
    return FALSE;
  }

  // Check correct version of cut-File
  TAP_Hdd_Fread(&Version, sizeof(byte), 1, fCut);
  if(Version != CUTFILEVERSION)
  {
    WriteLogMC(PROGRAM_NAME, "CutFileLoad: .cut version mismatch");
    TAP_Hdd_Fclose(fCut);
    HDD_TAP_PopDir();
    return FALSE;
  }

  // Check, if size of cut-File has been changed
  TAP_Hdd_Fread(&FileSize, sizeof(long64), 1, fCut);
  if(fRec->size != FileSize)
  {
    WriteLogMC(PROGRAM_NAME, "CutFileLoad: .cut file size mismatch");
    TAP_Hdd_Fclose(fCut);
    HDD_TAP_PopDir();
    return FALSE;
  }

  TAP_Hdd_Fread(&NrSegmentMarker, sizeof(int), 1, fCut);
  if (NrSegmentMarker > NRSEGMENTMARKER) NrSegmentMarker = NRSEGMENTMARKER;
  TAP_Hdd_Fread(&ActiveSegment, sizeof(int), 1, fCut);
  TAP_Hdd_Fread(SegmentMarker, sizeof(tSegmentMarker), NrSegmentMarker, fCut);
  TAP_Hdd_Fclose(fCut);

  if (SegmentMarker[NrSegmentMarker - 1].Block != PlayInfo.totalBlock) {
#ifdef FULLDEBUG
  TAP_PrintNet("CutFileLoad: Letzter Segment-Marker %d ist ungleich TotalBlock %d!\n", SegmentMarker[NrSegmentMarker - 1].Block, PlayInfo.totalBlock);
  TAP_SPrint(LogString, "CutFileLoad: Letzter Segment-Marker %d ist ungleich TotalBlock %d!\n", SegmentMarker[NrSegmentMarker - 1].Block, PlayInfo.totalBlock);
  WriteLogMC(PROGRAM_NAME, LogString);
#endif
    SegmentMarker[NrSegmentMarker - 1].Block = PlayInfo.totalBlock;
    SegmentMarker[NrSegmentMarker - 1].Time = 60*PlayInfo.duration + PlayInfo.durationSec;
    SegmentMarker[NrSegmentMarker - 1].Percent = 100;
  }
  SegmentMarker[NrSegmentMarker - 1].Selected = FALSE;        // the very last marker (no segment)
  if(NrSegmentMarker < 3) SegmentMarker[0].Selected = FALSE;  // there is only one segment (from start to end)
  if(ActiveSegment >= NrSegmentMarker - 1) ActiveSegment = NrSegmentMarker - 2;

  return TRUE;
}

void CutFileSave(void)
{
  char                  Name[MAX_FILE_NAME_SIZE + 1];
  byte                  Version;
  TYPE_File            *fCut, *fRec;
  long64                FileSize;

  //Save the file size to check if the file didn't change
  fRec = TAP_Hdd_Fopen(PlaybackName);
  if(!fRec) return;
  FileSize = fRec->size;
  TAP_Hdd_Fclose(fRec);

  strcpy(Name, PlaybackName);
  Name[strlen(Name) - 4] = '\0';
  strcat(Name, ".cut");

  Version = CUTFILEVERSION;
  TAP_Hdd_Delete(Name);
  TAP_Hdd_Create(Name, ATTR_NORMAL);
  fCut = TAP_Hdd_Fopen(Name);
  if(!fCut)
  {
    HDD_TAP_PopDir();
    return;
  }

  SegmentMarker[NrSegmentMarker - 1].Selected = FALSE;
  if(NrSegmentMarker < 3)SegmentMarker[0].Selected = FALSE;

  TAP_Hdd_Fwrite(&Version, sizeof(byte), 1, fCut);
  TAP_Hdd_Fwrite(&FileSize, sizeof(long64), 1, fCut);
  TAP_Hdd_Fwrite(&NrSegmentMarker, sizeof(int), 1, fCut);
  TAP_Hdd_Fwrite(&ActiveSegment, sizeof(int), 1, fCut);
  TAP_Hdd_Fwrite(SegmentMarker, sizeof(tSegmentMarker), NrSegmentMarker, fCut);
  TAP_Hdd_Fclose(fCut);
}

void CutFileDelete(void)
{
  char                  Name[MAX_FILE_NAME_SIZE + 1];

  strcpy(Name, PlaybackName);
  Name[strlen(Name) - 4] = '\0';
  strcat(Name, ".cut");
  TAP_Hdd_Delete(Name);
}

void CutDumpList(void)
{
  char                  TimeString[12];
  int                   i;

  WriteLogMC(PROGRAM_NAME, "Seg      Block Time    Pct Sel Act");
  for(i = 0; i < NrSegmentMarker; i++)
  {
    SecToTimeString(SegmentMarker[i].Time, TimeString);
    TAP_SPrint(LogString, "%02d: %010d %s %03d %3s %3s", i, SegmentMarker[i].Block, TimeString, (int)SegmentMarker[i].Percent, SegmentMarker[i].Selected ? "yes" : "no", (i == ActiveSegment ? "*" : ""));
    WriteLogMC(PROGRAM_NAME, LogString);
  }
}


//OSD functions
void CreateOSD(void)
{
  if(!rgnSegmentList)
  {
    TAP_ExitNormal();
    TAP_EnterNormal();
    TAP_ExitNormal();
    rgnSegmentList = TAP_Osd_Create(28, 85, _SegmentList_Background_Gd.width, _SegmentList_Background_Gd.height, 0, 0);
  }
  if(!rgnInfo) rgnInfo = TAP_Osd_Create(0, 576 - _Info_Background_Gd.height, _Info_Background_Gd.width, _Info_Background_Gd.height, 0, 0);
}

void OSDRedrawEverything(void)
{
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
}

void OSDSegmentListDrawList(void)
{
  int                   i, p;
  char                  StartTime[10], EndTime[10], TimeStr[24];
  char                  PageStr[15];
  dword                 fw;
  dword                 C1, C2;

  C1 = COLOR_Yellow;
  C2 = COLOR_White;

  if(rgnSegmentList)
  {
    TAP_Osd_PutGd(rgnSegmentList, 0, 0, &_SegmentList_Background_Gd, FALSE);
    FM_PutString(rgnSegmentList, 5, 3, 80, LangGetString(LS_Segments), COLOR_White, COLOR_None, &Calibri_12_FontData, TRUE, ALIGN_LEFT);
    TAP_Osd_PutGd(rgnSegmentList, 62, 23, &_Button_Up_Gd, TRUE);
    TAP_Osd_PutGd(rgnSegmentList, 88, 23, &_Button_Down_Gd, TRUE);

    if(NrSegmentMarker > 2)
    {
      if(ActiveSegment >= NrSegmentMarker-1)           ActiveSegment = NrSegmentMarker - 2;
      if(ActiveSegment >= 10)                          TAP_Osd_PutGd(rgnSegmentList, 62, 23, &_Button_Up2_Gd, TRUE);
      if((ActiveSegment < 10) && (NrSegmentMarker>11)) TAP_Osd_PutGd(rgnSegmentList, 88, 23, &_Button_Down2_Gd, TRUE);

      TAP_SPrint(PageStr, "%s%d/%d", LangGetString(LS_PageStr), (ActiveSegment/10)+1, ((NrSegmentMarker-2)/10)+1);
      FM_PutString(rgnSegmentList, 60, 3, 114, PageStr, COLOR_White, COLOR_None, &Calibri_12_FontData, FALSE, ALIGN_RIGHT);

      p = (ActiveSegment / 10) * 10;
      for(i = 0; i < min(10, (NrSegmentMarker - p) - 1); i++)
      {
        if((p+i) == ActiveSegment)
        {
          if((SegmentMarker[p+i + 1].Time - SegmentMarker[p+i].Time) < 61)
            TAP_Osd_PutGd(rgnSegmentList, 3, 44 + 28*i, &_Selection_Red_Gd, FALSE);
          else
            TAP_Osd_PutGd(rgnSegmentList, 3, 44 + 28*i, &_Selection_Blue_Gd, FALSE);
        }

        SecToTimeString(SegmentMarker[p+i].Time - SegmentMarker[0].Time, StartTime);
        SecToTimeString(SegmentMarker[p+i + 1].Time - SegmentMarker[0].Time, EndTime);
        TAP_SPrint(TimeStr, "%2d. %s-%s", (p+i + 1), StartTime, EndTime);
        fw = FM_GetStringWidth(TimeStr, &Calibri_12_FontData);            // 250
        FM_PutString(rgnSegmentList, max(0, 58 - (int)(fw >> 1)), 45 + 28*i, 116, TimeStr, (SegmentMarker[p+i].Selected ? C1 : C2), COLOR_None, &Calibri_12_FontData, FALSE, ALIGN_LEFT);
      }
    }
  }
}

void OSDInfoDrawBackground(void)
{
  int                   x;
  char                  s[50];

  if(rgnInfo)
  {
    TAP_Osd_PutGd(rgnInfo, 0, 0, &_Info_Background_Gd, FALSE);

    x = 350;
    TAP_Osd_PutGd(rgnInfo, x, 48, &_Button_Red_Gd, TRUE);
    x += _Button_Red_Gd.width;
    TAP_SPrint(s, LangGetString(LS_Delete));
    FM_PutString(rgnInfo, x, 47, 720, s, COLOR_White, COLOR_None, &Calibri_12_FontData, TRUE, ALIGN_LEFT);
    x += FM_GetStringWidth(s, &Calibri_12_FontData);

    TAP_Osd_PutGd(rgnInfo, x, 48, &_Button_Green_Gd, TRUE);
    x += _Button_Green_Gd.width;
    TAP_SPrint(s, LangGetString(LS_Add));
    FM_PutString(rgnInfo, x, 47, 720, s, COLOR_White, COLOR_None, &Calibri_12_FontData, TRUE, ALIGN_LEFT);
    x += FM_GetStringWidth(s, &Calibri_12_FontData);

    TAP_Osd_PutGd(rgnInfo, x, 48, &_Button_Yellow_Gd, TRUE);
    x += _Button_Yellow_Gd.width;
    TAP_SPrint(s, LangGetString(LS_Move));
    FM_PutString(rgnInfo, x, 47, 720, s, COLOR_White, COLOR_None, &Calibri_12_FontData, TRUE, ALIGN_LEFT);
    x += FM_GetStringWidth(s, &Calibri_12_FontData);

    TAP_Osd_PutGd(rgnInfo, x, 48, &_Button_Blue_Gd, TRUE);
    x += _Button_Blue_Gd.width;
    TAP_SPrint(s, LangGetString(LS_Select));
    FM_PutString(rgnInfo, x, 47, 720, s, COLOR_White, COLOR_None, &Calibri_12_FontData, TRUE, ALIGN_LEFT);

    x = 350;
    TAP_Osd_PutGd(rgnInfo, x, 68, &_Button_Ok_Gd, TRUE);
    x += _Button_Ok_Gd.width;
    TAP_SPrint(s, LangGetString(LS_Pause));
    FM_PutString(rgnInfo, x, 67, 720, s, COLOR_White, COLOR_None, &Calibri_12_FontData, TRUE, ALIGN_LEFT);
    x += FM_GetStringWidth(s, &Calibri_12_FontData);

    TAP_Osd_PutGd(rgnInfo, x, 68, &_Button_Exit_Gd, TRUE);
    x += _Button_Exit_Gd.width;
    TAP_SPrint(s, LangGetString(LS_Exit));
    FM_PutString(rgnInfo, x, 67, 720, s, COLOR_White, COLOR_None, &Calibri_12_FontData, TRUE, ALIGN_LEFT);
  }
}

void OSDInfoDrawRecName(void)
{
  char                  Name[MAX_FILE_NAME_SIZE + 1];

  if(rgnInfo)
  {
    strcpy(Name, PlaybackName);
    Name[strlen(Name) - 4] = '\0';
    FM_PutString(rgnInfo, 65, 11, 490, Name, COLOR_White, COLOR_None, &Calibri_14_FontData, TRUE, ALIGN_LEFT);
  }
}

void OSDInfoDrawProgressbar(bool Force)
{
  int                   y, i;
  dword                 pos = 0;
  dword                 x1, x2;
  dword                 totalBlock;
  static dword          LastDraw = 0;
  static dword          LastPos = 999;

  totalBlock = PlayInfo.totalBlock;
  if(rgnInfo)
  {
    if((abs(TAP_GetTick() - LastDraw) > 30) || Force)
    {
      if(totalBlock) pos = (dword)((float)PlayInfo.currentBlock * 653 / totalBlock);

      if(Force || (pos != LastPos))
      {
        LastPos = pos;

        //The background
        TAP_Osd_PutGd(rgnInfo, 28, 90, &_Info_Progressbar_Gd, FALSE);

        x1 = 34 + (int)((float)653 * SegmentMarker[ActiveSegment].Percent / 100);
        x2 = 34 + (int)((float)653 * SegmentMarker[ActiveSegment + 1].Percent / 100);

        if((SegmentMarker[ActiveSegment + 1].Time - SegmentMarker[ActiveSegment].Time) < 61)
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
}

void OSDInfoDrawPlayIcons(bool Force)
{
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
        case 0x11: FM_PutString(rgnInfo, 657, 26, 697, "2x", COLOR_White, COLOR_None, &Calibri_12_FontData, TRUE, ALIGN_CENTER); break;
        case 0x12: FM_PutString(rgnInfo, 657, 26, 697, "4x", COLOR_White, COLOR_None, &Calibri_12_FontData, TRUE, ALIGN_CENTER); break;
        case 0x13: FM_PutString(rgnInfo, 657, 26, 697, "8x", COLOR_White, COLOR_None, &Calibri_12_FontData, TRUE, ALIGN_CENTER); break;
        case 0x14: FM_PutString(rgnInfo, 657, 26, 697, "16x", COLOR_White, COLOR_None, &Calibri_12_FontData, TRUE, ALIGN_CENTER); break;
        case 0x15: FM_PutString(rgnInfo, 657, 26, 697, "32x", COLOR_White, COLOR_None, &Calibri_12_FontData, TRUE, ALIGN_CENTER); break;
        case 0x16: FM_PutString(rgnInfo, 657, 26, 697, "64x", COLOR_White, COLOR_None, &Calibri_12_FontData, TRUE, ALIGN_CENTER); break;
        case 0x21: FM_PutString(rgnInfo, 549, 26, 589, "2x", COLOR_White, COLOR_None, &Calibri_12_FontData, TRUE, ALIGN_CENTER); break;
        case 0x22: FM_PutString(rgnInfo, 549, 26, 589, "4x", COLOR_White, COLOR_None, &Calibri_12_FontData, TRUE, ALIGN_CENTER); break;
        case 0x23: FM_PutString(rgnInfo, 549, 26, 589, "8x", COLOR_White, COLOR_None, &Calibri_12_FontData, TRUE, ALIGN_CENTER); break;
        case 0x24: FM_PutString(rgnInfo, 549, 26, 589, "16x", COLOR_White, COLOR_None, &Calibri_12_FontData, TRUE, ALIGN_CENTER); break;
        case 0x25: FM_PutString(rgnInfo, 549, 26, 589, "32x", COLOR_White, COLOR_None, &Calibri_12_FontData, TRUE, ALIGN_CENTER); break;
        case 0x26: FM_PutString(rgnInfo, 549, 26, 589, "64x", COLOR_White, COLOR_None, &Calibri_12_FontData, TRUE, ALIGN_CENTER); break;
        case 0x31: FM_PutString(rgnInfo, 602, 26, 642, "1/2x", COLOR_White, COLOR_None, &Calibri_12_FontData, TRUE, ALIGN_CENTER); break;
        case 0x32: FM_PutString(rgnInfo, 602, 26, 642, "1/4x", COLOR_White, COLOR_None, &Calibri_12_FontData, TRUE, ALIGN_CENTER); break;
        case 0x33: FM_PutString(rgnInfo, 602, 26, 642, "1/8x", COLOR_White, COLOR_None, &Calibri_12_FontData, TRUE, ALIGN_CENTER); break;
        case 0x34: FM_PutString(rgnInfo, 602, 26, 642, "1/16x", COLOR_White, COLOR_None, &Calibri_12_FontData, TRUE, ALIGN_CENTER); break;
      }
      LastTrickModeSwitch = TrickModeSwitch;

      TAP_Osd_Sync();
    }
  }
}

void OSDInfoDrawCurrentPosition(bool Force)
{
  int                   Time;
  float                 Percent;
  char                  TimeString[10];
  char                  PercentString[10];
  dword                 fw;
  static byte           LastSec = 99;
  static dword          LastBlock = 9;


  if(PlayInfo.currentBlock != LastBlock) LastBlock = PlayInfo.currentBlock;

  if(rgnInfo && (PlayInfo.totalBlock > 0))
  {
    Percent = (float)PlayInfo.currentBlock / PlayInfo.totalBlock;
    Time = (int)(((float)60 * PlayInfo.duration + PlayInfo.durationSec) * Percent);
    if(((Time % 60) != LastSec) || Force)
    {
      SecToTimeString(Time, TimeString);
      TAP_SPrint(PercentString, " (%1.1f%%)", Percent * 100);
#ifdef FULLDEBUG
//  TAP_PrintNet("Time Refresh: CurrentBlock %d, CurrentTime %s\n", PlayInfo.currentBlock, TimeString);
#endif
      strcat(TimeString, PercentString);
      TAP_Osd_FillBox(rgnInfo, 60, 48, 283, 31, RGB(30, 30, 30));
      fw = FM_GetStringWidth(TimeString, &Calibri_14_FontData);
      FM_PutString(rgnInfo, max(0, 200 - (int)(fw >> 1)), 52, 660, TimeString, COLOR_White, COLOR_None, &Calibri_14_FontData, TRUE, ALIGN_LEFT);
      LastSec = Time % 60;
      TAP_Osd_Sync();
    }
  }
}

void OSDInfoDrawClock(bool Force)
{
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
      FM_PutString(rgnInfo, 638, 65, 710, Time, COLOR_White, COLOR_None, &Calibri_14_FontData, TRUE, ALIGN_LEFT);
      LastMin = min;
      TAP_Osd_Sync();
    }
  }
}

void OSDInfoDrawMinuteJump(void)
{
  char                  Time[4];

  if(rgnInfo)
  {
    TAP_Osd_FillBox(rgnInfo, 507, 8, 50, 35, RGB(51, 51, 51));
    if(MinuteJump)
    {
      TAP_Osd_PutGd(rgnInfo, 507, 8, &_Button_Left_Gd, TRUE);
      TAP_Osd_PutGd(rgnInfo, 507 + 1 + _Button_Left_Gd.width, 8, &_Button_Right_Gd, TRUE);

      TAP_SPrint(Time, "%d'", MinuteJump);

      FM_PutString(rgnInfo, 508, 26, 555, Time, COLOR_White, COLOR_None, &Calibri_12_FontData, TRUE, ALIGN_CENTER);
    }
    TAP_Osd_Sync();
  }
}


//Playback functions
void Playback_Faster(void)
{
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
}

void Playback_Slower(void)
{
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
}

void Playback_Normal(void)
{
  Appl_SetPlaybackSpeed(0, 1, TRUE);
  TrickMode = TM_Play;
  TrickModeSpeed = 0;
}

void Playback_Pause(void)
{
  Appl_SetPlaybackSpeed(4, 0, TRUE);
  TrickMode = TM_Pause;
  TrickModeSpeed = 0;
}

void Playback_FFWD(void)
{
  //Appl_SetPlaybackSpeed(1, 1, true) 2x FFWD
  //Appl_SetPlaybackSpeed(1, 2, true) 4x FFWD
  //Appl_SetPlaybackSpeed(1, 3, true) 8x FFWD
  //Appl_SetPlaybackSpeed(1, 4, true) 16x FFWD
  //Appl_SetPlaybackSpeed(1, 5, true) 32x FFWD
  //Appl_SetPlaybackSpeed(1, 6, true) 64x FFWD

  if(TrickModeSpeed < 6) TrickModeSpeed++;
  Appl_SetPlaybackSpeed(1, TrickModeSpeed, TRUE);
  TrickMode = TM_Fwd;
}

void Playback_RWD(void)
{
  //Appl_SetPlaybackSpeed(2, 1, true) 2x RWD
  //Appl_SetPlaybackSpeed(2, 2, true) 4x RWD
  //Appl_SetPlaybackSpeed(2, 3, true) 8x RWD
  //Appl_SetPlaybackSpeed(2, 4, true) 16x RWD
  //Appl_SetPlaybackSpeed(2, 5, true) 32x RWD
  //Appl_SetPlaybackSpeed(2, 6, true) 64x RWD

  if(TrickModeSpeed < 6) TrickModeSpeed++;
  Appl_SetPlaybackSpeed(2, TrickModeSpeed, TRUE);
  TrickMode = TM_Rwd;
}

void Playback_Slow(void)
{
  //Appl_SetPlaybackSpeed(3, 1, true) 1/2x Slow
  //Appl_SetPlaybackSpeed(3, 2, true) 1/4x Slow
  //Appl_SetPlaybackSpeed(3, 3, true) 1/8x Slow
  //Appl_SetPlaybackSpeed(3, 4, true) 1/16x Slow

  if(TrickModeSpeed < 4) TrickModeSpeed++;
  Appl_SetPlaybackSpeed(3, TrickModeSpeed, TRUE);
  TrickMode = TM_Slow;
}

void Playback_JumpForward(void)
{
  dword                 JumpBlock;
  JumpBlock = min(PlayInfo.currentBlock + MinuteJumpBlocks, BlockNrLast10Seconds);

  if(TrickMode == TM_Pause) Playback_Normal();
  TAP_Hdd_ChangePlaybackPos(JumpBlock);
}

void Playback_JumpBackward(void)
{
  dword                 JumpBlock;
  JumpBlock = max(PlayInfo.currentBlock - MinuteJumpBlocks, 0);

  if(TrickMode == TM_Pause) Playback_Normal();
  TAP_Hdd_ChangePlaybackPos(JumpBlock);
}

// *CW* ACHTUNG!! Setzt Ordnung der Bookmarks voraus! -> durch AddBookmark() derzeit nicht gegeben
void Playback_JumpNextBookmark(void)
{
  int i;

  if ((NrBookmarks > 0) && (PlayInfo.currentBlock > Bookmarks[NrBookmarks-1]))
  {
    if(TrickMode != TM_Play) Playback_Normal();
    TAP_Hdd_ChangePlaybackPos(Bookmarks[0]);
    return;
  }
  
  for(i = 0; i < NrBookmarks; i++)
  {
    if(PlayInfo.currentBlock < Bookmarks[i])
    {
      if(TrickMode != TM_Play) Playback_Normal();
      TAP_Hdd_ChangePlaybackPos(Bookmarks[i]);
      return;
    }
  }
}

void Playback_JumpPrevBookmark(void)
{
  int                   i;
  dword                 ThirtySeconds;

  if ((NrBookmarks > 0) && (PlayInfo.currentBlock < Bookmarks[0]))
  {
    if(TrickMode != TM_Play) Playback_Normal();
    TAP_Hdd_ChangePlaybackPos(Bookmarks[NrBookmarks-1]);
    return;
  }
  
  ThirtySeconds = PlayInfo.totalBlock * 30 / (60*PlayInfo.duration + PlayInfo.durationSec);
  for(i = NrBookmarks - 1; i >= 0; i--)
  {
    if(Bookmarks[i] < (PlayInfo.currentBlock - ThirtySeconds))
    {
      if(TrickMode != TM_Play) Playback_Normal();
      TAP_Hdd_ChangePlaybackPos(Bookmarks[i]);
      return;
    }
  }
}


//Some generic functions
void SecToTimeString(dword Time, char *TimeString)
{
  dword                 Hour, Min, Sec;

  Hour = (int)(Time / 3600);
  Min = (int)(Time / 60) % 60;
  Sec = Time % 60;
  TAP_SPrint(TimeString, "%d:%2.2d:%2.2d", Hour, Min, Sec);
}

bool isPlaybackRunning(void)
{
  TAP_Hdd_GetPlayInfo(&PlayInfo);
#ifdef FULLDEBUG
  if((int)PlayInfo.currentBlock < 0) {
    TAP_PrintNet("!!First bit of currentBlock is 1!\n");
    WriteLogMC(PROGRAM_NAME, "!!First bit of currentBlock is 1!");
  }
#endif
  if((int)PlayInfo.currentBlock < 0) PlayInfo.currentBlock = 0;

  return ((PlayInfo.playMode == PLAYMODE_Playing) || NoPlaybackCheck);
}

void Calc10Seconds(void)
{
  BlockNrLast10Seconds = PlayInfo.totalBlock - PlayInfo.totalBlock * 10 / (60*PlayInfo.duration + PlayInfo.durationSec);
}

void CheckLast10Seconds(void)
{
  if((PlayInfo.currentBlock > BlockNrLast10Seconds)  && (TrickMode != TM_Pause)) Playback_Pause();
}

bool isNavAvailable(void)
{
  char                  NavFileName[MAX_FILE_NAME_SIZE + 1];

  TAP_SPrint(NavFileName, "%s.nav", PlaybackName);
  return TAP_Hdd_Exist(NavFileName);
}

bool isCrypted(void)
{
  TYPE_File            *f;
  byte                  CryptFlag = 2;
  char                  InfFileName[MAX_FILE_NAME_SIZE + 1];

  TAP_SPrint(InfFileName, "%s.inf", PlaybackName);
  f = TAP_Hdd_Fopen(InfFileName);
  if(f)
  {
    TAP_Hdd_Fseek(f, 0x0010, 0);
    TAP_Hdd_Fread(&CryptFlag, 1, 1, f);
    TAP_Hdd_Fclose(f);
  }

  return ((CryptFlag & 1) != 0);
}


//Action Menu
void ActionMenuDraw(void)
{
  dword  C1, C2, C3, C4;
  int    x, y, i;

  if(!rgnActionMenu)
  {
    rgnActionMenu = TAP_Osd_Create((720 - _ActionMenu11_Gd.width) >> 1, 70, _ActionMenu11_Gd.width, _ActionMenu11_Gd.height, 0, 0);
    ActionMenuItem = 0;
  }

  TAP_Osd_PutGd(rgnActionMenu, 0, 0, &_ActionMenu11_Gd, FALSE);
  TAP_Osd_PutGd(rgnActionMenu, 8, 4 + 28 * ActionMenuItem, &_ActionMenu_Bar_Gd, FALSE);

  x = 20;
  y = MI_NrMenuItems * 30 - 18;
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
    switch(i)
    {
      case MI_SelectFunction:     FM_PutString(rgnActionMenu, 20, 4 + 28 * i, 300, LangGetString(LS_SelectFunction), (ActionMenuItem == MI_SelectFunction ? C1 : C2), COLOR_None, &Calibri_14_FontData, TRUE, ALIGN_LEFT); break;
      case MI_SaveSegment:        FM_PutString(rgnActionMenu, 20, 4 + 28 * i, 300, LangGetString(LS_SaveSegments), (ActionMenuItem == MI_SaveSegment ? C1 : C2), COLOR_None, &Calibri_14_FontData, TRUE, ALIGN_LEFT); break;
      case MI_DeleteSegment:      FM_PutString(rgnActionMenu, 20, 4 + 28 * i, 300, LangGetString(LS_DeleteSegments), (ActionMenuItem == MI_DeleteSegment ? C1 : C2), COLOR_None, &Calibri_14_FontData, TRUE, ALIGN_LEFT); break;
      case MI_SelectOddSegments:  FM_PutString(rgnActionMenu, 20, 4 + 28 * i, 300, LangGetString(LS_SelectOddSegments), (ActionMenuItem == MI_SelectOddSegments ? C1 : C2), COLOR_None, &Calibri_14_FontData, TRUE, ALIGN_LEFT); break;
      case MI_SelectEvenSegments: FM_PutString(rgnActionMenu, 20, 4 + 28 * i, 300, LangGetString(LS_SelectEvenSegments), (ActionMenuItem == MI_SelectEvenSegments ? C1 : C2), COLOR_None, &Calibri_14_FontData, TRUE, ALIGN_LEFT); break;
      case MI_SelectPadding:      FM_PutString(rgnActionMenu, 20, 4 + 28 * i, 300, LangGetString(LS_SelectPadding), (ActionMenuItem == MI_SelectPadding ? C1 : (NrSegmentMarker == 4 ? C2 : C4)), COLOR_None, &Calibri_14_FontData, TRUE, ALIGN_LEFT); break;
      case MI_DeleteFile:         FM_PutString(rgnActionMenu, 20, 4 + 28 * i, 300, LangGetString(LS_DeleteFile), (ActionMenuItem == MI_DeleteFile ? C1 : C3), COLOR_None, &Calibri_14_FontData, TRUE, ALIGN_LEFT); break;
      case MI_ImportBookmarks:    FM_PutString(rgnActionMenu, 20, 4 + 28 * i, 300, LangGetString(LS_ImportBM), (ActionMenuItem == MI_ImportBookmarks ? C1 : C2), COLOR_None, &Calibri_14_FontData, TRUE, ALIGN_LEFT); break;
      case MI_GotoNextBM:         FM_PutString(rgnActionMenu, 20, 4 + 28 * i, 300, LangGetString(LS_GotoNextBM), (ActionMenuItem == MI_GotoNextBM ? C1 : C2), COLOR_None, &Calibri_14_FontData, TRUE, ALIGN_LEFT); break;
      case MI_GotoPrevBM:         FM_PutString(rgnActionMenu, 20, 4 + 28 * i, 300, LangGetString(LS_GotoPrevBM), (ActionMenuItem == MI_GotoPrevBM ? C1 : C2), COLOR_None, &Calibri_14_FontData, TRUE, ALIGN_LEFT); break;
      case MI_ExitMC:             FM_PutString(rgnActionMenu, 20, 4 + 28 * i, 300, LangGetString(LS_ExitMC), (ActionMenuItem == MI_ExitMC ? C1 : C2), COLOR_None, &Calibri_14_FontData, TRUE, ALIGN_LEFT); break;
      case MI_NrMenuItems:        break;
    }
  }

  TAP_Osd_Sync();
}

void ActionMenuDown(void)
{
  if(ActionMenuItem >= (MI_NrMenuItems - 1))
    ActionMenuItem = 1;
  else
    ActionMenuItem++;

  ActionMenuDraw();
}

void ActionMenuUp(void)
{
  if(ActionMenuItem > 1)
    ActionMenuItem--;
  else
    ActionMenuItem = MI_NrMenuItems - 1;

  ActionMenuDraw();
}

void ActionMenuRemove(void)
{
  TAP_Osd_Delete(rgnActionMenu);
  rgnActionMenu = 0;
  OSDRedrawEverything();
  TAP_Osd_Sync();
}


//MovieCutter functions
void MovieCutterSaveSegments(void)
{
  MovieCutterProcess(TRUE, TRUE);
}

void MovieCutterDeleteSegments(void)
{
  MovieCutterProcess(TRUE, FALSE);
}

void MovieCutterSelectOddSegments(void)
{
  int                   i;

  for(i = 0; i < NrSegmentMarker-1; i++)
    SegmentMarker[i].Selected = ((i & 1) == 0);

  OSDSegmentListDrawList();
  OSDInfoDrawProgressbar(TRUE);
//  OSDRedrawEverything();
//  MovieCutterProcess(TRUE, FALSE);
}

void MovieCutterSelectEvenSegments(void)
{
  int                   i;

  for(i = 0; i < NrSegmentMarker-1; i++)
    SegmentMarker[i].Selected = ((i & 1) == 1);

  OSDSegmentListDrawList();
  OSDInfoDrawProgressbar(TRUE);
//  OSDRedrawEverything();
//  MovieCutterProcess(TRUE, FALSE);
}

void MovieCutterSelectPadding(void)
{
  if(NrSegmentMarker == 4)
  {
    SegmentMarker[0].Selected = TRUE;
    SegmentMarker[1].Selected = FALSE;
    SegmentMarker[2].Selected = TRUE;
    SegmentMarker[3].Selected = FALSE;

    OSDSegmentListDrawList();
    OSDInfoDrawProgressbar(TRUE);
//    OSDRedrawEverything();
//    TAP_Osd_Sync();
//    MovieCutterProcess(TRUE, FALSE);
  }
}

void MovieCutterDeleteFile(void)
{
  NoPlaybackCheck = TRUE;
  TAP_Hdd_StopTs();
  CutFileDelete();
  HDD_Delete(PlaybackName);
  NoPlaybackCheck = FALSE;
}

void MovieCutterProcess(bool KeepSource, bool KeepCut)
{
  int                   i, j;
  dword                 SelectedBlock;
  dword                 DeltaTime, DeltaBlock;
  bool                  isMultiSelect;
  unsigned int          NrSelectedSegments;
  int                   WorkingSegment;
  bool                  ret;

  NoPlaybackCheck = TRUE;
  isMultiSelect = FALSE;
  NrSelectedSegments = 0;
  for(i = 0; i < NrSegmentMarker - 1; i++)
  {
    if(SegmentMarker[i].Selected)
      NrSelectedSegments++;
  }
  isMultiSelect = (NrSelectedSegments > 0);

  CutDumpList();
  //ClearOSD();

  for(i = NrSegmentMarker - 2; i >= 0; i--)
  {
    //OSDMenuProgressBarShow(PROGRAM_NAME, LangGetString(LS_Cutting), NrSegmentMarker - i - 1, NrSegmentMarker, NULL);

    if(isMultiSelect)
      //If one or more segments have been selected, work with them.
      WorkingSegment = i;
    else
      //If no segment has been selected, use the active segment and break the loop
      WorkingSegment = ActiveSegment;

    if(!isMultiSelect || SegmentMarker[i].Selected)
    {
      TAP_SPrint(LogString, "Processing segment %d", WorkingSegment);
      WriteLogMC(PROGRAM_NAME, LogString);
      ret = MovieCutter(PlaybackName, SegmentMarker[WorkingSegment].Block, SegmentMarker[WorkingSegment + 1].Block, KeepSource, KeepCut, NrSelectedSegments-1);
      TAP_Hdd_PlayTs(PlaybackName);
      do
      {
        isPlaybackRunning();
      }while((int)PlayInfo.totalBlock == -1);

      //Bail out if the cut failed
      if(!ret)
      {
        State = ST_CutFailDialog;
        break;
      }

      SelectedBlock = SegmentMarker[WorkingSegment].Block;
      DeltaBlock = SegmentMarker[WorkingSegment + 1].Block - SelectedBlock;
      DeltaTime = SegmentMarker[WorkingSegment + 1].Time - SegmentMarker[WorkingSegment].Time;
      DeleteSegmentMarker(WorkingSegment);
      NrSelectedSegments--;

      TAP_SPrint(LogString, "Reported new totalBlock = %d", PlayInfo.totalBlock);
      WriteLogMC(PROGRAM_NAME, LogString);
      for(j = NrSegmentMarker - 1; j >= WorkingSegment; j--)
      {
        if(SegmentMarker[j].Block > SelectedBlock)
        {
          SegmentMarker[j].Block -= DeltaBlock;
          SegmentMarker[j].Time -= DeltaTime;
        }

        if((j == NrSegmentMarker - 1) && (SegmentMarker[j].Block != PlayInfo.totalBlock))
        {
#ifdef FULLDEBUG
  TAP_PrintNet("MovieCutterProcess: Letzter Segment-Marker %d ist ungleich TotalBlock %d!\n", SegmentMarker[NrSegmentMarker - 1].Block, PlayInfo.totalBlock);
  TAP_SPrint(LogString, "MovieCutterProcess: Letzter Segment-Marker %d ist ungleich TotalBlock %d!\n", SegmentMarker[NrSegmentMarker - 1].Block, PlayInfo.totalBlock);
  WriteLogMC(PROGRAM_NAME, LogString);
#endif
          SegmentMarker[j].Block = PlayInfo.totalBlock;
          SegmentMarker[j].Time = 60*PlayInfo.duration + PlayInfo.durationSec;
        }
        SegmentMarker[j].Percent = (float)SegmentMarker[j].Block * 100 / SegmentMarker[NrSegmentMarker - 1].Block;

        //If the first marker has moved to block 0, delete it
        if(SegmentMarker[j].Block == 0) DeleteSegmentMarker(j);
      }

      Calc10Seconds();
      //ReadBookmarks();
      ReadBookmarks_old();
      CutFileSave();
      CutDumpList();
    }
    if(!isMultiSelect) break;
  }

  //OSDMenuProgressBarShow(PROGRAM_NAME, LangGetString(LS_Cutting), 1, 1, NULL);
  //OSDMenuProgressBarDestroy();
  Playback_Normal();
  TAP_Hdd_ChangePlaybackPos(SegmentMarker[ActiveSegment].Block);
  OSDRedrawEverything();
  NoPlaybackCheck = FALSE;
}
