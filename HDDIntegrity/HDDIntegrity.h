#ifndef __HDDINTEGRITYH__
#define __HDDINTEGRITYH__

#define PROGRAM_NAME          "HDDIntegrity"
#define VERSION               "V0.1"
#define TAPID                 0x8E0A4275
//#define AUTHOR                "FireBird / Christian Wünsch"
#define AUTHOR                "chris86"
#define DESCRIPTION           "Enables integrity mode on internal HDD."

#define LOGDIR                "/ProgramFiles/Settings/HDDIntegrity"
#define LNGFILENAME           PROGRAM_NAME ".lng"
#define INIFILENAME           PROGRAM_NAME ".ini"


int   TAP_Main(void);
dword TAP_EventHandler(word event, dword param1, dword param2);

#endif
