#ifndef __CWTAPAPILIB__
#define __CWTAPAPILIB__

#include                "RecHeader.h"

// ============================================================================
//                               TAP-API-Lib
// ============================================================================
//#undef memcpy
//#undef memcmp
//#undef memset
#undef sprintf
//#define TAP_MemCpy    memcpy
//#define TAP_MemCmp    memcmp
//#define TAP_MemSet    memset
#define TAP_SPrint    snprintf

#define LOGDIR          "/ProgramFiles/Settings/MovieCutter"
#define LOGFILENAME     "MovieCutter.log"
#define NRBOOKMARKS     177   // eigentlich werden nur 48 Bookmarks unterstützt!! (SRP2401)


//tPVRTime   Unix2TFTimeSec(time_t UnixTimeStamp, byte *const outSec);
time_t     TF2UnixTimeSec(tPVRTime TFTimeStamp, byte TFTimeSec);
tPVRTime   TFNow(byte *const outSec);
tPVRTime   AddTimeSec(tPVRTime pvrTime, byte pvrTimeSec, byte *const outSec, int addSeconds);
int        TimeDiffSec(tPVRTime FromTime, byte FromTimeSec, tPVRTime ToTime, byte ToTimeSec);
void       GetFileNameFromRec(const char *RecFileName, const char *AbsDirectory, const char *NewExt, char *const OutCutFileName);
bool       GetRecInfosFromInf(const char *RecFileName, const char *AbsDirectory, bool *const isCrypted, bool *const isHDVideo, bool *const isStripped, tPVRTime *const DateTime, byte *const DateSec);
//bool       SaveRecDateToInf(const char* RecFileName, char const *AbsDirectory, tPVRTime NewTime, byte NewSec);
void       HDD_Rename2(const char *FileName, const char *NewFileName, const char *AbsDirectory, bool RenameInfNav, bool RenameCutSRT);
void       HDD_Delete2(const char *FileName, const char *AbsDirectory, bool DeleteInfNavCut, bool DeleteSRT);
bool       HDD_Exist2(const char *FileName, const char *AbsDirectory);
bool       HDD_TruncateFile(const char *FileName, const char *AbsDirectory, off_t NewFileSize);
bool       HDD_GetAbsolutePathByTypeFile2(TYPE_File *File, char *OutAbsFileName);    // OutAbsFileName: mind. FBLIB_DIR_SIZE Zeichen (inkl. Nullchar)
bool       HDD_GetFileSizeAndInode2(const char *FileName, const char *AbsDirectory, __ino64_t *OutCInode, __off64_t *OutFileSize);
//bool       HDD_GetFileDateTime(char const *FileName, char const *AbsDirectory, tPVRTime *const OutDateTime, byte *const OutDateSec);
bool       HDD_SetFileDateTime(char const *FileName, char const *AbsDirectory, tPVRTime NewDateTime, byte NewDateSec);
__off64_t  HDD_GetFreeDiscSpace(char *AnyFileName, char *AbsDirectory);
bool       HDD_TAP_CheckCollisionByID(dword MyTapID);
void       HDPlusRecordingHook(bool Activate);
bool       HDD_StartPlayback2(char *FileName, char *AbsDirectory, bool MediaFileMode);
bool       ReadBookmarks(dword *const Bookmarks, int *const NrBookmarks);
bool       SaveBookmarks(dword Bookmarks[], int NrBookmarks, bool OverwriteAll);
//TYPE_RepeatMode PlaybackRepeatMode(bool ChangeMode, TYPE_RepeatMode RepeatMode, dword RepeatStartBlock, dword RepeatEndBlock);
bool       PlaybackRepeatSet(bool EnableRepeatAll);
bool       PlaybackRepeatGet();
bool       HDD_FindMountPointDev2(const char *AbsPath, char *const OutMountPoint, char *const OutDeviceNode);  // OutMountPoint und OutDeviceNode: max. FBLIB_DIR_SIZE (inkl. Nullchar)
char*      RemoveEndLineBreak (char *const Text);
char       SysTypeToStr(void);
bool       ConvertUTFStr(char *DestStr, char *SourceStr, int MaxLen, bool ToUnicode);
void       WriteLogMC(char *ProgramName, char *s);
void       WriteLogMCf(char *ProgramName, const char *format, ...) __attribute__ ((format(__printf__, 2, 3)));
//void       WriteDebugLog(const char *format, ...) __attribute__ ((format(__printf__, 1, 2)));
void       CloseLogMC();
//bool       infData_Get2(const char *RecFileName, const char *AbsDirectory, const char *NameTag, dword *const PayloadSize, byte **Payload);
//bool       infData_Set2(const char *RecFileName, const char *AbsDirectory, const char *NameTag, dword PayloadSize, byte Payload[]);
//bool       infData_Delete2(const char *RecFileName, const char *AbsDirectory, const char *NameTag);


//These will prevent the compiler from complaining
//We've to use the 64-bit version of lstat() else it will fail with files >= 4GB
/*typedef struct
{
  __dev_t               st_dev;
  long int              st_pad1[2];
  __ino64_t             st_ino;
  __mode_t              st_mode;
  __nlink_t             st_nlink;
  __uid_t               st_uid;
  __gid_t               st_gid;
  __dev_t               st_rdev;
  long int              st_pad2[2];
  __off64_t             st_size;
  __time_t              st_atime;
  long int              __reserved0;
  __time_t              st_mtime;
  long int              __reserved1;
  __time_t              st_ctime;
  long int              __reserved2;
  __blksize_t           st_blksize;
  long int              st_pad4;
  __blkcnt64_t          st_blocks;
  long int              st_pad5[14];
} tstat64;

typedef struct
{
  unsigned long int     f_bsize;
  unsigned long int     f_frsize;
  __fsblkcnt64_t        f_blocks;
  __fsblkcnt64_t        f_bfree;
  __fsblkcnt64_t        f_bavail;
  __fsfilcnt64_t        f_files;
  __fsfilcnt64_t        f_ffree;
  __fsfilcnt64_t        f_favail;
  unsigned long int     f_fsid;
  int                   __f_unused;
  unsigned long int     f_flag;
  unsigned long int     f_namemax;
  int                   __f_spare[6];
} tstatvfs64;

extern int lstat64(__const char *__restrict __file, tstat64 *__restrict __buf) __THROW;
extern int statvfs64 (__const char *__restrict __file, tstatvfs64 *__restrict __buf) __THROW;
*/
#endif
