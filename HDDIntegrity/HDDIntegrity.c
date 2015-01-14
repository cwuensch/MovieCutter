#define _FILE_OFFSET_BITS  64
#define __USE_LARGEFILE64  1
#ifdef _MSC_VER
  #define __const const
#endif

#include                <stdio.h>
#include                <stdlib.h>
#include                <string.h>
#include                <unistd.h>
#include                <tap.h>
#include                <libFireBird.h>
#include                "HDDIntegrity.h"


TAP_ID                  (TAPID);
TAP_PROGRAM_NAME        (PROGRAM_NAME" "VERSION);
TAP_PROGRAM_VERSION     (VERSION);
TAP_AUTHOR_NAME         (AUTHOR);
TAP_DESCRIPTION         (DESCRIPTION);
TAP_ETCINFO             (__DATE__);


bool                    HIShowMessageBox = FALSE;
dword                   LastMessageBoxKey;



// ============================================================================
//                              IMPLEMENTIERUNG
// ============================================================================

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
  HIShowMessageBox = TRUE;
  while (HIShowMessageBox)
  {
    TAP_SystemProc();
    TAP_Sleep(1);
  }

  TAP_Osd_Sync();
  if(OldSysSubState != 0) TAP_EnterNormalNoInfo();

  HDD_TAP_PopDir();
  TRACEEXIT();
}


int TAP_Main(void)
{
  #if STACKTRACE == TRUE
    CallTraceInit();
    CallTraceEnable(TRUE);
    TRACEENTER();
  #endif

  if (system("mount -o remount,rw,integrity /dev/sda2") == 0)
  {
    TAP_PrintNet("Integrity-Modus aktiviert.")
    if ((GetUptime() / 6000) >= 1)
    {
      OSDMenuInfoBoxShow(PROGRAM_NAME " " VERSION, "Integrity-Modus aktiviert.", 500);
      do
      {
        TAP_SystemProc();
        OSDMenuEvent(NULL, NULL, NULL);
      } while(OSDMenuInfoBoxIsVisible());
      OSDMenuInfoBoxDestroy();
    }
  }
  else
  {
    TAP_PrintNet("Fehler! Integrity-Modus NICHT aktiviert!");
    ShowErrorMessage("Fehler! Integrity-Modus NICHT aktiviert!", PROGRAM_NAME " " VERSION);
  }


  TRACEEXIT();
  return 0;
}

// ----------------------------------------------------------------------------
//                            TAP EventHandler
// ----------------------------------------------------------------------------
dword TAP_EventHandler(word event, dword param1, dword param2)
{
  TRACEENTER();

  // Behandlung offener MessageBoxen (rekursiver Aufruf, auch bei DoNotReenter)
  if(HIShowMessageBox)
  {
    if(OSDMenuMessageBoxIsVisible())
      OSDMenuEvent(&event, &param1, &param2);
    if(!OSDMenuMessageBoxIsVisible())
      HIShowMessageBox = FALSE;
    param1 = 0;
  }

  TRACEEXIT();
  return param1;
}
