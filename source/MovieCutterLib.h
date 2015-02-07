#ifndef __MOVIECUTTERLIB__
#define __MOVIECUTTERLIB__
                                      // MinEncode   Header   MinInf    NormInf    MaxInf
#define INFSIZE                 2128  //    8192  // 2128  // 3320   // 132636  // 499572
#define NAVRECS_SD              2000
#define NAVRECS_HD              1000
#define BLOCKSIZE               9024
//#define TEMPCUTNAME        "__tempcut__.ts"

#ifndef FULLDEBUG
  #define FULLDEBUG             TRUE  // ***
#endif

//#define STACKTRACE            TRUE


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
} tTimeStamp;

typedef struct
{
  dword                 SHOffset; // = (FrameType shl 24) or SHOffset
  byte                  MPEGType;
  byte                  FrameIndex;
  byte                  Field5;
  byte                  Zero1;
  dword                 PHOffsetHigh;
  dword                 PHOffset;
  dword                 PTS2;
  dword                 NextPH;
  dword                 Timems;
  dword                 Zero5;
} tnavSD;

typedef struct
{
  dword                 LastPPS;
  byte                  MPEGType;
  byte                  FrameIndex;
  byte                  PPS_FirstSEI;
  byte                  Zero1;
  dword                 SEIOffsetHigh;
  dword                 SEIOffsetLow;
  dword                 PTS2;
  dword                 NextAUD;
  dword                 Timems;
  dword                 Zero2;
  dword                 LastSPS;
  dword                 PictFormat;
  dword                 IFrame;
  dword                 Zero4;
  dword                 Zero5;
  dword                 Zero6;
  dword                 Zero7;
  dword                 Zero8;
} tnavHD;

void        WriteLogMC(char *ProgramName, char *s);
void        WriteLogMCf(char *ProgramName, const char *format, ...);
void        WriteDebugLog(const char *format, ...);
tResultCode MovieCutter(char *SourceFileName, char *CutFileName, char *AbsDirectory, tTimeStamp *CutStartPoint, tTimeStamp *BehindCutPoint, bool KeepCut, bool isHD);
void        GetNextFreeCutName(const char *SourceFileName, char *const OutCutFileName, const char *AbsDirectory, int LeaveNamesOut);
void        SecToTimeString(dword Time, char *const TimeString);     // needs max. 4 + 1 + 2 + 1 + 2 + 1 = 11 chars
void        MSecToTimeString(dword Timems, char *const TimeString);  // needs max. 4 + 1 + 2 + 1 + 2 + 1 + 3 + 1 = 15 chars
void        Print64BitLong(long64 Number, char *const OutString);    // needs max. 2 + 2*9 + 1 = 19 chars
int         GetPacketSize(const char *RecFileName);
bool        isCrypted(const char *RecFileName, const char *AbsDirectory);
bool        isHDVideo(const char *RecFileName, const char *AbsDirectory, bool *const isHD);
bool        isNavAvailable(const char *RecFileName, const char *AbsDirectory);
bool        GetRecDateFromInf(const char *RecFileName, const char *AbsDirectory, dword *const DateTime);
// bool        SaveBookmarksToInf(const char *RecFileName, const char *AbsDirectory, const dword Bookmarks[], int NrBookmarks);
tTimeStamp* NavLoad(const char *RecFileName, const char *AbsDirectory, int *const OutNrTimeStamps, bool isHD);


int fseeko64 (FILE *__stream, __off64_t __off, int __whence);
__off64_t ftello64(FILE *__stream);

#endif
