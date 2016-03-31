#ifndef __MOVIECUTTERLIB__
#define __MOVIECUTTERLIB__

//#define MC_UNICODE      TRUE
#define MC_MULTILANG    TRUE
                                      // MinEncode   Header   MinInf    NormInf    MaxInf
//#define INFSIZE                 2128  //    8192  // 2128  // 3320   // 132636  // 499572
#define NAVRECS_SD              1024
#define BLOCKSIZE               9024
#define INFFILETAG              "MCCut"
//#define TEMPCUTNAME             "__tempcut__.ts"

#ifndef FULLDEBUG
//  #define FULLDEBUG             TRUE  // ***
#endif


typedef enum
{
  RC_Error,
  RC_Warning,
  RC_Ok
} tResultCode;

typedef struct
{
  dword                 BlockNr;
  dword                 Timems;
//  byte                  FrameType;
} tTimeStamp;

typedef struct
{
  dword                 SHOffset:24; // = (FrameType shl 24) or SHOffset
  dword                 FrameType:8;
  byte                  MPEGType;
  byte                  FrameIndex;
  word                  iFrameSeqOffset;
//  byte                  Zero1;
  dword                 PHOffsetHigh;
  dword                 PHOffset;

  dword                 PTS2;
  dword                 NextPH;
  dword                 Timems;
  dword                 Zero5;
} tnavSD;

typedef struct
{
  dword                 SEIPPS:24;
  dword                 FrameType:8;
  byte                  MPEGType;
  byte                  FrameIndex;
  word                  PPSLen;
//  byte                  Zero1;
  dword                 SEIOffsetHigh;
  dword                 SEIOffsetLow;

  dword                 SEIPTS;
  dword                 NextAUD;
  dword                 Timems;
  dword                 Zero2;

  dword                 SEISPS;
  dword                 SPSLen;
  dword                 IFrame;
  dword                 Zero4;

  dword                 Zero5;
  dword                 Zero6;
  dword                 Zero7;
  dword                 Zero8;
} tnavHD;


extern int  CUTPOINTSEARCHRADIUS;

void        CreateSettingsDir(void);
tResultCode MovieCutter(char *SourceFileName, char *CutFileName, char *AbsDirectory, tTimeStamp *CutStartPoint, tTimeStamp *BehindCutPoint, bool KeepCut, bool isHD, char *pCutCaption, char *pSourceCaption);
void        GetNextFreeCutName(const char *SourceFileName, char *const OutCutFileName, const char *AbsDirectory, int LeaveNamesOut);
void        SecToTimeString(dword Time, char *const OutTimeString);     // needs max. 4 + 1 + 2 + 1 + 2 + 1 = 11 chars
void        MSecToTimeString(dword Timems, char *const OutTimeString);  // needs max. 4 + 1 + 2 + 1 + 2 + 1 + 3 + 1 = 15 chars
dword       TimeStringToMSec(char *const TimeString);
void        Print64BitLong(long64 Number, char *const OutString);       // needs max. 2 + 2*9 + 1 = 19 chars
int         GetPacketSize(const char *RecFileName, const char *AbsDirectory);
//off_t       FindNextIFrame(const char *RecFileName, const char *AbsDirectory, dword BlockNr, bool isHD);
bool        isNavAvailable(const char *RecFileName, const char *AbsDirectory);
// bool        SaveBookmarksToInf(const char *RecFileName, const char *AbsDirectory, const dword Bookmarks[], int NrBookmarks);
tTimeStamp* NavLoad(const char *RecFileName, const char *AbsDirectory, int *const OutNrTimeStamps, bool isHD);


static inline dword CalcBlockSize(off_t Size)
{
  // Workaround für die Division durch BLOCKSIZE (9024)
  // Primfaktorenzerlegung: 9024 = 2^6 * 3 * 47
  // max. Dateigröße: 256 GB (dürfte reichen...)
  return (dword)(Size >> 6) / 141;
}

/* int fseeko64 (FILE *__stream, __off64_t __off, int __whence);
__off64_t ftello64(FILE *__stream); */

#endif
