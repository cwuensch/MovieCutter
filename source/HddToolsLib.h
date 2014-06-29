#ifndef __HDDTOOLSLIB__
#define __HDDTOOLSLIB__

#define FSCKPATH              TAPFSROOT "/ProgramFiles"
#define ABSLOGDIR             TAPFSROOT "/ProgramFiles/Settings/MovieCutter"

typedef void (*TProgBarHandler)(bool, dword);  // 0 - 100
typedef void (*TMessageHandler)(char*, char*);

bool  SetSystemTimeToCurrent();
//bool  AddInodeToFixingList(tInodeData curInode, const char *AbsListFile);
void  DumpInodeFixingList(const char *AbsListFile);
void  HDDCheck_InitProgBar(dword pProgressStart, dword pProgressEnd, dword pProgressMax, word pRegionToSave, char *pWarningStr, char *pCheckingStr);
bool  HDD_CheckFileSystem(const char *AbsMountPath, TProgBarHandler RefreshProgBar, TMessageHandler ShowErrorMessage, bool DoFix, bool Quick, bool InodeMonitoring, bool NoOkInfo, char *InodeNrs, char *SuccessString, char *ErrorStrFmt, char *AbortedString);
void  HDD_CancelCheckFS(void);
bool  HDD_CheckInode(const char *FileName, const char *AbsDirectory, bool DoFix);
int   HDD_CheckInodes(const char *InodeNrs, const char *AbsMountPath, bool DoFix);
bool  HDD_FixInodeList2(const char *ListFile, const char *DeviceNode, bool DeleteOldEntries);
bool  HDD_FixInodeList(const char *AbsMountPath, bool DeleteOldEntries);

#endif
