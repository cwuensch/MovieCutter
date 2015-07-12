#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define __USE_LARGEFILE64  1
#define _FILE_OFFSET_BITS  64
#ifdef _MSC_VER
  #define __const const
#endif
//#define STACKTRACE      TRUE

#include                <stdio.h>
#include                <stdlib.h>
#include                <string.h>
#include                <unistd.h>
#include                <tap.h>
#include                <libFireBird.h>
//#include                "../source/CWTapApiLib.h"
#include                "SetupTAP.h"


TAP_ID                  (TAPID);
TAP_PROGRAM_NAME        (PROGRAM_NAME " Setup " VERSION);
TAP_PROGRAM_VERSION     (VERSION);
TAP_AUTHOR_NAME         (AUTHOR);
TAP_DESCRIPTION         (DESCRIPTION);
TAP_ETCINFO             (__DATE__);


extern byte data_start[] asm("_binary_jfs_fsck_start");
extern byte *data_end    asm("_binary_jfs_fsck_end");
extern size_t *data_size asm("_binary_jfs_fsck_size");

bool                    STShowMessageBox = FALSE;
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

  OSDMenuMessageBoxInitialize((TitleStr) ? TitleStr : PROGRAM_NAME " Setup", MessageStr);
  OSDMenuMessageBoxDoNotEnterNormalMode(TRUE);
  OSDMenuMessageBoxButtonAdd("OK");
  OSDMenuMessageBoxShow();
  STShowMessageBox = TRUE;
  while (STShowMessageBox)
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
  bool                  ret = FALSE;

  #if STACKTRACE == TRUE
    CallTraceInit();
    CallTraceEnable(TRUE);
    TRACEENTER();
  #endif

  FILE *f = NULL;
  if ((f = fopen(TAPFSROOT INSTALLDIR "/" PROGRAM_NAME, "wb")))
  {
    if (fwrite(data_start, 1, (data_end - data_start), f) == *data_size)
      ret = TRUE;
    fclose(f);
    
    system("chmod a+x " TAPFSROOT INSTALLDIR "/" PROGRAM_NAME);
    system("touch -c -d " PROGRAM_DATE " " TAPFSROOT INSTALLDIR "/" PROGRAM_NAME);
//    HDD_SetFileDateTime(PROGRAM_NAME, TAPFSROOT INSTALLDIR, PROGRAM_DATE);
  }

  if (ret)
  {
    TAP_PrintNet(PROGRAM_NAME " erfolgreich installiert.")
    if ((GetUptime() / 6000) >= 1)
    {
      OSDMenuInfoBoxShow(PROGRAM_NAME " Setup", PROGRAM_NAME " wurde installiert.", 500);
      do
      {
        TAP_SystemProc();
        OSDMenuEvent(NULL, NULL, NULL);
      } while(OSDMenuInfoBoxIsVisible());
      OSDMenuInfoBoxDestroy();
    }
    else
    {
      TAP_PrintNet("Fehler! " PROGRAM_NAME " konnte NICHT installiert werden!");
      ShowErrorMessage("Fehler! " PROGRAM_NAME " NICHT installiert!", PROGRAM_NAME " Setup");
    }
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
  if(STShowMessageBox)
  {
    if(OSDMenuMessageBoxIsVisible())
      OSDMenuEvent(&event, &param1, &param2);
    if(!OSDMenuMessageBoxIsVisible())
      STShowMessageBox = FALSE;
    param1 = 0;
  }

  TRACEEXIT();
  return param1;
}
