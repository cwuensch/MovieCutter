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
#include                "libFireBird.h"
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
void HDDCheck_InitProgBar(dword pProgressStart, dword pProgressEnd, dword pProgressMax, word pRegionToSave, char *pWarningStr, char *pCheckingStr)
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

// ----------------------------------------------------------------------------------------------------------

tInodeData* ReadListFileAlloc(const char *AbsListFileName, int *OutNrInodes, int AddEntries)
{
  tInodeListHeader      InodeListHeader;
  tInodeData           *InodeList  = NULL;
  FILE                 *fInodeList = NULL;
  int                   NrInodes = 0;
  unsigned long         fs = 0;

  TRACEENTER();
  InodeListHeader.NrEntries = 0;
  #if STACKTRACE == TRUE
    TAP_PrintNet("ReadListFileAlloc: OutNrInodes: %d, HeaderNrEntries: %d\n", (OutNrInodes) ? *OutNrInodes : 0, InodeListHeader.NrEntries);
  #endif

  fInodeList = fopen(AbsListFileName, "rb");
  if(fInodeList)
  {
    // Dateigröße bestimmen um Puffer zu allozieren
    fseek(fInodeList, 0, SEEK_END);
    fs = ftell(fInodeList);
    rewind(fInodeList);

    // Header prüfen
    if (!( (fread(&InodeListHeader, sizeof(tInodeListHeader), 1, fInodeList))
        && (strncmp(InodeListHeader.Magic, "TFinos", 6) == 0)
        && (InodeListHeader.Version == 1)
        && (InodeListHeader.FileSize == fs)
        && (InodeListHeader.NrEntries * sizeof(tInodeData) + sizeof(tInodeListHeader) == fs)))
    {
      fclose(fInodeList);
      WriteLogMC("HddToolsLib", "ReadListFileAlloc: Invalid list file header!");
      TRACEEXIT();
      return NULL;
    }
  }
  #if STACKTRACE == TRUE
    else TAP_PrintNet("File could not be opened: %s\n", AbsListFileName);
  #endif

  if(OutNrInodes) *OutNrInodes = InodeListHeader.NrEntries;

  // Puffer allozieren
  if (InodeListHeader.NrEntries + AddEntries > 0)
  {
    #if STACKTRACE == TRUE
      TAP_PrintNet("ReadListFileAlloc(): Puffer alloziert (%d entries, %d Bytes)\n", InodeListHeader.NrEntries+AddEntries, (InodeListHeader.NrEntries+AddEntries)*sizeof(tInodeData));
    #endif
    InodeList = (tInodeData*) malloc((InodeListHeader.NrEntries + AddEntries) * sizeof(tInodeData));
    if(InodeList)
      memset(InodeList, '\0', (InodeListHeader.NrEntries + AddEntries) * sizeof(tInodeData));
    if (InodeList && fInodeList)
    {
      NrInodes = fread(InodeList, sizeof(tInodeData), InodeListHeader.NrEntries, fInodeList);
      if (NrInodes != InodeListHeader.NrEntries)
      {
        fclose(fInodeList);
        free(InodeList);
        WriteLogMC("HddToolsLib", "ReadListFileAlloc: Unexpected end of list file.");
        TRACEEXIT();
        return NULL;
      }
    }
    if (!InodeList)
      WriteLogMC("HddToolsLib", "ReadListFileAlloc: Not enough memory to store the list file.");
    #if STACKTRACE == TRUE
      TAP_PrintNet("END ReadListFileAlloc: OutNrInodes: %d, HeaderNrEntries: %d\n", (OutNrInodes) ? *OutNrInodes : 0, InodeListHeader.NrEntries);
    #endif
  }
  if(fInodeList) fclose(fInodeList);
  TRACEEXIT();
  return InodeList;
}

bool WriteListFile(const char *AbsListFileName, const tInodeData InodeList[], const int NrInodes)
{
  tInodeListHeader      InodeListHeader;
  FILE                 *fInodeList = NULL;
  bool                  ret = FALSE;

  TRACEENTER();

  fInodeList = fopen(AbsListFileName, "wb");
  if(fInodeList)
  {
    strncpy(InodeListHeader.Magic, "TFinos", 6);
    InodeListHeader.Version   = 1;
    InodeListHeader.NrEntries = NrInodes;
    InodeListHeader.FileSize  = (NrInodes * sizeof(tInodeData)) + sizeof(tInodeListHeader);

    if (fwrite(&InodeListHeader, sizeof(tInodeListHeader), 1, fInodeList))
    {
      if (NrInodes == 0)
        ret = TRUE;
      else if (InodeList && (fwrite(InodeList, sizeof(tInodeData), NrInodes, fInodeList) == (size_t) NrInodes))
        ret = TRUE;
    }
    fclose(fInodeList);
  }
  TRACEEXIT();
  return ret;
}

bool AddTempListToDevice(const char *AbsDeviceList, const char *AbsTempList, int *const OutMarkedFiles, int *const OutNewMarkedFiles)
{
  tInodeData           *InodeList = NULL, *TempInodeList = NULL;
  tInodeData           *curInode;
  int                   OldNrMarkedFiles = 0, NrMarkedFiles = 0, NrTempEntries = 0, NrNewMarkedFiles = 0;
  int                   i, j;
  bool                  ret = TRUE;

  TRACEENTER();
  #if STACKTRACE == TRUE
    TAP_PrintNet("AddTempListToDevice: NrMarkedFiles: %d, NrNewMarkedFiles: %d\n", (OutMarkedFiles) ? *OutMarkedFiles : 0, (OutNewMarkedFiles) ? *OutNewMarkedFiles : 0);
  #endif

  TempInodeList = ReadListFileAlloc(AbsTempList, &NrTempEntries, 0);
  if(TempInodeList)
  {  
    InodeList = ReadListFileAlloc(AbsDeviceList, &OldNrMarkedFiles, NrTempEntries);
    if(InodeList)
    {
      NrMarkedFiles = OldNrMarkedFiles;
      for (i = 0; i < NrTempEntries; i++)
      {
        curInode = &TempInodeList[i];
        for (j = 0; j < NrMarkedFiles; j++)
        {
          if (InodeList[j].InodeNr == curInode->InodeNr)
          {
            if (!curInode->FileName[0] && (curInode->di_size == InodeList[j].di_size))
              strcpy(curInode->FileName, InodeList[j].FileName);
            break;
          }
        }
        InodeList[j] = *curInode;
        if (j == NrMarkedFiles) NrMarkedFiles++;
        NrNewMarkedFiles++;
      }

      if (NrMarkedFiles > 0)
      {
        if (!WriteListFile(AbsDeviceList, InodeList, NrMarkedFiles))
        {
          WriteLogMC("HddToolsLib", "AddTempListToDevice: Error writing the inode list to file!");
          ret = FALSE;
        }
      }
      free(TempInodeList);
      free(InodeList);
    }
    else
    {
      free(TempInodeList);
      WriteLogMC("HddToolsLib", "AddTempListToDevice() E1d01.");
      ret = FALSE;
    }
  }
  else
  {
    int64_t fs = 0;
    if (HDD_GetFileSizeAndInode2(&AbsDeviceList[1], "", NULL, &fs))
      NrMarkedFiles = (fs - min(sizeof(tInodeListHeader), fs)) / sizeof(tInodeData);
    #if STACKTRACE == TRUE
      TAP_PrintNet("fs: %lld, NrMarkedFiles: %d\n", fs, NrMarkedFiles);
    #endif
  }
  #if STACKTRACE == TRUE
    TAP_PrintNet("END AddTempListToDevice: NrMarkedFiles: %d, NrNewMarkedFiles: %d\n", NrMarkedFiles, NrNewMarkedFiles);
  #endif

  if(OutMarkedFiles)    *OutMarkedFiles    = (ret) ? NrMarkedFiles    : OldNrMarkedFiles;
  if(OutNewMarkedFiles) *OutNewMarkedFiles = (ret) ? NrNewMarkedFiles : 0;

  #if STACKTRACE == TRUE
    TAP_PrintNet("END AddTempListToDevice: OutMarkedFiles: %d, OutNewMarkedFiles: %d\n", (OutMarkedFiles) ? *OutMarkedFiles : 0, (OutNewMarkedFiles) ? *OutNewMarkedFiles : 0);
  #endif
  TRACEEXIT();
  return ret;
}

// ----------------------------------------------------------------------------------------------------------

bool HDD_CheckFileSystem(const char *AbsMountPath, TProgBarHandler pRefreshProgBar, TMessageHandler pShowErrorMessage, int DoFix, bool Quick, bool InodeMonitoring, bool NoOkInfo, char *InodeNrs, char *SuccessString, char *ErrorStrFmt, char *AbortedString)
{
  TProgBarHandler       RefreshProgBar = pRefreshProgBar;
  TMessageHandler       ShowErrorMessage = pShowErrorMessage;

  TYPE_PlayInfo         PlayInfo;
  bool                  PlaybackWasRunning = FALSE, OldRepeatMode = FALSE, DeviceUnmounted = FALSE;
  char                  PlaybackName[MAX_FILE_NAME_SIZE + 1];
  char                  AbsPlaybackDir[FBLIB_DIR_SIZE];
  char                  MessageString[512];
  dword                 LastPlaybackPos = 0;

  FILE                 *fPidFile = NULL, *fLogFileIn = NULL, *fLogFileOut = NULL;
  char                  DeviceNode[20], MountPoint[FBLIB_DIR_SIZE];
  char                  CommandLine[1024], Buffer[512]; //, PidStr[13];
  char                  FirstErrorFile[50], *p = NULL;
  dword                 fsck_Pid = 0;
  unsigned long         StartTime;  byte sec = 0;
  int                   NrDefectFiles = 0, NrRepairedFiles = 0, NrMarkedFiles = 0, NrNewMarkedFiles = 0, ActivePhase = 0;
  bool                  fsck_Errors = FALSE;
  int                   i;

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

    //Extract the absolute path to the rec file
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
    if (system(CommandLine) == 0)
      DeviceUnmounted = TRUE;
    if (!DeviceUnmounted)
      WriteLogMC("HddToolsLib", "CheckFileSystem: Schreibgeschützter Remount nicht erfolgreich!");
  }

  // --- 4.) Run fsck and create a log file ---
//  if(DoFix) SetSystemTimeToCurrent();
  StartTime = TF2UnixTime(Now(&sec)) + sec;
  if (DeviceUnmounted || (DoFix != 2))
  {
    TAP_SPrint(CommandLine, sizeof(CommandLine), FSCKPATH "/jfs_fsck -v %s %s %s -L /tmp/FixInodes.tmp %s %s &> /tmp/fsck.log & echo $!", ((DoFix) ? ((DoFix==2) ? "-f" : "-n -r") : "-n"), ((Quick && DoFix!=2) ? "-q" : ""), ((Quick && InodeNrs) ? "-i" : ""), DeviceNode, ((InodeNrs) ? InodeNrs : ""));  // > /tmp/fsck.pid
//-    system(CommandLine);

    //Get the PID of the fsck-Process
//    fPidFile = fopen("/tmp/fsck.pid", "r");
    fPidFile = popen(CommandLine, "r");
    if(fPidFile)
    {
//      if (fgets(PidStr, 13, fPidFile))
//        fsck_Pid = atoi(PidStr);
      fscanf(fPidFile, "%ld", &fsck_Pid);
      pclose(fPidFile);
    }
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
    if (Quick && (DoFix != 2))
    {
      if ((i < 240) && !(i % 20))  // 12 Schritte á 2 sek
        RefreshProgBar(TRUE, 100 * i / 240);
    }
    else
    {
      if ((i < 600) && !(i % 50))  // 12 Schritte á 5 sek
        RefreshProgBar(TRUE, 100 * i / 600);
    }
    TAP_SystemProc();
    if(fsck_Cancelled && !(DoFix || Quick || InodeNrs))
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

    if (PlayInfo.playMode == PLAYMODE_Playing)
    {
      PlaybackRepeatSet(TRUE);
      if(LastPlaybackPos >= 1000)
        TAP_Hdd_ChangePlaybackPos(LastPlaybackPos);
    }
  }

  // --- 7.) Open and analyse the generated log file ---
  fsck_Cancelled = TRUE;
  fLogFileOut = fopen(ABSLOGDIR "/fsck.log", "a");

  if(fLogFileOut)
  {
    fprintf(fLogFileOut, "\r\n=========================================================\r\n");
    fprintf(fLogFileOut, "*** File system check started %s\r\n", ctime(&StartTime));
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

      else if (ActivePhase == 1)
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
          int Temp1, Temp2;
          if (sscanf(Buffer, "[MC3] %d incorrect inodes found, %d marked for correction.", &Temp1, &Temp2) == 2)
          {
            NrDefectFiles = Temp1;
            if (Temp2 != Temp1 || Temp1 != NrDefectFiles)
              fsck_Errors = TRUE;
          }
          else
            fsck_Errors = TRUE;
        }
        else
          fsck_Errors = TRUE;
      }
        
      else if (ActivePhase == 4)
      {
        if (!FirstErrorFile[0] && (strncmp(Buffer, "[MC4]", 5) == 0))  // [MC4] File system object %s%s%u is linked as: %s
        {
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
          else
            fsck_Errors = TRUE;
        }
//        else if (strncmp(Buffer, "MC corrupted inodes have been found. Would be released. Check aborted!", 70) == 0)
//          CheckAborted = TRUE;
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
  #ifdef FULLDEBUG
    system("cp /tmp/fsck.log " ABSLOGDIR "/Lastfsck.log");
  #endif

  // --- 8.) Copy the FixInodes list to root of drive ---
  if(InodeMonitoring)
  {
    TAP_SPrint(CommandLine, sizeof(CommandLine), "%s/FixInodes.lst", MountPoint);
    if (!AddTempListToDevice(CommandLine, "/tmp/FixInodes.tmp", &NrMarkedFiles, &NrNewMarkedFiles))
      fsck_Errors = TRUE;
    if (NrNewMarkedFiles != NrRepairedFiles)
      fsck_Errors = TRUE;
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

/* void DumpInodeFixingList(const char *AbsListFile)
{
  tInodeData           *InodeList = NULL;
  int                   NrInodes = 0, i;

  TRACEENTER();

  WriteLogMCf("HddToolsLib", "DUMPING inode fixing list %s:", AbsListFile);
  if (HDD_Exist2(&AbsListFile[1], ""))
  {
    InodeList = ReadListFileAlloc(AbsListFile, &NrInodes, 0);
    if(InodeList)
    {
      for (i = 0; i < NrInodes; i++)
        WriteLogMCf("HddToolsLib", "  %d: InodeNr=%lu, LastFixed=%lu, di_size=%lld, wrong=%lld, nblocks_real=%lld, FileName=%s.", i+1, InodeList[i].InodeNr, InodeList[i].LastFixTime, InodeList[i].di_size, InodeList[i].nblocks_wrong, InodeList[i].nblocks_real, InodeList[i].FileName);
      free(InodeList);
    }
  }
  else
    WriteLogMC("HddToolsLib", "-> List file not existing.");

  TRACEEXIT();
} */


// ----------------------------------------------------------------------------------------------------------

tReturnCode RunIcheckWithLog(const char *DeviceNode, const char *ParamString, char *const OutLastLine)
{
  FILE                 *LogStream;
  char                  CommandLine[1024], FullLog[512], LastLine[512], CurLine[512];
  int                   ret = -1;
  TRACEENTER();

/*  if(DoFix)
  {
    TAP_SPrint(CommandLine, sizeof(CommandLine), "mount -o remount,ro %s", DeviceNode);
    if (system(CommandLine) != 0)
      WriteLogMC("Schreibgeschützter Remount nicht erfolgreich!");
  }  */

  // Execute jfs_icheck and read its output (last line separately)
  FullLog[0] = '\0'; LastLine[0] = '\0'; CurLine[0] = '\0';
  TAP_SPrint(CommandLine, sizeof(CommandLine), FSCKPATH "/jfs_fsck icheck %s %s 2>&1", DeviceNode, ParamString);

  LogStream = popen(CommandLine, "r");
  if(LogStream)
  {
    fgets(CurLine, sizeof(CurLine), LogStream);
    CurLine[0] = '\0';
    while (fgets(CurLine, sizeof(CurLine), LogStream))
    {
      dword len = strlen(FullLog);
      if(FullLog[0] && (len < sizeof(FullLog) - 2))
      {
        FullLog[len] = '\r';
        FullLog[len+1] = '\n';
        FullLog[len+2] = '\0';
      }
//      if (strlen(FullLog) < sizeof(FullLog) - 1)
        strncpy(&FullLog[strlen(FullLog)], LastLine, sizeof(FullLog) - strlen(FullLog) - 1);
      FullLog[sizeof(FullLog) - 1] = '\0';
      RemoveEndLineBreak(CurLine);
      strcpy(LastLine, CurLine);
    }
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
  if (FullLog[0])
  {
    #ifdef FULLDEBUG
      if ((ret != rc_NOFILEFOUND) && (ret != rc_ALLFILESOKAY))
        WriteLogMC("HddToolsLib", FullLog);
    #endif
  }
  if (LastLine[0])
    WriteLogMC("HddToolsLib", LastLine);
  if (OutLastLine)
  {
    OutLastLine[0] = '\0';
    strncpy(OutLastLine, LastLine, 511);
    OutLastLine[511] = '\0';
  }

  TRACEEXIT();
  return (tReturnCode) ret;
}


bool HDD_CheckInode(const char *FileName, const char *AbsDirectory, bool DoFix, bool InodeMonitoring)
{
  char                  DeviceNode[20], ListFile[MAX_FILE_NAME_SIZE + 1];
  char                  AbsFilePath[FBLIB_DIR_SIZE];
  char                  ParamString[1024];
  tReturnCode           ret = rc_UNKNOWN;

  TRACEENTER();

  // Get the device and list file name
  HDD_FindMountPointDev2(AbsDirectory, ListFile, DeviceNode);
  strcat(ListFile, "/FixInodes.lst");
  WriteLogMCf("HddToolsLib", "Checking file inodes for wrong di_nblocks (Device=%s):", DeviceNode);

  // Set the system time to current time
//  if(DoFix) SetSystemTimeToCurrent();

  // Delete old list file (if present)
  if(InodeMonitoring && DoFix) remove("/tmp/FixInodes.tmp");

  // Execute jfs_icheck and read its output
  TAP_SPrint(AbsFilePath, sizeof(AbsFilePath), "%s/%s", AbsDirectory, FileName);
  StrReplace(AbsFilePath, "\"", "\\\"");
  TAP_SPrint(ParamString, sizeof(ParamString), "%s \"%s\"", ((DoFix) ? "-f -L /tmp/FixInodes.tmp" : ""), AbsFilePath);
  ret = RunIcheckWithLog(DeviceNode, ParamString, NULL);

  // Add the damaged inodes list to the device list
  if(InodeMonitoring && DoFix)
  {
    AddTempListToDevice(ListFile, "/tmp/FixInodes.tmp", NULL, NULL);
  }

  // Write result state to Logfile
  if (!(ret == rc_ALLFILESOKAY || ret == rc_ALLFILESFIXED || (!DoFix && ret == rc_SOMENOTFIXED)))  // NICHT: Datei ist ok, oder gefixt, oder sollte nicht gefixt werden
  {
    WriteLogMCf("HddToolsLib", "Error! jfs_icheck returned %d.", ret);
  }

  TRACEEXIT();
  return (ret == rc_ALLFILESOKAY);  // Datei ist okay
}

int HDD_CheckInodes(const char *InodeNrs, const char *AbsMountPath, bool DoFix, bool InodeMonitoring)
{
  char                  DeviceNode[20], ListFile[MAX_FILE_NAME_SIZE + 1];
  char                  ParamString[1024], LastLine[512];
  int                   NrDefectFiles = -1, ret = -1;

  TRACEENTER();
  // Get the device and list file name
  HDD_FindMountPointDev2(AbsMountPath, ListFile, DeviceNode);
  strcat(ListFile, "/FixInodes.lst");

  WriteLogMCf("HddToolsLib", "Inodes-Check mit icheck: %s (Device=%s, ListFile=%s):", InodeNrs, DeviceNode, ListFile);

  // Set the system time to current time
//  if(DoFix) SetSystemTimeToCurrent();

  // Delete old list file (if present)
  if(InodeMonitoring && DoFix) remove("/tmp/FixInodes.tmp");

  // Execute jfs_icheck and read its output (last line separately)
  TAP_SPrint(ParamString, sizeof(ParamString), "%s %s -i", ((DoFix) ? "-f -L /tmp/FixInodes.tmp" : ""), InodeNrs);
  ret = RunIcheckWithLog(DeviceNode, ParamString, LastLine);

  // Add the damaged inodes list to the device list
  if(InodeMonitoring && DoFix)
  {
    AddTempListToDevice(ListFile, "/tmp/FixInodes.tmp", NULL, &NrDefectFiles);
  }
  else
  {
    int NrFoundFiles = 0, NrOkFiles = 0;
    if (sscanf(LastLine, "%*d files given, %d found. %d of them ok", &NrFoundFiles, &NrOkFiles) == 2)
      NrDefectFiles = NrFoundFiles - NrOkFiles;
  }

  // Write result state to Logfile
  if (!(ret == rc_NOFILEFOUND || ret == rc_ALLFILESOKAY || ret == rc_ALLFILESFIXED || (!DoFix && ret == rc_SOMENOTFIXED)))  // NICHT: keine gefunden, alle ok, alle gefixt oder sollten nicht gefixt werden
  {
    WriteLogMCf("HddToolsLib", "Error! jfs_icheck returned %d.", ret);
  }

  TRACEEXIT();
  if (ret == rc_NOFILEFOUND || ret == rc_ALLFILESOKAY)  // keine gefunden oder alle okay
    return 0;
  else if (ret == rc_ALLFILESFIXED || (!DoFix && ret == rc_SOMENOTFIXED))  // alle gefixt oder sollten nicht gefixt werden
    return NrDefectFiles;
  else
    return -2;
}


bool HDD_FixInodeList2(const char *ListFile, const char *DeviceNode, bool DeleteOldEntries)
{
  char                  ParamString[512];
  tReturnCode           ret = rc_UNKNOWN;

  TRACEENTER();

  WriteLogMCf("HddToolsLib", "Checking list of suspect inodes (Device=%s, ListFile=%s):", DeviceNode, ListFile);

  // Check if ListFile exists on the drive
  if (access(ListFile, F_OK) != 0)
  {
    WriteLogMC("HddToolsLib", "-> No ListFile present.");
    TRACEEXIT();
    return TRUE;
  }

  // Set the system time to current time
//  SetSystemTimeToCurrent();

  // Execute jfs_icheck and read its output (last line separately)
  TAP_SPrint(ParamString, sizeof(ParamString), "-f %s \"%s\"", ((DeleteOldEntries) ? "-L" : "-l"), ListFile);
  ret = RunIcheckWithLog(DeviceNode, ParamString, NULL);

  // Write result state to Logfile
  if (!(ret == rc_NOFILEFOUND || ret == rc_ALLFILESOKAY || ret == rc_ALLFILESFIXED))  // NICHT: keine gefunden, alle ok oder alle gefixt
    WriteLogMCf("HddToolsLib", "Error! jfs_icheck returned %d.", ret);

  TRACEEXIT();
  return (ret == rc_NOFILEFOUND || ret == rc_ALLFILESOKAY || ret == rc_ALLFILESFIXED);  // keine gefunden, alle okay oder alle gefixt
}

bool HDD_FixInodeList(const char *AbsMountPath, bool DeleteOldEntries)
{
  char                  DeviceNode[20], ListFile[MAX_FILE_NAME_SIZE + 1];
  bool                  ret;

  TRACEENTER();

  // Get the list filename and check it
  HDD_FindMountPointDev2(AbsMountPath, ListFile, DeviceNode);
  strcat(ListFile, "/FixInodes.lst");
  ret = HDD_FixInodeList2(ListFile, DeviceNode, DeleteOldEntries);

  TRACEEXIT();
  return ret;
}


// ----------------------------------------------------------------------------------------------------------

/* bool SetSystemTimeToCurrent()
{
  unsigned long         CurTime, RealTime;
  byte                  sec = 0;
  int                   ret = 0;

  TRACEENTER();
  time(&CurTime);
  if (CurTime <= UNIXTIME2010)
  {
    RealTime = TF2UnixTime(Now(&sec)) + sec;
    ret = stime(&RealTime);
  }

  TRACEEXIT();
  return (ret == 0);
} */
