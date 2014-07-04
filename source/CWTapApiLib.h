#ifndef __CWTAPAPILIB__
#define __CWTAPAPILIB__


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

#define NRBOOKMARKS           144

void   HDD_Rename2(const char *FileName, const char *NewFileName, const char *AbsDirectory, bool RenameInfNav);
void   HDD_Delete2(const char *FileName, const char *AbsDirectory, bool DeleteInfNav);
bool   HDD_Exist2(const char *FileName, const char *AbsDirectory);
bool   HDD_GetAbsolutePathByTypeFile2(TYPE_File *File, char *OutAbsFileName);    // OutAbsFileName: mind. FBLIB_DIR_SIZE Zeichen (inkl. Nullchar)
bool   HDD_GetFileSizeAndInode2(const char *FileName, const char *AbsDirectory, __ino64_t *OutCInode, __off64_t *OutFileSize);
bool   HDD_SetFileDateTime(char const *FileName, char const *AbsDirectory, dword NewDateTime);
bool   HDD_StartPlayback2(char *FileName, char *AbsDirectory);
bool   ReadBookmarks(dword *const Bookmarks, int *const NrBookmarks);
bool   SaveBookmarks(dword Bookmarks[], int NrBookmarks);
//TYPE_RepeatMode PlaybackRepeatMode(bool ChangeMode, TYPE_RepeatMode RepeatMode, dword RepeatStartBlock, dword RepeatEndBlock);
bool   PlaybackRepeatSet(bool EnableRepeatAll);
bool   PlaybackRepeatGet();
bool   HDD_FindMountPointDev2(const char *AbsPath, char *const OutMountPoint, char *const OutDeviceNode);  // OutDeviceNode: max. 20 Zeichen, OutMountPoint: max. FILE_NAME_SIZE+1 (inkl. Nullchar)
char*  RemoveEndLineBreak (char *const Text);


//These will prevent the compiler from complaining
//We've to use the 64-bit version of lstat() else it will fail with files >= 4GB
typedef struct
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

extern int lstat64(__const char *__restrict __file, tstat64 *__restrict __buf) __THROW;

#endif
