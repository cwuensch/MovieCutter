#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define __USE_LARGEFILE64  1
#define _FILE_OFFSET_BITS  64
#ifdef _MSC_VER
  #define __const const
#endif
//#define STACKTRACE      TRUE

#include                <stdio.h>
#include                <stdlib.h>
#include                <string.h>
#include                <unistd.h>
#include                <stdarg.h>
#include                <sys/stat.h>
#include                <tap.h>
#include                "libFireBird.h"
#include                "CWTapApiLib.h"
#include                "MovieCutterLib.h"


static bool  FileCut(char *SourceFileName, char *CutFileName, char *AbsDirectory, dword StartBlock, dword NrBlocks);
static bool  RecTruncate(char *SourceFileName, char *AbsDirectory, off_t TruncPosition);
static bool  isPacketStart(const byte PacketArray[], int ArrayLen);
static bool  WriteByteToFile(const char *FileName, const char *AbsDirectory, off_t BytePosition, char OldValue, char NewValue);
static bool  PatchRecFile(const char *SourceFileName, const char *AbsDirectory, off_t RequestedCutPosition, byte CutPointArray[], off_t OutPatchedBytes[]);
static bool  UnpatchRecFile(const char *SourceFileName, const char *CutFileName, const char *AbsDirectory, off_t CutStartPos, off_t BehindCutPos, off_t PatchedBytes[], int NrPatchedBytes);
static bool  ReadCutPointArea(const char *SourceFileName, const char *AbsDirectory, off_t CutPosition, byte CutPointArray[]);
static bool  ReadFirstAndLastCutPacket(const char *CutFileName, const char *AbsDirectory, byte FirstCutPacket[], byte LastCutPacket[]);
static bool  FindCutPointOffset(const byte CutPacket[], const byte CutPointArray[], long *const OutOffset);
static bool  FindCutPointOffset2(const byte CutPointArray[], off_t RequestedCutPosition, bool CheckPacketStart, long *const OutOffset);
static bool  PatchInfFiles(const char *SourceFileName, const char *CutFileName, const char *AbsDirectory, dword SourcePlayTime, const tTimeStamp *CutStartPoint, const tTimeStamp *BehindCutPoint);
static bool  PatchNavFiles(const char *SourceFileName, const char *CutFileName, const char *AbsDirectory, off_t CutStartPos, off_t BehindCutPos, bool isHD, bool IgnoreRecordsAfterCut, dword *const OutCutStartTime, dword *const OutBehindCutTime, dword *const OutSourcePlayTime);

static int              PACKETSIZE = 192;
static int              SYNCBYTEPOS = 4;
static int              CUTPOINTSEARCHRADIUS = 9024;
static int              CUTPOINTSECTORRADIUS = 2;


// ----------------------------------------------------------------------------
//                           Hilfsfunktionen
// ----------------------------------------------------------------------------
void CreateSettingsDir(void)
{
  TRACEENTER();

//  struct stat64 st;
//  if (lstat64(TAPFSROOT "/ProgramFiles/Settings", &st) == -1)
    mkdir(TAPFSROOT "/ProgramFiles/Settings", 666);
//  if (lstat64(TAPFSROOT "/ProgramFiles/Settings/MovieCutter", &st) == -1)
    mkdir(TAPFSROOT "/ProgramFiles/Settings/MovieCutter", 666);

  TRACEEXIT();
}

void WriteLogMC(char *ProgramName, char *s)
{
//  HDD_TAP_PushDir();
//  TAP_Hdd_ChangeDir("/ProgramFiles/Settings/MovieCutter");
  LogEntry2(TAPFSROOT "/ProgramFiles/Settings/MovieCutter/MovieCutter.log", ProgramName, TRUE, TIMESTAMP_YMDHMS, s);
//  HDD_TAP_PopDir();
}
void WriteLogMCf(char *ProgramName, const char *format, ...)
{
  char Text[512];

  if(format)
  {
    va_list args;
    va_start(args, format);
    vsnprintf(Text, sizeof(Text), format, args);
    va_end(args);
    WriteLogMC(ProgramName, Text);
  }
}

/* void WriteDebugLog(const char *format, ...)
{
  char Text[512];

  HDD_TAP_PushDir();
  TAP_Hdd_ChangeDir("/ProgramFiles/Settings/MovieCutter");

  if(format)
  {
    va_list args;
    va_start(args, format);
    vsnprintf(Text, sizeof(Text), format, args);
    va_end(args);
    LogEntry("Aufnahmenfresser.log", "", FALSE, TIMESTAMP_YMDHMS, Text);
  }

  HDD_TAP_PopDir();
} */

static inline dword CalcBlockSize(off_t Size)
{
  // Workaround f�r die Division durch BLOCKSIZE (9024)
  // Primfaktorenzerlegung: 9024 = 2^6 * 3 * 47
  // max. Dateigr��e: 256 GB (d�rfte reichen...)
  return (dword)(Size >> 6) / 141;
}

void SecToTimeString(dword Time, char *const OutTimeString)  // needs max. 4 + 1 + 2 + 1 + 2 + 1 = 11 chars
{
  dword                 Hour, Min, Sec;

  TRACEENTER();

  if(OutTimeString)
  {
    Hour = (Time / 3600);
    Min  = (Time / 60) % 60;
    Sec  = Time % 60;
    if (Hour >= 10000) Hour = 9999;
    TAP_SPrint(OutTimeString, 11, "%lu:%02lu:%02lu", Hour, Min, Sec);
  }
  TRACEEXIT();
}

void MSecToTimeString(dword Timems, char *const OutTimeString)  // needs max. 4 + 1 + 2 + 1 + 2 + 1 + 3 + 1 = 15 chars
{
  dword                 Hour, Min, Sec, Millisec;

  TRACEENTER();

  if(OutTimeString)
  {
    Hour = (Timems / 3600000);
    Min  = (Timems / 60000) % 60;
    Sec  = (Timems / 1000) % 60;
    Millisec = Timems % 1000;
    TAP_SPrint(OutTimeString, 15, "%lu:%02lu:%02lu,%03lu", Hour, Min, Sec, Millisec);
  }
  TRACEEXIT();
}

dword TimeStringToMSec(char *const TimeString)
{
  dword                 Hour=0, Min=0, Sec=0, Millisec=0, ret=0;
  TRACEENTER();

  if(TimeString)
  {
    if (sscanf(TimeString, "%lu:%lu:%lu%*1[.,]%lu", &Hour, &Min, &Sec, &Millisec) == 4)
      ret = 1000*(60*(60*Hour + Min) + Sec) + Millisec;
  }
  TRACEEXIT();
  return ret;
}

void GetNextFreeCutName(const char *SourceFileName, char *const OutCutFileName, const char *AbsDirectory, int LeaveNamesOut)
{
  char                  CheckFileName[MAX_FILE_NAME_SIZE + 1];
  size_t                NameLen, ExtStart;
  int                   FreeIndices = 0, i = 0;

  TRACEENTER();
  if(OutCutFileName) OutCutFileName[0] = '\0';

  if (SourceFileName && OutCutFileName)
  {
    NameLen = ExtStart = strlen(SourceFileName) - 4;  // ".rec" entfernen
//    if((p = strstr(&SourceFileName[NameLen - 10], " (Cut-")) != NULL)
//      NameLen = p - SourceFileName;        // wenn schon ein ' (Cut-xxx)' vorhanden ist, entfernen
    strncpy(CheckFileName, SourceFileName, NameLen);

    do
    {
      i++;
      TAP_SPrint(&CheckFileName[NameLen], MAX_FILE_NAME_SIZE+1 - NameLen, " (Cut-%d)%s", i, &SourceFileName[ExtStart]);
      if (!HDD_Exist2(CheckFileName, AbsDirectory))
        FreeIndices++;
    } while (FreeIndices <= LeaveNamesOut);

    strcpy(OutCutFileName, CheckFileName);
  }
  TRACEEXIT();
}


// ----------------------------------------------------------------------------
//                       MovieCutter-Schnittfunktion
// ----------------------------------------------------------------------------
tResultCode MovieCutter(char *SourceFileName, char *CutFileName, char *AbsDirectory, tTimeStamp *CutStartPoint, tTimeStamp *BehindCutPoint, bool KeepCut, bool isHD)
{
  char                  FileName[MAX_FILE_NAME_SIZE + 1];
  off_t                 SourceFileSize, CutFileSize;
  __ino64_t             InodeNr;
  dword                 MaxBehindCutBlock;
  off_t                 CutStartPos, BehindCutPos;
  dword                 SourcePlayTime = 0;
  bool                  TruncateEnding = FALSE;
  bool                  SuppressNavGeneration = FALSE;
  dword                 RecDate;
//  char                  TimeStr[16];

  TRACEENTER();
  GetPacketSize(SourceFileName);

  // LOG file printing
  WriteLogMC ("MovieCutterLib", "----------------------------------------");
  WriteLogMCf("MovieCutterLib", "Source        = '%s'", SourceFileName);
  WriteLogMCf("MovieCutterLib", "Cut name      = '%s'", CutFileName);

  if(!HDD_GetFileSizeAndInode2(SourceFileName, AbsDirectory, &InodeNr, &SourceFileSize))
  {
    WriteLogMC("MovieCutterLib", "MovieCutter() E0001: cut file not created.");
    TRACEEXIT();
    return RC_Error;
  }

  WriteLogMCf("MovieCutterLib", "Inode Nr.     = %llu", InodeNr);
  WriteLogMCf("MovieCutterLib", "File size     = %llu Bytes (%lu blocks)", SourceFileSize, CalcBlockSize(SourceFileSize));
  WriteLogMCf("MovieCutterLib", "AbsDir        = '%s'", AbsDirectory);
  WriteLogMCf("MovieCutterLib", "PacketSize    = %d", PACKETSIZE);

  MaxBehindCutBlock = CalcBlockSize(SourceFileSize - CUTPOINTSEARCHRADIUS);
  if (BehindCutPoint->BlockNr == 0xFFFFFFFF)
    TruncateEnding = TRUE;
  if (BehindCutPoint->BlockNr > MaxBehindCutBlock)
    BehindCutPoint->BlockNr = MaxBehindCutBlock;
  CutStartPos = (off_t)CutStartPoint->BlockNr * BLOCKSIZE;
  BehindCutPos = (off_t)BehindCutPoint->BlockNr * BLOCKSIZE;

  WriteLogMCf("MovieCutterLib", "KeepCut       = %s", KeepCut ? "yes" : "no");
  WriteLogMCf("MovieCutterLib", "CutStartBlock = %lu,\tBehindCutBlock = %lu", CutStartPoint->BlockNr, BehindCutPoint->BlockNr);
  WriteLogMCf("MovieCutterLib", "CutStartPos   = %llu,\tBehindCutPos = %llu", CutStartPos, BehindCutPos);

  if (TruncateEnding && !KeepCut)
  {
//    FindCutPointOffset2(CutPointArea1, CutStartPos, TRUE, &CutStartPosOffset);         // unn�tig?
    CutStartPos = CutStartPos + 9024;
    BehindCutPos = SourceFileSize;
//    BehindCutPosOffset = BehindCutPos - ((off_t)BehindCutPoint->BlockNr * BLOCKSIZE);  // unn�tig!
//    WriteLogMCf("MovieCutterLib", "Cut start offset: %ld Bytes (=%ld packets and %ld Bytes), Cut end offset: %ld Bytes (=%ld packets and %ld Bytes)", CutStartPosOffset, CutStartPosOffset/PACKETSIZE, labs(CutStartPosOffset%PACKETSIZE), BehindCutPosOffset, BehindCutPosOffset/PACKETSIZE, labs(BehindCutPosOffset%PACKETSIZE));
    WriteLogMCf("MovieCutterLib", "Expected cut positions:  Cut Start = %llu, Behind Cut: %llu", CutStartPos, BehindCutPos);
  }
  else
  {
    byte                FirstCutPacket[PACKETSIZE], LastCutPacket[PACKETSIZE];
    off_t               PatchedBytes[4 * CUTPOINTSECTORRADIUS];
    byte               *CutPointArea1=NULL, *CutPointArea2=NULL;
    long                CutStartPosOffset, BehindCutPosOffset;
    int                 i;

    // Read the two blocks surrounding the cut points from the recording
    CutPointArea1 = (byte*) TAP_MemAlloc(CUTPOINTSEARCHRADIUS * 2);
    CutPointArea2 = (byte*) TAP_MemAlloc(CUTPOINTSEARCHRADIUS * 2);
    if (!CutPointArea1 || !CutPointArea2)
    {
      WriteLogMC("MovieCutterLib", "MovieCutter() W0001: nav creation suppressed.");
      SuppressNavGeneration = TRUE;
    }
    if(CutPointArea1)
    {
      if (!ReadCutPointArea(SourceFileName, AbsDirectory, CutStartPos, CutPointArea1))
      {
        WriteLogMC("MovieCutterLib", "MovieCutter() W0002: nav creation suppressed.");
        SuppressNavGeneration = TRUE;
        TAP_MemFree(CutPointArea1); CutPointArea1 = NULL;
      }
    }
    if(CutPointArea2)
    {
      if(!ReadCutPointArea(SourceFileName, AbsDirectory, BehindCutPos, CutPointArea2))
      {
        WriteLogMC("MovieCutterLib", "MovieCutter() W0003: nav creation suppressed.");
        SuppressNavGeneration = TRUE;
        TAP_MemFree(CutPointArea2); CutPointArea2 = NULL;
      }
    }

    // Patch the rec-File to prevent the firmware from cutting in the middle of a packet
    for (i = 0; i < 4 * CUTPOINTSECTORRADIUS; i++)
      PatchedBytes[i] = 0;
    if(CutPointArea1 && (CutStartPos > 0))
      PatchRecFile(SourceFileName, AbsDirectory, CutStartPos, CutPointArea1, PatchedBytes);
    if(CutPointArea2)
      PatchRecFile(SourceFileName, AbsDirectory, BehindCutPos, CutPointArea2, &PatchedBytes[2 * CUTPOINTSECTORRADIUS]);

    // DO THE CUTTING
    if(!FileCut(SourceFileName, CutFileName, AbsDirectory, CutStartPoint->BlockNr, BehindCutPoint->BlockNr - CutStartPoint->BlockNr))
    {
      WriteLogMC("MovieCutterLib", "MovieCutter() E0002: Firmware cutting routine failed.");
      TAP_MemFree(CutPointArea1);
      TAP_MemFree(CutPointArea2);
      TRACEEXIT();
      return RC_Error;
    }
    if(!HDD_Exist2(CutFileName, AbsDirectory))
    {
      WriteLogMC("MovieCutterLib", "MovieCutter() E0003: Cut file not created.");
      TAP_MemFree(CutPointArea1);
      TAP_MemFree(CutPointArea2);
      TRACEEXIT();
      return RC_Error;
    }
    WriteLogMC("MovieCutterLib", "Firmware cutting routine finished.");

    // Detect the size of the cut file
    if(!HDD_GetFileSizeAndInode2(CutFileName, AbsDirectory, &InodeNr, &CutFileSize))
    {
      WriteLogMC("MovieCutterLib", "MovieCutter() W0004: error detecting size of cut file.");
      SuppressNavGeneration = TRUE;
    }
    WriteLogMCf("MovieCutterLib", "Cut file size: %llu Bytes (=%lu blocks) - inode=%llu", CutFileSize, CalcBlockSize(CutFileSize), InodeNr);

    // Read the beginning and the ending from the cut file
    if(!ReadFirstAndLastCutPacket(CutFileName, AbsDirectory, FirstCutPacket, LastCutPacket))
    {
      WriteLogMC("MovieCutterLib", "MovieCutter() W0005: nav creation suppressed.");
      SuppressNavGeneration = TRUE;
    }

    // Detect the actual cutting positions (differing from requested cut points!) for the nav generation
    if(!SuppressNavGeneration)
    {
      if (FindCutPointOffset(FirstCutPacket, CutPointArea1, &CutStartPosOffset))
        CutStartPos = CutStartPos + CutStartPosOffset;
      else
      {
        WriteLogMC("MovieCutterLib", "MovieCutter() W0006: Cut start position not found.");
        SuppressNavGeneration = TRUE;
      }
      if (FindCutPointOffset(LastCutPacket, CutPointArea2, &BehindCutPosOffset))
      {
        BehindCutPosOffset = BehindCutPosOffset + PACKETSIZE;
        BehindCutPos = BehindCutPos + BehindCutPosOffset;

        // if cut start point was not found, re-calculate it from cut file size
        if (SuppressNavGeneration && (CutFileSize != 0)) {
          CutStartPos = BehindCutPos - CutFileSize;
          CutStartPosOffset = CutStartPos - ((off_t)CutStartPoint->BlockNr * BLOCKSIZE);  // unn�tig
          SuppressNavGeneration = FALSE;
        }
      }
      else
      {
        WriteLogMC("MovieCutterLib", "MovieCutter() W0007: Cut end position not found.");
        // if cut end point was not found, re-calculate it from cut file size
        if (!SuppressNavGeneration && (CutFileSize != 0))
        {
          BehindCutPos = CutStartPos + CutFileSize;
          BehindCutPosOffset = BehindCutPos - ((off_t)BehindCutPoint->BlockNr * BLOCKSIZE);  // unn�tig
        }
        else
          SuppressNavGeneration = TRUE;
      }
      if (SuppressNavGeneration)
        WriteLogMC("MovieCutterLib", "MovieCutter() W0008: Both cut points not found. Nav creation suppressed.");
      WriteLogMCf("MovieCutterLib", "Cut start offset: %ld Bytes (=%ld packets and %ld Bytes), Cut end offset: %ld Bytes (=%ld packets and %ld Bytes)", CutStartPosOffset, CutStartPosOffset/PACKETSIZE, labs(CutStartPosOffset%PACKETSIZE), BehindCutPosOffset, BehindCutPosOffset/PACKETSIZE, labs(BehindCutPosOffset%PACKETSIZE));
      WriteLogMCf("MovieCutterLib", "Real cut positions:  Cut Start = %llu, Behind Cut: %llu", CutStartPos, BehindCutPos);
    }

#ifdef FULLDEBUG
  bool  CutStartGuessed, BehindCutGuessed;
  long  GuessedCutStartOffset = 0, GuessedBehindCutOffset = 0;
  off_t ReqCutStartPos  = (off_t)CutStartPoint->BlockNr * BLOCKSIZE;
  off_t ReqBehindCutPos = (off_t)BehindCutPoint->BlockNr * BLOCKSIZE;

  CutStartGuessed  = FindCutPointOffset2(CutPointArea1, ReqCutStartPos, FALSE, &GuessedCutStartOffset);
  BehindCutGuessed = FindCutPointOffset2(CutPointArea2, ReqBehindCutPos, FALSE, &GuessedBehindCutOffset);

  if ((CutStartGuessed && (CutStartPos == ReqCutStartPos + GuessedCutStartOffset)) && (BehindCutGuessed && (BehindCutPos == ReqBehindCutPos + GuessedBehindCutOffset)))
    WriteLogMC("MovieCutterLib", "--> Real cutting points guessed correctly!");
  else
    WriteLogMCf("MovieCutterLib", "!! -- Real cutting points NOT correctly guessed: GuessedStart = %llu, GuessedBehind = %llu", (CutStartGuessed ? ReqCutStartPos + GuessedCutStartOffset : 0), (BehindCutGuessed ? ReqBehindCutPos + GuessedBehindCutOffset : 0));
#endif

    TAP_MemFree(CutPointArea1); CutPointArea1 = NULL;
    TAP_MemFree(CutPointArea2); CutPointArea2 = NULL;

    // Unpatch the rec-Files
    if (!SuppressNavGeneration)
      UnpatchRecFile(SourceFileName, CutFileName, AbsDirectory, CutStartPos, BehindCutPos, PatchedBytes, 4 * CUTPOINTSECTORRADIUS);
  }

  // Copy the real cutting positions into the cut point parameters to be returned
  if (!SuppressNavGeneration)
  {
    CutStartPoint->BlockNr = CalcBlockSize(CutStartPos + BLOCKSIZE-1);  // erster vollst�ndiger Block des CutFile
    BehindCutPoint->BlockNr = CutStartPoint->BlockNr + CalcBlockSize(BehindCutPos - CutStartPos + BLOCKSIZE/2);
  }

  // Truncate the source rec-File
  if(!SuppressNavGeneration && TruncateEnding)
  {
    if(!RecTruncate(SourceFileName, AbsDirectory, CutStartPos))
    {
      WriteLogMC("MovieCutterLib", "MovieCutter() E0004: Truncate routine failed.");
      if(!KeepCut)
        return RC_Error;
    }
  }

  // Rename old nav file to bak
  char BakFileName[MAX_FILE_NAME_SIZE + 1];
  TAP_SPrint(FileName, sizeof(FileName), "%s.nav", SourceFileName);
  TAP_SPrint(BakFileName, sizeof(BakFileName), "%s.nav.bak", SourceFileName);
  if (HDD_Exist2(FileName, AbsDirectory))
  {
    if (HDD_Exist2(BakFileName, AbsDirectory))
      HDD_Delete2(BakFileName, AbsDirectory, FALSE);
    HDD_Rename2(FileName, BakFileName, AbsDirectory, FALSE);
  }

  // Patch the nav files (and get the TimeStamps for the actual cutting positions)
  if(!SuppressNavGeneration)
  {
    if(PatchNavFiles(SourceFileName, CutFileName, AbsDirectory, CutStartPos, BehindCutPos, isHD, TruncateEnding, &(CutStartPoint->Timems), &(BehindCutPoint->Timems), &SourcePlayTime))
      HDD_Delete2(BakFileName, AbsDirectory, FALSE);
    else
    {
      WriteLogMC("MovieCutterLib", "MovieCutter() W0009: nav creation failed.");
      SuppressNavGeneration = TRUE;
      TAP_SPrint(FileName, sizeof(FileName), "%s.nav", SourceFileName);
      HDD_Delete2(FileName, AbsDirectory, FALSE);
      TAP_SPrint(FileName, sizeof(FileName), "%s.nav", CutFileName);
      HDD_Delete2(FileName, AbsDirectory, FALSE);
    }
  }

  // Copy the inf file and patch both play lengths
  if (!PatchInfFiles(SourceFileName, CutFileName, AbsDirectory, SourcePlayTime, CutStartPoint, BehindCutPoint))
    WriteLogMC("MovieCutterLib", "MovieCutter() W0010: inf creation failed.");

  // Fix the date info of all involved files
  if (GetRecDateFromInf(SourceFileName, AbsDirectory, &RecDate)) {
    //Source
    char LogString[512];
    LogString[0] = '\0';
    if (!HDD_SetFileDateTime(SourceFileName, AbsDirectory, RecDate))
      TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), "HDD_SetFileDateTime(%s) failed. ", SourceFileName);
    TAP_SPrint(FileName, sizeof(FileName), "%s.inf", SourceFileName);
    if (!HDD_SetFileDateTime(FileName, AbsDirectory, RecDate))
      TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), "HDD_SetFileDateTime(%s) failed. ", FileName);
    TAP_SPrint(FileName, sizeof(FileName), "%s.nav", SourceFileName);
    if (!HDD_SetFileDateTime(FileName, AbsDirectory, RecDate))
      TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), "HDD_SetFileDateTime(%s) failed. ", FileName);
    if(LogString[0])
      WriteLogMC("MovieCutterLib", LogString);
  }
  if (KeepCut && GetRecDateFromInf(CutFileName, AbsDirectory, &RecDate)) {
    //Cut
    char LogString[512];
    LogString[0] = '\0';
    if (!HDD_SetFileDateTime(CutFileName, AbsDirectory, RecDate))
      TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), "HDD_SetFileDateTime(%s) failed. ", CutFileName);
    TAP_SPrint(FileName, sizeof(FileName), "%s.inf", CutFileName);
    if (!HDD_SetFileDateTime(FileName, AbsDirectory, RecDate))
      TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), "HDD_SetFileDateTime(%s) failed. ", FileName);
    TAP_SPrint(FileName, sizeof(FileName), "%s.nav", CutFileName);
    if (!HDD_SetFileDateTime(FileName, AbsDirectory, RecDate))
      TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), "HDD_SetFileDateTime(%s) failed. ", FileName);
    if(LogString[0])
      WriteLogMC("MovieCutterLib", LogString);
  }
//  if(!KeepSource) HDD_Delete2(SourceFileName, Directory, TRUE);
  if(!KeepCut)
    HDD_Delete2(CutFileName, AbsDirectory, TRUE);

  WriteLogMC("MovieCutterLib", "MovieCutter() finished.");

  TRACEEXIT();
  return ((SuppressNavGeneration) ? RC_Warning : RC_Ok);
}

// ----------------------------------------------------------------------------
//              Firmware-Funktion zum Durchf�hren des Schnitts
// ----------------------------------------------------------------------------
bool FileCut(char *SourceFileName, char *CutFileName, char *AbsDirectory, dword StartBlock, dword NrBlocks)
{
  tDirEntry             FolderStruct, *pFolderStruct;
  TYPE_PlayInfo         PlayInfo;
  dword                 ret = -1;
  dword                 x;

  TRACEENTER();
  WriteLogMCf("MovieCutterLib", "FileCut('%s', '%s', '%s', %lu, %lu)", SourceFileName, CutFileName, AbsDirectory, StartBlock, NrBlocks);

  //If a playback is running, stop it
  TAP_Hdd_GetPlayInfo(&PlayInfo);
  if(PlayInfo.playMode == PLAYMODE_Playing)
  {
    Appl_StopPlaying();
    Appl_WaitEvt(0xE507, &x, 1, 0xFFFFFFFF, 300);
  }
  HDD_Delete2(CutFileName, AbsDirectory, TRUE);

  //Flush the caches *experimental*
  sync();
  TAP_Sleep(1);

  HDD_TAP_PushDir();

  //Initialize the directory structure
  memset(&FolderStruct, 0, sizeof(tDirEntry));
  FolderStruct.Magic = 0xbacaed31;

  //Save the current directory resources and change into our directory (current directory of the TAP)
  ApplHdd_SaveWorkFolder();
  if (ApplHdd_SelectFolder(&FolderStruct, &AbsDirectory[1]) == 0)  //do not include the leading slash
  {
    if (DevHdd_DeviceOpen(&pFolderStruct, &FolderStruct) == 0)
    {
      ApplHdd_SetWorkFolder(&FolderStruct);

      //Do the cutting
      #ifdef FULLDEBUG
        WriteLogMCf("MovieCutterLib", "ApplHdd_FileCutPaste('%s', %lu, %lu, '%s')", SourceFileName, StartBlock, NrBlocks, CutFileName);
      #endif
      ret = ApplHdd_FileCutPaste(SourceFileName, StartBlock, NrBlocks, CutFileName);
      #ifdef FULLDEBUG
        WriteLogMCf("MovieCutterLib", "ApplHdd_FileCutPaste() returned: %lu.", ret);
      #endif
      //Restore all resources
      DevHdd_DeviceClose(pFolderStruct);
    }
  }
  ApplHdd_RestoreWorkFolder();
  HDD_TAP_PopDir();

  //Flush the caches *experimental*
  sync();
  TAP_Sleep(1);
  sync();
  TAP_Sleep(1);
/*  for (i=0; i < 30; i++)
  {
//    TAP_SystemProc();
    TAP_Sleep(10);
  }
  char DeviceNode[FBLIB_DIR_SIZE], CommandLine[FBLIB_DIR_SIZE];
  HDD_FindMountPointDev2(AbsDirectory, NULL, DeviceNode);
  TAP_SPrint(CommandLine, sizeof(CommandLine), "/mnt/hd/ProgramFiles/busybox hdparm -f %s", DeviceNode);
  system(CommandLine); */

  if(ret != 0)
  {
    WriteLogMC("MovieCutterLib", "FileCut() E0501. Cut file not created.");
    TRACEEXIT();
    return FALSE;
  }

  TRACEEXIT();
  return TRUE;
}

bool RecTruncate(char *SourceFileName, char *AbsDirectory, off_t TruncPosition)
{
  TYPE_PlayInfo         PlayInfo;
  dword                 x;
  bool                  ret;

  TRACEENTER();
  WriteLogMCf("MovieCutterLib", "RecTruncate('%s', '%s', %llu)", SourceFileName, AbsDirectory, TruncPosition);

  //If a playback is running, stop it
  TAP_Hdd_GetPlayInfo(&PlayInfo);
  if(PlayInfo.playMode == PLAYMODE_Playing)
  {
    Appl_StopPlaying();
    Appl_WaitEvt(0xE507, &x, 1, 0xFFFFFFFF, 300);
  }

  //Flush the caches *experimental*
  sync();
  TAP_Sleep(1);

  ret = HDD_TruncateFile(SourceFileName, AbsDirectory, TruncPosition);

  TRACEEXIT();
  return ret;
}


// ----------------------------------------------------------------------------
//                         Analyse von REC-Files
// ----------------------------------------------------------------------------
int GetPacketSize(const char *RecFileName)
{
  TRACEENTER();

  if (strncmp(&RecFileName[strlen(RecFileName) - 4], ".mpg", 4) == 0)
  {
    PACKETSIZE = 188;
    SYNCBYTEPOS = 0;   // 4 - komisch, aber ist tats�chlich so!!!
    CUTPOINTSEARCHRADIUS = 99264;
    CUTPOINTSECTORRADIUS = CUTPOINTSEARCHRADIUS/4096;  // 24
  }
  else
  {
    PACKETSIZE = 192;
    SYNCBYTEPOS = 4;
    CUTPOINTSEARCHRADIUS = 9024;
    CUTPOINTSECTORRADIUS = CUTPOINTSEARCHRADIUS/4096;  // 2
  }

  TRACEEXIT();
  return PACKETSIZE;
}

bool isNavAvailable(const char *RecFileName, const char *AbsDirectory)
{
  char                  NavFileName[MAX_FILE_NAME_SIZE + 1];
  off_t                 NavFileSize;
  bool                  ret;

  TRACEENTER();
  ret = FALSE;

  TAP_SPrint(NavFileName, sizeof(NavFileName), "%s.nav", RecFileName);
  if (HDD_Exist2(NavFileName, AbsDirectory))
  {
    if (HDD_GetFileSizeAndInode2(NavFileName, AbsDirectory, NULL, &NavFileSize))
      if (NavFileSize != 0)
        ret = TRUE;
  }

  TRACEEXIT();
  return ret;
}

// TODO: Auslesen der Stream-Information aus dem InfCache im RAM
bool isCrypted(const char *RecFileName, const char *AbsDirectory)
{
  FILE                 *f = NULL;
  char                  AbsInfName[FBLIB_DIR_SIZE];
  byte                  CryptFlag = 2;
  bool                  ret;

  TRACEENTER();

  TAP_SPrint(AbsInfName, sizeof(AbsInfName), "%s/%s.inf", AbsDirectory, RecFileName);
  f = fopen(AbsInfName, "rb");
  if(f)
  {
    fseek(f, 0x0010, SEEK_SET);
    fread(&CryptFlag, 1, 1, f);
    fclose(f);
    ret = ((CryptFlag & 1) != 0);
  }
  else
    ret = TRUE;

  TRACEEXIT();
  return ret;
}

// TODO: Auslesen der Stream-Information aus dem InfCache im RAM
bool isHDVideo(const char *RecFileName, const char *AbsDirectory, bool *const isHD)
{
  FILE                 *f = NULL;
  char                  AbsInfName[FBLIB_DIR_SIZE];
  byte                  StreamType = STREAM_UNKNOWN;

  if (isHD == NULL) return FALSE;
  TRACEENTER();

  TAP_SPrint(AbsInfName, sizeof(AbsInfName), "%s/%s.inf", AbsDirectory, RecFileName);
  f = fopen(AbsInfName, "rb");
  if(f)
  {
    fseek(f, 0x0042, SEEK_SET);
    fread(&StreamType, 1, 1, f);
    fclose(f);

    if ((StreamType==STREAM_VIDEO_MPEG4_PART2) || (StreamType==STREAM_VIDEO_MPEG4_H264) || (StreamType==STREAM_VIDEO_MPEG4_H263))
      *isHD = TRUE;
    else if ((StreamType==STREAM_VIDEO_MPEG1) || (StreamType==STREAM_VIDEO_MPEG2))
      *isHD = FALSE;
    else
    {
      WriteLogMC("MovieCutterLib", "isHDVideo() E0102.");
      TRACEEXIT();
      return FALSE;
    }
  }
  else
  {
    WriteLogMC("MovieCutterLib", "isHDVideo() E0101.");
    TRACEEXIT();
    return FALSE;
  }

  TRACEEXIT();
  return TRUE;
}


// ----------------------------------------------------------------------------
//                          Patchen von REC-Files
// ----------------------------------------------------------------------------
bool isPacketStart(const byte PacketArray[], int ArrayLen)
{
  int                   i;
  bool                  ret = TRUE;

  TRACEENTER();
  for (i = 0; i < 10; i++)
  {
    if (SYNCBYTEPOS + (i * PACKETSIZE) >= ArrayLen)
      break;
    if (PacketArray[SYNCBYTEPOS + (i * PACKETSIZE)] != 'G')
    {
      ret = FALSE;
      break;
    }
  }
  TRACEEXIT();
  return ret;
}

bool WriteByteToFile(const char *FileName, const char *AbsDirectory, off_t BytePosition, char OldValue, char NewValue)
{
  char                  AbsFileName[FBLIB_DIR_SIZE];
  FILE                 *f = NULL;
  char                  ret;

  TRACEENTER();
  #ifdef FULLDEBUG
    WriteLogMCf("MovieCutterLib", "WriteByteToFile(file='%s', position=%llu, old=%#4hhx, new=%#4hhx).", FileName, BytePosition, OldValue, NewValue);
  #endif

  // Open the file for write access
  TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s/%s", AbsDirectory, FileName);
  f = fopen(AbsFileName, "r+b");
  if(f == 0)
  {
    WriteLogMC("MovieCutterLib", "WriteByteToFile() E0e01.");
    TRACEEXIT();
    return FALSE;
  }

  // Seek to the desired position
  if (fseeko64(f, BytePosition, SEEK_SET) != 0)
  {
    fclose(f);
    WriteLogMC("MovieCutterLib", "WriteByteToFile() E0e02.");
    TRACEEXIT();
    return FALSE;
  }

  // Check, if the old value is correct
  char old = (char)fgetc(f);
  #ifdef FULLDEBUG
    if ((NewValue == 'G') && (old != OldValue))
      WriteLogMCf("MovieCutterLib", "Restore Sync-Byte: value read from cache=%#4hhx (expected %#4hhx).", old, OldValue);
  #endif
  if ((old != OldValue) && (old != 'G'))
  {
    fclose(f);
    WriteLogMC("MovieCutterLib", "WriteByteToFile() E0e03.");
    TRACEEXIT();
    return FALSE;
  }

  // Write the new byte to the file
  fseeko64(f, -1, SEEK_CUR);
  ret = fputc(NewValue, f);
  fflush(f);
  fclose(f);
  if(ret != NewValue)
  {
    WriteLogMC("MovieCutterLib", "WriteByteToFile() E0e04.");
    TRACEEXIT();
    return FALSE;
  }

  // Print to log
  if (NewValue != 'G')
    WriteLogMCf("MovieCutterLib", "Remove Sync-Byte: Changed value in file '%s' at position %llu from %#4hhx to %#4hhx.", FileName, BytePosition, OldValue, NewValue);
  else
    WriteLogMCf("MovieCutterLib", "Restore Sync-Byte: Changed value in file '%s' at position %llu from %#4hhx to %#4hhx.", FileName, BytePosition, OldValue, NewValue);

  TRACEEXIT();
  return TRUE;
}

// Patches the rec-File to prevent the firmware from cutting in the middle of a packet
bool PatchRecFile(const char *SourceFileName, const char *AbsDirectory, off_t RequestedCutPosition, byte CutPointArray[], off_t OutPatchedBytes[])
{
  off_t                 pos;
  int                   ArrayPos;
  int                   i;
  bool                  ret = TRUE;

  TRACEENTER();
//  #ifdef FULLDEBUG
    WriteLogMC("MovieCutterLib", "PatchRecFile()");
//  #endif

  byte *const           MidArray = &CutPointArray[CUTPOINTSEARCHRADIUS];

  if (!OutPatchedBytes || !CutPointArray)
  {
    TRACEEXIT();
    return FALSE;
  }
  memset(OutPatchedBytes, 0, 2*CUTPOINTSECTORRADIUS);

  // For each of the 4 (Austr: 48) possible cut positions
  for (i = -(CUTPOINTSECTORRADIUS-1); i <= CUTPOINTSECTORRADIUS; i++)
  {
    pos = ((RequestedCutPosition >> 12) << 12) + (i * 4096);
    ArrayPos = (int)(pos-RequestedCutPosition);
    if ((PACKETSIZE==188) && (i == CUTPOINTSECTORRADIUS)) break;

    // Check, if the current position is a sync-byte
    if ((MidArray[ArrayPos+4] == 'G'))
    {
      // If there IS a sync-Byte, but NOT a packet start, then patch this byte
      if (!isPacketStart(&MidArray[ArrayPos], CUTPOINTSEARCHRADIUS-ArrayPos))
      {
        if (WriteByteToFile(SourceFileName, AbsDirectory, pos+4, 'G', 'F'))
        {
          OutPatchedBytes[i + (CUTPOINTSECTORRADIUS-1)] = pos+4;
          MidArray[ArrayPos+4] = 'F';    // ACHTUNG! N�tig, damit die neue Sch�tzung der CutPosition korrekt funktioniert.
        }                                // K�nnte aber ein Problem geben bei der (alten) CutPoint-Identifikation (denn hier wird der noch ungepatchte Wert aus dem Cache gelesen)!
        else
          ret = FALSE;
      }
    }

    // If Australian PVR, write a sync-Byte at the desired cut position + 4
    if (PACKETSIZE==188)
    {
      if (MidArray[ArrayPos+0] == 'G')
      {
        if (isPacketStart(&MidArray[ArrayPos], CUTPOINTSEARCHRADIUS-ArrayPos))
        {
          byte oldVal = MidArray[ArrayPos+4];
          if (WriteByteToFile(SourceFileName, AbsDirectory, pos+4, oldVal, 'G'))
          {
            OutPatchedBytes[2*CUTPOINTSECTORRADIUS - 1] = ((pos+4) << 8) + oldVal;  // missbrauche die letzte Position f�r den Schnitt-Patch
            MidArray[ArrayPos+4] = 'G';    // ACHTUNG! N�tig, damit die neue Sch�tzung der CutPosition korrekt funktioniert.
          }                                // K�nnte aber ein Problem geben bei der (alten) CutPoint-Identifikation (denn hier wird der noch ungepatchte Wert aus dem Cache gelesen)!
          else
            ret = FALSE;
        }
      }
    }
  }

  TRACEEXIT();
  return ret;
}

// Restores the patched Sync-Bytes in the rec-File
bool UnpatchRecFile(const char *SourceFileName, const char *CutFileName, const char *AbsDirectory, off_t CutStartPos, off_t BehindCutPos, off_t PatchedBytes[], int NrPatchedBytes)
{
  char                  LogString[512];
  word                  i;
  int                   ret = 0;

  TRACEENTER();

//  #ifdef FULLDEBUG
    WriteLogMC("MovieCutterLib", "UnpatchRecFile()");

    TAP_SPrint(LogString, sizeof(LogString), "Patched Bytes: ");
    for (i = 0; i < NrPatchedBytes; i++)
    {
      TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), "%llu%s", PatchedBytes[i], (i < NrPatchedBytes-1) ? ", " : "");
      if ((i != NrPatchedBytes - 1) && ((i + 1) % 24 == 0))
      {
        WriteLogMC("MovieCutterLib", LogString);
        LogString[0] = '\0';
      }
    }
    WriteLogMC("MovieCutterLib", LogString);
//  #endif
    
  for (i = 0; i < NrPatchedBytes; i++)
  {
    byte newVal = 'G';
    if ((PACKETSIZE == 188) && (i == 2*CUTPOINTSECTORRADIUS-1 || i == 4*CUTPOINTSECTORRADIUS-1))
    {
      newVal = PatchedBytes[i] & 0xFF;
      PatchedBytes[i] = PatchedBytes[i] >> 8;
    }

    if (PatchedBytes[i] > 0)
    {
      if (PatchedBytes[i] < CutStartPos)
        ret = ret + !WriteByteToFile(SourceFileName, AbsDirectory, PatchedBytes[i], 'F', newVal);
      else if (PatchedBytes[i] < BehindCutPos)
        ret = ret + !WriteByteToFile(CutFileName, AbsDirectory, PatchedBytes[i] - CutStartPos, 'F', newVal);
      else
        ret = ret + !WriteByteToFile(SourceFileName, AbsDirectory, PatchedBytes[i] - (BehindCutPos - CutStartPos), 'F', newVal);
    }
  }

  TRACEEXIT();
  return (ret == 0);
}


// ----------------------------------------------------------------------------
//               Ermittlung der tats�chlichen Schnittposition
// ----------------------------------------------------------------------------
bool ReadCutPointArea(const char *SourceFileName, const char *AbsDirectory, off_t CutPosition, byte CutPointArray[])
{
  char                  AbsFileName[FBLIB_DIR_SIZE];
  FILE                 *f = NULL;
  size_t                ret;

  TRACEENTER();
  #ifdef FULLDEBUG
    WriteLogMC("MovieCutterLib", "ReadCutPointArea()");
  #endif

  // Open the rec for read access
  TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s/%s", AbsDirectory, SourceFileName);
  f = fopen(AbsFileName, "rb");
  if(f == 0)
  {
    WriteLogMC("MovieCutterLib", "ReadCutPointArea() E0201.");
    TRACEEXIT();
    return FALSE;
  }

  // Read the 2 blocks from the rec
  memset(CutPointArray, 0, CUTPOINTSEARCHRADIUS * 2);
  if (CutPosition >= CUTPOINTSEARCHRADIUS)
  {
    fseeko64(f, CutPosition - CUTPOINTSEARCHRADIUS, SEEK_SET);
    ret = fread(&CutPointArray[0], 1, CUTPOINTSEARCHRADIUS * 2, f);
  }
  else
  {
//    fseeko64(f, 0, SEEK_SET);
//    memset(CutPointArray, 0, CUTPOINTSEARCHRADIUS - CutPosition);
    ret = fread(&CutPointArray[CUTPOINTSEARCHRADIUS - CutPosition], 1, CutPosition + CUTPOINTSEARCHRADIUS, f);
  }
  fclose(f);

  if(!ret)
  {
    WriteLogMC("MovieCutterLib", "ReadCutPointArea() E0202.");
    TRACEEXIT();
    return FALSE;
  }

  TRACEEXIT();
  return TRUE;
}

bool ReadFirstAndLastCutPacket(const char *CutFileName, const char *AbsDirectory, byte FirstCutPacket[], byte LastCutPacket[])
{
  char                  AbsFileName[FBLIB_DIR_SIZE];
  FILE                 *f = NULL;
  size_t                ret;

  TRACEENTER();
  #ifdef FULLDEBUG
    WriteLogMC("MovieCutterLib", "ReadFirstAndLastCutPacket()");
  #endif

  // Open the rec for read access
  TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s/%s", AbsDirectory, CutFileName);
  f = fopen(AbsFileName, "rb");
  if(f == 0)
  {
    WriteLogMC("MovieCutterLib", "ReadFirstAndLastCutPacket() E0301.");
    TRACEEXIT();
    return FALSE;
  }

  // Read the beginning of the cut file
  if(FirstCutPacket)
  {
    ret = fread(&FirstCutPacket[0], PACKETSIZE, 1, f);
    if(!ret)
    {
      fclose(f);
      WriteLogMC("MovieCutterLib", "ReadFirstAndLastCutPacket() E0302.");
      TRACEEXIT();
      return FALSE;
    }
  }

  // Seek to the end of the cut file
  if(LastCutPacket)
  {
    fseeko64(f, -PACKETSIZE, SEEK_END);

    //Read the last TS packet
    ret = fread(&LastCutPacket[0], PACKETSIZE, 1, f);
    fclose(f);
    if(!ret)
    {
      WriteLogMC("MovieCutterLib", "ReadFirstAndLastCutPacket() E0303.");
      TRACEEXIT();
      return FALSE;
    }
  }

  TRACEEXIT();
  return TRUE;
}

// Searches the best occurance of CutPacket in CutPointArray (nearest to the middle).
// (search is no longer packet-based, because sometimes the firmware cuts somewhere in the middle of a packet)
// Returns true if found. Offset is 0, if CutPacket starts at requested position, -x if it starts x bytes before, +x if it starts x bytes after
bool FindCutPointOffset(const byte CutPacket[], const byte CutPointArray[], long *const OutOffset)
{
  const byte           *MidArray;
  byte                  FirstByte;
  ptrdiff_t             i;    // negative array indices might be critical on 64-bit systems! (http://www.devx.com/tips/Tip/41349)

  if (OutOffset == NULL) return FALSE;
  TRACEENTER();

  FirstByte = CutPacket[0];
  MidArray = &CutPointArray[CUTPOINTSEARCHRADIUS];
  *OutOffset = CUTPOINTSEARCHRADIUS + 1;

  for (i = -CUTPOINTSEARCHRADIUS; i <= (CUTPOINTSEARCHRADIUS - PACKETSIZE); i++)
  {
    if (MidArray[i] == FirstByte)
    {
      if (memcmp(&MidArray[i], CutPacket, PACKETSIZE) == 0)
      {
        if (labs(*OutOffset) < CUTPOINTSEARCHRADIUS)
        {
          WriteLogMC("MovieCutterLib", "FindCutPointOffset() W0401: cut packet found more than once.");
//          if (i > labs(*Offset)) break;
          *OutOffset = CUTPOINTSEARCHRADIUS + 1;
          break;
        }
        *OutOffset = i;
      }
    }
  }
  if (labs(*OutOffset) > CUTPOINTSEARCHRADIUS)
  {
    *OutOffset = 0;
    TRACEEXIT();
    return FALSE;
  }

  TRACEEXIT();
  return TRUE;
}

// Searches for the Cut-Point according to (a) the (buggy) Topfield method or (b) a working method
bool FindCutPointOffset2(const byte CutPointArray[], off_t RequestedCutPosition, bool CheckPacketStart, long *const OutOffset)
{
  const byte           *MidArray;
  int                   ArrayPos;
  int                   i;
  bool                  ret = FALSE;

  if (OutOffset == NULL) return FALSE;
  TRACEENTER();

  MidArray = &CutPointArray[CUTPOINTSEARCHRADIUS];
  *OutOffset = 0;

  // For each of the 4 (Austr: 48) possible cut positions
  for (i = 0; i < CUTPOINTSECTORRADIUS; i++)
  {
    ret = TRUE;
    ArrayPos = (int)(((RequestedCutPosition >> 12) << 12) - (i * 4096) - RequestedCutPosition);
    if (!CheckPacketStart) {
      if (MidArray[ArrayPos+4] == 'G') break;
    } else {
      if (isPacketStart(&MidArray[ArrayPos], CUTPOINTSEARCHRADIUS-ArrayPos)) break;
    }

    ArrayPos = (int)(((RequestedCutPosition >> 12) << 12) + ((i+1) * 4096) - RequestedCutPosition);
    if (!CheckPacketStart) {
      if (MidArray[ArrayPos+4] == 'G') break;
    } else {
      if (isPacketStart(&MidArray[ArrayPos], CUTPOINTSEARCHRADIUS-ArrayPos)) break;
    }
    ret = FALSE;
  }
  if (ret)
    *OutOffset = ArrayPos;

  TRACEEXIT();
  if (PACKETSIZE==188 && !CheckPacketStart && RequestedCutPosition==0)
    return TRUE;
  return ret;
}
/*bool FindCutPointOffset2(const byte CutPointArray[], off_t RequestedCutPosition, bool CheckPacketStart, long *const OutOffset)
{
  const byte           *MidArray;
  int                   ArrayPos;
  int                   i, j;
  bool                  isPacketStart;
  bool                  ret = FALSE;

  if (Offset == NULL) return FALSE;
  TRACEENTER();

  MidArray = &CutPointArray[CUTPOINTSEARCHRADIUS];
  *Offset = 0;

  // For each of the 4 (Austr: 48) possible cut positions
  for (i = -(CUTPOINTSECTORRADIUS-1); i <= CUTPOINTSECTORRADIUS; i++)
  {
    ArrayPos = (int)(((RequestedCutPosition >> 12) << 12) + (i * 4096) - RequestedCutPosition);

    // Check, if the current position is a sync-byte
    if ((MidArray[ArrayPos+SYNCBYTEPOS] == 'G'))
    {
      isPacketStart = TRUE;

      // Check, if the current position is a packet start (192 bytes per packet)
      if (!CheckPacketStart || (isPacketStart(&MidArray[ArrayPos], CUTPOINTSEARCHRADIUS-ArrayPos)))
      {
        *Offset = ArrayPos;
        ret = TRUE;
      }
    }
  }

  TRACEEXIT();
  return ret;
} */


// ----------------------------------------------------------------------------
//                              INF-Funktionen
// ----------------------------------------------------------------------------
bool GetRecDateFromInf(const char *RecFileName, const char *AbsDirectory, dword *const DateTime)
{
  FILE                 *f = NULL;
  char                  AbsInfName[FBLIB_DIR_SIZE];

  if (DateTime == NULL) return FALSE;
  TRACEENTER();

  TAP_SPrint(AbsInfName, sizeof(AbsInfName), "%s/%s.inf", AbsDirectory, RecFileName);
  f = fopen(AbsInfName, "rb");
  if(f)
  {
    fseek(f, 0x08, 0);
    fread(DateTime, sizeof(dword), 1, f);
    fclose(f);
  }
  else
  {
    WriteLogMC("MovieCutterLib", "GetRecDateFromInf() E0601.");
    TRACEEXIT();
    return FALSE;
  }

  TRACEEXIT();
  return TRUE;
}

/* bool SaveBookmarksToInf(const char *RecFileName, const char *AbsDirectory, const dword Bookmarks[], int NrBookmarks)
{
  char                  AbsInfName[FBLIB_DIR_SIZE];
  tRECHeaderInfo        RECHeaderInfo;
  byte                 *Buffer = NULL;
  FILE                 *fInf = NULL;
  dword                 BytesRead;
  bool                  ret = FALSE;
  int                   i;

  TRACEENTER();

  #ifdef FULLDEBUG
    TAP_PrintNet("SaveBookmarksToInf()\n");
    for (i = 0; i < NrBookmarks; i++) {
      TAP_PrintNet("%lu\n", Bookmarks[i]);
    }
  #endif

  //Allocate and clear the buffer
  Buffer = (byte*) TAP_MemAlloc(8192);
  if(Buffer) 
    memset(Buffer, 0, 8192);
  else
  {
    WriteLogMC("MovieCutterLib", "SaveBookmarksToInf() E0f01: failed to allocate the memory!");
    TRACEEXIT();
    return FALSE;
  }

  //Read inf
  TAP_SPrint(AbsInfName, sizeof(AbsInfName), "%s/%s.inf", AbsDirectory, RecFileName);
  fInf = fopen(AbsInfName, "r+b");
  if(!fInf)
  {
    WriteLogMC("MovieCutterLib", "SaveBookmarksToInf() E0f02: failed to open the inf file!");
    TAP_MemFree(Buffer);
    TRACEEXIT();
    return FALSE;
  }
  BytesRead = fread(Buffer, 1, INFSIZE, fInf);

  //decode inf
  if(!HDD_DecodeRECHeader(Buffer, &RECHeaderInfo, ST_UNKNOWN))
  {
    WriteLogMC("MovieCutterLib", "SaveBookmarksToInf() E0f03: decoding of rec-header failed.");
    fclose(fInf);
    TAP_MemFree(Buffer);
    TRACEEXIT();
    return FALSE;
  }

  //add new Bookmarks
  for (i = 0; i < NrBookmarks; i++) {
    RECHeaderInfo.Bookmark[i] = Bookmarks[i];
  }
  for (i = NrBookmarks; i < 177; i++) {
    RECHeaderInfo.Bookmark[i] = 0;
  }
  RECHeaderInfo.NrBookmarks = NrBookmarks;

  //encode and write inf
  if(HDD_EncodeRECHeader(Buffer, &RECHeaderInfo, ST_UNKNOWN))
  {
    fseek(fInf, 0, SEEK_SET);
    if (fwrite(Buffer, 1, INFSIZE, fInf) == INFSIZE)
      ret = TRUE;
  }
  else
    WriteLogMC("MovieCutterLib", "SaveBookmarksToInf() E0f04: failed to encode the new inf header!");

  fclose(fInf);
  TAP_MemFree(Buffer);

  TRACEEXIT();
  return ret;
} */

bool PatchInfFiles(const char *SourceFileName, const char *CutFileName, const char *AbsDirectory, dword SourcePlayTime, const tTimeStamp *CutStartPoint, const tTimeStamp *BehindCutPoint)
{
  char                  AbsSourceInfName[FBLIB_DIR_SIZE], AbsCutInfName[FBLIB_DIR_SIZE];
  char                  T1[16], T2[16], T3[16];
  char                  LogString[512];
  FILE                 *fSourceInf = NULL, *fCutInf = NULL;
  byte                 *Buffer = NULL;
  tRECHeaderInfo        RECHeaderInfo;
  dword                 BytesRead;
  dword                 Bookmarks[NRBOOKMARKS];
  dword                 NrBookmarks;
  dword                 CutPlayTime;
  dword                 OrigHeaderStartTime;
  word                  i;
  bool                  SetCutBookmark;
  bool                  Result = TRUE;

  TRACEENTER();
  #ifdef FULLDEBUG
//    WriteLogMC("MovieCutterLib", "PatchInfFiles()");
  #endif

  //Allocate and clear the buffer
  Buffer = (byte*) TAP_MemAlloc(8192);
  if(Buffer) 
    memset(Buffer, 0, 8192);
  else
  {
    WriteLogMC("MovieCutterLib", "PatchInfFiles() E0901: source inf not patched, cut inf not created.");
    TRACEEXIT();
    return FALSE;
  }

  //Read the source .inf
//  TAP_SPrint(SourceInfName, sizeof(SourceInfName), "%s.inf", SourceFileName);
//  tf = TAP_Hdd_Fopen(SourceInfName);
  TAP_SPrint(AbsSourceInfName, sizeof(AbsSourceInfName), "%s/%s.inf", AbsDirectory, SourceFileName);
  fSourceInf = fopen(AbsSourceInfName, "rb");
  if(!fSourceInf)
  {
    WriteLogMC("MovieCutterLib", "PatchInfFiles() E0902: source inf not patched, cut inf not created.");
    TAP_MemFree(Buffer);
    TRACEEXIT();
    return FALSE;
  }

//  fs = TAP_Hdd_Flen(tf);
//  ret = TAP_Hdd_Fread(Buffer, min(fs, INFSIZE), 1, tf);
//  TAP_Hdd_Fclose(tf);
  BytesRead = fread(Buffer, 1, INFSIZE, fSourceInf);
  #ifdef FULLDEBUG
    fseek(fSourceInf, 0, SEEK_END);
    dword fs = ftell(fSourceInf);
    fclose(fSourceInf);
    WriteLogMCf("MovieCutterLib", "PatchInfFiles(): %lu / %lu Bytes read.", BytesRead, fs);
  #endif

  //Decode the source .inf
  if(!HDD_DecodeRECHeader(Buffer, &RECHeaderInfo, ST_UNKNOWN))
  {
    WriteLogMC("MovieCutterLib", "PatchInfFiles() E0903: source inf not patched, cut inf not created.");
    TAP_MemFree(Buffer);
    TRACEEXIT();
    return FALSE;
  }

  //Calculate the new play times
  CutPlayTime = BehindCutPoint->Timems - CutStartPoint->Timems;
  MSecToTimeString(CutPlayTime, T2);
  if (SourcePlayTime)
  {
    MSecToTimeString(SourcePlayTime, T1);
    SourcePlayTime -= min(CutPlayTime, SourcePlayTime);
    MSecToTimeString(SourcePlayTime, T3);
    SourcePlayTime = (dword)((SourcePlayTime + 500) / 1000);
    CutPlayTime    = (dword)((CutPlayTime + 500) / 1000);
  }
  else
  {
    SourcePlayTime = 60 * RECHeaderInfo.HeaderDuration + RECHeaderInfo.HeaderDurationSec;
    SecToTimeString(SourcePlayTime, T1);
    CutPlayTime    = (dword)((SourcePlayTime + 500) / 1000);
    SourcePlayTime -= min(CutPlayTime, SourcePlayTime);
    SecToTimeString(SourcePlayTime, T3);
  }
  WriteLogMCf("MovieCutterLib", "Playtimes: Orig = %s, Cut = %s, New = %s", T1, T2, T3);

  //Change the new source play time
  RECHeaderInfo.HeaderDuration = (word)(SourcePlayTime / 60);
  RECHeaderInfo.HeaderDurationSec = SourcePlayTime % 60;

  //Set recording time of the source file
  OrigHeaderStartTime = RECHeaderInfo.HeaderStartTime;
  if (CutStartPoint->BlockNr == 0)
    RECHeaderInfo.HeaderStartTime = AddTime(OrigHeaderStartTime, BehindCutPoint->Timems / 60000);

  //Save all bookmarks to a temporary array
  memcpy(Bookmarks, RECHeaderInfo.Bookmark, NRBOOKMARKS * sizeof(dword));
  NrBookmarks = RECHeaderInfo.NrBookmarks;
  TAP_SPrint(LogString, sizeof(LogString), "Bookmarks: ");
  for(i = 0; i < NrBookmarks; i++)
  {
    if(Bookmarks[i] == 0) break;
    TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), "%lu ", Bookmarks[i]);
  }
  WriteLogMC("MovieCutterLib", LogString);

  //Clear all source Bookmarks
  memset(RECHeaderInfo.Bookmark, 0, NRBOOKMARKS * sizeof(dword));
  RECHeaderInfo.NrBookmarks = 0;
  RECHeaderInfo.Resume = 0;

  //Copy all bookmarks which are < CutPointA or >= CutPointB
  //Move the second group by (CutPointB - CutPointA)
  SetCutBookmark = TRUE;
  if ((CutStartPoint->BlockNr <= 1) || (CutStartPoint->Timems+3000 >= SourcePlayTime*1000) /*(BehindCutPoint->Timems+3000 >= (SourcePlayTime+CutPlayTime)*1000)*/)
    SetCutBookmark = FALSE;

  TAP_SPrint(LogString, sizeof(LogString), "Bookmarks->Source: ");
  for(i = 0; i < NrBookmarks; i++)
  {
    if(((Bookmarks[i] >= 100) && (Bookmarks[i] + 100 < CutStartPoint->BlockNr)) || (Bookmarks[i] >= BehindCutPoint->BlockNr + 100))
    {
      if(Bookmarks[i] < BehindCutPoint->BlockNr)
        RECHeaderInfo.Bookmark[RECHeaderInfo.NrBookmarks] = Bookmarks[i];
      else
      {
        if (SetCutBookmark)
        {
          RECHeaderInfo.Bookmark[RECHeaderInfo.NrBookmarks] = CutStartPoint->BlockNr;
          TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), "*%lu ", CutStartPoint->BlockNr);
          RECHeaderInfo.NrBookmarks++;
          SetCutBookmark = FALSE;
        }
        RECHeaderInfo.Bookmark[RECHeaderInfo.NrBookmarks] = Bookmarks[i] - (BehindCutPoint->BlockNr - CutStartPoint->BlockNr);
      }
      TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), "%lu ", RECHeaderInfo.Bookmark[RECHeaderInfo.NrBookmarks]);
      if (RECHeaderInfo.NrBookmarks < 48)
        RECHeaderInfo.NrBookmarks++;
    }
//    if(Bookmarks[i+1] == 0) break;
  }
  // Setzt automatisch ein Bookmark an die Schnittstelle
  if (SetCutBookmark)
  {
    RECHeaderInfo.Bookmark[RECHeaderInfo.NrBookmarks] = CutStartPoint->BlockNr;
    TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), "*%lu ", CutStartPoint->BlockNr);
    if (RECHeaderInfo.NrBookmarks < 48)
      RECHeaderInfo.NrBookmarks++;
  }
  WriteLogMC("MovieCutterLib", LogString);

  //Encode and write the modified source inf
  if(HDD_EncodeRECHeader(Buffer, &RECHeaderInfo, ST_UNKNOWN))
  {
//    TAP_SPrint(AbsSourceInfName, sizeof(AbsSourceInfName), "%s/%s.inf", AbsDirectory, SourceFileName);
    fSourceInf = fopen(AbsSourceInfName, "r+b");
    if(fSourceInf != 0)
    {
      fseek(fSourceInf, 0, SEEK_SET);
      fwrite(Buffer, 1, INFSIZE, fSourceInf);
      fclose(fSourceInf);
//      infData_Delete2(SourceFileName, AbsDirectory, INFFILETAG);
    }
    else
    {
      WriteLogMC("MovieCutterLib", "PatchInfFiles() W0902: source inf not patched.");
      Result = FALSE;
    }
  }
  else
  {
    WriteLogMC("MovieCutterLib", "PatchInfFiles() W0901: source inf not patched.");
    Result = FALSE;
  }

  // --- Patch the cut inf ---
  //Set the length of the cut file
  RECHeaderInfo.HeaderDuration = (word)(CutPlayTime / 60);
  RECHeaderInfo.HeaderDurationSec = CutPlayTime % 60;

  //Set recording time of the cut file
  RECHeaderInfo.HeaderStartTime = AddTime(OrigHeaderStartTime, CutStartPoint->Timems / 60000);

  //Clear all source Bookmarks
  memset(RECHeaderInfo.Bookmark, 0, NRBOOKMARKS * sizeof(dword));
  RECHeaderInfo.NrBookmarks = 0;

  //Copy all bookmarks which are >= CutPointA and < CutPointB
  //Move them by CutPointA
  TAP_SPrint(LogString, sizeof(LogString), "Bookmarks->Cut: ");
  for(i = 0; i < NrBookmarks; i++)
  {
    if((Bookmarks[i] >= CutStartPoint->BlockNr + 100) && (Bookmarks[i] + 100 < BehindCutPoint->BlockNr))
    {
      RECHeaderInfo.Bookmark[RECHeaderInfo.NrBookmarks] = Bookmarks[i] - CutStartPoint->BlockNr;
      TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), "%lu ", RECHeaderInfo.Bookmark[RECHeaderInfo.NrBookmarks]);
      RECHeaderInfo.NrBookmarks++;
    }
//    if(Bookmarks[i+1] == 0) break;
  }
  WriteLogMC("MovieCutterLib", LogString);

  //Encode the cut inf and write it to the disk
  if(HDD_EncodeRECHeader(Buffer, &RECHeaderInfo, ST_UNKNOWN))
  {
    TAP_SPrint(AbsCutInfName, sizeof(AbsCutInfName), "%s/%s.inf", AbsDirectory, CutFileName);
    fCutInf = fopen(AbsCutInfName, "wb");
    if(fCutInf != 0)
    {
      fseek(fCutInf, 0, SEEK_SET);
      fwrite(Buffer, 1, INFSIZE, fCutInf);

      // Kopiere den Rest der Source-inf (falls vorhanden) in die neue inf hinein
//      TAP_SPrint(AbsSourceInfName, sizeof(AbsSourceInfName), "%s/%s.inf", AbsDirectory, SourceFileName);
      fSourceInf = fopen(AbsSourceInfName, "rb");
      if(fSourceInf)
      {
        fseek(fSourceInf, INFSIZE, SEEK_SET);
        do {
          BytesRead = fread(Buffer, 1, 8192, fSourceInf);
          if (BytesRead > 0) fwrite(Buffer, BytesRead, 1, fCutInf);
        } while(BytesRead > 0);
        fclose(fSourceInf);
      }
      fclose(fCutInf);
    }
    else
    {
      WriteLogMC("MovieCutterLib", "PatchInfFiles() E0905: cut inf not created.");
      Result = FALSE;
    }
  }
  else
  {
    WriteLogMC("MovieCutterLib", "PatchInfFiles() E0904: cut inf not created.");
    Result = FALSE;
  }

  TAP_MemFree(Buffer);
  TRACEEXIT();
  return Result;
}


// ----------------------------------------------------------------------------
//                              NAV-Patchen
// ----------------------------------------------------------------------------
bool PatchNavFiles(const char *SourceFileName, const char *CutFileName, const char *AbsDirectory, off_t CutStartPos, off_t BehindCutPos, bool isHD, bool IgnoreRecordsAfterCut, dword *const OutCutStartTime, dword *const OutBehindCutTime, dword *const OutSourcePlayTime)
{
  char                  AbsFileName[FBLIB_DIR_SIZE];
  FILE                 *fOldNav = NULL, *fSourceNav = NULL, *fCutNav = NULL;
  tnavSD               *navOld = NULL, *navSource = NULL, *navCut=NULL;
  off_t                 PictureHeaderOffset = 0;
  size_t                navsRead, navRecsSource, navRecsCut, i;
  bool                  IFrameCut, IFrameSource;
  dword                 FirstCutTime, LastCutTime, FirstSourceTime, LastSourceTime;

  TRACEENTER();
  #ifdef FULLDEBUG
    WriteLogMC("MovieCutterLib", "PatchNavFiles()");
  #endif

  //Open the original nav
  TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s/%s.nav.bak", AbsDirectory, SourceFileName);
  fOldNav = fopen(AbsFileName, "rb");
  if(!fOldNav)
  {
    WriteLogMC("MovieCutterLib", "PatchNavFiles() E0701.");
    TRACEEXIT();
    return FALSE;
  }

  //Create and open the new source nav
  TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s/%s.nav", AbsDirectory, SourceFileName);
  fSourceNav = fopen(AbsFileName, "wb");
  if(!fSourceNav)
  {
    fclose(fOldNav);
    WriteLogMC("MovieCutterLib", "PatchNavFiles() E0702.");
    TRACEEXIT();
    return FALSE;
  }

  //Create and open the cut nav
  TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s/%s.nav", AbsDirectory, CutFileName);
  fCutNav = fopen(AbsFileName, "wb");
  if(!fCutNav)
  {
    fclose(fOldNav);
    fclose(fSourceNav);
    WriteLogMC("MovieCutterLib", "PatchNavFiles() E0703.");

    TRACEEXIT();
    return FALSE;
  }

  //Loop through the nav
  navOld = (tnavSD*) TAP_MemAlloc(NAVRECS_SD * sizeof(tnavSD));
  navSource = (tnavSD*) TAP_MemAlloc(NAVRECS_SD * sizeof(tnavSD));
  navCut = (tnavSD*) TAP_MemAlloc(NAVRECS_SD * sizeof(tnavSD));
  if(!navOld || !navSource || !navCut)
  {
    fclose(fOldNav);
    fclose(fSourceNav);
    fclose(fCutNav);
    TAP_MemFree(navOld);
    TAP_MemFree(navSource);
    TAP_MemFree(navCut);
    WriteLogMC("MovieCutterLib", "PatchNavFiles() E0704.");

    TRACEEXIT();
    return FALSE;
  }

  navRecsSource = 0;
  navRecsCut = 0;
  IFrameCut = FALSE;
  IFrameSource = TRUE;
  FirstCutTime = 0xFFFFFFFF;
  LastCutTime = 0;
  FirstSourceTime = 0;
  LastSourceTime = 0;
  bool FirstRun = TRUE;
  while(TRUE)
  {
    navsRead = fread(navOld, sizeof(tnavSD), NAVRECS_SD, fOldNav);
    if(navsRead == 0) break;

    // Versuche, nav-Dateien aus Timeshift-Aufnahmen zu unterst�tzen ***experimentell***
    if(FirstRun)
    {
      FirstRun = FALSE;
      if(navOld[0].SHOffset == 0x72767062)  // 'bpvr'
      {
        fseek(fOldNav, 1056, SEEK_SET);
        continue;
      }
    }

    for(i = 0; i < navsRead; i++)
    {
      // Falls HD: Betrachte jeden tnavHD-Record als 2 tnavSD-Records, verwende den ersten und �berspringe den zweiten
      if (isHD && (i % 2 != 0)) continue;

      //Check if the entry lies between the CutPoints
      PictureHeaderOffset = ((off_t)(navOld[i].PHOffsetHigh) << 32) | navOld[i].PHOffset;
      if((PictureHeaderOffset >= CutStartPos) && (PictureHeaderOffset < BehindCutPos))
      {
        //nav entries for the cut file
        if(navRecsCut >= NAVRECS_SD)
        {
          fwrite(navCut, sizeof(tnavSD), navRecsCut, fCutNav);
          navRecsCut = 0;
        }

        if (FirstCutTime == 0xFFFFFFFF) 
        {
          if (CutStartPos == 0)
            FirstCutTime = 0;
          else
            FirstCutTime = navOld[i].Timems;
        }
        LastCutTime = navOld[i].Timems;

        //Subtract CutStartPos from the cut .nav PH address
        PictureHeaderOffset = PictureHeaderOffset - CutStartPos;
        if((navOld[i].SHOffset >> 24) == 1) IFrameCut = TRUE;
        if(IFrameCut)
        {
          memcpy(&navCut[navRecsCut], &navOld[i], sizeof(tnavSD));
          navCut[navRecsCut].PHOffsetHigh = PictureHeaderOffset >> 32;
          navCut[navRecsCut].PHOffset = PictureHeaderOffset & 0xffffffff;
          navCut[navRecsCut].Timems = navCut[navRecsCut].Timems - FirstCutTime;
          navRecsCut++;

          if (isHD)
          {
            memcpy(&navCut[navRecsCut], &navOld[i+1], sizeof(tnavSD));
            navRecsCut++;
          }
        }
        IFrameSource = FALSE;
      }
      else
      {
        //nav entries for the new source file
        if(navRecsSource >= NAVRECS_SD)
        {
          fwrite(navSource, sizeof(tnavSD), navRecsSource, fSourceNav);
          navRecsSource = 0;
        }

        if (PictureHeaderOffset >= BehindCutPos)
        {
          if (FirstSourceTime == 0) FirstSourceTime = navOld[i].Timems;
          LastSourceTime = navOld[i].Timems;
          if (IgnoreRecordsAfterCut) break;
        }

        if((navOld[i].SHOffset >> 24) == 1) IFrameSource = TRUE;
        if(IFrameSource)
        {
          memcpy(&navSource[navRecsSource], &navOld[i], sizeof(tnavSD));

          //if ph offset >= BehindCutPos, subtract (BehindCutPos - CutStartPos)
          if(PictureHeaderOffset >= BehindCutPos)
          {
            PictureHeaderOffset = PictureHeaderOffset - (BehindCutPos - CutStartPos);
            navSource[navRecsSource].PHOffsetHigh = PictureHeaderOffset >> 32;
            navSource[navRecsSource].PHOffset = PictureHeaderOffset & 0xffffffff;
            navSource[navRecsSource].Timems = navSource[navRecsSource].Timems - (FirstSourceTime - FirstCutTime);
          }
          navRecsSource++;

          if (isHD)
          {
            memcpy(&navSource[navRecsSource], &navOld[i+1], sizeof(tnavSD));
            navRecsSource++;
          }
        }
        IFrameCut = FALSE;
      }
    }
    if (IgnoreRecordsAfterCut && (PictureHeaderOffset >= BehindCutPos))
      break;
  }

  if(navRecsCut > 0) fwrite(navCut, sizeof(tnavSD), navRecsCut, fCutNav);
  if(navRecsSource > 0) fwrite(navSource, sizeof(tnavSD), navRecsSource, fSourceNav);

  fclose(fOldNav);
  fclose(fCutNav);
  fclose(fSourceNav);

  TAP_MemFree(navOld);
  TAP_MemFree(navSource);
  TAP_MemFree(navCut);

  if (FirstCutTime != 0xFFFFFFFF)
  {
    if(OutCutStartTime)
      *OutCutStartTime = FirstCutTime;
    if(OutBehindCutTime)
      *OutBehindCutTime = (FirstSourceTime) ? FirstSourceTime : LastCutTime;
    if(OutSourcePlayTime)
      *OutSourcePlayTime = (LastSourceTime) ? LastSourceTime : LastCutTime;
  }

  //Delete the orig source nav and make the new source nav the active one
//  TAP_SPrint(FileName, sizeof(FileName), "%s.nav.bak", SourceFileName);
//  TAP_Hdd_Delete(FileName);

  TRACEEXIT();
  return TRUE;
}


// ----------------------------------------------------------------------------
//                               NAV-Einlesen
// ----------------------------------------------------------------------------
tTimeStamp* NavLoad(const char *RecFileName, const char *AbsDirectory, int *const OutNrTimeStamps, bool isHD)
{
  char                  AbsFileName[FBLIB_DIR_SIZE];
  FILE                 *fNav = NULL;
  tnavSD               *navBuffer = NULL;
  tTimeStamp           *TimeStampBuffer = NULL;
  tTimeStamp           *TimeStamps = NULL;
  int                   NrTimeStamps = 0;
  dword                 NavRecordsNr;
  dword                 ret, i;
  dword                 FirstTime;
  dword                 LastTimeStamp;
  ulong64               AbsPos;
  dword                 NavSize;

  TRACEENTER();
  if(OutNrTimeStamps) *OutNrTimeStamps = 0;

  // Open the nav file
  TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s/%s.nav", AbsDirectory, RecFileName);
  fNav = fopen(AbsFileName, "rb");
  if(!fNav)
  {
    WriteLogMC("MovieCutterLib", "NavLoad() E0b01");
    TRACEEXIT();
    return(NULL);
  }

  // Reserve a (temporary) buffer to hold the entire file
  fseek(fNav, 0, SEEK_END);
  NavSize = ftell(fNav);
  rewind(fNav);
  NavRecordsNr = NavSize / (sizeof(tnavSD) * ((isHD) ? 2 : 1));

  TimeStampBuffer = (tTimeStamp*) TAP_MemAlloc(NavRecordsNr * sizeof(tTimeStamp));
  if (!TimeStampBuffer)
  {
    fclose(fNav);
    WriteLogMC("MovieCutterLib", "NavLoad() E0b02");
    TRACEEXIT();
    return(NULL);
  }

  //Count and save all the _different_ time stamps in the .nav
  LastTimeStamp = 0xFFFFFFFF;
  FirstTime = 0xFFFFFFFF;
  navBuffer = (tnavSD*) TAP_MemAlloc(NAVRECS_SD * sizeof(tnavSD));
  if (!navBuffer)
  {
    fclose(fNav);
    TAP_MemFree(TimeStampBuffer);
    WriteLogMC("MovieCutterLib", "NavLoad() E0b03");

    TRACEEXIT();
    return(NULL);
  }
  do
  {
    ret = fread(navBuffer, sizeof(tnavSD), NAVRECS_SD, fNav);
    if(ret == 0) break;

    // Versuche, nav-Dateien aus Timeshift-Aufnahmen zu unterst�tzen ***experimentell***
    if(FirstTime == 0xFFFFFFFF)
    {
      if(navBuffer[0].SHOffset == 0x72767062)  // 'bpvr'
      {
        fseek(fNav, 1056, SEEK_SET);
        continue;
      }
    }

    for(i = 0; i < ret; i++)
    {
      // Falls HD: Betrachte jeden tnavHD-Record als 2 tnavSD-Records, verwende den ersten und �berspringe den zweiten
      if (isHD && (i % 2 != 0)) continue;

      if(FirstTime == 0xFFFFFFFF) FirstTime = navBuffer[i].Timems;
      if(LastTimeStamp != navBuffer[i].Timems)
      {
        AbsPos = ((ulong64)(navBuffer[i].PHOffsetHigh) << 32) | navBuffer[i].PHOffset;
        TimeStampBuffer[NrTimeStamps].BlockNr = CalcBlockSize(AbsPos);
        TimeStampBuffer[NrTimeStamps].Timems = navBuffer[i].Timems;

/*        if (navBuffer[i].Timems >= FirstTime)
          // Timems ist gr��er als FirstTime -> kein �berlauf
          TimeStampBuffer[*NrTimeStamps].Timems = navBuffer[i].Timems - FirstTime;
        else if (FirstTime - navBuffer[i].Timems <= 3000)
          // Timems ist kaum kleiner als FirstTime -> liegt vermutlich am Anfang der Aufnahme
          TimeStampBuffer[*NrTimeStamps].Timems = 0;
        else
          // Timems ist (deutlich) kleiner als FirstTime -> ein �berlauf liegt vor
          TimeStampBuffer[*NrTimeStamps].Timems = (0xffffffff - FirstTime) + navBuffer[i].Timems + 1;
*/
        (NrTimeStamps)++;
        LastTimeStamp = navBuffer[i].Timems;
      }
    }
  } while(ret == NAVRECS_SD);

  // Free the nav-Buffer and close the file
  fclose(fNav);
  TAP_MemFree(navBuffer);

  // Reserve a new buffer of the correct size to hold only the different time stamps
  TimeStamps = (tTimeStamp*) TAP_MemAlloc(NrTimeStamps * sizeof(tTimeStamp));
  if(!TimeStamps)
  {
    TAP_MemFree(TimeStampBuffer);
    WriteLogMC("MovieCutterLib", "NavLoad() E0b04");
    TRACEEXIT();
    return(NULL);
  }

  // Copy the time stamps to the new array
  memcpy(TimeStamps, TimeStampBuffer, NrTimeStamps * sizeof(tTimeStamp));  
  TAP_MemFree(TimeStampBuffer);
  *OutNrTimeStamps = NrTimeStamps;

  TRACEEXIT();
  return(TimeStamps);
}
