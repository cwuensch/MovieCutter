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
#include                <mntent.h>
#include                <sys/stat.h>
#include                <sys/statvfs.h>
#include                <tap.h>
#include                <libFireBird.h>
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
  ret= (access(AbsFileName, F_OK) == 0);
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
      strncpy(OutAbsFileName, (char*)d[2], FBLIB_DIR_SIZE - 1);
      OutAbsFileName[FBLIB_DIR_SIZE - 1] = '\0';
      ret = TRUE;
    }
  }

  TRACEEXIT();
  return ret;
}

bool HDD_GetFileSizeAndInode2(const char *FileName, const char *AbsDirectory, __ino64_t *OutCInode, __off64_t *OutFileSize)
{
  char                  AbsFileName[FBLIB_DIR_SIZE];
  tstat64               statbuf;
  bool                  ret = FALSE;

  TRACEENTER();

  if(AbsDirectory && FileName)
  {
    TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s/%s", AbsDirectory, FileName);
    if(!lstat64(AbsFileName, &statbuf))
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


bool HDD_SetFileDateTime(char const *FileName, char const *AbsDirectory, dword NewDateTime)
{
  char                  AbsFileName[FBLIB_DIR_SIZE];
  tstat64               statbuf;
  struct utimbuf        utimebuf;

  TRACEENTER();

  TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s/%s", AbsDirectory, FileName);
  if(lstat64(AbsFileName, &statbuf))
  {
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

  TRACEEXIT();
  return FALSE;
}

__off64_t HDD_GetFreeDiscSpace(char *AnyFileName, char *AbsDirectory)
{
  char                  AbsFileName[FBLIB_DIR_SIZE];
  tstatvfs64            statbuf;
  __off64_t             ret = 0;

  TRACEENTER();

  TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s/%s", AbsDirectory, AnyFileName);
  if(statvfs64(AbsFileName, &statbuf) == 0)
    ret = (statbuf.f_bavail * statbuf.f_bsize);
//TAP_PrintNet("File System stat ('%s'): Block size=%lu, Fragment size=%lu, Total blocks=%llu, Free blocks=%llu, Avail blocks=%llu\n", AbsFileName, statbuf.f_bsize, statbuf.f_frsize, statbuf.f_blocks, statbuf.f_bfree, statbuf.f_bavail);

  TRACEEXIT();
  return ret;
}


bool HDD_StartPlayback2(char *FileName, char *AbsDirectory)
{
  tDirEntry             FolderStruct;
  bool                  ret = FALSE;

  TRACEENTER();

  //Initialize the directory structure
  memset(&FolderStruct, 0, sizeof(tDirEntry));
  FolderStruct.Magic = 0xbacaed31;
//  HDD_TAP_PushDir();

  //Save the current directory resources and change into our directory (current directory of the TAP)
  ApplHdd_SaveWorkFolder();
  if (!ApplHdd_SelectFolder(&FolderStruct, &AbsDirectory[1]))   //do not include the leading slash
  {
    ApplHdd_SetWorkFolder(&FolderStruct);

    //Start the playback
    ret = (Appl_StartPlayback(FileName, 0, TRUE, FALSE) == 0);
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
  dword                *PlayInfoBookmarkStruct;
  byte                 *TempRecSlot;
  bool                  ret = FALSE;

  TRACEENTER();

  PlayInfoBookmarkStruct = NULL;
  TempRecSlot = (byte*)FIS_vTempRecSlot();
  if(TempRecSlot)
    PlayInfoBookmarkStruct = (dword*)HDD_GetPvrRecTsPlayInfoPointer(*TempRecSlot);

  if(Bookmarks && NrBookmarks && PlayInfoBookmarkStruct)
  {
    *NrBookmarks = PlayInfoBookmarkStruct[0];
    memset(Bookmarks, 0, NRBOOKMARKS * sizeof(dword));
    memcpy(Bookmarks, &PlayInfoBookmarkStruct[1], *NrBookmarks * sizeof(dword));
    ret = TRUE;
  }
  else
  {
    if(NrBookmarks) *NrBookmarks = 0;
//    WriteLogMC(PROGRAM_NAME, "ReadBookmarks: Fatal error - inf cache entry point not found!");

    char s[128];
    TAP_SPrint(s, sizeof(s), "TempRecSlot=%p", TempRecSlot);
    if(TempRecSlot)
      TAP_SPrint(&s[strlen(s)], sizeof(s)-strlen(s), ", *TempRecSlot=%d, HDD_NumberOfRECSlots()=%lu", *TempRecSlot, HDD_NumberOfRECSlots());
//    WriteLogMC(PROGRAM_NAME, s);
    ret = FALSE;
  }

  TRACEEXIT();
  return ret;
}

//Experimentelle Methode, um Bookmarks direkt in der Firmware abzuspeichern.
bool SaveBookmarks(dword Bookmarks[], int NrBookmarks)
{
  dword                *PlayInfoBookmarkStruct;
  byte                 *TempRecSlot;
  bool                  ret = FALSE;

  TRACEENTER();

  PlayInfoBookmarkStruct = NULL;
  TempRecSlot = (byte*)FIS_vTempRecSlot();
  if(TempRecSlot)
    PlayInfoBookmarkStruct = (dword*)HDD_GetPvrRecTsPlayInfoPointer(*TempRecSlot);

  if(Bookmarks && PlayInfoBookmarkStruct)
  {
    PlayInfoBookmarkStruct[0] = NrBookmarks;
    memset(&PlayInfoBookmarkStruct[1], 0, NRBOOKMARKS * sizeof(dword));
    memcpy(&PlayInfoBookmarkStruct[1], Bookmarks, NrBookmarks * sizeof(dword));
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


bool HDD_FindMountPointDev2(const char *AbsPath, char *const OutMountPoint, char *const OutDeviceNode)  // OutDeviceNode: max. 20 Zeichen, OutMountPoint: max. FILE_NAME_SIZE+1 (inkl. Nullchar)
{
  char                  MountPoint[MAX_FILE_NAME_SIZE+1], DeviceNode[20];
  FILE                 *aFile;
  struct mntent        *ent;

  TRACEENTER();

  MountPoint[0] = '\0';
  DeviceNode[0] = '\0';
  aFile = setmntent("/proc/mounts", "r");
  if(aFile != NULL)
  {
    while((ent = getmntent(aFile)) != NULL)
    {
      if(strncmp(AbsPath, ent->mnt_dir, strlen(ent->mnt_dir)) == 0)
      {
        if(strlen(ent->mnt_dir) > strlen(MountPoint))
        {
          strncpy(MountPoint, ent->mnt_dir, sizeof(MountPoint));
          MountPoint[sizeof(MountPoint) - 1] = '\0';
          strncpy(DeviceNode, ent->mnt_fsname, sizeof(DeviceNode));
          DeviceNode[sizeof(DeviceNode) - 1] = '\0';
        }
      }
    }
    endmntent(aFile);
  }

  if(MountPoint[0] && (MountPoint[strlen(MountPoint) - 1] == '/')) MountPoint[strlen(MountPoint) - 1] = '\0';
  if(DeviceNode[0] && (DeviceNode[strlen(DeviceNode) - 1] == '/')) DeviceNode[strlen(DeviceNode) - 1] = '\0';

  if(OutMountPoint) strcpy(OutMountPoint, MountPoint);
  if(OutDeviceNode) strcpy(OutDeviceNode, DeviceNode);

  TRACEEXIT();
  return (MountPoint[0] != '\0');
}


char* RemoveEndLineBreak (char *const Text)
{
  TRACEENTER();

  int p = strlen(Text) - 1;
  if ((p >= 0) && (Text[p] == '\n')) Text[p] = '\0';
  
  TRACEEXIT();
  return Text;
}

// create, fopen, fread, fwrite
