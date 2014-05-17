#define _FILE_OFFSET_BITS  64
#define __USE_LARGEFILE64  1
#ifdef _MSC_VER
  #define __const const
#endif

#include                <stdio.h>
#include                <stdlib.h>
#include                <string.h>
#include                <unistd.h>
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

void HDD_GetDeviceNode(const char *Path, char *const OutDeviceNode)  // max. 20 Zeichen (inkl. Nullchar) in OutDeviceNode
{
  static char           LastMountPoint[FBLIB_DIR_SIZE], LastDeviceNode[20];
  char                  MountPoint[FBLIB_DIR_SIZE], CommandLine[512];
  char                 *p = NULL, *p2 = NULL;
  FILE                 *fMntStream;
  int                   i;

  TRACEENTER();

  // Falls Pfad mit '/..' beginnt, R�ckschritt entfernen und durch '/mnt' ersetzen
  if (strncmp(Path, "/../", 4) == 0)
  {
    TAP_SPrint(MountPoint, sizeof(MountPoint), "/mnt%s/", &Path[3]);
    // w�hle die ersten 2 Pfadebenen (/mnt/sdb2)
    i = 2;
  }
  else
  {
    // Falls Pfad nicht absolut ist, /mnt/hd davorsetzen und Slash anh�ngen
    if (strncmp(Path, "/mnt/", 5) == 0)
      TAP_SPrint(MountPoint, sizeof(MountPoint), "%s/", Path);
    else
      TAP_SPrint(MountPoint, sizeof(MountPoint), "%s%s/", TAPFSROOT, Path);
    // w�hle die ersten 4 Pfadebenen (/mnt/hd/DataFiles/WD)
    i = 4;
  }

  // Mount-Point aus dem Pfad extrahieren
  p = MountPoint;
  p2 = NULL;
  while ((p) && (i > 0))
  {
    p = strchr((p+1), '/');
    if (i == 3) p2 = p;  // (nur) beim zweiten Durchlauf p2 festlegen
    i--;
  }
  if(p)
    MountPoint[p - MountPoint] = '\0';
  else if(p2)
    MountPoint[p2 - MountPoint] = '\0';
  TAP_PrintNet("MountPoint: '%s'", MountPoint);

  // Abk�rzung
  if (strcmp(MountPoint, LastMountPoint) == 0)
    TAP_SPrint(OutDeviceNode, 20, LastDeviceNode);
  else
  {
    // R�ckgabewert initialisieren, falls es fehlschl�gt
    TAP_SPrint(OutDeviceNode, 20, "/dev/sda2");

    // Mount-Point in der Mount-Tabelle suchen
    TAP_SPrint(CommandLine, sizeof(CommandLine), "mount | egrep \"%s\"", MountPoint);  // > /tmp/fsck.dev
    system(CommandLine);

    // Device-Node aus der Mount-Tabelle auslesen
    fMntStream = popen(CommandLine, "r");
    if(fMntStream)
    {
      fgets(OutDeviceNode, 20, fMntStream);
      pclose(fMntStream);

      p = strchr(OutDeviceNode, ' ');
      if (p) *p = '\0';
    }
    TAP_SPrint(LastMountPoint, sizeof(LastMountPoint), "%s", MountPoint);
    TAP_SPrint(LastDeviceNode, sizeof(LastDeviceNode), OutDeviceNode);
  }
  TAP_PrintNet(" -> DeviceNode: '%s'\n", OutDeviceNode);

  TRACEEXIT();
}

// create, fopen, fread, fwrite
