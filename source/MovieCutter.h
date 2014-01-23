#ifndef __MOVIECUTTERH__
#define __MOVIECUTTERH__

#define PROGRAM_NAME          "MovieCutter"
#define VERSION               "V2.0g RC"  // alpha = α,  beta = β, � = ü
#define TAPID                 0x8E0A4247
//#define AUTHOR                "FireBird / Christian W�nsch"
#define AUTHOR                "FireBird / C. Wünsch"
#define DESCRIPTION           "MovieCutter"

#define NRSEGMENTMARKER       101            // max. number of file markers +1 (marker for the end of file)
#define NRBOOKMARKS           144
#define CUTFILEVERSION        2
#define LOGDIR                "/ProgramFiles/Settings/MovieCutter"
#define LNGFILENAME           PROGRAM_NAME ".lng"
#define INIFILENAME           PROGRAM_NAME ".ini"

int fseeko64 (FILE *__stream, __off64_t __off, int __whence);

int   TAP_Main(void);
dword TAP_EventHandler(word event, dword param1, dword param2);
void  ActionMenuDown(void);
void  ActionMenuDraw(void);
void  ActionMenuRemove(void);
void  ActionMenuUp(void);
bool  AddBookmark(dword newBlock);
void  AddBookmarksToSegmentList(void);
bool  AddDefaultSegmentMarker(void);
bool  AddSegmentMarker(dword newBlock, bool RejectSmallSegments);
void  CalcLastSeconds(void);
void  CheckLastSeconds(void);
void  Cleanup(bool DoClearOSD);
void  CleanupCut(void);
void  CreateOSD(void);
void  CreateSettingsDir(void);
void  ClearOSD(bool EnterNormal);
void  CutDumpList(void);
void  CutFileDelete(void);
bool  CutFileLoad(void);
void  CutFileSave(void);
bool  DeleteBookmark(word BookmarkIndex);
void  DeleteAllBookmarks(void);
bool  DeleteSegmentMarker(word MarkerIndex);
void  DeleteAllSegmentMarkers(void);
void  ExportSegmentsToBookmarks(void);
int   FindNearestBookmark(void);
int   FindNearestSegmentMarker(void);
dword NavGetBlockTimeStamp(dword PlaybackBlockNr);
bool  isPlaybackRunning(void);
void  LoadINI(void);
bool  MoveBookmark(dword newBlock);
bool  MoveSegmentMarker(dword newBlock);
void  MovieCutterDeleteFile(void);
void  MovieCutterDeleteSegments(void);
void  MovieCutterSelectOddSegments(void);
void  MovieCutterSelectEvenSegments(void);
void  MovieCutterUnselectAll(void);
void  MovieCutterProcess(bool KeepCut);
void  MovieCutterSaveSegments(void);
void  OSDInfoDrawBackground(void);
void  OSDInfoDrawBookmarkMode(void);
void  OSDInfoDrawClock(bool Force);
void  OSDInfoDrawCurrentPosition(bool Force);
void  OSDInfoDrawMinuteJump(void);
void  OSDInfoDrawPlayIcons(bool Force);
void  OSDInfoDrawProgressbar(bool Force);
void  OSDInfoDrawRecName(void);
void  OSDRedrawEverything(void);
void  OSDSegmentListDrawList();
void  Playback_Faster(void);
void  Playback_FFWD(void);
void  Playback_JumpBackward(void);
void  Playback_JumpForward(void);
void  Playback_JumpNextSegment(void);
void  Playback_JumpPrevSegment(void);
void  Playback_JumpNextBookmark(void);
void  Playback_JumpPrevBookmark(void);
void  Playback_Normal(void);
void  Playback_Pause(void);
void  Playback_RWD(void);
void  Playback_Slow(void);
void  Playback_Slower(void);
byte  PlaybackRepeatMode(bool ChangeMode, byte RepeatMode, dword RepeatStartBlock, dword RepeatEndBlock);
bool  PlaybackRepeatSet(bool EnableRepeatAll);
bool  PlaybackRepeatGet();
bool  ReadBookmarks(void);
bool  SaveBookmarks(void);
void  SaveINI(void);
void  SelectSegmentMarker(void);
void  SetCurrentSegment(void);
void  ShowConfirmationDialog(char* MessageStr);
void  ShowErrorMessage(char* MessageStr);
dword TMSCommander_handler(dword param1);
bool PatchOldNavFile(char *SourceFileName, bool isHD);

#endif
