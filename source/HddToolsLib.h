#ifndef __HDDTOOLSLIB__
#define __HDDTOOLSLIB__

#define FSCKPATH              TAPFSROOT "/ProgramFiles"
#define ABSLOGDIR             TAPFSROOT "/ProgramFiles/Settings/MovieCutter"
#define NRSEGMENTMARKER       101            // max. number of file markers +1 (marker for the end of file)

typedef void (*TProgBarHandler)(bool, dword);   // bool ShowProgBar, dword CurrentValue (0-100)
typedef void (*TMessageHandler)(char*, char*);  // char *MessageStr, char *TitleStr

//bool  SetSystemTimeToCurrent();
//bool  AddInodeToFixingList(tInodeData curInode, const char *AbsListFile);
void  DumpInodeFixingList(const char *AbsListFile);
void  HDDCheck_InitProgBar(dword pProgressStart, dword pProgressEnd, dword pProgressMax, word pRegionToSave, char *pWarningStr, char *pCheckingStr);
bool  HDD_CheckFileSystem(const char *AbsMountPath, TProgBarHandler pRefreshProgBar, TMessageHandler pShowErrorMessage, int DoFix, bool Quick, bool InodeMonitoring, bool NoOkInfo, const char *InodeNrs, char *SuccessString, char *ErrorStrFmt, char *AbortedString);
void  HDD_CancelCheckFS(void);
bool  HDD_CheckInode(const char *FileName, const char *AbsDirectory, bool DoFix, bool InodeMonitoring);
int   HDD_CheckInodes(const char *InodeNrs, const char *AbsMountPath, bool DoFix, bool InodeMonitoring);
bool  HDD_FixInodeList(const char *AbsMountPath, bool DeleteOldEntries);

#endif
