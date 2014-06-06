#define                 HAVE_STDBOOL_H

#include                <string.h>
#include                "tap.h"
#include                "libFireBird.h"

#define PROGRAM_NAME    "jfs_fsck_TAP"
#define VERSION         "V0.1"
#define LOGFILE         "jfsRepair.log"
#define LOGROOT         "/ProgramFiles/Settings/jfsRepair"

TAP_ID                  (0x8E0A4275);
TAP_PROGRAM_NAME        (PROGRAM_NAME" "VERSION);
TAP_AUTHOR_NAME         ("FireBird");
TAP_DESCRIPTION         ("jfs_fsck_TAP");
TAP_ETCINFO             (__DATE__);

#include                "jfs_fsck/xchkdsk.h"

char                    LogString[1024];

void WriteLogX(char *s)
{
  static bool                 FirstCall = TRUE;

  HDD_TAP_PushDir();
  if(FirstCall)
  {
    HDD_ChangeDir("/ProgramFiles");
    if(!TAP_Hdd_Exist("Settings")) TAP_Hdd_Create("Settings", ATTR_FOLDER);
    HDD_ChangeDir("Settings");
    if(!TAP_Hdd_Exist("jfsRepair")) TAP_Hdd_Create("jfsRepair", ATTR_FOLDER);

    FirstCall = FALSE;
  }

  HDD_ChangeDir(LOGROOT);

  if(isUTFToppy()) StrMkISO(s);

  LogEntry(LOGFILE, PROGRAM_NAME, TRUE, TIMESTAMP_NONE, s);

  HDD_TAP_PopDir();
}

void DoCheck(dword inode, bool FixIt)
{
  int xchkdsk_main(int argc, char **argv);

  char      param[20][80];
  char     *pparam[20];
  int       NrParam;

  strcpy(param[0], "jfs_fsck_TAP");
  pparam[0] = param[0];

  if(FixIt)
  {
    strcpy(param[1], "-f");
  }
  else
  {
    strcpy(param[1], "-n");
  }
  pparam[1] = param[1];

  strcpy(param[2], "-v");
  pparam[2] = param[2];

  if(inode != 0)
  {
    TAP_SPrint(param[3], "-i %d", inode);
    pparam[3] = param[3];

    strcpy(param[4], "/dev/sda2");
    pparam[4] = param[4];

    NrParam = 5;
  }
  else
  {
    strcpy(param[3], "/dev/sda2");
    pparam[3] = param[3];

    NrParam = 4;
  }

  xchkdsk_main(NrParam, pparam);
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

  WriteLogX("--------------------------------------------------------------------------");
  TAP_SPrint(LogString, "%s", PROGRAM_NAME "  " VERSION);
  WriteLogX(LogString);

  DoCheck(442547, FALSE);

  return 0;
}
