#ifndef __STANDBYSWITCHH__
#define __STANDBYSWITCHH__
#undef sprintf
#define TAP_SPrint    snprintf

#define PROGRAM_NAME          "StandbySwitch"
#define VERSION               "0.1"
#define TAPID                 0x2A0A0005
#define AUTHOR                "chris86"
#define DESCRIPTION           "De-/activation of the passive standby mode."

#define LOGDIR                "/ProgramFiles/Settings/" PROGRAM_NAME
#define LNGFILENAME           PROGRAM_NAME ".lng"

typedef union {
  byte                ModeByte;
  struct
  {
    byte              VFD:1;
    byte              Tuner:1;
    byte              Scart:1;
    byte              Active2:1;
    byte              Zeros:3;
    byte              Active1:1;
  };
} tStandbyMode;

int   TAP_Main(void);
dword TAP_EventHandler(word event, dword param1, dword param2);

#endif
