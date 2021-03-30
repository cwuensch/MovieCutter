#define _LARGEFILE_SOURCE   1
#define _LARGEFILE64_SOURCE 1
#define _FILE_OFFSET_BITS  64
#ifdef _MSC_VER
  #define __const const
#endif
//#define STACKTRACE      TRUE

#define _GNU_SOURCE
#include                <stdio.h>
#include                <stdlib.h>
#include                <string.h>
#include                <unistd.h>
#include                <fcntl.h>
#include                <sys/stat.h>
#include                <tap.h>
#include                "libFireBird.h"
#include                "RecHeader.h"
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
static bool  PatchInfFiles(const char *SourceFileName, const char *CutFileName, const char *AbsDirectory, dword SourcePlayTime, const tTimeStamp *CutStartPoint, const tTimeStamp *BehindCutPoint, char *pCutCaption, char *pSourceCaption);
static bool  PatchNavFiles(const char *SourceFileName, const char *CutFileName, const char *AbsDirectory, off_t CutStartPos, off_t BehindCutPos, bool isHD, bool IgnoreRecordsAfterCut, dword *const OutCutStartTime, dword *const OutBehindCutTime, dword *const OutSourcePlayTime);

static int              PACKETSIZE = 192;
static int              SYNCBYTEPOS = 4;
static int              CUTPOINTSECTORRADIUS = 2;
int                     CUTPOINTSEARCHRADIUS = 9024;


// ----------------------------------------------------------------------------
//                           Hilfsfunktionen
// ----------------------------------------------------------------------------
void CreateSettingsDir(void)
{
  TRACEENTER();

//  struct stat64 st;
//  if (lstat64(TAPFSROOT "/ProgramFiles/Settings", &st) == -1)
    mkdir(TAPFSROOT "/ProgramFiles/Settings", 0666);
//  if (lstat64(TAPFSROOT "/ProgramFiles/Settings/MovieCutter", &st) == -1)
    mkdir(TAPFSROOT "/ProgramFiles/Settings/MovieCutter", 0666);

  TRACEEXIT();
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
  char                  CheckFileName[MAX_FILE_NAME_SIZE], *p;
  size_t                NameLen, ExtStart;
  int                   FreeIndices = 0, i = 0;

  TRACEENTER();
  if(OutCutFileName) OutCutFileName[0] = '\0';

  if (SourceFileName && OutCutFileName)
  {
    p = strrchr(SourceFileName, '.');  // ".rec" entfernen
    NameLen = ExtStart = ((p) ? (size_t)(p - SourceFileName) : strlen(SourceFileName));
//    if((p = strstr(&SourceFileName[NameLen - 10], " (Cut-")) != NULL)
//      NameLen = p - SourceFileName;        // wenn schon ein ' (Cut-xxx)' vorhanden ist, entfernen
    strncpy(CheckFileName, SourceFileName, NameLen);

    do
    {
      i++;
      TAP_SPrint(&CheckFileName[NameLen], sizeof(CheckFileName) - NameLen, " (Cut-%d)%s", i, &SourceFileName[ExtStart]);
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
tResultCode MovieCutter(char *SourceFileName, char *CutFileName, char *AbsDirectory, tTimeStamp *CutStartPoint, tTimeStamp *BehindCutPoint, bool KeepCut, bool isHD, char *pCutCaption, char *pSourceCaption)
{
  char                  FileName[MAX_FILE_NAME_SIZE];
  off_t                 SourceFileSize, CutFileSize;
  __ino64_t             InodeNr;
  dword                 MaxBehindCutBlock;
  off_t                 CutStartPos, BehindCutPos;
  dword                 SourcePlayTime = 0;
  bool                  TruncateEnding = FALSE;
  bool                  SuppressNavGeneration = FALSE;
  tPVRTime              RecDate = 0;
  byte                  RecDateSec = 0;
//  char                  TimeStr[16];

  TRACEENTER();

  // LOG file printing
  WriteLogMC ("MovieCutterLib", "----------------------------------------");
  WriteLogMCf("MovieCutterLib", "Source        = '%s'", SourceFileName);
  WriteLogMCf("MovieCutterLib", "Cut name      = '%s'", CutFileName);

  if (!HDD_GetFileSizeAndInode2(SourceFileName, AbsDirectory, &InodeNr, &SourceFileSize))
  {
    WriteLogMC("MovieCutterLib", "MovieCutter() E0001: cut file not created.");
    TRACEEXIT();
    return RC_Error;
  }

  WriteLogMCf("MovieCutterLib", "Inode Nr.     = %llu", InodeNr);
  WriteLogMCf("MovieCutterLib", "File size     = %llu Bytes (%lu blocks)", SourceFileSize, CalcBlockSize(SourceFileSize));
  WriteLogMCf("MovieCutterLib", "AbsDir        = '%s'", AbsDirectory);

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
//    FindCutPointOffset2(CutPointArea1, CutStartPos, TRUE, &CutStartPosOffset);         // unnötig?
    CutStartPos = CutStartPos + 9024;
    BehindCutPos = SourceFileSize;
//    BehindCutPosOffset = BehindCutPos - ((off_t)BehindCutPoint->BlockNr * BLOCKSIZE);  // unnötig!
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
    if(!SuppressNavGeneration)
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
          CutStartPosOffset = CutStartPos - ((off_t)CutStartPoint->BlockNr * BLOCKSIZE);  // unnötig
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
          BehindCutPosOffset = BehindCutPos - ((off_t)BehindCutPoint->BlockNr * BLOCKSIZE);  // unnötig
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
  bool  CutStartGuessed = FALSE,   BehindCutGuessed = FALSE;
  long  GuessedCutStartOffset = 0, GuessedBehindCutOffset = 0;
  off_t ReqCutStartPos  = (off_t)CutStartPoint->BlockNr * BLOCKSIZE;
  off_t ReqBehindCutPos = (off_t)BehindCutPoint->BlockNr * BLOCKSIZE;

  if(CutPointArea1)
    CutStartGuessed  = FindCutPointOffset2(CutPointArea1, ReqCutStartPos, FALSE, &GuessedCutStartOffset);
  if(CutPointArea2)
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
    CutStartPoint->BlockNr = CalcBlockSize(CutStartPos + BLOCKSIZE-1);  // erster vollständiger Block des CutFile
    BehindCutPoint->BlockNr = CutStartPoint->BlockNr + CalcBlockSize(BehindCutPos - CutStartPos + BLOCKSIZE/2);
  }

  // Truncate the source rec-File
  if(!SuppressNavGeneration && TruncateEnding)
  {
    if(!RecTruncate(SourceFileName, AbsDirectory, CutStartPos))
    {
      WriteLogMC("MovieCutterLib", "MovieCutter() E0004: Truncate routine failed.");
      if(!KeepCut)
      {
        TRACEEXIT();
        return RC_Error;
      }
    }
  }

  // Rename old nav file to bak
  char BakFileName[MAX_FILE_NAME_SIZE];
  TAP_SPrint(FileName, sizeof(FileName), "%s.nav", SourceFileName);
  TAP_SPrint(BakFileName, sizeof(BakFileName), "%s.nav.bak", SourceFileName);
  if (HDD_Exist2(FileName, AbsDirectory))
  {
    if (HDD_Exist2(BakFileName, AbsDirectory))
      HDD_Delete2(BakFileName, AbsDirectory, FALSE, FALSE);
    HDD_Rename2(FileName, BakFileName, AbsDirectory, FALSE, FALSE);
  }

  // Patch the nav files (and get the TimeStamps for the actual cutting positions)
  if(!SuppressNavGeneration)
  {
    if(PatchNavFiles(SourceFileName, CutFileName, AbsDirectory, CutStartPos, BehindCutPos, isHD, TruncateEnding, &(CutStartPoint->Timems), &(BehindCutPoint->Timems), &SourcePlayTime))
      HDD_Delete2(BakFileName, AbsDirectory, FALSE, FALSE);
    else
    {
      WriteLogMC("MovieCutterLib", "MovieCutter() W0009: nav creation failed.");
      SuppressNavGeneration = TRUE;
      TAP_SPrint(FileName, sizeof(FileName), "%s.nav", SourceFileName);
      HDD_Delete2(FileName, AbsDirectory, FALSE, FALSE);
      TAP_SPrint(FileName, sizeof(FileName), "%s.nav", CutFileName);
      HDD_Delete2(FileName, AbsDirectory, FALSE, FALSE);
    }
  }

  // Copy the inf file and patch both play lengths
  if (!PatchInfFiles(SourceFileName, CutFileName, AbsDirectory, SourcePlayTime, CutStartPoint, BehindCutPoint, pCutCaption, pSourceCaption))
    WriteLogMC("MovieCutterLib", "MovieCutter() W0010: inf creation failed.");

  // Fix the date info of all involved files
  if (GetRecInfosFromInf(SourceFileName, AbsDirectory, NULL, NULL, NULL, &RecDate, &RecDateSec)) {
    //Source
    char LogString[512];
    LogString[0] = '\0';
    if (!HDD_SetFileDateTime(SourceFileName, AbsDirectory, RecDate, RecDateSec))
      TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), "HDD_SetFileDateTime(%s) failed. ", SourceFileName);
    TAP_SPrint(FileName, sizeof(FileName), "%s.inf", SourceFileName);
    if (!HDD_SetFileDateTime(FileName, AbsDirectory, RecDate, RecDateSec))
      TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), "HDD_SetFileDateTime(%s) failed. ", FileName);
    TAP_SPrint(FileName, sizeof(FileName), "%s.nav", SourceFileName);
    if (!HDD_SetFileDateTime(FileName, AbsDirectory, RecDate, RecDateSec))
      TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), "HDD_SetFileDateTime(%s) failed. ", FileName);
    if(LogString[0])
      WriteLogMC("MovieCutterLib", LogString);
  }
  if (KeepCut && GetRecInfosFromInf(CutFileName, AbsDirectory, NULL, NULL, NULL, &RecDate, &RecDateSec)) {
    //Cut
    char LogString[512];
    LogString[0] = '\0';
    if (!HDD_SetFileDateTime(CutFileName, AbsDirectory, RecDate, RecDateSec))
      TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), "HDD_SetFileDateTime(%s) failed. ", CutFileName);
    TAP_SPrint(FileName, sizeof(FileName), "%s.inf", CutFileName);
    if (!HDD_SetFileDateTime(FileName, AbsDirectory, RecDate, RecDateSec))
      TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), "HDD_SetFileDateTime(%s) failed. ", FileName);
    TAP_SPrint(FileName, sizeof(FileName), "%s.nav", CutFileName);
    if (!HDD_SetFileDateTime(FileName, AbsDirectory, RecDate, RecDateSec))
      TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), "HDD_SetFileDateTime(%s) failed. ", FileName);
    if(LogString[0])
      WriteLogMC("MovieCutterLib", LogString);
  }
//  if(!KeepSource) HDD_Delete2(SourceFileName, Directory, TRUE, TRUE);
  if(!KeepCut)
    HDD_Delete2(CutFileName, AbsDirectory, TRUE, TRUE);

  WriteLogMC("MovieCutterLib", "MovieCutter() finished.");

  TRACEEXIT();
  return ((SuppressNavGeneration) ? RC_Warning : RC_Ok);
}

// ----------------------------------------------------------------------------
//              Firmware-Funktion zum Durchführen des Schnitts
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
  if(PlayInfo.playMode == PLAYMODE_Playing || PlayInfo.playMode == 8)
  {
    Appl_StopPlaying();
    Appl_WaitEvt(0xE507, &x, 1, 0xFFFFFFFF, 300);
  }
  HDD_Delete2(CutFileName, AbsDirectory, TRUE, TRUE);
  CloseLogMC();

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
        CloseLogMC();
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
//  sync();
//  TAP_Sleep(1);
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
  if(PlayInfo.playMode == PLAYMODE_Playing || PlayInfo.playMode == 8)
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
int GetPacketSize(const char *RecFileName, const char *AbsDirectory)
{
  char                 *p;
  bool                  ret = FALSE;
  TRACEENTER();

  p = strrchr(RecFileName, '.');
  if (p && strcmp(p, ".rec") == 0)
  {
    PACKETSIZE = 192;
    SYNCBYTEPOS = 4;
    ret = TRUE;
  }
  else
  {
    char                AbsRecName[FBLIB_DIR_SIZE];
    byte               *RecStartArray = NULL;
    int                 f = -1;

    TAP_SPrint(AbsRecName, sizeof(AbsRecName), "%s/%s", AbsDirectory, RecFileName);

    RecStartArray = (byte*) TAP_MemAlloc(1733);  // 1733 = 9*192 + 5
    if (RecStartArray)
    {
      f = open(AbsRecName, O_RDONLY);
      if(f >= 0)
      {
        if (read(f, RecStartArray, 1733) == 1733)
        {
          PACKETSIZE = 188;
          SYNCBYTEPOS = 0;
          ret = isPacketStart(RecStartArray, 1733);

          if (!ret)
          {
            PACKETSIZE = 192;
            SYNCBYTEPOS = 4;
            ret = isPacketStart(RecStartArray, 1733);
          }
        }
        close(f);
      }
      TAP_MemFree(RecStartArray);
    }
  }

  if (PACKETSIZE == 192)
  {
    CUTPOINTSEARCHRADIUS = 9024;
    CUTPOINTSECTORRADIUS = CUTPOINTSEARCHRADIUS/4096;  // 2
  }
  else
  {
    CUTPOINTSEARCHRADIUS = 99264;
    CUTPOINTSECTORRADIUS = CUTPOINTSEARCHRADIUS/4096;  // 24
  }

  TRACEEXIT();
  return (ret ? PACKETSIZE : 0);
}

// ACHTUNG! Schlägt fehl, wenn der Header durch Paketgrenze unterbrochen wird (-> kommt vor, klappt aber meistens!)
static off_t FindNextIFrame(const char *RecFileName, const char *AbsDirectory, dword BlockNr, bool isHD)
{
  FILE                 *fRec = NULL;
  byte                 *Buffer = NULL;
  char                  AbsRecName[FBLIB_DIR_SIZE];
  dword                *Header;
  byte                 *p;
  int                   BytesRead;
  bool                  IFrameFound = FALSE;
  off_t                 pos, SEI = 0;

  TRACEENTER();

  TAP_SPrint(AbsRecName, sizeof(AbsRecName), "%s/%s", AbsDirectory, RecFileName);
  pos = (__off64_t)BlockNr * 9024;

  Buffer = (byte*)TAP_MemAlloc(9024);
  if (Buffer)
  {
    fRec = fopen64(AbsRecName, "rb");
    if (fRec)
    {
      if (fseeko64(fRec, pos, SEEK_SET) == 0)
      {
        while ((!IFrameFound || !SEI) && (BytesRead = fread(Buffer, 1, 9024, fRec)) > 0)
        {
          for (p = Buffer; p < &Buffer[BytesRead-5]; p++)
          {
            Header = (dword*)p;
            if (isHD)
            {
              // HD:
              // Suche nach erstem 00 00 01 06  nach  00 00 01 08
              // mit               FF FF FF 9F        FF FF FF 9F
              //
              // oder nach erstem 00 00 01 09 00
              // mit              FF FF FF 9F E0

              if ((*Header & 0x9fffffff) == 0x08010000)
              {
                IFrameFound = TRUE;
                SEI = pos + (p - Buffer);
              }
              else if (IFrameFound && ((*Header & 0x9fffffff) == 0x06010000))
              {
                if (pos + (p - Buffer) <= SEI + 3*192)
                  SEI = pos + (p - Buffer);
                break;
              }
            }
            else
            {
              // SD:
              // Suche nach letztem 00 00 01 B3  vor  00 00 01 00 xx 08
              // mit                                  FF FF FF FF 00 F8
              // (dword == 0x00010000) && (word & 0xf800 == 0x0800)

              if (*Header == 0xB3010000)
                SEI = pos + (p - Buffer);
              else if (SEI)
              {
                IFrameFound = ((*Header == 0x00010000) && (((p[5] >> 3) & 0x03) == 1));
                break;
              }
            }
          }
          if (!IFrameFound || !SEI)
            pos += BytesRead;
        }
      }
      fclose(fRec);
    }
    TAP_MemFree(Buffer);
  }

  TRACEEXIT();
  return (IFrameFound) ? SEI : 0;
}

bool isNavAvailable(const char *RecFileName, const char *AbsDirectory)
{
  char                  NavFileName[MAX_FILE_NAME_SIZE];
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
  int                   f = -1;
  bool                  ret = FALSE;

  TRACEENTER();
  #ifdef FULLDEBUG
    WriteLogMCf("MovieCutterLib", "WriteByteToFile(file='%s', position=%llu, old=%#4hhx, new=%#4hhx).", FileName, BytePosition, OldValue, NewValue);
  #endif

  // Open the file for write access
  TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s/%s", AbsDirectory, FileName);
  f = open64(AbsFileName, O_RDWR | O_LARGEFILE);
  if(f < 0)
  {
    WriteLogMC("MovieCutterLib", "WriteByteToFile() E0e01.");
    TRACEEXIT();
    return FALSE;
  }

  // Seek to the desired position
  if (lseek64(f, BytePosition, SEEK_SET) != BytePosition)
  {
    close(f);
    WriteLogMC("MovieCutterLib", "WriteByteToFile() E0e02.");
    TRACEEXIT();
    return FALSE;
  }

  // Check, if the old value is correct
  char old = 0;
  read(f, &old, 1);
  #ifdef FULLDEBUG
    if ((NewValue == 'G') && (old != OldValue))
      WriteLogMCf("MovieCutterLib", "Restore Sync-Byte: value read from cache=%#4hhx (expected %#4hhx).", old, OldValue);
  #endif
  if ((old != OldValue) && (old != 'G'))
  {
    close(f);
    WriteLogMC("MovieCutterLib", "WriteByteToFile() E0e03.");
    TRACEEXIT();
    return FALSE;
  }

  // Write the new byte to the file
  if (lseek64(f, -1, SEEK_CUR) == BytePosition)
    ret = (write(f, &NewValue, 1) == 1);
  ret = (fsync(f) == 0) && ret;
  ret = (close(f) == 0) && ret;
  if(!ret)
  {
    WriteLogMC("MovieCutterLib", "WriteByteToFile() E0e04.");
    TRACEEXIT();
    return FALSE;
  }

  // Print to log
  WriteLogMCf("MovieCutterLib", "%s Sync-Byte: File '%s', position %llu: Changed value %#4hhx -> %#4hhx.", ((NewValue=='G') ? "Restore" : "Remove"), FileName, BytePosition, OldValue, NewValue);
/* if (NewValue != 'G')
    WriteLogMCf("MovieCutterLib", "Remove Sync-Byte: Changed value in file '%s' at position %llu from %#4hhx to %#4hhx.", FileName, BytePosition, OldValue, NewValue);
  else
    WriteLogMCf("MovieCutterLib", "Restore Sync-Byte: Changed value in file '%s' at position %llu from %#4hhx to %#4hhx.", FileName, BytePosition, OldValue, NewValue);
*/

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

  if (CutPointArray == NULL || OutPatchedBytes == NULL) return FALSE;
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
          MidArray[ArrayPos+4] = 'F';    // ACHTUNG! Nötig, damit die neue Schätzung der CutPosition korrekt funktioniert.
        }                                // Könnte aber ein Problem geben bei der (alten) CutPoint-Identifikation (denn hier wird der noch ungepatchte Wert aus dem Cache gelesen)!
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
            OutPatchedBytes[2*CUTPOINTSECTORRADIUS - 1] = ((pos+4) << 8) + oldVal;  // missbrauche die letzte Position für den Schnitt-Patch
            MidArray[ArrayPos+4] = 'G';    // ACHTUNG! Nötig, damit die neue Schätzung der CutPosition korrekt funktioniert.
          }                                // Könnte aber ein Problem geben bei der (alten) CutPoint-Identifikation (denn hier wird der noch ungepatchte Wert aus dem Cache gelesen)!
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
//               Ermittlung der tatsächlichen Schnittposition
// ----------------------------------------------------------------------------
bool ReadCutPointArea(const char *SourceFileName, const char *AbsDirectory, off_t CutPosition, byte CutPointArray[])
{
  char                  AbsFileName[FBLIB_DIR_SIZE];
  int                   f = -1;
  bool                  ret = FALSE;

  if (CutPointArray == NULL) return FALSE;
  TRACEENTER();
  #ifdef FULLDEBUG
    WriteLogMC("MovieCutterLib", "ReadCutPointArea()");
  #endif

  // Open the rec for read access
  TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s/%s", AbsDirectory, SourceFileName);
  f = open64(AbsFileName, O_RDONLY | O_LARGEFILE);
  if(f < 0)
  {
    WriteLogMC("MovieCutterLib", "ReadCutPointArea() E0201.");
    TRACEEXIT();
    return FALSE;
  }

  // Read the 2 blocks from the rec
  memset(CutPointArray, 0, CUTPOINTSEARCHRADIUS * 2);
  if (CutPosition >= CUTPOINTSEARCHRADIUS)
  {
    if (lseek64(f, CutPosition - CUTPOINTSEARCHRADIUS, SEEK_SET) == CutPosition - CUTPOINTSEARCHRADIUS)
      ret = (read(f, &CutPointArray[0], CUTPOINTSEARCHRADIUS * 2) == CUTPOINTSEARCHRADIUS * 2);
  }
  else
  {
//    fseeko64(f, 0, SEEK_SET);
//    memset(CutPointArray, 0, CUTPOINTSEARCHRADIUS - CutPosition);
    ret = (read(f, &CutPointArray[CUTPOINTSEARCHRADIUS - CutPosition], CutPosition + CUTPOINTSEARCHRADIUS) == CutPosition + CUTPOINTSEARCHRADIUS);
  }
  close(f);

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
  int                   f = -1;
  bool                  ret = FALSE;

  TRACEENTER();
  #ifdef FULLDEBUG
    WriteLogMC("MovieCutterLib", "ReadFirstAndLastCutPacket()");
  #endif

  // Open the rec for read access
  TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s/%s", AbsDirectory, CutFileName);
  f = open64(AbsFileName, O_RDONLY | O_LARGEFILE);
  if(f < 0)
  {
    WriteLogMC("MovieCutterLib", "ReadFirstAndLastCutPacket() E0301.");
    TRACEEXIT();
    return FALSE;
  }

  // Read the beginning of the cut file
  if(FirstCutPacket)
  {
    ret = (read(f, &FirstCutPacket[0], PACKETSIZE) == PACKETSIZE);
    if(!ret)
    {
      close(f);
      WriteLogMC("MovieCutterLib", "ReadFirstAndLastCutPacket() E0302.");
      TRACEEXIT();
      return FALSE;
    }
  }

  // Seek to the end of the cut file
  if(LastCutPacket)
  {
    lseek64(f, -PACKETSIZE, SEEK_END);

    //Read the last TS packet
    ret = (read(f, &LastCutPacket[0], PACKETSIZE) == PACKETSIZE);
    close(f);
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

  if (CutPointArray == NULL || OutOffset == NULL) return FALSE;
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

  if (CutPointArray == NULL || OutOffset == NULL) return FALSE;
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
  for (i = 0; i < min(NrBookmarks, NRBOOKMARKS); i++) {
    RECHeaderInfo.Bookmark[i] = Bookmarks[i];
  }
  for (i = NrBookmarks; i < NRBOOKMARKS; i++) {
    RECHeaderInfo.Bookmark[i] = 0;
  }
  RECHeaderInfo.NrBookmarks = min(NrBookmarks, NRBOOKMARKS);

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

bool PatchInfFiles(const char *SourceFileName, const char *CutFileName, const char *AbsDirectory, dword SourcePlayTime, const tTimeStamp *CutStartPoint, const tTimeStamp *BehindCutPoint, char *pCutCaption, char *pSourceCaption)
{
  char                  AbsSourceInfName[FBLIB_DIR_SIZE], AbsCutInfName[FBLIB_DIR_SIZE];
  char                  T1[16], T2[16], T3[16];
  char                 *NewEventText, OldEventText[1025], LogString[1024];
  FILE                 *fSourceInf = NULL, *fCutInf = NULL;
  byte                 *Buffer = NULL;
  TYPE_RecHeader_Info  *RecHeaderInfo = NULL;
  TYPE_Bookmark_Info   *BookmarkInfo = NULL;
  size_t                BufSize, InfSize, BytesRead;
  dword                 Bookmarks[NRBOOKMARKS];
  dword                 NrBookmarks;
  dword                 CutPlayTime;
  tPVRTime              OrigHdrStartTime;
  byte                  OrigHdrStartSec;
  dword                 i;
  bool                  SetCutBookmark;
  bool                  Result = FALSE;

  TRACEENTER();
  #ifdef FULLDEBUG
//    WriteLogMC("MovieCutterLib", "PatchInfFiles()");
  #endif

  //Calculate inf header size
  InfSize = ((GetSystemType()==ST_TMSC) ? sizeof(TYPE_RecHeader_TMSC) : sizeof(TYPE_RecHeader_TMSS));
  BufSize = max(InfSize, 32768);

  //Allocate and clear the buffer
  Buffer = (byte*) TAP_MemAlloc(BufSize);
  if(Buffer) 
    memset(Buffer, 0, BufSize);
  else
  {
    WriteLogMC("MovieCutterLib", "PatchInfFiles() E0901: source inf not patched, cut inf not created.");
    TRACEEXIT();
    return FALSE;
  }

  //Read the source .inf
  TAP_SPrint(AbsSourceInfName, sizeof(AbsSourceInfName), "%s/%s.inf", AbsDirectory, SourceFileName);
  fSourceInf = fopen(AbsSourceInfName, "rb");
  if(!fSourceInf)
  {
    WriteLogMC("MovieCutterLib", "PatchInfFiles() E0902: source inf not patched, cut inf not created.");
    TAP_MemFree(Buffer);
    TRACEEXIT();
    return FALSE;
  }

  BytesRead = fread(Buffer, 1, BufSize, fSourceInf);
  #ifdef FULLDEBUG
    struct stat statbuf;
    fstat(fileno(fSourceInf), &statbuf);
    WriteLogMCf("MovieCutterLib", "PatchInfFiles(): %d / %llu Bytes read.", BytesRead, statbuf.st_size);
  #endif
  fclose(fSourceInf);

  //Decode the source .inf
  RecHeaderInfo = (TYPE_RecHeader_Info*) Buffer;
  switch (GetSystemType())
  {
    case ST_TMSS:  BookmarkInfo  = &(((TYPE_RecHeader_TMSS*)Buffer)->BookmarkInfo); break;
    case ST_TMSC:  BookmarkInfo  = &(((TYPE_RecHeader_TMSC*)Buffer)->BookmarkInfo); break;
    case ST_TMST:  BookmarkInfo  = &(((TYPE_RecHeader_TMST*)Buffer)->BookmarkInfo); break;
    default:
      WriteLogMC("MovieCutterLib", "PatchInfFiles() E0903: source inf not patched, cut inf not created.");
      TAP_MemFree(Buffer);
      TRACEEXIT();
      return FALSE;
  }

  //Captions in den ExtEventText einfügen und Event-Strings von Datenmüll reinigen
  TYPE_RecHeader_TMSC *RecHeader = (TYPE_RecHeader_TMSC*)Buffer;
  dword p = strlen(RecHeader->EventInfo.EventNameDescription);
  if (p < sizeof(RecHeader->EventInfo.EventNameDescription))
    memset(&RecHeader->EventInfo.EventNameDescription[p], 0, sizeof(RecHeader->EventInfo.EventNameDescription) - p);
  p = RecHeader->ExtEventInfo.TextLength;
  if (p < sizeof(RecHeader->ExtEventInfo.Text))
    memset(&RecHeader->ExtEventInfo.Text[p], 0, sizeof(RecHeader->ExtEventInfo.Text) - p);

  // ggf. Itemized Items in ExtEventText entfernen
  memset(OldEventText, 0, sizeof(OldEventText));
  if (pSourceCaption || pCutCaption)
  {
    int j = 0, k = 0; dword p = 0;
    while ((j < 2*RecHeader->ExtEventInfo.NrItemizedPairs) && (p < RecHeader->ExtEventInfo.TextLength))
      if (RecHeader->ExtEventInfo.Text[p++] == '\0')  j++;

    if (j == 2*RecHeader->ExtEventInfo.NrItemizedPairs)
    {
      strncpy(OldEventText, &RecHeader->ExtEventInfo.Text[p], min(RecHeader->ExtEventInfo.TextLength - p, sizeof(OldEventText)-1));

      p = 0;
      for (k = 0; k < j; k++)
      {
        if((byte)(RecHeader->ExtEventInfo.Text[p]) < 0x20)  p++;
        TAP_SPrint(&OldEventText[strlen(OldEventText)], sizeof(OldEventText)-strlen(OldEventText), ((k % 2 == 0) ? ((isUTFToppy() && (byte)OldEventText[0] >= 0x15) ? "\xC2\x8A%s: " : "\x8A%s: ") : "%s"), &RecHeader->ExtEventInfo.Text[p]);
        p += strlen(&RecHeader->ExtEventInfo.Text[p]) + 1;
      }
    }
    else
      strncpy(OldEventText, RecHeader->ExtEventInfo.Text, min(RecHeader->ExtEventInfo.TextLength, (int)sizeof(OldEventText)-1));
  }

  if (pSourceCaption)
  {
    if ((NewEventText = (char*)TAP_MemAlloc(2 * strlen(pSourceCaption) + strlen(OldEventText) + 5)))
    {
      if (isUTFToppy() && (byte)OldEventText[0] >= 0x15)
      {
        StrToUTF8(pSourceCaption, NewEventText, 9);
        if (*OldEventText)
          sprintf(&NewEventText[strlen(NewEventText)], "\xC2\x8A\xC2\x8A%s", &OldEventText[((byte)OldEventText[0] < 0x20) ? 1 : 0]);
      }
      else
        sprintf(NewEventText, "\5%s\x8A\x8A%s", pSourceCaption, &OldEventText[((byte)OldEventText[0] < 0x20) ? 1 : 0]);
      strncpy(RecHeader->ExtEventInfo.Text, NewEventText, sizeof(RecHeader->ExtEventInfo.Text) - 1);
      if (RecHeader->ExtEventInfo.Text[sizeof(RecHeader->ExtEventInfo.Text) - 2] != 0)
        TAP_SPrint(&RecHeader->ExtEventInfo.Text[sizeof(RecHeader->ExtEventInfo.Text) - 4], 4, "...");
      RecHeader->ExtEventInfo.TextLength = strlen(RecHeader->ExtEventInfo.Text);
      RecHeader->ExtEventInfo.NrItemizedPairs = 0;
      TAP_MemFree(NewEventText);
    }
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
    SourcePlayTime = 60 * RecHeaderInfo->DurationMin + RecHeaderInfo->DurationSec;
    SecToTimeString(SourcePlayTime, T1);
    CutPlayTime    = (dword)((CutPlayTime + 500) / 1000);
    SourcePlayTime -= min(CutPlayTime, SourcePlayTime);
    SecToTimeString(SourcePlayTime, T3);
  }
  WriteLogMCf("MovieCutterLib", "Playtimes: Orig = %s, Cut = %s, New = %s", T1, T2, T3);

  //Change the new source play time
  RecHeaderInfo->DurationMin = (word)(SourcePlayTime / 60);
  RecHeaderInfo->DurationSec = SourcePlayTime % 60;

  //Set recording time of the source file
  OrigHdrStartTime = RecHeaderInfo->StartTime;
  OrigHdrStartSec  = RecHeaderInfo->StartTimeSec;
  if (CutStartPoint->BlockNr == 0)
    RecHeaderInfo->StartTime = AddTimeSec(OrigHdrStartTime, OrigHdrStartSec, &RecHeaderInfo->StartTimeSec, BehindCutPoint->Timems / 1000);

  //Save all bookmarks to a temporary array
  memcpy(Bookmarks, BookmarkInfo->Bookmarks, NRBOOKMARKS * sizeof(dword));
  NrBookmarks = min(BookmarkInfo->NrBookmarks, NRBOOKMARKS);
  TAP_SPrint(LogString, sizeof(LogString), "Bookmarks: ");
  for(i = 0; i < NrBookmarks; i++)
  {
    if(Bookmarks[i] == 0) break;
    TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), "%lu ", Bookmarks[i]);
  }
  WriteLogMC("MovieCutterLib", LogString);

  //Clear all source Bookmarks
//  memset(BookmarkInfo, 0, sizeof(TYPE_Bookmark_Info));
  memset(BookmarkInfo->Bookmarks, 0, NRBOOKMARKS * sizeof(dword));
  BookmarkInfo->NrBookmarks = 0;

//  if (BookmarkInfo->Resume >= BehindCutPoint->BlockNr)  // unnötig
//    BookmarkInfo->Resume -= (BehindCutPoint->BlockNr - CutStartPoint->BlockNr);
//  else if (BookmarkInfo->Resume > CutStartPoint->BlockNr)
    BookmarkInfo->Resume = 0;

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
        BookmarkInfo->Bookmarks[BookmarkInfo->NrBookmarks] = Bookmarks[i];
      else
      {
        if (SetCutBookmark)
        {
          BookmarkInfo->Bookmarks[BookmarkInfo->NrBookmarks] = CutStartPoint->BlockNr;
          TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), "*%lu ", CutStartPoint->BlockNr);
          BookmarkInfo->NrBookmarks++;
          SetCutBookmark = FALSE;
        }
        BookmarkInfo->Bookmarks[BookmarkInfo->NrBookmarks] = Bookmarks[i] - (BehindCutPoint->BlockNr - CutStartPoint->BlockNr);
      }
      TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), "%lu ", BookmarkInfo->Bookmarks[BookmarkInfo->NrBookmarks]);
      if (BookmarkInfo->NrBookmarks < 48)
        BookmarkInfo->NrBookmarks++;
    }
//    if(Bookmarks[i+1] == 0) break;
  }
  // Setzt automatisch ein Bookmark an die Schnittstelle
  if (SetCutBookmark)
  {
    BookmarkInfo->Bookmarks[BookmarkInfo->NrBookmarks] = CutStartPoint->BlockNr;
    TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), "*%lu ", CutStartPoint->BlockNr);
    if (BookmarkInfo->NrBookmarks < 48)
      BookmarkInfo->NrBookmarks++;
  }
  WriteLogMC("MovieCutterLib", LogString);

  //Encode and write the modified source inf
//  TAP_SPrint(AbsSourceInfName, sizeof(AbsSourceInfName), "%s/%s.inf", AbsDirectory, SourceFileName);
  fSourceInf = fopen(AbsSourceInfName, "r+b");
  if(fSourceInf)
  {
    fseek(fSourceInf, 0, SEEK_SET);
    Result = (fwrite(Buffer, 1, InfSize, fSourceInf) == InfSize);
//    Result = (fflush(fSourceInf) == 0) && Result;
    Result = (fclose(fSourceInf) == 0) && Result;
//    infData_Delete2(SourceFileName, AbsDirectory, INFFILETAG);
  }
  else
  {
    WriteLogMC("MovieCutterLib", "PatchInfFiles() W0901: source inf not patched.");
    Result = FALSE;
  }

  // --- Patch the cut inf ---
  // ggf. Caption in den ExtEventText einfügen
  if (pSourceCaption || pCutCaption)
  {
    memset(RecHeader->ExtEventInfo.Text, 0, sizeof(RecHeader->ExtEventInfo.Text));
    if (pCutCaption)
    {
      if ((NewEventText = (char*)TAP_MemAlloc(2 * strlen(pCutCaption) + strlen(OldEventText) + 5)))
      {
        if (isUTFToppy() && (byte)OldEventText[0] >= 0x15)
        {
          StrToUTF8(pCutCaption, NewEventText, 9);
          if (*OldEventText)
            sprintf(&NewEventText[strlen(NewEventText)], "\xC2\x8A\xC2\x8A%s", &OldEventText[((byte)OldEventText[0] < 0x20) ? 1 : 0]);
        }
        else
          sprintf(NewEventText, "\5%s\x8A\x8A%s", pCutCaption, &OldEventText[((byte)OldEventText[0] < 0x20) ? 1 : 0]);
        strncpy(RecHeader->ExtEventInfo.Text, NewEventText, sizeof(RecHeader->ExtEventInfo.Text) - 1);
        if (RecHeader->ExtEventInfo.Text[sizeof(RecHeader->ExtEventInfo.Text) - 2] != 0)
          TAP_SPrint(&RecHeader->ExtEventInfo.Text[sizeof(RecHeader->ExtEventInfo.Text) - 4], 4, "...");
        TAP_MemFree(NewEventText);
      }
    }
    else
      strncpy(RecHeader->ExtEventInfo.Text, OldEventText, sizeof(RecHeader->ExtEventInfo.Text));
    RecHeader->ExtEventInfo.TextLength = min(strlen(RecHeader->ExtEventInfo.Text), sizeof(RecHeader->ExtEventInfo.Text));
    RecHeader->ExtEventInfo.NrItemizedPairs = 0;
  }

  //Set the length of the cut file
  RecHeaderInfo->DurationMin = (word)(CutPlayTime / 60);
  RecHeaderInfo->DurationSec = CutPlayTime % 60;

  //Set recording time of the cut file
  RecHeaderInfo->StartTime = AddTimeSec(OrigHdrStartTime, OrigHdrStartSec, &RecHeaderInfo->StartTimeSec, CutStartPoint->Timems / 1000);

  //Clear all source Bookmarks
//  memset(BookmarkInfo, 0, sizeof(TYPE_Bookmark_Info));
  memset(BookmarkInfo->Bookmarks, 0, NRBOOKMARKS * sizeof(dword));
  BookmarkInfo->NrBookmarks = 0;
  BookmarkInfo->Resume = 0;

  //Copy all bookmarks which are >= CutPointA and < CutPointB
  //Move them by CutPointA
  TAP_SPrint(LogString, sizeof(LogString), "Bookmarks->Cut: ");
  for(i = 0; i < NrBookmarks; i++)
  {
    if((Bookmarks[i] >= CutStartPoint->BlockNr + 100) && (Bookmarks[i] + 100 < BehindCutPoint->BlockNr))
    {
      BookmarkInfo->Bookmarks[BookmarkInfo->NrBookmarks] = Bookmarks[i] - CutStartPoint->BlockNr;
      TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), "%lu ", BookmarkInfo->Bookmarks[BookmarkInfo->NrBookmarks]);
      BookmarkInfo->NrBookmarks++;
    }
//    if(Bookmarks[i+1] == 0) break;
  }
  WriteLogMC("MovieCutterLib", LogString);

  //Encode the cut inf and write it to the disk
  TAP_SPrint(AbsCutInfName, sizeof(AbsCutInfName), "%s/%s.inf", AbsDirectory, CutFileName);
  fCutInf = fopen(AbsCutInfName, "wb");
  if(fCutInf)
  {
    Result = (fwrite(Buffer, 1, max(InfSize, BytesRead), fCutInf) == max(InfSize, BytesRead)) && Result;

    // Kopiere den Rest der Source-inf (falls vorhanden) in die neue inf hinein
//    TAP_SPrint(AbsSourceInfName, sizeof(AbsSourceInfName), "%s/%s.inf", AbsDirectory, SourceFileName);
    fSourceInf = fopen(AbsSourceInfName, "rb");
    if(fSourceInf)
    {
      fseek(fSourceInf, BytesRead, SEEK_SET);
      do {
        BytesRead = fread(Buffer, 1, 32768, fSourceInf);
        if (BytesRead > 0)
          Result = (fwrite(Buffer, 1, BytesRead, fCutInf) == BytesRead) && Result;
      } while (BytesRead > 0);
      fclose(fSourceInf);
    }
//    Result = (fflush(fCutInf) == 0) && Result;
    Result = (fclose(fCutInf) == 0) && Result;
  }
  else
  {
    WriteLogMC("MovieCutterLib", "PatchInfFiles() E0905: cut inf not created.");
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
  tnavSD                NavBuffer[2], *CurNavRec = &NavBuffer[0];
  off_t                 PictureHeaderOffset = 0;
  bool                  IFrameCut, IFrameSrc, PFrame;
  dword                 FirstCutTime, LastCutTime, FirstSourceTime, LastSourceTime;
  bool                  ret = FALSE;

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

  // Versuche, nav-Dateien aus Timeshift-Aufnahmen zu unterstützen ***experimentell***
  dword FirstDword = 0;
  fread(&FirstDword, 4, 1, fOldNav);
  if(FirstDword == 0x72767062)  // 'bpvr'
    fseek(fOldNav, 1056, SEEK_SET);
  else
    rewind(fOldNav);

  //Loop through the nav
  IFrameCut = FALSE; IFrameSrc = TRUE; PFrame = TRUE;
  FirstCutTime = 0xFFFFFFFF;
  LastCutTime = 0;
  FirstSourceTime = 0;
  LastSourceTime = 0;

  while (fread(CurNavRec, sizeof(tnavSD) * (isHD ? 2 : 1), 1, fOldNav))
  {
    ret = TRUE;

    //Check if the entry lies between the CutPoints
    PictureHeaderOffset = ((off_t)(CurNavRec->PHOffsetHigh) << 32) | CurNavRec->PHOffset;
    if((PictureHeaderOffset >= CutStartPos) && (PictureHeaderOffset < BehindCutPos))
    {
      if (FirstCutTime == 0xFFFFFFFF)
      {
        if (CutStartPos == 0)
          FirstCutTime = 0;
        else
          FirstCutTime = CurNavRec->Timems;
      }
      LastCutTime = CurNavRec->Timems;

      //Subtract CutStartPos from the cut .nav PH address
      PictureHeaderOffset = PictureHeaderOffset - CutStartPos;
      if (!PFrame && CurNavRec->FrameType <= 2)
        PFrame = TRUE;
      if(!IFrameCut && CurNavRec->FrameType == 1) {
        IFrameCut = TRUE; PFrame = FALSE; }
      if(IFrameCut && (/*isHD ||*/ PFrame || CurNavRec->FrameType <= 2))
      {
        CurNavRec->PHOffsetHigh = PictureHeaderOffset >> 32;
        CurNavRec->PHOffset = PictureHeaderOffset & 0xffffffff;
        CurNavRec->Timems = CurNavRec->Timems - FirstCutTime;
        ret = fwrite(CurNavRec, sizeof(tnavSD) * (isHD ? 2 : 1), 1, fCutNav) && ret;
      }
      IFrameSrc = FALSE;
    }
    else
    {
      if (PictureHeaderOffset >= BehindCutPos)
      {
        if (FirstSourceTime == 0) FirstSourceTime = CurNavRec->Timems;
        LastSourceTime = CurNavRec->Timems;
        if (IgnoreRecordsAfterCut) break;
      }

      if (!PFrame && CurNavRec->FrameType <= 2)
        PFrame = TRUE;
      if(!IFrameSrc && CurNavRec->FrameType == 1) {
        IFrameSrc = TRUE; PFrame = FALSE; }
      if(IFrameSrc && (/*isHD ||*/ PFrame || CurNavRec->FrameType <= 2))
      {
        //if ph offset >= BehindCutPos, subtract (BehindCutPos - CutStartPos)
        if(PictureHeaderOffset >= BehindCutPos)
        {
          PictureHeaderOffset = PictureHeaderOffset - (BehindCutPos - CutStartPos);
          CurNavRec->PHOffsetHigh = PictureHeaderOffset >> 32;
          CurNavRec->PHOffset = PictureHeaderOffset & 0xffffffff;
          CurNavRec->Timems = CurNavRec->Timems - (FirstSourceTime - FirstCutTime);
        }
        ret = fwrite(CurNavRec, sizeof(tnavSD) * (isHD ? 2 : 1), 1, fSourceNav) && ret;
      }
      IFrameCut = FALSE;
    }

    if (IgnoreRecordsAfterCut && (PictureHeaderOffset >= BehindCutPos))
      break;
  }

//  ret = (fflush(fSourceNav) == 0) && ret;
  ret = (fclose(fSourceNav) == 0) && ret;
//  ret = (fflush(fCutNav) == 0) && ret;
  ret = (fclose(fCutNav) == 0) && ret;
  fclose(fOldNav);

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
  return ret;
}


// ----------------------------------------------------------------------------
//                               NAV-Einlesen
// ----------------------------------------------------------------------------
tTimeStamp* NavLoad(const char *RecFileName, const char *AbsDirectory, int *const OutNrTimeStamps, bool isHD)
{
  char                  AbsFileName[FBLIB_DIR_SIZE];
  FILE                 *fNav = NULL;
  tnavSD                NavBuffer[2], *CurNavRec = &NavBuffer[0];
  tTimeStamp           *TimeStampBuffer = NULL;
  tTimeStamp           *TimeStamps = NULL;
  int                   NavRecordsNr, NrTimeStamps = 0;
  dword                 FirstTime, LastTime;
  ulong64               AbsPos;
  dword                 NavSize = 0;

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
  struct stat statbuf;
  if (fstat(fileno(fNav), &statbuf) == 0)
    NavSize = statbuf.st_size;
  NavRecordsNr = (NavSize / (sizeof(tnavSD) * ((isHD) ? 2 : 1))) / 4;  // höchstens jedes 4. Frame ist ein I-Frame (?)

  if (!NavRecordsNr || !((TimeStampBuffer = (tTimeStamp*) malloc(NavRecordsNr * sizeof(tTimeStamp)))))
  {
    fclose(fNav);
    WriteLogMC("MovieCutterLib", "NavLoad() E0b02");
    TRACEEXIT();
    return(NULL);
  }

  // Versuche, nav-Dateien aus Timeshift-Aufnahmen zu unterstützen ***experimentell***
  dword FirstDword = 0;
  fread(&FirstDword, 4, 1, fNav);
  if(FirstDword == 0x72767062)  // 'bpvr'
    fseek(fNav, 1056, SEEK_SET);
  else
    rewind(fNav);

  //Count and save all the _different_ time stamps in the .nav
  LastTime = 0xFFFFFFFF;
  FirstTime = 0xFFFFFFFF;

  while (fread(CurNavRec, sizeof(tnavSD) * (isHD ? 2 : 1), 1, fNav) && (NrTimeStamps < NavRecordsNr))
  {
    if(FirstTime == 0xFFFFFFFF) FirstTime = CurNavRec->Timems;
    if(CurNavRec->FrameType == 1)  // erfasse nur noch I-Frames
    {
      AbsPos = ((ulong64)(CurNavRec->PHOffsetHigh) << 32) | CurNavRec->PHOffset;
/*      if(CurNavRec->Timems == LastTime)
      {
TAP_PrintNet("Achtung! I-Frame an %llu hat denselben Timestamp wie sein Vorgänger!\n", AbsPos);
      } */
      TimeStampBuffer[NrTimeStamps].BlockNr   = CalcBlockSize(AbsPos);
      TimeStampBuffer[NrTimeStamps].Timems    = CurNavRec->Timems;

/*        if (CurNavRec->Timems >= FirstTime)
        // Timems ist größer als FirstTime -> kein Überlauf
        TimeStampBuffer[*NrTimeStamps].Timems = CurNavRec->Timems - FirstTime;
      else if (FirstTime - CurNavRec->Timems <= 3000)
        // Timems ist kaum kleiner als FirstTime -> liegt vermutlich am Anfang der Aufnahme
        TimeStampBuffer[*NrTimeStamps].Timems = 0;
      else
        // Timems ist (deutlich) kleiner als FirstTime -> ein Überlauf liegt vor
        TimeStampBuffer[*NrTimeStamps].Timems = (0xffffffff - FirstTime) + CurNavRec->Timems + 1;
*/
      NrTimeStamps++;
      LastTime = CurNavRec->Timems;
    }
  }

  // Free the nav-Buffer and close the file
  fclose(fNav);

  // Reserve a new buffer of the correct size to hold only the different time stamps
  if (NrTimeStamps > 0)
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
