#ifndef __MOVIECUTTERLIB__
#define __MOVIECUTTERLIB__

#define TIMECODEARRAYSIZE  94         // 94 packets = 2 blocks a 47 packets (9024 Bytes per block, 192 Bytes per packet)
#define TEMPCUTNAME        "__tempcut__.ts"
//#define FULLDEBUG        1

typedef struct
{
  dword                 BlockNr;
  dword                 Timems;
}tTimeStamp;

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

bool        MovieCutter(char *SourceFileName, dword CutPointA, dword CutPointB, bool KeepSource, bool KeepCut, unsigned int LeaveNamesOut);
void        WriteLogMC(char *ProgramName, char *s);
void        SecToTimeString(dword Time, char *TimeString);
void        MSecToTimeString(dword Timems, char *TimeString);
bool        isCrypted(char *SourceFileName);
bool        isHDVideo(char *SourceFileName, bool *isHD);
bool        isNavAvailable(char *SourceFileName);
dword       NavGetBlockTimeStamp(dword PlaybackBlockNr);
tTimeStamp* NavLoad(char *SourceFileName, dword *NrTimeStamps, bool isHDVideo);
tTimeStamp* NavLoadSD(char *SourceFileName, dword *NrTimeStamps);
tTimeStamp* NavLoadHD(char *SourceFileName, dword *NrTimeStamps);

//old
bool        GetPCRPID(char *FileName, word *PCRPID);
bool        GetPCR(char *FileName, dword Block, int PacketSize, word PCRPID, dword *PCR);
dword       DeltaPCR(dword FirstPCR, dword SecondPCR);
bool        is192ByteTS(char *FileName);


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
