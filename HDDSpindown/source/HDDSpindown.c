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
  ST_Init,
  ST_Menu,
  ST_Finished
} tState;

typedef enum {
  MI_SetStandbyTo,
  MI_SetStandbyNow,
//  MI_Dummy,
  MI_Save,
  MI_NrMenuItems
} tMenuItem;

typedef enum
{
  LS_MenuTitle,
  LS_StandbyMode,
  LS_Scart,
  LS_Tuner,
  LS_VFD,
  LS_Save,
  LS_Exit,
  LS_active,
  LS_passive,
  LS_on,
  LS_off,
  LS_ErrorReading,
  LS_NrStrings
} tLngStrings;

char* DefaultStrings[LS_NrStrings] =
{
  "Standby-Verhalten einstellen",
  "Standby-Modus:",
  "SCART durchschleifen",
  "Tuner durchschleifen",
  "Display leuchtet",
  "Übernehmen",
  "Beenden",
  "Aktiv",
  "Passiv",
  "Ein",
  "Aus",
  "Fehler beim Lesen des Standby-Modus.\n\nUnerwarteter Wert im EEPROM gefunden."
};

#define LangGetString(x)  LangGetStringDefault(x, DefaultStrings[x])


/* APT Functions */
int apt_detect (int fd, int verbose);
int apt_is_apt (void);

static void  interpret_standby (int standby, char *output);
static int   process_dev (char *devname, tDriveCommand command, int standby);
static void  ShowErrorMessage(char *MessageStr, char *TitleStr);


// Globale Variablen
tState                  State = ST_Init;
byte                    StandbyTime = 0;
int                     verbose = 1, prefer_ata12 = 0;


// ============================================================================
//                              IMPLEMENTIERUNG
// ============================================================================
int TAP_Main(void)
{
  TRACEENTER();

  State = ST_Init;
/*  {
    OSDMenuInfoBoxShow(PROGRAM_NAME " " VERSION, LangGetString(LS_ErrorReading), 500);
    do
    {
      OSDMenuEvent(NULL, NULL, NULL);
    } while(OSDMenuInfoBoxIsVisible());
    TRACEEXIT();
    return 0;
  } */

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
    OSDMenuEvent(&event, &param1, &param2);
    param1 = 0;
  } */

  if (!DoNotReenter)
  {
    DoNotReenter = TRUE;
    switch (State)
    {
      case ST_Init:
      {
//        char* hString = (char*)TAP_MemAlloc(256);

        OSDMenuInitialize(FALSE, TRUE, FALSE, TRUE, LangGetString(LS_MenuTitle), NULL);
        OSDMenuItemAdd("Set Standby to:", "off", NULL, NULL, TRUE, TRUE, DC_SetStandbyTo);
        OSDMenuItemAdd("Set Standby now", NULL, NULL, NULL, TRUE, FALSE, DC_SetStandbyNow);
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
                process_dev("/dev/sda", DC_SetStandbyNow, 0);
                State = ST_Finished;
              }
              break;

            case MI_Save:
              if (param1 == RKEY_Ok)
              {
                process_dev("/dev/sda", DC_SetStandbyTo, StandbyTime);
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
            process_dev("/dev/sda", DC_SetStandbyTo, StandbyTime);
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
    }

    if ((event == EVT_KEY) && (param1 == RKEY_Sleep))
    {
      process_dev("/dev/sda", DC_SetStandbyNow, 0);
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
      strcpy(output, "21 minutes");
      break;
    case 253:
      strcpy(output, "vendor-specific");
      break;
    case 254:
      strcpy(output, "?reserved");
      break;
    case 255:
      strcpy(output, "21 minutes + 15 seconds");
      break;
    default:
      if (standby <= 240)
      {
        unsigned int secs = standby * 5;
        unsigned int mins = secs / 60;
        secs %= 60;
        if (mins && secs) sprintf(output, "%u minutes + %u seconds", mins, secs);
        else if (mins)    sprintf(output, "%u minutes", mins);
        else if (secs)    sprintf(output, "%u seconds", secs);
      }
      else if (standby <= 251)
      {
        unsigned int mins = (standby - 240) * 30;
        unsigned int hrs  = mins / 60;
        mins %= 60;
        if (hrs && mins)  sprintf(output, "%u hours + %u minutes", hrs, mins);
        else if (hrs)     sprintf(output, "%u hours", hrs);
        else if (mins)    sprintf(output, "%u minutes", mins);
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
void ShowErrorMessage(char *MessageStr, char *TitleStr)
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
