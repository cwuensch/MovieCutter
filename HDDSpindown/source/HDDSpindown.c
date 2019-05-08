#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define __USE_LARGEFILE64  1
#define _FILE_OFFSET_BITS  64
#ifdef _MSC_VER
  #define __const const
#endif
//#undef malloc
//#undef free

//#define __ALTEFBLIB__

//#define  STACKTRACE     TRUE
#define _GNU_SOURCE
#include                <linux/types.h>
#include                <unistd.h>
#include                <stdint.h>
#include                <fcntl.h>
#include                <stdio.h>
#include                <errno.h>
#include                <string.h>
#include                "sgio.h"

#include                <tap.h>
#include                <libFireBird.h>
#include                "HDDSpindown.h"

TAP_ID                  (TAPID);
TAP_PROGRAM_NAME        (PROGRAM_NAME" "VERSION);
TAP_PROGRAM_VERSION     (VERSION);
TAP_AUTHOR_NAME         (AUTHOR);
TAP_DESCRIPTION         (DESCRIPTION);
TAP_ETCINFO             (__DATE__);

typedef enum
{
  DC_SetStandbyTo,
  DC_SetStandbyNow,
  DC_None
} tDriveCommand;

typedef enum
{
  ST_None,
  ST_Init,
  ST_Menu,
  ST_Finished
} tState;

typedef enum {
  MI_SelectDevice,
  MI_SetStandbyTo,
  MI_SetStandbyNow,
//  MI_Dummy,
  MI_Save,
  MI_NrMenuItems
} tMenuItem;

typedef enum
{
  LS_MenuTitle,
  LS_SelectDevice,
  LS_SetStandbyTo,
  LS_SetStandbyNow,
  LS_Save,
  LS_Exit,
  LS_NrStrings
} tLngStrings;

char* DefaultStrings[LS_NrStrings] =
{
  "Standby-Modus der Festplatte",
  "Laufwerk wählen:",
  "Standby aktivieren nach:",
  "Standby jetzt aktivieren",
  "Übernehmen",
  "Beenden"
};

#define LangGetString(x)  LangGetStringDefault(x, DefaultStrings[x])


/* APT Functions */
int apt_detect (int fd, int verbose);
int apt_is_apt (void);

static void  interpret_standby (int standby, char *output);
static int   process_dev (char *devname, tDriveCommand command, int standby);
static void  CreateSettingsDir(void);
static bool  LoadINI(void);
static void  SaveINI(void);
static void  ShowErrorMessage(char *MessageStr, char *TitleStr);


// Globale Variablen
tState                  State = ST_None;
char                    Device[FBLIB_DIR_SIZE];
byte                    StandbyTime = 0;
int                     verbose = 1, prefer_ata12 = 0;


// ============================================================================
//                              IMPLEMENTIERUNG
// ============================================================================
int TAP_Main(void)
{
  #if STACKTRACE == TRUE
    CallTraceInit();
    CallTraceEnable(TRUE);
  #endif
  TRACEENTER();
  
  CreateSettingsDir();
  LoadINI();
  if (GetUptime() <= 6000)   // während der ersten Minute nicht das OSD starten, wenn eine ini existiert
  {
    if (StandbyTime != 0)
      process_dev(Device, DC_SetStandbyTo, StandbyTime);
    TRACEEXIT();
    return 0;
  }

  State = ST_Init;
  TRACEEXIT();
  return 1;
}

dword TAP_EventHandler(word event, dword param1, dword param2)
{
  static bool           DoNotReenter = FALSE;
  TRACEENTER();

  // Behandlung offener MessageBoxen (rekursiver Aufruf, auch bei DoNotReenter)
/*  if(OSDMenuMessageBoxIsVisible() || OSDMenuInfoBoxIsVisible())
  {
    if(OSDMenuMessageBoxIsVisible())
    {
      #ifdef __ALTEFBLIB__
        OSDMenuMessageBoxDoScrollOver(&event, &param1);
      #endif
    }
    OSDMessageEvent(&event, &param1, &param2);
    param1 = 0;
  } */

  if (!DoNotReenter)
  {
    DoNotReenter = TRUE;
    switch (State)
    {
      case ST_Init:
      {
        // Load Language Strings
        char standby_txt[128];
        if (TAP_GetSystemVar(SYSVAR_OsdLan) != LAN_German)
          LangLoadStrings(TAPFSROOT LOGDIR "/" LNGFILENAME, LS_NrStrings, LAN_English);
        
        // Initialize Main Menu
        OSDMenuInitialize(FALSE, TRUE, FALSE, TRUE, LangGetString(LS_MenuTitle), NULL);
        interpret_standby (StandbyTime, standby_txt);
        OSDMenuItemAdd(LangGetString(LS_SelectDevice), Device, NULL, NULL, TRUE, TRUE, MI_SelectDevice);
        OSDMenuItemAdd(LangGetString(LS_SetStandbyTo), standby_txt, NULL, NULL, TRUE, TRUE, MI_SetStandbyTo);
        OSDMenuItemAdd(LangGetString(LS_SetStandbyNow), NULL, NULL, NULL, TRUE, FALSE, MI_SetStandbyNow);
//        OSDMenuItemAdd(" ", NULL, NULL, NULL, FALSE, FALSE, MI_Dummy);
        OSDMenuItemAdd(LangGetString(LS_Save), NULL, NULL, NULL, TRUE, FALSE, MI_Save);

        OSDMenuButtonAdd(1, BI_Green, NULL, LangGetString(LS_Save));
        OSDMenuButtonAdd(1, BI_Exit, NULL, LangGetString(LS_Exit));
        OSDMenuUpdate(FALSE);

        State = ST_Menu;
        break;
      }

      case ST_Menu:
      {
        OSDMenuEvent(&event, &param1, &param2);
        if ((event == EVT_KEY) && (param1 == RKEY_Ok || param1 == RKEY_Left || param1 == RKEY_Right))
        {
          int curItem = OSDMenuGetCurrentItem();
          switch (curItem)
          {
            case MI_SelectDevice:
            {
              struct stat statbuf;
              char hString[FBLIB_DIR_SIZE];

              if ((param1 == RKEY_Left) && (Device[7] > 'a'))   Device[7]--;
              else if (param1 == RKEY_Right)                    Device[7]++;

              TAP_SPrint(hString, sizeof(hString), "/sys/block/%s", &Device[5]);
              if (stat(hString, &statbuf) != 0)
                Device[7] = 'a';
              OSDMenuItemModifyValue(curItem, Device);
              break;
            }

            case MI_SetStandbyTo:
            {
              char standby_txt[128];
              if (param1 == RKEY_Left)        StandbyTime--;
              else if (param1 == RKEY_Right)  StandbyTime++;
              interpret_standby (StandbyTime, standby_txt);
              OSDMenuItemModifyValue(curItem, standby_txt);
              break;
            }

            case MI_SetStandbyNow:
              if (param1 == RKEY_Ok)
              {
                process_dev(Device, DC_SetStandbyNow, 0);
                State = ST_Finished;
              }
              break;

            case MI_Save:
              if (param1 == RKEY_Ok)
              {
                process_dev(Device, DC_SetStandbyTo, StandbyTime);
                SaveINI();
                State = ST_Finished;
              }
              break;
          }

          OSDMenuUpdate(FALSE);
          param1 = 0;
        }
        else if ((event == EVT_KEY) && (param1 == RKEY_Green || param1 == RKEY_Exit))
        {
          if (param1 == RKEY_Green)
          {
            process_dev(Device, DC_SetStandbyTo, StandbyTime);
            SaveINI();
          }
          State = ST_Finished;
          param1 = 0;
        }
        break;
      }

      case ST_Finished:
      {
        OSDMenuDestroy();
        TAP_Exit();
      }

      default:
        break;
    }

    if ((event == EVT_KEY) && (param1 == RKEY_Sleep))
    {
      process_dev(Device, DC_SetStandbyNow, 0);
      State = ST_Finished;
      param1 = 0;
    }

    DoNotReenter = FALSE;
  }

  TRACEEXIT();
  return param1;
}


// ----------------------------------------------------------------------------
//                           Hauptfunktionen
// ----------------------------------------------------------------------------

static void interpret_standby (int standby, char* output)
{
  switch(standby) {
    case 0:    
      strcpy(output, "off");
      break;
    case 252:
      strcpy(output, "21 min");
      break;
    case 253:
      strcpy(output, "vendor-specific");
      break;
    case 254:
      strcpy(output, "?reserved");
      break;
    case 255:
      strcpy(output, "21 min + 15 sec");
      break;
    default:
      if (standby <= 240)
      {
        unsigned int secs = standby * 5;
        unsigned int mins = secs / 60;
        secs %= 60;
        if (mins && secs) sprintf(output, "%u min + %u sec", mins, secs);
        else if (mins)    sprintf(output, "%u min", mins);
        else if (secs)    sprintf(output, "%u sec", secs);
      }
      else if (standby <= 251)
      {
        unsigned int mins = (standby - 240) * 30;
        unsigned int hrs  = mins / 60;
        mins %= 60;
        if (hrs && mins)  sprintf(output, "%u h + %u min", hrs, mins);
        else if (hrs)     sprintf(output, "%u h", hrs);
        else if (mins)    sprintf(output, "%u min", mins);
      }
      else
        strcpy(output, "illegal value");
      break;
  }
}

static int process_dev (char *devname, tDriveCommand command, int standby)
{
  int fd;
  int err = 0;

  fd = open(devname, O_RDONLY|O_NONBLOCK);
  if (fd < 0) {
    err = errno;
    perror(devname);
    return err;
  }
  printf("\n%s:\n", devname);

  if (apt_detect(fd, TRUE) == -1) {
    err = errno;
    perror(devname);
    close(fd);
    return err;
  }

  if (command == DC_SetStandbyTo)
  {
    uint8_t args[4] = {ATA_OP_SETIDLE, standby, 0, 0};
//    if (command == DC_SetStandbyVerbose)
    {
      char standby_txt[128];
      interpret_standby(standby, standby_txt);
      printf(" setting standby to %u (%s)\n", standby, standby_txt);
    }
    if (do_drive_cmd(fd, args, 0)) {
      err = errno;
      perror(" HDIO_DRIVE_CMD(setidle) failed");
    }
  }

  else if (command == DC_SetStandbyNow)
  {
    uint8_t args1[4] = {ATA_OP_STANDBYNOW1, 0, 0, 0};
    uint8_t args2[4] = {ATA_OP_STANDBYNOW2, 0, 0, 0};

    sync();
    sync();
    TAP_Sleep(1);

//    if (get_standbynow)
      printf(" issuing standby command\n");
    if (do_drive_cmd(fd, args1, 0) || do_drive_cmd(fd, args2, 0)) {
      err = errno;
      perror(" HDIO_DRIVE_CMD(standby) failed");
    }
  }
  close (fd);
  return err;
}


// ----------------------------------------------------------------------------
//                           MessageBox-Funktionen
// ----------------------------------------------------------------------------

// Die Funktionen zeigt einen Informationsdialog (OK) an, und wartet auf die Bestätigung des Benutzers.
static void ShowErrorMessage(char *MessageStr, char *TitleStr)
{
  dword OldSysState, OldSysSubState;

  TRACEENTER();
  HDD_TAP_PushDir();
  TAP_GetState(&OldSysState, &OldSysSubState);

  OSDMenuMessageBoxInitialize((TitleStr) ? TitleStr : PROGRAM_NAME, MessageStr);
  OSDMenuMessageBoxDoNotEnterNormalMode(TRUE);
  OSDMenuMessageBoxButtonAdd("OK");
  OSDMenuMessageBoxShow();
//  CSShowMessageBox = TRUE;
  while (OSDMenuMessageBoxIsVisible())
  {
    TAP_SystemProc();
    TAP_Sleep(1);
  }

  TAP_Osd_Sync();
  if(OldSysSubState != 0) TAP_EnterNormalNoInfo();

  HDD_TAP_PopDir();
  TRACEEXIT();
}

// ----------------------------------------------------------------------------
//                             INI-Funktionen
// ----------------------------------------------------------------------------
void CreateSettingsDir(void)
{
  TRACEENTER();

//  struct stat64 st;
//  if (lstat64(TAPFSROOT "/ProgramFiles/Settings", &st) == -1)
    mkdir(TAPFSROOT "/ProgramFiles/Settings", 0666);
//  if (lstat64(TAPFSROOT "/ProgramFiles/Settings/HDDSpindown", &st) == -1)
    mkdir(TAPFSROOT "/ProgramFiles/Settings/HDDSpindown", 0666);

  TRACEEXIT();
}

bool LoadINI(void)
{
  FILE                 *f = NULL;
  char                 *Buffer = NULL;
  size_t                BufSize = 0;
  char                 *c = NULL;
  int                   p;
  char                  Name[50];
  long                  Value = 0;
  bool                  ret = FALSE;

  TRACEENTER();
  strcpy(Device, "/dev/sda");

  if ((f = fopen(TAPFSROOT LOGDIR "/" INIFILENAME, "rb")) != NULL)
  {
    while (getline(&Buffer, &BufSize, f) >= 0)
    {
      //Interpret the following characters as remarks: //
      c = strstr(Buffer, "//");
      if(c) *c = '\0';

      // Remove line breaks in the end
      p = strlen(Buffer);
      while (p && (Buffer[p-1] == '\r' || Buffer[p-1] == '\n' || Buffer[p-1] == ';'))
        Buffer[--p] = '\0';

      if (strncmp(Buffer, "Device", 6) == 0)
      {
        c = strchr(&Buffer[5], '=');
        if(c)
        {
          do c++; while (*c == ' ');
          strncpy(Device, c, sizeof(Device));
        }
      }
      else if (sscanf(Buffer, "%49[^= ] = %lu", Name, &Value) == 2)
      {
        if (strncmp(Name, "StandbyTime", 11) == 0)   StandbyTime = Value;
      }
    }
    fclose(f);
    ret = TRUE;
  }
//  else
//    SaveINI();
  
  TRACEEXIT();
  return ret;
}

void SaveINI(void)
{
  FILE *f = NULL;
  TRACEENTER();

  if ((f = fopen(TAPFSROOT LOGDIR "/" INIFILENAME, "wb")) != NULL)
  {
    fprintf(f, "[HDDSpindown]\r\n");
    fprintf(f, "%s=%s\r\n",    "Device",            Device);
    fprintf(f, "%s=%hhu\r\n",  "StandbyTime",       StandbyTime);
    fclose(f);
//    HDD_SetFileDateTime(INIFILENAME, TAPFSROOT LOGDIR, 0);
  }
  TRACEEXIT();
}
