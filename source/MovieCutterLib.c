#define _FILE_OFFSET_BITS  64
#define __USE_LARGEFILE64  1

#include                <stdio.h>
#include                <stdlib.h>
#include                <string.h>
#include                <utime.h>
#include                "tap.h"
#include                "libFireBird.h"
#include                "MovieCutterLib.h"

//Time code pointers:
//P1 = last packet in the source file before the cut
//P2 = first packet of the cut file
//P3 = requested cut in
//P4 = requested cut out
//P5 = last packet of the cut file
//P6 = first packet in the source file after the cut

typedef struct
{
  off_t                 OrigFilePos;
  byte                  TSPacket[192];
} tTimeCodeArray;

typedef enum
{
  RC_Ok,
  RC_Warning,
  RC_Error
}tResultCode;

bool        FillTimeCodeArray(char const *SourceFileName, dword Block, int PacketSize, tTimeCodeArray *TimeCodeArray);
bool        FileCut(char *SourceFileName, dword StartBlock, dword EndBlock);
void        GetNextFreeCutName(char const *SourceFileName, char *CutFileName, unsigned int LeaveNamesOut);
bool        GetFirstAndLastTSPacket(char const *FileName, int PacketSize, byte *FirstTSPacket, byte *LastTSPacket);
tResultCode PatchInfFiles(char *SourceFileName, char *CutFileName, dword CutPointA, dword CutPointB, int PacketSize);
bool        PatchNavFiles(char *SourceFileName, char *CutFileName, tTimeCodeArray *P2, tTimeCodeArray *P5, tTimeCodeArray *P6);
bool        PatchNavFilesSD(char *SourceFileName, char *CutFileName, tTimeCodeArray *P2, tTimeCodeArray *P5, tTimeCodeArray *P6);
bool        PatchNavFilesHD(char *SourceFileName, char *CutFileName, tTimeCodeArray *P2, tTimeCodeArray *P5, tTimeCodeArray *P6);
void        SecToTimeStringMC(dword Time, char *TimeString);
dword       GetTimeStampFromInf(char *FileName);
bool        HDD_RepairTimeStamp(char *Directory, char *FileName, dword NewTimeStamp);

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
  dword                 Time;
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

char                    LogString[512];
word                    PCRPID;


void WriteLogMC(char *ProgramName, char *s)
{
  static bool FirstCall = TRUE;

  HDD_TAP_PushDir();

  if(FirstCall)
  {
    HDD_ChangeDir("/ProgramFiles");
    if(!TAP_Hdd_Exist("Settings")) TAP_Hdd_Create("Settings", ATTR_FOLDER);
    HDD_ChangeDir("Settings");
    if(!TAP_Hdd_Exist("MovieCutter")) TAP_Hdd_Create("MovieCutter", ATTR_FOLDER);
    FirstCall = FALSE;
  }

  TAP_Hdd_ChangeDir("/ProgramFiles/Settings/MovieCutter");
  LogEntry("MovieCutter.log", ProgramName, TRUE, TIMESTAMP_YMDHMS, s);
  HDD_TAP_PopDir();
}

bool MovieCutter(char *SourceFileName, dword CutPointA, dword CutPointB, bool KeepSource, bool KeepCut, unsigned int LeaveNamesOut)
{
  char                  CutFileName[MAX_FILE_NAME_SIZE + 1];
  char                  FileName[MAX_FILE_NAME_SIZE + 1];
  tTimeCodeArray        TimeCodeArrayA[TIMECODEARRAYSIZE], TimeCodeArrayB[TIMECODEARRAYSIZE];
  tTimeCodeArray       *P1, *P2, *P3, *P4, *P5, *P6;
  byte                  FirstTSPacket[192], LastTSPacket[192];
  off_t                 CutA, CutB;
  int                   i;
  char                  CurrentDir[512];
  __off64_t             SourceSize;
  int                   PacketSize;
  dword                 TimeStamp;
  tResultCode           ResultCode;
  bool                  SuppressNavGeneration;

  PacketSize = is192ByteTS(SourceFileName) ? 192 : 188;
  SuppressNavGeneration = FALSE;

  WriteLogMC("MovieCutterLib", "----------------------------------------");
  TAP_SPrint(LogString, "Source      = '%s'", SourceFileName);
  WriteLogMC("MovieCutterLib", LogString);

  HDD_TAP_GetCurrentDir(CurrentDir);
  if(!HDD_GetFileSizeAndInode(CurrentDir, SourceFileName, NULL, &SourceSize))
  {
    WriteLogMC("MovieCutterLib", "MovieCutter() E0001: cut file not created.");
    return FALSE;
  }

  TAP_SPrint(LogString, "Size        = %d blocks", (dword)(SourceSize / 9024));
  WriteLogMC("MovieCutterLib", LogString);
  TAP_SPrint(LogString, "Dir         = '%s'", CurrentDir);
  WriteLogMC("MovieCutterLib", LogString);
  TAP_SPrint(LogString, "Packet size = %d", PacketSize);
  WriteLogMC("MovieCutterLib", LogString);

  TAP_SPrint(LogString, "CutPointA   = %d, CutPointB = %d, KeepSource = %s, KeepCut = %s", CutPointA, CutPointB, KeepSource ? "yes" : "no", KeepCut ? "yes" : "no");
  WriteLogMC("MovieCutterLib", LogString);

  if(!FillTimeCodeArray(SourceFileName, CutPointA, PacketSize, TimeCodeArrayA))
  {
    WriteLogMC("MovieCutterLib", "MovieCutter() W0002: nav creation suppressed.");
    SuppressNavGeneration = TRUE;
  }

  if(!FillTimeCodeArray(SourceFileName, CutPointB, PacketSize, TimeCodeArrayB))
  {
    WriteLogMC("MovieCutterLib", "MovieCutter() W0003: nav creation suppressed.");
    SuppressNavGeneration = TRUE;
  }

  if(!FileCut(SourceFileName, CutPointA, CutPointB))
  {
    WriteLogMC("MovieCutterLib", "MovieCutter() E0004");
    return FALSE;
  }

  //Rename the newly created cut file
  GetNextFreeCutName(SourceFileName, CutFileName, LeaveNamesOut);
  TAP_SPrint(LogString, "Cut name    = '%s'", CutFileName);
  WriteLogMC("MovieCutterLib", LogString);
  TAP_Hdd_Rename(TEMPCUTNAME, CutFileName);

  //Get the first and last TS packet from the cut file
  if(!GetFirstAndLastTSPacket(CutFileName, PacketSize, FirstTSPacket, LastTSPacket))
  {
    WriteLogMC("MovieCutterLib", "MovieCutter() W0005: nav creation suppressed.");
    SuppressNavGeneration = TRUE;
  }

  //Copy the inf file and patch both play lengths
  ResultCode = PatchInfFiles(SourceFileName, CutFileName, CutPointA, CutPointB, PacketSize);

  //Assign all time code pointers needed for the .nav calculations
  if(!SuppressNavGeneration)
  {
    CutA = (off_t)CutPointA * 9024;
    CutB = (off_t)CutPointB * 9024;
    P2 = NULL;
    P3 = NULL;
    P4 = NULL;
    P5 = NULL;
    for(i = 0; i < TIMECODEARRAYSIZE; i++)
    {
      if(memcmp(&TimeCodeArrayA[i].TSPacket, FirstTSPacket, PacketSize) == 0) P2 = &TimeCodeArrayA[i];
      if(memcmp(&TimeCodeArrayB[i].TSPacket, LastTSPacket, PacketSize) == 0)  P5 = &TimeCodeArrayB[i];
      if(TimeCodeArrayA[i].OrigFilePos == CutA) P3 = &TimeCodeArrayA[i];
      if(TimeCodeArrayB[i].OrigFilePos == CutB) P4 = &TimeCodeArrayB[i];
    }
    if(!P2 || !P3 || !P4 || !P5)
    {
#ifdef FULLDEBUG
      TAP_PrintNet("%s%s%s%s\n", !P2 ? "P2 " : "", !P3 ? "P3 " : "", !P4 ? "P4 " : "", !P5 ? "P5 " : "");
#endif
      WriteLogMC("MovieCutterLib", "MovieCutter() W0006: nav creation suppressed.");
      SuppressNavGeneration = TRUE;
    }
    P1 = P2 - 1;
    P6 = P5 + 1;
  }

  if(!SuppressNavGeneration)
  {
    TAP_SPrint(LogString, "Cut offset: P3 - P2 = %d packets, P4 - P6 = %d packets", (int)(P3->OrigFilePos - P2->OrigFilePos)/PacketSize, (int)(P4->OrigFilePos - P6->OrigFilePos)/PacketSize);
    WriteLogMC("MovieCutterLib", LogString);

    if(!PatchNavFiles(SourceFileName, CutFileName, P2, P5, P6))
    {
      WriteLogMC("MovieCutterLib", "MovieCutter() W0007: nav creation suppressed.");
      SuppressNavGeneration = TRUE;
    }
  }

  if(SuppressNavGeneration)
  {
    TAP_SPrint(FileName, "%s.nav", SourceFileName);
    TAP_Hdd_Delete(FileName);
    TAP_SPrint(FileName, "%s.nav", CutFileName);
    TAP_Hdd_Delete(FileName);
  }

  //Fix the time stamps of all involved files
  //Source
  TimeStamp = GetTimeStampFromInf(SourceFileName);
  HDD_RepairTimeStamp(CurrentDir, SourceFileName, TimeStamp);
  TAP_SPrint(FileName, "%s.inf", SourceFileName);
  HDD_RepairTimeStamp(CurrentDir, FileName, TimeStamp);
  TAP_SPrint(FileName, "%s.nav", SourceFileName);
  HDD_RepairTimeStamp(CurrentDir, FileName, TimeStamp);

  //Cut
  HDD_RepairTimeStamp(CurrentDir, CutFileName, TimeStamp);
  TAP_SPrint(FileName, "%s.inf", CutFileName);
  HDD_RepairTimeStamp(CurrentDir, FileName, TimeStamp);
  TAP_SPrint(FileName, "%s.nav", CutFileName);
  HDD_RepairTimeStamp(CurrentDir, FileName, TimeStamp);

  if(!KeepSource) HDD_Delete(SourceFileName);
  if(!KeepCut) HDD_Delete(CutFileName);

  WriteLogMC("MovieCutterLib", "MovieCutter() finished.");
  return TRUE;
}


bool FillTimeCodeArray(char const *SourceFileName, dword Block, int PacketSize, tTimeCodeArray *TimeCodeArray)
{
  char                  AbsFileName[512];
  char                  CurrentDir[512];
  FILE                 *f;
  off_t                 FileOffset;
  size_t                ret;
  int                   i;

  #ifdef FULLDEBUG
    WriteLogMC("MovieCutterLib", "FillTimeCodeArray()");
  #endif

  //Open the rec for read access
  HDD_TAP_GetCurrentDir(CurrentDir);
  TAP_SPrint(AbsFileName, "%s%s/%s", TAPFSROOT, CurrentDir, SourceFileName);
  f = fopen(AbsFileName, "r");
  if(f == 0)
  {
    WriteLogMC("MovieCutterLib", "FillTimeCodeArray() E0101.");
    return FALSE;
  }

  //Read the 2 blocks from the rec
  FileOffset = (off_t)(Block ? Block - 1 : 0) * 9024;
  fseeko64(f, FileOffset, SEEK_SET);
  for(i = 0; i < TIMECODEARRAYSIZE; i++)
  {
    TimeCodeArray[i].OrigFilePos = FileOffset + PacketSize*i;

    ret = fread(&TimeCodeArray[i].TSPacket, PacketSize, 1, f);
    if(ret != 1)
    {
      fclose(f);
      WriteLogMC("MovieCutterLib", "FillTimeCodeArray() E0102.");
      return FALSE;
    }
  }
  fclose(f);

  return TRUE;
}

bool FileCut(char *SourceFileName, dword StartBlock, dword EndBlock)
{
  tDirEntry             FolderStruct, *pFolderStruct;
  dword                 x;
  dword                 ret;
  TYPE_PlayInfo         PlayInfo;
  char                  CurrentDir[512], TAPDir[512];

  #ifdef FULLDEBUG
    WriteLogMC("MovieCutterLib", "FileCut()");
  #endif

  //Initialize the directory structure
  memset(&FolderStruct, 0, sizeof(tDirEntry));
  FolderStruct.Magic = 0xbacaed31;
  HDD_TAP_GetCurrentDir(TAPDir);

  TAP_Hdd_Delete(TEMPCUTNAME);

  //Save the current directory resources and change into our directory (current directory of the TAP)
  ApplHdd_SaveWorkFolder();
  strcpy(CurrentDir, &TAPFSROOT[1]); //do not include the leading slash
  HDD_TAP_GetCurrentDir(&CurrentDir[strlen(CurrentDir)]);
  ApplHdd_SelectFolder(&FolderStruct, CurrentDir);
  DevHdd_DeviceOpen(&pFolderStruct, &FolderStruct);
  ApplHdd_SetWorkFolder(&FolderStruct);

  //If a playback is running, stop it
  TAP_Hdd_GetPlayInfo(&PlayInfo);
  if(PlayInfo.playMode == PLAYMODE_Playing)
  {
    Appl_StopPlaying();
    Appl_WaitEvt(0xE507, &x, 1, 0xFFFFFFFF, 300);
  }

  //Do the cutting
  ret = ApplHdd_FileCutPaste(SourceFileName, StartBlock, EndBlock - StartBlock, TEMPCUTNAME);

  //Restore all resources
  DevHdd_DeviceClose(&pFolderStruct);
  ApplHdd_RestoreWorkFolder();

  TAP_Hdd_ChangeDir(TAPDir);

  if(ret)
  {
    WriteLogMC("MovieCutterLib", "FileCut() E020a. Cut file not created.");
    return FALSE;
  }

  return TRUE;
}

void GetNextFreeCutName(char const *SourceFileName, char *CutFileName, unsigned int LeaveNamesOut)
{
  int                   NameLen;
  int                   i;
  char                  NextFileName[MAX_FILE_NAME_SIZE + 1];

  #ifdef FULLDEBUG
    WriteLogMC("MovieCutterLib", "GetNextFreeCutName()");
  #endif

  NameLen = strlen(SourceFileName) - 4;  // ".rec" entfernen

  i = 0;
  do
  {
    i++;
    strncpy(NextFileName, SourceFileName, NameLen);
    strncpy(CutFileName, SourceFileName, NameLen);
    TAP_SPrint(&NextFileName[NameLen], " (Cut-%d)%s", i, &SourceFileName[NameLen]);
    TAP_SPrint(&CutFileName[NameLen], " (Cut-%d)%s", i+LeaveNamesOut, &SourceFileName[NameLen]);
  }while(TAP_Hdd_Exist(NextFileName) || TAP_Hdd_Exist(CutFileName));
}

bool GetFirstAndLastTSPacket(char const *FileName, int PacketSize, byte *FirstTSPacket, byte *LastTSPacket)
{
  char                  AbsFileName[512];
  char                  CurrentDir[512];
  FILE                 *f;
  size_t                ret;

  #ifdef FULLDEBUG
    WriteLogMC("MovieCutterLib", "GetFirstAndLastTSPacket()");
  #endif

  //Open the rec for read access
  HDD_TAP_GetCurrentDir(CurrentDir);
  TAP_SPrint(AbsFileName, "%s%s/%s", TAPFSROOT, CurrentDir, FileName);
  f = fopen(AbsFileName, "r");
  if(f == 0)
  {
    WriteLogMC("MovieCutterLib", "GetFirstAndLastTSPacket() E0301.");
    return FALSE;
  }

  //Get the first TS packet
  if(FirstTSPacket)
  {
    ret = fread(FirstTSPacket, PacketSize, 1, f);
    if(!ret)
    {
      fclose(f);
      WriteLogMC("MovieCutterLib", "GetFirstAndLastTSPacket() E0302.");
      return FALSE;
    }
  }

  //Seek to the last TS packet
  if(LastTSPacket)
  {
    fseeko64(f, -PacketSize, SEEK_END);

    //Read the last TS packet
    ret = fread(LastTSPacket, PacketSize, 1, f);
    fclose(f);
    if(!ret)
    {
      WriteLogMC("MovieCutterLib", "GetFirstAndLastTSPacket() E0303.");
      return FALSE;
    }
  }

  return TRUE;
}

tResultCode PatchInfFiles(char *SourceFileName, char *CutFileName, dword CutPointA, dword CutPointB, int PacketSize)
{
  char                  InfFileName[MAX_FILE_NAME_SIZE + 1];
  char                  CurrentDir[512];
  char                  T1[20], T2[20], T3[20];
  tRECHeaderInfo        RECHeaderInfo;
  TYPE_File            *f;
  byte                 *Buffer;
  dword                 fs;
  dword                 SourcePlayTime, CutPlayTime;
  __off64_t             SourceSize, CutSize;
  dword                 SourceBlocks, CutBlocks;
  dword                *Bookmarks = NULL;
  int                   i;
  dword                 PCRA, PCRB;
  bool                  NoBlockInfo;
  tResultCode           ResultCode = RC_Ok;

  #define INFSIZE       499572

  #ifdef FULLDEBUG
    WriteLogMC("MovieCutterLib", "PatchInfFiles()");
  #endif

  //Get the file size of the source and cut files (info or linear timing mode)
  NoBlockInfo = FALSE;
  HDD_TAP_GetCurrentDir(CurrentDir);

  if(HDD_GetFileSizeAndInode(CurrentDir, SourceFileName, NULL, &SourceSize))
  {
    SourceBlocks = (dword)(SourceSize / 9024);
  }
  else
  {
    WriteLogMC("MovieCutterLib", "PatchInfFiles() W0401: no source block reporting.");
    ResultCode = RC_Warning;
    SourceBlocks = 0;
    NoBlockInfo = TRUE;
  }

  if(HDD_GetFileSizeAndInode(CurrentDir, CutFileName, NULL, &CutSize))
  {
    CutBlocks = (dword)(CutSize / 9024);
  }
  else
  {
    WriteLogMC("MovieCutterLib", "PatchInfFiles() W0402: no cut block reporting.");
    ResultCode = RC_Warning;
    CutBlocks = 0;
    NoBlockInfo = TRUE;
  }

  TAP_SPrint(LogString, "New size: Cut = %d blocks, Source = %d blocks", CutBlocks, SourceBlocks);
  WriteLogMC("MovieCutterLib", LogString);

  //Read the source .inf
  TAP_SPrint(InfFileName, "%s.inf", SourceFileName);
  f = TAP_Hdd_Fopen(InfFileName);
  if(!f)
  {
    WriteLogMC("MovieCutterLib", "PatchInfFiles() E0403: source inf not patched, cut inf not created.");
    return RC_Error;
  }

  fs = TAP_Hdd_Flen(f);
  Buffer = TAP_MemAlloc(INFSIZE);
  if(!Buffer)
  {
    TAP_Hdd_Fclose(f);
    WriteLogMC("MovieCutterLib", "PatchInfFiles() E0404: source inf not patched, cut inf not created.");
    return RC_Error;
  }
  TAP_Hdd_Fread(Buffer, fs, 1, f);
  TAP_Hdd_Fclose(f);

  //Decode the source .inf
  if(!HDD_DecodeRECHeader(Buffer, &RECHeaderInfo, ST_UNKNOWN))
  {
    TAP_MemFree(Buffer);
    WriteLogMC("MovieCutterLib", "PatchInfFiles() E0405: source inf not patched, cut inf not created.");
    return RC_Error;
  }

  //Calculate the play times using the PCRs
  SourcePlayTime =  60 * RECHeaderInfo.HeaderDuration + RECHeaderInfo.HeaderDurationSec;

  //Copy the orig inf so that GetPCR has access to the PCR PID
  //Abuse the log string buffer
  TAP_SPrint(LogString, "cp \"%s%s/%s.inf\" \"%s%s/%s.inf\"", TAPFSROOT, CurrentDir, SourceFileName, TAPFSROOT, CurrentDir, CutFileName);
  system(LogString);

  if(!GetPCR(CutFileName, 0, PacketSize, PCRPID, &PCRA))
  {
    if(NoBlockInfo)
    {
      WriteLogMC("MovieCutterLib", "PatchInfFiles() E0406: source inf not patched, cut inf not created.");
      TAP_MemFree(Buffer);
      return RC_Error;
    }
    else
    {
      WriteLogMC("MovieCutterLib", "PatchInfFiles() W0407: using linear timing mode.");
      ResultCode = RC_Warning;
    }

    //Guess the source and cut play times according to the size of the files
    PCRA = 0;
    PCRB = (dword)((float)SourcePlayTime * CutBlocks * 1000 / (CutBlocks + SourceBlocks));
  }
  else if(!GetPCR(CutFileName, CutPointB - CutPointA, PacketSize, PCRPID, &PCRB))
  {
    if(NoBlockInfo)
    {
      WriteLogMC("MovieCutterLib", "PatchInfFiles() E0408: source inf not patched, cut inf not created.");
      TAP_MemFree(Buffer);
      return RC_Error;
    }
    else
    {
      WriteLogMC("MovieCutterLib", "PatchInfFiles() W0409: using linear timing mode.");
      ResultCode = RC_Warning;
    }

    PCRA = 0;
    PCRB = (dword)((float)SourcePlayTime * CutBlocks * 1000 / (CutBlocks + SourceBlocks));
  }

  CutPlayTime = (dword)(DeltaPCR(PCRA, PCRB) / 1000);

  //Check if CutPlayTime <= 6h, else use linear timing mode
  if(CutPlayTime > 21600)
  {
    WriteLogMC("MovieCutterLib", "PatchInfFiles() W040a: using linear timing mode.");
    ResultCode = RC_Warning;
    PCRA = 0;
    PCRB = (dword)((float)SourcePlayTime * CutBlocks * 1000 / (CutBlocks + SourceBlocks));
    CutPlayTime = (dword)(DeltaPCR(PCRA, PCRB) / 1000);
  }

  SecToTimeStringMC(SourcePlayTime, T1);
  SecToTimeStringMC(CutPlayTime, T2);
  SecToTimeStringMC(SourcePlayTime - CutPlayTime, T3);
  TAP_SPrint(LogString, "Playtimes: Orig = %s, Cut = %s, New = %s", T1, T2, T3);
  WriteLogMC("MovieCutterLib", LogString);

  SourcePlayTime -= CutPlayTime;

  //Change the new source play time
  RECHeaderInfo.HeaderDuration = (word)(SourcePlayTime / 60);
  RECHeaderInfo.HeaderDurationSec = SourcePlayTime % 60;

  //Save all bookmarks to a temporary array
  Bookmarks = TAP_MemAlloc(177 * sizeof(dword));
  if(Bookmarks)
  {
    memcpy(Bookmarks, RECHeaderInfo.Bookmark, 177 * sizeof(dword));
    TAP_SPrint(LogString, "Bookmarks: ");
    for(i = 0; i < 177; i++)
    {
      if(Bookmarks[i] == 0) break;
      TAP_SPrint(&LogString[strlen(LogString)], "%d ", Bookmarks[i]);
    }
    WriteLogMC("MovieCutterLib", LogString);
  }

  //Clear all source Bookmarks
  memset(RECHeaderInfo.Bookmark, 0, 177 * sizeof(dword));
  RECHeaderInfo.NrBookmarks = 0;
  RECHeaderInfo.Resume = 0;

  //Copy all bookmarks which are < CutPointA or >= CutPointB
  //Move the second group by (CutPointB - CutPointA)
  if(Bookmarks)
  {
    TAP_SPrint(LogString, "Bookmarks->Source: ");
    for(i = 0; i < 177; i++)
    {
      if(Bookmarks[i] == 0) break;

      if(Bookmarks[i] < CutPointA || Bookmarks[i] >= CutPointB)
      {
        if(Bookmarks[i] < CutPointB)
          RECHeaderInfo.Bookmark[RECHeaderInfo.NrBookmarks] = Bookmarks[i];
        else
          RECHeaderInfo.Bookmark[RECHeaderInfo.NrBookmarks] = Bookmarks[i] - (CutPointB - CutPointA);
        TAP_SPrint(&LogString[strlen(LogString)], "%d ", Bookmarks[i]);
        RECHeaderInfo.NrBookmarks++;
      }
    }
    WriteLogMC("MovieCutterLib", LogString);
  }

  //Encode and write the modified source inf
  if(HDD_EncodeRECHeader(Buffer, &RECHeaderInfo, ST_UNKNOWN))
  {
    f = TAP_Hdd_Fopen(InfFileName);
    if(f)
    {
      TAP_Hdd_Fseek(f, 0, SEEK_SET);
      TAP_Hdd_Fwrite(Buffer, fs, 1, f);
      TAP_Hdd_Fclose(f);
    }
    else
    {
      WriteLogMC("MovieCutterLib", "PatchInfFiles() W040b: source inf not patched.");
    }
  }
  else
  {
    WriteLogMC("MovieCutterLib", "PatchInfFiles() W040c: source inf not patched.");
    ResultCode = RC_Warning;
  }

  //Open the cut inf
  TAP_SPrint(InfFileName, "%s.inf", CutFileName);
  if(!TAP_Hdd_Exist(InfFileName)) TAP_Hdd_Create(InfFileName, ATTR_NORMAL);
  f = TAP_Hdd_Fopen(InfFileName);
  if(!f)
  {
    WriteLogMC("MovieCutterLib", "PatchInfFiles() E040d: cut inf not created.");
    TAP_MemFree(Buffer);
    return RC_Error;
  }

  //Set the length of the cut file
  RECHeaderInfo.HeaderDuration = (word)(CutPlayTime / 60);
  RECHeaderInfo.HeaderDurationSec = CutPlayTime % 60;

  //Clear all source Bookmarks
  memset(RECHeaderInfo.Bookmark, 0, 177 * sizeof(dword));
  RECHeaderInfo.NrBookmarks = 0;

  //Copy all bookmarks which are >= CutPointA and < CutPointB
  //Move them by CutPointA
  if(Bookmarks)
  {
    TAP_SPrint(LogString, "Bookmarks->Cut: ");
    for(i = 0; i < 177; i++)
    {
      if(Bookmarks[i] == 0) break;

      if(Bookmarks[i] >= CutPointA && Bookmarks[i] < CutPointB)
      {
        RECHeaderInfo.Bookmark[RECHeaderInfo.NrBookmarks] = Bookmarks[i] - CutPointA;
        TAP_SPrint(&LogString[strlen(LogString)], "%d ", Bookmarks[i]);
        RECHeaderInfo.NrBookmarks++;
      }
    }
    WriteLogMC("MovieCutterLib", LogString);
  }

  //Encode the cut inf and write it to the disk
  if(HDD_EncodeRECHeader(Buffer, &RECHeaderInfo, ST_UNKNOWN))
  {
    TAP_Hdd_Fwrite(Buffer, fs, 1, f);
  }
  else
  {
    WriteLogMC("MovieCutterLib", "PatchInfFiles() E040e: cut inf not created.");
    ResultCode = RC_Error;
  }

  TAP_Hdd_Fclose(f);
  TAP_MemFree(Buffer);
  if(Bookmarks) TAP_MemFree(Bookmarks);

  return ResultCode;
}

bool PatchNavFiles(char *SourceFileName, char *CutFileName, tTimeCodeArray *P2, tTimeCodeArray *P5, tTimeCodeArray *P6)
{
  bool                  ret;
  byte                 *Buffer;
  bool                  isHD = FALSE;
  char                  InfFileName[MAX_FILE_NAME_SIZE + 1];
  TYPE_File            *f;
  dword                 fs;
  tRECHeaderInfo        RECHeaderInfo;

  #ifdef FULLDEBUG
    WriteLogMC("MovieCutterLib", "PatchNavFiles()");
  #endif

  //Read the source .inf and check if this is a HD recording
  TAP_SPrint(InfFileName, "%s.inf", SourceFileName);
  f = TAP_Hdd_Fopen(InfFileName);
  if(f)
  {
    fs = TAP_Hdd_Flen(f);
    Buffer = TAP_MemAlloc(fs);
    if(Buffer)
    {
      TAP_Hdd_Fread(Buffer, fs, 1, f);
      TAP_Hdd_Fclose(f);
      if(HDD_DecodeRECHeader(Buffer, &RECHeaderInfo, ST_UNKNOWN))
      {
        isHD = (RECHeaderInfo.SIVideoStreamType == STREAM_VIDEO_MPEG4_PART2) ||
               (RECHeaderInfo.SIVideoStreamType == STREAM_VIDEO_MPEG4_H263)  ||
               (RECHeaderInfo.SIVideoStreamType == STREAM_VIDEO_MPEG4_H264);
      }
            else
      {
        WriteLogMC("MovieCutterLib", "PatchNavFiles() E0503.");
        TAP_MemFree(Buffer);
        return FALSE;
      }
      TAP_MemFree(Buffer);
    }
    else
    {
      TAP_Hdd_Fclose(f);
      WriteLogMC("MovieCutterLib", "PatchNavFiles() E0502.");
      return FALSE;
    }
  }
  else
  {
    WriteLogMC("MovieCutterLib", "PatchNavFiles() E0501.");
    return FALSE;
  }

  if(isHD)
    ret = PatchNavFilesHD(SourceFileName, CutFileName, P2, P5, P6);
  else
    ret = PatchNavFilesSD(SourceFileName, CutFileName, P2, P5, P6);

  return ret;
}

bool PatchNavFilesSD(char *SourceFileName, char *CutFileName, tTimeCodeArray *P2, tTimeCodeArray *P5, tTimeCodeArray *P6)
{
  TYPE_File            *fCutNav, *fSourceNavNew;
  char                  FileName[MAX_FILE_NAME_SIZE + 1];
  char                  FileName2[MAX_FILE_NAME_SIZE + 1];
  off_t                 PictureHeaderOffset;
  tnavSD               *navSource, *navSourceNew, *navCut;
  int                   navsRead, navRecsSourceNew, navRecsCut, i;
  bool                  IFrameSource, IFrameCut;
  char                  CurrentDir[512];
  char                  AbsFileName[512];
  FILE                 *fSourceNav;

  #define NAVRECS       1000

  #ifdef FULLDEBUG
    WriteLogMC("MovieCutterLib", "PatchNavFilesSD()");
  #endif

  //Open the original nav
  HDD_TAP_GetCurrentDir(CurrentDir);
  TAP_SPrint(AbsFileName, "%s%s/%s.nav", TAPFSROOT, CurrentDir, SourceFileName);
  fSourceNav = fopen(AbsFileName, "r");
  if(fSourceNav == NULL)
  {
    WriteLogMC("MovieCutterLib", "PatchNavFilesSD() E0601.");
    return FALSE;
  }

  //Create and open the cut nav
  TAP_SPrint(FileName, "%s.nav", CutFileName);
  TAP_Hdd_Delete(FileName);
  TAP_Hdd_Create(FileName, ATTR_NORMAL);
  fCutNav = TAP_Hdd_Fopen(FileName);
  if(!fCutNav)
  {
    fclose(fSourceNav);
    WriteLogMC("MovieCutterLib", "PatchNavFilesSD() E0602.");
    return FALSE;
  }

  //Create and open the new source nav
  TAP_SPrint(FileName, "%s.new.nav", SourceFileName);
  TAP_Hdd_Delete(FileName);
  TAP_Hdd_Create(FileName, ATTR_NORMAL);
  fSourceNavNew = TAP_Hdd_Fopen(FileName);
  if(!fSourceNavNew)
  {
    fclose(fSourceNav);
    TAP_Hdd_Fclose(fCutNav);
    WriteLogMC("MovieCutterLib", "PatchNavFilesSD() E0603.");
    return FALSE;
  }

  //Loop through the nav
  navSource = TAP_MemAlloc(NAVRECS * sizeof(tnavSD));
  navSourceNew = TAP_MemAlloc(NAVRECS * sizeof(tnavSD));
  navCut = TAP_MemAlloc(NAVRECS * sizeof(tnavSD));

  navRecsSourceNew = 0;
  navRecsCut = 0;

  IFrameSource = FALSE;
  IFrameCut = FALSE;
  while(TRUE)
  {
    navsRead = fread(navSource, sizeof(tnavSD), NAVRECS, fSourceNav);
    if(navsRead == 0) break;

    for(i = 0; i < navsRead; i++)
    {
      //Check if the entry lies between points P2 and P5
      PictureHeaderOffset = ((off_t)(navSource[i].PHOffsetHigh) << 32) | navSource[i].PHOffset;
      if((PictureHeaderOffset >= P2->OrigFilePos) && (PictureHeaderOffset <= P5->OrigFilePos))
      {
        //nav entries for the cut file
        if(navRecsCut >= NAVRECS)
        {
          TAP_Hdd_Fwrite(navCut, sizeof(tnavSD), navRecsCut, fCutNav);
          navRecsCut = 0;
        }

        //Subtract (P2 - 1) from the cut .nav PH address
        PictureHeaderOffset -= P2->OrigFilePos;

        if((navSource[i].SHOffset >> 24) == 1) IFrameSource = TRUE;
        if(IFrameSource)
        {
          memcpy(&navCut[navRecsCut], &navSource[i], sizeof(tnavSD));
          navCut[navRecsCut].PHOffsetHigh = PictureHeaderOffset >> 32;
          navCut[navRecsCut].PHOffset = PictureHeaderOffset & 0xffffffff;
          navRecsCut++;
        }
        IFrameCut = FALSE;
      }
      else
      {
        //nav entries for the source file
        if(navRecsSourceNew >= NAVRECS)
        {
          TAP_Hdd_Fwrite(navSourceNew, sizeof(tnavSD), navRecsSourceNew, fSourceNavNew);
          navRecsSourceNew = 0;
        }

        if((navSource[i].SHOffset >> 24) == 1) IFrameCut = TRUE;
        if(IFrameCut)
        {
          memcpy(&navSourceNew[navRecsSourceNew], &navSource[i], sizeof(tnavSD));

          //if ph offset > P5, subtract (P6 - P2)
          if(PictureHeaderOffset > P5->OrigFilePos)
          {
            PictureHeaderOffset -= (P6->OrigFilePos - P2->OrigFilePos);
            navSourceNew[navRecsSourceNew].PHOffsetHigh = PictureHeaderOffset >> 32;
            navSourceNew[navRecsSourceNew].PHOffset = PictureHeaderOffset & 0xffffffff;
          }
          navRecsSourceNew++;
        }
        IFrameSource = FALSE;
      }
    }
  }

  if(navRecsCut > 0) TAP_Hdd_Fwrite(navCut, sizeof(tnavSD), navRecsCut, fCutNav);
  if(navRecsSourceNew > 0) TAP_Hdd_Fwrite(navSourceNew, sizeof(tnavSD), navRecsSourceNew, fSourceNavNew);

  fclose(fSourceNav);
  TAP_Hdd_Fclose(fCutNav);
  TAP_Hdd_Fclose(fSourceNavNew);

  TAP_MemFree(navSource);
  TAP_MemFree(navSourceNew);
  TAP_MemFree(navCut);

  //Delete the orig source nav and make the new source nav the active one
  TAP_SPrint(FileName, "%s.nav", SourceFileName);
  TAP_Hdd_Delete(FileName);

  TAP_SPrint(FileName2, "%s.new.nav", SourceFileName);
  TAP_Hdd_Rename(FileName2, FileName);

  return TRUE;
}

bool PatchNavFilesHD(char *SourceFileName, char *CutFileName, tTimeCodeArray *P2, tTimeCodeArray *P5, tTimeCodeArray *P6)
{
  TYPE_File            *fSourceNav, *fCutNav, *fSourceNavNew;
  char                  FileName[MAX_FILE_NAME_SIZE + 1];
  char                  FileName2[MAX_FILE_NAME_SIZE + 1];
  off_t                 PictureHeaderOffset;
  tnavHD               *navSource, *navSourceNew, *navCut;
  int                   navsRead, navRecsSourceNew, navRecsCut, i;
  bool                  IFrameSource, IFrameCut;

  #define NAVRECS       1000

  #ifdef FULLDEBUG
    WriteLogMC("MovieCutterLib", "PatchNavFilesHD()");
  #endif

  //Open the original nav
  TAP_SPrint(FileName, "%s.nav", SourceFileName);
  fSourceNav = TAP_Hdd_Fopen(FileName);
  if(!fSourceNav)
  {
    WriteLogMC("MovieCutterLib", "PatchNavFilesHD() E0701.");
    return FALSE;
  }

  //Create and open the cut nav
  TAP_SPrint(FileName, "%s.nav", CutFileName);
  TAP_Hdd_Delete(FileName);
  TAP_Hdd_Create(FileName, ATTR_NORMAL);
  fCutNav = TAP_Hdd_Fopen(FileName);
  if(!fCutNav)
  {
    TAP_Hdd_Fclose(fSourceNav);
    WriteLogMC("MovieCutterLib", "PatchNavFilesHD() E0702.");
    return FALSE;
  }

  //Create and open the new source nav
  TAP_SPrint(FileName, "%s.new.nav", SourceFileName);
  TAP_Hdd_Delete(FileName);
  TAP_Hdd_Create(FileName, ATTR_NORMAL);
  fSourceNavNew = TAP_Hdd_Fopen(FileName);
  if(!fSourceNavNew)
  {
    TAP_Hdd_Fclose(fSourceNav);
    TAP_Hdd_Fclose(fCutNav);
    WriteLogMC("MovieCutterLib", "PatchNavFilesHD() E0703.");
    return FALSE;
  }

  //Loop through the nav
  navSource = TAP_MemAlloc(NAVRECS * sizeof(tnavHD));
  navSourceNew = TAP_MemAlloc(NAVRECS * sizeof(tnavHD));
  navCut = TAP_MemAlloc(NAVRECS * sizeof(tnavHD));

  navRecsSourceNew = 0;
  navRecsCut = 0;
  IFrameSource = FALSE;
  IFrameCut = FALSE;
  while(TRUE)
  {
    navsRead = TAP_Hdd_Fread(navSource, sizeof(tnavHD), NAVRECS, fSourceNav);
    if(navsRead == 0) break;

    for(i = 0; i < navsRead; i++)
    {
      //Check if the entry lies between points P2 and P5
      PictureHeaderOffset = ((off_t)(navSource[i].SEIOffsetHigh) << 32) | navSource[i].SEIOffsetLow;
      if((PictureHeaderOffset >= P2->OrigFilePos) && (PictureHeaderOffset <= P5->OrigFilePos))
      {
        //nav entries for the cut file
        if(navRecsCut >= NAVRECS)
        {
          TAP_Hdd_Fwrite(navCut, sizeof(tnavHD), navRecsCut, fCutNav);
          navRecsCut = 0;
        }

        //Subtract (P2 - 1) from the cut .nav PH address
        PictureHeaderOffset -= P2->OrigFilePos;
        if((navSource[i].LastPPS >> 24) == 1) IFrameSource = TRUE;
        if(IFrameSource)
        {
          memcpy(&navCut[navRecsCut], &navSource[i], sizeof(tnavHD));
          navCut[navRecsCut].SEIOffsetHigh = PictureHeaderOffset >> 32;
          navCut[navRecsCut].SEIOffsetLow = PictureHeaderOffset & 0xffffffff;
          navRecsCut++;
        }
        IFrameCut = FALSE;
      }
      else
      {
        //nav entries for the source file
        if(navRecsSourceNew >= NAVRECS)
        {
          TAP_Hdd_Fwrite(navSourceNew, sizeof(tnavHD), navRecsSourceNew, fSourceNavNew);
          navRecsSourceNew = 0;
        }

        if((navSource[i].LastPPS >> 24) == 1) IFrameCut = TRUE;
        if(IFrameCut)
        {
          memcpy(&navSourceNew[navRecsSourceNew], &navSource[i], sizeof(tnavHD));

          //if ph offset > P5, subtract (P6 - P2)
          if(PictureHeaderOffset > P5->OrigFilePos)
          {
            PictureHeaderOffset -= (P6->OrigFilePos - P2->OrigFilePos);
            navSourceNew[navRecsSourceNew].SEIOffsetHigh = PictureHeaderOffset >> 32;
            navSourceNew[navRecsSourceNew].SEIOffsetLow = PictureHeaderOffset & 0xffffffff;
          }
          navRecsSourceNew++;
        }
        IFrameSource = FALSE;
      }
    }
  }

  if(navRecsCut > 0) TAP_Hdd_Fwrite(navCut, sizeof(tnavHD), navRecsCut, fCutNav);
  if(navRecsSourceNew > 0) TAP_Hdd_Fwrite(navSourceNew, sizeof(tnavHD), navRecsSourceNew, fSourceNavNew);

  TAP_Hdd_Fclose(fSourceNav);
  TAP_Hdd_Fclose(fCutNav);
  TAP_Hdd_Fclose(fSourceNavNew);

  TAP_MemFree(navSource);
  TAP_MemFree(navSourceNew);
  TAP_MemFree(navCut);

  //Delete the orig source nav and make the new source nav the active one
  TAP_SPrint(FileName, "%s.nav", SourceFileName);
  TAP_Hdd_Delete(FileName);

  TAP_SPrint(FileName2, "%s.new.nav", SourceFileName);
  TAP_Hdd_Rename(FileName2, FileName);

  return TRUE;
}

void SecToTimeStringMC(dword Time, char *TimeString)
{
  dword                 Hour, Min, Sec;

  Hour = (int)(Time / 3600);
  Min = (int)(Time / 60) % 60;
  Sec = Time % 60;
  TAP_SPrint(TimeString, "%d:%2.2d:%2.2d", Hour, Min, Sec);
}

bool GetPCRPID(char *FileName, word *Result)
{
  char                  InfFileName[MAX_FILE_NAME_SIZE + 1];
  TYPE_File            *fInf;
  byte                 *Buffer;
  dword                 FileSize;
  tRECHeaderInfo        RECHeaderInfo;

  #define INFSIZE       499572

  //Read the source .inf
  TAP_SPrint(InfFileName, "%s.inf", FileName);
  fInf = TAP_Hdd_Fopen(InfFileName);
  if(!fInf)
  {
    WriteLogMC("MovieCutterLib", "GetPCRPID() E0801.");
    return FALSE;
  }

  FileSize = TAP_Hdd_Flen(fInf);
  Buffer = TAP_MemAlloc(INFSIZE);
  if(!Buffer)
  {
    TAP_Hdd_Fclose(fInf);
    WriteLogMC("MovieCutterLib", "GetPCRPID() E0802.");
    return FALSE;
  }
  TAP_Hdd_Fread(Buffer, (FileSize > INFSIZE) ? INFSIZE : FileSize, 1, fInf);
  TAP_Hdd_Fclose(fInf);

  //Decode the source .inf
  if(!HDD_DecodeRECHeader(Buffer, &RECHeaderInfo, ST_UNKNOWN))
  {
    TAP_MemFree(Buffer);
    WriteLogMC("MovieCutterLib", "GetPCRPID() E0803.");
    return FALSE;
  }
  TAP_MemFree(Buffer);

  PCRPID = RECHeaderInfo.SIPCRPID;
  *Result = PCRPID;
  return TRUE;
}

bool GetPCR(char *FileName, dword Block, int PacketSize, word PCRPID, dword *PCR)
{
  FILE                 *fRec;
  byte                 *Buffer;
  off_t                 Offset;
  int                   i;
  char                  AbsFileName[512];
  char                  CurrentDir[512];
  word                  PID;
  size_t                ret;

  #define BUFFERSIZE    90240       //needs to be divideable by 9024 (LCM(188, 192))  *CW*: reduziert, vorher 902400

  //Open the rec for read access
  HDD_TAP_GetCurrentDir(CurrentDir);
  TAP_SPrint(AbsFileName, "%s%s/%s", TAPFSROOT, CurrentDir, FileName);
  fRec = fopen(AbsFileName, "r");
  if(fRec == 0)
  {
    WriteLogMC("MovieCutterLib", "GetPCR() E0901.");
    return FALSE;
  }

  //Reserve a buffer which holds about 512kB
  Buffer = TAP_MemAlloc(BUFFERSIZE);
  if(!Buffer)
  {
    fclose(fRec);
    WriteLogMC("MovieCutterLib", "GetPCR() E0902.");
    return FALSE;
  }

  //Read the data block starting at Block and look for the first PCR
  Offset = (off_t)Block * 9024;
  fseeko64(fRec, Offset, SEEK_SET);
  ret = fread(Buffer, BUFFERSIZE, 1, fRec);
  if(ret == 0)
  {
    fseeko64(fRec, -BUFFERSIZE, SEEK_END);
    ret = fread(Buffer, BUFFERSIZE, 1, fRec);
  }
  fclose(fRec);
  if(ret == 0)
  {
    TAP_MemFree(Buffer);
    WriteLogMC("MovieCutterLib", "GetPCR() E0903.");
    return FALSE;
  }

  i = 0;
  while(((Buffer[i] != 0x47) || (Buffer[i + PacketSize] != 0x47) || (Buffer[i + 2*PacketSize] != 0x47)) && (i < BUFFERSIZE - 10)) i+=4;
  while(i < (BUFFERSIZE - 10))
  {
    PID = ((Buffer[i + 1] & 0x1f) << 8) | Buffer[i + 2];
    if((PID == PCRPID) && ((Buffer[i + 3] & 0x20) != 0) && (Buffer[i + 4] > 0) && (Buffer[i + 5] & 0x10))
    {
      //Extract the time out of the PCR bit pattern
      //The PCR is clocked by a 90kHz generator. To convert to milliseconds
      //the 33 bit number can be shifted right and divided by 45
      *PCR = (dword)((((dword)Buffer[i + 6] << 24) | (Buffer[i + 7] << 16) | (Buffer[i + 8] << 8) | Buffer[i + 9]) / 45);

      TAP_MemFree(Buffer);
      return TRUE;
    }
    i += PacketSize;
  }

  TAP_MemFree(Buffer);
  WriteLogMC("MovieCutterLib", "GetPCR() E0904.");
  return FALSE;
}

dword DeltaPCR(dword FirstPCR, dword SecondPCR)
{
  dword                 d;

  if(FirstPCR <= SecondPCR)
  {
    d = SecondPCR - FirstPCR;
  }
  else
  {
    d = 95443718 - FirstPCR + SecondPCR;  // 95443718 = (2^33)/90  (Überlauf-Behandlung des PCR nach Konvertierung in Millisekunden)
  }

  return d;
}

bool is192ByteTS(char *FileName)
{
  TYPE_File            *f;
  byte                 *Buffer;
  int                   NrTSHeader188 = 0, NrTSHeader192 = 0, TSStartOffset;
  int                   i;

  f = TAP_Hdd_Fopen(FileName);
  if(f)
  {
    Buffer = TAP_MemAlloc(11 * 192);
    if(Buffer)
    {
      TAP_Hdd_Fread(Buffer, 192, 11, f);
      TAP_Hdd_Fclose(f);

      TSStartOffset = 0;
      while( (Buffer[TSStartOffset] != 0x47) 
          || ((Buffer[TSStartOffset + 188] != 0x47) && (Buffer[TSStartOffset + 192] != 0x47))
          || ((Buffer[TSStartOffset + 376] != 0x47) && (Buffer[TSStartOffset + 384] != 0x47)))
        TSStartOffset+=4;

      for(i = 1; i < 10; i++)
      {
        if(Buffer[188 * i + TSStartOffset] == 0x47) NrTSHeader188++;
        if(Buffer[192 * i + TSStartOffset] == 0x47) NrTSHeader192++;
      }
      TAP_MemFree(Buffer);
    }
  }

  return ((NrTSHeader192 > 5) || (NrTSHeader188 < 9));
}

bool HDD_RepairTimeStamp(char *Directory, char *FileName, dword NewTimeStamp)
{
  char                  AbsFileName[256];
  tstat64               statbuf;
  int                   status;
  struct utimbuf        utimebuf;

  TAP_SPrint(AbsFileName, "%s%s/%s", TAPFSROOT, Directory, FileName);
  if((status = lstat64(AbsFileName, &statbuf)))
  {
    WriteLogMC("MovieCutterLib", "HDD_RepairTimeStamp() E0a01.");
    return FALSE;
  }


  if(NewTimeStamp > 0xd0790000)
  {
    utimebuf.actime = statbuf.st_atime;
    utimebuf.modtime = TF2UnixTime(NewTimeStamp);
    utime(AbsFileName, &utimebuf);
    return TRUE;
  }

  WriteLogMC("MovieCutterLib", "HDD_RepairTimeStamp() E0a02.");
  return FALSE;
}

dword GetTimeStampFromInf(char *FileName)
{
  char                  FullFileName[TS_FILE_NAME_SIZE];
  TYPE_File            *f;
  dword                 TimeStamp = 0;

  TAP_SPrint(FullFileName, "%s.inf", FileName);
  f = TAP_Hdd_Fopen(FullFileName);
  if(f)
  {
    TAP_Hdd_Fseek(f, 0x08, 0);
    TAP_Hdd_Fread(&TimeStamp, sizeof(dword), 1, f);
    TAP_Hdd_Fclose(f);
  }
  else
  {
    WriteLogMC("MovieCutterLib", "GetTimeStampFromInf() E0b01.");
  }

  return TimeStamp;
}
