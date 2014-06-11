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
#include                <tap.h>
#include                <libFireBird.h>
#include                "CWTapApiLib.h"


// ============================================================================
//                               TAP-API-Lib
// ============================================================================
void HDD_Rename2(const char *FileName, const char *NewFileName, const char *Directory, bool RenameInfNav)
{
  char AbsFileName[FBLIB_DIR_SIZE], AbsNewFileName[FBLIB_DIR_SIZE];
  TRACEENTER();

  TAP_SPrint  (AbsFileName, sizeof(AbsFileName), "%s%s/%s",     TAPFSROOT, Directory, FileName);  TAP_SPrint(AbsNewFileName, sizeof(AbsNewFileName), "%s%s/%s",     TAPFSROOT, Directory, NewFileName);  rename(AbsFileName, AbsNewFileName);
  if(RenameInfNav)
  {
    TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s%s/%s.inf", TAPFSROOT, Directory, FileName);  TAP_SPrint(AbsNewFileName, sizeof(AbsNewFileName), "%s%s/%s.inf", TAPFSROOT, Directory, NewFileName);  rename(AbsFileName, AbsNewFileName);
    TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s%s/%s.nav", TAPFSROOT, Directory, FileName);  TAP_SPrint(AbsNewFileName, sizeof(AbsNewFileName), "%s%s/%s.nav", TAPFSROOT, Directory, NewFileName);  rename(AbsFileName, AbsNewFileName);
  }

  TRACEEXIT();
}

void HDD_Delete2(const char *FileName, const char *Directory, bool DeleteInfNav)
{
  char AbsFileName[FBLIB_DIR_SIZE];
  TRACEENTER();

  TAP_SPrint  (AbsFileName, sizeof(AbsFileName), "%s%s/%s",     TAPFSROOT, Directory, FileName);  remove(AbsFileName);
  if(DeleteInfNav)
  {
    TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s%s/%s.inf", TAPFSROOT, Directory, FileName);  remove(AbsFileName);
    TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s%s/%s.nav", TAPFSROOT, Directory, FileName);  remove(AbsFileName);
  }

  TRACEEXIT();
}

bool HDD_Exist2(char *FileName, const char *Directory)
{
  char AbsFileName[FBLIB_DIR_SIZE];
  bool ret;

  TRACEENTER();
  TAP_SPrint  (AbsFileName, sizeof(AbsFileName), "%s%s/%s",     TAPFSROOT, Directory, FileName);
  ret = (access(AbsFileName, F_OK) == 0);
  TRACEEXIT();
  return ret;
}


bool HDD_GetFileSizeAndInode2(const char *FileName, const char *Directory, __ino64_t *OutCInode, __off64_t *OutFileSize)
{
  char                  AbsFileName[FBLIB_DIR_SIZE];
  bool                  ret;

  TRACEENTER();

  if (strncmp(__FBLIB_RELEASEDATE__, "2014-03-20", 10) >= 0)
  {
    bool                (*__HDD_GetFileSizeAndInode)(char*, __ino64_t*, __off64_t*);
    __HDD_GetFileSizeAndInode = (bool(*)(char*, __ino64_t*, __off64_t*)) HDD_GetFileSizeAndInode;

    TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s%s/%s", TAPFSROOT, Directory, FileName);
    ret = __HDD_GetFileSizeAndInode(AbsFileName, OutCInode, OutFileSize);
  }
  else
  {
    bool                (*__HDD_GetFileSizeAndInode)(char*, char*, __ino64_t*, __off64_t*);
    __HDD_GetFileSizeAndInode = (bool(*)(char*, char*, __ino64_t*, __off64_t*)) HDD_GetFileSizeAndInode;
    ret = __HDD_GetFileSizeAndInode(Directory, FileName, OutCInode, OutFileSize);
  }

  TRACEEXIT();
  return ret;
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


bool HDD_StartPlayback2(char *FileName, const char *Directory)
{
  tDirEntry             FolderStruct;
  char                  AbsDirectory[FBLIB_DIR_SIZE];
  bool                  ret = FALSE;

  TRACEENTER();

  //Initialize the directory structure
  memset(&FolderStruct, 0, sizeof(tDirEntry));
  FolderStruct.Magic = 0xbacaed31;
//  HDD_TAP_PushDir();

  //Save the current directory resources and change into our directory (current directory of the TAP)
  ApplHdd_SaveWorkFolder();
  TAP_SPrint(AbsDirectory, sizeof(AbsDirectory), "%s%s", &TAPFSROOT[1], Directory);  //do not include the leading slash
  if (!ApplHdd_SelectFolder(&FolderStruct, AbsDirectory))
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


bool HDD_FindMountPointDev2(const char *Path, char *const OutMountPoint, char *const OutDeviceNode)  // OutDeviceNode: max. 20 Zeichen, OutMountPoint: max. FILE_NAME_SIZE+1 (inkl. Nullchar)
{
  char                  AbsPath[FBLIB_DIR_SIZE], MountPoint[MAX_FILE_NAME_SIZE+1], DeviceNode[20];
  FILE                 *aFile;
  struct mntent        *ent;

  TRACEENTER();

  // Falls Pfad mit '/..' beginnt, Rückschritt entfernen und durch '/mnt' ersetzen
  if (strncmp(Path, "/../", 4) == 0)
    TAP_SPrint(AbsPath, sizeof(AbsPath), "/mnt%s", &Path[3]);
  else
  {
    // Falls Pfad nicht absolut ist, /mnt/hd davorsetzen und Slash anhängen
    if (strncmp(Path, "/mnt/", 5) == 0)
      TAP_SPrint(AbsPath, sizeof(AbsPath), "%s", Path);
    else
      TAP_SPrint(AbsPath, sizeof(AbsPath), "%s%s", TAPFSROOT, Path);
  }

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
