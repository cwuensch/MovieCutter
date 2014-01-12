#define _FILE_OFFSET_BITS  64
#define __USE_LARGEFILE64  1

#include                <stdio.h>
#include                <stdlib.h>
#include                <string.h>
#include                <utime.h>
#include                "tap.h"
#include                "libFireBird.h"
#include                "MovieCutterLib.h"


bool        FileCut(char *SourceFileName, char *CutFileName, dword StartBlock, dword NrBlocks);
bool        PatchRecFile(char const *SourceFileName, off_t RequestedCutPosition, byte CutPointArray[], off_t OutPatchedBytes[]);
bool        UnpatchRecFile(char const *SourceFileName, char const *CutFileName, off_t CutStartPos, off_t BehindCutPos, off_t const PatchedBytes[], dword NrPatchedBytes);
bool        ReadCutPointArea(char const *SourceFileName, off_t CutPosition, byte CutPointArray[]);
bool        ReadFirstAndLastCutPacket(char const *FileName, byte FirstCutPacket[], byte LastCutPacket[]);
bool        FindCutPointOffset(const byte CutPacket[], const byte CutPointArray[], long *const Offset);
bool        PatchInfFiles(char const *SourceFileName, char const *CutFileName, dword SourcePlayTime, tTimeStamp const *CutStartPoint, tTimeStamp const *BehindCutPoint);
bool        PatchNavFiles(char const *SourceFileName, char const *CutFileName, off_t CutStartPos, off_t BehindCutPos, bool isHD, dword *const OutCutStartTime, dword *const OutBehindCutTime, dword *const OutSourcePlayTime);
//bool        PatchNavFilesSD(char const *SourceFileName, char const *CutFileName, off_t CutStartPos, off_t BehindCutPos, dword *const OutCutStartTime, dword *const OutBehindCutTime);
//bool        PatchNavFilesHD(char const *SourceFileName, char const *CutFileName, off_t CutStartPos, off_t BehindCutPos, dword *const OutCutStartTime, dword *const OutBehindCutTime);
//tTimeStamp* NavLoadSD(char const *SourceFileName, dword *const NrTimeStamps);
//tTimeStamp* NavLoadHD(char const *SourceFileName, dword *const NrTimeStamps);

static int              PACKETSIZE = 192;
static int              CUTPOINTSEARCHRADIUS = 9024;
static int              CUTPOINTSECTORRADIUS = 2;
char                    LogString[512];


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
  TAP_SPrint(TimeString, "%u:%2.2u:%2.2u", Hour, Min, Sec);

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
  TAP_SPrint(TimeString, "%u:%2.2u:%2.2u.%03u", Hour, Min, Sec, Millisec);

  TRACEEXIT();
}

void GetNextFreeCutName(char const *SourceFileName, char *CutFileName, word LeaveNamesOut)
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
    TAP_SPrint(&NextFileName[NameLen], " (Cut-%d)%s", i, &SourceFileName[NameLen]);
    TAP_SPrint(&CutFileName[NameLen], " (Cut-%d)%s", i + LeaveNamesOut, &SourceFileName[NameLen]);
  } while (TAP_Hdd_Exist(NextFileName) || TAP_Hdd_Exist(CutFileName));

  TRACEEXIT();
}


// ----------------------------------------------------------------------------
//                       MovieCutter-Schnittfunktion
// ----------------------------------------------------------------------------
tResultCode MovieCutter(char *SourceFileName, char *CutFileName, tTimeStamp *CutStartPoint, tTimeStamp *BehindCutPoint, bool KeepCut, bool isHD)
{
  char                  CurrentDir[512];
  char                  FileName[MAX_FILE_NAME_SIZE + 1];
  off_t                 SourceFileSize, CutFileSize;
  dword                 SourceFileBlocks;
  off_t                 CutStartPos, BehindCutPos;
  long                  CutStartPosOffset, BehindCutPosOffset;
  dword                 SourcePlayTime = 0;
  bool                  SuppressNavGeneration;
  dword                 RecDate;
//  char                  TimeStr[16];
//  int                   i;

  TRACEENTER();
  DetectPacketSize(SourceFileName);
  byte                  CutPointArea1[CUTPOINTSEARCHRADIUS * 2], CutPointArea2[CUTPOINTSEARCHRADIUS * 2];
  byte                  FirstCutPacket[PACKETSIZE], LastCutPacket[PACKETSIZE];
  off_t                 PatchedBytes[4 * CUTPOINTSECTORRADIUS];

  SuppressNavGeneration = FALSE;

  // LOG file printing
  WriteLogMC("MovieCutterLib", "----------------------------------------");
  TAP_SPrint(LogString, "Source        = '%s'", SourceFileName);
  WriteLogMC("MovieCutterLib", LogString);
  TAP_SPrint(LogString, "Cut name      = '%s'", CutFileName);
  WriteLogMC("MovieCutterLib", LogString);

  HDD_TAP_GetCurrentDir(CurrentDir);
  if(!HDD_GetFileSizeAndInode(CurrentDir, SourceFileName, NULL, &SourceFileSize))
  {
    WriteLogMC("MovieCutterLib", "MovieCutter() E0001: cut file not created.");
    TRACEEXIT();
    return RC_Error;
  }
  SourceFileBlocks = CalcBlockSize(SourceFileSize);

  TAP_SPrint(LogString, "File size     = %llu Bytes (%u blocks)", SourceFileSize, SourceFileBlocks);
  WriteLogMC("MovieCutterLib", LogString);
  TAP_SPrint(LogString, "Dir           = '%s'", CurrentDir);
  WriteLogMC("MovieCutterLib", LogString);
  TAP_SPrint(LogString, "PacketSize    = %d", PACKETSIZE);
  WriteLogMC("MovieCutterLib", LogString);

  if (BehindCutPoint->BlockNr > SourceFileBlocks)
    BehindCutPoint->BlockNr = SourceFileBlocks - 1;
  CutStartPos = (off_t)CutStartPoint->BlockNr * BLOCKSIZE;
  BehindCutPos = (off_t)BehindCutPoint->BlockNr * BLOCKSIZE;

  TAP_SPrint(LogString, "CutStartBlock = %u,\tBehindCutBlock = %u, KeepCut = %s", CutStartPoint->BlockNr, BehindCutPoint->BlockNr, KeepCut ? "yes" : "no");
  WriteLogMC("MovieCutterLib", LogString);

  TAP_SPrint(LogString, "CutStartPos   = %llu,\tBehindCutPos = %llu", CutStartPos, BehindCutPos);
  WriteLogMC("MovieCutterLib", LogString);

  // Read the two blocks surrounding the cut points from the recording
  if(!ReadCutPointArea(SourceFileName, CutStartPos, CutPointArea1))
  {
    WriteLogMC("MovieCutterLib", "MovieCutter() W0001: nav creation suppressed.");
    SuppressNavGeneration = TRUE;
  }
  if(!ReadCutPointArea(SourceFileName, BehindCutPos, CutPointArea2))
  {
    WriteLogMC("MovieCutterLib", "MovieCutter() W0002: nav creation suppressed.");
    SuppressNavGeneration = TRUE;
  }

// *********** V!deotext Test
  char                 AbsFileName[MAX_FILE_NAME_SIZE + 1];
  FILE                *test;
  int                  iterations = 0;

  HDD_TAP_GetCurrentDir(CurrentDir);
  TAP_SPrint(AbsFileName, "%s%s/%s", TAPFSROOT, CurrentDir, SourceFileName);
  do
  {
    test = fopen(AbsFileName, "r+b");
    if (test == 0)
    {
      TAP_SPrint(LogString, "Could not obtain write handle for rec-file '%s'!", AbsFileName);
      WriteLogMC("MovieCutterLib", LogString);
      TAP_Delay(10);
    }
    iterations++;
  } while ((iterations <= 200) && (test == 0));
  if (test != 0)
  {
    WriteLogMC("MovieCutterLib", "Write handle for rec-file successfully obtained!");
    fclose(test);
  }
// *********** V!deotext Test End

  // Patch the rec-File to prevent the firmware from cutting in the middle of a packet
  if (!SuppressNavGeneration)
  {
    PatchRecFile(SourceFileName, CutStartPos, CutPointArea1, PatchedBytes);
    PatchRecFile(SourceFileName, BehindCutPos, CutPointArea2, &PatchedBytes[2 * CUTPOINTSECTORRADIUS]);
  }

  // DO THE CUTTING
  if(!FileCut(SourceFileName, CutFileName, CutStartPoint->BlockNr, BehindCutPoint->BlockNr - CutStartPoint->BlockNr))
  {
    WriteLogMC("MovieCutterLib", "MovieCutter() E0002: Firmware cutting routine failed.");
    TRACEEXIT();
    return RC_Error;
  }
  if(!TAP_Hdd_Exist(CutFileName))
  {
    WriteLogMC("MovieCutterLib", "MovieCutter() E0003: Cut file not created.");
    TRACEEXIT();
    return RC_Error;
  }

  // Detect the size of the cut file
  if(!HDD_GetFileSizeAndInode(CurrentDir, CutFileName, NULL, &CutFileSize))
  {
    WriteLogMC("MovieCutterLib", "MovieCutter() W0003: error detecting size of cut file.");
    SuppressNavGeneration = TRUE;
  }
  TAP_SPrint(LogString, "Cut file size: %llu Bytes (=%d blocks)", CutFileSize, CalcBlockSize(CutFileSize));
  WriteLogMC("MovieCutterLib", LogString);

  // Read the beginning and the ending from the cut file
  if(!ReadFirstAndLastCutPacket(CutFileName, FirstCutPacket, LastCutPacket))
  {
    WriteLogMC("MovieCutterLib", "MovieCutter() W0004: nav creation suppressed.");
    SuppressNavGeneration = TRUE;
  }

  // Detect the actual cutting positions (differing from requested cut points!) for the nav generation
  if(!SuppressNavGeneration)
  {
    if (FindCutPointOffset(FirstCutPacket, CutPointArea1, &CutStartPosOffset))
      CutStartPos = CutStartPos + CutStartPosOffset;
    else
    {
      WriteLogMC("MovieCutterLib", "MovieCutter() W0005: Cut start position not found.");
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
      WriteLogMC("MovieCutterLib", "MovieCutter() W0006: Cut end position not found.");
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
      WriteLogMC("MovieCutterLib", "MovieCutter() W0007: Both cut points not found. Nav creation suppressed.");
    TAP_SPrint(LogString, "Cut start offset: %d Bytes (=%d packets and %d Bytes), Cut end offset: %d Bytes (=%d packets and %d Bytes)", CutStartPosOffset, CutStartPosOffset/PACKETSIZE, labs(CutStartPosOffset%PACKETSIZE), BehindCutPosOffset, BehindCutPosOffset/PACKETSIZE, labs(BehindCutPosOffset%PACKETSIZE));
    WriteLogMC("MovieCutterLib", LogString);

    TAP_SPrint(LogString, "Real cut positions:  Cut Start = %llu, Behind Cut: %llu", CutStartPos, BehindCutPos);
    WriteLogMC("MovieCutterLib", LogString);

#ifdef FULLDEBUG
    int i;
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
      TAP_SPrint(LogString, "!! -- Real cutting points NOT correctly guessed: GuessedStart = %llu, GuessedBehind = %llu", GuessedCutStartPos, GuessedBehindCutPos);
      WriteLogMC("MovieCutterLib", LogString);
    }
#endif
  }

  // Copy the real cutting positions into the cut point parameters to be returned
  if (!SuppressNavGeneration)
  {
    CutStartPoint->BlockNr = CalcBlockSize(CutStartPos + BLOCKSIZE/2);
    BehindCutPoint->BlockNr = CutStartPoint->BlockNr + CalcBlockSize(BehindCutPos - CutStartPos + BLOCKSIZE-1);
  }

  // Unpatch the rec-File
  if (!SuppressNavGeneration)
    UnpatchRecFile(SourceFileName, CutFileName, CutStartPos, BehindCutPos, PatchedBytes, 4 * CUTPOINTSECTORRADIUS);

  // Rename old nav file to bak
  char BakFileName[MAX_FILE_NAME_SIZE + 1];
  TAP_SPrint(FileName, "%s.nav", SourceFileName);
  TAP_SPrint(BakFileName, "%s.nav.bak", SourceFileName);
  if (TAP_Hdd_Exist(FileName))
  {
    if (TAP_Hdd_Exist(BakFileName))
      TAP_Hdd_Delete(BakFileName);
    TAP_Hdd_Rename(FileName, BakFileName);
  }

  // Patch the nav files (and get the TimeStamps for the actual cutting positions)
  if(!SuppressNavGeneration)
  {
    if(PatchNavFiles(SourceFileName, CutFileName, CutStartPos, BehindCutPos, isHD, &(CutStartPoint->Timems), &(BehindCutPoint->Timems), &SourcePlayTime))
      TAP_Hdd_Delete(BakFileName);
    else
    {
      WriteLogMC("MovieCutterLib", "MovieCutter() W0008: nav creation failed.");
      SuppressNavGeneration = TRUE;
      TAP_SPrint(FileName, "%s.nav", SourceFileName);
      TAP_Hdd_Delete(FileName);
      TAP_SPrint(FileName, "%s.nav", CutFileName);
      TAP_Hdd_Delete(FileName);
    }
  }

  // Copy the inf file and patch both play lengths
  if (!PatchInfFiles(SourceFileName, CutFileName, SourcePlayTime, CutStartPoint, BehindCutPoint))
    WriteLogMC("MovieCutterLib", "MovieCutter() W0009: inf creation failed.");

  // Fix the date info of all involved files
  if (GetRecDateFromInf(SourceFileName, &RecDate)) {
    //Source
    HDD_SetFileDateTime(CurrentDir, SourceFileName, RecDate);
    TAP_SPrint(FileName, "%s.inf", SourceFileName);
    HDD_SetFileDateTime(CurrentDir, FileName, RecDate);
    TAP_SPrint(FileName, "%s.nav", SourceFileName);
    HDD_SetFileDateTime(CurrentDir, FileName, RecDate);
  }
  if (GetRecDateFromInf(CutFileName, &RecDate)) {
    //Cut
    HDD_SetFileDateTime(CurrentDir, CutFileName, RecDate);
    TAP_SPrint(FileName, "%s.inf", CutFileName);
    HDD_SetFileDateTime(CurrentDir, FileName, RecDate);
    TAP_SPrint(FileName, "%s.nav", CutFileName);
    HDD_SetFileDateTime(CurrentDir, FileName, RecDate);
  }
//  if(!KeepSource) HDD_Delete(SourceFileName);
  if(!KeepCut) HDD_Delete(CutFileName);

  WriteLogMC("MovieCutterLib", "MovieCutter() finished.");

  TRACEEXIT();
  return ((SuppressNavGeneration) ? RC_Warning : RC_Ok);
}

// ----------------------------------------------------------------------------
//              Firmware-Funktion zum Durchführen des Schnitts
// ----------------------------------------------------------------------------
bool FileCut(char *SourceFileName, char *CutFileName, dword StartBlock, dword NrBlocks)
{
  tDirEntry             FolderStruct, *pFolderStruct;
  dword                 x;
  dword                 ret;
  TYPE_PlayInfo         PlayInfo;
  char                  CurrentDir[512], TAPDir[512];

  TRACEENTER();
  #ifdef FULLDEBUG
    WriteLogMC("MovieCutterLib", "FileCut()");
  #endif

  //Initialize the directory structure
  memset(&FolderStruct, 0, sizeof(tDirEntry));
  FolderStruct.Magic = 0xbacaed31;
  HDD_TAP_GetCurrentDir(TAPDir);

  TAP_Hdd_Delete(CutFileName);

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
  ret = ApplHdd_FileCutPaste(SourceFileName, StartBlock, NrBlocks, CutFileName);

  //Restore all resources
  DevHdd_DeviceClose(&pFolderStruct);
  ApplHdd_RestoreWorkFolder();

//  TAP_Hdd_ChangeDir(TAPDir);
  HDD_ChangeDir(TAPDir);

  if(ret)
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
  TYPE_File            *f;
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

bool HDD_SetFileDateTime(char const *Directory, char const *FileName, dword NewDateTime)
{
  char                  AbsFileName[256];
  tstat64               statbuf;
  int                   status;
  struct utimbuf        utimebuf;

  TRACEENTER();

  TAP_SPrint(AbsFileName, "%s%s/%s", TAPFSROOT, Directory, FileName);
  if((status = lstat64(AbsFileName, &statbuf)))
  {
    TAP_SPrint(LogString, "HDD_SetFileDateTime(%s) E0a01.", AbsFileName);
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

  TAP_SPrint(LogString, "HDD_SetFileDateTime(%s) E0a02.", AbsFileName);
  WriteLogMC("MovieCutterLib", LogString);

  TRACEEXIT();
  return FALSE;
}


// ----------------------------------------------------------------------------
//                         Analyse von REC-Files
// ----------------------------------------------------------------------------
int DetectPacketSize(char const *SourceFileName)
{
  char                 RecExtension[5];

  TRACEENTER();

  strcpy(RecExtension, &SourceFileName[strlen(SourceFileName) - 4]);
  if (strcmp(RecExtension, ".mpg") == 0)
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

bool isNavAvailable(char const *SourceFileName)
{
  char                  NavFileName[MAX_FILE_NAME_SIZE + 1];
  bool                  ret;

  TRACEENTER();

  TAP_SPrint(NavFileName, "%s.nav", SourceFileName);
  ret = TAP_Hdd_Exist(NavFileName);

  TRACEEXIT();
  return ret;
}

// TODO: Auslesen der Stream-Information aus dem InfCache im RAM
bool isCrypted(char const *SourceFileName)
{
  TYPE_File            *f;
  byte                  CryptFlag = 2;
  char                  InfFileName[MAX_FILE_NAME_SIZE + 1];
  bool                  ret;

  TRACEENTER();

  TAP_SPrint(InfFileName, "%s.inf", SourceFileName);
  f = TAP_Hdd_Fopen(InfFileName);
  if(f)
  {
    TAP_Hdd_Fseek(f, 0x0010, SEEK_SET);
    TAP_Hdd_Fread(&CryptFlag, 1, 1, f);
    TAP_Hdd_Fclose(f);
    ret = ((CryptFlag & 1) != 0);
  }
  else
    ret = TRUE;

  TRACEEXIT();
  return ret;
}

// TODO: Auslesen der Stream-Information aus dem InfCache im RAM
bool isHDVideo(char const *SourceFileName, bool *const isHD)
{
  TYPE_File            *f;
  byte                  StreamType = STREAM_UNKNOWN;
  char                  InfFileName[MAX_FILE_NAME_SIZE + 1];

  TRACEENTER();

  TAP_SPrint(InfFileName, "%s.inf", SourceFileName);
  f = TAP_Hdd_Fopen(InfFileName);
  if(f)
  {
    TAP_Hdd_Fseek(f, 0x0042, SEEK_SET);
    TAP_Hdd_Fread(&StreamType, 1, 1, f);
    TAP_Hdd_Fclose(f);

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
bool WriteByteToFile(char const *FileName, off_t BytePosition, char OldValue, char NewValue)
{
  char                  AbsFileName[512];
  char                  CurrentDir[512];
  FILE                 *f;
  char                  ret;

  TRACEENTER();
  #ifdef FULLDEBUG
    TAP_SPrint(LogString, "WriteByteToFile(file=%s, position=%llu, old=%c, new=%c.", FileName, BytePosition, OldValue, NewValue);
    WriteLogMC("MovieCutterLib", LogString);
  #endif

  // Open the file for write access
  HDD_TAP_GetCurrentDir(CurrentDir);
  TAP_SPrint(AbsFileName, "%s%s/%s", TAPFSROOT, CurrentDir, FileName);
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
  fclose(f);
  if(ret != NewValue)
  {
    WriteLogMC("MovieCutterLib", "WriteByteToFile() E0e04.");
    TRACEEXIT();
    return FALSE;
  }

  // Print to log
  if (NewValue != 'G')
    TAP_SPrint(LogString, "PatchRecFile(): Replaced Sync-Byte in file %s at position %llu.", FileName, BytePosition);
  else
    TAP_SPrint(LogString, "UnpatchRecFile(): Restored Sync-Byte in file %s at position %llu.", FileName, BytePosition);
  WriteLogMC("MovieCutterLib", LogString);

  TRACEEXIT();
  return TRUE;
}

// Patches the rec-File to prevent the firmware from cutting in the middle of a packet
bool PatchRecFile(char const *SourceFileName, off_t RequestedCutPosition, byte CutPointArray[], off_t OutPatchedBytes[])
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
        if (WriteByteToFile(SourceFileName, pos+4, 'G', 'F'))
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
bool UnpatchRecFile(char const *SourceFileName, char const *CutFileName, off_t CutStartPos, off_t BehindCutPos, off_t const PatchedBytes[], dword NrPatchedBytes)
{
  word                  i;
  int                   ret = 0;

  TRACEENTER();

//  #ifdef FULLDEBUG
    WriteLogMC("MovieCutterLib", "UnpatchRecFile()");

    TAP_SPrint(LogString, "Patched Bytes: ");
    for (i = 0; i < NrPatchedBytes; i++)
    {
      TAP_SPrint(&LogString[strlen(LogString)], "%llu%s", PatchedBytes[i], (i < NrPatchedBytes-1) ? ", " : "");
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
        ret = ret + !WriteByteToFile(SourceFileName, PatchedBytes[i], 'F', 'G');
      else if (PatchedBytes[i] < BehindCutPos)
        ret = ret + !WriteByteToFile(CutFileName, PatchedBytes[i] - CutStartPos, 'F', 'G');
      else
        ret = ret + !WriteByteToFile(SourceFileName, PatchedBytes[i] - (BehindCutPos - CutStartPos), 'F', 'G');
    }
  }

  TRACEEXIT();
  return (ret == 0);
}


// ----------------------------------------------------------------------------
//               Ermittlung der tatsächlichen Schnittposition
// ----------------------------------------------------------------------------
bool ReadCutPointArea(char const *SourceFileName, off_t CutPosition, byte CutPointArray[])
{
  char                  AbsFileName[512];
  char                  CurrentDir[512];
  FILE                 *f;
  size_t                ret;

  TRACEENTER();
  #ifdef FULLDEBUG
    WriteLogMC("MovieCutterLib", "ReadCutPointArea()");
  #endif

  // Open the rec for read access
  HDD_TAP_GetCurrentDir(CurrentDir);
  TAP_SPrint(AbsFileName, "%s%s/%s", TAPFSROOT, CurrentDir, SourceFileName);
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

bool ReadFirstAndLastCutPacket(char const *FileName, byte FirstCutPacket[], byte LastCutPacket[])
{
  char                  AbsFileName[512];
  char                  CurrentDir[512];
  FILE                 *f;
  size_t                ret;

  TRACEENTER();
  #ifdef FULLDEBUG
    WriteLogMC("MovieCutterLib", "ReadFirstAndLastCutPacket()");
  #endif

  // Open the rec for read access
  HDD_TAP_GetCurrentDir(CurrentDir);
  TAP_SPrint(AbsFileName, "%s%s/%s", TAPFSROOT, CurrentDir, FileName);
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
bool GetRecDateFromInf(char const *FileName, dword *const DateTime)
{
  TYPE_File            *f;
  char                  InfFileName[MAX_FILE_NAME_SIZE + 1];

  TRACEENTER();

  TAP_SPrint(InfFileName, "%s.inf", FileName);
  f = TAP_Hdd_Fopen(InfFileName);
  if(f)
  {
    TAP_Hdd_Fseek(f, 0x08, 0);
    TAP_Hdd_Fread(DateTime, sizeof(dword), 1, f);
    TAP_Hdd_Fclose(f);
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

bool PatchInfFiles(char const *SourceFileName, char const *CutFileName, dword SourcePlayTime, tTimeStamp const *CutStartPoint, tTimeStamp const *BehindCutPoint)
{
  char                  InfFileName[MAX_FILE_NAME_SIZE + 1];
  char                  CurrentDir[512];
  char                  T1[12], T2[10], T3[12];
  byte                  Buffer[INFSIZE];
  tRECHeaderInfo        RECHeaderInfo;
  TYPE_File            *f2;
  FILE                 *f;
  dword                 fs;
  dword                 Bookmarks[177];
  dword                 CutPlayTime;
  dword                 OrigHeaderStartTime;
  word                  i;
  bool                  SetCutBookmark;
  bool                  result = TRUE;

  TRACEENTER();
  #ifdef FULLDEBUG
    WriteLogMC("MovieCutterLib", "PatchInfFiles()");
  #endif

  //Copy the orig inf to cut inf (abuse the log string buffer)
  TAP_SPrint(LogString, "cp \"%s%s/%s.inf\" \"%s%s/%s.inf\"", TAPFSROOT, CurrentDir, SourceFileName, TAPFSROOT, CurrentDir, CutFileName);
  system(LogString);


// DEBUG
//Calculate the new play times
CutPlayTime = (dword)(((BehindCutPoint->Timems - CutStartPoint->Timems) + 500) / 1000);
SecToTimeString(CutPlayTime, T2);
TAP_SPrint(LogString, "Playtimes: Cut = %s", T2);
WriteLogMC("MovieCutterLib", LogString);
// DEBUG


  //Read the source .inf
  HDD_TAP_GetCurrentDir(CurrentDir);
//  TAP_SPrint(InfFileName, "%s.inf", SourceFileName);
//  f2 = TAP_Hdd_Fopen(InfFileName);
  TAP_SPrint(InfFileName, "%s%s/%s.inf", TAPFSROOT, CurrentDir, SourceFileName);
  f = fopen(InfFileName, "rb");
  if(!f)
  {
    WriteLogMC("MovieCutterLib", "PatchInfFiles() E0901: source inf not patched, cut inf not created.");
    TRACEEXIT();
    return FALSE;
  }

//  fs = TAP_Hdd_Flen(f2);
//  TAP_Hdd_Fread(Buffer, 1, INFSIZE, f2);
//  TAP_Hdd_Fclose(f2);
  fseek(f, 0, SEEK_END);
  fs = ftell(f);
  rewind(f);
dword BytesRead = fread(Buffer, 1, INFSIZE, f);
  fclose(f);
TAP_SPrint(LogString, "PatchInfFiles(): %u / %u Bytes read.", BytesRead, fs);
WriteLogMC("MovieCutterLib", LogString);

  //Decode the source .inf
  if(!HDD_DecodeRECHeader(Buffer, &RECHeaderInfo, ST_UNKNOWN))
  {
    WriteLogMC("MovieCutterLib", "PatchInfFiles() E0902: source inf not patched, cut inf not created.");
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
  TAP_SPrint(LogString, "Playtimes: Orig = %s, Cut = %s, New = %s", T1, T2, T3);
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
  TAP_SPrint(LogString, "Bookmarks: ");
  for(i = 0; i < 177; i++)
  {
    if(Bookmarks[i] == 0) break;
    TAP_SPrint(&LogString[strlen(LogString)], "%u ", Bookmarks[i]);
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

  TAP_SPrint(LogString, "Bookmarks->Source: ");
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
          TAP_SPrint(&LogString[strlen(LogString)], "*%u ", CutStartPoint->BlockNr);
          RECHeaderInfo.NrBookmarks++;
          SetCutBookmark = FALSE;
        }
        RECHeaderInfo.Bookmark[RECHeaderInfo.NrBookmarks] = Bookmarks[i] - (BehindCutPoint->BlockNr - CutStartPoint->BlockNr);
      }
      TAP_SPrint(&LogString[strlen(LogString)], "%u ", RECHeaderInfo.Bookmark[RECHeaderInfo.NrBookmarks]);
      RECHeaderInfo.NrBookmarks++;
    }
    if(Bookmarks[i+1] == 0) break;
  }
  // Setzt automatisch ein Bookmark an die Schnittstelle
  if (SetCutBookmark)
  {
    RECHeaderInfo.Bookmark[RECHeaderInfo.NrBookmarks] = CutStartPoint->BlockNr;
    TAP_SPrint(&LogString[strlen(LogString)], "*%u ", CutStartPoint->BlockNr);
    RECHeaderInfo.NrBookmarks++;
  }
  WriteLogMC("MovieCutterLib", LogString);

  //Encode and write the modified source inf
  if(HDD_EncodeRECHeader(Buffer, &RECHeaderInfo, ST_UNKNOWN))
  {
    TAP_SPrint(InfFileName, "%s%s/%s.inf", TAPFSROOT, CurrentDir, SourceFileName);
    f = fopen(InfFileName, "r+b");
    if(f != 0)
    {
      fseek(f, 0, SEEK_SET);
      fwrite(Buffer, 1, min(fs, INFSIZE), f);
      fclose(f);
    }
    else
    {
      WriteLogMC("MovieCutterLib", "PatchInfFiles() W0902: source inf not patched.");
      result = FALSE;
    }
  }
  else
  {
    WriteLogMC("MovieCutterLib", "PatchInfFiles() W0901: source inf not patched.");
    result = FALSE;
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
  TAP_SPrint(LogString, "Bookmarks->Cut: ");
  for(i = 0; i < 177; i++)
  {
    if((Bookmarks[i] >= CutStartPoint->BlockNr + 100) && (Bookmarks[i] + 100 < BehindCutPoint->BlockNr))
    {
      RECHeaderInfo.Bookmark[RECHeaderInfo.NrBookmarks] = Bookmarks[i] - CutStartPoint->BlockNr;
      TAP_SPrint(&LogString[strlen(LogString)], "%u ", RECHeaderInfo.Bookmark[RECHeaderInfo.NrBookmarks]);
      RECHeaderInfo.NrBookmarks++;
    }
    if(Bookmarks[i+1] == 0) break;
  }
  WriteLogMC("MovieCutterLib", LogString);

  //Encode the cut inf and write it to the disk
  if(HDD_EncodeRECHeader(Buffer, &RECHeaderInfo, ST_UNKNOWN))
  {
    TAP_SPrint(InfFileName, "%s.inf", CutFileName);
    if(!TAP_Hdd_Exist(InfFileName)) TAP_Hdd_Create(InfFileName, ATTR_NORMAL);

    TAP_SPrint(InfFileName, "%s%s/%s.inf", TAPFSROOT, CurrentDir, CutFileName);
    f = fopen(InfFileName, "r+b");
    if(f != 0)
    {
      fseek(f, 0, SEEK_SET);
      fwrite(Buffer, 1, min(fs, INFSIZE), f);
      fclose(f);
    }
    else
    {
      WriteLogMC("MovieCutterLib", "PatchInfFiles() E0904: cut inf not created.");
      result = FALSE;
    }
  }
  else
  {
    WriteLogMC("MovieCutterLib", "PatchInfFiles() E0903: cut inf not created.");
    result = FALSE;
  }

  TRACEEXIT();
  return result;
}


// ----------------------------------------------------------------------------
//                              NAV-Patchen
// ----------------------------------------------------------------------------
/*bool PatchNavFiles(char const *SourceFileName, char const *CutFileName, off_t CutStartPos, off_t BehindCutPos, bool isHD, dword *const OutCutStartTime, dword *const OutBehindCutTime)
{
  bool ret;

  TRACEENTER();

  if(isHD)
    ret = PatchNavFilesHD(SourceFileName, CutFileName, CutStartPos, BehindCutPos, OutCutStartTime, OutBehindCutTime);
  else
    ret = PatchNavFilesSD(SourceFileName, CutFileName, CutStartPos, BehindCutPos, OutCutStartTime, OutBehindCutTime);

  TRACEEXIT();
  return ret;
} */

bool PatchNavFiles(char const *SourceFileName, char const *CutFileName, off_t CutStartPos, off_t BehindCutPos, bool isHD, dword *const OutCutStartTime, dword *const OutBehindCutTime, dword *const OutSourcePlayTime)
{
  FILE                 *fSourceNav;
  TYPE_File            *fCutNav, *fSourceNavNew;
  char                  FileName[MAX_FILE_NAME_SIZE + 1];
  off_t                 PictureHeaderOffset;
  tnavSD               *navSource=NULL, *navSourceNew=NULL, *navCut=NULL;
  size_t                navsRead, navRecsSourceNew, navRecsCut, i;
  bool                  IFrameCut, IFrameSource;
  char                  CurrentDir[512];
  char                  AbsFileName[512];
  dword                 FirstCutTime, LastCutTime, FirstSourceTime, LastSourceTime;

  TRACEENTER();
  #ifdef FULLDEBUG
    WriteLogMC("MovieCutterLib", "PatchNavFilesSD()");
  #endif

  //Open the original nav
  HDD_TAP_GetCurrentDir(CurrentDir);
  TAP_SPrint(AbsFileName, "%s%s/%s.nav.bak", TAPFSROOT, CurrentDir, SourceFileName);
  fSourceNav = fopen(AbsFileName, "rb");
  if(!fSourceNav)
  {
    WriteLogMC("MovieCutterLib", "PatchNavFilesSD() E0701.");
    TRACEEXIT();
    return FALSE;
  }

  //Create and open the new source nav
  TAP_SPrint(FileName, "%s.nav", SourceFileName);
  TAP_Hdd_Delete(FileName);
  TAP_Hdd_Create(FileName, ATTR_NORMAL);
  fSourceNavNew = TAP_Hdd_Fopen(FileName);
  if(!fSourceNavNew)
  {
    fclose(fSourceNav);
    WriteLogMC("MovieCutterLib", "PatchNavFilesSD() E0702.");
    TRACEEXIT();
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
    TAP_Hdd_Fclose(fSourceNavNew);
    WriteLogMC("MovieCutterLib", "PatchNavFilesSD() E0703.");

    TRACEEXIT();
    return FALSE;
  }

  //Loop through the nav
  navSource = (tnavSD*) TAP_MemAlloc(NAVRECS_SD * sizeof(tnavSD));
  navSourceNew = (tnavSD*) TAP_MemAlloc(NAVRECS_SD * sizeof(tnavSD));
  navCut = (tnavSD*) TAP_MemAlloc(NAVRECS_SD * sizeof(tnavSD));
  if(!navSource || !navSourceNew || !navCut)
  {
    fclose(fSourceNav);
    TAP_Hdd_Fclose(fSourceNavNew);
    TAP_Hdd_Fclose(fCutNav);
    TAP_MemFree(navSource);
    TAP_MemFree(navSourceNew);
    TAP_MemFree(navCut);
    WriteLogMC("MovieCutterLib", "PatchNavFilesSD() E0704.");

    TRACEEXIT();
    return FALSE;
  }

  navRecsSourceNew = 0;
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
    navsRead = fread(navSource, sizeof(tnavSD), NAVRECS_SD, fSourceNav);
    if(navsRead == 0) break;

    // Versuche, nav-Dateien aus Timeshift-Aufnahmen zu unterstützen ***experimentell***
    if(FirstRun)
    {
      FirstRun = FALSE;
      if(navSource[0].SHOffset == 0x72767062)  // 'bpvr'
      {
        fseek(fSourceNav, 1056, SEEK_SET);
        continue;
      }
    }

    for(i = 0; i < navsRead; i++)
    {
      // Falls HD: Betrachte jeden tnavHD-Record als 2 tnavSD-Records, verwende den ersten und überspringe den zweiten
      if (isHD && (i % 2 != 0)) continue;

      //Check if the entry lies between the CutPoints
      PictureHeaderOffset = ((off_t)(navSource[i].PHOffsetHigh) << 32) | navSource[i].PHOffset;
      if((PictureHeaderOffset >= CutStartPos) && (PictureHeaderOffset < BehindCutPos))
      {
        //nav entries for the cut file
        if(navRecsCut >= NAVRECS_SD)
        {
          TAP_Hdd_Fwrite(navCut, sizeof(tnavSD), navRecsCut, fCutNav);
          navRecsCut = 0;
        }

        if (FirstCutTime == 0xFFFFFFFF) FirstCutTime = navSource[i].Timems;
        LastCutTime = navSource[i].Timems;

        //Subtract CutStartPos from the cut .nav PH address
        PictureHeaderOffset = PictureHeaderOffset - CutStartPos;
        if((navSource[i].SHOffset >> 24) == 1) IFrameCut = TRUE;
        if(IFrameCut)
        {
          memcpy(&navCut[navRecsCut], &navSource[i], sizeof(tnavSD));
          navCut[navRecsCut].PHOffsetHigh = PictureHeaderOffset >> 32;
          navCut[navRecsCut].PHOffset = PictureHeaderOffset & 0xffffffff;
          navCut[navRecsCut].Timems = navCut[navRecsCut].Timems - FirstCutTime;
          navRecsCut++;

          if (isHD)
          {
            memcpy(&navCut[navRecsCut], &navSource[i+1], sizeof(tnavSD));
            navRecsCut++;
          }
        }
        IFrameSource = FALSE;
      }
      else
      {
        //nav entries for the new source file
        if(navRecsSourceNew >= NAVRECS_SD)
        {
          TAP_Hdd_Fwrite(navSourceNew, sizeof(tnavSD), navRecsSourceNew, fSourceNavNew);
          navRecsSourceNew = 0;
        }

        if (PictureHeaderOffset >= BehindCutPos)
        {
          if (FirstSourceTime == 0) FirstSourceTime = navSource[i].Timems; 
          LastSourceTime = navSource[i].Timems;
        }

        if((navSource[i].SHOffset >> 24) == 1) IFrameSource = TRUE;
        if(IFrameSource)
        {
          memcpy(&navSourceNew[navRecsSourceNew], &navSource[i], sizeof(tnavSD));

          //if ph offset >= BehindCutPos, subtract (BehindCutPos - CutStartPos)
          if(PictureHeaderOffset >= BehindCutPos)
          {
            PictureHeaderOffset = PictureHeaderOffset - (BehindCutPos - CutStartPos);
            navSourceNew[navRecsSourceNew].PHOffsetHigh = PictureHeaderOffset >> 32;
            navSourceNew[navRecsSourceNew].PHOffset = PictureHeaderOffset & 0xffffffff;
            navSourceNew[navRecsSourceNew].Timems = navSourceNew[navRecsSourceNew].Timems - (FirstSourceTime - FirstCutTime);
          }
          navRecsSourceNew++;

          if (isHD)
          {
            memcpy(&navSourceNew[navRecsSourceNew], &navSource[i+1], sizeof(tnavSD));
            navRecsSourceNew++;
          }
        }
        IFrameCut = FALSE;
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

  *OutCutStartTime = FirstCutTime;
  *OutBehindCutTime = (FirstSourceTime) ? FirstSourceTime : LastCutTime;
  *OutSourcePlayTime = (LastSourceTime) ? LastSourceTime : LastCutTime;

  //Delete the orig source nav and make the new source nav the active one
//  TAP_SPrint(FileName, "%s.nav.bak", SourceFileName);
//  TAP_Hdd_Delete(FileName);

  TRACEEXIT();
  return TRUE;
}

bool PatchNavFilesHD(char const *SourceFileName, char const *CutFileName, off_t CutStartPos, off_t BehindCutPos, dword *const OutCutStartTime, dword *const OutBehindCutTime)
{
  FILE                 *fSourceNav;
  TYPE_File            *fCutNav, *fSourceNavNew;
  char                  FileName[MAX_FILE_NAME_SIZE + 1];
  off_t                 PictureHeaderOffset;
  tnavHD               *navSource=NULL, *navSourceNew=NULL, *navCut=NULL;
  size_t                navsRead, navRecsSourceNew, navRecsCut, i;
  bool                  IFrameCut, IFrameSource;
  char                  CurrentDir[512];
  char                  AbsFileName[512];
  dword                 FirstCutTime, LastCutTime, FirstSourceTime;

  TRACEENTER();
  #ifdef FULLDEBUG
    WriteLogMC("MovieCutterLib", "PatchNavFilesHD()");
  #endif

  //Open the original nav
  HDD_TAP_GetCurrentDir(CurrentDir);
  TAP_SPrint(AbsFileName, "%s%s/%s.nav.bak", TAPFSROOT, CurrentDir, SourceFileName);
  fSourceNav = fopen(AbsFileName, "rb");
  if(!fSourceNav)
  {
    WriteLogMC("MovieCutterLib", "PatchNavFilesHD() E0801.");
    TRACEEXIT();
    return FALSE;
  }

  //Create and open the new source nav
  TAP_SPrint(FileName, "%s.nav", SourceFileName);
  TAP_Hdd_Delete(FileName);
  TAP_Hdd_Create(FileName, ATTR_NORMAL);
  fSourceNavNew = TAP_Hdd_Fopen(FileName);
  if(!fSourceNavNew)
  {
    fclose(fSourceNav);
    WriteLogMC("MovieCutterLib", "PatchNavFilesHD() E0802.");
    TRACEEXIT();
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
    TAP_Hdd_Fclose(fSourceNavNew);
    WriteLogMC("MovieCutterLib", "PatchNavFilesHD() E0803.");

    TRACEEXIT();
    return FALSE;
  }

  //Loop through the nav
  navSource = (tnavHD*) TAP_MemAlloc(NAVRECS_HD * sizeof(tnavHD));
  navSourceNew = (tnavHD*) TAP_MemAlloc(NAVRECS_HD * sizeof(tnavHD));
  navCut = (tnavHD*) TAP_MemAlloc(NAVRECS_HD * sizeof(tnavHD));
  if(!navSource || !navSourceNew || !navCut)
  {
    fclose(fSourceNav);
    TAP_Hdd_Fclose(fSourceNavNew);
    TAP_Hdd_Fclose(fCutNav);
    TAP_MemFree(navSource);
    TAP_MemFree(navSourceNew);
    TAP_MemFree(navCut);
    WriteLogMC("MovieCutterLib", "PatchNavFilesHD() E0804.");

    TRACEEXIT();
    return FALSE;
  }

  navRecsSourceNew = 0;
  navRecsCut = 0;
  IFrameCut = FALSE;
  IFrameSource = TRUE;
  FirstCutTime = 0xFFFFFFFF;
  LastCutTime = 0;
  FirstSourceTime = 0;
  bool FirstRun = TRUE;
  while(TRUE)
  {
    navsRead = fread(navSource, sizeof(tnavHD), NAVRECS_HD, fSourceNav);
    if(navsRead == 0) break;

    // Versuche, nav-Dateien aus Timeshift-Aufnahmen zu unterstützen ***experimentell***
    if(FirstRun)
    {
      FirstRun = FALSE;
      if(navSource[0].LastPPS == 0x72767062)  // 'bpvr'
      {
        fseek(fSourceNav, 1056, SEEK_SET);
        continue;
      }
    }

    for(i = 0; i < navsRead; i++)
    {
      //Check if the entry lies between the CutPoints
      PictureHeaderOffset = ((off_t)(navSource[i].SEIOffsetHigh) << 32) | navSource[i].SEIOffsetLow;
      if((PictureHeaderOffset >= CutStartPos) && (PictureHeaderOffset < BehindCutPos))
      {
        //nav entries for the cut file
        if(navRecsCut >= NAVRECS_HD)
        {
          TAP_Hdd_Fwrite(navCut, sizeof(tnavHD), navRecsCut, fCutNav);
          navRecsCut = 0;
        }

        if (FirstCutTime == 0xFFFFFFFF) FirstCutTime = navSource[i].Timems;
        LastCutTime = navSource[i].Timems;

        //Subtract CutStartPos from the cut .nav PH address
        PictureHeaderOffset = PictureHeaderOffset - CutStartPos;
        if((navSource[i].LastPPS >> 24) == 1) IFrameCut = TRUE;
        if(IFrameCut)
        {
          memcpy(&navCut[navRecsCut], &navSource[i], sizeof(tnavHD));
          navCut[navRecsCut].SEIOffsetHigh = PictureHeaderOffset >> 32;
          navCut[navRecsCut].SEIOffsetLow = PictureHeaderOffset & 0xffffffff;
          navCut[navRecsCut].Timems = navCut[navRecsCut].Timems - FirstCutTime;
          navRecsCut++;
        }
        IFrameSource = FALSE;
      }
      else
      {
        //nav entries for the new source file
        if(navRecsSourceNew >= NAVRECS_HD)
        {
          TAP_Hdd_Fwrite(navSourceNew, sizeof(tnavHD), navRecsSourceNew, fSourceNavNew);
          navRecsSourceNew = 0;
        }

        if (PictureHeaderOffset >= BehindCutPos)
          if (FirstSourceTime == 0) FirstSourceTime = navSource[i].Timems; 
//        LastSourceTime = navSource[i].Timems;

        if((navSource[i].LastPPS >> 24) == 1) IFrameSource = TRUE;
        if(IFrameSource)
        {
          memcpy(&navSourceNew[navRecsSourceNew], &navSource[i], sizeof(tnavHD));

          //if ph offset >= BehindCutPos, subtract (BehindCutPos - CutStartPos)
          if(PictureHeaderOffset >= BehindCutPos)
          {
            PictureHeaderOffset = PictureHeaderOffset - (BehindCutPos - CutStartPos);
            navSourceNew[navRecsSourceNew].SEIOffsetHigh = PictureHeaderOffset >> 32;
            navSourceNew[navRecsSourceNew].SEIOffsetLow = PictureHeaderOffset & 0xffffffff;
            navSourceNew[navRecsSourceNew].Timems = navSourceNew[navRecsSourceNew].Timems - (FirstSourceTime - FirstCutTime);
          }
          navRecsSourceNew++;
        }
        IFrameCut = FALSE;
      }
    }
  }

  if(navRecsCut > 0) TAP_Hdd_Fwrite(navCut, sizeof(tnavHD), navRecsCut, fCutNav);
  if(navRecsSourceNew > 0) TAP_Hdd_Fwrite(navSourceNew, sizeof(tnavHD), navRecsSourceNew, fSourceNavNew);

  fclose(fSourceNav);
  TAP_Hdd_Fclose(fCutNav);
  TAP_Hdd_Fclose(fSourceNavNew);

  TAP_MemFree(navSource);
  TAP_MemFree(navSourceNew);
  TAP_MemFree(navCut);

  *OutCutStartTime = FirstCutTime;
  *OutBehindCutTime = (FirstSourceTime) ? FirstSourceTime : LastCutTime;

  //Delete the orig source nav and make the new source nav the active one
//  TAP_SPrint(FileName, "%s.nav.bak", SourceFileName);
//  TAP_Hdd_Delete(FileName);

  TRACEEXIT();
  return TRUE;
}

// ----------------------------------------------------------------------------
//                               NAV-Einlesen
// ----------------------------------------------------------------------------
/*tTimeStamp* NavLoad(char const *SourceFileName, dword *const NrTimeStamps, bool isHD)
{
  tTimeStamp* ret;

  TRACEENTER();

  if(isHD)
    ret = NavLoadHD(SourceFileName, NrTimeStamps);
  else
    ret = NavLoadSD(SourceFileName, NrTimeStamps);

  TRACEEXIT();
  return ret;
} */

tTimeStamp* NavLoad(char const *SourceFileName, dword *const NrTimeStamps, bool isHD)
{
  char                  CurrentDir[512];
  char                  AbsFileName[512];
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
  HDD_TAP_GetCurrentDir(CurrentDir);
  TAP_SPrint(AbsFileName, "%s%s/%s.nav", TAPFSROOT, CurrentDir, SourceFileName);
  fNav = fopen(AbsFileName, "rb");
  if(!fNav)
  {
    WriteLogMC("MovieCutterLib", "NavLoadSD() E0b01");
    TRACEEXIT();
    return(NULL);
  }

  // Reserve a (temporary) buffer to hold the entire file
  fseek(fNav, 0, SEEK_END);
  NavSize = ftell(fNav);
  rewind(fNav);
  NavRecordsNr = NavSize / (sizeof(tnavSD) * ((isHD) ? 2 : 1));

  TimeStampBuffer = (tTimeStamp*) TAP_MemAlloc(sizeof(tTimeStamp) * NavRecordsNr);
  if (!TimeStampBuffer)
  {
    fclose(fNav);
    WriteLogMC("MovieCutterLib", "NavLoadSD() E0b02");
    TRACEEXIT();
    return(NULL);
  }

#ifdef FULLDEBUG
  TAP_PrintNet("NavSize: %u\t\tBufSize: %u\n", NavSize, sizeof(tTimeStamp) * NavRecordsNr);
  TAP_PrintNet("Expected Nav-Records: %u\n", NavRecordsNr);
#endif

  //Count and save all the _different_ time stamps in the .nav
  LastTimeStamp = 0xFFFFFFFF;
  FirstTime = 0xFFFFFFFF;
  navBuffer = (tnavSD*) TAP_MemAlloc(NAVRECS_SD * sizeof(tnavSD));
  if (!navBuffer)
  {
    fclose(fNav);
    TAP_MemFree(TimeStampBuffer);
    WriteLogMC("MovieCutterLib", "NavLoadSD() E0b03");

    TRACEEXIT();
    return(NULL);
  }
  do
  {
    ret = fread(navBuffer, sizeof(tnavSD), NAVRECS_SD, fNav);

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
  TAP_PrintNet("FirstTime: %u\n", FirstTime);
  TAP_PrintNet("NrTimeStamps: %u\n", *NrTimeStamps);
#endif

  // Free the nav-Buffer and close the file
  fclose(fNav);
  TAP_MemFree(navBuffer);

  // Reserve a new buffer of the correct size to hold only the different time stamps
  TimeStamps = (tTimeStamp*) TAP_MemAlloc(*NrTimeStamps * sizeof(tTimeStamp));
  if(!TimeStamps)
  {
    TAP_MemFree(TimeStampBuffer);
    WriteLogMC("MovieCutterLib", "NavLoadSD() E0b04");
    TRACEEXIT();
    return(NULL);
  }

  // Copy the time stamps to the new array
  memcpy(TimeStamps, TimeStampBuffer, *NrTimeStamps * sizeof(tTimeStamp));  
  TAP_MemFree(TimeStampBuffer);

  TRACEEXIT();
  return(TimeStamps);
}

tTimeStamp* NavLoadHD(char const *SourceFileName, dword *const NrTimeStamps)
{
  char                  CurrentDir[512];
  char                  AbsFileName[512];
  FILE                 *fNav = NULL;
  tnavHD               *navBuffer = NULL;
  tTimeStamp           *TimeStampBuffer = NULL;
  tTimeStamp           *TimeStamps = NULL;
  dword                 ret, i;
  dword                 FirstTime;
  dword                 LastTimeStamp;
  ulong64               AbsPos;
  dword                 NavSize;

  TRACEENTER();
  *NrTimeStamps = 0;

  // Open the nav file
  HDD_TAP_GetCurrentDir(CurrentDir);
  TAP_SPrint(AbsFileName, "%s%s/%s.nav", TAPFSROOT, CurrentDir, SourceFileName);
  fNav = fopen(AbsFileName, "rb");
  if(!fNav)
  {
    WriteLogMC("MovieCutterLib", "NavLoadHD() E0c01");
    TRACEEXIT();
    return(NULL);
  }

  // Reserve a (temporary) buffer to hold the entire file
  fseek(fNav, 0, SEEK_END);
  NavSize = ftell(fNav);
  rewind(fNav);
  TimeStampBuffer = (tTimeStamp*) TAP_MemAlloc(sizeof(tTimeStamp) * (NavSize / sizeof(tnavHD)));
  if (!TimeStampBuffer)
  {
    fclose(fNav);
    WriteLogMC("MovieCutterLib", "NavLoadHD() E0c02");
    TRACEEXIT();
    return(NULL);
  }

#ifdef FULLDEBUG
  TAP_PrintNet("NavSize: %u\t\tBufSize: %u\n", NavSize, sizeof(tTimeStamp) * (NavSize / sizeof(tnavHD)));
  TAP_PrintNet("Expected Nav-Records: %u\n", NavSize / sizeof(tnavHD));
#endif

  //Count and save all the _different_ time stamps in the .nav
  LastTimeStamp = 0xFFFFFFFF;
  FirstTime = 0xFFFFFFFF;
  navBuffer = (tnavHD*) TAP_MemAlloc(NAVRECS_HD * sizeof(tnavHD));
  if (!navBuffer)
  {
    fclose(fNav);
    TAP_MemFree(TimeStampBuffer);
    WriteLogMC("MovieCutterLib", "NavLoadHD() E0c03");

    TRACEEXIT();
    return(NULL);
  }
  do
  {
    ret = fread(navBuffer, sizeof(tnavHD), NAVRECS_HD, fNav);

    // Versuche, nav-Dateien aus Timeshift-Aufnahmen zu unterstützen ***experimentell***
    if(FirstTime == 0xFFFFFFFF)
    {
      if(navBuffer[0].LastPPS == 0x72767062)  // 'bpvr'
      {
        fseek(fNav, 1056, SEEK_SET);
        continue;
      }
    }

    for(i = 0; i < ret; i++)
    {
      if(FirstTime == 0xFFFFFFFF) FirstTime = navBuffer[i].Timems;
      if(LastTimeStamp != navBuffer[i].Timems)
      {
        AbsPos = ((ulong64)(navBuffer[i].SEIOffsetHigh) << 32) | navBuffer[i].SEIOffsetLow;
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
  } while(ret == NAVRECS_HD);
#ifdef FULLDEBUG
  TAP_PrintNet("FirstTime: %u\n", FirstTime);
  TAP_PrintNet("NrTimeStamps: %u\n", *NrTimeStamps);
#endif

  // Free the nav-Buffer and close the file
  fclose(fNav);
  TAP_MemFree(navBuffer);

  // Reserve a new buffer of the correct size to hold only the different time stamps
  TimeStamps = (tTimeStamp*) TAP_MemAlloc(*NrTimeStamps * sizeof(tTimeStamp));
  if(!TimeStamps)
  {
    TAP_MemFree(TimeStampBuffer);
    WriteLogMC("MovieCutterLib", "NavLoadHD() E0c04");
    TRACEEXIT();
    return(NULL);
  }

  // Copy the time stamps to the new array
  memcpy(TimeStamps, TimeStampBuffer, *NrTimeStamps * sizeof(tTimeStamp));  
  TAP_MemFree(TimeStampBuffer);

  TRACEEXIT();
  return(TimeStamps);
}
