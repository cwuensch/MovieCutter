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
#include                <stdio.h>
#include                <sys/stat.h>
#include                <tap.h>
#include                <libFireBird.h>
#include                "StandbySwitch.h"

TAP_ID                  (TAPID);
TAP_PROGRAM_NAME        (PROGRAM_NAME" "VERSION);
TAP_PROGRAM_VERSION     (VERSION);
TAP_AUTHOR_NAME         (AUTHOR);
TAP_DESCRIPTION         (DESCRIPTION);
TAP_ETCINFO             (__DATE__);

typedef enum
{
  ST_None,
  ST_Init,
  ST_Menu,
  ST_Finished
} tState;

typedef enum {
  MI_StandbyMode,
  MI_Scart,
  MI_Tuner,
  MI_VFD,
//  MI_Dummy,
  MI_EPGMode,
  MI_DumpEEPROM,
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
  LS_SaveEPG,
  LS_DumpEEPROM,
  LS_Save,
  LS_Exit,
  LS_active,
  LS_passive,
  LS_on,
  LS_off,
  LS_yes,
  LS_no,
  LS_EEPDumped,
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
  "EPG-Daten auf Festplatte speichern",
  "EEPROM Abbild speichern",
  "Übernehmen",
  "Beenden",
  "Aktiv",
  "Passiv",
  "Ein",
  "Aus",
  "Ja",
  "Nein",
  "EEPROM Abbild gespeichert.",
  "Fehler beim Lesen des Standby-Modus.\n\nUnerwarteter Wert im EEPROM gefunden."
};


#define LangGetString(x)  LangGetStringDefault(x, DefaultStrings[x])

static void  CreateSettingsDir(void);
static int   GetStandbyOffset();
static int   GetEPGOffset();
static bool  DumpEEPROM(unsigned int length);
static bool  GetStandbyMode(tStandbyMode *outMode, bool *outEPG);
static bool  SetStandbyMode(tStandbyMode *const newMode, bool newEPG);
static void  ShowErrorMessage(char *MessageStr, char *TitleStr);


// Globale Variablen
tState                  State = ST_None;
tStandbyMode            curStandby;
bool                    curSaveEPG;


// ============================================================================
//                              IMPLEMENTIERUNG
// ============================================================================
int TAP_Main(void)
{
  TRACEENTER();
  CreateSettingsDir();

  if (GetStandbyMode(&curStandby, &curSaveEPG))
  {
    State = ST_Init;
  }
  else
  {
    OSDMenuInfoBoxShow(PROGRAM_NAME " " VERSION, LangGetString(LS_ErrorReading), 500);
    do
    {
      OSDMenuEvent(NULL, NULL, NULL);
    } while(OSDMenuInfoBoxIsVisible());
    TRACEEXIT();
    return 0;
  }

  TRACEEXIT();
  return 1;
}

dword TAP_EventHandler(word event, dword param1, dword param2)
{
  static bool           DoNotReenter = FALSE;
  TRACEENTER();

  // Behandlung offener MessageBoxen (rekursiver Aufruf, auch bei DoNotReenter)
  if(OSDMenuMessageBoxIsVisible() || OSDMenuInfoBoxIsVisible())
  {
    if(OSDMenuMessageBoxIsVisible())
    {
      #ifdef __ALTEFBLIB__
        OSDMenuMessageBoxDoScrollOver(&event, &param1);
      #endif
    }
    OSDMenuEvent(&event, &param1, &param2);
    param1 = 0;
  }

  if (!DoNotReenter)
  {
    DoNotReenter = TRUE;
    switch (State)
    {
      case ST_Init:
      {
        char* hString = (char*)TAP_MemAlloc(256);

        OSDMenuInitialize(FALSE, TRUE, FALSE, TRUE, LangGetString(LS_MenuTitle), NULL);
        OSDMenuItemAdd(LangGetString(LS_StandbyMode), LangGetString((curStandby.Active1 ? LS_active : LS_passive)), NULL, NULL, TRUE, TRUE, MI_StandbyMode);
        TAP_SPrint(hString, 256, "    %s", LangGetString(LS_Scart));
        OSDMenuItemAdd(hString, LangGetString((curStandby.Scart ? LS_on : LS_off)), NULL, NULL, curStandby.Active1, TRUE, MI_Scart);
        TAP_SPrint(hString, 256, "    %s", LangGetString(LS_Tuner));
        OSDMenuItemAdd(hString, LangGetString((curStandby.Tuner ? LS_on : LS_off)), NULL, NULL, curStandby.Active1, TRUE, MI_Tuner);
        TAP_SPrint(hString, 256, "    %s", LangGetString(LS_VFD));
        OSDMenuItemAdd(hString, LangGetString((curStandby.VFD ? LS_on : LS_off)), NULL, NULL, curStandby.Active1, TRUE, MI_VFD);
        OSDMenuItemAdd(LangGetString(LS_SaveEPG), LangGetString(curSaveEPG ? LS_yes : LS_no), NULL, NULL, isUTFToppy(), TRUE, MI_EPGMode);
        OSDMenuItemAdd(LangGetString(LS_DumpEEPROM), NULL, NULL, NULL, TRUE, FALSE, MI_DumpEEPROM);
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
        if ((event == EVT_KEY) && (param1 == RKEY_Left || param1 == RKEY_Right || param1 == RKEY_Ok))
        {
          int curItem = OSDMenuGetCurrentItem();
          switch (curItem)
          {
            case MI_StandbyMode:
              curStandby.Active1 = !curStandby.Active1;
              curStandby.Active2 = curStandby.Active1;
              OSDMenuItemModifyValue(curItem, LangGetString((curStandby.Active1 ? LS_active : LS_passive)));

              curStandby.Scart = curStandby.Active1;
              curStandby.Tuner = curStandby.Active1;
              curStandby.VFD   = curStandby.Active1;

              OSDMenuItemModifyValue(MI_Scart, LangGetString((curStandby.Scart ? LS_on : LS_off)));
              OSDMenuItemModifyValue(MI_Tuner, LangGetString((curStandby.Tuner ? LS_on : LS_off)));
              OSDMenuItemModifyValue(MI_VFD,   LangGetString((curStandby.VFD   ? LS_on : LS_off)));

              OSDMenuItemModifySelectable(MI_Scart, curStandby.Active1);
              OSDMenuItemModifySelectable(MI_Tuner, curStandby.Active1);
              OSDMenuItemModifySelectable(MI_VFD,   curStandby.Active1);
              break;

            case MI_Scart:
              curStandby.Scart = !curStandby.Scart;
              OSDMenuItemModifyValue(curItem, LangGetString((curStandby.Scart ? LS_on : LS_off)));
              break;

            case MI_Tuner:
              curStandby.Tuner = !curStandby.Tuner;
              OSDMenuItemModifyValue(curItem, LangGetString((curStandby.Tuner ? LS_on : LS_off)));
              break;

            case MI_VFD:
              curStandby.VFD = !curStandby.VFD;
              OSDMenuItemModifyValue(curItem, LangGetString((curStandby.VFD ? LS_on : LS_off)));
              break;

            case MI_EPGMode:
              curSaveEPG = !curSaveEPG;
              OSDMenuItemModifyValue(curItem, LangGetString((curSaveEPG ? LS_yes : LS_no)));
              break;

            case MI_DumpEEPROM:
              if (param1 == RKEY_Ok)
                if (DumpEEPROM(1024))
                  ShowErrorMessage(LangGetString(LS_EEPDumped), PROGRAM_NAME);
              break;

            case MI_Save:
              if (param1 == RKEY_Ok)
                SetStandbyMode(&curStandby, curSaveEPG);
              State = ST_Finished;
              break;
          }

          OSDMenuUpdate(FALSE);
          param1 = 0;
        }
        else if ((event == EVT_KEY) && (param1 == RKEY_Green || param1 == RKEY_Exit))
        {
          if (param1 == RKEY_Green)
            SetStandbyMode(&curStandby, curSaveEPG);
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
    DoNotReenter = FALSE;
  }

  TRACEEXIT();
  return param1;
}

void CreateSettingsDir(void)
{
  TRACEENTER();

//  struct stat64 st;
//  if (lstat64(TAPFSROOT "/ProgramFiles/Settings", &st) == -1)
    mkdir(TAPFSROOT "/ProgramFiles/Settings", 0666);
//  if (lstat64(TAPFSROOT LOGDIR, &st) == -1)
    mkdir(TAPFSROOT LOGDIR, 0666);

  TRACEEXIT();
}


// ----------------------------------------------------------------------------
//                           Hauptfunktionen
// ----------------------------------------------------------------------------
int GetStandbyOffset()
{
  switch(TAP_GetSystemId())
  {
    case 42031:    // CRP-2401CI+
    case 42561:    // CRP-2401CI+ Conax
      return 0x2c;
    case 22121:    // SRP-2410
    case 22122:    // SRP-2410M
    case 22120:    // SRP-2401CI+ Conax
    case 22130:    // SRP-2401CI+
    case 22010:    // TMS-2100
      return 0x2b;
/*    case 22570:    // SRP-2401CI+ Eco */
    default:
      return 0x2b;
  }
}
int GetEPGOffset()
{
  switch(TAP_GetSystemId())
  {
    case 42561:    // CRP-2401CI+ Conax
      return 0x1c;
    case 42031:    // CRP-2401CI+
      return 0x1c; // 0x1f ??
    case 22010:    // TMS-2100
      return 0x1b;
/*    case 22121:    // SRP-2410
    case 22122:    // SRP-2410M
    case 22120:    // SRP-2401CI+ Conax
    case 22130:    // SRP-2401CI+
    case 22570:    // SRP-2401CI+ Eco */
    default:
      return 0x1b;
  }
}

bool DumpEEPROM(unsigned int length)
{
  byte                 *__etcInfo;
  FILE                 *fp;
  bool                  ret = FALSE;

  __etcInfo = (byte*)FIS_vEtcInfo();
  if(!__etcInfo) return FALSE;

  if ((fp = fopen(TAPFSROOT LOGDIR "/EEPROM.dump", "wb")))
  {
    ret = (fwrite(__etcInfo, 1, length, fp) == length);
    fclose(fp);
    return ret;
  }
  return FALSE;
}

bool GetStandbyMode(tStandbyMode *outMode, bool *outEPG)
{
  byte                 *__etcInfo;
  tStandbyMode          curMode;
  tEPGMode              curEPG;

  __etcInfo = (byte*)FIS_vEtcInfo();
  if(!__etcInfo) return FALSE;

  curMode.ModeByte = __etcInfo[GetStandbyOffset()];
  curEPG.ModeByte = __etcInfo[GetEPGOffset()];
  TAP_PrintNet("Standby mode: Byte=0x%02hhx (Active1=%hhu, Active2=%hhu, Zeros=%hhu, Scart=%hhu, Tuner=%hhu, VFD=%hhu)\n", curMode.ModeByte, curMode.Active1, curMode.Active2, curMode.Zeros, curMode.Scart, curMode.Tuner, curMode.VFD);
  TAP_PrintNet("SaveEPG mode: Byte=0x%02hhx (active=%d)\n", curEPG.ModeByte, curEPG.SaveEPG);
  if (curMode.Zeros == 0 || curMode.ModeByte == 0xff)
  {
curMode.Zeros = 0;
    if(outMode) *outMode = curMode;
    if(outEPG)  *outEPG = curEPG.SaveEPG;
    return TRUE;
  }
  return FALSE;
}

bool SetStandbyMode(tStandbyMode *const newMode, bool newEPG)
{
  byte                 *__etcInfo;
  int                   address;
  tStandbyMode          curMode;
  tEPGMode              curEPG;
  bool                  doWrite = FALSE;

  static void (*__Appl_WriteEeprom)(bool) = NULL;

  if(!__Appl_WriteEeprom)
  {
    __Appl_WriteEeprom = (void*)TryResolve("_Z16Appl_WriteEepromb");
    if(!__Appl_WriteEeprom) return FALSE;
  }

  __etcInfo = (byte*)FIS_vEtcInfo();
  if(!__etcInfo) return FALSE;

  // Standby-Modus setzen
  address = GetStandbyOffset();
  curMode.ModeByte = __etcInfo[address];
  if(/*(curMode.Zeros!=0) ||*/ (newMode->Zeros!=0)) return FALSE;

  if(newMode->ModeByte != curMode.ModeByte)
  {
    __etcInfo[address] = newMode->ModeByte;
    TAP_PrintNet("NEW Standby mode: Byte=0x%02hhx (Active1=%hhu, Active2=%hhu, Zeros=%hhu, Scart=%hhu, Tuner=%hhu, VFD=%hhu)\n", newMode->ModeByte, newMode->Active1, newMode->Active2, newMode->Zeros, newMode->Scart, newMode->Tuner, newMode->VFD);
    doWrite = TRUE;
  }

  // EPG-Modus setzen
  address = GetEPGOffset();
  curEPG.ModeByte = __etcInfo[address];

  if(newEPG != curEPG.SaveEPG)
  {
    curEPG.SaveEPG = (newEPG ? 1 : 0);
    __etcInfo[address] = curEPG.ModeByte;
    TAP_PrintNet("NEW SaveEPG mode: Byte=0x%02hhx (active=%d)\n", curEPG.ModeByte, newEPG);
    doWrite = TRUE;
  }

  if(doWrite) __Appl_WriteEeprom(FALSE);
  return TRUE;
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
