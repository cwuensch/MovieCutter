#ifndef __MOVIECUTTERH__
#define __MOVIECUTTERH__

#define PROGRAM_NAME          "MovieCutter"
#define VERSION               "V3.5 beta"  // alpha = α,  beta = β, � = ü
#define TAPID                 0x8E0A4247
//#define AUTHOR                "FireBird / Christian W�nsch"
#define AUTHOR                "FireBird / C. Wünsch"
#define DESCRIPTION           "Allows cutting of recorded TV programmes."

#define NRSEGMENTMARKER       101            // max. number of file markers +1 (marker for the end of file)
//#define NRBOOKMARKS           144
#define NRUNDOEVENTS          100
#define SIZESUSPECTHDDS       1024
#define LOGDIR                "/ProgramFiles/Settings/MovieCutter"
#define LNGFILENAME           PROGRAM_NAME ".lng"
#define INIFILENAME           PROGRAM_NAME ".ini"

typedef struct
{
  dword                 Block;  //Block nr
  dword                 Timems; //Time in ms
  float                 Percent;
  bool                  Selected;
} tSegmentMarker;


int fseeko64 (FILE *__stream, __off64_t __off, int __whence);

int   TAP_Main(void);
dword TAP_EventHandler(word event, dword param1, dword param2);
static dword TMSCommander_handler(dword param1);

static bool  ActionMenuItemInactive(int MenuItem);
static void  ActionMenuDown(void);
static void  ActionMenuDraw(void);
static void  ActionMenuRemove(void);
static void  ActionMenuUp(void);
static int   AddBookmark(dword newBlock, bool RejectSmallScenes);
static bool  AddDefaultSegmentMarker(void);
static int   AddSegmentMarker(dword newBlock, bool RejectSmallSegments);
static void  CalcLastSeconds(void);
static bool  CheckFileSystem(char *MountPath, dword ProgressStart, dword ProgressEnd, dword ProgressMax, bool DoFix, bool Quick, bool NoOkInfo, bool ErrorMessage, int SuspectFiles, char *InodeNrs);
static void  CheckLastSeconds(void);
static void  Cleanup(bool DoClearOSD);
static void  CleanupCut(void);
static void  CreateOSD(void);
static void  ClearOSD(bool EnterNormal);
static void  CountSelectedSegments();
static void  CutDumpList(void);
static bool  CutDecodeFromBM(void);
static bool  CutEncodeToBM(tSegmentMarker SegmentMarker[], int NrSegmentMarker, dword Bookmarks[], int NrBookmarks);
static void  CutFileDelete(void);
static bool  CutFileDecodeBin(FILE *fCut, __off64_t *OutSavedSize);
static bool  CutFileDecodeTxt(FILE *fCut, __off64_t *OutSavedSize);
static bool  CutFileLoad(void);
static bool  CutFileSave(void);
static bool  CutFileSave2(tSegmentMarker SegmentMarker[], int NrSegmentMarker, const char* RecFileName);
static bool  CutSaveToBM(bool ReadBMBefore);
static bool  CutSaveToInf(tSegmentMarker SegmentMarker[], int NrSegmentMarker, const char* RecFileName);
static bool  DeleteBookmark(int BookmarkIndex);
static bool  DeleteAllBookmarks(void);
static bool  DeleteSegmentMarker(int MarkerIndex);
static void  DeleteAllSegmentMarkers(void);
static void  ExportSegmentsToBookmarks(void);
static int   FindNearestBookmark(dword curBlock);
static int   FindNearestSegmentMarker(dword curBlock);
static int   FindSegmentWithBlock(dword curBlock);
static void  ImportBookmarksToSegments(void);
static bool  isLargeSegment(dword SegStartBlock, dword SegEndBlock, bool isLastSegment, bool KeepCut);
static bool  isPlaybackRunning(void);
static void  LoadINI(void);
static bool  MoveBookmark(int BookmarkIndex, dword newBlock, bool RejectSmallScenes);
static bool  MoveSegmentMarker(int MarkerIndex, dword newBlock, bool RejectSmallSegments);
static void  MovieCutterDeleteFile(void);
static void  MovieCutterDeleteSegments(void);
static bool  MovieCutterRenameFile(void);
static void  MovieCutterSelectEvOddSegments(void);
static void  MovieCutterSplitMovie(void);
static void  MovieCutterUnselectAll(void);
static void  MovieCutterProcess(bool KeepCut, bool SplitHere);
static void  MovieCutterSaveSegments(void);
static dword NavGetBlockTimeStamp(dword PlaybackBlockNr);
static void  OSDInfoDrawBackground(void);
static void  OSDInfoDrawBookmarkMode(bool DoSync);
static void  OSDInfoDrawClock(bool Force);
static void  OSDInfoDrawCurrentPlayTime(bool Force);
static void  OSDInfoDrawMinuteJump(bool DoSync);
static void  OSDInfoDrawPlayIcons(bool Force, bool DoSync);
static void  OSDInfoDrawProgressbar(bool Force, bool DoSync);
static void  OSDInfoDrawRecName(void);
static void  OSDRedrawEverything(void);
static void  OSDSegmentListDrawList(bool DoSync);
static void  OSDTextStateWindow(int MessageID);
static void  Playback_Faster(void);
static void  Playback_FFWD(void);
static void  Playback_SetJumpNavigate(bool pJumpRequest, bool pNavRequest, bool pBackwards);
static void  Playback_JumpBackward(void);
static void  Playback_JumpForward(void);
static void  Playback_JumpNextSegment(void);
static void  Playback_JumpPrevSegment(void);
static void  Playback_JumpNextBookmark(void);
static void  Playback_JumpPrevBookmark(void);
static void  Playback_Normal(void);
static void  Playback_Pause(void);
static void  Playback_RWD(void);
static void  Playback_Slow(void);
static void  Playback_Slower(void);
static void  SaveINI(void);
static bool  SelectSegmentMarker(void);
static void  SetCurrentSegment(void);
static bool  SetPlaybackSpeed(TYPE_TrickMode newTrickMode, byte newTrickModeSpeed);
static bool  ShowConfirmationDialog(char *MessageStr);
static void  ShowErrorMessage(char *MessageStr, char *TitleStr);
static void  UndoAddEvent(bool Bookmark, dword PreviousBlock, dword NewBlock, bool SegmentWasSelected);
static bool  UndoLastAction(void);
static void  UndoResetStack(void);
static bool  PatchOldNavFile(const char *RecFileName, const char *AbsDirectory, bool isHD);

//extern void  OSDMenuFreeStdFonts(void);

#endif
