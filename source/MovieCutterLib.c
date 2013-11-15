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
bool        ReadCutPointArea(char const *SourceFileName, off_t CutPosition, byte CutPointArray[]);
bool        ReadFirstAndLastCutPacket(char const *FileName, byte FirstCutPacket[], byte LastCutPacket[]);
bool        FindCutPointOffset(const byte CutPacket[], const byte CutPointArray[], long *const Offset);
bool        PatchInfFiles(char const *SourceFileName, char const *CutFileName, tTimeStamp *CutStartPoint, tTimeStamp *BehindCutPoint);
bool        PatchNavFiles(char const *SourceFileName, char const *CutFileName, off_t CutStartPos, off_t BehindCutPos, bool isHD, dword *const CutStartTime, dword *const BehindCutTime);
bool        PatchNavFilesSD(char const *SourceFileName, char const *CutFileName, off_t CutStartPos, off_t BehindCutPos, dword *const OutCutStartTime, dword *const OutBehindCutTime);
bool        PatchNavFilesHD(char const *SourceFileName, char const *CutFileName, off_t CutStartPos, off_t BehindCutPos, dword *const OutCutStartTime, dword *const OutBehindCutTime);
tTimeStamp* NavLoadSD(char const *SourceFileName, dword *const NrTimeStamps);
tTimeStamp* NavLoadHD(char const *SourceFileName, dword *const NrTimeStamps);

char                    LogString[512];


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

void SecToTimeString(dword Time, char *const TimeString)  // needs max. 4 + 1 + 2 + 1 + 2 + 1 = 11 chars
{
  dword                 Hour, Min, Sec;

  Hour = (int)(Time / 3600);
  Min = (int)(Time / 60) % 60;
  Sec = Time % 60;
  if (Hour >= 10000) Hour = 9999;
  TAP_SPrint(TimeString, "%u:%2.2u:%2.2u", Hour, Min, Sec);
}
void MSecToTimeString(dword Timems, char *const TimeString)  // needs max. 4 + 1 + 2 + 1 + 2 + 1 + 3 + 1 = 15 chars
{
  dword                 Hour, Min, Sec, Millisec;

  Hour = (int)(Timems / 3600000);
  Min = (int)(Timems / 60000) % 60;
  Sec = (int)(Timems / 1000) % 60;
  Millisec = Timems % 1000;
  TAP_SPrint(TimeString, "%u:%2.2u:%2.2u.%03u", Hour, Min, Sec, Millisec);
}
void Print64BitLong(ulong64 Number, char *const OutString)  // needs max. 16 + 2 + 1 = 19 chars
{
  unsigned long LowPart, HighPart;
  HighPart = (Number >> 32);
  LowPart = (Number & 0xffffffff);
  if (HighPart != 0)
    TAP_SPrint(OutString, "0x%X%X", HighPart, LowPart);
  else
    TAP_SPrint(OutString, "0x%X", LowPart);
}


bool MovieCutter(char *SourceFileName, char *CutFileName, tTimeStamp *CutStartPoint, tTimeStamp *BehindCutPoint, bool KeepSource, bool KeepCut, bool isHD)
{
  char                  CurrentDir[512];
  char                  FileName[MAX_FILE_NAME_SIZE + 1];
  __off64_t             SourceFileSize, CutFileSize;
  byte                  CutPointArea1[CUTPOINTSEARCHRADIUS * 2], CutPointArea2[CUTPOINTSEARCHRADIUS * 2];
  byte                  FirstCutPacket[PACKETSIZE], LastCutPacket[PACKETSIZE];
  off_t                 CutStartPos, BehindCutPos;
  long                  CutStartPosOffset, BehindCutPosOffset;
  bool                  SuppressNavGeneration;
  dword                 RecDate;
  char                  SizeStr[20];
//  char                  TimeStr[16];
//  int                   i;

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
    return FALSE;
  }

  Print64BitLong(SourceFileSize, SizeStr);
  TAP_SPrint(LogString, "File size     = %u Bytes (%u blocks)", SizeStr, (dword)(SourceFileSize / BLOCKSIZE));
  WriteLogMC("MovieCutterLib", LogString);
  TAP_SPrint(LogString, "Dir           = '%s'", CurrentDir);
  WriteLogMC("MovieCutterLib", LogString);

  TAP_SPrint(LogString, "CutStartBlock = %12u,  BehindCutBlock = %12u, KeepSource = %s, KeepCut = %s", CutStartPoint->BlockNr, BehindCutPoint->BlockNr, KeepSource ? "yes" : "no", KeepCut ? "yes" : "no");
  WriteLogMC("MovieCutterLib", LogString);

  CutStartPos = CutStartPoint->BlockNr * BLOCKSIZE;
  BehindCutPos = BehindCutPoint->BlockNr * BLOCKSIZE;

  Print64BitLong(CutStartPos, SizeStr);
  TAP_SPrint(LogString, "CutStartPos   = %s", SizeStr);
  Print64BitLong(BehindCutPos, SizeStr);
  TAP_SPrint(&LogString[strlen(LogString)], ",  BehindCutPos = %s", SizeStr);
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

  // DO THE CUTTING
  if(!FileCut(SourceFileName, CutFileName, CutStartPoint->BlockNr, BehindCutPoint->BlockNr - CutStartPoint->BlockNr))
  {
    WriteLogMC("MovieCutterLib", "MovieCutter() E0002: Firmware cutting routine failed.");
    return FALSE;
  }
  if(!TAP_Hdd_Exist(CutFileName))
  {
    WriteLogMC("MovieCutterLib", "MovieCutter() E0003: Cut file not created.");
    return FALSE;
  }

  // Detect the size of the cut file
  if(!HDD_GetFileSizeAndInode(CurrentDir, CutFileName, NULL, &CutFileSize))
  {
    WriteLogMC("MovieCutterLib", "MovieCutter() W0003: error detecting size of cut file.");
    SuppressNavGeneration = TRUE;
  }
  Print64BitLong(CutFileSize, SizeStr);
  TAP_SPrint(LogString, "Cut file size: %s Bytes (=%d blocks)", SizeStr, (dword)(CutFileSize / BLOCKSIZE));
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
        SuppressNavGeneration = FALSE;
      }
    }
    else
    {
      WriteLogMC("MovieCutterLib", "MovieCutter() W0006: Cut end position not found.");
      // if cut end point was not found, re-calculate it from cut file size
      if (!SuppressNavGeneration && (CutFileSize != 0))
        BehindCutPos = CutStartPos + CutFileSize;
      else
        SuppressNavGeneration = TRUE;
    }
    if (SuppressNavGeneration)
      WriteLogMC("MovieCutterLib", "MovieCutter() W0007: Both cut points not found. Nav creation suppressed.");
    TAP_SPrint(LogString, "Cut start offset: %d Bytes (=%d packets and %d Bytes), Cut end offset: %d Bytes (=%d packets and %d Bytes)", CutStartPosOffset, CutStartPosOffset/PACKETSIZE, labs(CutStartPosOffset%PACKETSIZE), BehindCutPosOffset, BehindCutPosOffset/PACKETSIZE, labs(BehindCutPosOffset%PACKETSIZE));
    WriteLogMC("MovieCutterLib", LogString);
  }

  // Copy the real cutting positions into the cut point parameters to be returned
  if (!SuppressNavGeneration)
  {
    CutStartPoint->BlockNr = (dword)((CutStartPos + CutStartPosOffset) / BLOCKSIZE);
    BehindCutPoint->BlockNr = (dword)((BehindCutPos + BehindCutPosOffset) / BLOCKSIZE);
  }

  // Rename old nav file to bak
  char BakFileName[MAX_FILE_NAME_SIZE + 1];
  TAP_SPrint(FileName, "%s.nav", SourceFileName);
  TAP_SPrint(BakFileName, "%s.nav.bak", SourceFileName);
  TAP_Hdd_Rename(FileName, BakFileName);

  // Patch the nav files (and get the TimeStamps for the actual cutting positions)
  if(!SuppressNavGeneration)
  {
    if(PatchNavFiles(SourceFileName, CutFileName, CutStartPos, BehindCutPos, isHD, &(CutStartPoint->Timems), &(BehindCutPoint->Timems)))
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
  if (!PatchInfFiles(SourceFileName, CutFileName, CutStartPoint, BehindCutPoint))
    WriteLogMC("MovieCutterLib", "MovieCutter() W0009: inf creation failed.");

  // Fix the date info of all involved files
  if (GetRecDateFromInf(SourceFileName, &RecDate)) {
    //Source
    HDD_SetFileDateTime(CurrentDir, SourceFileName, RecDate);
    TAP_SPrint(FileName, "%s.inf", SourceFileName);
    HDD_SetFileDateTime(CurrentDir, FileName, RecDate);
    TAP_SPrint(FileName, "%s.nav", SourceFileName);
    HDD_SetFileDateTime(CurrentDir, FileName, RecDate);

    //Cut
    HDD_SetFileDateTime(CurrentDir, CutFileName, RecDate);
    TAP_SPrint(FileName, "%s.inf", CutFileName);
    HDD_SetFileDateTime(CurrentDir, FileName, RecDate);
    TAP_SPrint(FileName, "%s.nav", CutFileName);
    HDD_SetFileDateTime(CurrentDir, FileName, RecDate);
  }
  if(!KeepSource) HDD_Delete(SourceFileName);
  if(!KeepCut) HDD_Delete(CutFileName);

  WriteLogMC("MovieCutterLib", "MovieCutter() finished.");
  return TRUE;
}


bool isNavAvailable(char const *SourceFileName)
{
  char                  NavFileName[MAX_FILE_NAME_SIZE + 1];

  TAP_SPrint(NavFileName, "%s.nav", SourceFileName);
  return TAP_Hdd_Exist(NavFileName);
}

// TODO: Auslesen der Stream-Information aus dem InfCache im RAM
bool isCrypted(char const *SourceFileName)
{
  TYPE_File            *f;
  byte                  CryptFlag = 2;
  char                  InfFileName[MAX_FILE_NAME_SIZE + 1];

  TAP_SPrint(InfFileName, "%s.inf", SourceFileName);
  f = TAP_Hdd_Fopen(InfFileName);
  if(f)
  {
    TAP_Hdd_Fseek(f, 0x0010, SEEK_SET);
    TAP_Hdd_Fread(&CryptFlag, 1, 1, f);
    TAP_Hdd_Fclose(f);
    return ((CryptFlag & 1) != 0);
  }
  else
    return TRUE;
}

// TODO: Auslesen der Stream-Information aus dem InfCache im RAM
bool isHDVideo(char const *SourceFileName, bool *const isHD)
{
  TYPE_File            *f;
  byte                  StreamType = STREAM_UNKNOWN;
  char                  InfFileName[MAX_FILE_NAME_SIZE + 1];

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
      return FALSE;
    }
  }
  else
  {
    WriteLogMC("MovieCutterLib", "isHDVideo() E0101.");
    return FALSE;
  }
  return TRUE;
}


bool ReadCutPointArea(char const *SourceFileName, off_t CutPosition, byte CutPointArray[])
{
  char                  AbsFileName[512];
  char                  CurrentDir[512];
  FILE                 *f;
  off_t                 FileOffset;
  size_t                ret;

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
    return FALSE;
  }

  // Read the 2 blocks from the rec
  FileOffset = (off_t)((CutPosition >= CUTPOINTSEARCHRADIUS) ? CutPosition : CUTPOINTSEARCHRADIUS);
  fseeko64(f, FileOffset - CUTPOINTSEARCHRADIUS, SEEK_SET);
  ret = fread(&CutPointArray[0], CUTPOINTSEARCHRADIUS * 2, 1, f);
  if(ret != 1)
  {
    fclose(f);
    WriteLogMC("MovieCutterLib", "ReadCutPointArea() E0202.");
    return FALSE;
  }
  fclose(f);

  return TRUE;
}

bool ReadFirstAndLastCutPacket(char const *FileName, byte FirstCutPacket[], byte LastCutPacket[])
{
  char                  AbsFileName[512];
  char                  CurrentDir[512];
  FILE                 *f;
  size_t                ret;

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
      return FALSE;
    }
  }
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

  FirstByte = CutPacket[0];
  MidArray = &CutPointArray[CUTPOINTSEARCHRADIUS];  
  *Offset = CUTPOINTSEARCHRADIUS + 1;

  for (i = -CUTPOINTSEARCHRADIUS; i <= (CUTPOINTSEARCHRADIUS - PACKETSIZE); i++)
  {
    if (MidArray[i] == FirstByte)
    {
      if (memcmp(&MidArray[i], CutPacket, PACKETSIZE) == 0)
      {
        if (labs(*Offset) <= CUTPOINTSEARCHRADIUS)
        {
          WriteLogMC("MovieCutterLib", "FindCutPointOffset() W0401: cut packet found more than once.");
          if (i > labs(*Offset)) break;
        }
        *Offset = i;
      }
    }
  }
  return (labs(*Offset) <= CUTPOINTSEARCHRADIUS);
}

void GetNextFreeCutName(char const *SourceFileName, char *CutFileName, word LeaveNamesOut)
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
    TAP_SPrint(&CutFileName[NameLen], " (Cut-%d)%s", i + LeaveNamesOut, &SourceFileName[NameLen]);
  }while(TAP_Hdd_Exist(NextFileName) || TAP_Hdd_Exist(CutFileName));
}

bool FileCut(char *SourceFileName, char *CutFileName, dword StartBlock, dword NrBlocks)
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
    return FALSE;
  }

  return TRUE;
}

dword GetRecDateFromInf(char const *FileName, dword *const DateTime)
{
  TYPE_File            *f;
  char                  InfFileName[MAX_FILE_NAME_SIZE + 1];

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
    return FALSE;
  }
  return TRUE;
}

bool PatchInfFiles(char const *SourceFileName, char const *CutFileName, tTimeStamp *CutStartPoint, tTimeStamp *BehindCutPoint)
{
  char                  InfFileName[MAX_FILE_NAME_SIZE + 1];
  char                  CurrentDir[512];
  char                  T1[12], T2[10], T3[12];
  tRECHeaderInfo        RECHeaderInfo;
  TYPE_File            *f;
  dword                 fs;
  byte                 *Buffer;
  dword                 SourcePlayTime, CutPlayTime;
  dword                *Bookmarks = NULL;
  word                  i;
  bool                  CutBookmarkSet;
  bool                  result = TRUE;

  #ifdef FULLDEBUG
    WriteLogMC("MovieCutterLib", "PatchInfFiles()");
  #endif

  //Read the source .inf
  TAP_SPrint(InfFileName, "%s.inf", SourceFileName);
  f = TAP_Hdd_Fopen(InfFileName);
  if(!f)
  {
    WriteLogMC("MovieCutterLib", "PatchInfFiles() E0901: source inf not patched, cut inf not created.");
    return FALSE;
  }

  fs = TAP_Hdd_Flen(f);
  Buffer = (byte*) TAP_MemAlloc(INFSIZE);
  if(!Buffer)
  {
    TAP_Hdd_Fclose(f);
    WriteLogMC("MovieCutterLib", "PatchInfFiles() E0902: source inf not patched, cut inf not created.");
    return FALSE;
  }
  TAP_Hdd_Fread(Buffer, fs, 1, f);
  TAP_Hdd_Fclose(f);

  //Decode the source .inf
  if(!HDD_DecodeRECHeader(Buffer, &RECHeaderInfo, ST_UNKNOWN))
  {
    TAP_MemFree(Buffer);
    WriteLogMC("MovieCutterLib", "PatchInfFiles() E0903: source inf not patched, cut inf not created.");
    return FALSE;
  }

  //Copy the orig inf to nav inf (abuse the log string buffer)
  TAP_SPrint(LogString, "cp \"%s%s/%s.inf\" \"%s%s/%s.inf\"", TAPFSROOT, CurrentDir, SourceFileName, TAPFSROOT, CurrentDir, CutFileName);
  system(LogString);

  SourcePlayTime = 60 * RECHeaderInfo.HeaderDuration + RECHeaderInfo.HeaderDurationSec;
  CutPlayTime = (dword)(((BehindCutPoint->Timems - CutStartPoint->Timems) / 1000) + 0.5);

  SecToTimeString(SourcePlayTime, T1);
  SecToTimeString(CutPlayTime, T2);
  SecToTimeString(SourcePlayTime - CutPlayTime, T3);
  TAP_SPrint(LogString, "Playtimes: Orig = %s, Cut = %s, New = %s", T1, T2, T3);
  WriteLogMC("MovieCutterLib", LogString);

  SourcePlayTime -= CutPlayTime;

  //Change the new source play time
  RECHeaderInfo.HeaderDuration = (word)(SourcePlayTime / 60);
  RECHeaderInfo.HeaderDurationSec = SourcePlayTime % 60;

  //Save all bookmarks to a temporary array
  Bookmarks = (dword*) TAP_MemAlloc(177 * sizeof(dword));
  if(Bookmarks)
  {
    memcpy(Bookmarks, RECHeaderInfo.Bookmark, 177 * sizeof(dword));
    TAP_SPrint(LogString, "Bookmarks: ");
    for(i = 0; i < 177; i++)
    {
      if(Bookmarks[i] == 0) break;
      TAP_SPrint(&LogString[strlen(LogString)], "%u ", Bookmarks[i]);
    }
    WriteLogMC("MovieCutterLib", LogString);
  }

  //Clear all source Bookmarks
  memset(RECHeaderInfo.Bookmark, 0, 177 * sizeof(dword));
  RECHeaderInfo.NrBookmarks = 0;
  RECHeaderInfo.Resume = 0;

  //Copy all bookmarks which are < CutPointA or >= CutPointB
  //Move the second group by (CutPointB - CutPointA)
  CutBookmarkSet = FALSE;
  if(Bookmarks)
  {
    TAP_SPrint(LogString, "Bookmarks->Source: ");
    for(i = 0; i < 177; i++)
    {
      if((Bookmarks[i] >= 100 && Bookmarks[i] < CutStartPoint->BlockNr - 100) || (Bookmarks[i] >= BehindCutPoint->BlockNr + 100))
      {
        if(Bookmarks[i] < BehindCutPoint->BlockNr)
          RECHeaderInfo.Bookmark[RECHeaderInfo.NrBookmarks] = Bookmarks[i];
        else
        {
          if (!CutBookmarkSet)
          {
            RECHeaderInfo.Bookmark[RECHeaderInfo.NrBookmarks] = CutStartPoint->BlockNr;
            TAP_SPrint(&LogString[strlen(LogString)], "*%u ", CutStartPoint->BlockNr);
            RECHeaderInfo.NrBookmarks++;
            CutBookmarkSet = TRUE;
          }
          RECHeaderInfo.Bookmark[RECHeaderInfo.NrBookmarks] = Bookmarks[i] - (BehindCutPoint->BlockNr - CutStartPoint->BlockNr);
        }
        TAP_SPrint(&LogString[strlen(LogString)], "%u ", Bookmarks[i]);
        RECHeaderInfo.NrBookmarks++;
      }
      if(Bookmarks[i+1] == 0) break;
    }
    WriteLogMC("MovieCutterLib", LogString);
  }
  // Setzt automatisch ein Bookmark an die Schnittstelle
  if (!CutBookmarkSet)
  {
    RECHeaderInfo.Bookmark[RECHeaderInfo.NrBookmarks] = CutStartPoint->BlockNr;
    RECHeaderInfo.NrBookmarks++;
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
      WriteLogMC("MovieCutterLib", "PatchInfFiles() W0902: source inf not patched.");
      result = FALSE;
    }
  }
  else
  {
    WriteLogMC("MovieCutterLib", "PatchInfFiles() W0901: source inf not patched.");
    result = FALSE;
  }

  //Open the cut inf
  TAP_SPrint(InfFileName, "%s.inf", CutFileName);
  if(!TAP_Hdd_Exist(InfFileName)) TAP_Hdd_Create(InfFileName, ATTR_NORMAL);
  f = TAP_Hdd_Fopen(InfFileName);
  if(!f)
  {
    WriteLogMC("MovieCutterLib", "PatchInfFiles() E0904: cut inf not patched.");
    TAP_MemFree(Buffer);
    if(Bookmarks) TAP_MemFree(Bookmarks);
    return FALSE;
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
      if((Bookmarks[i] >= CutStartPoint->BlockNr + 100) && (Bookmarks[i] < BehindCutPoint->BlockNr - 100))
      {
        RECHeaderInfo.Bookmark[RECHeaderInfo.NrBookmarks] = Bookmarks[i] - CutStartPoint->BlockNr;
        TAP_SPrint(&LogString[strlen(LogString)], "%u ", Bookmarks[i]);
        RECHeaderInfo.NrBookmarks++;
      }
      if(Bookmarks[i+1] == 0) break;
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
    WriteLogMC("MovieCutterLib", "PatchInfFiles() E0905: cut inf not created.");
    result = FALSE;
  }

  TAP_Hdd_Fclose(f);
  TAP_MemFree(Buffer);
  if(Bookmarks) TAP_MemFree(Bookmarks);

  return result;
}

bool PatchNavFiles(char const *SourceFileName, char const *CutFileName, off_t CutStartPos, off_t BehindCutPos, bool isHD, dword *const OutCutStartTime, dword *const OutBehindCutTime)
{
  #ifdef FULLDEBUG
    WriteLogMC("MovieCutterLib", "PatchNavFiles()");
  #endif

  if(isHD)
    return PatchNavFilesHD(SourceFileName, CutFileName, CutStartPos, BehindCutPos, OutCutStartTime, OutBehindCutTime);
  else
    return PatchNavFilesSD(SourceFileName, CutFileName, CutStartPos, BehindCutPos, OutCutStartTime, OutBehindCutTime);
}


bool PatchNavFilesSD(char const *SourceFileName, char const *CutFileName, off_t CutStartPos, off_t BehindCutPos, dword *const OutCutStartTime, dword *const OutBehindCutTime)
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
  dword                 FirstCutTime, FirstSourceTime; // LastSourceTime;

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
    return FALSE;
  }

  navRecsSourceNew = 0;
  navRecsCut = 0;
//  IFrameCut = FALSE;
//  IFrameSource = FALSE;
  FirstCutTime = 0;
  FirstSourceTime = 0;
//  LastSourceTime = 0;
  while(TRUE)
  {
    navsRead = fread(navSource, sizeof(tnavSD), NAVRECS_SD, fSourceNav);
    if(navsRead == 0) break;

    for(i = 0; i < navsRead; i++)
    {
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

        if (FirstCutTime == 0) FirstCutTime = navSource[i].Timems;

        //Subtract CutStartPos from the cut .nav PH address
        PictureHeaderOffset = PictureHeaderOffset - CutStartPos;
//        if((navSource[i].SHOffset >> 24) == 1) IFrameCut = TRUE;
//        if(IFrameCut)
//        {
          memcpy(&navCut[navRecsCut], &navSource[i], sizeof(tnavSD));
          navCut[navRecsCut].PHOffsetHigh = PictureHeaderOffset >> 32;
          navCut[navRecsCut].PHOffset = PictureHeaderOffset & 0xffffffff;
          navCut[navRecsCut].Timems = navCut[navRecsCut].Timems - FirstCutTime;
          navRecsCut++;
//        }
//        IFrameSource = FALSE;
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
          if (FirstSourceTime == 0) FirstSourceTime = navSource[i].Timems; 
//        LastSourceTime = navSource[i].Timems;
        
//        if((navSource[i].SHOffset >> 24) == 1) IFrameSource = TRUE;
//        if(IFrameSource)
//        {
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
//        }
//        IFrameCut = FALSE;
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
  *OutBehindCutTime = FirstSourceTime;

  //Delete the orig source nav and make the new source nav the active one
//  TAP_SPrint(FileName, "%s.nav.bak", SourceFileName);
//  TAP_Hdd_Delete(FileName);
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
  dword                 FirstCutTime, FirstSourceTime; // LastSourceTime;

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
    return FALSE;
  }

  navRecsSourceNew = 0;
  navRecsCut = 0;
//  IFrameCut = FALSE;
//  IFrameSource = FALSE;
  FirstCutTime = 0;
  FirstSourceTime = 0;
//  LastSourceTime = 0;
  while(TRUE)
  {
    navsRead = fread(navSource, sizeof(tnavHD), NAVRECS_HD, fSourceNav);
    if(navsRead == 0) break;

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

        if (FirstCutTime == 0) FirstCutTime = navSource[i].Timems;

        //Subtract CutStartPos from the cut .nav PH address
        PictureHeaderOffset = PictureHeaderOffset - CutStartPos;
//        if((navSource[i].LastPPS >> 24) == 1) IFrameCut = TRUE;
//        if(IFrameCut)
//        {
          memcpy(&navCut[navRecsCut], &navSource[i], sizeof(tnavHD));
          navCut[navRecsCut].SEIOffsetHigh = PictureHeaderOffset >> 32;
          navCut[navRecsCut].SEIOffsetLow = PictureHeaderOffset & 0xffffffff;
          navCut[navRecsCut].Timems = navCut[navRecsCut].Timems - FirstCutTime;
          navRecsCut++;
//        }
//        IFrameSource = FALSE;
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

//        if((navSource[i].LastPPS >> 24) == 1) IFrameSource = TRUE;
//        if(IFrameSource)
//        {
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
//        }
//        IFrameCut = FALSE;
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
  *OutBehindCutTime = FirstSourceTime;

  //Delete the orig source nav and make the new source nav the active one
//  TAP_SPrint(FileName, "%s.nav.bak", SourceFileName);
//  TAP_Hdd_Delete(FileName);

  return TRUE;
}

long64 HDD_GetFileSize(char const *FileName)
{
  TYPE_File            *f;
  long64                FileSize;

  f = TAP_Hdd_Fopen(FileName);
  if(!f)
  {
    WriteLogMC("MovieCutterLib", "HDD_GetFileSize(): E0d01");
    return -1;
  }
  FileSize = f->size;
  TAP_Hdd_Fclose(f);
  return FileSize;
}

bool HDD_SetFileDateTime(char const *Directory, char const *FileName, dword NewDateTime)
{
  char                  AbsFileName[256];
  tstat64               statbuf;
  int                   status;
  struct utimbuf        utimebuf;

  TAP_SPrint(AbsFileName, "%s%s/%s", TAPFSROOT, Directory, FileName);
  if((status = lstat64(AbsFileName, &statbuf)))
  {
    WriteLogMC("MovieCutterLib", "HDD_SetFileDateTime() E0a01.");
    return FALSE;
  }

  if(NewDateTime > 0xd0790000)
  {
    utimebuf.actime = statbuf.st_atime;
    utimebuf.modtime = TF2UnixTime(NewDateTime);
    utime(AbsFileName, &utimebuf);
    return TRUE;
  }
  WriteLogMC("MovieCutterLib", "HDD_SetFileDateTime() E0a02.");
  return FALSE;
}


//.nav functions
tTimeStamp* NavLoad(char const *SourceFileName, dword *NrTimeStamps, bool isHD)
{
  tTimeStamp* ret;

  #if STACKTRACE == TRUE
    CallTraceEnter("NavLoad");
  #endif

  if(isHD)
    ret = NavLoadHD(SourceFileName, NrTimeStamps);
  else
    ret = NavLoadSD(SourceFileName, NrTimeStamps);

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
  return ret;
}

tTimeStamp* NavLoadSD(char const *SourceFileName, dword *const NrTimeStamps)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("NavLoadSD");
  #endif

  char                  CurrentDir[512];
  char                  AbsFileName[512];
  FILE                 *fNav = NULL;
  tnavSD               *navBuffer = NULL;
  tTimeStamp           *TimeStampBuffer = NULL;
  tTimeStamp           *TimeStamps = NULL;
  dword                 ret, i;
  dword                 FirstTime;
  dword                 LastTimeStamp;
  ulong64               AbsPos;
  dword                 NavSize;

  *NrTimeStamps = 0;
  
  // Open the nav file
  HDD_TAP_GetCurrentDir(CurrentDir);
  TAP_SPrint(AbsFileName, "%s%s/%s.nav", TAPFSROOT, CurrentDir, SourceFileName);
  fNav = fopen(AbsFileName, "rb");
  if(!fNav)
  {
    WriteLogMC("MovieCutterLib", "NavLoadSD() E0b01");
    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return(NULL);
  }

  // Reserve a (temporary) buffer to hold the entire file
  fseek(fNav, 0, SEEK_END);
  NavSize = ftell(fNav);
  rewind(fNav);
  TimeStampBuffer = (tTimeStamp*) TAP_MemAlloc(sizeof(tTimeStamp) * (NavSize / sizeof(tnavSD)));
  if (!TimeStampBuffer)
  {
    fclose(fNav);
    WriteLogMC("MovieCutterLib", "NavLoadSD() E0b02");
    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return(NULL);
  }

#ifdef FULLDEBUG
  TAP_PrintNet("NavSize: %u\t\tBufSize: %u\n", NavSize, sizeof(tTimeStamp) * (NavSize / sizeof(tnavSD)));
  TAP_PrintNet("Expected Nav-Records: %u\n", NavSize / sizeof(tnavSD));
#endif

  //Count and save all the _different_ time stamps in the .nav
  LastTimeStamp = 0;
  FirstTime = 0;
  navBuffer = (tnavSD*) TAP_MemAlloc(NAVRECS_SD * sizeof(tnavSD));
  if (!navBuffer)
  {
    fclose(fNav);
    TAP_MemFree(TimeStampBuffer);
    WriteLogMC("MovieCutterLib", "NavLoadSD() E0b03");
    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return(NULL);
  }
  do
  {
    ret = fread(navBuffer, sizeof(tnavSD), NAVRECS_SD, fNav);
#ifdef FULLDEBUG
  TAP_PrintNet("Returned Nav-Records: %u\n", ret);
#endif

    for(i = 0; i < ret; i++)
    {
      if(FirstTime == 0) FirstTime = navBuffer[i].Timems;
      if(LastTimeStamp != navBuffer[i].Timems)
      {
        AbsPos = ((ulong64)(navBuffer[i].PHOffsetHigh) << 32) | navBuffer[i].PHOffset;
        TimeStampBuffer[*NrTimeStamps].BlockNr = (dword)(AbsPos >> 6) / 141;
        TimeStampBuffer[*NrTimeStamps].Timems = navBuffer[i].Timems;

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
    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return(NULL);
  }

  // Copy the time stamps to the new array
  memcpy(TimeStamps, TimeStampBuffer, *NrTimeStamps * sizeof(tTimeStamp));  
  TAP_MemFree(TimeStampBuffer);

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
  return(TimeStamps);
}

tTimeStamp* NavLoadHD(char const *SourceFileName, dword *const NrTimeStamps)
{
  #if STACKTRACE == TRUE
    CallTraceEnter("NavLoadHD");
  #endif

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

  *NrTimeStamps = 0;

  // Open the nav file
  HDD_TAP_GetCurrentDir(CurrentDir);
  TAP_SPrint(AbsFileName, "%s%s/%s.nav", TAPFSROOT, CurrentDir, SourceFileName);
  fNav = fopen(AbsFileName, "rb");
  if(!fNav)
  {
    WriteLogMC("MovieCutterLib", "NavLoadHD() E0c01");
    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
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
    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return(NULL);
  }

#ifdef FULLDEBUG
  TAP_PrintNet("NavSize: %u\t\tBufSize: %u\n", NavSize, sizeof(tTimeStamp) * (NavSize / sizeof(tnavHD)));
  TAP_PrintNet("Expected Nav-Records: %u\n", NavSize / sizeof(tnavHD));
#endif

  //Count and save all the _different_ time stamps in the .nav
  LastTimeStamp = 0;
  FirstTime = 0;
  navBuffer = (tnavHD*) TAP_MemAlloc(NAVRECS_HD * sizeof(tnavHD));
  if (!navBuffer)
  {
    fclose(fNav);
    TAP_MemFree(TimeStampBuffer);
    WriteLogMC("MovieCutterLib", "NavLoadHD() E0c03");
    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return(NULL);
  }
  do
  {
    ret = fread(navBuffer, sizeof(tnavHD), NAVRECS_HD, fNav);
#ifdef FULLDEBUG
  TAP_PrintNet("Returned Nav-Records: %u\n", ret);
#endif

    for(i = 0; i < ret; i++)
    {
      if(FirstTime == 0) FirstTime = navBuffer[i].Timems;
      if(LastTimeStamp != navBuffer[i].Timems)
      {
        AbsPos = ((ulong64)(navBuffer[i].SEIOffsetHigh) << 32) | navBuffer[i].SEIOffsetLow;
        TimeStampBuffer[*NrTimeStamps].BlockNr = (dword)(AbsPos >> 6) / 141;
        TimeStampBuffer[*NrTimeStamps].Timems = navBuffer[i].Timems;

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
    #if STACKTRACE == TRUE
      CallTraceExit(NULL);
    #endif
    return(NULL);
  }

  // Copy the time stamps to the new array
  memcpy(TimeStamps, TimeStampBuffer, *NrTimeStamps * sizeof(tTimeStamp));  
  TAP_MemFree(TimeStampBuffer);

  #if STACKTRACE == TRUE
    CallTraceExit(NULL);
  #endif
  return(TimeStamps);
}
