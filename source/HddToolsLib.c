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
#include                "../jfsutils/icheck/jfs_icheck.h"

bool                    fsck_Cancelled;
word                    RegionToSave;
char*                   LS_Dummy = "< Dummy >";
char                   *LS_Warning, *LS_CheckingFileSystem;


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
void  HDDCheck_InitProgBar(dword pProgressStart, dword pProgressEnd, dword pProgressMax, word pRegionToSave, char *pWarningStr, char *pCheckingStr)
{
  OSDMenuSaveMyRegion(pRegionToSave);
  if (pWarningStr)    LS_Warning            = pWarningStr;      else LS_Warning            = LS_Dummy;
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
  TAP_Delay(300);
  OSDMenuInfoBoxDestroyNoOSDUpdate();
}

void HDD_CancelCheckFS()
{
  fsck_Cancelled = TRUE;
}

bool  HDD_CheckFileSystem(const char *AbsMountPath, TProgBarHandler pRefreshProgBar, TMessageHandler pShowErrorMessage, bool DoFix, bool Quick, bool InodeMonitoring, bool NoOkInfo, char *InodeNrs, char *SuccessString, char *ErrorStrFmt, char *AbortedString)
{
  TProgBarHandler       RefreshProgBar = pRefreshProgBar;
  TMessageHandler       ShowErrorMessage = pShowErrorMessage;

  TYPE_PlayInfo         PlayInfo;
  bool                  PlaybackWasRunning = FALSE, OldRepeatMode = FALSE;
  char                  PlaybackName[MAX_FILE_NAME_SIZE + 1];
  char                  AbsPlaybackDir[FBLIB_DIR_SIZE];
  char                  MessageString[512];
  dword                 LastPlaybackPos = 0;

  FILE                 *fPidFile = NULL, *fLogFileIn = NULL, *fLogFileOut = NULL;
  char                  DeviceNode[20], MountPoint[FBLIB_DIR_SIZE];
  char                  CommandLine[1024], Buffer[512]; //, PidStr[13];
  char                  FirstErrorFile[50], *p = NULL;
  dword                 fsck_Pid = 0;
  dword                 StartTime;
  tInodeData            curInode, *InodeList;
  unsigned int          NrDefectFiles = 0, NrRepairedFiles = 0, NrMarkedFiles = 0, NrNewMarkedFiles = 0, ActivePhase = 0;
  bool                  fsck_Errors = FALSE;
  unsigned int          i;

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
    // Get infos about the playback file
    strncpy(PlaybackName, PlayInfo.file->name, sizeof(PlaybackName));
    PlaybackName[MAX_FILE_NAME_SIZE] = '\0';
    PlaybackName[strlen(PlaybackName) - 4] = '\0';

    //Extract the absolute path to the rec file and change into that dir
    HDD_GetAbsolutePathByTypeFile2(PlayInfo.file, AbsPlaybackDir);
//    AbsPlaybackDir[FBLIB_DIR_SIZE - 1] = '\0';

    p = strstr(AbsPlaybackDir, PlaybackName);
    if(p) *(p-1) = '\0';

    LastPlaybackPos = PlayInfo.currentBlock;
    OldRepeatMode = PlaybackRepeatGet();
    PlaybackRepeatSet(TRUE);
    PlaybackWasRunning = TRUE;

    if (DoFix)
      TAP_Hdd_StopTs();
  }
  sync();
  TAP_Sleep(1);

  // --- 2.) Detect the device node of the partition to be checked ---
  HDD_FindMountPointDev2(AbsMountPath, MountPoint, DeviceNode);
  WriteLogMCf("HddToolsLib", "CheckFileSystem: Checking file system '%s' ('%s')...", MountPoint, DeviceNode);

  // --- 3.) Try remounting the HDD as read-only first
  if (DoFix)
  {
    TAP_SPrint(CommandLine, sizeof(CommandLine), "mount -o remount,ro %s", DeviceNode);
    if (system(CommandLine) != 0)
      WriteLogMC("HddToolsLib", "CheckFileSystem: Schreibgeschützter Remount nicht erfolgreich!");
  }

  // --- 4.) Run fsck and create a log file ---
  StartTime = time(NULL);
  TAP_SPrint(CommandLine, sizeof(CommandLine), FSCKPATH "/jfs_fsck -n -v %s %s %s -L /tmp/FixInodes.tmp %s %s &> /tmp/fsck.log & echo $!", ((DoFix) ? "-r" : ""), ((Quick) ? "-q" : ""), ((Quick && InodeNrs) ? "-i" : ""), DeviceNode, ((InodeNrs) ? InodeNrs : ""));  // > /tmp/fsck.pid
//-  system(CommandLine);

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
      if ((i < 120) && !(i % 10))  // 12 Schritte á 1 sek
        RefreshProgBar(TRUE, 100 * i / 120);
    }
    else
    {
      if ((i < 600) && !(i % 50))  // 12 Schritte á 5 sek
        RefreshProgBar(TRUE, 100 * i / 600);
    }
    TAP_SystemProc();
    if(fsck_Cancelled && (!DoFix || !Quick || !InodeNrs))
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
  if (DoFix && PlaybackWasRunning)
  {
    HDD_StartPlayback2(PlaybackName, AbsPlaybackDir);

    // auf das Starten des Playbacks warten
    i = 0;
    do {
      TAP_SystemProc();
      TAP_Hdd_GetPlayInfo(&PlayInfo);
      i++;
    } while ((i < 2000) && (PlayInfo.playMode != PLAYMODE_Playing || (int)PlayInfo.totalBlock <= 0 || (int)PlayInfo.currentBlock < 0));

    PlaybackRepeatSet(TRUE);
    if(LastPlaybackPos >= 1000)
      TAP_Hdd_ChangePlaybackPos(LastPlaybackPos);
  }

  // --- 7.) Open and analyse the generated log file ---
  fsck_Cancelled = TRUE;
  fLogFileOut = fopen(ABSLOGDIR "/fsck.log", "a");

  if(fLogFileOut)
  {
    fprintf(fLogFileOut, "\r\n=========================================================\r\n");
    fprintf(fLogFileOut, "*** File system check started %s\r\n", asctime(localtime(&StartTime)));
  }

  fLogFileIn = fopen("/tmp/fsck.log", "r");
  if(fLogFileIn)
  {
    FirstErrorFile[0] = '\0';
    while (fgets(Buffer, sizeof(Buffer), fLogFileIn))
    {
      if(fLogFileOut)
      {
        dword p = strlen(Buffer);
        if (p)
        {
          fwrite(Buffer, p-1, 1, fLogFileOut);
          fwrite("\r\n", 2, 1, fLogFileOut);
        }
      }

      if (strncmp(Buffer, "**Phase", 7) == 0)
      {
        switch (Buffer[8])
        {
          case '1':  ActivePhase =  1; break;
          case '4':  ActivePhase =  4; break;
          case 'i':  ActivePhase = 10; break;
          default:   ActivePhase =  0;
        }
      }
      else if (strncmp(Buffer, "**Finished", 10) == 0)
      {
        ActivePhase = 0;
        if(!DoFix || (strncmp(Buffer, "**Finished all.", 15) == 0))
          fsck_Cancelled = FALSE;
      }

      if (ActivePhase == 1)
      {
        if (strncmp(Buffer, "[MC1]", 5) == 0)       // [MC1] %lu: inode is incorrect (nblocks=%lld, real=%lld, size=%lld).
        {
          NrDefectFiles++;                            
/*          // Parse the line to get inode values and add them to fixing-list
          curInode.LastFixTime = time(NULL);
          if (sscanf(Buffer, "[MC1] %ld: inode is incorrect (nblocks=%lld, real=%lld, size=%lld).", &curInode.InodeNr, &curInode.nblocks_wrong, &curInode.nblocks_real, &curInode.di_size) == 4)
            AddInodeToFixingList(curInode, DeviceNode);  */
        }
        else if (strncmp(Buffer, "[MC2]", 5) == 0)  // [MC2] %lu: Error marking the inode for correction.
        {
          fsck_Errors = TRUE;
        }
        else if (strncmp(Buffer, "[MC3]", 5) == 0)  // [MC3] %d incorrect inodes found, %d marked for correction.
        {
          unsigned int Temp1, Temp2;
          if (sscanf(Buffer, "[MC3] %d incorrect inodes found, %d marked for correction.", &Temp1, &Temp2) == 2)
          {
            NrDefectFiles = Temp1;
            if (Temp2 != Temp1 || Temp1 != NrDefectFiles)
              fsck_Errors = TRUE;
          }
        }
      }
        
      else if (ActivePhase == 4)
      {
        if (!FirstErrorFile[0] && (strncmp(Buffer, "[MC4]", 5) == 0))  // [MC4] File system object %s%s%u is linked as: %s
          if (sscanf(Buffer, "[MC4] File system object FF%*d is linked as: %511[^\n]", MessageString) == 1)
          {
            if (NrDefectFiles == 0)
              fsck_Errors = TRUE;
            p = NULL;
            if (strlen(MessageString) > sizeof(FirstErrorFile) - 10)
              p = strrchr(MessageString, '/');
            p = (p && p[1]) ? (p+1) : MessageString;
            p[sizeof(FirstErrorFile) - 10] = '\0';
            TAP_SPrint(FirstErrorFile, sizeof(FirstErrorFile) - 6, "\n'%s'", p);
          }
      }
        
      else if (ActivePhase == 10)
      {
        if (strncmp(Buffer, "[MC5]", 5) == 0)       // [MC5] %d marked files successfully fixed. (%d)
        {
          sscanf(Buffer, "[MC5] %d marked files successfully fixed.", &NrRepairedFiles);
          if (NrRepairedFiles != NrDefectFiles)
            fsck_Errors = TRUE;
        }
        else if (strncmp(Buffer, "[MC6]", 5) == 0)  // [MC6] Error! Correction with icheck returned %d.
          fsck_Errors = TRUE;
        else if (strncmp(Buffer, "[MC9]", 5) == 0)  // [MC9] Error! Cannot write to list file '%s'.
          fsck_Errors = TRUE;
      }

      if(ActivePhase != 0)
      {
        RemoveEndLineBreak(Buffer);
        WriteLogMC("CheckFS", Buffer);
      }
    }
    fclose(fLogFileIn);
  }
  else
    WriteLogMC("HddToolsLib", "CheckFileSystem() E1c01.");
  if(fLogFileOut) fclose(fLogFileOut);

  // Copy the log to MovieCutter folder
  system("cp /tmp/fsck.log " ABSLOGDIR "/Lastfsck.log");

  // --- 8.) Copy the FixInodes list to root of drive ---
  if(InodeMonitoring)
  {
    TAP_SPrint(CommandLine, sizeof(CommandLine), "%s/FixInodes.lst", MountPoint);

    // Dateigröße bestimmen um Puffer zu allozieren
    int64_t fsOut = 0, fsIn = 0;
    HDD_GetFileSizeAndInode2("FixInodes.lst", MountPoint, NULL, &fsOut);
    HDD_GetFileSizeAndInode2("FixInodes.tmp", "/tmp", NULL, &fsIn);

    if (fsIn > sizeof(tInodeListHeader))
    {
      tInodeListHeader InListHeader, OutListHeader;

TAP_PrintNet("Puffer alloz. %llu\n", (fsOut - min(sizeof(tInodeListHeader), fsOut) + fsIn - min(sizeof(tInodeListHeader), fsIn)) / sizeof(tInodeData) * sizeof(tInodeData));
      InodeList = (tInodeData*) malloc((fsOut - min(sizeof(tInodeListHeader), fsOut) + fsIn - min(sizeof(tInodeListHeader), fsIn)) / sizeof(tInodeData) * sizeof(tInodeData));
      if(InodeList)
      {
        // bisherige FixInodes.lst einlesen
        fLogFileOut = fopen(CommandLine, "rb");
        if(fLogFileOut)
        {
          // Header prüfen
          if ( (fread(&OutListHeader, sizeof(tInodeListHeader), 1, fLogFileOut))
            && (strncmp(OutListHeader.Magic, "TFinos", 6) == 0)
            && (OutListHeader.Version == 1)
            && (OutListHeader.FileSize == fsOut)
            && (OutListHeader.NrEntries * sizeof(tInodeData) + sizeof(tInodeListHeader) == fsOut))
          {
            NrMarkedFiles = fread(InodeList, sizeof(tInodeData), OutListHeader.NrEntries, fLogFileOut);
            if (NrMarkedFiles != OutListHeader.NrEntries)
              WriteLogMC("HddToolsLib", "CheckFileSysem() W1c02.");
          }
          else
            WriteLogMC("HddToolsLib", "CheckFileSysem() W1c01.");
          fclose(fLogFileOut);
        }

        // neue Inodes hinzufügen
        fLogFileIn = fopen("/tmp/FixInodes.tmp", "rb");
        if(fLogFileIn)
        {
          // Header prüfen
          if ( (fread(&InListHeader, sizeof(tInodeListHeader), 1, fLogFileIn))
            && (strncmp(InListHeader.Magic, "TFinos", 6) == 0)
            && (InListHeader.Version == 1)
            && (InListHeader.FileSize == fsIn)
            && (InListHeader.NrEntries * sizeof(tInodeData) + sizeof(tInodeListHeader) == fsIn))
          {
            while (!feof(fLogFileIn))
            {
              if (fread(&curInode, sizeof(tInodeData), 1, fLogFileIn))
              {
                for (i = 0; i < NrMarkedFiles; i++)
                {
                  if (InodeList[i].InodeNr == curInode.InodeNr)
                  {
                    if (!curInode.FileName[0] && (curInode.di_size == InodeList[i].di_size))
                      strcpy(curInode.FileName, InodeList[i].FileName);
                    break;
                  }
                }
                InodeList[i] = curInode;
                if (i == NrMarkedFiles) NrMarkedFiles++;
                NrNewMarkedFiles++;
              }
            }
          }
          else
            WriteLogMC("HddToolsLib", "CheckFileSysem() W1c03.");
          fclose(fLogFileIn);
        }

        // aktualisierte Liste speichern
        if (NrMarkedFiles > 0)
        {
          fLogFileOut = fopen(CommandLine, "wb");
          if(fLogFileOut)
          {
            strcpy(OutListHeader.Magic, "TFinos");
            OutListHeader.Version = 1;
            OutListHeader.NrEntries = NrMarkedFiles;
            OutListHeader.FileSize  = (NrMarkedFiles * sizeof(tInodeData)) + sizeof(tInodeListHeader);
            if(!fwrite(&OutListHeader, sizeof(tInodeListHeader), 1, fLogFileOut))
              WriteLogMC("HddToolsLib", "CheckFileSysem() W1c05.");

            if(fwrite(InodeList, sizeof(tInodeData), NrMarkedFiles, fLogFileOut) != NrMarkedFiles)
              WriteLogMC("HddToolsLib", "CheckFileSysem() W1c06.");
            fclose(fLogFileOut);
          }
          else
            WriteLogMC("HddToolsLib", "CheckFileSysem() W1c04.");
        }
        if (NrNewMarkedFiles != NrRepairedFiles)
          fsck_Errors = TRUE;
      }
      else
        WriteLogMC("HddToolsLib", "CheckFileSysem() E1c01.");
    }
    else
      NrMarkedFiles = (fsOut - min(sizeof(tInodeListHeader), fsOut)) / sizeof(tInodeData);
DumpInodeFixingList(CommandLine);
  }

  // --- 9.) Output after completion or abortion of process ---
  if (!fsck_Cancelled)
  {
    // Display full ProgressBar and destroy it
    RefreshProgBar(TRUE, 100);
    TAP_Sleep(100);
    RefreshProgBar(FALSE, 100);

    // Display information message box
    if(!fsck_Errors && (NrDefectFiles == 0))
    {
      WriteLogMCf("HddToolsLib", "CheckFileSystem: File system seems valid. Monitored files: %u", NrMarkedFiles);
      if (!NoOkInfo)
      {
        TAP_SPrint(MessageString, sizeof(MessageString), SuccessString, NrMarkedFiles);
        ShowInfoBox(MessageString, "FileSystemCheck");
      }
//      TAP_Osd_Sync();
    }
    else
    {
      WriteLogMC("HddToolsLib", "CheckFileSystem: WARNING! File system is inconsistent...");

      // Detaillierten Fehler-String in die Message schreiben
      if (FirstErrorFile[0] && (NrDefectFiles > 1))
        TAP_SPrint(&FirstErrorFile[strlen(FirstErrorFile)], sizeof(FirstErrorFile)-strlen(FirstErrorFile), ", + %u", NrDefectFiles-1);
//      StrMkISO(FirstErrorFile);
      TAP_SPrint(MessageString, sizeof(MessageString), ErrorStrFmt, ((InodeMonitoring) ? min(NrNewMarkedFiles, NrRepairedFiles) : NrRepairedFiles), NrDefectFiles, ((fsck_Errors) ? "??" : "ok"), ((FirstErrorFile[0]) ? FirstErrorFile : ""), NrMarkedFiles);
      //#ifdef Calibri_10_FontDataUC
        if (isUTFToppy()) StrMkISO(MessageString);
      //#endif
      WriteLogMC("HddToolsLib", MessageString);
      ShowErrorMessage(MessageString, LS_Warning);
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
  return (!fsck_Cancelled && !fsck_Errors && NrDefectFiles == 0);
}

// ----------------------------------------------------------------------------------------------------------

/* bool AddInodeToFixingList(tInodeData pInode, const char *AbsListFile)
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
}  */

void DumpInodeFixingList(const char *AbsListFile)
{
  FILE                 *fListFile = NULL;
  tInodeData            curInode;
  int                   i = 0;

  TRACEENTER();

  WriteLogMCf("HddToolsLib", "DUMPING inode fixing list %s:", AbsListFile);
  fListFile = fopen(AbsListFile, "rb");
  if(fListFile)
  {
    // Header prüfen
    tInodeListHeader ListFileHeader;
    if ( (fread(&ListFileHeader, sizeof(tInodeListHeader), 1, fListFile))
      && (strncmp(ListFileHeader.Magic, "TFinos", 6) == 0)
      && (ListFileHeader.Version == 1)
//      && (ListFileHeader.FileSize == fs)
      && (ListFileHeader.NrEntries * sizeof(tInodeData) + sizeof(tInodeListHeader) == ListFileHeader.FileSize))
    {
      while(fread(&curInode, sizeof(tInodeData), 1, fListFile))
        WriteLogMCf("HddToolsLib", "  %d: InodeNr=%lu, LastFixed=%lu, di_size=%lld, wrong=%lld, nblocks_real=%lld, FileName=%s.", ++i, curInode.InodeNr, curInode.LastFixTime, curInode.di_size, curInode.nblocks_wrong, curInode.nblocks_real, curInode.FileName);
    }
    else
      WriteLogMC("HddToolsLib", "--> Invalid list file header!");
    fclose(fListFile);
  }
  else
    WriteLogMC("HddToolsLib", "--> List file not existing.");

  TRACEEXIT();
}

bool HDD_FixInodeList(const char *AbsMountPath, bool DeleteOldEntries)
{
  char                  DeviceNode[20], ListFile[MAX_FILE_NAME_SIZE + 1];
  char                  CommandLine[512], FullLog[512], LastLine[512];
  FILE                 *LogStream;
  int                   ret = -1;

  TRACEENTER();

  // Get the list filename
  HDD_FindMountPointDev2(AbsMountPath, ListFile, DeviceNode);
  strcat(ListFile, "/FixInodes.lst");

  WriteLogMCf("HddToolsLib", "Checking list of suspect inodes (Device=%s, ListFile=%s):", DeviceNode, ListFile);
  WriteDebugLog(             "Checking list of suspect inodes (Device=%s, ListFile=%s):", DeviceNode, ListFile);

  // Check if ListFile exists on the drive
  if (access(ListFile, F_OK) != 0)
  {
    WriteLogMC("HddToolsLib", "-> No ListFile present.");
    WriteDebugLog("-> No ListFile present.");
    TRACEEXIT();
    return TRUE;
  }

  // Execute jfs_icheck and read its output (last line separately)
  FullLog[0] = '\0';
  TAP_SPrint(CommandLine, sizeof(CommandLine), FSCKPATH "/jfs_fsck icheck -f %s \"%s\" %s 2>&1", ((DeleteOldEntries) ? "-L" : "-l"), ListFile, DeviceNode);
  LogStream = popen(CommandLine, "r");
  if(LogStream)
  {
    fgets(FullLog, sizeof(FullLog), LogStream);
    FullLog[0] = '\0';
    while (fgets(LastLine, sizeof(LastLine), LogStream))
    {
      RemoveEndLineBreak(LastLine);
      if(!feof(LogStream))
      {
        dword p = strlen(FullLog);
        if(FullLog[0] && p < sizeof(FullLog)-2)
        {
          FullLog[p] = '\r';
          FullLog[p+1] = '\n';
          FullLog[p+2] = '\0';
        }
        strncpy(&FullLog[strlen(FullLog)], LastLine, sizeof(FullLog) - strlen(FullLog) - 1);
        FullLog[sizeof(FullLog) - 1] = '\0';
      }
    }
    ret = pclose(LogStream) / 256;
  }

  // Write output of jfs_icheck to Logfile
  if (FullLog[0])
  {
    WriteLogMC("HddToolsLib", FullLog);
    WriteLogMC("HddToolsLib", LastLine);
    WriteDebugLog(FullLog);
    WriteDebugLog(LastLine);
  }
  // Write result state to Logfile
  if (!(ret == rc_NOFILEFOUND || ret == rc_ALLFILESOKAY || ret == rc_ALLFILESFIXED))  // NICHT: keine gefunden, alle ok oder alle gefixt
  {
    WriteLogMCf("HddToolsLib", "Error! jfs_icheck returned %d.", ret);
    WriteDebugLog(             "Error! jfs_icheck returned %d.", ret);
  }
DumpInodeFixingList(ListFile);

  TRACEEXIT();
  return (ret == rc_NOFILEFOUND || ret == rc_ALLFILESOKAY || ret == rc_ALLFILESFIXED);  // keine gefunden, alle okay oder alle gefixt
}

// ----------------------------------------------------------------------------------------------------------

bool HDD_CheckInode(const char *FileName, const char *AbsDirectory, const char *DeviceNode, bool DoFix, const char *Comment)
{
  char                  CommandLine[1024];
  char                  LogString[512];
  FILE                 *LogStream;
  int                   ret = -1;

  TRACEENTER();
  WriteLogMCf("HddToolsLib", "Checking file inodes for wrong di_nblocks (Device=%s):", DeviceNode);

/*  if(DoFix)
  {
    TAP_SPrint(CommandLine, sizeof(CommandLine), "mount -o remount,ro %s", DeviceNode);
    if (system(CommandLine) != 0)
      WriteDebugLog("Schreibgeschützter Remount nicht erfolgreich!");
  }  */

  // Execute jfs_icheck and read its output
  LogString[0] = '\0';
  TAP_SPrint(CommandLine, sizeof(CommandLine), FSCKPATH "/jfs_fsck icheck %s %s \"%s/%s\" 2>&1", ((DoFix) ? "-f" : ""), DeviceNode, AbsDirectory, FileName);
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
  if (!(ret == rc_ALLFILESOKAY || ret == rc_ALLFILESFIXED || (!DoFix && ret == rc_SOMENOTFIXED)))  // NICHT: Datei ist ok, oder gefixt, oder sollte nicht gefixt werden
  {
    WriteLogMCf("HddToolsLib", "Error! jfs_icheck returned %d.", ret);
    WriteDebugLog(             "Error! jfs_icheck returned %d.", ret);
  }

  TRACEEXIT();
  return (ret == rc_ALLFILESOKAY);  // Datei ist okay
}
