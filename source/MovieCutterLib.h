#ifndef __MOVIECUTTERLIB__
#define __MOVIECUTTERLIB__

#define INFSIZE               499572
#define NAVRECS_SD              2000
#define NAVRECS_HD              1000
#define BLOCKSIZE               9024
#define CUTPOINTSEARCHRADIUS    9024   // in both directions
#define PACKETSIZE               192   // number of bytes to compare (in fact the actual packet size doesn't matter)
//#define TEMPCUTNAME        "__tempcut__.ts"

typedef enum
{
  RC_Error,
  RC_Warning,
  RC_Ok
}tResultCode;

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
tResultCode MovieCutter(char *SourceFileName, char *CutFileName, tTimeStamp *CutStartPoint, tTimeStamp *BehindCutPoint, bool KeepCut, bool isHD);
void        GetNextFreeCutName(char const *SourceFileName, char *CutFileName, word LeaveNamesOut);
void        SecToTimeString(dword Time, char *const TimeString);     // needs max. 4 + 1 + 2 + 1 + 2 + 1 = 11 chars
void        MSecToTimeString(dword Timems, char *const TimeString);  // needs max. 4 + 1 + 2 + 1 + 2 + 1 + 3 + 1 = 15 chars
void        Print64BitLong(long64 Number, char *const OutString);    // needs max. 2 + 2*9 + 1 = 19 chars
bool        isCrypted(char const *SourceFileName);
bool        isHDVideo(char const *SourceFileName, bool *const isHD);
bool        isNavAvailable(char const *SourceFileName);
bool        GetRecDateFromInf(char const *FileName, dword *const DateTime);
long64      HDD_GetFileSize(char const *FileName);
bool        HDD_SetFileDateTime(char const *Directory, char const *FileName, dword NewDateTime);
tTimeStamp* NavLoad(char const *SourceFileName, dword *const NrTimeStamps, bool isHDVideo);


//These will prevent the compiler from complaining
//We've to use the 64-bit version of lstat() else it will fail with files >= 4GB
typedef struct
{
  __dev_t               st_dev;
  long int              st_pad1[2];
  __ino64_t             st_ino;
  __mode_t              st_mode;
  __nlink_t             st_nlink;
  __uid_t               st_uid;
  __gid_t               st_gid;
  __dev_t               st_rdev;
  long int              st_pad2[2];
  __off64_t             st_size;
  __time_t              st_atime;
  long int              __reserved0;
  __time_t              st_mtime;
  long int              __reserved1;
  __time_t              st_ctime;
  long int              __reserved2;
  __blksize_t           st_blksize;
  long int              st_pad4;
  __blkcnt64_t          st_blocks;
  long int              st_pad5[14];
} tstat64;

extern int lstat64(__const char *__restrict __file, tstat64 *__restrict __buf) __THROW;
int fseeko64 (FILE *__stream, __off64_t __off, int __whence);
__off64_t ftello64(FILE *__stream);

#endif
