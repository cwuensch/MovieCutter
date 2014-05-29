#define                 HAVE_STDBOOL_H
#include                "jfs_includes/jfs_byteorder.h"
#include                "jfs_includes/jfs_filsys.h"
#include                "jfs_includes/jfs_dinode.h"
#include                "jfs_includes/xpeek.h"
#include                "jfs_includes/devices.h"
#include                "jfs_includes/super.h"
#include                "jfs_includes/jfs_logmgr.h"

#include                <sys/types.h>
#include                <sys/stat.h>
#include                <linux/fs.h>
#include                <fcntl.h>
#include                <unistd.h>
#include                <string.h>
#include                <dirent.h>

#include                "tap.h"
#include                "libFireBird.h"

#define PROGRAM_NAME    "jfsRepair"
#define VERSION         "V0.1"
#define LOGFILE         "jfsRepair.txt"
#define LOGROOT         "/ProgramFiles/Settings/" PROGRAM_NAME

TAP_ID                  (0x8E0A4274);
TAP_PROGRAM_NAME        (PROGRAM_NAME" "VERSION);
TAP_AUTHOR_NAME         ("FireBird");
TAP_DESCRIPTION         ("jfsRepair");
TAP_ETCINFO             (__DATE__);

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

/* Global Data for the jfs_utils*/
extern int              bsize;               /* aggregate block size         */
extern FILE            *fp;                  /* Used by libfs routines       */
extern short            l2bsize;             /* log2 of aggregate block size */
extern int64_t          AIT_2nd_offset;      /* Used by find_iag routines    */
extern int64_t          fsckwsp_offset;      /* Used by fsckcbbl routines    */
extern int64_t          jlog_super_offset;   /* Used by fsckcbbl routines    */
extern unsigned         type_jfs;

char                    LogString[1024];

void WriteLog(char *s)
{
  static bool                 FirstCall = TRUE;

  HDD_TAP_PushDir();
  if(FirstCall)
  {
    HDD_ChangeDir("/ProgramFiles");
    if(!TAP_Hdd_Exist("Settings")) TAP_Hdd_Create("Settings", ATTR_FOLDER);
    HDD_ChangeDir("Settings");
    if(!TAP_Hdd_Exist(PROGRAM_NAME)) TAP_Hdd_Create(PROGRAM_NAME, ATTR_FOLDER);
  }

  HDD_ChangeDir(LOGROOT);

  if(FirstCall)
  {
    if(TAP_Hdd_Exist(LOGFILE)) TAP_Hdd_Delete(LOGFILE);
    FirstCall = FALSE;
  }

  if(isUTFToppy()) StrMkISO(s);

  LogEntry(LOGFILE, PROGRAM_NAME, FALSE, TIMESTAMP_NONE, s);

  HDD_TAP_PopDir();
}

/**
 * 1 -> drop pagecache
 * 2 -> drop dentries and inodes
 * 3 -> drop pagecache, dentries and inodes
 */
bool drop_caches()
{
  FILE *fp = fopen("/proc/sys/vm/drop_caches", "w");

  /* we are syncing only our wanted disk here... */
  sync();
  sync();

  /* open proc file */
  if(!fp)
  {
    TAP_PrintNet("fopen /proc/sys/vm/drop_caches failed, can't flush disk!");
    return FALSE;
  }

  /* drop all caches */
  if(fprintf(fp, "3\n") == -1)
  {
    TAP_PrintNet("echo 3 > /proc/sys/vm/drop_caches failed!");
    fclose(fp);
    return FALSE;
  }

  fclose(fp);
  return TRUE;
}

bool jfs_OpenDisk(char *Device)
{
  //Copied from xpeek.c
  struct superblock     sb;

  /* Open device */
  fp = fopen(Device, "r+");
  if (fp == NULL)
  {
    TAP_PrintNet("jfs_debugfs: Cannot open device %s.\n", Device);
    return FALSE;
  }

  /* Get block size information from the superblock       */
  if (ujfs_get_superblk(fp, &sb, 1))
  {
    TAP_PrintNet("jfs_debugfs: error reading primary superblock\n", stderr);
    if (ujfs_get_superblk(fp, &sb, 0))
    {
      TAP_PrintNet("jfs_debugfs: error reading secondary superblock\n", stderr);
      ujfs_flush_dev(fp);
      fclose(fp);
      return FALSE;
    }
    else
      TAP_PrintNet("jfs_debugfs: using secondary superblock\n", stderr);
  }

  type_jfs = sb.s_flag;

  bsize = sb.s_bsize;
  l2bsize = sb.s_l2bsize;
  AIT_2nd_offset = addressPXD(&(sb.s_ait2)) * bsize;
  fsckwsp_offset = addressPXD(&(sb.s_fsckpxd)) << l2bsize;
  jlog_super_offset = (addressPXD(&(sb.s_logpxd)) << l2bsize) + LOGPSIZE;

  TAP_PrintNet("jfs_debugfs: Aggregate Block Size: %d\n\n", bsize);

  drop_caches();

  return TRUE;
}

void jfs_CloseDisk(void)
{
  if(fp)
  {
    ujfs_flush_dev(fp);
    fclose(fp);
    fp = NULL;

    drop_caches();
  }
}

bool jfs_Tino(char *FullPath, tstat64 *Stat, char *Log, int64_t *FIBBlocks)
{
  //copied from jfs_icheck.c
  int64_t       total_blks, blk;
  int64_t       RealBlocks;
  int           fd;
  dword         blknum;

  #define SOME_MORE_BLOCKS 1024 * 4

  if(FIBBlocks) *FIBBlocks = 0;

  fd = open64(FullPath, O_RDONLY);
  if(fd >= 0)
  {
    fsync(fd);

    total_blks = (Stat->st_size + Stat->st_blksize - 1) / Stat->st_blksize + SOME_MORE_BLOCKS;
    for(RealBlocks = 0, blk = 0; blk < total_blks; blk++)
    {
      blknum = blk;  /* FIBMAP ist nur 32bit ... */
      if(ioctl(fd, FIBMAP, &blknum) == -1)
      {
        strcat(Log, "ioctl(FIBMAP) failed\t");
        close(fd);
        return FALSE;
      }
      if (blknum != 0) RealBlocks++;
    }
    if(FIBBlocks) *FIBBlocks = RealBlocks;
    TAP_SPrint(&Log[strlen(Log)], "%8lld\t", RealBlocks);
    return TRUE;
  }

  strcat(Log, "File not found\t");

  return FALSE;
}

void RecursiveScan(char *RootDir)
{
  DIR                  *dirp;
  struct dirent        *dp;
  tstat64               Stat;
  char                  FullPath[512];

  if(RootDir && *RootDir)
  {
    dirp = opendir(RootDir);
    if(dirp)
    {
      while((dp = readdir(dirp)) != NULL)
      {
        strcpy(FullPath, RootDir);
        if(FullPath[strlen(FullPath) - 1] != '/') strcat(FullPath, "/");
        strcat(FullPath, dp->d_name);

        TAP_PrintNet("%s\n", FullPath);

        if((lstat64(FullPath, &Stat) == 0) && S_ISDIR(Stat.st_mode))
        {
          //Ignore . and ..
          if(strcmp(dp->d_name, ".") && strcmp(dp->d_name, ".."))
          {
            strcat(FullPath, "/");
            RecursiveScan(FullPath);
          }
        }
        else
        {
          //copied from dirctory.c
          int64_t               inode_address, FIBBlocks;
          unsigned              which_table = FILESYSTEM_I;
          struct dinode         inode;

          if(find_inode(Stat.st_ino, which_table, &inode_address) || xRead(inode_address, sizeof (struct dinode), (char *) &inode))
          {
            TAP_SPrint(LogString, "%lld\terror reading inode\terror reading inode\terror reading inode\t", Stat.st_ino);
          }
          else
          {
            if((inode.di_mode & IFMT) == IFDIR)
            {
              TAP_SPrint(LogString, "%lld\tdir\tdir\tdir\t", Stat.st_ino);
            }

            bool OK = FALSE;

            if((inode.di_mode & IFMT) == IFREG)
            {
              //Expected blocks = ceil(size / 4096)
              int64_t ExpBlocks = inode.di_size >> 12;
              if(inode.di_size & 0xfff) ExpBlocks++;
              TAP_SPrint(LogString, "%lld\t%lld\t%lld\t%lld\t", Stat.st_ino, inode.di_size, inode.di_nblocks, ExpBlocks);

              OK = ExpBlocks == inode.di_nblocks;
            }

            if(jfs_Tino(FullPath, &Stat, LogString, &FIBBlocks) && (FIBBlocks != inode.di_nblocks)) OK = FALSE;

            if(OK)
              strcat(LogString, "ok\t");
            else
              strcat(LogString, "NOK\t");

            strcat(LogString, FullPath);
          }
          WriteLog(LogString);
        }
      }
      closedir(dirp);
    }
  }
}

dword TAP_EventHandler(word event, dword param1, dword param2)
{
  (void) event;
  (void) param1;
  (void) param2;

  return param1;
}

int TAP_Main(void)
{
  KeyTranslate(TRUE, &TAP_EventHandler);

  TAP_SPrint(LogString, "%s", PROGRAM_NAME "  " VERSION);
  WriteLog(LogString);
  WriteLog("inode\tsize\tinode blks\tcalc blks\tFIBMAP blks\tResult\tFileName\tComments");

  if(jfs_OpenDisk("/dev/sda2"))
  {
    RecursiveScan("/mnt/hd/DataFiles");
    jfs_CloseDisk();
  }

  return 0;
}
