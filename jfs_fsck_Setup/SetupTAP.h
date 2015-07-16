#ifndef __SETUPTAPH__
#define __SETUPTAPH__

#define PROGRAM_NAME          "jfs_fsck"
#define PROGRAM_DATE          "2015-07-01 12:00:00"  //0xDF740C00
#define INSTALLDIR            "/ProgramFiles"

#define VERSION               "v1.1.15 (0.3b)"
#define TAPID                 0x8E0A4275
#define AUTHOR                "chris86"
#define DESCRIPTION           "Installs " PROGRAM_NAME " on the internal HDD."


int   TAP_Main(void);
dword TAP_EventHandler(word event, dword param1, dword param2);

#endif
