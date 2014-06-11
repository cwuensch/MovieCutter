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
#include                "MovieCutterLib.h"
#include                "HddToolsLib.h"

bool                    fsck_Cancelled;
word                    RegionToSave;
char                    LogString[512];
char*                   LS_Dummy = "< Dummy >";
char                   *LS_Warning, *LS_MoreErrors, *LS_CheckingFileSystem;


void HDDCheck_ProgBarHandler(bool ShowProgBar, dword CurrentValue, dword pProgressStart, dword pProgressEnd, dword pProgressMax, dword pRegionToSave)
{
  // Initialisierung der statischen Variablen
  static dword ProgressStart = 0, ProgressEnd = 100, ProgressMax = 100;
  if ((pProgressStart != 0) || (pProgressEnd != 0) || (pProgressMax != 0) || (pRegionToSave != 0))
  {
    ProgressStart = pProgressStart;
    ProgressEnd   = pProgressEnd;
    ProgressMax   = pProgressMax;
    RegionToSave  = pRegionToSave;
  }
  else
  {
    if (!ShowProgBar)
      if (OSDMenuProgressBarIsVisible()) OSDMenuProgressBarDestroyNoOSDUpdate();
  }
  if (ShowProgBar)
    OSDMenuProgressBarShow("CheckFileSystem", LS_CheckingFileSystem, 100*ProgressStart + CurrentValue*(ProgressEnd-ProgressStart), 100*ProgressMax, NULL);
}
void  HDDCheck_InitProgBar(dword pProgressStart, dword pProgressEnd, dword pProgressMax, word pRegionToSave, char *pWarningStr, char *pMoreErrorsStr, char *pCheckingStr)
{
  OSDMenuSaveMyRegion(pRegionToSave);
  if (pWarningStr)    LS_Warning            = pWarningStr;      else LS_Warning            = LS_Dummy;
  if (pMoreErrorsStr) LS_MoreErrors         = pMoreErrorsStr;   else LS_MoreErrors         = LS_Dummy;
  if (pCheckingStr)   LS_CheckingFileSystem = pCheckingStr;     else LS_CheckingFileSystem = LS_Dummy;
  HDDCheck_ProgBarHandler(FALSE, 0, pProgressStart, pProgressEnd, pProgressMax, pRegionToSave);
}
void HDDCheck_RefreshProgBar(bool ShowProgBar, dword CurrentValue)
{
  HDDCheck_ProgBarHandler(ShowProgBar, CurrentValue, 0, 0, 0, 0);
}

void ShowInfoBox(char *MessageStr, char *TitleStr)
{
  OSDMenuSaveMyRegion(RegionToSave);
  OSDMenuInfoBoxShow(TitleStr, MessageStr, 0);
  TAP_SystemProc();
  TAP_Delay(200);
  OSDMenuInfoBoxDestroyNoOSDUpdate();
}

void HDD_CancelCheckFS()
{
  fsck_Cancelled = TRUE;
}

bool  HDD_CheckFileSystem(const char *MountPath, TProgBarHandler pRefreshProgBar, TMessageHandler pShowErrorMessage, bool DoFix, bool Quick, char *SuccessString, char *ErrorStrFmt, char *AbortedString)
{
  TProgBarHandler       RefreshProgBar = pRefreshProgBar;
  TMessageHandler       ShowErrorMessage = pShowErrorMessage;

  TYPE_PlayInfo         PlayInfo;
  bool                  OldRepeatMode = FALSE;
  char                  PlaybackName[MAX_FILE_NAME_SIZE + 1];
  char                  AbsPlaybackDir[FBLIB_DIR_SIZE];
  dword                 LastPlaybackPos;

  FILE                 *fPidFile = NULL, *fLogFileIn = NULL, *fLogFileOut = NULL;
  char                  DeviceNode[20], ListFile[FBLIB_DIR_SIZE];
  char                  CommandLine[512], Buffer[512]; //, PidStr[13];
  char                  FirstErrorFile[80], *p = NULL, *p2 = NULL;
  dword                 fsck_Pid = 0;
  dword                 StartTime;
  int                   DefectFiles = 0, RepairedFiles = 0;
  int                   i;
  bool                  StartProcessing = FALSE, Phase1Active = FALSE, Phase1Counter = 0, Phase4Active = FALSE, Phase10Active;
  bool                  fsck_Errors, fsck_FixErrors;

  TRACEENTER();
  HDD_TAP_PushDir();
  fsck_Cancelled = FALSE;
  remove("/tmp/fsck.log");

  if(!ShowErrorMessage) ShowErrorMessage = &ShowInfoBox;
  if(!RefreshProgBar) RefreshProgBar = &HDDCheck_RefreshProgBar;
  RefreshProgBar(TRUE, 0);

  // --- 1.) Stop the current playback (if any) ---
  TAP_Hdd_GetPlayInfo(&PlayInfo);
  if (PlayInfo.playMode == PLAYMODE_Playing)
  {
    OldRepeatMode = PlaybackRepeatGet();
    PlaybackRepeatSet(TRUE);
    if (DoFix)
      TAP_Hdd_StopTs();
  }
  sync();
  TAP_Sleep(1);

  // --- 2.) Detect the device node of the partition to be checked ---
  HDD_FindMountPointDev2(MountPath, ListFile, DeviceNode);
  TAP_SPrint(LogString, sizeof(LogString), "CheckFileSystem: Checking file system '%s' ('%s')...", ListFile, DeviceNode);
  WriteLogMC("HddToolsLib", LogString);
  strcat(ListFile, "/FixInodes.lst");

  // --- 3.) Try remounting the HDD as read-only first
  if (DoFix)
  {
    TAP_SPrint(CommandLine, sizeof(CommandLine), "mount -o remount,ro %s", DeviceNode);
    if (system(CommandLine) != 0)
      WriteLogMC("HddToolsLib", "CheckFileSystem: Schreibgesch�tzter Remount nicht erfolgreich!");
  }

  // --- 4.) Run fsck and create a log file ---
  StartTime = time(NULL);
  TAP_SPrint(CommandLine, sizeof(CommandLine), "%s/ProgramFiles/jfs_fsck -n -v %s %s %s &> /tmp/fsck.log & echo $!", TAPFSROOT, ((DoFix) ? "-r" : ""), ((Quick) ? "-q" : ""), DeviceNode);  // > /tmp/fsck.pid
  system(CommandLine);

  //Get the PID of the fsck-Process
//  fPidFile = fopen("/tmp/fsck.pid", "r");
  fPidFile = popen(CommandLine, "r");
  if(fPidFile)
  {
//    if (fgets(PidStr, 13, fPidFile))
//      fsck_Pid = atoi(PidStr);
    fscanf(fPidFile, "%ld", &fsck_Pid);
    pclose(fPidFile);
  }

  //Wait for termination of fsck
  i = 0;
  TAP_SPrint(CommandLine, sizeof(CommandLine), "/proc/%lu", fsck_Pid);
  while (access(CommandLine, F_OK) != -1)
  {
//    BytesRead += fread(&LogBuffer[BytesRead], 1, BufSize-BytesRead-1, fLogFile);
//    LogBuffer[BytesRead] = '\0';
//    TAP_PrintNet(LogBuffer);
    TAP_Sleep(100);
    i++;
    if(Quick)
    {
      if ((i < 120) && !(i % 10))  // 12 Schritte � 1 sek
        RefreshProgBar(TRUE, 100 * i / 120);
    }
    else
    {
      if ((i < 600) && !(i % 50))  // 12 Schritte � 5 sek
        RefreshProgBar(TRUE, 100 * i / 600);
    }
    TAP_SystemProc();
    if(fsck_Cancelled)
    {
      char KillCommand[16];
      TAP_SPrint(KillCommand, sizeof(KillCommand), "kill %lu", fsck_Pid);
      system(KillCommand);
    }
  }
  fsck_Pid = 0;

  // --- 5.) Make HDD writable again ---
  if (DoFix)
  {
    TAP_SPrint(CommandLine, sizeof(CommandLine), "mount -o remount,rw %s", DeviceNode);
    system(CommandLine);
  }

  // --- 6.) Restart the playback again ---
  if (DoFix)
  {
    if (PlayInfo.playMode == PLAYMODE_Playing)
    {
      // Get infos about the playback file
      strncpy(PlaybackName, PlayInfo.file->name, sizeof(PlaybackName));
      PlaybackName[MAX_FILE_NAME_SIZE] = '\0';
      PlaybackName[strlen(PlaybackName) - 4] = '\0';

      //Extract the absolute path to the rec file and change into that dir
      HDD_GetAbsolutePathByTypeFile(PlayInfo.file, AbsPlaybackDir);
      AbsPlaybackDir[FBLIB_DIR_SIZE - 1] = '\0';

      char *p;
      p = strstr(AbsPlaybackDir, PlaybackName);
      if(p) *(p-1) = '\0';

      if (strncmp(AbsPlaybackDir, TAPFSROOT, strlen(TAPFSROOT)) != 0)
      {
        char TempStr[FBLIB_DIR_SIZE];
        strncpy(TempStr, AbsPlaybackDir, sizeof(TempStr));
        TAP_SPrint(AbsPlaybackDir, sizeof(AbsPlaybackDir), "%s/..%s", TAPFSROOT, &TempStr[4]);
      }
      LastPlaybackPos = PlayInfo.currentBlock;

      HDD_StartPlayback2(PlaybackName, &AbsPlaybackDir[strlen(TAPFSROOT)]);

      // auf das Starten des Playbacks warten
      i = 0;
      do {
        TAP_SystemProc();
        TAP_Hdd_GetPlayInfo(&PlayInfo);
        i++;
      } while ((i < 2000) && (PlayInfo.playMode != PLAYMODE_Playing || (int)PlayInfo.totalBlock <= 0 || (int)PlayInfo.currentBlock < 0));
  
      PlaybackRepeatSet(TRUE);
      if(LastPlaybackPos > 500)
        TAP_Hdd_ChangePlaybackPos(LastPlaybackPos);
    }
  }

  // --- 7.) Open and analyse the generated log file ---
  fsck_Cancelled = TRUE;
  fsck_Errors = FALSE;
  fsck_FixErrors = FALSE;
  TAP_SPrint(CommandLine, sizeof(CommandLine), "%s/ProgramFiles/Settings/MovieCutter/fsck.log", TAPFSROOT);
  fLogFileOut = fopen(CommandLine, "a");

  if(fLogFileOut)
  {
    fprintf(fLogFileOut, "\n=========================================================\n");
    fprintf(fLogFileOut, "*** File system check started %s\n", asctime(localtime(&StartTime)));
  }

  fLogFileIn = fopen("/tmp/fsck.log", "r");
  if(fLogFileIn)
  {
    FirstErrorFile[0] = '\0';
    while (fgets(Buffer, sizeof(Buffer), fLogFileIn))
    {
      if(fLogFileOut) fputs(Buffer, fLogFileOut);

      if (strncmp(Buffer, "**Phase 1 ", 9) == 0)
      {
        StartProcessing = TRUE;
        Phase1Active = TRUE;
        Phase1Counter++;
        continue;
      }
      else if (StartProcessing == FALSE)
      {
        continue;
      }
      else if (strncmp(Buffer, "**Phase 2", 9) == 0)
      {
        Phase1Active = FALSE;
      }
      else if (strncmp(Buffer, "**Phase 4", 9) == 0)
      {
        Phase4Active = TRUE;
      }
      else if ((strncmp(Buffer, "**Phase 5", 9) == 0) || (strncmp(Buffer, "**Finished", 10) == 0))
      {
        StartProcessing = FALSE;
        Phase1Active = FALSE;
        Phase4Active = FALSE;
        fsck_Cancelled = FALSE;
      }
      else if (strncmp(Buffer, "[MC1]", 5) == 0)  // [MC1] 1234: inode has incorrect nblocks value (nblocks=123, real=456, size=789).
      {                                           
        if (Phase1Counter == 1) DefectFiles++;                            
        RemoveEndLineBreak(Buffer); WriteLogMC("CheckFS", Buffer);

        // Parse the line to get inode values and add them to fixing-list
        tInodeData curInode;        
        curInode.LastFixTime = time(NULL);
        if (sscanf(Buffer, "[MC1] %ld: inode has incorrect nblocks value (nblocks=%lld, real=%lld, size=%lld)", &curInode.InodeNr, &curInode.nblocks_wrong, &curInode.nblocks_real, &curInode.di_size) == 4)
          AddInodeToFixingList(curInode, ListFile);
          
/*        strtok(Buffer, " ");
        curInode.InodeNr        = strtoul(strtok(NULL, ": "),   NULL, 10);
        strtok(NULL, "=");
        curInode.nblocks_wrong  = strtoll(strtok(NULL, ", )."), NULL, 10);
        strtok(NULL, "=");
        curInode.nblocks_real   = strtoll(strtok(NULL, ", )."), NULL, 10);
        strtok(NULL, "=");
        curInode.di_size        = strtoll(strtok(NULL, ", )."), NULL, 10);
        curInode.LastFixTime    = time(NULL);
        AddInodeToFixingList(curInode, DeviceNode);  */
      }
      else if (strncmp(Buffer, "[MC2]", 5) == 0)  // [MC2] 1234: inode has been successfully fixed.
      {
        fsck_Errors = TRUE;
        RepairedFiles++;
      }
      else if (strncmp(Buffer, "[MC3]", 5) == 0)  // [MC3] 1234: Error fixing nblocks value (return code x).
      {
        fsck_Errors = TRUE;
        fsck_FixErrors = TRUE;
      }
      else if (strncmp(Buffer, "[MC4]", 5) == 0)  // [MC4] x of y files successfully fixed.
      {
        fsck_Errors = TRUE;
      }
      else if ((p = strstr(Buffer, "is linked as: ")) && !FirstErrorFile[0])
      {
        p = strstr(Buffer, "is linked as: ");
        if(p && (p+14))
        {
          p += 14;
          p2 = strrchr(p, '/');
          if (p2 && (p2+1))
            TAP_SPrint(FirstErrorFile, sizeof(FirstErrorFile), "\"%s\"", p2+1);
          else
            TAP_SPrint(FirstErrorFile, sizeof(FirstErrorFile), "\"%s\"", p);
          FirstErrorFile[sizeof(FirstErrorFile)-1] = '\0';
        }
      }
      else if (Phase1Active)
      {
        if (Phase1Counter > 1) fsck_FixErrors = TRUE;
      }
      else if (Phase4Active)
      {
        fsck_Errors = TRUE;
//        if (!FirstErrorFile[0])
          TAP_SPrint(FirstErrorFile, sizeof(FirstErrorFile), LS_MoreErrors);
      }

      if(Phase1Active)
      {
        RemoveEndLineBreak(Buffer);
        WriteLogMC("CheckFS", Buffer);
      }
    }
    fclose(fLogFileIn);
    if(fLogFileOut) fclose(fLogFileOut);
  }
  else
    WriteLogMC("HddToolsLib", "CheckFileSystem() E1c01.");

  // Copy the log to MovieCutter folder
  TAP_SPrint(CommandLine, sizeof(CommandLine), "cp /tmp/fsck.log %s/ProgramFiles/Settings/MovieCutter/Lastfsck.log", TAPFSROOT);
  system(CommandLine);

  // --- 8.) Output after completion or abortion of process ---
  if (!fsck_Cancelled)
  {
    // Display full ProgressBar and destroy it
    RefreshProgBar(TRUE, 100);
    TAP_Sleep(100);
    RefreshProgBar(FALSE, 100);

    // Display information message box
    if(!fsck_Errors)
    {
      WriteLogMC("HddToolsLib", "CheckFileSystem: File system seems valid.");
      ShowInfoBox(SuccessString, "FileSystemCheck");
//      TAP_Osd_Sync();
    }
    else
    {
      WriteLogMC("HddToolsLib", "CheckFileSystem: WARNING! File system is inconsistent...");

      // Detaillierten Fehler-String in die Message schreiben
      if (DefectFiles > 1)
        TAP_SPrint(&FirstErrorFile[strlen(FirstErrorFile)], sizeof(FirstErrorFile)-strlen(FirstErrorFile), ", + %d", DefectFiles-1);
//      StrMkISO(FirstErrorFile);
      TAP_SPrint(LogString, sizeof(LogString), ErrorStrFmt, RepairedFiles, DefectFiles, ((fsck_FixErrors) ? "??" : "ok"), ((FirstErrorFile[0]) ? FirstErrorFile : ""));
      //#ifdef Calibri_10_FontDataUC
        if (isUTFToppy()) StrMkISO(LogString);
      //#endif
      WriteLogMC("HddToolsLib", LogString);
      ShowErrorMessage(LogString, LS_Warning);
    }
  }
  
  RefreshProgBar(FALSE, 100);
  if(fsck_Cancelled)
  {
    WriteLogMC("HddToolsLib", "CheckFileSystem: File system check has been aborted!");
    ShowErrorMessage(AbortedString, LS_Warning);
  }

  if (PlayInfo.playMode == PLAYMODE_Playing)
    PlaybackRepeatSet(OldRepeatMode);
  HDD_TAP_PopDir();
  TRACEEXIT();
  return (!fsck_Cancelled && !fsck_Errors);
}

// ----------------------------------------------------------------------------------------------------------

bool AddInodeToFixingList(tInodeData pInode, const char *AbsListFile)
{
//  char                  MountPoint[MAX_FILE_NAME_SIZE + 1], AbsListFile[MAX_FILE_NAME_SIZE + 1];
  FILE                 *fListFile = NULL;
  tInodeData            curInode;
  bool                  ret = FALSE;

  TRACEENTER();
  TAP_SPrint(LogString, sizeof(LogString), "AddInodeToFixingList: InodeNr=%lu, LastFixedTime=%lu, di_size=%lld, nblocks_wrong=%lld, nblocks_real=%lld.", pInode.InodeNr, pInode.LastFixTime, pInode.nblocks_wrong, pInode.nblocks_real, pInode.di_size);
  WriteLogMC("HddToolsLib", LogString);
  WriteDebugLog(LogString);

  // Get the list filename
//  HDD_GetMountPointFromDevice(DeviceNode, MountPoint);
//  TAP_SPrint(AbsListFile, sizeof(AbsListFile), "%s/FixInodes.lst", MountPoint);

  fListFile = fopen(AbsListFile, "r+b");
  if(!fListFile)
    fListFile = fopen(AbsListFile, "wb");
  if(fListFile)
  {
    while(fread(&curInode, sizeof(tInodeData), 1, fListFile))
    {
      if(curInode.InodeNr == pInode.InodeNr)
      {
        fseek(fListFile, -sizeof(tInodeData), SEEK_CUR);
        break;
      }
    }
    if (pInode.LastFixTime > (dword)time(NULL))
      pInode.LastFixTime = time(NULL);
    ret = fwrite(&pInode,  sizeof(tInodeData), 1, fListFile);
    fclose(fListFile);
  }

  TRACEEXIT();
  return ret;
}

void DumpInodeFixingList(const char *AbsListFile)
{
  FILE                 *fListFile = NULL;
  tInodeData            curInode;
  int                   i = 0;

  TRACEENTER();

  TAP_PrintNet("DUMPING inode fixing list %s:\n", AbsListFile);
  TAP_PrintNet("-----------------------------\n");
  fListFile = fopen(AbsListFile, "rb");
  if(fListFile)
  {
    while(fread(&curInode, sizeof(tInodeData), 1, fListFile))
      TAP_PrintNet("  %d: InodeNr=%lu, LastFixed=%lu, di_size=%lld, wrong=%lld,\n      nblocks_real=%lld, FileName=%s.\n", ++i, curInode.InodeNr, curInode.LastFixTime, curInode.di_size, curInode.nblocks_wrong, curInode.nblocks_real, curInode.FileName);
    fclose(fListFile);
  }

  TRACEEXIT();
}

bool HDD_FixInodeList(const char *MountPath, bool DeleteOldEntries)
{
  char                  DeviceNode[20], ListFile[MAX_FILE_NAME_SIZE + 1];
  char                  CommandLine[512], LastLine[512];
  FILE                 *LogStream;
  int                   ret = -1;

  TRACEENTER();

  // Get the list filename
  HDD_FindMountPointDev2(MountPath, ListFile, DeviceNode);
  strcat(ListFile, "/FixInodes.lst");

  TAP_SPrint(LogString, sizeof(LogString), "Checking list of suspect inodes (Device=%s, ListFile=%s):", DeviceNode, ListFile);
  WriteLogMC("HddToolsLib", LogString);
  WriteDebugLog(LogString);

  // Check if ListFile exists on the drive
  if (access(ListFile, F_OK) != 0)
  {
    WriteLogMC("HddToolsLib", "-> No ListFile present.");
    WriteDebugLog("-> No ListFile present.");
    TRACEEXIT();
    return TRUE;
  }

  // Execute jfs_icheck and read its output (last line separately)
  LogString[0] = '\0';
  TAP_SPrint(CommandLine, sizeof(CommandLine), "%s/ProgramFiles/jfs_fsck icheck -f %s %s \"%s\" 2>&1", TAPFSROOT, DeviceNode, ((DeleteOldEntries) ? "-L" : "-l"), ListFile);
  LogStream = popen(CommandLine, "r");
  if(LogStream)
  {
    fgets(LogString, sizeof(LogString), LogStream);
    LogString[0] = '\0';
    while (fgets(LastLine, sizeof(LastLine), LogStream))
    {
      if(!feof(LogStream))
      {
        strncpy(&LogString[strlen(LogString)], LastLine, sizeof(LogString)-strlen(LogString)-1);
        LogString[sizeof(LogString)-1] = '\0';
      }
    }
    ret = pclose(LogStream) / 256;
  }

  // Write output of jfs_icheck to Logfile
  if (LogString[0])
  {
    RemoveEndLineBreak(LogString);
    WriteLogMC("HddToolsLib", LogString);
    WriteLogMC("HddToolsLib", LastLine);
    WriteDebugLog(LogString);
    WriteDebugLog(LastLine);
  }
  // Write result state to Logfile
  if ((ret < 0) || (ret & !0x27) || ((ret & 0x02) && !(ret & 0x04)))
  {
    TAP_SPrint(LogString, sizeof(LogString), "Error! jfs_icheck returned %d.", ret);
    WriteLogMC("HddToolsLib", LogString);
    WriteDebugLog(LogString);
  }

  TRACEEXIT();
  return (ret >= 0 && !(ret & 0x02));  // 0x02 = File needs fix
}

// ----------------------------------------------------------------------------------------------------------

bool HDD_CheckInode(const char *FileName, const char *Directory, const char *DeviceNode, bool DoFix, const char *Comment)
{
  char                  CommandLine[1024];
  FILE                 *LogStream;
  int                   ret = -1;

  TRACEENTER();
  TAP_SPrint(LogString, sizeof(LogString), "Checking file inodes for wrong di_nblocks (Device=%s):", DeviceNode);
  WriteLogMC("HddToolsLib", LogString);

/*  if(DoFix)
  {
    TAP_SPrint(CommandLine, sizeof(CommandLine), "mount -o remount,ro %s", DeviceNode);
    if (system(CommandLine) != 0)
      WriteDebugLog("Schreibgesch�tzter Remount nicht erfolgreich!");
  }  */

  // Execute jfs_icheck and read its output
  LogString[0] = '\0';
  TAP_SPrint(CommandLine, sizeof(CommandLine), "%s/ProgramFiles/jfs_fsck icheck %s %s \"%s%s/%s\" 2>&1", TAPFSROOT, (DoFix) ? "-f" : "", DeviceNode, TAPFSROOT, Directory, FileName);
  LogStream = popen(CommandLine, "r");
  if(LogStream)
  {
    fgets(LogString, sizeof(LogString), LogStream);
    LogString[0] = '\0';
    while (fgets(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), LogStream))
    { /* nothing */ }
    ret = pclose(LogStream) / 256;
  }

/*  if (DoFix)
  {
    TAP_SPrint(CommandLine, sizeof(CommandLine), "mount -o remount,rw %s", DeviceNode);
    system(CommandLine);
    sync();
    TAP_Sleep(1);
  }  */

  // Write output of jfs_icheck to Logfile
  if (LogString[0])
  {
    RemoveEndLineBreak(LogString);
    WriteLogMC("HddToolsLib", LogString);
    WriteDebugLog("%s:\t%s", Comment, LogString);
  }
  // Write result state to Logfile
  if ((ret < 0) || (ret & !0x06) || ((ret & 0x02) && DoFix && !(ret & 0x04)))
  {
    TAP_SPrint(LogString, sizeof(LogString), "Error! jfs_icheck returned %d.", ret);
    WriteLogMC("HddToolsLib", LogString);
    WriteDebugLog(LogString);
  }

  TRACEEXIT();
  return (ret >= 0 && !(ret & 0x01) && !(ret & 0x02));  // 0x02 = File needs fix
}
