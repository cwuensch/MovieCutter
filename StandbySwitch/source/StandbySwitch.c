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
#include                <stdarg.h>
#include                <sys/types.h>
#include                <sys/stat.h>
#include                <utime.h>
#include                <tap.h>
#include                <libFireBird.h>
#include                "../../../../../FireBirdLib/TMSOSDMenu/FBLib_TMSOSDMenu.h"
#include                "StandbySwitch.h"

TAP_ID                  (TAPID);
TAP_PROGRAM_NAME        (PROGRAM_NAME" "VERSION);
TAP_PROGRAM_VERSION     (VERSION);
TAP_AUTHOR_NAME         (AUTHOR);
TAP_DESCRIPTION         (DESCRIPTION);
TAP_ETCINFO             (__DATE__);

extern TYPE_GrData      _Button_0_Gd;
extern TYPE_GrData      _Button_9_Gd;
extern TYPE_GrData      _Button_red_Gd;
extern TYPE_GrData      _Button_green_Gd;
extern TYPE_GrData      _Button_yellow_Gd;
extern TYPE_GrData      _Button_blue_Gd;
extern TYPE_GrData      _Button_white_Gd;
extern TYPE_GrData      _Button_stop_Gd;
extern TYPE_GrData      _Button_ok_Gd;
extern TYPE_GrData      _Button_exit_Gd;


typedef enum
{
  ST_None,
  ST_Init,
  ST_Menu,
  ST_MACEdit,
  ST_Finished
} tState;

typedef enum {
  MI_StandbyMode,
  MI_Scart,
  MI_Tuner,
  MI_VFD,
//  MI_Dummy,
  MI_EPGMode,
  MI_MACAddress,
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
  LS_MACAddress,
  LS_DumpEEPROM,
  LS_Save,
  LS_Exit,
  LS_active,
  LS_passive,
  LS_on,
  LS_off,
  LS_yes,
  LS_no,
  LS_Cancel,
  LS_EEPDumped,
  LS_ErrorReading,
  LS_ErrorWriting,
  LS_RebootNeeded,
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
  "MAC-Adresse",
  "EEPROM Abbild speichern",
  "Übernehmen",
  "Beenden",
  "Aktiv",
  "Passiv",
  "Ein",
  "Aus",
  "Ja",
  "Nein",
  "Abbrechen",
  "EEPROM Abbild gespeichert.",
  "Fehler beim Lesen des Standby-Modus.\n\nUnerwarteter Wert im EEPROM gefunden.",
  "Unerwarteter Wert im EEPROM gefunden.\n\nEs wurde nicht geschrieben.",
  "Geänderte MAC-Adresse noch nicht aktiv.\n\nBitte den Receiver neu starten!"
};


#define LangGetString(x)  LangGetStringDefault(x, DefaultStrings[x])

void OSDKeyboard_TMSRemoteDirectMode(bool DirectMode);

static void  CreateSettingsDir(void);
static void  DrawButtons(bool EditMode);
static void  DrawMACCursor(void);
static int   GetStandbyOffset(void);
static int   GetEPGOffset(void);
static int   GetMACOffset(void);
static char* PrintMAC(tMAC *MAC);
static bool  DumpEEPROM(unsigned int length);
static bool  GetStandbyMode(tStandbyMode *outMode, bool *outEPG, tMAC *outMAC);
static bool  SetStandbyMode(tStandbyMode *const newMode, bool newEPG);
static bool  SetMacAddress(tMAC *newMAC);
static void  CloseLogMC(void);
static void  WriteLogMC(char *ProgramName, char *Text);
static void  WriteLogMCf(const char *format, ...);
static void  ShowErrorMessage(char *MessageStr, char *TitleStr);


// Globale Variablen
tState                  State = ST_None;
tStandbyMode            curStandby;
bool                    curSaveEPG;
bool                    MACchanged;
tMAC                    curMAC, oldMAC;
int                     curPos;


// ============================================================================
//                              IMPLEMENTIERUNG
// ============================================================================
int TAP_Main(void)
{
  TRACEENTER();
  CreateSettingsDir();

  WriteLogMC(PROGRAM_NAME, "StandbySwitch " VERSION " started.");

  if (GetStandbyMode(&curStandby, &curSaveEPG, &curMAC))
  {
    MACchanged = FALSE;
    State = ST_Init;
  }
  else
  {
    OSDMenuInfoBoxShow(PROGRAM_NAME " " VERSION, LangGetString(LS_ErrorReading), 500);
    do
    {
      OSDMessageEvent(NULL, NULL, NULL);
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
    OSDMessageEvent(&event, &param1, &param2);
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
        OSDMenuItemAdd(LangGetString(LS_MACAddress), PrintMAC(&curMAC), NULL, NULL, TRUE, FALSE, MI_MACAddress);
        OSDMenuItemAdd(LangGetString(LS_DumpEEPROM), NULL, NULL, NULL, TRUE, FALSE, MI_DumpEEPROM);
//        OSDMenuItemAdd(" ", NULL, NULL, NULL, FALSE, FALSE, MI_Dummy);
        OSDMenuItemAdd(LangGetString(LS_Save), NULL, NULL, NULL, TRUE, FALSE, MI_Save);

        DrawButtons(FALSE);
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
              curStandby.Zeros = 0;
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
              curStandby.Zeros = 0;
              OSDMenuItemModifyValue(curItem, LangGetString((curStandby.Scart ? LS_on : LS_off)));
              break;

            case MI_Tuner:
              curStandby.Tuner = !curStandby.Tuner;
              curStandby.Zeros = 0;
              OSDMenuItemModifyValue(curItem, LangGetString((curStandby.Tuner ? LS_on : LS_off)));
              break;

            case MI_VFD:
              curStandby.VFD = !curStandby.VFD;
              curStandby.Zeros = 0;
              OSDMenuItemModifyValue(curItem, LangGetString((curStandby.VFD ? LS_on : LS_off)));
              break;

            case MI_EPGMode:
              curSaveEPG = !curSaveEPG;
              OSDMenuItemModifyValue(curItem, LangGetString((curSaveEPG ? LS_yes : LS_no)));
              break;

            case MI_MACAddress:
              memcpy(&oldMAC, &curMAC, sizeof(tMAC));
              curPos = 0;
//              OSDMenuItemModifyValue(curItem, "");
              OSDMenuSetCursor(CT_Dark);
              DrawButtons(TRUE);
              OSDKeyboard_TMSRemoteDirectMode(TRUE);
              State = ST_MACEdit;
              break;

            case MI_DumpEEPROM:
              if (param1 == RKEY_Ok)
                if (DumpEEPROM(1024))
                  ShowErrorMessage(LangGetString(LS_EEPDumped), PROGRAM_NAME);
              break;

            case MI_Save:
              if (param1 == RKEY_Ok)
                if (!SetStandbyMode(&curStandby, curSaveEPG) || (MACchanged && !SetMacAddress(&curMAC)))
                  ShowErrorMessage(LangGetString(LS_ErrorWriting), PROGRAM_NAME);
              State = ST_Finished;
              break;
          }

          OSDMenuUpdate(FALSE);
          if (State == ST_MACEdit) DrawMACCursor();
          param1 = 0;
        }
        else if ((event == EVT_KEY) && (param1 == RKEY_Green || param1 == RKEY_Exit))
        {
          if (param1 == RKEY_Green)
            if (!SetStandbyMode(&curStandby, curSaveEPG) || (MACchanged && !SetMacAddress(&curMAC)))
              ShowErrorMessage(LangGetString(LS_ErrorWriting), PROGRAM_NAME);
          State = ST_Finished;
          param1 = 0;
        }
        break;
      }

      case ST_MACEdit:
      {
        bool GoRight = TRUE;
        byte newHalf = 0xff;
        byte oldHalf;
        tByte *curByte = (tByte*) &curMAC.Bytes[curPos / 2];
        if (curPos % 2 == 0)
          oldHalf = curByte->FirstHalf;
        else
          oldHalf = curByte->LastHalf;

        if (event == EVT_KEY)
        {
          switch (param1)
          {
            case RKEY_0:
            case RKEY_1:
            case RKEY_2:
            case RKEY_3:
            case RKEY_4:
            case RKEY_5:
            case RKEY_6:
            case RKEY_7:
            case RKEY_8:
            case RKEY_9:
              newHalf = param1 & 0xff;
              break;

            case RKEY_Red:
            case RKEY_F1:
              newHalf = 0xA;
              break;

            case RKEY_Green:
              newHalf = 0xB;
              break;

            case RKEY_Yellow:
              newHalf = 0xC;
              break;

            case RKEY_Blue:
              newHalf = 0xD;
              break;

            case RKEY_White:
            case RKEY_Ab:
              newHalf = 0xE;
              break;

            case RKEY_Stop:
            case RKEY_Fav:
              newHalf = 0xF;
              break;

            case RKEY_Left:
              if (curPos > 0) curPos--;
              break;

            case RKEY_Right:
              if (curPos < 11) curPos++;
              break;

            case RKEY_Up:
              if (oldHalf < 0xF) newHalf = oldHalf + 1;
              else newHalf = 0;
              GoRight = FALSE;
              break;

            case RKEY_Down:
              if (oldHalf > 0) newHalf = oldHalf - 1;
              else newHalf = 0xF;
              GoRight = FALSE;
              break;

            case RKEY_Ok:
              MACchanged = TRUE;
              OSDMenuSetCursor(CT_Standard);
              State = ST_Menu;
              break;

            case RKEY_Exit:
              memcpy(&curMAC, &oldMAC, sizeof(tMAC));
              OSDMenuSetCursor(CT_Standard);
              State = ST_Menu;
              break;
          }
          param1 = 0;
        }

        if (event == EVT_TMSREMOTEASCII)
        {
          switch(param1)
          {
            case 0x0D:     //CR
              MACchanged = TRUE;
              OSDMenuSetCursor(CT_Standard);
              State = ST_Menu;
              break;

            case 0x1B:     //ESC
              memcpy(&curMAC, &oldMAC, sizeof(tMAC));
              OSDMenuSetCursor(CT_Standard);
              State = ST_Menu;
              break;

            case 0x0124:   //Pos1
              curPos = 0;
              break;

            case 0x0123:   //End
              curPos = 11;
              break;

            case 0x0125:   //Left
              if (curPos > 0) curPos--;
              break;

            case 0x0127:   //Right
              if (curPos < 11) curPos++;
              break;

            case 0x0126:   //Up
              if (oldHalf < 0xF) newHalf = oldHalf + 1;
              else newHalf = 0;
              GoRight = FALSE;
              break;

            case 0x0128:   //Down
              if (oldHalf > 0) newHalf = oldHalf - 1;
              else newHalf = 0xF;
              GoRight = FALSE;
              break;

            case 'A':
            case 'a':
            case 0x0170:   //F1 = RED
              newHalf = 0xA;
              break;

            case 'B':
            case 'b':
            case 0x0171:   //F2 = GREEN
              newHalf = 0xB;
              break;

            case 'C':
            case 'c':
            case 0x0172:   //F3 = YELLOW
              newHalf = 0xC;
              break;

            case 'D':
            case 'd':
            case 0x0173:   //F4 = BLUE
              newHalf = 0xD;
              break;

            case 'E':
            case 'e':
            case 0x0174:   //F5
              newHalf = 0xE;
              break;

            case 'F':
            case 'f':
            case 0x0175:   //F6
              newHalf = 0xF;
              break;

            default:
              //ASCII Codes
              if ((param1 >= '0') && (param1 <= '9'))
                newHalf = param1 - '0';
          }
          param1 = 0;
        }

        if (event == EVT_KEY || event == EVT_TMSREMOTEASCII)
        {
          if (newHalf != 0xff)
          {
            tByte *curByte = (tByte*) &curMAC.Bytes[curPos / 2];
            if (curPos % 2 == 0)
              curByte->FirstHalf = newHalf;
            else
              curByte->LastHalf = newHalf;
            if (GoRight && curPos < 11) curPos++;
          }
          OSDMenuItemModifyValue(MI_MACAddress, PrintMAC(&curMAC));

          if (State == ST_Menu)
          {
            OSDKeyboard_TMSRemoteDirectMode(FALSE);
            DrawButtons(FALSE);
          }
          OSDMenuUpdate(FALSE);
          if (State == ST_MACEdit)
            DrawMACCursor();
        }
        break;
      }

      case ST_Finished:
      {
        OSDMenuDestroy();
        WriteLogMC(PROGRAM_NAME, "Exit TAP.\r\n");
        CloseLogMC();
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

void DrawButtons(bool EditMode)
{
  TRACEENTER();
  OSDMenuButtonsClear();

  if (EditMode)
  {
    OSDMenuButtonAdd(1, BI_UserDefined, &_Button_0_Gd,      "..");
    OSDMenuButtonAdd(1, BI_UserDefined, &_Button_9_Gd,      "  ");
    OSDMenuButtonAdd(1, BI_UserDefined, &_Button_red_Gd,    "A  ");
    OSDMenuButtonAdd(1, BI_UserDefined, &_Button_green_Gd,  "B  ");
    OSDMenuButtonAdd(1, BI_UserDefined, &_Button_yellow_Gd, "C  ");
    OSDMenuButtonAdd(1, BI_UserDefined, &_Button_blue_Gd,   "D  ");
    OSDMenuButtonAdd(1, BI_UserDefined, &_Button_white_Gd,  "E  ");
    OSDMenuButtonAdd(1, BI_UserDefined, &_Button_stop_Gd,   "F     ");
    OSDMenuButtonAdd(1, BI_UserDefined, &_Button_ok_Gd,     LangGetString(LS_Save));
    OSDMenuButtonAdd(1, BI_UserDefined, &_Button_exit_Gd,   LangGetString(LS_Cancel));
  }
  else
  {
    OSDMenuButtonAdd(1, BI_UserDefined, &_Button_green_Gd,  LangGetString(LS_Save));
    OSDMenuButtonAdd(1, BI_UserDefined, &_Button_exit_Gd,   LangGetString(LS_Exit));
  }

  TRACEEXIT();
}

void DrawMACCursor(void)
{
  TRACEENTER();

  tMenu *pMenu = &Menu[CurrentMenuLevel];
  tItem *pItem = &pMenu->Item[pMenu->CurrentTopIndex + MI_MACAddress];

  int    YText = 101 - (5 + FONTYOFFSET);
  int    hL    = 35;
  dword  yT    = YText + (MI_MACAddress * (hL + 2));
  char   curChar[2];
  int    cursorWidth, cursorPos;

  curChar[0] = pItem->Value[curPos + curPos/2];
  curChar[1] = '\0';
  pItem->Value[curPos + curPos/2] = '\0';

  cursorWidth = FM_GetStringWidth(curChar, pMenu->FontListValueColumn);
  cursorPos = FM_GetStringWidth(pItem->Value, pMenu->FontListValueColumn);
  pItem->Value[curPos + curPos/2] = curChar[0];

//  FM_PutStringAA(OSDRgn, pMenu->ValueXPos + 30 + pMenu->ValueXOffset, yT + 5 + FONTYOFFSET, 645, "   :   :   :   :   :   ", COLOR_White, COLOR_None, pMenu->FontListValueColumn, TRUE, ALIGN_LEFT, 1);

  TAP_Osd_DrawRectangle(OSDRgn, pMenu->ValueXPos + 30 + pMenu->ValueXOffset + cursorPos, yT + 5 + FONTYOFFSET + 21, cursorWidth, 2, 1, COLOR_Yellow);

  TRACEEXIT();
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
int GetMACOffset()
{
  switch(TAP_GetSystemId())
  {
//    case 42561:    // CRP-2401CI+ Conax
//      return 0x1c;
    case 42031:    // CRP-2401CI+
      return 0xf6;
//    case 22010:    // TMS-2100
//      return 0x1b;
/*    case 22121:    // SRP-2410
    case 22122:    // SRP-2410M
    case 22120:    // SRP-2401CI+ Conax
    case 22130:    // SRP-2401CI+
    case 22570:    // SRP-2401CI+ Eco */
    default:
      return 0xf6;
  }
}

char* PrintMAC(tMAC *MAC)
{
  static char Buffer[18];
  TRACEENTER();
  TAP_SPrint(Buffer, sizeof(Buffer), "%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX", MAC->Bytes[0], MAC->Bytes[1], MAC->Bytes[2], MAC->Bytes[3], MAC->Bytes[4], MAC->Bytes[5]);
  TRACEEXIT();
  return Buffer;
}

bool DumpEEPROM(unsigned int length)
{
  byte                 *__etcInfo;
  FILE                 *fp;
  bool                  ret = FALSE;

  TRACEENTER();
  __etcInfo = (byte*)FIS_vEtcInfo();
  if(!__etcInfo)
  {
    TRACEEXIT();
    return FALSE;
  }

  if ((fp = fopen(TAPFSROOT LOGDIR "/EEPROM.dump", "wb")))
  {
    ret = (fwrite(__etcInfo, 1, length, fp) == length);
    fclose(fp);
    TRACEEXIT();
    return ret;
  }
  TRACEEXIT();
  return FALSE;
}

bool GetStandbyMode(tStandbyMode *outMode, bool *outEPG, tMAC *outMAC)
{
  byte                 *__etcInfo;
  tStandbyMode          curMode;
  tEPGMode              curEPG;

  TRACEENTER();
  __etcInfo = (byte*)FIS_vEtcInfo();
  if(!__etcInfo)
  {
    TRACEEXIT();
    return FALSE;
  }

  curMode.ModeByte = __etcInfo[GetStandbyOffset()];
  curEPG.ModeByte = __etcInfo[GetEPGOffset()];

  WriteLogMCf("Standby mode: Byte=0x%02hhx (Active1=%hhu, Active2=%hhu, Zeros=%hhu, Scart=%hhu, Tuner=%hhu, VFD=%hhu)", curMode.ModeByte, curMode.Active1, curMode.Active2, curMode.Zeros, curMode.Scart, curMode.Tuner, curMode.VFD);
  WriteLogMCf("SaveEPG mode: Byte=0x%02hhx (active=%d)", curEPG.ModeByte, curEPG.SaveEPG);
  WriteLogMCf("MAC address:  %s", PrintMAC((tMAC*) &__etcInfo[GetMACOffset()]));
  if (curMode.Zeros == 0 || curMode.ModeByte == 0xff)
  {
//curMode.Zeros = 0;
    if(outMode) *outMode = curMode;
    if(outEPG)  *outEPG = curEPG.SaveEPG;
    if(outMAC)  memcpy(outMAC, &__etcInfo[GetMACOffset()], sizeof(tMAC));
    TRACEEXIT();
    return TRUE;
  }

  TRACEEXIT();
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
  TRACEENTER();

  if(!__Appl_WriteEeprom)
  {
    __Appl_WriteEeprom = (void*)TryResolve("_Z16Appl_WriteEepromb");
    if(!__Appl_WriteEeprom)
    {
      TRACEEXIT();
      return FALSE;
    }
  }

  __etcInfo = (byte*)FIS_vEtcInfo();
  if(!__etcInfo)
  {
    TRACEEXIT();
    return FALSE;
  }

  // Standby-Modus setzen
  address = GetStandbyOffset();
  curMode.ModeByte = __etcInfo[address];

  if(newMode->ModeByte != curMode.ModeByte)
  {
    if(/*(curMode.Zeros!=0) ||*/ (newMode->Zeros!=0))
    {
      WriteLogMC(PROGRAM_NAME, "Error writing new Standby mode: Zeros is not 0. Nothing written.");
      TRACEEXIT();
      return FALSE;
    }
    __etcInfo[address] = newMode->ModeByte;
    WriteLogMCf("NEW Standby mode: Byte=0x%02hhx (Active1=%hhu, Active2=%hhu, Zeros=%hhu, Scart=%hhu, Tuner=%hhu, VFD=%hhu)", newMode->ModeByte, newMode->Active1, newMode->Active2, newMode->Zeros, newMode->Scart, newMode->Tuner, newMode->VFD);
    doWrite = TRUE;
  }

  // EPG-Modus setzen
  address = GetEPGOffset();
  curEPG.ModeByte = __etcInfo[address];

  if(newEPG != curEPG.SaveEPG)
  {
    curEPG.SaveEPG = (newEPG ? 1 : 0);
    __etcInfo[address] = curEPG.ModeByte;
    WriteLogMCf("NEW SaveEPG mode: Byte=0x%02hhx (active=%d)", curEPG.ModeByte, newEPG);
    doWrite = TRUE;
  }

  if(doWrite) __Appl_WriteEeprom(FALSE);

  TRACEEXIT();
  return TRUE;
}

bool SetMacAddress(tMAC *newMAC)
{
  byte                 *__etcInfo;
  int                   address;

  static void(*_DevEeprom_SetMacAddr)(byte *MAC) = NULL;
  TRACEENTER();

  if(!_DevEeprom_SetMacAddr)
  {
    _DevEeprom_SetMacAddr = (void*)TryResolve("DevEeprom_SetMacAddr");
    if(!_DevEeprom_SetMacAddr)
    {
      TRACEEXIT();
      return FALSE;
    }
  }

  __etcInfo = (byte*)FIS_vEtcInfo();
  if(!__etcInfo)
  {
    TRACEEXIT();
    return FALSE;
  }

  address = GetMACOffset();
  if (memcmp(&__etcInfo[address], newMAC, sizeof(tMAC)) != 0)
  {
    WriteLogMCf("NEW MAC address:  %s", PrintMAC(newMAC));
    _DevEeprom_SetMacAddr(newMAC->Bytes);
    ShowErrorMessage(LangGetString(LS_RebootNeeded), PROGRAM_NAME);
  }
  TRACEEXIT();
  return TRUE;
}


// ----------------------------------------------------------------------------
//                         LogFile-Funktionen
// ----------------------------------------------------------------------------
FILE *fLogMC = NULL;

void CloseLogMC(void)
{
  TRACEENTER();
  if (fLogMC)
  {
    fclose(fLogMC);

    //As the log would receive the Linux time stamp (01.01.2000), adjust to the PVR's time
    struct utimbuf      times;
    times.actime = PvrTimeToLinux(Now(NULL));
    times.modtime = times.actime;
    utime(TAPFSROOT LOGDIR "/" LOGFILENAME, &times);
  }
  fLogMC = NULL;
  TRACEEXIT();
}

void WriteLogMC(char *ProgramName, char *Text)
{
  char                  TS[256];
  byte                  Sec;

  TRACEENTER();
  TimeFormat(Now(&Sec), Sec, TIMESTAMP_YMDHMS, TS);

  if (!fLogMC)
  {
    fLogMC = fopen(TAPFSROOT LOGDIR "/" LOGFILENAME, "ab");
    setvbuf(fLogMC, NULL, _IOLBF, 4096);  // zeilenweises Buffering
  }
  if (fLogMC)
  {
    fprintf(fLogMC, "%s %s\r\n", TS, Text);
//    close(fLogMC);
  }

  TAP_PrintNet("%s %s: %s\n", TS, ((ProgramName && ProgramName[0]) ? ProgramName : ""), Text);
  TRACEEXIT();
}

void WriteLogMCf(const char *format, ...)
{
  char Text[512];
  TRACEENTER();

  if(format)
  {
    va_list args;
    va_start(args, format);
    vsnprintf(Text, sizeof(Text), format, args);
    va_end(args);
    WriteLogMC(PROGRAM_NAME, Text);
  }
  TRACEEXIT();
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
