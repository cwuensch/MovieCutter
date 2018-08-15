#ifndef __HDDSPINDOWNH__
#define __HDDSPINDOWNH__
#undef sprintf
#define TAP_SPrint    snprintf

#define PROGRAM_NAME          "HDDSpindown"
#define VERSION               "0.2"
#define TAPID                 0x2A0A0006
#define AUTHOR                "chris86"
#define DESCRIPTION           "Activates/changes hard disk standby mode."

#define LOGDIR                "/ProgramFiles/Settings/" PROGRAM_NAME
#define INIFILENAME           PROGRAM_NAME ".ini"
#define LNGFILENAME           PROGRAM_NAME ".lng"

int   TAP_Main(void);
dword TAP_EventHandler(word event, dword param1, dword param2);

#endif
