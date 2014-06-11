#ifndef __HDDTOOLSLIB__
#define __HDDTOOLSLIB__

typedef struct
{
  dword                 InodeNr;
  dword                 LastFixTime;
  int64_t               di_size;
  int64_t               nblocks_real;
  int64_t               nblocks_wrong;
  char                  FileName[64];
} tInodeData;

typedef void (*TProgBarHandler)(bool, dword);  // 0 - 100
typedef void (*TMessageHandler)(char*, char*);

bool  AddInodeToFixingList(tInodeData curInode, const char *AbsListFile);
void  DumpInodeFixingList(const char *AbsListFile);
void  HDDCheck_InitProgBar(dword pProgressStart, dword pProgressEnd, dword pProgressMax, word pRegionToSave, char *pWarningStr, char *pMoreErrorsStr, char *pCheckingStr);
bool  HDD_CheckFileSystem(const char *MountPath, TProgBarHandler RefreshProgBar, TMessageHandler ShowErrorMessage, bool DoFix, bool Quick, char *SuccessString, char *ErrorStrFmt, char *AbortedString);
void  HDD_CancelCheckFS(void);
bool  HDD_CheckInode(const char *FileName, const char *Directory, const char *DeviceNode, bool DoFix, const char *Comment);
bool  HDD_FixInodeList(const char *MountPath, bool DeleteOldEntries);

#endif
