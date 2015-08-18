#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define __USE_LARGEFILE64  1
#define _FILE_OFFSET_BITS  64
#ifdef _MSC_VER
  #define __const const
#endif
//#define STACKTRACE      TRUE

#define _GNU_SOURCE
#include                <stdio.h>
#include                <stdlib.h>
#include                <string.h>
#include                <stdarg.h>
#include                <unistd.h>
#include                <utime.h>
#include                <mntent.h>
#include                <fcntl.h>
#include                <sys/stat.h>
#include                <sys/statvfs.h>
#include                <tap.h>
#include                "libFireBird.h"
#include                "CWTapApiLib.h"


// ============================================================================
//                               TAP-API-Lib
// ============================================================================
void HDD_Rename2(const char *FileName, const char *NewFileName, const char *AbsDirectory, bool RenameInfNav)
{
  char AbsFileName[FBLIB_DIR_SIZE], AbsNewFileName[FBLIB_DIR_SIZE];
  TRACEENTER();

  TAP_SPrint  (AbsFileName, sizeof(AbsFileName), "%s/%s",     AbsDirectory, FileName);  TAP_SPrint(AbsNewFileName, sizeof(AbsNewFileName), "%s/%s",     AbsDirectory, NewFileName);  rename(AbsFileName, AbsNewFileName);
  if(RenameInfNav)
  {
    TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s/%s.inf", AbsDirectory, FileName);  TAP_SPrint(AbsNewFileName, sizeof(AbsNewFileName), "%s/%s.inf", AbsDirectory, NewFileName);  rename(AbsFileName, AbsNewFileName);
    TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s/%s.nav", AbsDirectory, FileName);  TAP_SPrint(AbsNewFileName, sizeof(AbsNewFileName), "%s/%s.nav", AbsDirectory, NewFileName);  rename(AbsFileName, AbsNewFileName);
  }

  TRACEEXIT();
}

void HDD_Delete2(const char *FileName, const char *AbsDirectory, bool DeleteInfNav)
{
  char AbsFileName[FBLIB_DIR_SIZE];
  TRACEENTER();

  TAP_SPrint  (AbsFileName, sizeof(AbsFileName), "%s/%s",     AbsDirectory, FileName);  remove(AbsFileName);
  if(DeleteInfNav)
  {
    TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s/%s.inf", AbsDirectory, FileName);  remove(AbsFileName);
    TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s/%s.nav", AbsDirectory, FileName);  remove(AbsFileName);
  }

  TRACEEXIT();
}

bool HDD_Exist2(const char *FileName, const char *AbsDirectory)
{
  char                  AbsFileName[FBLIB_DIR_SIZE];
  bool                  ret;

  TRACEENTER();
  TAP_SPrint  (AbsFileName, sizeof(AbsFileName), "%s/%s",     AbsDirectory, FileName);
  ret = (access(AbsFileName, F_OK) == 0);
  TRACEEXIT();
  return ret;
}

bool HDD_TruncateFile(const char *FileName, const char *AbsDirectory, off_t NewFileSize)
{
  char                  AbsFileName[FBLIB_DIR_SIZE];
  bool                  ret;

  TRACEENTER();
  TAP_SPrint  (AbsFileName, sizeof(AbsFileName), "%s/%s",     AbsDirectory, FileName);
  ret = (truncate(AbsFileName, NewFileSize) == 0);
  TRACEEXIT();
  return ret;
}


bool HDD_GetAbsolutePathByTypeFile2(TYPE_File *File, char *OutAbsFileName)
{
  dword                *d;
  bool                  ret = FALSE;

  TRACEENTER();
  if(OutAbsFileName) OutAbsFileName[0] = '\0';

  if(File && OutAbsFileName)
  {
    //TYPE_File->handle points to a structure with 4 dwords. The third one points to the absolute path
    d = (dword*) File->handle;
    if(d && d[2])
    {
      TAP_SPrint(OutAbsFileName, FBLIB_DIR_SIZE, "%s", (char*)d[2]);
      ret = TRUE;
    }
  }

  TRACEEXIT();
  return ret;
}

bool HDD_GetFileSizeAndInode2(const char *FileName, const char *AbsDirectory, __ino64_t *OutCInode, __off64_t *OutFileSize)
{
  char                  AbsFileName[FBLIB_DIR_SIZE];
  struct stat64         statbuf;
  bool                  ret = FALSE;

  TRACEENTER();

  if(AbsDirectory && FileName)
  {
    TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s/%s", AbsDirectory, FileName);
    if (lstat64(AbsFileName, &statbuf) == 0)
    {
      if(OutCInode)   *OutCInode = statbuf.st_ino;
      if(OutFileSize) *OutFileSize = statbuf.st_size;
      ret = TRUE;
    }
  }
  TRACEEXIT();
  return ret;
}
/* bool HDD_GetFileSizeAndInode2(char *FileName, const char *AbsDirectory, __ino64_t *OutCInode, __off64_t *OutFileSize)
{
  bool                  ret = FALSE;

  TRACEENTER();

  if (strncmp(__FBLIB_RELEASEDATE__, "2014-03-20", 10) >= 0)
  {
    char                AbsFileName[FBLIB_DIR_SIZE];
    bool                (*__HDD_GetFileSizeAndInode)(char*, __ino64_t*, __off64_t*);
    __HDD_GetFileSizeAndInode = (bool(*)(char*, __ino64_t*, __off64_t*)) HDD_GetFileSizeAndInode;

    TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s/%s", AbsDirectory, FileName);
    ret = __HDD_GetFileSizeAndInode(AbsFileName, OutCInode, OutFileSize);
  }
  else
  {
    char                Directory[FBLIB_DIR_SIZE];
    bool                (*__HDD_GetFileSizeAndInode)(char*, char*, __ino64_t*, __off64_t*);
    __HDD_GetFileSizeAndInode = (bool(*)(char*, char*, __ino64_t*, __off64_t*)) HDD_GetFileSizeAndInode;

    // Extract the relative path from the absolute path (for old FBLib-version)
    if (strncmp(AbsDirectory, TAPFSROOT, strlen(TAPFSROOT)) == 0)
      TAP_SPrint(Directory, sizeof(Directory), &AbsDirectory[strlen(TAPFSROOT)]);
    else if (strncmp(AbsDirectory, "/mnt", 4) == 0)
      TAP_SPrint(Directory, sizeof(Directory), "/..%s", &AbsDirectory[4]);
    else
      TAP_SPrint(Directory, sizeof(Directory), "/../..%s", AbsDirectory);

    ret = __HDD_GetFileSizeAndInode(Directory, FileName, OutCInode, OutFileSize);
  }

  TRACEEXIT();
  return ret;
} */


/*bool HDD_GetFileDateTime(char const *FileName, char const *AbsDirectory, dword *OutDateTime)
{
  char                  AbsFileName[FBLIB_DIR_SIZE];
  struct stat64         statbuf;
  struct utimbuf        utimebuf;

  TRACEENTER();

  if(FileName && AbsDirectory && OutDateTime)
  {
    TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s/%s", AbsDirectory, FileName);
    if(lstat64(AbsFileName, &statbuf) == 0)
    {
      *OutDateTime = Unix2TFTime(statbuf.st_mtime);
      TRACEEXIT();
      return TRUE;
    }
  }
  TRACEEXIT();
  return FALSE;
} */
bool HDD_SetFileDateTime(char const *FileName, char const *AbsDirectory, dword NewDateTime)
{
  char                  AbsFileName[FBLIB_DIR_SIZE];
  struct stat64         statbuf;
  struct utimbuf        utimebuf;

  if(NewDateTime == 0)
  {
    byte Sec;
    NewDateTime = Now(&Sec);
    NewDateTime += Sec;
  }

  if(FileName && AbsDirectory && (NewDateTime > 0xd0790000))
  {
    TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s/%s", AbsDirectory, FileName);
    if(lstat64(AbsFileName, &statbuf) == 0)
    {
      utimebuf.actime = statbuf.st_atime;
      utimebuf.modtime = PvrTimeToLinux(NewDateTime);
      utime(AbsFileName, &utimebuf);
      TRACEEXIT();
      return TRUE;
    }
  }
  TRACEEXIT();
  return FALSE;
}

__off64_t HDD_GetFreeDiscSpace(char *AnyFileName, char *AbsDirectory)
{
  char                  AbsFileName[FBLIB_DIR_SIZE];
  struct statvfs64      statbuf;
  __off64_t             ret = 0;

  TRACEENTER();

  TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s/%s", AbsDirectory, AnyFileName);
  if(statvfs64(AbsFileName, &statbuf) == 0)
    ret = (statbuf.f_bavail * statbuf.f_bsize);
  #if STACKTRACE == TRUE
    TAP_PrintNet("File System stat ('%s'): Block size=%lu, Fragment size=%lu, Total blocks=%llu, Free blocks=%llu, Avail blocks=%llu\n", AbsFileName, statbuf.f_bsize, statbuf.f_frsize, statbuf.f_blocks, statbuf.f_bfree, statbuf.f_bavail);
  #endif

  TRACEEXIT();
  return ret;
}

bool HDD_TAP_CheckCollisionByID(dword MyTapID)
{
  dword                *pCurrentTAPIndex;
  dword                 myTAPIndex, i;
  bool                  ret = FALSE;

  pCurrentTAPIndex = (dword*)FIS_vCurTapTask();
  if(pCurrentTAPIndex)
  {
    //Get the ID of myself
    myTAPIndex = *pCurrentTAPIndex;
//    myTAPId = HDD_TAP_GetIDByIndex(myTAPIndex);

    //Check all other TAPs
    for(i = 0; i < TAP_MAX; i++)
      if((i != myTAPIndex) && (HDD_TAP_GetIDByIndex(i) == MyTapID))
      {
        ret = TRUE;
        break;
      }
  }
  return ret;
}


// ----------------------------------------------------------------------------
//                        Playback-Operationen
// ----------------------------------------------------------------------------
bool HDD_StartPlayback2(char *FileName, char *AbsDirectory)
{
  char                  InfFileName[MAX_FILE_NAME_SIZE + 1];
  tDirEntry             FolderStruct;
  bool                  ret = FALSE;

  TRACEENTER();
//  HDD_TAP_PushDir();

  //Initialize the directory structure
  memset(&FolderStruct, 0, sizeof(tDirEntry));
  FolderStruct.Magic = 0xbacaed31;

  //Save the current directory resources and change into our directory (current directory of the TAP)
  ApplHdd_SaveWorkFolder();
  if (!ApplHdd_SelectFolder(&FolderStruct, &AbsDirectory[1]))   //do not include the leading slash
  {
    ApplHdd_SetWorkFolder(&FolderStruct);

    //Start the playback
    TAP_SPrint(InfFileName, sizeof(InfFileName), "%s.inf", FileName);
    if (HDD_Exist2(InfFileName, AbsDirectory))
      ret = (Appl_StartPlayback(FileName, 0, TRUE, FALSE) == 0);
    else
      ret = (Appl_StartPlaybackMedia(FileName, 0, TRUE, FALSE) == 0);
  }
  ApplHdd_RestoreWorkFolder();
//  HDD_TAP_PopDir();

  TRACEEXIT();
  return ret;
}

// Sets the Playback Repeat Mode: 0=REPEAT_None, 1=REPEAT_Region, 2=REPEAT_Total.
// If RepeatMode != REPEAT_Region, the other parameters are without function.
// Returns the old value if success, N_RepeatMode if failure
TYPE_RepeatMode PlaybackRepeatMode(bool ChangeMode, TYPE_RepeatMode RepeatMode, dword RepeatStartBlock, dword RepeatEndBlock)
{
  static TYPE_RepeatMode  *_RepeatMode = NULL;
  static int              *_RepeatStart = NULL;
  static int              *_RepeatEnd = NULL;
  TYPE_RepeatMode          OldValue;

  if(_RepeatMode == NULL)
  {
    _RepeatMode = (TYPE_RepeatMode*)TryResolve("_playbackRepeatMode");
    if(_RepeatMode == NULL) return N_RepeatMode;
  }

  if(_RepeatStart == NULL)
  {
    _RepeatStart = (int*)TryResolve("_playbackRepeatRegionStart");
    if(_RepeatStart == NULL) return N_RepeatMode;
  }

  if(_RepeatEnd == NULL)
  {
    _RepeatEnd = (int*)TryResolve("_playbackRepeatRegionEnd");
    if(_RepeatEnd == NULL) return N_RepeatMode;
  }

  OldValue = *_RepeatMode;
  if (ChangeMode)
  {
    *_RepeatMode = RepeatMode;
    *_RepeatStart = (RepeatMode == REPEAT_Region) ? (int)RepeatStartBlock : -1;
    *_RepeatEnd = (RepeatMode == REPEAT_Region) ? (int)RepeatEndBlock : -1;
  }
  return OldValue;
}
bool PlaybackRepeatSet(bool EnableRepeatAll)
{
  return (PlaybackRepeatMode(TRUE, ((EnableRepeatAll) ? REPEAT_Total : REPEAT_None), 0, 0) != N_RepeatMode);
}
bool PlaybackRepeatGet()
{
  return (PlaybackRepeatMode(FALSE, 0, 0, 0) == REPEAT_Total);
}


bool ReadBookmarks(dword *const Bookmarks, int *const NrBookmarks)
{
  TYPE_PlayInfo         PlayInfo;
  dword                *PlayInfoBookmarkStruct = NULL;
  byte                 *TempRecSlot = NULL;
  bool                  ret = FALSE;

  TRACEENTER();

  TAP_Hdd_GetPlayInfo(&PlayInfo);
  if (PlayInfo.playMode != PLAYMODE_Playing)
  {
    TRACEEXIT();
    return FALSE;
  }

  TempRecSlot = (byte*)FIS_vTempRecSlot();
  if(TempRecSlot)
    PlayInfoBookmarkStruct = (dword*)HDD_GetPvrRecTsPlayInfoPointer(*TempRecSlot);

  if(Bookmarks && NrBookmarks && PlayInfoBookmarkStruct)
  {
    *NrBookmarks = PlayInfoBookmarkStruct[0];
//    memset(Bookmarks, 0, NRBOOKMARKS * sizeof(dword));
    memcpy(Bookmarks, &PlayInfoBookmarkStruct[1], NRBOOKMARKS * sizeof(dword));
    ret = TRUE;
  }
  else
  {
    if(NrBookmarks) *NrBookmarks = 0;
//    WriteLogMC(PROGRAM_NAME, "ReadBookmarks: Fatal error - inf cache entry point not found!");

/*    char s[128];
    TAP_SPrint(s, sizeof(s), "TempRecSlot=%p", TempRecSlot);
    if(TempRecSlot)
      TAP_SPrint(&s[strlen(s)], sizeof(s)-strlen(s), ", *TempRecSlot=%d, HDD_NumberOfRECSlots()=%lu", *TempRecSlot, HDD_NumberOfRECSlots());
    WriteLogMC("*DEBUG*", s);  */
    ret = FALSE;
  }

  TRACEEXIT();
  return ret;
}

//Experimentelle Methode, um Bookmarks direkt in der Firmware abzuspeichern.
bool SaveBookmarks(dword Bookmarks[], int NrBookmarks, bool OverwriteAll)
{
  TYPE_PlayInfo         PlayInfo;
  dword                *PlayInfoBookmarkStruct = NULL;
  byte                 *TempRecSlot = NULL;
  bool                  ret = FALSE;

  TRACEENTER();

  TAP_Hdd_GetPlayInfo(&PlayInfo);
  if (PlayInfo.playMode != PLAYMODE_Playing)
  {
    TRACEEXIT();
    return FALSE;
  }

  TempRecSlot = (byte*)FIS_vTempRecSlot();
  if(TempRecSlot)
    PlayInfoBookmarkStruct = (dword*)HDD_GetPvrRecTsPlayInfoPointer(*TempRecSlot);

  if(Bookmarks && PlayInfoBookmarkStruct)
  {
    PlayInfoBookmarkStruct[0] = NrBookmarks;
//    memset(&PlayInfoBookmarkStruct[1], 0, NRBOOKMARKS * sizeof(dword));
    memcpy(&PlayInfoBookmarkStruct[1], Bookmarks, (OverwriteAll ? NRBOOKMARKS : NrBookmarks) * sizeof(dword));
    ret = TRUE;
  }
  else
  {
//    WriteLogMC(PROGRAM_NAME, "SaveBookmarks: Fatal error - inf cache entry point not found!");
    ret = FALSE;
  }

  TRACEEXIT();
  return ret;
}


// ----------------------------------------------------------------------------
//                               Festplatte
// ----------------------------------------------------------------------------
bool HDD_FindMountPointDev2(const char *AbsPath, char *const OutMountPoint, char *const OutDeviceNode)  // OutMountPoint und OutDeviceNode: max. FBLIB_DIR_SIZE (inkl. Nullchar)
{
  struct mntent        *ent;
  FILE                 *aFile;
  char                  MountPoint[FBLIB_DIR_SIZE], DeviceNode[FBLIB_DIR_SIZE];
  char                 *x;

  TRACEENTER();

  MountPoint[0] = '\0';
  DeviceNode[0] = '\0';

  if(AbsPath && AbsPath[0])
  {
    aFile = setmntent("/proc/mounts", "r");
    if(aFile != NULL)
    {
      while((ent = getmntent(aFile)) != NULL)
      {
        x = ansicstr(ent->mnt_dir, strlen(ent->mnt_dir), 0, NULL, NULL);
        if(x)
        {
          if((strncmp(AbsPath, x, strlen(x)) == 0) && (strlen(x) > strlen(MountPoint)))
          {
            TAP_SPrint(MountPoint, sizeof(MountPoint), x);
            TAP_SPrint(DeviceNode, sizeof(DeviceNode), ent->mnt_fsname);
          }
          TAP_MemFree(x);
        }
        else if((strncmp(AbsPath, ent->mnt_dir, strlen(ent->mnt_dir)) == 0) && (strlen(ent->mnt_dir) > strlen(MountPoint)))
        {
          TAP_SPrint(MountPoint, sizeof(MountPoint), ent->mnt_dir);
          TAP_SPrint(DeviceNode, sizeof(DeviceNode), ent->mnt_fsname);
        }
      }
      endmntent(aFile);
    }
    if(MountPoint[0] && (MountPoint[strlen(MountPoint) - 1] == '/')) MountPoint[strlen(MountPoint) - 1] = '\0';
    if(DeviceNode[0] && (DeviceNode[strlen(DeviceNode) - 1] == '/')) DeviceNode[strlen(DeviceNode) - 1] = '\0';
  }

  if(OutMountPoint) strcpy(OutMountPoint, MountPoint);
  if(OutDeviceNode) strcpy(OutDeviceNode, DeviceNode);

  TRACEEXIT();
  return (MountPoint[0] != '\0');
}


// ----------------------------------------------------------------------------
//                          String-Funktionen
// ----------------------------------------------------------------------------
char* RemoveEndLineBreak (char *const Text)
{
  TRACEENTER();

  int p = strlen(Text) - 1;
  if ((p >= 0) && (Text[p] == '\n')) Text[p] = '\0';
  
  TRACEEXIT();
  return Text;
}

char SysTypeToStr(void)
{
  switch (GetSystemType())
  {
    case ST_TMSS:  return 'S';
    case ST_TMSC:  return 'C';
    case ST_TMST:  return 'T';
    default:       return '?';
  }
}


// ----------------------------------------------------------------------------
//                         LogFile-Funktionen
// ----------------------------------------------------------------------------
int fLogMC = -1;

void CloseLogMC()
{
  if (fLogMC >= 0)
  {
    fsync(fLogMC);
    close(fLogMC);

    //As the log would receive the Linux time stamp (01.01.2000), adjust to the PVR's time
    struct utimbuf      times;
    times.actime = PvrTimeToLinux(Now(NULL));
    times.modtime = times.actime;
    utime(TAPFSROOT LOGDIR "/" LOGFILENAME, &times);
  }
  fLogMC = -1;
}

void WriteLogMC(char *ProgramName, char *Text)
{
  char                  Buffer[512];
  char                 *TS = NULL;
  byte                  Sec;

  TS = TimeFormat(Now(&Sec), Sec, TIMESTAMP_YMDHMS);

  if (fLogMC < 0)
    fLogMC = open(TAPFSROOT LOGDIR "/" LOGFILENAME, O_WRONLY | O_APPEND | O_CREAT /*| O_SYNC*/, 0666);

  if (fLogMC >= 0)
  {
    TAP_SPrint(Buffer, sizeof(Buffer), "%s %s\r\n", TS, Text);
    write(fLogMC, Buffer, strlen(Buffer));
//    close(fLogMC);
  }

//  if (Console)
  {
    TAP_SPrint(Buffer, sizeof(Buffer), "%s %s: %s\n", TS, ((ProgramName && ProgramName[0]) ? ProgramName : ""), Text);
    TAP_PrintNet(Buffer);
  }

  fsync(fLogMC);
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


/*// ----------------------------------------------------------------------------
//                      infData-API (FireBirdLib)
// ----------------------------------------------------------------------------
#define INFDATASTART      0x7d000   //500kB
#define INFDATAMAXSIG     64
#define INFDATMAGIC       "TFr+"

typedef struct
{
  char                  Magic[4];
  dword                 NameTagLen;
  dword                 PayloadSize;
} tTFRPlusHdr;

FILE *infDatainfFile = NULL;
dword infDataFileSize = 0;


static bool infData_OpenFile(const char *RecFileName, const char *AbsDirectory)
{
  char                  AbsFileName[FBLIB_DIR_SIZE];

  TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s/%s.inf", AbsDirectory, RecFileName);
  infDatainfFile = fopen(AbsFileName, "r+b");
  if (infDatainfFile)
  {  
    fseek(infDatainfFile, 0, SEEK_END);
    infDataFileSize = ftell(infDatainfFile);
    rewind(infDatainfFile);
    return TRUE;
  }
  else
    return FALSE;
}

static bool infData_LocateSig(const char *NameTag, dword *PayloadSize)
{
  bool                  ret;
  tTFRPlusHdr           TFRPlusHdr;
  char                  NameTagHdr[INFDATAMAXSIG];
  dword                 CurrentPos;

  //Format
  //  char Magic[4]             //TFr+
  //  dword PayloadSize
  //  word NameTagLength        //includes the NULL character
  //  char NameTag[SigLength]
  //  byte Payload[PayloadSize]

  TRACEENTER();

  ret = FALSE;
  if(PayloadSize) *PayloadSize = 0;

  if(NameTag && NameTag[0] && infDatainfFile && (infDataFileSize > INFDATASTART))
  {
    fseek(infDatainfFile, INFDATASTART, SEEK_SET);

    while((CurrentPos = ftell(infDatainfFile)) < infDataFileSize)
    {
      fread(&TFRPlusHdr, sizeof(tTFRPlusHdr), 1, infDatainfFile);

      //Stop parsing if the magic is invalid
      if(memcmp(TFRPlusHdr.Magic, INFDATMAGIC, 4) != 0) break;

      fread(NameTagHdr, min(TFRPlusHdr.NameTagLen, INFDATAMAXSIG), 1, infDatainfFile);

      if(strncmp(NameTag, NameTagHdr, INFDATAMAXSIG) == 0)
      {
        ret = TRUE;
        if(PayloadSize) *PayloadSize = TFRPlusHdr.PayloadSize;
        fseek(infDatainfFile, CurrentPos, SEEK_SET);
        break;
      }
      fseek(infDatainfFile, TFRPlusHdr.PayloadSize, SEEK_CUR);
    }
  }

  TRACEEXIT();
  return ret;
}

bool infData_Get2(const char *RecFileName, const char *AbsDirectory, const char *NameTag, dword *const PayloadSize, byte **Payload)
{
  byte                 *DataBlock;
  bool                  ret;
  tTFRPlusHdr           TFRPlusHdr;
  char                  NameTagHdr[INFDATAMAXSIG];

  TRACEENTER();

  ret = FALSE;
  if(PayloadSize) *PayloadSize = 0;
  if(Payload) *Payload = NULL;

  if(NameTag && NameTag[0] && Payload && infData_OpenFile(RecFileName, AbsDirectory) && infData_LocateSig(NameTag, NULL))
  {
    ret = TRUE;
    DataBlock = NULL;

    fread(&TFRPlusHdr, sizeof(tTFRPlusHdr), 1, infDatainfFile);
    fread(NameTagHdr, min(TFRPlusHdr.NameTagLen, INFDATAMAXSIG), 1, infDatainfFile);
    if(PayloadSize) *PayloadSize = TFRPlusHdr.PayloadSize;

    if(TFRPlusHdr.PayloadSize > 0)
    {
      DataBlock = (byte*) TAP_MemAlloc(TFRPlusHdr.PayloadSize);
      if(DataBlock)
        fread(DataBlock, TFRPlusHdr.PayloadSize, 1, infDatainfFile);
//      else
//        WriteLogMCf(PROGRAM_NAME, "infData: failed to reserve %d bytes for %s.inf:%s", TFRPlusHdr.PayloadSize, RecFileName, NameTag);
    }
    *Payload = DataBlock;
    fclose(infDatainfFile);
  }
  infDatainfFile = NULL;

  TRACEEXIT();
  return ret;
}

bool infData_Set2(const char *RecFileName, const char *AbsDirectory, const char *NameTag, dword PayloadSize, byte Payload[])
{
  char InfFileName[MAX_FILE_NAME_SIZE + 1];
  bool                  ret;
  tTFRPlusHdr           TFRPlusHdr;

  TRACEENTER();

  ret = FALSE;

  infData_Delete2(RecFileName, AbsDirectory, NameTag);
  if(NameTag && NameTag[0] && infData_OpenFile(RecFileName, AbsDirectory))
  {
    ret = TRUE;

    //Ensure the minimum size of INFDATASTART bytes
    TAP_SPrint(InfFileName, sizeof(InfFileName), "%s.inf", RecFileName);
    if(infDataFileSize < INFDATASTART)
    {
      fclose(infDatainfFile);
      HDD_TruncateFile(InfFileName, AbsDirectory, INFDATASTART);
      infData_OpenFile(RecFileName, AbsDirectory);
    }

    //Add the data block
    fseek(infDatainfFile, 0, SEEK_END);

    memcpy(TFRPlusHdr.Magic, INFDATMAGIC, 4);
    TFRPlusHdr.NameTagLen = min(strlen(NameTag) + 1, INFDATAMAXSIG);
    TFRPlusHdr.PayloadSize = PayloadSize;

    if(PayloadSize && (Payload == NULL))
    {
//      WriteLogMCf(PROGRAM_NAME, "infData: PayloadSize of %s.inf:%s is not 0, but data pointer is NULL!", RecFileName, NameTag);
      TFRPlusHdr.PayloadSize = 0;
      ret = FALSE;
    }

    fwrite(&TFRPlusHdr, sizeof(tTFRPlusHdr), 1, infDatainfFile);
    fwrite(NameTag, TFRPlusHdr.NameTagLen, 1, infDatainfFile);

    if(Payload)
      fwrite(Payload, TFRPlusHdr.PayloadSize, 1, infDatainfFile);
    fclose(infDatainfFile);
    HDD_SetFileDateTime(InfFileName, AbsDirectory, 0);
  }
  infDatainfFile = NULL;

  TRACEEXIT();
  return ret;
}

bool infData_Delete2(const char *RecFileName, const char *AbsDirectory, const char *NameTag)
{
  char                  InfFileName[MAX_FILE_NAME_SIZE + 1];
  dword                 SourcePos, DestPos, Len;
  tTFRPlusHdr           TFRPlusHdr;
  char                  NameTagHdr[INFDATAMAXSIG];
  byte                  *Data;
  bool                  ret;

  TRACEENTER();

  ret = FALSE;

  if(NameTag && NameTag[0] && infData_OpenFile(RecFileName, AbsDirectory) && infData_LocateSig(NameTag, NULL))
  {
    //Now the file pointer is located at the beginning of the data block
    //which should be deleted.
    DestPos = ftell(infDatainfFile);
    fread(&TFRPlusHdr, sizeof(tTFRPlusHdr), 1, infDatainfFile);
    Len = sizeof(tTFRPlusHdr) + TFRPlusHdr.NameTagLen + TFRPlusHdr.PayloadSize;
    SourcePos = DestPos + Len;
    fseek(infDatainfFile, SourcePos, SEEK_SET);

    while(SourcePos < infDataFileSize)
    {
      //Stop if we're unable to read the whole header
      if(fread(&TFRPlusHdr, sizeof(tTFRPlusHdr), 1, infDatainfFile) == 0) break;

      //Stop parsing if the magic is invalid
      if(memcmp(TFRPlusHdr.Magic, INFDATMAGIC, 4) != 0) break;

      Len = sizeof(tTFRPlusHdr) + TFRPlusHdr.NameTagLen + TFRPlusHdr.PayloadSize;

      fread(NameTagHdr, min(TFRPlusHdr.NameTagLen, INFDATAMAXSIG), 1, infDatainfFile);

      if(TFRPlusHdr.PayloadSize)
      {
        Data = TAP_MemAlloc(TFRPlusHdr.PayloadSize);
        if(Data)
        {
          fread(Data, TFRPlusHdr.PayloadSize, 1, infDatainfFile);
        }
        else
        {
//          WriteLogMCf(PROGRAM_NAME, "infData: failed to reserve %d bytes for deletion of %s.inf:%s", TFRPlusHdr.PayloadSize, RecFileName, NameTagHdr);
          TFRPlusHdr.PayloadSize = 0;
        }
      }
      else
        Data = NULL;

      fseek(infDatainfFile, DestPos, SEEK_SET);
      fwrite(&TFRPlusHdr, sizeof(tTFRPlusHdr), 1, infDatainfFile);
      fwrite(NameTagHdr, min(TFRPlusHdr.NameTagLen, INFDATAMAXSIG), 1, infDatainfFile);

      if(Data)
      {
        fwrite(Data, TFRPlusHdr.PayloadSize, 1, infDatainfFile);
        TAP_MemFree(Data);
      }

      DestPos += Len;
      SourcePos += Len;

      fseek(infDatainfFile, SourcePos, SEEK_SET);
    }
    fclose(infDatainfFile);

    TAP_SPrint(InfFileName, sizeof(InfFileName), "%s.inf", RecFileName);
    HDD_TruncateFile(InfFileName, AbsDirectory, DestPos);
    HDD_SetFileDateTime(InfFileName, AbsDirectory, 0));
    ret = TRUE;
  }
  infDatainfFile = NULL;

  TRACEEXIT();
  return ret;
}  */

// create, fopen, fread, fwrite
