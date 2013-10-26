#ifndef __MOVIECUTTERH__
#define __MOVIECUTTERH__

#define PROGRAM_NAME          "MovieCutter"
#define VERSION               "V1.5"
#define TAPID                 0x8E0A4247
#define AUTHOR                "FireBird"
#define DESCRIPTION           "MovieCutter"

#define NRSEGMENTMARKER       14            // max. number of file markers +1 (marker for the end of file)
#define NRBOOKMARKS           144
#define RECBUFFERENTRIES      3000
#define CUTFILEVERSION        1
#define LOGDIR                "/ProgramFiles/Settings/MovieCutter"
#define LNGFILENAME           PROGRAM_NAME ".lng"
#define INIFILENAME           PROGRAM_NAME ".ini"

int   TAP_Main(void);
dword TAP_EventHandler(word event, dword param1, dword param2);
void  ActionMenuDown(void);
void  ActionMenuDraw(void);
void  ActionMenuRemove(void);
void  ActionMenuUp(void);
bool  AddBookmark(dword Block);
void  AddBookmarksToSegmentList(void);
void  AddDefaultSegmentMarker(void);
bool  AddSegmentMarker(dword Block);
void  Calc10Seconds(void);
void  CheckLast10Seconds(void);
void  Cleanup(void);
void  CleanupCut(void);
void  CreateOSD(void);
void  CreateSettingsDir(void);
void  ClearOSD(void);
void  CutDumpList(void);
void  CutFileDelete(void);
bool  CutFileLoad(void);
void  CutFileSave(void);
void  DeleteBookmark(int BookmarkIndex);
void  DeleteSegmentMarker(int MarkerIndex);
void  DeleteAllSegmentMarkers(void);
int   FindNearestBookmark(void);
int   FindNearestSegmentMarker(void);
void  HookBookmarkFunction(bool SetHook);
bool  Hooked_Appl_SetBookmark(void);
bool  isNavAvailable(void);
bool  isCrypted(void);
bool  isHDVideo(void);
bool  isPlaybackRunning(void);
void  LoadINI(void);
void  MoveBookmark(dword Block);
void  MoveSegmentMarker(void);
void  MovieCutterDeleteFile(void);
void  MovieCutterSelectPadding(void);
void  MovieCutterDeleteSegments(void);
void  MovieCutterSelectOddSegments(void);
void  MovieCutterSelectEvenSegments(void);
void  MovieCutterProcess(bool KeepSource, bool KeepCut);
void  MovieCutterSaveSegments(void);
dword NavGetBlockTimeStamp(dword PlaybackBlockNr);
bool  NavLoad(void);
bool  NavLoadSD(void);
bool  NavLoadHD(void);
void  OSDInfoDrawBackground(void);
void  OSDInfoDrawClock(bool Force);
void  OSDInfoDrawCurrentPosition(bool Force);
void  OSDInfoDrawMinuteJump(void);
void  OSDInfoDrawPlayIcons(bool Force);
void  OSDInfoDrawProgressbar(bool Force);
void  OSDInfoDrawRecName(void);
void  OSDRedrawEverything(void);
void  OSDSegmentListDrawList(void);
void  Playback_Faster(void);
void  Playback_FFWD(void);
void  Playback_JumpBackward(void);
void  Playback_JumpForward(void);
void  Playback_JumpNextBookmark(void);
void  Playback_JumpPrevBookmark(void);
void  Playback_Normal(void);
void  Playback_Pause(void);
void  Playback_RWD(void);
void  Playback_Slow(void);
void  Playback_Slower(void);
void  ReadBookmarks(void);
void  SaveBookmarks(void);
void  SaveINI(void);
void  SecToTimeString(dword Time, char *TimeString);
void  SelectSegmentMarker(void);
void  SetCurrentSegment(void);
bool  TimeCodeCheck(void);
dword TMSCommander_handler(dword param1);

#endif
