#ifndef __MOVIECUTTERH__
#define __MOVIECUTTERH__

#define PROGRAM_NAME          "MovieCutter"
#define VERSION               "V3.0 RC3"  // alpha = α,  beta = β, � = ü
#define TAPID                 0x8E0A4247
//#define AUTHOR                "FireBird / Christian W�nsch"
#define AUTHOR                "FireBird / C. Wünsch"
#define DESCRIPTION           "MovieCutter"

#define NRSEGMENTMARKER       101            // max. number of file markers +1 (marker for the end of file)
//#define NRBOOKMARKS           144
#define NRUNDOEVENTS          100
#define CUTFILEVERSION        2
#define LOGDIR                "/ProgramFiles/Settings/MovieCutter"
#define LNGFILENAME           PROGRAM_NAME ".lng"
#define INIFILENAME           PROGRAM_NAME ".ini"
#define FSCKPATH              TAPFSROOT "/ProgramFiles"

int fseeko64 (FILE *__stream, __off64_t __off, int __whence);

int   TAP_Main(void);
dword TAP_EventHandler(word event, dword param1, dword param2);
void  ActionMenuDown(void);
void  ActionMenuDraw(void);
void  ActionMenuRemove(void);
void  ActionMenuUp(void);
int   AddBookmark(dword newBlock, bool RejectSmallScenes);
bool  AddDefaultSegmentMarker(void);
int   AddSegmentMarker(dword newBlock, bool RejectSmallSegments);
void  CalcLastSeconds(void);
bool  CheckFileSystem(dword ProgressStart, dword ProgressEnd, dword ProgressMax, bool DoFix, bool Quick, bool NoOkInfo, unsigned int SuspectFiles, char *InodeNrs);
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
bool  DeleteBookmark(int BookmarkIndex);
bool  DeleteAllBookmarks(void);
bool  DeleteSegmentMarker(int MarkerIndex);
void  DeleteAllSegmentMarkers(void);
void  ExportSegmentsToBookmarks(void);
int   FindNearestBookmark(dword curBlock);
int   FindNearestSegmentMarker(dword curBlock);
void  ImportBookmarksToSegments(void);
bool  isPlaybackRunning(void);
void  LoadINI(void);
bool  MoveBookmark(int BookmarkIndex, dword newBlock, bool RejectSmallScenes);
bool  MoveSegmentMarker(int MarkerIndex, dword newBlock, bool RejectSmallSegments);
void  MovieCutterDeleteFile(void);
void  MovieCutterDeleteSegments(void);
void  MovieCutterSelectOddSegments(void);
void  MovieCutterSelectEvenSegments(void);
void  MovieCutterUnselectAll(void);
void  MovieCutterProcess(bool KeepCut);
void  MovieCutterSaveSegments(void);
dword NavGetBlockTimeStamp(dword PlaybackBlockNr);
void  OSDInfoDrawBackground(void);
void  OSDInfoDrawBookmarkMode(bool DoSync);
void  OSDInfoDrawClock(bool Force);
void  OSDInfoDrawCurrentPlayTime(bool Force);
void  OSDInfoDrawMinuteJump(bool DoSync);
void  OSDInfoDrawPlayIcons(bool Force, bool DoSync);
void  OSDInfoDrawProgressbar(bool Force, bool DoSync);
void  OSDInfoDrawRecName(void);
void  OSDRedrawEverything(void);
void  OSDSegmentListDrawList(bool DoSync);
void  OSDTextStateWindow(int MessageID);
void  Playback_Faster(void);
void  Playback_FFWD(void);
void  Playback_SetJumpNavigate(bool pJumpRequest, bool pNavRequest, bool pBackwards);
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
//bool  ReadBookmarks(void);
char* RemoveEndLineBreak (char *const Text);
//bool  SaveBookmarks(void);
void  SaveINI(void);
bool  SelectSegmentMarker(void);
void  SetCurrentSegment(void);
bool  ShowConfirmationDialog(char *MessageStr);
void  ShowErrorMessage(char *MessageStr, char *TitleStr);
dword TMSCommander_handler(dword param1);
void  UndoAddEvent(bool Bookmark, dword PreviousBlock, dword NewBlock, bool SegmentWasSelected);
bool  UndoLastAction(void);
void  UndoResetStack(void);
bool  PatchOldNavFile(const char *RecFileName, const char *AbsDirectory, bool isHD);
extern void OSDMenuFreeStdFonts(void);

#endif
