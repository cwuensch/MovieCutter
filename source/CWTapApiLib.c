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
#include                <stdarg.h>
#include                <unistd.h>
#include                <utime.h>
#include                <mntent.h>
#include                <fcntl.h>
#include                <sys/stat.h>
#include                <sys/statvfs.h>
#include                <tap.h>
#include                "libFireBird.h"
#include                "RecHeader.h"
#include                "CWTapApiLib.h"


// ============================================================================
//                               TAP-API-Lib
// ============================================================================
void GetFileNameFromRec(const char *RecFileName, const char *AbsDirectory, const char *NewExt, char *const OutCutFileName)
{
  char *p = NULL;
  TRACEENTER();

  if (RecFileName && OutCutFileName)
  {
    TAP_SPrint(OutCutFileName, FBLIB_DIR_SIZE, "%s/%s", AbsDirectory, RecFileName);
    if ((p = strrchr(OutCutFileName, '.')) == NULL)
      p = &OutCutFileName[strlen(OutCutFileName)];
    strcpy(p, NewExt);
  }
  TRACEEXIT();
}

void HDD_Rename2(const char *FileName, const char *NewFileName, const char *AbsDirectory, bool RenameInfNav, bool RenameCutSRT)
{
  char AbsFileName[FBLIB_DIR_SIZE], AbsNewFileName[FBLIB_DIR_SIZE];
  TRACEENTER();

  TAP_SPrint  (AbsFileName, sizeof(AbsFileName), "%s/%s",     AbsDirectory, FileName);  TAP_SPrint(AbsNewFileName, sizeof(AbsNewFileName), "%s/%s",     AbsDirectory, NewFileName);  rename(AbsFileName, AbsNewFileName);
  if(RenameInfNav)
  {
    TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s/%s.inf", AbsDirectory, FileName);  TAP_SPrint(AbsNewFileName, sizeof(AbsNewFileName), "%s/%s.inf", AbsDirectory, NewFileName);  rename(AbsFileName, AbsNewFileName);
    TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s/%s.nav", AbsDirectory, FileName);  TAP_SPrint(AbsNewFileName, sizeof(AbsNewFileName), "%s/%s.nav", AbsDirectory, NewFileName);  rename(AbsFileName, AbsNewFileName);
  }
  if (RenameCutSRT)
  {
    GetFileNameFromRec(FileName, AbsDirectory, ".cut", AbsFileName);                    GetFileNameFromRec(NewFileName, AbsDirectory, ".cut", AbsNewFileName);                       rename(AbsFileName, AbsNewFileName);
    GetFileNameFromRec(FileName, AbsDirectory, ".srt", AbsFileName);                    GetFileNameFromRec(NewFileName, AbsDirectory, ".srt", AbsNewFileName);                       rename(AbsFileName, AbsNewFileName);
  }
  TRACEEXIT();
}

void HDD_Delete2(const char *FileName, const char *AbsDirectory, bool DeleteInfNavCut, bool DeleteSRT)
{
  char AbsFileName[FBLIB_DIR_SIZE];
  TRACEENTER();

  TAP_SPrint  (AbsFileName, sizeof(AbsFileName), "%s/%s",     AbsDirectory, FileName);  remove(AbsFileName);
  if(DeleteInfNavCut)
  {
    TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s/%s.inf", AbsDirectory, FileName);  remove(AbsFileName);
    TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s/%s.nav", AbsDirectory, FileName);  remove(AbsFileName);
    GetFileNameFromRec(FileName, AbsDirectory, ".cut", AbsFileName);                    remove(AbsFileName);
  }
  if (DeleteSRT)
  {
    GetFileNameFromRec(FileName, AbsDirectory, ".srt", AbsFileName);                    remove(AbsFileName);
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


bool HDD_GetFileDateTime(char const *FileName, char const *AbsDirectory, tPVRTime *const OutDateTime, byte *const OutDateSec)
{
  char                  AbsFileName[FBLIB_DIR_SIZE];
  struct stat64         statbuf;

  TRACEENTER();
  if(FileName && AbsDirectory && OutDateTime)
  {
    TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s/%s", AbsDirectory, FileName);
    if(lstat64(AbsFileName, &statbuf) == 0)
    {
      *OutDateTime = Unix2TFTimeSec(statbuf.st_mtime, OutDateSec);
      TRACEEXIT();
      return TRUE;
    }
  }
  TRACEEXIT();
  return FALSE;
}
bool HDD_SetFileDateTime(char const *FileName, char const *AbsDirectory, tPVRTime NewDateTime, byte NewDateSec)
{
  char                  AbsFileName[FBLIB_DIR_SIZE];
  struct stat64         statbuf;
  struct utimbuf        utimebuf;

  if (NewDateTime == 0)
  {
    NewDateTime = TFNow(&NewDateSec);
  }

  if(FileName && AbsDirectory && (NewDateTime > 0xd0790000))
  {
    TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s/%s", AbsDirectory, FileName);
    if(lstat64(AbsFileName, &statbuf) == 0)
    {
      utimebuf.actime = statbuf.st_atime;
      utimebuf.modtime = TF2UnixTimeSec(NewDateTime, NewDateSec);
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
//                             Time-Funktionen
// ----------------------------------------------------------------------------

time_t TF2UnixTimeSec(tPVRTime TFTimeStamp, byte TFTimeSec)
{ 
  return (MJD(TFTimeStamp) - 0x9e8b) * 86400 + HOUR(TFTimeStamp) * 3600 + MINUTE(TFTimeStamp) * 60 + TFTimeSec;
}

tPVRTime Unix2TFTimeSec(time_t UnixTimeStamp, byte *const outSec)
{
  if (outSec)
    *outSec = UnixTimeStamp % 60;
  return (DATE ( (UnixTimeStamp / 86400) + 0x9e8b, (UnixTimeStamp / 3600) % 24, (UnixTimeStamp / 60) % 60 ));
}

tPVRTime TFNow(byte *const outSec)
{
  word      Day;
  byte      Hour, Min, Sec;

  TAP_GetTime(&Day, &Hour, &Min, (outSec) ? outSec : &Sec);
  return (DATE(Day, Hour, Min));
}

tPVRTime AddTimeSec(tPVRTime pvrTime, byte pvrTimeSec, byte *const outSec, int addSeconds)
{
  word  Day  = MJD(pvrTime);
  short Hour = HOUR(pvrTime);
  short Min  = MINUTE(pvrTime);  
  short Sec = pvrTimeSec;
  TRACEENTER();

  Sec += addSeconds % 60;
  if(Sec < 0)       { Min--; Sec += 60; }
  else if(Sec > 59) { Min++; Sec -= 60; }

  Min += (addSeconds / 60) % 60;
  if(Min < 0)       { Hour--; Min += 60; }
  else if(Min > 59) { Hour++; Min -= 60; }

  Hour += (addSeconds / 3600) % 24;
  if(Hour < 0)      { Day--; Hour += 24; }
  else if(Hour >23) { Day++; Hour -= 24; }

  Day += (addSeconds / 86400);

  if(outSec) *outSec = Sec;

  TRACEEXIT();
  return (DATE(Day, Hour, Min));
}

int TimeDiffSec(tPVRTime FromTime, byte FromTimeSec, tPVRTime ToTime, byte ToTimeSec)
{
  dword             From, To;
  TRACEENTER();

  From = (MJD(FromTime) - 0x9e8b) * 86400 + HOUR(FromTime) * 3600 + MINUTE(FromTime) * 60 + FromTimeSec;
  To   = (MJD(ToTime) - 0x9e8b) * 86400   + HOUR(ToTime) * 3600   + MINUTE(ToTime) * 60   + ToTimeSec;

  TRACEEXIT();
  return (int)(To - From);
}


// ----------------------------------------------------------------------------
//                              INF-Funktionen
// ----------------------------------------------------------------------------

bool GetRecInfosFromInf(const char *RecFileName, const char *AbsDirectory, bool *const isCrypted, bool *const isHDVideo, bool *const isStripped, tPVRTime *const DateTime, byte *const DateSec)
{
  char                  AbsInfName[FBLIB_DIR_SIZE];
  TYPE_RecHeader_Info   RecHeaderInfo;
  TYPE_Service_Info     ServiceInfo;
  int                   f = -1;
  bool                  ret = FALSE;

  TRACEENTER();

  TAP_SPrint(AbsInfName, sizeof(AbsInfName), "%s/%s.inf", AbsDirectory, RecFileName);
  f = open(AbsInfName, O_RDONLY);
  if(f >= 0)
  {
    ret = (read(f, &RecHeaderInfo, sizeof(RecHeaderInfo)) == sizeof(RecHeaderInfo));
    if (ret)
    {
      if (isCrypted)  *isCrypted  = ((RecHeaderInfo.CryptFlag & 1) != 0);
      if (isStripped) *isStripped = RecHeaderInfo.rs_HasBeenStripped;
      if (DateTime)   *DateTime   = RecHeaderInfo.StartTime;
      if (DateSec)    *DateSec    = RecHeaderInfo.StartTimeSec;

      if (isHDVideo)
      {
        ret = (ret && read(f, &ServiceInfo, sizeof(ServiceInfo)) == sizeof(ServiceInfo));
        if ((ServiceInfo.VideoStreamType==STREAM_VIDEO_MPEG4_PART2) || (ServiceInfo.VideoStreamType==STREAM_VIDEO_MPEG4_H264) || (ServiceInfo.VideoStreamType==STREAM_VIDEO_MPEG4_H263))
          *isHDVideo = TRUE;
        else if ((ServiceInfo.VideoStreamType==STREAM_VIDEO_MPEG1) || (ServiceInfo.VideoStreamType==STREAM_VIDEO_MPEG2))
          *isHDVideo = FALSE;
        else
        {
          WriteLogMCf("CWTapApiLib", "GetRecInfosFromInfFile(): Unknown video stream type 0x%hhx.\n", ServiceInfo.VideoStreamType);
          ret = FALSE;
        }
      }
    }
    close(f);
  }

  TRACEEXIT();
  return ret;
}

/*bool SaveRecDateToInf(const char* RecFileName, char const *AbsDirectory, tPVRTime NewTime, byte NewSec)
{
  FILE                 *fInf = NULL;
  char                  AbsInfName[FBLIB_DIR_SIZE];
  TYPE_RecHeader_Info   RecHeaderInfo;
  bool                  ret = FALSE;

  TRACEENTER();

  TAP_SPrint(AbsInfName, sizeof(AbsInfName), "%s/%s.inf", AbsDirectory, RecFileName);

  if ((fInf = fopen(AbsInfName, "r+b")))
  {
    fread(&RecHeaderInfo, 1, 12, fInf);
    rewind(fInf);
    if ((strncmp(RecHeaderInfo.Magic, "TFrc", 4) == 0) && (RecHeaderInfo.Version == 0x8000))
    {
      RecHeaderInfo.StartTime = NewTime;
      RecHeaderInfo.StartTimeSec = NewSec;
      ret = (fwrite(&RecHeaderInfo, 1, 12, fInf) == 12);
      ret = fclose(fInf) && ret;
    }
  }
  else
    WriteLogMC(PROGRAM_NAME, "SaveRecDateToInf: Failed to open the inf file!");

  TRACEEXIT();
  return ret;
}*/


// ----------------------------------------------------------------------------
//                        Playback-Operationen
// ----------------------------------------------------------------------------
bool HDD_StartPlayback2(char *FileName, char *AbsDirectory, bool MediaFileMode)
{
//  char                  InfFileName[MAX_FILE_NAME_SIZE];
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
//    TAP_SPrint(InfFileName, sizeof(InfFileName), "%s.inf", FileName);
//    if (HDD_Exist2(InfFileName, AbsDirectory))
    if (!MediaFileMode)
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
//  TYPE_PlayInfo         PlayInfo;
  TYPE_Bookmark_Info   *PlayInfoBookmarkStruct = NULL;
  byte                 *TempRecSlot = NULL;
  bool                  ret = FALSE;

  TRACEENTER();

/*  TAP_Hdd_GetPlayInfo(&PlayInfo);
  if (PlayInfo.playMode != PLAYMODE_Playing)
  {
    TRACEEXIT();
    return FALSE;
  }  */

  TempRecSlot = (byte*)FIS_vTempRecSlot();
  if(TempRecSlot)
    PlayInfoBookmarkStruct = (TYPE_Bookmark_Info*)HDD_GetPvrRecTsPlayInfoPointer(*TempRecSlot);

  if(Bookmarks && NrBookmarks && PlayInfoBookmarkStruct)
  {
    *NrBookmarks = min(PlayInfoBookmarkStruct->NrBookmarks, NRBOOKMARKS);
//    memset(Bookmarks, 0, NRBOOKMARKS * sizeof(dword));
    memcpy(Bookmarks, &PlayInfoBookmarkStruct->Bookmarks, NRBOOKMARKS * sizeof(dword));
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
//  TYPE_PlayInfo         PlayInfo;
  TYPE_Bookmark_Info   *PlayInfoBookmarkStruct = NULL;
  byte                 *TempRecSlot = NULL;
  bool                  ret = FALSE;

  TRACEENTER();

/*  TAP_Hdd_GetPlayInfo(&PlayInfo);
  if (PlayInfo.playMode != PLAYMODE_Playing)
  {
    TRACEEXIT();
    return FALSE;
  }  */

  TempRecSlot = (byte*)FIS_vTempRecSlot();
  if(TempRecSlot)
    PlayInfoBookmarkStruct = (TYPE_Bookmark_Info*)HDD_GetPvrRecTsPlayInfoPointer(*TempRecSlot);

  if(Bookmarks && PlayInfoBookmarkStruct)
  {
    NrBookmarks = min(NrBookmarks, NRBOOKMARKS);
    PlayInfoBookmarkStruct->NrBookmarks = NrBookmarks;
//    memset(&PlayInfoBookmarkStruct->Bookmarks, 0, NRBOOKMARKS * sizeof(dword));
    memcpy(&PlayInfoBookmarkStruct->Bookmarks, Bookmarks, (OverwriteAll ? NRBOOKMARKS : NrBookmarks) * sizeof(dword));
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

bool ConvertUTFStr(char *DestStr, char *SourceStr, int MaxLen, bool ToUnicode)
{
  char *TempStr = NULL;
  TRACEENTER();

  TempStr = (char*) TAP_MemAlloc(MaxLen * 2);
  if (TempStr)
  {
    memset(TempStr, 0, sizeof(TempStr));
    if (ToUnicode)
    {
      #ifdef __ALTEFBLIB__
        if (SourceStr[0] < 0x20) SourceStr++;
        StrToUTF8(SourceStr, TempStr);
      #else
        StrToUTF8(SourceStr, TempStr, 9);
      #endif
    }
    else
      StrToISO(SourceStr, TempStr);

    if (!ToUnicode)
    {
      if ((SourceStr[0] >= 0x20) && (strlen(TempStr) < strlen(SourceStr)))
      {
        DestStr[0] = 0x05;
        DestStr++;
        MaxLen--;
      }
      else if (SourceStr[0] >= 0x15)
      {
        DestStr[0] = 0x05;
        DestStr++;
        SourceStr++;
      }
    }

    TempStr[MaxLen-1] = 0;
    if (ToUnicode && ((TempStr[strlen(TempStr)-1] & 0xC0) == 0xC0))
      TempStr[strlen(TempStr)-1] = 0;
    strcpy(DestStr, TempStr);

    TAP_MemFree(TempStr);
    TRACEEXIT();
    return TRUE;
  }
  TRACEEXIT();
  return FALSE;
}


// ----------------------------------------------------------------------------
//                         LogFile-Funktionen
// ----------------------------------------------------------------------------
FILE *fLogMC = NULL;

void CloseLogMC(void)
{
  if (fLogMC)
  {
    fclose(fLogMC);

    //As the log would receive the Linux time stamp (01.01.2000), adjust to the PVR's time
    struct utimbuf      times;
    byte Sec;
    times.actime = TF2UnixTimeSec(TFNow(&Sec), Sec);
    times.modtime = times.actime;
    utime(TAPFSROOT LOGDIR "/" LOGFILENAME, &times);
  }
  fLogMC = NULL;
}

void WriteLogMC(char *ProgramName, char *Text)
{
  tPVRTime              Time;
  char                  TS[20];
  word                  Year = 0;
  byte                  Month = 0, Day = 0, WeekDay = 0, Sec;

  Time = TFNow(&Sec);
  if (Time)
    TAP_ExtractMjd (MJD(Time), &Year, &Month, &Day, &WeekDay);

  if (!fLogMC)
  {
    fLogMC = fopen(TAPFSROOT LOGDIR "/" LOGFILENAME, "ab");
    setvbuf(fLogMC, NULL, _IOLBF, 4096);  // zeilenweises Buffering
  }

  TAP_SPrint(TS, sizeof(TS), "%04d-%02d-%02d %02d:%02d:%02d", Year, Month, Day, HOUR(Time), MINUTE(Time), Sec);
  if (fLogMC)
  {
    fprintf(fLogMC, "%s %s\r\n", TS, Text);
//    close(fLogMC);
  }

//  if (Console)
  {
    TAP_PrintNet("%s %s: %s\n", TS, ((ProgramName && ProgramName[0]) ? ProgramName : ""), Text);
  }
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
  char InfFileName[MAX_FILE_NAME_SIZE];
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
  char                  InfFileName[MAX_FILE_NAME_SIZE];
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
