#define _FILE_OFFSET_BITS  64
#define __USE_LARGEFILE64  1
#ifdef _MSC_VER
  #define __const const
#endif

#include                <stdio.h>
#include                <stdlib.h>
#include                <string.h>
#include                <unistd.h>
#include                <utime.h>
#include                <stdarg.h>
#include                <tap.h>
#include                <libFireBird.h>
#include                "CWTapApiLib.h"
#include                "MovieCutterLib.h"


bool        FileCut(char *SourceFileName, char *CutFileName, char const *Directory, dword StartBlock, dword NrBlocks);
bool        WriteByteToFile(char const *FileName, char const *Directory, off_t BytePosition, char OldValue, char NewValue);
bool        PatchRecFile(char const *SourceFileName, char const *Directory, off_t RequestedCutPosition, byte CutPointArray[], off_t OutPatchedBytes[]);
bool        UnpatchRecFile(char const *SourceFileName, char const *CutFileName, char const *Directory, off_t CutStartPos, off_t BehindCutPos, off_t const PatchedBytes[], dword NrPatchedBytes);
bool        ReadCutPointArea(char const *SourceFileName, char const *Directory, off_t CutPosition, byte CutPointArray[]);
bool        ReadFirstAndLastCutPacket(char const *FileName, char const *Directory, byte FirstCutPacket[], byte LastCutPacket[]);
bool        FindCutPointOffset(const byte CutPacket[], const byte CutPointArray[], long *const Offset);
bool        PatchInfFiles(char const *SourceFileName, char const *CutFileName, char const *Directory, dword SourcePlayTime, tTimeStamp const *CutStartPoint, tTimeStamp const *BehindCutPoint);
bool        PatchNavFiles(char const *SourceFileName, char const *CutFileName, char const *Directory, off_t CutStartPos, off_t BehindCutPos, bool isHD, dword *const OutCutStartTime, dword *const OutBehindCutTime, dword *const OutSourcePlayTime);

static int              PACKETSIZE = 192;
static int              CUTPOINTSEARCHRADIUS = 9024;
static int              CUTPOINTSECTORRADIUS = 2;
static char             LogString[512];


// ----------------------------------------------------------------------------
//                           Hilfsfunktionen
// ----------------------------------------------------------------------------
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

void WriteDebugLog(char *format, ...)
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
}

static inline dword CalcBlockSize(off_t Size)
{
  // Workaround für die Division durch BLOCKSIZE (9024)
  // Primfaktorenzerlegung: 9024 = 2^6 * 3 * 47
  // max. Dateigröße: 256 GB (dürfte reichen...)
  return (dword)(Size >> 6) / 141;
}

void SecToTimeString(dword Time, char *const TimeString)  // needs max. 4 + 1 + 2 + 1 + 2 + 1 = 11 chars
{
  dword                 Hour, Min, Sec;

  TRACEENTER();

  Hour = (int)(Time / 3600);
  Min  = (int)(Time / 60) % 60;
  Sec  = Time % 60;
  if (Hour >= 10000) Hour = 9999;
  TAP_SPrint(TimeString, 11, "%lu:%02lu:%02lu", Hour, Min, Sec);

  TRACEEXIT();
}

void MSecToTimeString(dword Timems, char *const TimeString)  // needs max. 4 + 1 + 2 + 1 + 2 + 1 + 3 + 1 = 15 chars
{
  dword                 Hour, Min, Sec, Millisec;

  TRACEENTER();

  Hour = (int)(Timems / 3600000);
  Min  = (int)(Timems / 60000) % 60;
  Sec  = (int)(Timems / 1000) % 60;
  Millisec = Timems % 1000;
  TAP_SPrint(TimeString, 15, "%lu:%02lu:%02lu.%03lu", Hour, Min, Sec, Millisec);

  TRACEEXIT();
}

void GetNextFreeCutName(char const *SourceFileName, char *CutFileName, char const *Directory, word LeaveNamesOut)
{
  size_t                NameLen;
  int                   i;
  char                  NextFileName[MAX_FILE_NAME_SIZE + 1];

  TRACEENTER();

  NameLen = strlen(SourceFileName) - 4;  // ".rec" entfernen
  strncpy(NextFileName, SourceFileName, NameLen);
  strncpy(CutFileName, SourceFileName, NameLen);

  i = 0;
  do
  {
    i++;
    TAP_SPrint(&NextFileName[NameLen], MAX_FILE_NAME_SIZE+1 - NameLen, " (Cut-%d)%s", i, &SourceFileName[NameLen]);
    TAP_SPrint(&CutFileName[NameLen],  MAX_FILE_NAME_SIZE+1 - NameLen, " (Cut-%d)%s", i + LeaveNamesOut, &SourceFileName[NameLen]);
  } while (HDD_Exist2(NextFileName, Directory) || HDD_Exist2(CutFileName, Directory));

  TRACEEXIT();
}


// ----------------------------------------------------------------------------
//                       MovieCutter-Schnittfunktion
// ----------------------------------------------------------------------------
tResultCode MovieCutter(char *SourceFileName, char *CutFileName, char *Directory, tTimeStamp *CutStartPoint, tTimeStamp *BehindCutPoint, bool KeepCut, bool isHD)
{
  char                  FileName[MAX_FILE_NAME_SIZE + 1];
  byte                 *CutPointArea1=NULL, *CutPointArea2=NULL;
  off_t                 SourceFileSize, CutFileSize;
  dword                 MaxBehindCutBlock;
  off_t                 CutStartPos, BehindCutPos;
  long                  CutStartPosOffset, BehindCutPosOffset;
  dword                 SourcePlayTime = 0;
  bool                  SuppressNavGeneration;
  dword                 RecDate;
  int                   i;
//  char                  TimeStr[16];

  TRACEENTER();
  DetectPacketSize(SourceFileName);
  byte                  FirstCutPacket[PACKETSIZE], LastCutPacket[PACKETSIZE];
  off_t                 PatchedBytes[4 * CUTPOINTSECTORRADIUS];

  SuppressNavGeneration = FALSE;

  // LOG file printing
  WriteLogMC("MovieCutterLib", "----------------------------------------");
  TAP_SPrint(LogString, sizeof(LogString), "Source        = '%s'", SourceFileName);
  WriteLogMC("MovieCutterLib", LogString);
  TAP_SPrint(LogString, sizeof(LogString), "Cut name      = '%s'", CutFileName);
  WriteLogMC("MovieCutterLib", LogString);

  if(!HDD_GetFileSizeAndInode2(SourceFileName, Directory, NULL, &SourceFileSize))
  {
    WriteLogMC("MovieCutterLib", "MovieCutter() E0001: cut file not created.");
    TRACEEXIT();
    return RC_Error;
  }

  TAP_SPrint(LogString, sizeof(LogString), "File size     = %llu Bytes (%lu blocks)", SourceFileSize, CalcBlockSize(SourceFileSize));
  WriteLogMC("MovieCutterLib", LogString);
  TAP_SPrint(LogString, sizeof(LogString), "Dir           = '%s'", Directory);
  WriteLogMC("MovieCutterLib", LogString);
  TAP_SPrint(LogString, sizeof(LogString), "PacketSize    = %d", PACKETSIZE);
  WriteLogMC("MovieCutterLib", LogString);

  MaxBehindCutBlock = CalcBlockSize(SourceFileSize - CUTPOINTSEARCHRADIUS);
  if (BehindCutPoint->BlockNr > MaxBehindCutBlock)
    BehindCutPoint->BlockNr = MaxBehindCutBlock;
  CutStartPos = (off_t)CutStartPoint->BlockNr * BLOCKSIZE;
  BehindCutPos = (off_t)BehindCutPoint->BlockNr * BLOCKSIZE;

  TAP_SPrint(LogString, sizeof(LogString), "KeepCut       = %s", KeepCut ? "yes" : "no");
  WriteLogMC("MovieCutterLib", LogString);
  TAP_SPrint(LogString, sizeof(LogString), "CutStartBlock = %lu,\tBehindCutBlock = %lu", CutStartPoint->BlockNr, BehindCutPoint->BlockNr);
  WriteLogMC("MovieCutterLib", LogString);
  TAP_SPrint(LogString, sizeof(LogString), "CutStartPos   = %llu,\tBehindCutPos = %llu", CutStartPos, BehindCutPos);
  WriteLogMC("MovieCutterLib", LogString);

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
    if (!ReadCutPointArea(SourceFileName, Directory, CutStartPos, CutPointArea1))
    {
      WriteLogMC("MovieCutterLib", "MovieCutter() W0002: nav creation suppressed.");
      SuppressNavGeneration = TRUE;
      TAP_MemFree(CutPointArea1); CutPointArea1 = NULL;
    }
  }
  if(CutPointArea2)
  {
    if(!ReadCutPointArea(SourceFileName, Directory, BehindCutPos, CutPointArea2))
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
    PatchRecFile(SourceFileName, Directory, CutStartPos, CutPointArea1, PatchedBytes);
  if(CutPointArea2)
    PatchRecFile(SourceFileName, Directory, BehindCutPos, CutPointArea2, &PatchedBytes[2 * CUTPOINTSECTORRADIUS]);

  // DO THE CUTTING
  if(!FileCut(SourceFileName, CutFileName, Directory, CutStartPoint->BlockNr, BehindCutPoint->BlockNr - CutStartPoint->BlockNr))
  {
    WriteLogMC("MovieCutterLib", "MovieCutter() E0002: Firmware cutting routine failed.");
    TAP_MemFree(CutPointArea1);
    TAP_MemFree(CutPointArea2);
    TRACEEXIT();
    return RC_Error;
  }
  if(!HDD_Exist2(CutFileName, Directory))
  {
    WriteLogMC("MovieCutterLib", "MovieCutter() E0003: Cut file not created.");
    TAP_MemFree(CutPointArea1);
    TAP_MemFree(CutPointArea2);
    TRACEEXIT();
    return RC_Error;
  }

  // Detect the size of the cut file
  if(!HDD_GetFileSizeAndInode2(CutFileName, Directory, NULL, &CutFileSize))
  {
    WriteLogMC("MovieCutterLib", "MovieCutter() W0004: error detecting size of cut file.");
    SuppressNavGeneration = TRUE;
  }
  TAP_SPrint(LogString, sizeof(LogString), "Cut file size: %llu Bytes (=%lu blocks)", CutFileSize, CalcBlockSize(CutFileSize));
  WriteLogMC("MovieCutterLib", LogString);

  // Read the beginning and the ending from the cut file
  if(!ReadFirstAndLastCutPacket(CutFileName, Directory, FirstCutPacket, LastCutPacket))
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
    TAP_SPrint(LogString, sizeof(LogString), "Cut start offset: %ld Bytes (=%ld packets and %ld Bytes), Cut end offset: %ld Bytes (=%ld packets and %ld Bytes)", CutStartPosOffset, CutStartPosOffset/PACKETSIZE, labs(CutStartPosOffset%PACKETSIZE), BehindCutPosOffset, BehindCutPosOffset/PACKETSIZE, labs(BehindCutPosOffset%PACKETSIZE));
    WriteLogMC("MovieCutterLib", LogString);

    TAP_SPrint(LogString, sizeof(LogString), "Real cut positions:  Cut Start = %llu, Behind Cut: %llu", CutStartPos, BehindCutPos);
    WriteLogMC("MovieCutterLib", LogString);

#ifdef FULLDEBUG
    off_t GuessedCutStartPos = 0, GuessedBehindCutPos = 0;
    off_t ReqCutStartPos  = (off_t)CutStartPoint->BlockNr * BLOCKSIZE;
    off_t ReqBehindCutPos = (off_t)BehindCutPoint->BlockNr * BLOCKSIZE;

    for (i = 0; i < CUTPOINTSECTORRADIUS; i++)
    {
      GuessedCutStartPos = ((ReqCutStartPos >> 12) << 12) - (i * 4096);
//      if (GuessedCutStartPos % 192 == 0) break;
      if (CutPointArea1[(int)(GuessedCutStartPos - ReqCutStartPos) + CUTPOINTSEARCHRADIUS + 4] == 'G') break;
      GuessedCutStartPos = ((ReqCutStartPos >> 12) << 12) + ((i+1) * 4096);
      if (CutPointArea1[(int)(GuessedCutStartPos - ReqCutStartPos) + CUTPOINTSEARCHRADIUS + 4] == 'G')
        break;
      else
        GuessedCutStartPos = 0;
    }
    for (i = 0; i < CUTPOINTSECTORRADIUS; i++)
    {
      GuessedBehindCutPos = ((ReqBehindCutPos >> 12) << 12) - (i * 4096);
      if (CutPointArea2[(int)(GuessedBehindCutPos - ReqBehindCutPos) + CUTPOINTSEARCHRADIUS + 4] == 'G') break;
      GuessedBehindCutPos = ((ReqBehindCutPos >> 12) << 12) + ((i+1) * 4096);
      if (CutPointArea2[(int)(GuessedBehindCutPos - ReqBehindCutPos) + CUTPOINTSEARCHRADIUS + 4] == 'G')
        break;
      else
        GuessedBehindCutPos = 0;
    }

    if ((CutStartPos == GuessedCutStartPos) && (BehindCutPos == GuessedBehindCutPos))
      WriteLogMC("MovieCutterLib", "--> Real cutting points guessed correctly!");
    else
    {
      TAP_SPrint(LogString, sizeof(LogString), "!! -- Real cutting points NOT correctly guessed: GuessedStart = %llu, GuessedBehind = %llu", GuessedCutStartPos, GuessedBehindCutPos);
      WriteLogMC("MovieCutterLib", LogString);
    }
#endif
  }
  TAP_MemFree(CutPointArea1); CutPointArea1 = NULL;
  TAP_MemFree(CutPointArea2); CutPointArea2 = NULL;

  // Copy the real cutting positions into the cut point parameters to be returned
  if (!SuppressNavGeneration)
  {
    CutStartPoint->BlockNr = CalcBlockSize(CutStartPos + BLOCKSIZE-1);  // erster vollständiger Block des CutFile
    BehindCutPoint->BlockNr = CutStartPoint->BlockNr + CalcBlockSize(BehindCutPos - CutStartPos + BLOCKSIZE/2);
  }

  // Unpatch the rec-File
  if (!SuppressNavGeneration)
    UnpatchRecFile(SourceFileName, CutFileName, Directory, CutStartPos, BehindCutPos, PatchedBytes, 4 * CUTPOINTSECTORRADIUS);

  // Rename old nav file to bak
  char BakFileName[MAX_FILE_NAME_SIZE + 1];
  TAP_SPrint(FileName, sizeof(FileName), "%s.nav", SourceFileName);
  TAP_SPrint(BakFileName, sizeof(BakFileName), "%s.nav.bak", SourceFileName);
  if (HDD_Exist2(FileName, Directory))
  {
    if (HDD_Exist2(BakFileName, Directory))
      HDD_Delete2(BakFileName, Directory, FALSE);
    HDD_Rename2(FileName, BakFileName, Directory, FALSE);
  }

  // Patch the nav files (and get the TimeStamps for the actual cutting positions)
  if(!SuppressNavGeneration)
  {
    if(PatchNavFiles(SourceFileName, CutFileName, Directory, CutStartPos, BehindCutPos, isHD, &(CutStartPoint->Timems), &(BehindCutPoint->Timems), &SourcePlayTime))
      HDD_Delete2(BakFileName, Directory, FALSE);
    else
    {
      WriteLogMC("MovieCutterLib", "MovieCutter() W0009: nav creation failed.");
      SuppressNavGeneration = TRUE;
      TAP_SPrint(FileName, sizeof(FileName), "%s.nav", SourceFileName);
      HDD_Delete2(FileName, Directory, FALSE);
      TAP_SPrint(FileName, sizeof(FileName), "%s.nav", CutFileName);
      HDD_Delete2(FileName, Directory, FALSE);
    }
  }

  // Copy the inf file and patch both play lengths
  if (!PatchInfFiles(SourceFileName, CutFileName, Directory, SourcePlayTime, CutStartPoint, BehindCutPoint))
    WriteLogMC("MovieCutterLib", "MovieCutter() W0010: inf creation failed.");

  // Fix the date info of all involved files
  if (GetRecDateFromInf(SourceFileName, Directory, &RecDate)) {
    //Source
    HDD_SetFileDateTime(SourceFileName, Directory, RecDate);
    TAP_SPrint(FileName, sizeof(FileName), "%s.inf", SourceFileName);
    HDD_SetFileDateTime(FileName, Directory, RecDate);
    TAP_SPrint(FileName, sizeof(FileName), "%s.nav", SourceFileName);
    HDD_SetFileDateTime(FileName, Directory, RecDate);
  }
  if (GetRecDateFromInf(CutFileName, Directory, &RecDate)) {
    //Cut
    HDD_SetFileDateTime(CutFileName, Directory, RecDate);
    TAP_SPrint(FileName, sizeof(FileName), "%s.inf", CutFileName);
    HDD_SetFileDateTime(FileName, Directory, RecDate);
    TAP_SPrint(FileName, sizeof(FileName), "%s.nav", CutFileName);
    HDD_SetFileDateTime(FileName, Directory, RecDate);
  }
//  if(!KeepSource) HDD_Delete2(SourceFileName, Directory, TRUE);
  if(!KeepCut)
    HDD_Delete2(CutFileName, Directory, TRUE);

  WriteLogMC("MovieCutterLib", "MovieCutter() finished.");

  TRACEEXIT();
  return ((SuppressNavGeneration) ? RC_Warning : RC_Ok);
}

// ----------------------------------------------------------------------------
//              Firmware-Funktion zum Durchführen des Schnitts
// ----------------------------------------------------------------------------
bool FileCut(char *SourceFileName, char *CutFileName, char const *Directory, dword StartBlock, dword NrBlocks)
{
  char                  AbsDirectory[FBLIB_DIR_SIZE];
  tDirEntry             FolderStruct, *pFolderStruct;
  TYPE_PlayInfo         PlayInfo;
  dword                 ret = -1;
  dword                 x;

  TRACEENTER();
  #ifdef FULLDEBUG
    WriteLogMC("MovieCutterLib", "FileCut()");
  #endif

  //If a playback is running, stop it
  TAP_Hdd_GetPlayInfo(&PlayInfo);
  if(PlayInfo.playMode == PLAYMODE_Playing)
  {
    Appl_StopPlaying();
    Appl_WaitEvt(0xE507, &x, 1, 0xFFFFFFFF, 300);
  }
  HDD_Delete2(CutFileName, Directory, FALSE);

  //Flush the caches *experimental*
  sync();
  TAP_Sleep(1);

  HDD_TAP_PushDir();

  //Initialize the directory structure
  memset(&FolderStruct, 0, sizeof(tDirEntry));
  FolderStruct.Magic = 0xbacaed31;

  //Save the current directory resources and change into our directory (current directory of the TAP)
  TAP_SPrint(AbsDirectory, sizeof(AbsDirectory), "%s%s", &TAPFSROOT[1], Directory);  //do not include the leading slash
  ApplHdd_SaveWorkFolder();
  if (ApplHdd_SelectFolder(&FolderStruct, AbsDirectory) == 0)
  {
    if (DevHdd_DeviceOpen(&pFolderStruct, &FolderStruct) == 0)
    {
      ApplHdd_SetWorkFolder(&FolderStruct);

      //Do the cutting
      ret = ApplHdd_FileCutPaste(SourceFileName, StartBlock, NrBlocks, CutFileName);

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
  system("/mnt/hd/ProgramFiles/busybox hdparm -f /dev/sda");
  system("/mnt/hd/ProgramFiles/busybox hdparm -f /dev/sdb");
  system("/mnt/hd/ProgramFiles/busybox hdparm -f /dev/sdc");  */

  if(ret != 0)
  {
    WriteLogMC("MovieCutterLib", "FileCut() E0501. Cut file not created.");
    TRACEEXIT();
    return FALSE;
  }

  TRACEEXIT();
  return TRUE;
}


// ----------------------------------------------------------------------------
//                         Dateisystem-Funktionen
// ----------------------------------------------------------------------------
long64 HDD_GetFileSize(char const *FileName)
{
  TYPE_File            *f = NULL;
  long64                FileSize;

  TRACEENTER();

  f = TAP_Hdd_Fopen(FileName);
  if(!f)
  {
    WriteLogMC("MovieCutterLib", "HDD_GetFileSize(): E0d01");
    TRACEEXIT();
    return -1;
  }
  FileSize = f->size;
  TAP_Hdd_Fclose(f);

  TRACEEXIT();
  return FileSize;
}

bool HDD_SetFileDateTime(char const *FileName, char const *Directory, dword NewDateTime)
{
  char                  AbsFileName[FBLIB_DIR_SIZE];
  tstat64               statbuf;
  int                   status;
  struct utimbuf        utimebuf;

  TRACEENTER();

  TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s%s/%s", TAPFSROOT, Directory, FileName);
  if((status = lstat64(AbsFileName, &statbuf)))
  {
    TAP_SPrint(LogString, sizeof(LogString), "HDD_SetFileDateTime(%s) E0a01.", AbsFileName);
    WriteLogMC("MovieCutterLib", LogString);
    TRACEEXIT();
    return FALSE;
  }

  if(NewDateTime > 0xd0790000)
  {
    utimebuf.actime = statbuf.st_atime;
    utimebuf.modtime = TF2UnixTime(NewDateTime);
    utime(AbsFileName, &utimebuf);

    TRACEEXIT();
    return TRUE;
  }

  TAP_SPrint(LogString, sizeof(LogString), "HDD_SetFileDateTime(%s) E0a02.", AbsFileName);
  WriteLogMC("MovieCutterLib", LogString);

  TRACEEXIT();
  return FALSE;
}


// ----------------------------------------------------------------------------
//                         Analyse von REC-Files
// ----------------------------------------------------------------------------
int DetectPacketSize(char const *SourceFileName)
{
  TRACEENTER();

  if (strncmp(&SourceFileName[strlen(SourceFileName) - 4], ".mpg", 4) == 0)
  {
    PACKETSIZE = 188;
    CUTPOINTSEARCHRADIUS = 99264;
    CUTPOINTSECTORRADIUS = CUTPOINTSEARCHRADIUS/4096;  // 24
  }
  else
  {
    PACKETSIZE = 192;
    CUTPOINTSEARCHRADIUS = 9024;
    CUTPOINTSECTORRADIUS = CUTPOINTSEARCHRADIUS/4096;  // 2
  }

  TRACEEXIT();
  return PACKETSIZE;
}

bool isNavAvailable(char const *SourceFileName, char *Directory)
{
  char                  NavFileName[MAX_FILE_NAME_SIZE + 1];
  off_t                 NavFileSize;
  bool                  ret;

  TRACEENTER();
  ret = FALSE;

  TAP_SPrint(NavFileName, sizeof(NavFileName), "%s.nav", SourceFileName);
  if (HDD_Exist2(NavFileName, Directory))
  {
    if (HDD_GetFileSizeAndInode2(NavFileName, Directory, NULL, &NavFileSize))
      if (NavFileSize != 0)
        ret = TRUE;
  }

  TRACEEXIT();
  return ret;
}

// TODO: Auslesen der Stream-Information aus dem InfCache im RAM
bool isCrypted(char const *SourceFileName, char const *Directory)
{
  FILE                 *f = NULL;
  char                  AbsInfName[FBLIB_DIR_SIZE];
  byte                  CryptFlag = 2;
  bool                  ret;

  TRACEENTER();

  TAP_SPrint(AbsInfName, sizeof(AbsInfName), "%s%s/%s.inf", TAPFSROOT, Directory, SourceFileName);
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
bool isHDVideo(char const *SourceFileName, char const *Directory, bool *const isHD)
{
  FILE                 *f = NULL;
  char                  AbsInfName[FBLIB_DIR_SIZE];
  byte                  StreamType = STREAM_UNKNOWN;

  TRACEENTER();

  TAP_SPrint(AbsInfName, sizeof(AbsInfName), "%s%s/%s.inf", TAPFSROOT, Directory, SourceFileName);
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
bool WriteByteToFile(char const *FileName, char const *Directory, off_t BytePosition, char OldValue, char NewValue)
{
  char                  AbsFileName[FBLIB_DIR_SIZE];
  FILE                 *f = NULL;
  char                  ret;

  TRACEENTER();
  #ifdef FULLDEBUG
    TAP_SPrint(LogString, sizeof(LogString), "WriteByteToFile(file=%s, position=%llu, old=%c, new=%c.", FileName, BytePosition, OldValue, NewValue);
    WriteLogMC("MovieCutterLib", LogString);
  #endif

  // Open the file for write access
  TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s%s/%s", TAPFSROOT, Directory, FileName);
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
    TAP_SPrint(LogString, sizeof(LogString), "PatchRecFile(): Replaced Sync-Byte in file %s at position %llu.", FileName, BytePosition);
  else
    TAP_SPrint(LogString, sizeof(LogString), "UnpatchRecFile(): Restored Sync-Byte in file %s at position %llu.", FileName, BytePosition);
  WriteLogMC("MovieCutterLib", LogString);

  TRACEEXIT();
  return TRUE;
}

// Patches the rec-File to prevent the firmware from cutting in the middle of a packet
bool PatchRecFile(char const *SourceFileName, char const *Directory, off_t RequestedCutPosition, byte CutPointArray[], off_t OutPatchedBytes[])
{
  off_t                 pos;
  int                   ArrayPos;
  int                   i, j;
  bool                  isPacketStart;
  bool                  ret = TRUE;

  TRACEENTER();
//  #ifdef FULLDEBUG
    WriteLogMC("MovieCutterLib", "PatchRecFile()");
//  #endif

  byte *const           MidArray = &CutPointArray[CUTPOINTSEARCHRADIUS];

  // For each of the 4 (Austr: 48) possible cut positions
  for (i = -(CUTPOINTSECTORRADIUS-1); i <= CUTPOINTSECTORRADIUS; i++)
  {
    OutPatchedBytes[i + (CUTPOINTSECTORRADIUS-1)] = 0;
    pos = ((RequestedCutPosition >> 12) << 12) + (i * 4096);
    ArrayPos = (int)(pos-RequestedCutPosition);

    // Check, if the current position is a sync-byte
    if ((MidArray[ArrayPos+4] == 'G'))
    {
      // Check, if the current position is a packet start (192 bytes per packet)
      isPacketStart = TRUE;
      for (j = 0; j < 10; j++)
      {
        if (ArrayPos+4 + (j * PACKETSIZE) >= CUTPOINTSEARCHRADIUS)
          break;
        if (MidArray[ArrayPos+4 + (j * PACKETSIZE)] != 'G')
        {
          isPacketStart = FALSE;
          break;
        }
      }

      // If there IS a sync-Byte, but NOT a packet start, then patch this byte
      if (!isPacketStart)
      {
        if (WriteByteToFile(SourceFileName, Directory, pos+4, 'G', 'F'))
        {
          OutPatchedBytes[i + (CUTPOINTSECTORRADIUS-1)] = pos+4;
          MidArray[ArrayPos+4] = 'F';       // ACHTUNG! Nötig, damit die neue Schätzung der CutPosition korrekt funktioniert.
        }                                   // Könnte aber ein Problem geben bei der (alten) CutPoint-Identifikation (denn hier wird der noch ungepatchte Wert aus dem Cache gelesen)!
        else
          ret = FALSE;
      }
    }
  }

  TRACEEXIT();
  return ret;
}

// Restores the patched Sync-Bytes in the rec-File
bool UnpatchRecFile(char const *SourceFileName, char const *CutFileName, char const *Directory, off_t CutStartPos, off_t BehindCutPos, off_t const PatchedBytes[], dword NrPatchedBytes)
{
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
    if (PatchedBytes[i] > 0)
    {
      if (PatchedBytes[i] < CutStartPos)
        ret = ret + !WriteByteToFile(SourceFileName, Directory, PatchedBytes[i], 'F', 'G');
      else if (PatchedBytes[i] < BehindCutPos)
        ret = ret + !WriteByteToFile(CutFileName, Directory, PatchedBytes[i] - CutStartPos, 'F', 'G');
      else
        ret = ret + !WriteByteToFile(SourceFileName, Directory, PatchedBytes[i] - (BehindCutPos - CutStartPos), 'F', 'G');
    }
  }

  TRACEEXIT();
  return (ret == 0);
}


// ----------------------------------------------------------------------------
//               Ermittlung der tatsächlichen Schnittposition
// ----------------------------------------------------------------------------
bool ReadCutPointArea(char const *SourceFileName, char const *Directory, off_t CutPosition, byte CutPointArray[])
{
  char                  AbsFileName[FBLIB_DIR_SIZE];
  FILE                 *f = NULL;
  size_t                ret;

  TRACEENTER();
  #ifdef FULLDEBUG
    WriteLogMC("MovieCutterLib", "ReadCutPointArea()");
  #endif

  // Open the rec for read access
  TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s%s/%s", TAPFSROOT, Directory, SourceFileName);
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

bool ReadFirstAndLastCutPacket(char const *FileName, char const *Directory, byte FirstCutPacket[], byte LastCutPacket[])
{
  char                  AbsFileName[FBLIB_DIR_SIZE];
  FILE                 *f = NULL;
  size_t                ret;

  TRACEENTER();
  #ifdef FULLDEBUG
    WriteLogMC("MovieCutterLib", "ReadFirstAndLastCutPacket()");
  #endif

  // Open the rec for read access
  TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s%s/%s", TAPFSROOT, Directory, FileName);
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
bool FindCutPointOffset(const byte CutPacket[], const byte CutPointArray[], long *const Offset)
{
  const byte           *MidArray;
  byte                  FirstByte;
  ptrdiff_t             i;    // negative array indices might be critical on 64-bit systems! (http://www.devx.com/tips/Tip/41349)

  TRACEENTER();

  FirstByte = CutPacket[0];
  MidArray = &CutPointArray[CUTPOINTSEARCHRADIUS];
  *Offset = CUTPOINTSEARCHRADIUS + 1;

  for (i = -CUTPOINTSEARCHRADIUS; i <= (CUTPOINTSEARCHRADIUS - PACKETSIZE); i++)
  {
    if (MidArray[i] == FirstByte)
    {
      if (memcmp(&MidArray[i], CutPacket, PACKETSIZE) == 0)
      {
        if (labs(*Offset) < CUTPOINTSEARCHRADIUS)
        {
          WriteLogMC("MovieCutterLib", "FindCutPointOffset() W0401: cut packet found more than once.");
//          if (i > labs(*Offset)) break;
          *Offset = CUTPOINTSEARCHRADIUS + 1;
          break;
        }
        *Offset = i;
      }
    }
  }
  if (labs(*Offset) > CUTPOINTSEARCHRADIUS)
  {
    *Offset = 0;
    TRACEEXIT();
    return FALSE;
  }

  TRACEEXIT();
  return TRUE;
}


// ----------------------------------------------------------------------------
//                              INF-Funktionen
// ----------------------------------------------------------------------------
bool GetRecDateFromInf(char const *FileName, char const *Directory, dword *const DateTime)
{
  FILE                 *f = NULL;
  char                  AbsInfName[FBLIB_DIR_SIZE];

  TRACEENTER();

  TAP_SPrint(AbsInfName, sizeof(AbsInfName), "%s%s/%s.inf", TAPFSROOT, Directory, FileName);
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

bool SaveBookmarksToInf(char const *SourceFileName, char const *Directory, const dword Bookmarks[], int NrBookmarks)
{
  char                  AbsInfName[FBLIB_DIR_SIZE];
  tRECHeaderInfo        RECHeaderInfo;
  byte                 *Buffer = NULL;
  FILE                 *fInf = NULL;
  dword                 BytesRead;
  int                   i;

  TRACEENTER();

#ifdef FULLDEBUG
  TAP_PrintNet("SaveBookmarksToInf()\n");
  for (i = 0; i < NrBookmarks; i++) {
    TAP_PrintNet("%lu\n", Bookmarks[i]);
  }
#endif

  //Allocate and clear the buffer
  Buffer = (byte*) TAP_MemAlloc(INFSIZE);
  if(Buffer) 
    memset(Buffer, 0, INFSIZE);
  else
  {
    WriteLogMC("MovieCutterLib", "SaveBookmarksToInf() E0f01: failed to allocate the memory!");
    TRACEEXIT();
    return FALSE;
  }

  //Read inf
  TAP_SPrint(AbsInfName, sizeof(AbsInfName), "%s%s/%s.inf", TAPFSROOT, Directory, SourceFileName);
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
    fwrite(Buffer, 1, min(BytesRead, INFSIZE), fInf);
  }
  else
    WriteLogMC("MovieCutterLib", "SaveBookmarksToInf() E0f04: failed to encode the new inf header!");

  fclose(fInf);
  TAP_MemFree(Buffer);

  TRACEEXIT();
  return TRUE;
}

bool PatchInfFiles(char const *SourceFileName, char const *CutFileName, char const *Directory, dword SourcePlayTime, tTimeStamp const *CutStartPoint, tTimeStamp const *BehindCutPoint)
{
  char                  AbsSourceInfName[FBLIB_DIR_SIZE], AbsCutInfName[FBLIB_DIR_SIZE];
  char                  T1[12], T2[12], T3[12];
  FILE                 *fSourceInf = NULL, *fCutInf = NULL;
  byte                 *Buffer = NULL;
  tRECHeaderInfo        RECHeaderInfo;
  dword                 BytesRead;
  dword                 Bookmarks[177];
  dword                 CutPlayTime;
  dword                 OrigHeaderStartTime;
  word                  i;
  bool                  SetCutBookmark;
  bool                  Result = TRUE;

  TRACEENTER();
  #ifdef FULLDEBUG
    WriteLogMC("MovieCutterLib", "PatchInfFiles()");
  #endif

  //Allocate and clear the buffer
  Buffer = (byte*) TAP_MemAlloc(INFSIZE);
  if(Buffer) 
    memset(Buffer, 0, INFSIZE);
  else
  {
    WriteLogMC("MovieCutterLib", "PatchInfFiles() E0901: source inf not patched, cut inf not created.");
    TRACEEXIT();
    return FALSE;
  }

  //Read the source .inf
//  TAP_SPrint(SourceInfName, sizeof(SourceInfName), "%s.inf", SourceFileName);
//  tf = TAP_Hdd_Fopen(SourceInfName);
  TAP_SPrint(AbsSourceInfName, sizeof(AbsSourceInfName), "%s%s/%s.inf", TAPFSROOT, Directory, SourceFileName);
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
  fseek(fSourceInf, 0, SEEK_END);
  dword fs = ftell(fSourceInf);
  rewind(fSourceInf);
  BytesRead = fread(Buffer, 1, INFSIZE, fSourceInf);
  fclose(fSourceInf);
TAP_SPrint(LogString, sizeof(LogString), "PatchInfFiles(): %lu / %lu Bytes read.", BytesRead, fs);
WriteLogMC("MovieCutterLib", LogString);

  //Decode the source .inf
  if(!HDD_DecodeRECHeader(Buffer, &RECHeaderInfo, ST_UNKNOWN))
  {
    WriteLogMC("MovieCutterLib", "PatchInfFiles() E0903: source inf not patched, cut inf not created.");
    TAP_MemFree(Buffer);
    TRACEEXIT();
    return FALSE;
  }
WriteLogMC("MovieCutterLib", "Header erfolgreich decoded!");

  //Calculate the new play times
  if (SourcePlayTime)
    SourcePlayTime = (dword)((SourcePlayTime + 500) / 1000);
  else
    SourcePlayTime = 60 * RECHeaderInfo.HeaderDuration + RECHeaderInfo.HeaderDurationSec;
  CutPlayTime = (dword)(((BehindCutPoint->Timems - CutStartPoint->Timems) + 500) / 1000);
  SecToTimeString(SourcePlayTime, T1);
  SecToTimeString(CutPlayTime, T2);

  SourcePlayTime -= min(CutPlayTime, SourcePlayTime);
  SecToTimeString(SourcePlayTime, T3);
  TAP_SPrint(LogString, sizeof(LogString), "Playtimes: Orig = %s, Cut = %s, New = %s", T1, T2, T3);
  WriteLogMC("MovieCutterLib", LogString);

  //Change the new source play time
  RECHeaderInfo.HeaderDuration = (word)(SourcePlayTime / 60);
  RECHeaderInfo.HeaderDurationSec = SourcePlayTime % 60;

  //Set recording time of the source file
  OrigHeaderStartTime = RECHeaderInfo.HeaderStartTime;
  if (CutStartPoint->BlockNr == 0)
    RECHeaderInfo.HeaderStartTime = AddTime(OrigHeaderStartTime, BehindCutPoint->Timems / 60000);

  //Save all bookmarks to a temporary array
  memcpy(Bookmarks, RECHeaderInfo.Bookmark, 177 * sizeof(dword));
  TAP_SPrint(LogString, sizeof(LogString), "Bookmarks: ");
  for(i = 0; i < 177; i++)
  {
    if(Bookmarks[i] == 0) break;
    TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), "%lu ", Bookmarks[i]);
  }
  WriteLogMC("MovieCutterLib", LogString);

  //Clear all source Bookmarks
  memset(RECHeaderInfo.Bookmark, 0, 177 * sizeof(dword));
  RECHeaderInfo.NrBookmarks = 0;
  RECHeaderInfo.Resume = 0;

  //Copy all bookmarks which are < CutPointA or >= CutPointB
  //Move the second group by (CutPointB - CutPointA)
  SetCutBookmark = TRUE;
  if ((CutStartPoint->BlockNr <= 1) || (CutStartPoint->Timems+3000 >= SourcePlayTime*1000) /*(BehindCutPoint->Timems+3000 >= (SourcePlayTime+CutPlayTime)*1000)*/)
    SetCutBookmark = FALSE;

  TAP_SPrint(LogString, sizeof(LogString), "Bookmarks->Source: ");
  for(i = 0; i < 177; i++)
  {
    if((Bookmarks[i] >= 100 && Bookmarks[i] + 100 < CutStartPoint->BlockNr) || (Bookmarks[i] >= BehindCutPoint->BlockNr + 100))
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
      RECHeaderInfo.NrBookmarks++;
    }
    if(Bookmarks[i+1] == 0) break;
  }
  // Setzt automatisch ein Bookmark an die Schnittstelle
  if (SetCutBookmark)
  {
    RECHeaderInfo.Bookmark[RECHeaderInfo.NrBookmarks] = CutStartPoint->BlockNr;
    TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), "*%lu ", CutStartPoint->BlockNr);
    RECHeaderInfo.NrBookmarks++;
  }
  WriteLogMC("MovieCutterLib", LogString);

  //Encode and write the modified source inf
  if(HDD_EncodeRECHeader(Buffer, &RECHeaderInfo, ST_UNKNOWN))
  {
//    TAP_SPrint(AbsSourceInfName, sizeof(AbsSourceInfName), "%s%s/%s.inf", TAPFSROOT, Directory, SourceFileName);
    fSourceInf = fopen(AbsSourceInfName, "r+b");
    if(fSourceInf != 0)
    {
      fseek(fSourceInf, 0, SEEK_SET);
      fwrite(Buffer, 1, min(BytesRead, INFSIZE), fSourceInf);
      fclose(fSourceInf);
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
  memset(RECHeaderInfo.Bookmark, 0, 177 * sizeof(dword));
  RECHeaderInfo.NrBookmarks = 0;

  //Copy all bookmarks which are >= CutPointA and < CutPointB
  //Move them by CutPointA
  TAP_SPrint(LogString, sizeof(LogString), "Bookmarks->Cut: ");
  for(i = 0; i < 177; i++)
  {
    if((Bookmarks[i] >= CutStartPoint->BlockNr + 100) && (Bookmarks[i] + 100 < BehindCutPoint->BlockNr))
    {
      RECHeaderInfo.Bookmark[RECHeaderInfo.NrBookmarks] = Bookmarks[i] - CutStartPoint->BlockNr;
      TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), "%lu ", RECHeaderInfo.Bookmark[RECHeaderInfo.NrBookmarks]);
      RECHeaderInfo.NrBookmarks++;
    }
    if(Bookmarks[i+1] == 0) break;
  }
  WriteLogMC("MovieCutterLib", LogString);

  //Encode the cut inf and write it to the disk
  if(HDD_EncodeRECHeader(Buffer, &RECHeaderInfo, ST_UNKNOWN))
  {
    TAP_SPrint(AbsCutInfName, sizeof(AbsCutInfName), "%s%s/%s.inf", TAPFSROOT, Directory, CutFileName);
    fCutInf = fopen(AbsCutInfName, "wb");
    if(fCutInf != 0)
    {
      fseek(fCutInf, 0, SEEK_SET);
      fwrite(Buffer, 1, min(BytesRead, INFSIZE), fCutInf);

      // Kopiere den Rest der Source-inf (falls vorhanden) in die neue inf hinein
//      TAP_SPrint(AbsSourceInfName, sizeof(AbsSourceInfName), "%s%s/%s.inf", TAPFSROOT, Directory, SourceFileName);
      fSourceInf = fopen(AbsSourceInfName, "rb");
      if(fSourceInf)
      {
        fseek(fSourceInf, min(BytesRead, INFSIZE), SEEK_SET);
        do {
          BytesRead = fread(Buffer, 1, INFSIZE, fSourceInf);
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
bool PatchNavFiles(char const *SourceFileName, char const *CutFileName, char const *Directory, off_t CutStartPos, off_t BehindCutPos, bool isHD, dword *const OutCutStartTime, dword *const OutBehindCutTime, dword *const OutSourcePlayTime)
{
  char                  AbsFileName[FBLIB_DIR_SIZE];
  FILE                 *fOldNav = NULL, *fSourceNav = NULL, *fCutNav = NULL;
  tnavSD               *navOld = NULL, *navSource = NULL, *navCut=NULL;
  off_t                 PictureHeaderOffset;
  size_t                navsRead, navRecsSource, navRecsCut, i;
  bool                  IFrameCut, IFrameSource;
  dword                 FirstCutTime, LastCutTime, FirstSourceTime, LastSourceTime;

  TRACEENTER();
  #ifdef FULLDEBUG
    WriteLogMC("MovieCutterLib", "PatchNavFiles()");
  #endif

  //Open the original nav
  TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s%s/%s.nav.bak", TAPFSROOT, Directory, SourceFileName);
  fOldNav = fopen(AbsFileName, "rb");
  if(!fOldNav)
  {
    WriteLogMC("MovieCutterLib", "PatchNavFiles() E0701.");
    TRACEEXIT();
    return FALSE;
  }

  //Create and open the new source nav
  TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s%s/%s.nav", TAPFSROOT, Directory, SourceFileName);
  fSourceNav = fopen(AbsFileName, "wb");
  if(!fSourceNav)
  {
    fclose(fOldNav);
    WriteLogMC("MovieCutterLib", "PatchNavFiles() E0702.");
    TRACEEXIT();
    return FALSE;
  }

  //Create and open the cut nav
  TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s%s/%s.nav", TAPFSROOT, Directory, CutFileName);
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

    // Versuche, nav-Dateien aus Timeshift-Aufnahmen zu unterstützen ***experimentell***
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
      // Falls HD: Betrachte jeden tnavHD-Record als 2 tnavSD-Records, verwende den ersten und überspringe den zweiten
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

        if (FirstCutTime == 0xFFFFFFFF) FirstCutTime = navOld[i].Timems;
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
    *OutCutStartTime = FirstCutTime;
    *OutBehindCutTime = (FirstSourceTime) ? FirstSourceTime : LastCutTime;
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
tTimeStamp* NavLoad(char const *SourceFileName, char const *Directory, dword *const NrTimeStamps, bool isHD)
{
  char                  AbsFileName[FBLIB_DIR_SIZE];
  FILE                 *fNav = NULL;
  tnavSD               *navBuffer = NULL;
  tTimeStamp           *TimeStampBuffer = NULL;
  tTimeStamp           *TimeStamps = NULL;
  dword                 NavRecordsNr;
  dword                 ret, i;
  dword                 FirstTime;
  dword                 LastTimeStamp;
  ulong64               AbsPos;
  dword                 NavSize;

  TRACEENTER();
  *NrTimeStamps = 0;

  // Open the nav file
  TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s%s/%s.nav", TAPFSROOT, Directory, SourceFileName);
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

#ifdef FULLDEBUG
  TAP_PrintNet("NavSize: %lu\t\tBufSize: %lu\n", NavSize, NavRecordsNr * sizeof(tTimeStamp));
  TAP_PrintNet("Expected Nav-Records: %lu\n", NavRecordsNr);
#endif

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

    // Versuche, nav-Dateien aus Timeshift-Aufnahmen zu unterstützen ***experimentell***
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
      // Falls HD: Betrachte jeden tnavHD-Record als 2 tnavSD-Records, verwende den ersten und überspringe den zweiten
      if (isHD && (i % 2 != 0)) continue;

      if(FirstTime == 0xFFFFFFFF) FirstTime = navBuffer[i].Timems;
      if(LastTimeStamp != navBuffer[i].Timems)
      {
        AbsPos = ((ulong64)(navBuffer[i].PHOffsetHigh) << 32) | navBuffer[i].PHOffset;
        TimeStampBuffer[*NrTimeStamps].BlockNr = CalcBlockSize(AbsPos);
        TimeStampBuffer[*NrTimeStamps].Timems = navBuffer[i].Timems;

/*        if (navBuffer[i].Timems >= FirstTime)
          // Timems ist größer als FirstTime -> kein Überlauf
          TimeStampBuffer[*NrTimeStamps].Timems = navBuffer[i].Timems - FirstTime;
        else if (FirstTime - navBuffer[i].Timems <= 3000)
          // Timems ist kaum kleiner als FirstTime -> liegt vermutlich am Anfang der Aufnahme
          TimeStampBuffer[*NrTimeStamps].Timems = 0;
        else
          // Timems ist (deutlich) kleiner als FirstTime -> ein Überlauf liegt vor
          TimeStampBuffer[*NrTimeStamps].Timems = (0xffffffff - FirstTime) + navBuffer[i].Timems + 1;
*/
        (*NrTimeStamps)++;
        LastTimeStamp = navBuffer[i].Timems;
      }
    }
  } while(ret == NAVRECS_SD);
#ifdef FULLDEBUG
  TAP_PrintNet("FirstTime: %lu\n", FirstTime);
  TAP_PrintNet("NrTimeStamps: %lu\n", *NrTimeStamps);
#endif

  // Free the nav-Buffer and close the file
  fclose(fNav);
  TAP_MemFree(navBuffer);

  // Reserve a new buffer of the correct size to hold only the different time stamps
  TimeStamps = (tTimeStamp*) TAP_MemAlloc(*NrTimeStamps * sizeof(tTimeStamp));
  if(!TimeStamps)
  {
    TAP_MemFree(TimeStampBuffer);
    WriteLogMC("MovieCutterLib", "NavLoad() E0b04");
    TRACEEXIT();
    return(NULL);
  }

  // Copy the time stamps to the new array
  memcpy(TimeStamps, TimeStampBuffer, *NrTimeStamps * sizeof(tTimeStamp));  
  TAP_MemFree(TimeStampBuffer);

  TRACEEXIT();
  return(TimeStamps);
}
