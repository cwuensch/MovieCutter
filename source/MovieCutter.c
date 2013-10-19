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
#include                "Graphics/Button_Down.gd"
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
  ST_Init,
  ST_IdleNoPlayback,
  ST_Idle,
  ST_IdleInvisible,
  ST_IdleUnacceptedFile,
  ST_ActionDialog,
  ST_CutFailDialog,
  ST_Exit
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
  MI_RemovePadding,
  MI_SaveSegment,
  MI_DeleteSegment,
  MI_DeleteOddSegments,
  MI_DeleteEvenSegments,
  MI_ImportBookmarks,
  MI_GotoNextBM,
  MI_GotoPrevBM,
  MI_DeleteFile,
  MI_ExitMC,
  MI_NrMenuItems
}tMenuItem;

typedef enum
{
  LS_Add,
  LS_Delete,
  LS_Exit,
  LS_Move,
  LS_Pause,
  LS_Select,
  LS_MissingNav,
  LS_DeleteSegments,
  LS_DeleteFile,
  LS_ExitMC,
  LS_FailedResolve,
  LS_GotoNextBM,
  LS_GotoPrevBM,
  LS_ImportBM,
  LS_OK,
  LS_RemovePadding,
  LS_SaveSegments,
  LS_Segments,
  LS_SelectFunction,
  LS_CutHasFailed,
  LS_IsCrypted,
  LS_Cutting,
  LS_DeleteOddSegments,
  LS_DeleteEvenSegments,
  LS_NrStrings
}tLngStrings;

tState                  State = ST_Init;
int                     NrSegmentMarker;
int                     ActiveSegment;
tSegmentMarker          SegmentMarker[NRSEGMENTMARKER];       //[0]=Start of file, [x]=End of file
int                     NrBookmarks;
dword                  *Bookmarks;
word                    rgnSegmentList = 0;
word                    rgnInfo = 0;
extern tFontData        Calibri_14_FontData;
extern tFontData        Calibri_12_FontData;

TYPE_PlayInfo           PlayInfo;
char                    PlaybackName[MAX_FILE_NAME_SIZE + 1];
char                    PlaybackDirectory[512];
tTrickMode              TrickMode;
dword                   TrickModeSpeed;
dword                   BlockNrLast10Seconds;
dword                   MinuteJump;                           //Seconds or 0 if deactivated
int                     PacketSize;

word                    rgnActionMenu = 0;
int                     ActionMenuItem;
bool                    NoPlaybackCheck;                      //Used to circumvent a race condition during the cutting process
bool                    AutoOSDPolicy = TRUE;

dword                  *pCurTAPTask;
byte                    OurTAPTask;

char                    LogString[512];

void (*Appl_PvrPause)(bool) = NULL;
void (*Appl_SetPlaybackSpeed)(unsigned char, int, bool) = NULL;
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
      Appl_PvrPause = (void*)TryResolve("_Z13Appl_PvrPauseb");
      Appl_SetPlaybackSpeed = (void*)TryResolve("_Z21Appl_SetPlaybackSpeedhib");

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

          PacketSize = is192ByteTS(PlaybackName) ? 192 : 188;
          NrSegmentMarker = 0;
          ActiveSegment = 0;
          MinuteJump = 0;
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
      if(SysSubState == SUBSTATE_PvrPlayingSearch && (event == EVT_KEY) && ((param1 == RKEY_Ab) || (param1 == RKEY_Option))) State = ST_IdleNoPlayback;

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
            if((NrSegmentMarker > 2) && (ActiveSegment < (NrSegmentMarker - 2)))
            {
              ActiveSegment++;
              OSDSegmentListDrawList();
              if(TrickMode == TM_Pause) Playback_Normal();
              TAP_Hdd_ChangePlaybackPos(SegmentMarker[ActiveSegment].Block);
            }
            break;
          }

          case RKEY_Up:
          {
            if((NrSegmentMarker > 2) && (ActiveSegment > 0))
            {
              ActiveSegment--;
              OSDSegmentListDrawList();
              if(TrickMode == TM_Pause) Playback_Normal();
              TAP_Hdd_ChangePlaybackPos(SegmentMarker[ActiveSegment].Block);
            }
            break;
          }

          case RKEY_Red:
          {
            int         NearestMarkerIndex;
            int         MinDelta;
            int         i;

            //Delete nearest marker
            if(NrSegmentMarker > 2)
            {
              NearestMarkerIndex = -1;

              MinDelta = 0x7fffffff;
              for(i = 1; i < NrSegmentMarker - 1; i++)
              {
                if(abs(SegmentMarker[i].Block - PlayInfo.currentBlock) < MinDelta)
                {
                  MinDelta = abs(SegmentMarker[i].Block - PlayInfo.currentBlock);
                  NearestMarkerIndex = i;
                }
              }

              if(NearestMarkerIndex != -1)
              {
                DeleteSegmentMarker(NearestMarkerIndex);
                OSDRedrawEverything();
              }
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
            MinuteJump = 60 * (param1 & 0x0f);
            if(MinuteJump == 0) MinuteJump = 600;
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
                case MI_DeleteOddSegments:  MovieCutterDeleteOddSegments(); break;
                case MI_DeleteEvenSegments: MovieCutterDeleteEvenSegments(); break;
                case MI_RemovePadding:      MovieCutterDeletePadding(); break;
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
  TAP_Hdd_Create("Settings", ATTR_FOLDER);
  HDD_ChangeDir("Settings");
  TAP_Hdd_Create("MovieCutter", ATTR_FOLDER);
  HDD_TAP_PopDir();
}

void LoadINI(void)
{
  HDD_TAP_PushDir();
  HDD_ChangeDir(LOGDIR);
  if(INIOpenFile(INIFILENAME, PROGRAM_NAME) != INILOCATION_NewFile)
  {
    AutoOSDPolicy = INIGetInt("AutoOSDPolicy", 1, 0, 1) != 0;
  }
  INICloseFile();
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

  TAP_SPrint(LogString, "Importing %d bookmarks", NrBookmarks);
  WriteLogMC(PROGRAM_NAME, LogString);

  if(NrBookmarks <= 10)
    for(i = 0; i < NrBookmarks; i++)
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
  dword                 PCR1, PCR2;
  char                  StartTime[12];

  if(NrSegmentMarker == (NRSEGMENTMARKER - 1))
  {
    WriteLogMC(PROGRAM_NAME, "SegmentMarker list is full");
    return FALSE;
  }

  if(!GetPCR(PlaybackName, 0, PacketSize, &PCR1)) return FALSE;
  if(!GetPCR(PlaybackName, Block, PacketSize, &PCR2)) return FALSE;

  //Find the point where to insert the new marker so that the list stays sorted
  if(NrSegmentMarker < 2)
  {
    //The first two markers will always be from the beginning and the end of the current file
    SegmentMarker[NrSegmentMarker].Block = Block;
    SegmentMarker[NrSegmentMarker].Time  = (dword)(DeltaPCR(PCR1, PCR2) / 1000);
    SegmentMarker[NrSegmentMarker].Percent = (float)Block * 100 / PlayInfo.totalBlock;
    NrSegmentMarker++;
  }
  else
  {
    for(i = 0; i < NrSegmentMarker; i++)
    {
      if(Block < SegmentMarker[i].Block)
      {
        for(j = NRSEGMENTMARKER - 1; j > i; j--)
          memcpy(&SegmentMarker[j], &SegmentMarker[j - 1], sizeof(tSegmentMarker));

        SegmentMarker[i].Block = Block;
        SegmentMarker[i].Time  = (dword)(DeltaPCR(PCR1, PCR2) / 1000);
        SegmentMarker[i].Percent = (float)Block * 100 / PlayInfo.totalBlock;

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

void MoveSegmentMarker(void)
{
  int                   NearestMarkerIndex;
  int                   MinDelta;
  int                   i;

  //MoveSegmentMarker nearest marker
  if(NrSegmentMarker > 2)
  {
    NearestMarkerIndex = -1;
    MinDelta = 0x7fffffff;
    for(i = 1; i < NrSegmentMarker - 1; i++)
    {
      if(abs(SegmentMarker[i].Block - PlayInfo.currentBlock) < MinDelta)
      {
        MinDelta = abs(SegmentMarker[i].Block - PlayInfo.currentBlock);
        NearestMarkerIndex = i;
      }
    }

    if(NearestMarkerIndex != -1)
    {
      SegmentMarker[NearestMarkerIndex].Block = PlayInfo.currentBlock;
      SegmentMarker[NearestMarkerIndex].Time  = (dword)((float)PlayInfo.currentBlock * (PlayInfo.duration * 60 + PlayInfo.durationSec) / PlayInfo.totalBlock);
      SegmentMarker[NearestMarkerIndex].Percent = (float)PlayInfo.currentBlock * 100 / PlayInfo.totalBlock;
    }
  }
}

void DeleteSegmentMarker(int MarkerIndex)
{
  int i;

  if((MarkerIndex <= 0) || (MarkerIndex >= (NrSegmentMarker - 1))) return;

  for(i = MarkerIndex; i < NRSEGMENTMARKER - 1; i++)
    memcpy(&SegmentMarker[i], &SegmentMarker[i + 1], sizeof(tSegmentMarker));

  memset(&SegmentMarker[NRSEGMENTMARKER - 1], 0, sizeof(tSegmentMarker));
  NrSegmentMarker--;
  if(ActiveSegment >= NrSegmentMarker - 1) ActiveSegment--;
}

void SetCurrentSegment(void)
{
  int                   i;

  if(NrSegmentMarker < 3) return;

  for(i = 0; i < NrSegmentMarker; i++)
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
    ___bookmarkTime = (dword*)TryResolve("_bookmarkTime");
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
  TAP_PrintNet("NrBookmarks=%d\n", NrBookmarks);
  Bookmarks = TAP_MemAlloc(NrBookmarks * sizeof(dword));
  if(Bookmarks)
  {
    for(i = 0; i < NrBookmarks; i++)
      Bookmarks[i] = ___bookmarkTime[i];
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
  TYPE_File            *f, *fRec;
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

  if(!TAP_Hdd_Exist(Name))
  {
    WriteLogMC(PROGRAM_NAME, "CutFileLoad: .cut doesn't exist");
    HDD_TAP_PopDir();
    return FALSE;
  }

  f = TAP_Hdd_Fopen(Name);
  if(!f)
  {
    WriteLogMC(PROGRAM_NAME, "CutFileLoad: failed to open .cut");
    HDD_TAP_PopDir();
    return FALSE;
  }

  TAP_Hdd_Fread(&Version, sizeof(byte), 1, f);
  if(Version != CUTFILEVERSION)
  {
    WriteLogMC(PROGRAM_NAME, "CutFileLoad: .cut version mismatch");
    TAP_Hdd_Fclose(f);
    HDD_TAP_PopDir();
    return FALSE;
  }

  TAP_Hdd_Fread(&FileSize, sizeof(long64), 1, f);
  if(fRec->size != FileSize)
  {
    WriteLogMC(PROGRAM_NAME, "CutFileLoad: .cut file size mismatch");
    TAP_Hdd_Fclose(f);
    HDD_TAP_PopDir();
    return FALSE;
  }

  TAP_Hdd_Fread(&NrSegmentMarker, sizeof(int), 1, f);
  TAP_Hdd_Fread(&ActiveSegment, sizeof(int), 1, f);
  TAP_Hdd_Fread(SegmentMarker, sizeof(tSegmentMarker), NRSEGMENTMARKER, f);
  TAP_Hdd_Fclose(f);

  SegmentMarker[NrSegmentMarker - 1].Selected = FALSE;
  if(NrSegmentMarker < 3)SegmentMarker[0].Selected = FALSE;

  return TRUE;
}

void CutFileSave(void)
{
  char                  Name[MAX_FILE_NAME_SIZE + 1];
  byte                  Version;
  TYPE_File            *f, *fRec;
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
  f = TAP_Hdd_Fopen(Name);
  if(!f)
  {
    HDD_TAP_PopDir();
    return;
  }

  SegmentMarker[NrSegmentMarker - 1].Selected = FALSE;
  if(NrSegmentMarker < 3)SegmentMarker[0].Selected = FALSE;

  TAP_Hdd_Fwrite(&Version, sizeof(byte), 1, f);
  TAP_Hdd_Fwrite(&FileSize, sizeof(long64), 1, f);
  TAP_Hdd_Fwrite(&NrSegmentMarker, sizeof(int), 1, f);
  TAP_Hdd_Fwrite(&ActiveSegment, sizeof(int), 1, f);
  TAP_Hdd_Fwrite(SegmentMarker, sizeof(tSegmentMarker), NRSEGMENTMARKER, f);
  TAP_Hdd_Fclose(f);
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
  int                   i;
  char                  StartTime[12], EndTime[12], Time[24];
  dword                 fw;
  dword                 C1, C2;

  C1 = COLOR_Yellow;
  C2 = COLOR_White;

  if(rgnSegmentList)
  {
    TAP_Osd_PutGd(rgnSegmentList, 0, 0, &_SegmentList_Background_Gd, FALSE);
    FM_PutString(rgnSegmentList, 5, 3, 100, LangGetString(LS_Segments), COLOR_White, COLOR_None, &Calibri_12_FontData, TRUE, ALIGN_LEFT);
    TAP_Osd_PutGd(rgnSegmentList, 62, 23, &_Button_Up_Gd, TRUE);
    TAP_Osd_PutGd(rgnSegmentList, 88, 23, &_Button_Down_Gd, TRUE);

    if(NrSegmentMarker > 2)
    {
      if(ActiveSegment >= NrSegmentMarker) ActiveSegment = NrSegmentMarker - 1;
      for(i = 0; i < NrSegmentMarker - 1; i++)
      {
        if(i == ActiveSegment)
        {
          if((SegmentMarker[i + 1].Time - SegmentMarker[i].Time) < 61)
            TAP_Osd_PutGd(rgnSegmentList, 3, 44 + 28*i, &_Selection_Red_Gd, FALSE);
          else
            TAP_Osd_PutGd(rgnSegmentList, 3, 44 + 28*i, &_Selection_Blue_Gd, FALSE);
        }

        SecToTimeString(SegmentMarker[i].Time - SegmentMarker[0].Time, StartTime);
        SecToTimeString(SegmentMarker[i + 1].Time - SegmentMarker[0].Time, EndTime);
        TAP_SPrint(Time, "%s-%s", StartTime, EndTime);
        fw = FM_GetStringWidth(Time, &Calibri_12_FontData);
        FM_PutString(rgnSegmentList, 58 - (fw >> 1), 45 + 28*i, 250, Time, (SegmentMarker[i].Selected ? C1 : C2), COLOR_None, &Calibri_12_FontData, TRUE, ALIGN_LEFT);
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
  dword                 p = 0;
  dword                 x1, x2;
  dword                 totalBlock;
  static dword          NextDraw = 0;
  static dword          Lastp = 999;

  totalBlock = PlayInfo.totalBlock;
  if(rgnInfo)
  {
    if((TAP_GetTick() > NextDraw) || Force)
    {
      if(totalBlock) p = (dword)((float)PlayInfo.currentBlock * 653 / totalBlock);

      if(Force || (p != Lastp))
      {
        Lastp = p;

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
            p = (dword)((float)SegmentMarker[i].Block * 653 / totalBlock);
          else
            p = 0;
          TAP_Osd_PutGd(rgnInfo, 31 + p, 93, &_SegmentMarker_Gd, TRUE);
        }

        //Bookmarks: 0% = 31/112, 100% = 683/112
        for(i = 0; i < NrBookmarks; i++)
        {
          if(totalBlock)
            p = (dword)((float)Bookmarks[i] * 653 / totalBlock);
          else
            p = 0;
          TAP_Osd_PutGd(rgnInfo, 31 + p, 112, &_BookmarkMarker_Gd, TRUE);
        }

        //Draw the current position
        //0% = X34, 100% = X686
        for(y = 102; y < 112; y++)
          TAP_Osd_PutPixel(rgnInfo, 34 + Lastp, y, COLOR_DarkRed);

        TAP_Osd_Sync();
      }

      NextDraw = TAP_GetTick() + 20;
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
  char                  TimeString[20];
  char                  PercentString[20];
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
      strcat(TimeString, PercentString);
      TAP_Osd_FillBox(rgnInfo, 60, 48, 283, 31, RGB(30, 30, 30));
      fw = FM_GetStringWidth(TimeString, &Calibri_14_FontData);
      FM_PutString(rgnInfo, 200 - (fw >> 1), 52, 660, TimeString, COLOR_White, COLOR_None, &Calibri_14_FontData, TRUE, ALIGN_LEFT);
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
  char                  Time[20];

  if(rgnInfo)
  {
    TAP_Osd_FillBox(rgnInfo, 507, 8, 50, 35, RGB(51, 51, 51));
    if(MinuteJump)
    {
      TAP_Osd_PutGd(rgnInfo, 507, 8, &_Button_Left_Gd, TRUE);
      TAP_Osd_PutGd(rgnInfo, 507 + 1 + _Button_Left_Gd.width, 8, &_Button_Right_Gd, TRUE);

      TAP_SPrint(Time, "%d'", (int)(MinuteJump / 60));

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
      if(Appl_SetPlaybackSpeed) Appl_SetPlaybackSpeed(3, TrickModeSpeed, TRUE);
      TrickMode = TM_Slow;
      break;
    }

    case TM_Rwd:
    {
      if(TrickModeSpeed > 1)
      {
        // 64xRWD down to 2xRWD
        TrickModeSpeed--;
        if(Appl_SetPlaybackSpeed) Appl_SetPlaybackSpeed(2, TrickModeSpeed, TRUE);
      }
      else
      {
        // 1/16xFWD
        TrickModeSpeed = 4;
        if(Appl_SetPlaybackSpeed) Appl_SetPlaybackSpeed(3, TrickModeSpeed, TRUE);
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
        if(Appl_SetPlaybackSpeed) Appl_SetPlaybackSpeed(3, TrickModeSpeed, TRUE);
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
        if(Appl_SetPlaybackSpeed) Appl_SetPlaybackSpeed(1, TrickModeSpeed, TRUE);
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
  if(Appl_SetPlaybackSpeed) Appl_SetPlaybackSpeed(0, 1, TRUE);
  TrickMode = TM_Play;
  TrickModeSpeed = 0;
}

void Playback_Pause(void)
{
  if(Appl_SetPlaybackSpeed) Appl_SetPlaybackSpeed(4, 0, TRUE);
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
  if(Appl_SetPlaybackSpeed) Appl_SetPlaybackSpeed(1, TrickModeSpeed, TRUE);
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
  if(Appl_SetPlaybackSpeed) Appl_SetPlaybackSpeed(2, TrickModeSpeed, TRUE);
  TrickMode = TM_Rwd;
}

void Playback_Slow(void)
{
  //Appl_SetPlaybackSpeed(3, 1, true) 1/2x Slow
  //Appl_SetPlaybackSpeed(3, 2, true) 1/4x Slow
  //Appl_SetPlaybackSpeed(3, 3, true) 1/8x Slow
  //Appl_SetPlaybackSpeed(3, 4, true) 1/16x Slow

  if(TrickModeSpeed < 4) TrickModeSpeed++;
  if(Appl_SetPlaybackSpeed) Appl_SetPlaybackSpeed(3, TrickModeSpeed, TRUE);
  TrickMode = TM_Slow;
}

void Playback_JumpForward(void)
{
  float                 CurrentTime, JumpTime;
  dword                 JumpBlock;

  CurrentTime = (float)(60 * PlayInfo.duration + PlayInfo.durationSec) * PlayInfo.currentBlock / PlayInfo.totalBlock;
  JumpTime = CurrentTime + MinuteJump;
  JumpBlock = (dword)(JumpTime * PlayInfo.totalBlock / (60 * PlayInfo.duration + PlayInfo.durationSec));
  if(JumpBlock > BlockNrLast10Seconds) JumpBlock = BlockNrLast10Seconds;

  if(TrickMode == TM_Pause) Playback_Normal();
  TAP_Hdd_ChangePlaybackPos(JumpBlock);
}

void Playback_JumpBackward(void)
{
  float                 CurrentTime, JumpTime;
  dword                 JumpBlock;

  CurrentTime = (float)(60 * PlayInfo.duration + PlayInfo.durationSec) * PlayInfo.currentBlock / PlayInfo.totalBlock;
  JumpTime = CurrentTime - MinuteJump;
  if(JumpTime < 0) JumpTime = 0;
  JumpBlock = (dword)(JumpTime * PlayInfo.totalBlock / (60 * PlayInfo.duration + PlayInfo.durationSec));

  if(TrickMode == TM_Pause) Playback_Normal();
  TAP_Hdd_ChangePlaybackPos(JumpBlock);
}

void Playback_JumpNextBookmark(void)
{
  int i;

  for(i = 0; i < NrBookmarks; i++)
  {
    if(PlayInfo.currentBlock < Bookmarks[i])
    {
      if(TrickMode != TM_Play) Playback_Normal();
      TAP_Hdd_ChangePlaybackPos(Bookmarks[i]);
      break;
    }
  }
}

void Playback_JumpPrevBookmark(void)
{
  int                   i;
  dword                 ThirtySeconds;

  ThirtySeconds = PlayInfo.totalBlock * 30 / (60*PlayInfo.duration + PlayInfo.durationSec);

  for(i = NrBookmarks - 1; i >= 0; i--)
  {
    if(Bookmarks[i] < (PlayInfo.currentBlock - ThirtySeconds))
    {
      if(TrickMode != TM_Play) Playback_Normal();
      TAP_Hdd_ChangePlaybackPos(Bookmarks[i]);
      break;
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
      case MI_DeleteOddSegments:  FM_PutString(rgnActionMenu, 20, 4 + 28 * i, 300, LangGetString(LS_DeleteOddSegments), (ActionMenuItem == MI_DeleteOddSegments ? C1 : C2), COLOR_None, &Calibri_14_FontData, TRUE, ALIGN_LEFT); break;
      case MI_DeleteEvenSegments: FM_PutString(rgnActionMenu, 20, 4 + 28 * i, 300, LangGetString(LS_DeleteEvenSegments), (ActionMenuItem == MI_DeleteEvenSegments ? C1 : C2), COLOR_None, &Calibri_14_FontData, TRUE, ALIGN_LEFT); break;
      case MI_RemovePadding:      FM_PutString(rgnActionMenu, 20, 4 + 28 * i, 300, LangGetString(LS_RemovePadding), (ActionMenuItem == MI_RemovePadding ? C1 : (NrSegmentMarker == 4 ? C2 : C4)), COLOR_None, &Calibri_14_FontData, TRUE, ALIGN_LEFT); break;
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

void MovieCutterDeleteOddSegments(void)
{
  int                   i;

  for(i = 0; i < NRSEGMENTMARKER; i++)
    SegmentMarker[i].Selected = ((i & 1) == 0);

  MovieCutterProcess(TRUE, FALSE);
}

void MovieCutterDeleteEvenSegments(void)
{
  int                   i;

  for(i = 0; i < NRSEGMENTMARKER; i++)
    SegmentMarker[i].Selected = ((i & 1) == 1);

  MovieCutterProcess(TRUE, FALSE);
}

void MovieCutterProcess(bool KeepSource, bool KeepCut)
{
  int                   i, j;
  dword                 SelectedBlock;
  dword                 DeltaTime, DeltaBlock;
  bool                  isMultiSelect;
  int                   WorkingSegment;
  bool                  ret;

  NoPlaybackCheck = TRUE;
  isMultiSelect = FALSE;
  for(i = 0; i < NRSEGMENTMARKER; i++)
    if(SegmentMarker[i].Selected)
    {
      isMultiSelect = TRUE;
      break;
    }

  //If one or more segments have been selected, work with them.
  //If no segment has been selected, use the active segment and break the loop
  CutDumpList();
  //ClearOSD();

  for(i = NrSegmentMarker - 1; i >= 0; i--)
  {
    //OSDMenuProgressBarShow(PROGRAM_NAME, LangGetString(LS_Cutting), NrSegmentMarker - i - 1, NrSegmentMarker, NULL);

    if(isMultiSelect)
      WorkingSegment = i;
    else
      WorkingSegment = ActiveSegment;

    if(!isMultiSelect || SegmentMarker[i].Selected)
    {
      TAP_SPrint(LogString, "Processing segment %d", WorkingSegment);
      WriteLogMC(PROGRAM_NAME, LogString);
      ret = MovieCutter(PlaybackName, SegmentMarker[WorkingSegment].Block, SegmentMarker[WorkingSegment + 1].Block, KeepSource, KeepCut);
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

      TAP_SPrint(LogString, "Reported new totalBlock = %d", PlayInfo.totalBlock);
      WriteLogMC(PROGRAM_NAME, LogString);
      for(j = NrSegmentMarker - 1; j > 0; j--)
      {
        if(SegmentMarker[j].Block > SelectedBlock)
        {
          SegmentMarker[j].Block -= DeltaBlock;
          SegmentMarker[j].Time -= DeltaTime;
        }

        if(j == NrSegmentMarker - 1) SegmentMarker[j].Block = PlayInfo.totalBlock;
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

void MovieCutterDeletePadding(void)
{
  if(NrSegmentMarker == 4)
  {
    SegmentMarker[0].Selected = TRUE;
    SegmentMarker[1].Selected = FALSE;
    SegmentMarker[2].Selected = TRUE;
    SegmentMarker[3].Selected = FALSE;
    OSDRedrawEverything();
    TAP_Osd_Sync();
    MovieCutterProcess(TRUE, FALSE);
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
