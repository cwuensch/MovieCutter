#define _LARGEFILE_SOURCE   1
#define _LARGEFILE64_SOURCE 1
#define _FILE_OFFSET_BITS  64
#ifdef _MSC_VER
  #define __const const
#endif
//#define STACKTRACE      TRUE

#define _GNU_SOURCE
#include                <stdio.h>
#include                <stdlib.h>
#include                <string.h>
#include                <unistd.h>
#include                <fcntl.h>
#include                <sys/stat.h>
#include                <sys/wait.h>
#include                <signal.h>
#include                <sys/ioctl.h>
#include                <tap.h>
#include                "libFireBird.h"   // <libFireBird.h>
#include                "RecHeader.h"
#include                "CWTapApiLib.h"
#include                "HddToolsLib.h"
#include                "MovieCutterLib.h"
#include                "MovieCutter_TAPCOM.h"
#include                "MovieCutter.h"
#include                "Graphics/ActionMenu9.gd"
#include                "Graphics/ActionMenu_Bar.gd"
#include                "Graphics/SubMenu_Bar.gd"
#include                "Graphics/SegmentList_Background.gd"
#include                "Graphics/SegmentList_ScrollBar.gd"
#include                "Graphics/SegmentList_ScrollButton.gd"
#include                "Graphics/Selection_Blue.gd"
//#include                "Graphics/Selection_Red.gd"
#include                "Graphics/Button_Ffwd_Active.gd"
#include                "Graphics/Button_Ffwd_Inactive.gd"
#include                "Graphics/Button_Pause_Active.gd"
#include                "Graphics/Button_Pause_Inactive.gd"
#include                "Graphics/Button_Play_Active.gd"
#include                "Graphics/Button_Play_Inactive.gd"
#include                "Graphics/Button_Rwd_Active.gd"
#include                "Graphics/Button_Rwd_Inactive.gd"
#include                "Graphics/Button_Slow_Active.gd"
#include                "Graphics/Button_Slow_Inactive.gd"
#include                "Graphics/Button_SkipLeft.gd"
#include                "Graphics/Button_SkipRight.gd"
#include                "Graphics/Button_Up.gd"
#include                "Graphics/Button_Up_small.gd"
#include                "Graphics/Button_Down.gd"
#include                "Graphics/Button_Down_small.gd"
#include                "Graphics/Button_red.gd"
//#include                "Graphics/Button_green.gd"
//#include                "Graphics/Button_yellow.gd"
//#include                "Graphics/Button_blue.gd"
#include                "Graphics/Button_white.gd"
#include                "Graphics/Button_recall.gd"
#include                "Graphics/Button_vf.gd"
#include                "Graphics/Button_menu.gd"
#include                "Graphics/Button_ProgPlusMinus.gd"
#include                "Graphics/Button_Exit.gd"
#include                "Graphics/Button_Ok.gd"
#include                "Graphics/BookmarkMarker.gd"
#include                "Graphics/BookmarkMarker_current.gd"
#include                "Graphics/BookmarkMarker_gray.gd"
#include                "Graphics/SegmentMarker.gd"
#include                "Graphics/SegmentMarker_current.gd"
#include                "Graphics/SegmentMarker_gray.gd"
//#include                "Graphics/PositionMarker.gd"
#include                "Graphics/PlayState_Background.gd"
#include                "Graphics/Icon_Ffwd.gd"
#include                "Graphics/Icon_Pause.gd"
#include                "Graphics/Icon_Playing.gd"
#include                "Graphics/Icon_Rwd.gd"
#include                "Graphics/Icon_Slow.gd"
#include                "Graphics/Icon_SegmentText.gd"
#include                "Graphics/Button_Sleep_small.gd"
#include                "Graphics/Button_1_small.gd"
#include                "Graphics/Button_2_small.gd"
#include                "Graphics/Button_3_small.gd"
#include                "Graphics/Button_4_small.gd"
#include                "Graphics/Button_5_small.gd"
#include                "Graphics/Button_6_small.gd"
#include                "Graphics/Button_7_small.gd"
#include                "Graphics/Button_8_small.gd"
//#include                "Graphics/Button_9_small.gd"
#include                "TMSCommander.h"
extern TYPE_GrData      _Button_Green_Gd, _Button_Yellow_Gd, _Button_Blue_Gd;
//extern TYPE_GrData      _Button_red_Gd, _Button_green_Gd, _Button_yellow_Gd, _Button_blue_Gd, _Button_white_Gd;
//extern TYPE_GrData      _Button_recall_Gd, _Button_menu_Gd, _Button_vf_Gd;

TAP_ID                  (TAPID);
TAP_PROGRAM_NAME        (PROGRAM_NAME" "VERSION);
TAP_PROGRAM_VERSION     (VERSION);
TAP_AUTHOR_NAME         (AUTHOR);
TAP_DESCRIPTION         (DESCRIPTION);
TAP_ETCINFO             (__DATE__);

extern char            *LangStrings;

// ============================================================================
//                              Definitionen
// ============================================================================
#ifdef MC_UNICODE
  tFontDataUC           Calibri_10_FontData;
  tFontDataUC           Calibri_12_FontData;
  tFontDataUC           Calibri_14_FontData;
  tFontDataUC           Courier_New_13_FontData;
  #define FM_GetStringHeight(...)   FMUC_GetStringHeight( __VA_ARGS__ )
  #define FM_GetStringWidth(...)    FMUC_GetStringWidth( __VA_ARGS__ )
  #define FM_PutString(...)         FMUC_PutString( __VA_ARGS__ )
  #define FM_PutStringAA(...)       FMUC_PutStringAA( __VA_ARGS__ )
#else
  extern tFontData      Calibri_10_FontData;
  extern tFontData      Calibri_12_FontData;
  extern tFontData      Calibri_14_FontData;
#endif

#define PLAYINFOVALID() (((int)PlayInfo.totalBlock > 0) && ((int)PlayInfo.currentBlock >= 0))

typedef struct
{
  bool                  Segment;  // TRUE for Segment-Event, FALSE for Bookmark-Event
  dword                 PrevBlockNr;
  dword                 NewBlockNr;
  bool                  SegmentWasSelected;
  char                 *pPrevCaption;
} tUndoEvent;

typedef struct
{
  byte                  Version;
  __off64_t             RecFileSize;
  dword                 NrSegmentMarker;
  dword                 ActiveSegment;
}__attribute__((packed)) tCutHeader1;
typedef struct
{
  word                  Version;
  __off64_t             RecFileSize;
  word                  NrSegmentMarker;
  word                  ActiveSegment;
  word                  Padding;
}__attribute__((packed)) tCutHeader2;

typedef enum
{
  ST_Init,               //                      // TAP start (executed only once)
  ST_WaitForPlayback,    // [ST_IdleNoPlayback]  // TAP inactive, changes to active mode, as soon as a playback starts
  ST_ActiveOSD,          // [ST_Idle]            // OSD is active (playback running)
  ST_InactiveModePlaying,// [ST_IdleInvisible]   // OSD is hidden, checks if playback stopps or changes to start AutoOSD
  ST_InactiveMode,       //   "       "          // OSD is hidden and has to be manually activated
  ST_UnacceptedFile,     // ->[ST_Inact.Playing] // TAP is not active and cannot be entered (playback of unsupported file)
  ST_ActionMenu,         // [ST_ActionDialog]    // Action Menu is open, navigation only within menu
  ST_ActionSubMenu,      //                      // Sub Menu for selection of RecStrip options is open
//  ST_CutFailDialog,    //                      // Show the failure dialog
//  ST_RecStrip,         //                      // RecStrip executing, wait to finish
  ST_Exit,               //                      // Preparing to exit TAP
  ST_ExitNoSave
} tState;

typedef enum
{
  MI_SelectFunction,
  MI_SaveSegments,
  MI_DeleteSegments,
  MI_CopySegSplitMovie,
  MI_SelectEvOddSegments,
  MI_ClearAll,
  MI_ImExportBookmarks,
  MI_RenameMovie,
  MI_RecStripCheckFS,
  MI_ExitMC,
  MI_NrMenuItems
} tMenuItem;

typedef enum
{
  MD_NoOSD,
  MD_FullOSD,
  MD_NoSegmentList,
  MD_MiniOSD
} tOSDMode;

typedef enum
{
  RC_auto,
  RC_SRP2401,
  RC_SRP2410,
  RC_CRP,
  RC_TF5000,
  RC_NoVolKeys
} tRCUMode;

typedef enum
{
  CM_Both,
  CM_CutOnly,
  CM_InfOnly
} tCutSaveMode;

typedef enum
{
  IM_Never,
  IM_ROEnd,
  IM_RWEnd,
  IM_ROBetween,
  IM_RWBetween
} tiCheckMode;

typedef enum
{
  FM_Never,
  FM_Auto,
  FM_Always,
  FM_Shutdown
} tCheckFSMode;

typedef enum
{
 // Symbols in info bar
  LS_Add,
  LS_Delete,
  LS_Exit,
  LS_Move,
  LS_PauseMenu,
  LS_Select,
  LS_NoNavMessage,   // error message
 // Menu entries (1)
  LS_DeleteSegments,
  LS_DeleteFile,
  LS_ExitMC,
  LS_FailedResolve,  // error message
 // Menu entries (2)
  LS_GotoNextBM,
  LS_GotoPrevBM,
  LS_ImportBM,
  LS_OK,             // dialog button
 // Menu entries (3)
  LS_RemovePadding,
  LS_SaveSegments,
  LS_SegmentList,
  LS_SelectFunction,
  LS_CutHasFailed,   // error message
  LS_IsCrypted,      // error message
  LS_Cutting,
 // Menu entries (4)
  LS_DeleteOddSegments,
  LS_DeleteEvenSegments,
 // new language strings V2.0
  LS_SaveNrSegments,
  LS_Save1Segment,
  LS_SaveCurSegment,
  LS_DeleteNrSegments,
  LS_Delete1Segment,
  LS_DeleteCurSegment,
  LS_SelectOddSegments,
  LS_SelectPadding,
  LS_SelectEvenSegments,
  LS_SelectMiddle,
  LS_UnselectAll,
  LS_ClearSegmentList,
  LS_DeleteAllBookmarks,
  LS_ExportToBM,
  LS_Segments,
  LS_Bookmarks,
  LS_PageStr,
  LS_BeginStr,
  LS_EndStr,
  LS_AskConfirmation,
  LS_Yes,
  LS_No,
  LS_ListIsFull,
  LS_NoRecSize,
  LS_HDDetectionFailed,
  LS_NavLoadFailed,
  LS_NavLengthWrong,
  LS_RebootMessage,
  LS_NavPatched,
  LS_FileSystemCheck,
  LS_CheckingFileSystem,
  LS_CheckFSFailed,
  LS_CheckFSAborted,
  LS_CheckFSSuccess,
  LS_SuspectFilesFound,
  LS_FilesNotVerified,
  LS_FileNameTooLong,
  LS_Warning,
  LS_Undo,
  LS_ChangeMode,
  LS_FastNav,
  LS_OSD,
  LS_BM,
  LS_B,
  LS_S,
 // User notifications (10)
  LS_MovieCutterActive,
  LS_SegmentMode,
  LS_BookmarkMode,
  LS_SegMarkerCreated,
  LS_SegMarkerMoved,
  LS_SegMarkerDeleted,
  LS_BookmarkCreated,
  LS_BookmarkMoved,
  LS_BookmarkDeleted,
  LS_SegmentSelected,
  LS_SegmentUnselected,
  LS_UndoLastAction,
  LS_MinuteJumpActive,
  LS_MinuteJumpDisabled,
  LS_NotEnoughSpace,
  LS_UnknownSystemType,
 // Menu entries (5)
  LS_SplitMovie,
  LS_RenameMovie,
 // Keyboard and Renaming (12)
  LS_NameInvalid,
  LS_NameAlreadyExists,
  LS_RenamingFailed,
  LS_KeybSpace,
  LS_KeybDelete1,
  LS_KeybDelete2,
  LS_KeybOriginal,
  LS_KeybClearAll,
  LS_KeybCopy,
  LS_KeybPaste,
  LS_KeybCancel,
  LS_KeybSave,
 // Strippen und Teile kopieren (8)
  LS_SegmentNr,
  LS_EnterText,
  LS_CopyNrSegments,
  LS_Copy1Segment,
  LS_CopyCurSegment,
  LS_StripRec,
  LS_StrippedRec,
  LS_StrippingMsg,
  LS_StripSuccess,
  LS_StripFailed,
  LS_AbortRecStrip,
  LS_ShutdownAfterRS,
  LS_CopySeparate,
  LS_CopyCommon,
  LS_CopySeparateStrip,
  LS_CopyCommonStrip,
  LS_NrStrings
} tLngStrings;

static char* DefaultStrings[LS_NrStrings] =
{
  "Neu",
  "Löschen",
  "Ausblenden",
  "Verschieben",
  "Pause/Menü",
  "Auswählen",
  "Keine nav-Datei! Zeitangaben evtl. ungenau.\nTrotzdem fortfahren?",
  "",   // Ausgewählte Segmente löschen
  "",   // Diese Datei löschen
  "MovieCutter beenden",
  "",   // Konnte F/W-Funktion _bookmarkTime nicht finden.
  "",   // Nächster Bookmark
  "",   // Vorhergehender Bookmark
  "Import: Bookmarks -> Segmente",
  "OK",
  "Löschen: Vor-/Nachlauf",
  "",   // Ausgewählte Segmente speichern
  "Segmente",
#ifdef MC_UNICODE
  "â€• Funktion auswählen â€•",
#else
  "-- Funktion auswählen --",
#endif
  "Der Schnitt ist fehlgeschlagen!\nBitte das Log prüfen!",
  "Die Aufnahme ist (teilweise) verschlüsselt.",
  "Schneide Aufnahme...\n(Schnitt %d von %d)",
  "Löschen: Ungerade Segmente",
  "Löschen: Gerade Segmente",   // Gerade Segmente löschen
  "Speichern: %d markierte Segmente",
  "Speichern: Markiertes Segment",
  "Speichern: Aktuelles Segment",
  "Löschen: %d markierte Segmente",
  "Löschen: Markiertes Segment",
  "Löschen: Aktuelles Segment",
  "Markieren: Ungerade Segmente",
  "Markieren: Vor-/Nachlauf",
  "Markieren: Gerade Segmente",
  "Markieren: Mittelstück",
  "Auswahl zurücksetzen",
  "Schnittmarken löschen",
  "Bookmarks löschen",
  "Export: Segmente -> Bookmarks",
  "Segmente",
  "Bookmarks",
  "Seite",
  "Anfang",
  " Ende",
  "Sind Sie sicher?",
  "Ja",
  "Nein",
  "Liste der Segment-Marker ist voll.",
  "Aufnahme-Datei nicht gefunden.",
  "HD-Video Erkennung fehlgeschlagen.",
  "Die .nav-Datei konnte nicht geladen werden.",
  "Länge der .nav weicht um %d sek. ab.\nTrotzdem fortfahren?",
  "Receiver vor dem Schneiden neu starten!\nTrotzdem fortfahren?",
  ".nav-Datei scheint inkorrekt. Repariert!\nWird erneut geladen...",
  "Dateisystem überprüfen",
  "Dateisystem wird geprüft...",
  "Es wurden Fehler gefunden!\n%s Dateien suspekt, %%d von %%d in Reparatur. [%%s]%%s\n(insgesamt überwachte Dateien: %%d)",
  "Prüfung des Dateisystems abgebrochen.",
  "Dateisystem scheint einwandfrei.\n\n(aktuell überwachte Dateien: %d)",
  "%s suspekte Datei(en) entdeckt.\nBitte das Dateisystem überprüfen!",
  "Geschnittene Dateien nicht verifiziert.\nBitte das Dateisystem überprüfen!",
  "Dateiname oder -pfad ist zu lang!",
  "Warnung",
  "Rückgängig",
  "Modus",
  "FastNav",
  "OSD",
  "BM",
  "B",
  "S",
  "MovieCutter aktiv",
  "Segment-Modus",
  "Bookmark-Modus",
  "Schnittmarke erzeugt",
  "Schnittmarke bewegt",
  "Schnittmarke gelöscht",
  "Bookmark angelegt",
  "Bookmark verschoben",
  "Bookmark gelöscht",
  "Segment markiert",
  "Selektion aufgehoben",
  "Letzte rückgängig",
  "Minutensprung ein: %d'",
  "Minutensprung aus",
  "Wenig freier Festplattenspeicher!\nTrotzdem fortfahren?",
  "SystemType ist unbekannt.\nBitte die FirmwareTMS.dat überprüfen!",
  "Aufnahme hier teilen",
  "Aufnahme umbenennen",
  "Eingegebener Dateiname ist ungültig.\nZeichen '/' ist nicht erlaubt.",
  "Neuer Dateiname bereits vorhanden!\nÜberschreiben?",
  "Umbenennen der Datei fehlgeschlagen.",
  "Leer",
  "Lös",
  "chen",
  "Original",
  "Alles löschen",
  "Kopieren",
  "Einfügen",
  "Abbrechen",
  "Speichern",
  "%hd. Segment",
  "Text eingeben",
  "Teile kopieren: %d markierte Seg.",
  "Teile kopieren: Markiertes Segm.",
  "Teile kopieren: Aktuelles Segment",
  "Aufnahme strippen",
  "Aufnahme bereits gestrippt",
  "Kopie erstellen...",
  "Aktion erfolgreich abgeschlossen.\n\n%.1f von %.1f MB (%.1f %%) kopiert.",
  "Kopieren ist fehlgeschlagen!\nBitte das Log prüfen!",
  "Strippen jetzt abbrechen?",
  "(Dialog stehen lassen, um zu warten.)",
  "Separat in einzelne Rec-Files",
  "Gemeinsam in eine Aufnahme",
  "Separat, mit Strippen",
  "Gemeinsam, mit Strippen"
};
#ifdef MC_MULTILANG
  #define LangGetString(x)  LangGetStringDefault(x, DefaultStrings[x])
#else
  #define LangGetString(x)  DefaultStrings[x]  
#endif

// Konstanten
static const int        ScreenWidth = 720;
static const int        ScreenHeight = 576;
static const dword      ColorInfoBarTitle      = RGB(51, 51, 51);
static const dword      ColorDarkBackground    = RGB(16, 16, 16);
static const dword      ColorLightBackground   = RGB(24, 24, 24);
static const dword      ColorInfoBarDarkSub    = RGB(30, 30, 30);
static const dword      ColorInfoBarLightSub   = RGB(43, 43, 43);
static const int        InfoBarRightAreaWidth  = 190;
static const int        InfoBarModeAreaWidth   = 72;
static const int        InfoBarLine1_Y         = 49,   InfoBarLine1Height = 30;
static const int        InfoBarLine2_Y         = 84;
static const int        InfoBarLine3_Y         = 128;  // alternativ: 135

static TYPE_GrData* ShortButtons[]       =  {&_Button_1_small_Gd, &_Button_2_small_Gd, &_Button_3_small_Gd, &_Button_4_small_Gd, &_Button_5_small_Gd, &_Button_6_small_Gd, &_Button_7_small_Gd, &_Button_8_small_Gd, &_Button_Sleep_small_Gd};


// MovieCutter INI-Flags
static bool             AutoOSDPolicy      = FALSE;
static tOSDMode         DefaultOSDMode     = MD_FullOSD;
static int              DefaultMinuteJump  = 0;
static bool             ShowRebootMessage  = TRUE;
static dword            MaxNavDiscrepancy  = 5000;
static bool             AskBeforeEdit      = TRUE;
static tCutSaveMode     CutFileMode        = CM_Both;   // [0] in cut- und inf-Datei, [1] nur cut-Datei, [2] nur inf-Datei
static bool             SaveCutBak         = TRUE;
static int              DeleteCutFiles     = 1;         // Verweiste .cut-Files löschen: [0] nie, [1] in /DataFiles, [2] auch rekursiv in Unterverzeichnissen
static bool             ForceSpecialEnd    = FALSE;     // TRUE = immer umgekehrte Endbehandlung, FALSE = nur, wenn ungefährlich (< 4 GB)
static bool             DisableSpecialEnd  = FALSE;     // TRUE = niemals umgekehrte Endbehandlung, überschreibt ForceSpecialEnd
static tiCheckMode      DoiCheckTest       = IM_ROEnd;  // [0] nie, [1] gesammelt am Ende (ro), [2] gesammelt am Ende (fix), [3] nach jedem Schnitt (ro), [4] nach jedem Schnitt (fix)
static tCheckFSMode     CheckFSAfterCut    = FM_Auto;   // [0] nie, [1] auto, [2] immer, [3] beim Beenden
static bool             InodeMonitoring    = FALSE;
static int              StripMode          = 1;         // [0] normal, [1] EPG strippen, [2] Teletext strippen, [3] beides
static dword            Overscan_X         = 56;  // 50
static dword            Overscan_Y         = 25;
static dword            SegmentList_X      = 55;
static dword            SegmentList_Y      = 78;
static tRCUMode         RCUMode            = RC_SRP2401;
static bool             DirectSegmentsCut  = FALSE;
static bool             DisableSleepKey    = FALSE;


// MovieCutter state variables
static tState           State = ST_Init;
static tOSDMode         OSDMode = MD_NoOSD;
static bool             BookmarkMode;
static bool             MediaFileMode = FALSE;
static bool             LinearTimeMode = FALSE;
//static bool             MCShowMessageBox = FALSE;
static bool             OldRepeatMode = FALSE;
static bool             ActionMenuEvenOdd;
static bool             ShowSegmentText = FALSE;
static TYPE_TrickMode   TrickMode;
static byte             TrickModeSpeed;
static int              MinuteJump;                           //Seconds or 0 if deactivated
//static bool             NoPlaybackCheck = FALSE;              //Used to circumvent a race condition during the cutting process
static int              NrSelectedSegments;
static word             JumpRequestedSegment = 0xFFFF;        //Is set, when the user presses up/down to jump to another segment
static dword            JumpRequestedBlock = (dword) -1;      //Is set, when user presses Vol+/- to navigate in the progressbar
static dword            JumpRequestedTime = 0;                //Is set, when one of the options on top is active
static dword            JumpPerformedTime = 0;                //Is set after a segment jump has been performed to reduce flicker
static tUndoEvent      *UndoStack = NULL;
static int              UndoLastItem;
static char            *SuspectHDDs = NULL;
static int              NrAllSuspectInodes = 0;

// Playback information
static TYPE_PlayInfo    PlayInfo;
static char             PlaybackName[MAX_FILE_NAME_SIZE + 1];
static char             AbsPlaybackDir[FBLIB_DIR_SIZE];
static __off64_t        RecFileSize = 0;
static dword            RecDateTime = 0;
static dword            LastTotalBlocks = 0;
static dword            BlocksOneSecond;
static dword            BlockNrLastSecond;
static dword            BlockNrLast10Seconds;

// Video parameters
static tSegmentMarker  *SegmentMarker = NULL;       //[0]=Start of file, [x]=End of file
static dword           *Bookmarks = NULL;
static int              NrSegmentMarker;
static int              ActiveSegment;
static int              NrBookmarks = 0;
static int              NrTimeStamps = 0;
static tTimeStamp      *TimeStamps = NULL;
static tTimeStamp      *LastTimeStamp = NULL;
static bool             HDVideo, isStripped;

// OSD object variables
static word             rgnSegmentList = 0;
static word             rgnInfoBar = 0;
static word             rgnInfoBarMini = 0;
static word             rgnPlayState = 0;
static word             rgnTextState = 0;
static word             rgnActionMenu = 0;
static word             rgnActionSubMenu = 0;
static word             rgnSegmentText = 0;
static word             rgnStripProgBar = 0;
static int              ActionMenuItem, ActionSubMenuItem;
static dword            LastPlayStateChange = 0;
static int              SegmentTextStartLine = 0;
static bool             jfs_fsck_present = FALSE;
static bool             RecStrip_present = FALSE, RecStrip_active = FALSE;
static int              RecStrip_DoCut = FALSE;
static pid_t            RecStrip_Pid = 0;
static int              RecStrip_Percent = 0;


static inline dword currentVisibleBlock()
{
  return ((JumpRequestedBlock != (dword) -1) ? JumpRequestedBlock : PlayInfo.currentBlock);
}

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

  if(HDD_TAP_CheckCollisionByID(TAPID))
  {
    TAP_PrintNet("MovieCutter: Duplicate instance of the same TAP already started!\r\n");
    TRACEEXIT();
    return 0;
  }

  char HDDModel[41], HDDSerial[21], HDDFirmware[9];

  KeyTranslate(TRUE, &TAP_EventHandler);
  CreateSettingsDir();

  WriteLogMC (PROGRAM_NAME, "***  MovieCutter " VERSION " started! (FBLib " __FBLIB_VERSION__ ") ***");
  WriteLogMC (PROGRAM_NAME, "=======================================================");
  WriteLogMCf(PROGRAM_NAME, "Receiver Model: %s (%u), System Type: TMS-%c (%d)", GetToppyString(GetSysID()), GetSysID(), SysTypeToStr(), GetSystemType());
  WriteLogMCf(PROGRAM_NAME, "Firmware: %s", GetApplVer());
  if (HDD_GetHddID(HDDModel, HDDSerial, HDDFirmware))
    WriteLogMCf(PROGRAM_NAME, "Hard disk: %s, FW %s, Serial: %s", HDDModel, HDDFirmware, HDDSerial);
  jfs_fsck_present = HDD_Exist2("jfs_fsck", FSCKPATH);
  if(jfs_fsck_present)
    chmod(FSCKPATH "/jfs_fsck", 0777);
//    system("chmod a+x " FSCKPATH "/jfs_fsck &");
  else
    WriteLogMC(PROGRAM_NAME, "WARNING! '" FSCKPATH "/jfs_fsck' not found.");
  RecStrip_present = HDD_Exist2("RecStrip", RECSTRIPPATH);
  if(RecStrip_present)
    chmod(FSCKPATH "/RecStrip", 0777);

  // Load Fonts
  #ifdef MC_UNICODE
    if (!(FMUC_LoadFontFile("Calibri_10.ufnt", &Calibri_10_FontData)
       && FMUC_LoadFontFile("Calibri_12.ufnt", &Calibri_12_FontData)
       && FMUC_LoadFontFile("Calibri_14.ufnt", &Calibri_14_FontData)
       && FMUC_LoadFontFile("Courier_New_13.ufnt", &Courier_New_13_FontData)))
    {
      WriteLogMC(PROGRAM_NAME, "Loading fonts failed!\r\n");
      FMUC_FreeFontFile(&Calibri_10_FontData);
      FMUC_FreeFontFile(&Calibri_12_FontData);
      FMUC_FreeFontFile(&Calibri_14_FontData);
      FMUC_FreeFontFile(&Courier_New_13_FontData);

      CloseLogMC();
      TRACEEXIT();
      return 0;
    }
  #endif

  // Load Language Strings
  #ifdef MC_MULTILANG
    if (TAP_GetSystemVar(SYSVAR_OsdLan) != LAN_German)
      if(!LangLoadStrings(LNGFILENAME, LS_NrStrings, LAN_English, PROGRAM_NAME))
      {
/*        char LogString[512];
        TAP_SPrint(LogString, sizeof(LogString), "Language file '%s' not found!", LNGFILENAME);
        WriteLogMCf(PROGRAM_NAME, "%s\r\n", LogString);
        OSDMenuInfoBoxShow(PROGRAM_NAME " " VERSION, LogString, 500);
        do
        {
          OSDMenuEvent(NULL, NULL, NULL);
        } while(OSDMenuInfoBoxIsVisible());

        #ifdef MC_UNICODE
          FMUC_FreeFontFile(&Calibri_10_FontData);
          FMUC_FreeFontFile(&Calibri_12_FontData);
          FMUC_FreeFontFile(&Calibri_14_FontData);
          FMUC_FreeFontFile(&Courier_New_13_FontData);
          OSDMenuFreeStdFonts();
        #endif

        CloseLogMC();
        TRACEEXIT();
        return 0;  */
      }
  #endif

  // Check FirmwareTMS.dat and SystemType
  if((GetSystemType() != ST_TMSS) && (GetSystemType() != ST_TMSC) && (GetSystemType() != ST_TMST))
  {
    WriteLogMC(PROGRAM_NAME, "Unknown SystemType. Please check FirmwareTMS.dat!\r\n");
    OSDMenuInfoBoxShow(PROGRAM_NAME " " VERSION, LangGetString(LS_UnknownSystemType), 500);
    do
    {
      OSDMenuEvent(NULL, NULL, NULL);
    } while(OSDMenuInfoBoxIsVisible());

    #ifdef MC_UNICODE
      FMUC_FreeFontFile(&Calibri_10_FontData);
      FMUC_FreeFontFile(&Calibri_12_FontData);
      FMUC_FreeFontFile(&Calibri_14_FontData);
      FMUC_FreeFontFile(&Courier_New_13_FontData);
      OSDMenuFreeStdFonts();
    #endif

    CloseLogMC();
    TRACEEXIT();
    return 0;
  }

  // Allocate Buffers
  SegmentMarker = (tSegmentMarker*) TAP_MemAlloc(NRSEGMENTMARKER * sizeof(tSegmentMarker));
  Bookmarks     = (dword*)          TAP_MemAlloc(NRBOOKMARKS     * sizeof(dword));
  UndoStack     = (tUndoEvent*)     TAP_MemAlloc(NRUNDOEVENTS    * sizeof(tUndoEvent));
  SuspectHDDs   = (char*)           TAP_MemAlloc(SIZESUSPECTHDDS);
  if (!SegmentMarker || !Bookmarks || !UndoStack || !SuspectHDDs)
  {
    WriteLogMC(PROGRAM_NAME, "Failed to allocate buffers!\r\n");

    TAP_MemFree(SuspectHDDs);
    TAP_MemFree(UndoStack);
    TAP_MemFree(Bookmarks);
    TAP_MemFree(SegmentMarker);
    #ifdef MC_MULTILANG
      LangUnloadStrings();
    #endif
    #ifdef MC_UNICODE
      FMUC_FreeFontFile(&Calibri_10_FontData);
      FMUC_FreeFontFile(&Calibri_12_FontData);
      FMUC_FreeFontFile(&Calibri_14_FontData);
      FMUC_FreeFontFile(&Courier_New_13_FontData);
    #endif

    CloseLogMC();
    TRACEEXIT();
    return 0;
  }

  // Reset Buffers
  memset(SegmentMarker, 0, NRSEGMENTMARKER * sizeof(tSegmentMarker));
  memset(UndoStack,     0, NRUNDOEVENTS    * sizeof(tUndoEvent));
  memset(SuspectHDDs,   0, SIZESUSPECTHDDS);

  TRACEEXIT();
  return 1;
}

// ----------------------------------------------------------------------------
//                            TAP EventHandler
// ----------------------------------------------------------------------------
dword TAP_EventHandler(word event, dword param1, dword param2)
{
  dword                 SysState, SysSubState;
  static bool           DoNotReenter = FALSE;
  static tOSDMode       LastOSDMode = MD_FullOSD;
  static dword          LastMinuteKey = 0;
//  static dword          LastDraw = 0;
  bool                  WasNotReenter = DoNotReenter;

  TRACEENTER();
  #if STACKTRACE == TRUE
    if (event != EVT_IDLE)
      TAP_PrintNet("Event: event=%d, param1=%ld, param2=%ld\n", event, param1, param2);
    TAP_PrintNet("Status = %u\n", State);
  #endif

  // Behandlung offener MessageBoxen (rekursiver Aufruf, auch bei DoNotReenter)
//  if(MCShowMessageBox)
//  {
    if(OSDMenuMessageBoxIsVisible())
    {
      #ifdef __ALTEFBLIB__
        OSDMenuMessageBoxDoScrollOver(&event, &param1);
      #endif
      OSDMenuEvent(&event, &param1, &param2);
      param1 = 0;
    }
//    if(!OSDMenuMessageBoxIsVisible())
//      MCShowMessageBox = FALSE;
//  }

  // Behandlung der OSD-Tastatur, falls offen (auch bei DoNotReenter?)
  if(OSDMenuKeyboard_isVisible())
  {
    OSDMenuKeyboard_EventHandler(&event, &param1, &param2);
    param1 = 0;
  }

  // Abbruch von fsck ermöglichen (selbst bei DoNotReenter)
  if(DoNotReenter && OSDMenuProgressBarIsVisible())
  {
    if((event==EVT_KEY && (param1==RKEY_Exit || param1==FKEY_Exit || param1==RKEY_Sleep)) || (event==EVT_STOP && param1!=2))
      HDD_CancelCheckFS();
    param1 = 0;
  }


  // Vorsicht! Ausnahme von DoNotReenter für den Fall, dass RecStrip arbeitet und der "RecStrip abbrechen?"-Dialog geöffnet ist
  if(DoNotReenter && !(RecStrip_active && (event==EVT_IDLE || event==EVT_STOP)))
  {
    TRACEEXIT();
    return param1;
  }
  DoNotReenter = TRUE;


  // Stop-Event (PowerOff) wird nur abgefangen, wenn kein DoNotReenter aktiv ist
  if((event == EVT_STOP) && (param1 != 2) && !RecStrip_active)
  {
    State = (State==ST_ActiveOSD || State==ST_ActionMenu || State==ST_ActionSubMenu) ? ST_Exit : ST_ExitNoSave;
  }

  if(event == EVT_TMSCommander)
  {
    param1 = TMSCommander_handler(param1);
  }

  if(event == EVT_TAPCOM)
  {
    TAPCOM_Channel  Channel;
    dword           ServiceID;

    Channel = TAPCOM_GetChannel(param1, NULL, &ServiceID, NULL, NULL);
    if(ServiceID == TAPCOM_SERVICE_ISOSDVISIBLE)
      TAPCOM_Finish(Channel, (State==ST_ActiveOSD || State==ST_ActionMenu || State==ST_ActionSubMenu || State==ST_Exit) ? 1 : 0);
    else
      TAPCOM_Reject(Channel);
  }

  // Notfall-AUS
#ifdef FULLDEBUG
  if((event == EVT_KEY) && (param1 == RKEY_Sleep) && !DisableSleepKey && !RecStrip_active)
  {
//    if (OSDMenuMessageBoxIsVisible()) OSDMenuMessageBoxDestroy();

//    TAP_EnterNormal();
    State = (State==ST_ActiveOSD || State==ST_ActionMenu || State==ST_ActionSubMenu) ? ST_Exit : ST_ExitNoSave;
    param1 = 0;
  }

/* // **** LÖSCHEN ****
  if((event == EVT_KEY) && (param1 == RKEY_Sat) && (State==ST_ActiveOSD || State==ST_InactiveModePlaying || State==ST_InactiveMode || State==ST_WaitForPlayback || State==ST_UnacceptedFile) && !DisableSleepKey)
  {
    CheckFileSystem((AbsPlaybackDir[0] ? AbsPlaybackDir : TAPFSROOT), 0, 1, 1, TRUE, TRUE, FALSE, FALSE, 0, NULL);
  } */
#endif

  if(!WasNotReenter)
  switch(State)
  {
    // Initialisierung bei TAP-Start
    // -----------------------------
    case ST_Init:
    {
/*
      LastTotalBlocks = 0;
      MCShowMessageBox = FALSE;
      NrTimeStamps = 0;
      TimeStamps = NULL;
      LastTimeStamp = NULL;
      JumpRequestedSegment = 0xFFFF;
      JumpRequestedBlock = (dword) -1;
      JumpRequestedTime = 0;
      JumpPerformedTime = 0;
*/
      // Load INI
      LoadINI();
      WriteLogMCf(PROGRAM_NAME, "Options: SpecialEnd=%s, DoiCheckTest=%d, CheckFSAfterCut=%d, InodeMonitoring=%d", (!DisableSpecialEnd ? (ForceSpecialEnd ? "always" : "only_safe") : "never"), DoiCheckTest, CheckFSAfterCut, InodeMonitoring);
      OSDMode = DefaultOSDMode;

      // Set the system time to current time
//      /*if (InodeMonitoring)*/ SetSystemTimeToCurrent();

      State = AutoOSDPolicy ? ST_WaitForPlayback : ST_InactiveMode;

      // Fix list of defect inodes
      if(InodeMonitoring) HDD_FixInodeList(TAPFSROOT, TRUE);  // Problem: NUR für die interne HDD wird die Liste jemals zurückgesetzt!

      CleanupCut();
      CloseLogMC();
      break;
    }

    // Warten / Aufnahme analysieren
    // -----------------------------
    case ST_WaitForPlayback:    // Idle loop while there is no playback active, shows OSD as soon as playback starts
    {
      if(isPlaybackRunning())
      {
        param1 = 0;
        if(!PlayInfo.file || !PlayInfo.file->name[0]) break;
        if(((int)PlayInfo.totalBlock <= 0) || ((int)PlayInfo.currentBlock < 0)) break;

        TAP_GetState(&SysState, &SysSubState);
        if(SysState != STATE_Normal || (SysSubState != SUBSTATE_Normal && SysSubState != SUBSTATE_PvrPlayingSearch && SysSubState != 0)) break;

//        OSDMode = DefaultOSDMode;  // unnötig
        BookmarkMode = FALSE;
        ResetSegmentMarkers();
        ActiveSegment = 0;
        MinuteJump = DefaultMinuteJump;
        JumpRequestedSegment = 0xFFFF;    // eigentlich unnötig
        JumpRequestedBlock = (dword) -1;  //   "
//        JumpRequestedTime = 0;            //   "
//        NoPlaybackCheck = FALSE;
        LastTotalBlocks = PlayInfo.totalBlock;
//        UndoResetStack();  // unnötig

        OldRepeatMode = PlaybackRepeatGet();
        PlaybackRepeatSet(TRUE);

        //Flush the caches *experimental*
//        sync();
//        TAP_Sleep(1);

        WriteLogMC(PROGRAM_NAME, "========================================\r\n");

        //Identify the file name (.rec or .mpg)
        TAP_SPrint(PlaybackName, sizeof(PlaybackName), PlayInfo.file->name);
        if (strcmp(&PlaybackName[strlen(PlaybackName) - 4], ".inf") == 0)
          PlaybackName[strlen(PlaybackName) - 4] = '\0';
        else
          MediaFileMode = TRUE;

        //Find out the absolute path to the rec file and check for max length
        HDD_GetAbsolutePathByTypeFile2(PlayInfo.file, AbsPlaybackDir);
        if((strlen(PlaybackName) + 18 >= MAX_FILE_NAME_SIZE) || (strlen(AbsPlaybackDir) + 14 >= FBLIB_DIR_SIZE))  // 18 = ' (Cut-123)' + '.nav.bak' | 14 = ' (Cut-123)' + '.bak'
        {
          State = ST_UnacceptedFile;
          WriteLogMC(PROGRAM_NAME, "File name or path is too long!");
          ShowErrorMessage(LangGetString(LS_FileNameTooLong), NULL);
          PlaybackRepeatSet(OldRepeatMode);
          ClearOSD(TRUE);
          break;
        }

        //Save only the absolute path to rec folder
        char *p;
        p = strstr(AbsPlaybackDir, PlaybackName);
        if(p) *(p-1) = '\0';

//        HDD_ChangeDir(PlaybackDir);
        if (strncmp(AbsPlaybackDir, TAPFSROOT, strlen(TAPFSROOT)) == 0)
          HDD_ChangeDir(&AbsPlaybackDir[strlen(TAPFSROOT)]);

        WriteLogMCf(PROGRAM_NAME, "Attaching to %s/%s", AbsPlaybackDir, PlaybackName);
        WriteLogMC (PROGRAM_NAME, "----------------------------------------");

        //Free the old timing array, so that it is empty (NULL pointer) if something goes wrong
        if(TimeStamps)
        {
          TAP_MemFree(TimeStamps);
          TimeStamps = NULL;
        }
        NrTimeStamps = 0;
        LastTimeStamp = NULL;

        // Detect rec file size and packet size
        __ino64_t InodeNr = 0;
        int PacketSize;
        RecFileSize = 0;
        if ( (!HDD_GetFileSizeAndInode2(PlaybackName, AbsPlaybackDir, &InodeNr, &RecFileSize) || !RecFileSize)
          || ((PacketSize = GetPacketSize(PlaybackName, AbsPlaybackDir)) == 0) )
        {
          State = ST_UnacceptedFile;
          WriteLogMC(PROGRAM_NAME, ".rec size or packet size could not be detected!");
          if(!RecFileSize)
            ShowErrorMessage(LangGetString(LS_NoRecSize), NULL);
          PlaybackRepeatSet(OldRepeatMode);
          ClearOSD(TRUE);
          break;
        }
        WriteLogMCf(PROGRAM_NAME, "Inode Nr   = %llu", InodeNr);
        WriteLogMCf(PROGRAM_NAME, "File size  = %llu Bytes (%lu blocks)", RecFileSize, (dword)(RecFileSize / BLOCKSIZE));
        WriteLogMCf(PROGRAM_NAME, "PacketSize = %d", PacketSize);
        WriteLogMCf(PROGRAM_NAME, "Reported total blocks: %lu", PlayInfo.totalBlock);

        //Read video information from inf
        bool infDetected, isCrypted;
        HDVideo = FALSE; isStripped = FALSE; RecDateTime = 0;
        infDetected = GetRecInfosFromInf(PlaybackName, AbsPlaybackDir, &isCrypted, &HDVideo, &isStripped, &RecDateTime);
        
        //Check if file is crypted
        if (infDetected && isCrypted)
        {
          State = ST_UnacceptedFile;
          WriteLogMC(PROGRAM_NAME, "File is crypted!");
          ShowErrorMessage(LangGetString(LS_IsCrypted), NULL);
          PlaybackRepeatSet(OldRepeatMode);
          ClearOSD(TRUE);
          break;
        }

        //Check if a nav is available
        if (MediaFileMode || !isNavAvailable(PlaybackName, AbsPlaybackDir))
        {
          if (MediaFileMode || ShowConfirmationDialog(LangGetString(LS_NoNavMessage)))
          {
            WriteLogMC(PROGRAM_NAME, ".nav file not found! Using linear time mode...");
            LinearTimeMode = TRUE;
          }
          else
          {
            WriteLogMC(PROGRAM_NAME, ".nav is missing!");
            PlaybackRepeatSet(OldRepeatMode);
            ClearOSD(TRUE);
            if (AutoOSDPolicy)
              State = ST_UnacceptedFile;
            else
            {
              Cleanup(FALSE);
              State = ST_InactiveMode;
            }
            break;
          }
        }

        // Detect if video stream is in HD
        if (!LinearTimeMode && !infDetected)
        {
          State = ST_UnacceptedFile;
          WriteLogMC(PROGRAM_NAME, "Could not detect type of video stream!");
          ShowErrorMessage(LangGetString(LS_HDDetectionFailed), NULL);
          PlaybackRepeatSet(OldRepeatMode);
          ClearOSD(TRUE);
          break;
        }
        WriteLogMCf(PROGRAM_NAME, "Type of recording: %s", (HDVideo) ? "HD" : "SD");

        // Try to load the nav
        char TimeStr[16];
        if (!LinearTimeMode)
        {
          TimeStamps = NavLoad(PlaybackName, AbsPlaybackDir, &NrTimeStamps, HDVideo);
          if (TimeStamps)
          {
            // Write duration to log file
            WriteLogMCf(PROGRAM_NAME, ".nav-file loaded: %d I-Frames found.", NrTimeStamps);
            MSecToTimeString(TimeStamps[0].Timems, TimeStr);
            WriteLogMCf(PROGRAM_NAME, "First Timestamp: Block=%lu, Time=%s", TimeStamps[0].BlockNr, TimeStr);
            MSecToTimeString(TimeStamps[NrTimeStamps-1].Timems, TimeStr);
            WriteLogMCf(PROGRAM_NAME, "Playback Duration (from nav): %s", TimeStr);
            LastTimeStamp = &TimeStamps[0];
          }
          else
          {
            State = ST_UnacceptedFile;
            WriteLogMC(PROGRAM_NAME, "Error loading the .nav file!");
            ShowErrorMessage(LangGetString(LS_NavLoadFailed), NULL);
            PlaybackRepeatSet(OldRepeatMode);
            ClearOSD(TRUE);
            break;
          }
        }
        SecToTimeString(60*PlayInfo.duration + PlayInfo.durationSec, TimeStr);
        WriteLogMCf(PROGRAM_NAME, "Playback Duration (from inf): %s", TimeStr);

        // Check if nav has correct length!
        if(TimeStamps)
        {
          dword CurNavDiscrepancy = labs(TimeStamps[NrTimeStamps-1].Timems - (1000 * (60*PlayInfo.duration + PlayInfo.durationSec)));
          if(((MaxNavDiscrepancy > 0) && (CurNavDiscrepancy > MaxNavDiscrepancy)) || (CurNavDiscrepancy > 5000))
          {
            char  NavLengthWrongStr[128];
            WriteLogMC(PROGRAM_NAME, ".nav file length not matching duration!");

            // [COMPATIBILITY LAYER - fill holes in old nav file]
            if (PatchOldNavFile(PlaybackName, AbsPlaybackDir, HDVideo))
            {
              LastTotalBlocks = 0;
              WriteLogMC(PROGRAM_NAME, ".nav file patched by Compatibility Layer.");
              ShowErrorMessage(LangGetString(LS_NavPatched), NULL);
              PlaybackRepeatSet(OldRepeatMode);
              break;
            }
            else
            {
              TAP_SPrint(NavLengthWrongStr, sizeof(NavLengthWrongStr), LangGetString(LS_NavLengthWrong), (TimeStamps[NrTimeStamps-1].Timems/1000) - (60*PlayInfo.duration + PlayInfo.durationSec));
              if ((MaxNavDiscrepancy > 0) && (CurNavDiscrepancy > MaxNavDiscrepancy))
              {
                if (!ShowConfirmationDialog(NavLengthWrongStr))
                {
                  PlaybackRepeatSet(OldRepeatMode);
                  ClearOSD(TRUE);
                  if (AutoOSDPolicy)
                    State = ST_UnacceptedFile;
                  else
                  {
                    Cleanup(FALSE);
                    State = ST_InactiveMode;
                  }
                  break;
                }
              }
            }
          }
        }

        // Check if receiver has been rebooted since the recording
        if (RecDateTime)
        {
          dword TimeSinceRec = TimeDiff(RecDateTime, Now(NULL));
          dword UpTime = GetUptime() / 6000;
          #ifdef FULLDEBUG
            if ((void*)FIS_fwTimeToLinux() == NULL)
              WriteLogMC(PROGRAM_NAME, "Warning! Firmware function FIS_fwTimeToLinux() not found!");
            #if STACKTRACE == TRUE
              if (RecDateTime > 0xd0790000)
                RecDateTime = PvrTimeToLinux(RecDateTime);
              TAP_PrintNet("Reboot-Check (%s): TimeSinceRec=%lu, UpTime=%lu, RecDateTime=%s", (TimeSinceRec <= UpTime + 1) ? "true" : "false", TimeSinceRec, UpTime, ctime((time_t*) &RecDateTime));
            #endif
          #endif

          if (TimeSinceRec <= UpTime + 1)
          {
            if (ShowRebootMessage && !ShowConfirmationDialog(LangGetString(LS_RebootMessage)))
            {
              PlaybackRepeatSet(OldRepeatMode);
              ClearOSD(TRUE);
              if (AutoOSDPolicy)
                State = ST_UnacceptedFile;
              else
              {
                Cleanup(FALSE);
                State = ST_InactiveMode;
              }
              break;
            }
          }
        }

        // Check if free disc space is greater than RecFileSize
        __off64_t FreeDiscSpace = 0;
        FreeDiscSpace = HDD_GetFreeDiscSpace(PlaybackName, AbsPlaybackDir);
        WriteLogMCf(PROGRAM_NAME, "HDD free space = %llu Bytes", FreeDiscSpace);
        if (FreeDiscSpace < RecFileSize)
        {
          if (!ShowConfirmationDialog(LangGetString(LS_NotEnoughSpace)))
          {
            PlaybackRepeatSet(OldRepeatMode);
            ClearOSD(TRUE);
            if (AutoOSDPolicy)
              State = ST_UnacceptedFile;
            else
            {
              Cleanup(FALSE);
              State = ST_InactiveMode;
            }
            break;
          }
        }

        CalcLastSeconds();
        if (!MediaFileMode)
          if (!ReadBookmarks(Bookmarks, &NrBookmarks))
            WriteLogMC(PROGRAM_NAME, "Error: ReadBookmarks() failed!");
        if(!CutFileLoad())
          AddDefaultSegmentMarker();

        CloseLogMC();
        State = ST_ActiveOSD;
        OSDRedrawEverything();
        OSDTextStateWindow(LS_MovieCutterActive);
      }
      break;
    }

    // Inaktiver Modus
    // ---------------
    case ST_UnacceptedFile:
    case ST_InactiveModePlaying:  // OSD is hidden, checks if playback stopps or changes to start AutoOSD
    {
/*      if (State != ST_UnacceptedFile && !AutoOSDPolicy)  // dann kein Cleanup mehr, wenn Playback stoppt
      {
        State = ST_InactiveMode;
        break;
      } 
*/
      // if playback stopped or changed -> show MovieCutter as soon as next playback is started (ST_WaitForPlayback)
      if (!isPlaybackRunning() || (LastTotalBlocks == 0) || (LastTotalBlocks != PlayInfo.totalBlock))
      {
        if(InodeMonitoring) HDD_FixInodeList(AbsPlaybackDir, TRUE);
        State = (AutoOSDPolicy) ? ST_WaitForPlayback : ST_InactiveMode;
        Cleanup(FALSE);
        break;
      }

      if (State == ST_UnacceptedFile) break;

      if((event == EVT_KEY) && (param1 == RKEY_Ab || param1 == RKEY_Option))
      {
        TAP_GetState(&SysState, &SysSubState);
        if(SysState != STATE_Normal || (SysSubState != SUBSTATE_Normal && SysSubState != SUBSTATE_PvrPlayingSearch)) break;  // (nur wenn kein OSD eingeblendet ist!)

        // beim erneuten Einblenden kann man sich das Neu-Berechnen aller Werte sparen (AUCH wenn 2 Aufnahmen gleiche Blockzahl haben!!)
        if ((int)PlayInfo.currentBlock >= 0)
        {
          OSDMode = LastOSDMode;
//          NoPlaybackCheck = FALSE;
//          BookmarkMode = FALSE;
//          MinuteJump = DefaultMinuteJump;
          JumpRequestedSegment = 0xFFFF;    // eigentlich unnötig
          JumpRequestedBlock = (dword) -1;  //   "
//          JumpRequestedTime = 0;            //   "
          if (!MediaFileMode)
            if (!ReadBookmarks(Bookmarks, &NrBookmarks))
              WriteLogMC(PROGRAM_NAME, "Error: ReadBookmarks() failed!");
          OldRepeatMode = PlaybackRepeatGet();
          PlaybackRepeatSet(TRUE);
          State = ST_ActiveOSD;
          OSDRedrawEverything();
          OSDTextStateWindow(LS_MovieCutterActive);
        }
        param1 = 0;
      }
      break;
    }

    case ST_InactiveMode:    // OSD is hidden and has to be manually activated
    {
      // if cut-key is pressed -> show MovieCutter OSD (ST_WaitForPlayback)
      if((event == EVT_KEY) && (param1 == RKEY_Ab || param1 == RKEY_Option))
      {
        TAP_GetState(&SysState, &SysSubState);
        if(SysState != STATE_Normal || (SysSubState != SUBSTATE_Normal && SysSubState != SUBSTATE_PvrPlayingSearch)) break;  // (nur wenn kein OSD eingeblendet ist!)
        if (!isPlaybackRunning()) break;

        State = ST_WaitForPlayback;
        param1 = 0;
      }
      break;
    }

    // Aktiver OSD-Modus
    // -----------------
    case ST_ActiveOSD:             //Playback is active and OSD is visible
    {
      if(!isPlaybackRunning())
      {
        if (LastTotalBlocks > 0)
        {
#ifdef FULLDEBUG
  WriteLogMC(PROGRAM_NAME, "TAP_EventHandler: State=ST_ActiveOSD, !isPlaybackRunning --> Aufruf von CutFileSave()");
#endif
          CutFileSave();
        }
        if(InodeMonitoring) HDD_FixInodeList(AbsPlaybackDir, TRUE);
        State = AutoOSDPolicy ? ST_WaitForPlayback : ST_InactiveMode;
        Cleanup(TRUE);
        break;
      }

      if ((int)PlayInfo.currentBlock < 0)
        break;  // *** kritisch ***

      TAP_GetState(&SysState, &SysSubState);
      if(SysSubState != 0) TAP_ExitNormal();

      if(event == EVT_KEY)
      {
        bool ReturnKey = FALSE;

        switch(param1)
        {
          case RKEY_Record:  // Warum das!?
            break;
        
          case RKEY_Teletext:
          {
            if (ShowSegmentText)
            {
              ChangeSegmentText();
              break;
            }
          }  // Wenn nicht SegmentText-Anzeige -> fortsetzen mit Deaktivierung...

          case RKEY_Exit:
          {
            if (param1 == RKEY_Exit)
            {
              // Wenn Segmentsprung geplant -> diesen abbrechen
              if ((JumpRequestedSegment != 0xFFFF) || (JumpRequestedBlock != (dword) -1))
              {
                JumpRequestedSegment = 0xFFFF;
                JumpRequestedBlock = (dword) -1;
                JumpRequestedTime = 0;
                SetCurrentSegment();
                OSDSegmentListDrawList(FALSE);
                OSDInfoDrawProgressbar(TRUE, TRUE);
                break;
              }

              // Wenn SegmentText aktiv -> dieses ausblenden
              if (ShowSegmentText)
              {
                ShowSegmentText = FALSE;
                OSDSegmentTextDraw(TRUE);
                break;
              }

              // Wenn echtes OSD aktiv -> in NoOSD-Modus wechseln
              if (OSDMode != MD_NoOSD || rgnInfoBarMini)
              {
                if (OSDMode != MD_NoOSD)
                  LastOSDMode = OSDMode;
                OSDMode = MD_NoOSD;
                ShowSegmentText = FALSE;
                OSDRedrawEverything();
                OSDTextStateWindow(LS_MovieCutterActive);
                break;
              }
            }
          }  // Wenn NoOSD-Modus oder Teletext-Taste, fortsetzen mit Deaktivierung...

          case FKEY_Exit:
          case RKEY_Stop:
          case RKEY_Info:
          case RKEY_PlayList:
          case RKEY_AudioTrk:
          case RKEY_Uhf:  // =RKEY_Aspect (MediaAspect)
          {        
            // MC deaktivieren
#ifdef FULLDEBUG
  WriteLogMC(PROGRAM_NAME, "TAP_EventHandler: State=ST_ActiveOSD, Key=RKEY_Exit --> Aufruf von CutFileSave()");
#endif
            if (OSDMode != MD_NoOSD)
              LastOSDMode = OSDMode;
            JumpRequestedSegment = 0xFFFF;
            JumpRequestedBlock = (dword) -1;
            CutSaveToBM(FALSE);
            CutFileSave();
            PlaybackRepeatSet(OldRepeatMode);
            State = ST_InactiveModePlaying;
            ClearOSD(TRUE);
            if (RecStrip_active && (param1 == RKEY_Exit || param1 == FKEY_Exit || param1 == RKEY_Stop))
              OSDRecStripProgressBar();
            if ((param1 != RKEY_Exit) && (param1 != FKEY_Exit)) ReturnKey = TRUE;
//            else if (param1 == RKEY_Info) TAP_GenerateEvent(EVT_KEY, RKEY_Info, 0);
            break;
          }

/*          case RKEY_Stop:
          {
            CutSaveToBM(FALSE);
            TAP_Hdd_StopTs();
//            HDD_ChangeDir(PlaybackDir);
//            ReturnKey = TRUE;
            break;
          }  */

          case RKEY_Ab:
          case RKEY_Option:
          {
            if (OSDMode == MD_NoOSD)
              OSDMode = LastOSDMode;
            else
              OSDMode = (tOSDMode) ((OSDMode + 1) % 4);
            if(OSDMode == MD_NoOSD) OSDMode = MD_FullOSD;
            OSDRedrawEverything();
//            OSDTextStateWindow(LS_MovieCutterActive);
            break;
          }

          case RKEY_VFormat:
          case RKEY_Fav:
          case RKEY_Guide:
          {
            if (!MediaFileMode)
            {
              BookmarkMode = !BookmarkMode;
//              OSDRedrawEverything();
              OSDInfoDrawMinuteJump(FALSE);
              OSDInfoDrawBookmarkMode(FALSE);
              OSDInfoDrawProgressbar(TRUE, TRUE);
              OSDTextStateWindow((BookmarkMode) ? LS_BookmarkMode : LS_SegmentMode);
            }
            break;
          }

          case RKEY_Subt:
          {
            if (!ShowSegmentText)
            {
              ShowSegmentText = TRUE;
              SegmentTextStartLine = 0;
            }
            else
              SegmentTextStartLine += 3;
            OSDSegmentTextDraw(TRUE);
            break;
          }

          case RKEY_Up:
          case FKEY_ChUp:
          {
            Playback_SetJumpNavigate(TRUE, FALSE, TRUE);
            break;
          }
          case RKEY_Down:
          case FKEY_ChDown:
          {
            Playback_SetJumpNavigate(TRUE, FALSE, FALSE);
            break;
          }

          case RKEY_Prev:
          case RKEY_Next:
          case RKEY_VolDown:
          case RKEY_VolUp:
          case RKEY_ChDown:
          case RKEY_ChUp:
          {
            // Navigation nach links
            if (   ((RCUMode==RC_SRP2401 || RCUMode==RC_NoVolKeys) && param1==RKEY_ChDown) || (RCUMode==RC_SRP2410 && param1==RKEY_ChUp)   || (param1==RKEY_VolDown && RCUMode!=RC_SRP2410 && RCUMode!=RC_SRP2401 && RCUMode!=RC_NoVolKeys))
              Playback_SetJumpNavigate(FALSE, TRUE, TRUE);

            // Navigation nach rechts
            else if( ((RCUMode==RC_SRP2401 || RCUMode==RC_NoVolKeys) && param1==RKEY_ChUp) || (RCUMode==RC_SRP2410 && param1==RKEY_ChDown) || (param1==RKEY_VolUp   && RCUMode!=RC_SRP2410 && RCUMode!=RC_SRP2401 && RCUMode!=RC_NoVolKeys))
              Playback_SetJumpNavigate(FALSE, TRUE, FALSE);

            // MultiJump nach links
            else if ( (param1==RKEY_Prev) || (RCUMode==RC_SRP2401 && param1==RKEY_VolDown) || (RCUMode==RC_SRP2410 && param1==RKEY_VolUp))
            {
              if(MinuteJump)
                Playback_JumpBackward();
              else if(BookmarkMode)
                Playback_JumpPrevBookmark();
              else
                Playback_JumpPrevSegment();  
            }

            // MultiJump nach rechts
            else if ( (param1==RKEY_Next) || (RCUMode==RC_SRP2401 && param1==RKEY_VolUp)   || (RCUMode==RC_SRP2410 && param1==RKEY_VolDown))
            {
              if(MinuteJump)
                Playback_JumpForward();
              else if(BookmarkMode)
                Playback_JumpNextBookmark();
              else
                Playback_JumpNextSegment();
            }

            // bei anderen Fernbedienungen (ohne Up/Down) steuert ChUp/ChDown die Up/Down-Tase
            else if (RCUMode != RC_SRP2410 && RCUMode != RC_SRP2401 && RCUMode != RC_NoVolKeys)
            {
//              WriteLogMC("DEBUG", "ChUp/ChDown-Event empfangen, das nicht durch Up/Down gehandelt wurde!");
              if (param1 == RKEY_ChUp)          Playback_SetJumpNavigate(TRUE, FALSE, TRUE);
              else if (param1 == RKEY_ChDown)   Playback_SetJumpNavigate(TRUE, FALSE, FALSE);
            }

            // bei RCUMode = RC_NoVolKeys die Volume-Tasten durchreichen
            else if ((RCUMode == RC_NoVolKeys) && (param1 == RKEY_VolUp || param1 == RKEY_VolDown))
              ReturnKey = TRUE;
            break;
          }

          case RKEY_Left:
          case FKEY_VolDown:
          {
            Playback_Slower();
            break;
          }

          case RKEY_Right:
          case FKEY_VolUp:
          {
            Playback_Faster();
            break;
          }

          case RKEY_Recall:
          {
            if (UndoLastAction())
            {
              OSDSegmentListDrawList(FALSE);
              OSDSegmentTextDraw(TRUE);
              OSDInfoDrawProgressbar(TRUE, TRUE);
              OSDTextStateWindow(LS_UndoLastAction);
            }
            break;
          }

          case RKEY_F1:
          case RKEY_Red:
          {
            if (BookmarkMode)
            {  
              int NearestIndex = FindNearestBookmark(currentVisibleBlock());
              if (NearestIndex >= 0)
              {
                dword NearestBlock = Bookmarks[NearestIndex];
                if (DeleteBookmark(NearestIndex))
                {
                  if(NearestBlock == 0) NearestBlock = (dword) -1;
                  UndoAddEvent(FALSE, NearestBlock, 0, FALSE, NULL);
                  OSDTextStateWindow(LS_BookmarkDeleted);
                }
                OSDInfoDrawProgressbar(TRUE, TRUE);
//                OSDRedrawEverything();
              }
            }
            else
            {
              if (JumpRequestedSegment != 0xFFFF)
                break;
              int NearestIndex = FindNearestSegmentMarker(currentVisibleBlock());
              if (NearestIndex > 0)
              {
                dword NearestBlock = SegmentMarker[NearestIndex].Block;
                char *pPrevCaption  = SegmentMarker[NearestIndex].pCaption;
                bool SegmentWasSelected = SegmentMarker[NearestIndex].Selected;
                if (DeleteSegmentMarker(NearestIndex, FALSE))
                {
                  UndoAddEvent(TRUE, NearestBlock, 0, SegmentWasSelected, pPrevCaption);
                  OSDSegmentListDrawList(FALSE);
                  OSDInfoDrawProgressbar(TRUE, TRUE);
                  OSDTextStateWindow(LS_SegMarkerDeleted);
//                  OSDRedrawEverything();
                }
              }
            } 
            break;
          }

          case RKEY_Green:
          {
            dword newBlock = currentVisibleBlock();
            if (BookmarkMode)
            {
              if(AddBookmark(newBlock, TRUE) >= 0)
              {
                if(newBlock == 0) newBlock = (dword) -1;
                UndoAddEvent(FALSE, 0, newBlock, FALSE, NULL);
                OSDTextStateWindow(LS_BookmarkCreated);
              }
              OSDInfoDrawProgressbar(TRUE, TRUE);
//              OSDRedrawEverything();
            }
            else
            {
              if(AddSegmentMarker(&newBlock, TRUE, TRUE) > 0)
              {
                UndoAddEvent(TRUE, 0, newBlock, FALSE, NULL);
                OSDSegmentListDrawList(FALSE);
                OSDInfoDrawProgressbar(TRUE, TRUE);
                OSDTextStateWindow(LS_SegMarkerCreated);
//                OSDRedrawEverything();
                JumpPerformedTime = TAP_GetTick();
                if(!JumpPerformedTime) JumpPerformedTime = 1;
              }
            }
            break;
          }

          case RKEY_Yellow:
          {
            dword newBlock = currentVisibleBlock();
            if (BookmarkMode)
            {
              int NearestIndex = FindNearestBookmark(newBlock);
              if (NearestIndex >= 0)
              {
                dword NearestBlock = Bookmarks[NearestIndex];
                if(MoveBookmark(NearestIndex, newBlock, TRUE))
                {
                  if(NearestBlock == 0) NearestBlock = (dword) -1;
                  if(newBlock == 0) newBlock = (dword) -1;
                  UndoAddEvent(FALSE, NearestBlock, newBlock, FALSE, NULL);
                  OSDTextStateWindow(LS_BookmarkMoved);
                }
                OSDInfoDrawProgressbar(TRUE, TRUE);
              }
            }
            else
            {
              int NearestIndex = FindNearestSegmentMarker(newBlock);
              if (NearestIndex > 0)
              {
                dword NearestBlock = SegmentMarker[NearestIndex].Block;
                if(MoveSegmentMarker(NearestIndex, &newBlock, TRUE, TRUE))
                {
                  UndoAddEvent(TRUE, NearestBlock, newBlock, FALSE, NULL);
                  if(ActiveSegment + 1 == NearestIndex) ActiveSegment++;
                  OSDSegmentListDrawList(FALSE);
                  OSDInfoDrawProgressbar(TRUE, TRUE);
                  OSDTextStateWindow(LS_SegMarkerMoved);
                  JumpPerformedTime = TAP_GetTick();
                  if(!JumpPerformedTime) JumpPerformedTime = 1;
                }
              }
            }
//            OSDRedrawEverything();
            break;
          }

          case RKEY_Blue:
          {
            bool Selected = SelectSegmentMarker();
            OSDSegmentListDrawList(FALSE);
            OSDInfoDrawProgressbar(TRUE, TRUE);
            OSDTextStateWindow((Selected) ? LS_SegmentSelected : LS_SegmentUnselected);
//            OSDRedrawEverything();
            break;
          }

          case RKEY_Menu:
          case FKEY_Ok:
          {
            if (JumpRequestedSegment != 0xFFFF)
              break;

            if(TrickMode != TRICKMODE_Pause)
            {
              Playback_Pause();
            }
            ShowSegmentText = FALSE;
            if (OSDMode != MD_FullOSD)
            {
              LastOSDMode = OSDMode;
              OSDMode = MD_FullOSD;
              OSDRedrawEverything();
            }
            else
              OSDSegmentTextDraw(FALSE);
            State = ST_ActionMenu;
            ActionMenuItem = 0;
            ActionMenuEvenOdd = 1;
            ActionMenuDraw();
            break;
          }

          case RKEY_Ok:
          {
            if(TrickMode == TRICKMODE_Normal)
              Playback_Pause();
            else
              Playback_Normal();
            break;
          }

          case RKEY_Play:
          {
            if(TrickMode == TRICKMODE_Normal)
            {
/*              switch (OSDMode)
              {
                case MD_NoOSD:
                case MD_FullOSD:       OSDMode = MD_MiniOSD; break;
                case MD_MiniOSD:       OSDMode = MD_NoSegmentList; break;
                case MD_NoSegmentList: OSDMode = MD_FullOSD; break;
              }  
              OSDRedrawEverything();  */
              if (OSDMode == MD_NoOSD)
              {
                LastPlayStateChange = TAP_GetTick();
                if(!LastPlayStateChange) LastPlayStateChange = 1;
                OSDMode = MD_MiniOSD;
                OSDRedrawEverything();
                OSDMode = MD_NoOSD;
              }
//              OSDTextStateWindow(LS_MovieCutterActive);
            }
            else
              Playback_Normal();
            break;
          }

          case RKEY_Pause:
          {
            Playback_Pause();
            break;
          }

          case RKEY_Forward:
          {
            Playback_FFWD();
            break;
          }

          case RKEY_Rewind:
          {
            Playback_RWD();
            break;
          }

          case RKEY_Slow:
          {
            Playback_Slow();
            break;
          }

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
          {
//            if(PLAYINFOVALID())   // PlayInfo ist sicher, da bereits in Z. 804 überprüft
//            {
              if ((MinuteJump > 0) && (MinuteJump < 10) && (labs(TAP_GetTick() - LastMinuteKey) < 200))
                // We are already in minute jump mode, but only one digit already entered
                MinuteJump = 10 * MinuteJump + (param1 & 0x0f);
              else
                MinuteJump = (param1 & 0x0f);
              LastMinuteKey = TAP_GetTick();
              OSDInfoDrawMinuteJump(TRUE);
              OSDTextStateWindow((MinuteJump) ? LS_MinuteJumpActive : LS_MinuteJumpDisabled);
              break;
//            }
          }

          case RKEY_Sleep:
          {
            if(RecStrip_active) ReturnKey = TRUE;
            break;
          }
//          case RKEY_Stop:
          case RKEY_Mute:
          case RKEY_Power:
/*          case RKEY_Sat:
          case RKEY_TvRadio:
          case RKEY_TvSat:
          case RKEY_Easy: */
          {
            ReturnKey = TRUE;
            break;
          }
          default:
            ReturnKey = FALSE;
        }
        if (!ReturnKey)
          param1 = 0;
      }

      else if (event == EVT_IDLE)
      {
        // VORSICHT!!! Das hier wird interaktiv ausgeführt
//        if(labs(TAP_GetTick() - LastDraw) > 10)
//        {
          bool JumpPerformed = FALSE;
          if (JumpRequestedTime && (labs(TAP_GetTick() - JumpRequestedTime) >= 100))
          {
            if ((JumpRequestedSegment != 0xFFFF) || (JumpRequestedBlock != (dword) -1))
            {
              if(TrickMode == TRICKMODE_Pause) Playback_Normal();
              if (JumpRequestedSegment != 0xFFFF)
              {
                TAP_Hdd_ChangePlaybackPos(SegmentMarker[JumpRequestedSegment].Block);
                ActiveSegment = JumpRequestedSegment;
                JumpRequestedBlock = (dword) -1;
              }
              else if (JumpRequestedBlock != (dword) -1)
                TAP_Hdd_ChangePlaybackPos(JumpRequestedBlock);
              JumpPerformedTime = TAP_GetTick();    if(!JumpPerformedTime) JumpPerformedTime = 1;
              LastPlayStateChange = TAP_GetTick();  if(!LastPlayStateChange) LastPlayStateChange = 1;
              JumpRequestedSegment = 0xFFFF;
              JumpPerformed = TRUE;
            }
            JumpRequestedTime = 0;
          }
          if(LastPlayStateChange && (labs(TAP_GetTick() - LastPlayStateChange) > 300))
          {
            if(rgnPlayState)
            {
              TAP_Osd_Delete(rgnPlayState);
              rgnPlayState = 0;
            }
            if(rgnTextState)
            {
              TAP_Osd_Delete(rgnTextState);
              rgnTextState = 0;
            }
            if(rgnInfoBarMini && OSDMode != MD_MiniOSD)
            {
              TAP_Osd_Delete(rgnInfoBarMini);
              rgnInfoBarMini = 0;
            }
            LastPlayStateChange = 0;
          }
          CheckLastSeconds();
          SetCurrentSegment();
          OSDInfoDrawProgressbar(JumpPerformed, FALSE);
//          if(!LastPlayStateChange || (labs(TAP_GetTick() - LastPlayStateChange) > 20))
            OSDInfoDrawPlayIcons(FALSE, FALSE);

          if ((JumpRequestedBlock != (dword) -1) && !JumpRequestedTime)  // nach FastNav-Sprung den grauen Balken aktualisieren
            OSDInfoDrawCurrentPlayTime(TRUE);
          else
            OSDInfoDrawCurrentPlayTime(FALSE);
          OSDInfoDrawClock(FALSE);
          TAP_Osd_Sync();
//          LastDraw = TAP_GetTick();
//        }
      }
      break;
    }

    // ActionMenu eingeblendet
    // -----------------------
    case ST_ActionMenu:        //Action dialog is visible
    {
      if(event == EVT_KEY)
      {
        switch(param1)
        {
          case RKEY_1:
          case RKEY_2:
          case RKEY_3:
          case RKEY_4:
          case RKEY_5:
          case RKEY_6:
          case RKEY_7:
          case RKEY_8:
          {
            dword SelectedMenuItem = param1 & 0x0f;
            if (ActionMenuItemInactive(SelectedMenuItem))
              break;
            ActionMenuItem = SelectedMenuItem;
            ActionMenuDraw();
            // fortsetzen mit OK-Button ...
          }

          case RKEY_Ok:
          case FKEY_Ok:
          {
            if(ActionMenuItem != MI_SelectFunction)
            {
              // Deaktivierte Aktionen
              if (ActionMenuItemInactive(ActionMenuItem))
                break;

              // Aktionen mit Confirmation-Dialog
              if (ActionMenuItem==MI_SaveSegments || ActionMenuItem==MI_DeleteSegments || (ActionMenuItem==MI_CopySegSplitMovie && BookmarkMode) || (ActionMenuItem==MI_ClearAll && (BookmarkMode || NrSelectedSegments==0)) || (ActionMenuItem==MI_ImExportBookmarks && ((!BookmarkMode && NrSegmentMarker>2) || (BookmarkMode && NrBookmarks>0))))
              {
                if(AskBeforeEdit && !ShowConfirmationDialog(LangGetString(LS_AskConfirmation)))
                {
//                  OSDRedrawEverything();
                  ActionMenuDraw();
                  break;
                }
              }

              // PlayInfo prüfen
              if (!(isPlaybackRunning() && PLAYINFOVALID()) && (ActionMenuItem != MI_ExitMC) && (ActionMenuItem != MI_RenameMovie) && (ActionMenuItem != MI_RecStripCheckFS))   // PlayInfo wird nicht aktualisiert
                break;

              ActionMenuRemove();
              State = ST_ActiveOSD;

              switch(ActionMenuItem)
              {
                case MI_SaveSegments:        MovieCutterSaveSegments(); break;
                case MI_DeleteSegments:      MovieCutterDeleteSegments(); break;
                case MI_SelectEvOddSegments: MovieCutterSelectEvOddSegments(); break;
                case MI_ClearAll:            
                {  
                  if(BookmarkMode)
                    DeleteAllBookmarks();
                  else
                  {
                    if(NrSelectedSegments > 0)
                    {
                      MovieCutterUnselectAll();
                      break;
                    }
                    else
                      DeleteAllSegmentMarkers();
                  }
                  OSDSegmentListDrawList(FALSE);
                  OSDInfoDrawProgressbar(TRUE, TRUE);
                  break;
                }
                case MI_ImExportBookmarks:
                {
                  if (!BookmarkMode)
                    ImportBookmarksToSegments();
                  else
                    ExportSegmentsToBookmarks();
                  break;
                }  
                case MI_RenameMovie:         MovieCutterRenameFile(); break;

                case MI_CopySegSplitMovie:
                  if (BookmarkMode)
                  {
                    MovieCutterSplitMovie(); break;
                  }
                  else
                  {
                    if (NrSelectedSegments == 0)
                      SegmentMarker[ActiveSegment].Selected = TRUE;
                    State = ST_ActionSubMenu;
                    ActionSubMenuItem = 0;
                    OSDSegmentListDrawList(FALSE);
                    OSDInfoDrawProgressbar(TRUE, TRUE);
                    ActionMenuDraw();
                    ActionSubMenuDraw();
                    break;
                  }

                case MI_RecStripCheckFS:
                {
                  if ((ActionMenuItem == MI_CopySegSplitMovie) || (ActionMenuItem == MI_RecStripCheckFS && !BookmarkMode))
                  {
                    WriteLogMCf(PROGRAM_NAME, "[Action 'Strip movie' started...]");
                    CutSaveToBM(FALSE);
                    CutFileSave();
                    TAP_Hdd_StopTs();
//                    RecStrip_DoCut = FALSE;
                    RecStrip_active = TRUE;
                    RecStrip_Percent = 0;
//                    ClearOSD(TRUE);
                  }
                  else
                  {
                    WriteLogMC(PROGRAM_NAME, "[Action 'Check file system' started...]");
                    CheckFileSystem(AbsPlaybackDir, 0, 1, 1, TRUE, FALSE, FALSE, TRUE, 0, NULL);
                  }
                  break;
                }
                case MI_ExitMC:
                {
                  if (RecStrip_active)
                    event = EVT_STOP;
                  else
                    State = ST_Exit;
                  break;
                }
                case MI_NrMenuItems:         break;
              }
            }
            break;
          }

          case RKEY_Down:
          case RKEY_ChDown:
          case FKEY_ChDown:
          {
            ActionMenuDown();
            break;
          }

          case RKEY_Up:
          case RKEY_ChUp:
          case FKEY_ChUp:
          {
            ActionMenuUp();
            break;
          }

          case RKEY_VFormat:
          case RKEY_Fav:
          case RKEY_Guide:
          {
            if (!MediaFileMode)
            {
              BookmarkMode = !BookmarkMode;
              OSDInfoDrawMinuteJump(FALSE);
              OSDInfoDrawBookmarkMode(FALSE);
              OSDInfoDrawProgressbar(TRUE, TRUE);
              ActionMenuDraw();
            }
            break;
          }

          case RKEY_Exit:
          case FKEY_Exit:
          {
            ActionMenuRemove();
            State = ST_ActiveOSD;
            break;
          }
        }
        
        if ((param1 != RKEY_Mute) && (param1 != RKEY_Power) && !(param1 == RKEY_Sleep && RecStrip_active))
          param1 = 0;
      }
      break;
    }

    // ActionSubMenu eingeblendet
    // --------------------------
    case ST_ActionSubMenu:        //RecStrip Sub Menu is visible
    {
      if(event == EVT_KEY)
      {
        switch(param1)
        {
          case RKEY_1:
          case RKEY_2:
          case RKEY_3:
          case RKEY_4:
          {
            ActionSubMenuItem = param1 & 0x0f;
            ActionSubMenuDraw();
            // fortsetzen mit OK-Button ...
          }

          case RKEY_Ok:
          case FKEY_Ok:
          {
            // PlayInfo prüfen
            if (!(isPlaybackRunning() && PLAYINFOVALID()))   // PlayInfo wird nicht aktualisiert
              break;

            ActionMenuRemove();
            State = ST_ActiveOSD;

            WriteLogMCf(PROGRAM_NAME, "[Action 'Copy parts' started...]");
            CutSaveToBM(FALSE);
            CutFileSave();
            TAP_Hdd_StopTs();
            RecStrip_DoCut = ActionSubMenuItem + 1;
            RecStrip_active = TRUE;
            RecStrip_Percent = 0;
//            ClearOSD(TRUE);
            break;
          }

          case RKEY_Down:
          case RKEY_ChDown:
          case FKEY_ChDown:
          {
            ActionSubMenuItem = (ActionSubMenuItem+1) % 4;
            ActionSubMenuDraw();
            break;
          }

          case RKEY_Up:
          case RKEY_ChUp:
          case FKEY_ChUp:
          {
            if (ActionSubMenuItem > 0)
              ActionSubMenuItem--;
            else
              ActionSubMenuItem = 3;
            ActionSubMenuDraw();
            break;
          }

          case RKEY_Exit:
          case FKEY_Exit:
          case RKEY_Left:
          {
            TAP_Osd_Delete(rgnActionSubMenu);
            rgnActionSubMenu = 0;
            State = ST_ActionMenu;
            ActionMenuDraw();
            break;
          }
        }
        
        if ((param1 != RKEY_Mute) && (param1 != RKEY_Power) && !(param1 == RKEY_Sleep && RecStrip_active))
          param1 = 0;
      }
      break;
    }

    case ST_Exit:
    case ST_ExitNoSave:
      break;
  }


  // RecStrip läuft: Fortschritt anzeigen und auf Beendigung warten
  // --------------------------------------------------------------
  if(RecStrip_active)         //RecStrip is running in background
  {
    static char         PercentBuf[7];  PercentBuf[0] = '\0';
    static bool         ShutdownAfterRS = FALSE;

    if (event == EVT_IDLE)
    {
      static char       RS_OrigDir[FBLIB_DIR_SIZE], RS_OrigName[MAX_FILE_NAME_SIZE];
      static char       RS_StripName[MAX_FILE_NAME_SIZE], RS_BakName[MAX_FILE_NAME_SIZE];
      static __off64_t  RS_OrigSize;
      static dword      LastRefresh;
      static int        RecStrip_Return = -1;
      static int        pipefd[2];
      static int        ReadBytes = 0;
      __off64_t         StripFileSize;

      // INIT: Starte RecStrip
      if (!RecStrip_Pid && RecStrip_Return == -1)
      {
        // Progress-OSD einblenden
        RecStrip_Percent = 0;
        OSDRecStripProgressBar();

        // Aktuelle Playback-Daten speichern (da Playback gestoppt wurde)
        strcpy(RS_OrigDir, AbsPlaybackDir);
        strcpy(RS_OrigName, PlaybackName);
        RS_OrigSize = RecFileSize;

        // Neue Dateinamen berechnen
        if (RecStrip_DoCut)
        {
          GetNextFreeCutName(RS_OrigName, RS_StripName, RS_OrigDir, 0);
/*            RS_OrigSize = 0;
          int i;
          for (i = 0; i < NrSegmentMarker-2; i++)
            if (SegmentMarker[i].Selected)
              RS_OrigSize += (SegmentMarker[i+1].Block - SegmentMarker[i].Block) * 9024; */
        }
        else
        {
          char         *ExtensionStart;
          int           NameLength = 0;

          ExtensionStart = strrchr(RS_OrigName, '.');
          if (ExtensionStart) NameLength = ExtensionStart - RS_OrigName;
          strcpy(RS_StripName, RS_OrigName);
          strcpy(RS_BakName, RS_OrigName);
          TAP_SPrint(&RS_StripName[NameLength], sizeof(RS_StripName)-NameLength, "_strip%s", ExtensionStart);
          TAP_SPrint(&RS_BakName[NameLength], sizeof(RS_BakName)-NameLength, "_bak%s", ExtensionStart);
        }
        HDD_Delete2(RS_StripName, RS_OrigDir, TRUE);
        ReadBytes = 0;

        // RecStrip starten
        {
          char AbsStripName[FBLIB_DIR_SIZE], AbsOrigName[FBLIB_DIR_SIZE];
          pid_t CurPid;  int flags = 1;

          if (pipe(pipefd) < 0)
          {
            pipefd[0] = 0;  pipefd[1] = 0;
          }
          if (pipefd[0])
            ioctl(pipefd[0], FIONBIO, &flags);
//            flags = fcntl(pipefd[0], F_GETFL, 0);
//            fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK);

          CurPid = fork();
          if (CurPid == 0)
          {
            // Child-Process
            char* args[7];
            int i = 0;
            int fd = open("/mnt/hd/ProgramFiles/Settings/RecStripper/RecStrip.log", O_WRONLY | O_APPEND | O_CREAT);
            if (fd >= 0)
            {
              char Buffer[128];
              time_t StartTime; byte sec;
              StartTime = PvrTimeToLinux(Now(&sec)) + sec;
              TAP_SPrint(Buffer, sizeof(Buffer), "\r\n=========================================================\r\n*** RecStrip started %s \r\n", ctime(&StartTime));
              write(fd, Buffer, strlen(Buffer));
              dup2(fd, 1);
              close(fd);
            }
            if (pipefd[0]) close(pipefd[0]);    // close reading end in the child
            if (pipefd[1])
            {
              dup2(pipefd[1], 2);  // send stderr to the pipe
              close(pipefd[1]);    // this descriptor is no longer needed
            }

            TAP_SPrint(AbsOrigName, sizeof(AbsOrigName), "%s/%s", RS_OrigDir, RS_OrigName);
            TAP_SPrint(AbsStripName, sizeof(AbsStripName), "%s/%s", RS_OrigDir, RS_StripName);
            args[i++] = "RecStrip";
            if (RecStrip_DoCut)
            {
              if (RecStrip_DoCut == 1 || RecStrip_DoCut == 3)
                args[i++] = "-c";
              else
                args[i++] = "-r";
            }
            if (!RecStrip_DoCut || RecStrip_DoCut >= 3)
            {
              args[i++] = "-s";
              if (StripMode & 0x1) args[i++] = "-e";
              if (StripMode & 0x2) args[i++] = "-t";
            }
            args[i++] = AbsOrigName;
            args[i++] = AbsStripName;
            args[i]   = (char*) 0;
            execv(RECSTRIPPATH "/RecStrip", args);
            exit(-1);
          }
          else if (CurPid > 0)
          {
            // Parent-Process
            RecStrip_Pid = CurPid;
            if (pipefd[1]) close(pipefd[1]);  // close the write end of the pipe in the parent
          }
        }
      }

      // WAIT: Auf Beendigung von RecStrip warten
      if (RecStrip_Pid)
      {
        int status;
        if (waitpid(RecStrip_Pid, &status, WNOHANG) != RecStrip_Pid)
        {
          if (labs(TAP_GetTick() - LastRefresh) > 10)
          {
            int p;
            if (pipefd[0] && (ReadBytes < (int)sizeof(PercentBuf)-1))
              if ((p = read(pipefd[0], &PercentBuf[ReadBytes], sizeof(PercentBuf)-ReadBytes-1)) > 0)
                ReadBytes += p;
            if (ReadBytes >= (int)sizeof(PercentBuf)-1)
            {
              PercentBuf[sizeof(PercentBuf)-1] = '\0';
//TAP_PrintNet("READ: %s\n", PercentBuf);
              RecStrip_Percent = atoi(PercentBuf);
              if (rgnStripProgBar)
                OSDRecStripProgressBar();
              ReadBytes = 0;
            }
//else if (ReadBytes > 0)
//  TAP_PrintNet("ACHTUNG Teil: %s\n", PercentBuf);
/*                if (fStripFile < 0)
              fStripFile = open(AbsStripName, O_RDONLY);
            if (fStripFile >= 0)
              fsync(fStripFile);
            if (HDD_GetFileSizeAndInode2(RS_StripName, RS_OrigDir, NULL, &StripFileSize))
              OSDRecStripProgressBar(CalcBlockSize(StripFileSize) / (CalcBlockSize(RS_OrigSize) / 100)); */
            LastRefresh = TAP_GetTick();
          }
        }
        else
        {
//TAP_PrintNet("RecStrip finished with: WIFEXITED=%d and exit status %d.\n", WIFEXITED(status), WEXITSTATUS(status));
          if (WIFEXITED(status))
            RecStrip_Return = WEXITSTATUS(status);
          if (RecStrip_Return >= 0)
            WriteLogMC(PROGRAM_NAME, "RecStrip finished.");
          else
            WriteLogMC(PROGRAM_NAME, "RecStrip aborted.");
          RecStrip_Pid = 0;
          if (pipefd[0]) close(pipefd[0]);
        }
      }

      // FINISHED: Nachverarbeitung
      if (!RecStrip_Pid)
      {
        if (RecStrip_Return == 0)
        {
          // Set Date of Recording
          char FileName[FBLIB_DIR_SIZE];
//          dword FileDate;

//          sync();
//          TAP_Sleep(1);
          HDD_GetFileSizeAndInode2(RS_StripName, RS_OrigDir, NULL, &StripFileSize);

/*          TAP_SPrint(FileName, sizeof(FileName), "%s/%s", RS_OrigDir, RS_OrigName);
          FileDate = Unix2TFTime(HDD_GetFileTimeByAbsFileName(FileName));

          TAP_SPrint(FileName, sizeof(FileName), "%s.inf", RS_OrigName);    HDD_SetFileDateTime(FileName, RS_OrigDir, FileDate); 

          if (RecDateTime)
            FileDate = RecDateTime;
          HDD_SetFileDateTime(RS_StripName, RS_OrigDir, FileDate); 
          GetCutNameFromRec(RS_StripName, RS_OrigDir, FileName);            HDD_SetFileDateTime(FileName, RS_OrigDir, FileDate);
          TAP_SPrint(FileName, sizeof(FileName), "%s.nav", RS_StripName);   HDD_SetFileDateTime(FileName, RS_OrigDir, FileDate);
          TAP_SPrint(FileName, sizeof(FileName), "%s.inf", RS_StripName);   HDD_SetFileDateTime(FileName, RS_OrigDir, FileDate); 
*/
          // Falls erfolgreich, Aufnahmen umbenennen
          if (!RecStrip_DoCut)
          {
            if (HDD_Exist2(FileName, RS_OrigDir))
            {
              WriteLogMCf(PROGRAM_NAME, "Renaming original recording '%s' to '%s'", RS_OrigName, RS_BakName);
              HDD_Rename2(RS_OrigName, RS_BakName, RS_OrigDir, TRUE, TRUE);
              WriteLogMCf(PROGRAM_NAME, "Renaming stripped recording '%s' to '%s'", RS_StripName, RS_OrigName);
              HDD_Rename2(RS_StripName, RS_OrigName, RS_OrigDir, TRUE, TRUE);
            }
            else RecStrip_Return = -1;
          }
        }
//        else if (RecStrip_Return < 0)
//          HDD_Delete2(RS_StripName, RS_OrigDir, TRUE);

        RecStrip_active = FALSE;  // State muss vor Message gesetzt werden, da für RecStrip_active eine Ausnahme von DoNotReenter bei geöffnetem Dialog besteht
        if (OSDMenuMessageBoxIsVisible())  // "RecStrip abbrechen?" Dialog entfernen (evtl. blöd, wenn es ein anderer Dialog ist)
        {
          word p_Event = EVT_KEY;
          dword p_Key = RKEY_Exit;
          OSDMenuEvent(&p_Event, &p_Key, NULL);
        }

        // END: Ausgabe
        if (rgnStripProgBar)
        {
          TAP_Osd_Delete(rgnStripProgBar);
//          TAP_EnterNormalNoInfo();
          rgnStripProgBar = 0;
        }
        TAP_Osd_Sync();

        // Output RecStrip return
        if (RecStrip_Return == 0)
        {
          char MessageStr[128];
          TAP_SPrint(MessageStr, sizeof(MessageStr), LangGetString(LS_StripSuccess), CalcBlockSize(StripFileSize)/116.1986, CalcBlockSize(RS_OrigSize)/116.1986, ((double)CalcBlockSize(StripFileSize)/CalcBlockSize(RS_OrigSize))*100);
          WriteLogMC(PROGRAM_NAME, MessageStr);
          if (!ShutdownAfterRS)
            ShowErrorMessage(MessageStr, NULL);
        }
        else if (RecStrip_Return > 0)
        {
          WriteLogMCf(PROGRAM_NAME, "RecStrip returned error code %d!", RecStrip_Return);
          if (!ShutdownAfterRS)
            ShowErrorMessage(LangGetString(LS_StripFailed), NULL);
        }
        RecStrip_DoCut = FALSE;
        RecStrip_Return = -1;
      }
    }

    else if (event == EVT_KEY)
    {
      if (param1 == RKEY_Option || param1 == RKEY_Ab)
      {
        if (rgnStripProgBar)
        {
          TAP_Osd_Delete(rgnStripProgBar);
//          if (OldSysSubState != 0)
//            TAP_EnterNormalNoInfo();
          rgnStripProgBar = 0;
          TAP_Osd_Sync();
        }
        else
          OSDRecStripProgressBar();
        param1 = 0;
      }
      else if ((rgnStripProgBar && (param1 == RKEY_Exit || param1 == FKEY_Exit)) || param1 == RKEY_Sleep)
      {
        if (RecStrip_Pid && !rgnStripProgBar)
          OSDRecStripProgressBar();
        if (RecStrip_Pid)
        {
          if (ShowConfirmationDialog(LangGetString(LS_AbortRecStrip)))
            if (RecStrip_Pid) kill(RecStrip_Pid, SIGKILL);  // Wenn RS während Dialog beendet wird, führt der "Aktion erfolgreich" Dialog zu einer positiven Auswertung von ShowConfirmDialog -> kill(0)
/*            if (!RecStrip_Pid)
          {
            TRACEEXIT();
            return 0;  // Wenn RS während Dialogfeld beendet wird (EventHandler wurde normal verlassen), darf DoNotReenter nicht nochmals zurückgesetzt werden!
          } */
        }
        param1 = 0;
      }
      else if (rgnStripProgBar && !(param1==RKEY_Stop || (param1>=RKEY_0 && param1<=RKEY_9)))
        param1 = 0;
    }

    else if ((event == EVT_STOP) && (param1 != 2))
    {
      ShutdownAfterRS = TRUE;
      if (RecStrip_Pid && !rgnStripProgBar)
        OSDRecStripProgressBar();
      if (RecStrip_Pid)
      {
        char Msg[128];
        TAP_SPrint(Msg, sizeof(Msg), "%s\n\n%s", LangGetString(LS_AbortRecStrip), LangGetString(LS_ShutdownAfterRS));
        ShowErrorMessage(Msg, NULL);
        while (RecStrip_Pid)
        {
          kill (RecStrip_Pid, SIGKILL);
          TAP_EventHandler(EVT_IDLE, 0, 0);  // gehe ins IDLE-Event, um den Abschluss von RecStrip durchzuführen
        }
      }
      State = (State==ST_ActiveOSD || State==ST_ActionMenu || State==ST_ActionSubMenu) ? ST_Exit : ST_ExitNoSave;
    }
    // nach Stop-Event fortsetzen mit ST_ExitNoSave ...
  }

  // Beendigung des TAPs
  // -------------------
  if (State == ST_Exit || State == ST_ExitNoSave)    //Preparing to terminate the TAP
  {
    if (State == ST_Exit)
    {
      if (LastTotalBlocks > 0)
      {
#ifdef FULLDEBUG
  WriteLogMC(PROGRAM_NAME, "TAP_EventHandler: State=ST_Exit --> Aufruf von CutFileSave()");
#endif
        if (isPlaybackRunning() && (LastTotalBlocks == PlayInfo.totalBlock))
          CutSaveToBM(TRUE);
        CutFileSave();
      }
    }  // fortsetzen mit ST_ExitNoSave ...

    // wenn nötig, beim Beenden HDDCheck und/oder Inode-Fix durchführen
    if (SuspectHDDs[0])
    {
      int   i = 0, NrHDDs = 0;
      char *p;
        
      if (CheckFSAfterCut == FM_Shutdown)
      {
        while (SuspectHDDs[i] != '\0')
          if (SuspectHDDs[i++] == ';') NrHDDs++;
      }        

      i = 0;
      p = strtok(SuspectHDDs, ";");
      while (p && p[0])
      {
        if (CheckFSAfterCut == FM_Shutdown)
        {
//            WriteLogMCf(PROGRAM_NAME, "Inode-Check (%d/%d) mit fsck für Mount: %s", i+1, NrHDDs, p);
          CheckFileSystem(p, i, i+1, NrHDDs, TRUE, TRUE, FALSE, FALSE, NrAllSuspectInodes, NULL);
        }
        else if (InodeMonitoring)
          HDD_FixInodeList(p, TRUE);
        p = strtok(NULL, ";");
        i++;
      }
    }
    if(InodeMonitoring)
      HDD_FixInodeList(((AbsPlaybackDir[0]) ? AbsPlaybackDir : TAPFSROOT), TRUE);

    if(isPlaybackRunning()) PlaybackRepeatSet(OldRepeatMode);
    Cleanup(TRUE);
    TAP_MemFree(SuspectHDDs);
    TAP_MemFree(UndoStack);
    TAP_MemFree(Bookmarks);
    ResetSegmentMarkers();
    TAP_MemFree(SegmentMarker);
    #ifdef MC_MULTILANG
      LangUnloadStrings();
    #endif
    #ifdef MC_UNICODE
      FMUC_FreeFontFile(&Calibri_10_FontData);
      FMUC_FreeFontFile(&Calibri_12_FontData);
      FMUC_FreeFontFile(&Calibri_14_FontData);
      FMUC_FreeFontFile(&Courier_New_13_FontData);
      OSDMenuFreeStdFonts();
    #endif
    WriteLogMC(PROGRAM_NAME, "MovieCutter Exit.\r\n");
    CloseLogMC();
    TAP_Exit();
  }

  DoNotReenter = WasNotReenter;
  TRACEEXIT();
  return param1;
}

// ----------------------------------------------------------------------------
//                           MessageBox-Funktionen
// ----------------------------------------------------------------------------
// Die Funktion zeigt eine Bestätigungsfrage (Ja/Nein) an, und wartet auf die Bestätigung des Benutzers.
// Nach Beendigung der Message kehrt das TAP in den Normal-Mode zurück, FALLS dieser zuvor aktiv war.
// Beim Beenden wird das entsprechende OSDRange gelöscht. Überdeckte Bereiche anderer OSDs werden NICHT wiederhergestellt.
bool ShowConfirmationDialog(char *MessageStr)
{
  dword OldSysState, OldSysSubState;
  bool ret;

  TRACEENTER();
  HDD_TAP_PushDir();
  TAP_GetState(&OldSysState, &OldSysSubState);

  OSDMenuMessageBoxInitialize(PROGRAM_NAME, MessageStr);
  OSDMenuSaveMyRegion(rgnSegmentList);
  OSDMenuMessageBoxDoNotEnterNormalMode(TRUE);
  OSDMenuMessageBoxAllowScrollOver();
  OSDMenuMessageBoxButtonAdd(LangGetString(LS_Yes));
  OSDMenuMessageBoxButtonAdd(LangGetString(LS_No));
  OSDMenuMessageBoxButtonSelect(1);
  OSDMenuMessageBoxShow();
//  MCShowMessageBox = TRUE;
  while (OSDMenuMessageBoxIsVisible())
  {
    TAP_SystemProc();
    TAP_Sleep(1);
  }
  ret = (OSDMenuMessageBoxLastButton() == 0);

  TAP_Osd_Sync();
  if(OldSysSubState != 0) TAP_EnterNormalNoInfo();
//  OSDMenuFreeStdFonts();

  HDD_TAP_PopDir();
  TRACEEXIT();
  return ret;
}

// Die Funktion zeigt einen Informationsdialog (OK) an, und wartet auf die Bestätigung des Benutzers.
void ShowErrorMessage(char *MessageStr, char *TitleStr)
{
  dword OldSysState, OldSysSubState;

  TRACEENTER();
  HDD_TAP_PushDir();
  TAP_GetState(&OldSysState, &OldSysSubState);

  OSDMenuMessageBoxInitialize((TitleStr) ? TitleStr : PROGRAM_NAME, MessageStr);
  OSDMenuSaveMyRegion(rgnSegmentList);
  OSDMenuMessageBoxDoNotEnterNormalMode(TRUE);
  OSDMenuMessageBoxButtonAdd(LangGetString(LS_OK));
  OSDMenuMessageBoxShow();
//  MCShowMessageBox = TRUE;
  while (OSDMenuMessageBoxIsVisible())
  {
    TAP_SystemProc();
    TAP_Sleep(1);
  }

  TAP_Osd_Sync();
  if(OldSysSubState != 0) TAP_EnterNormalNoInfo();
//  OSDMenuFreeStdFonts();

  HDD_TAP_PopDir();
  TRACEEXIT();
}

dword TMSCommander_handler(dword param1)
{
  TRACEENTER();

  switch (param1)
  {
    case TMSCMDR_Capabilities:
    {
      TRACEEXIT();
      return (dword)(TMSCMDR_CanBeStopped | TMSCMDR_HaveUserEvent);
    }

    case TMSCMDR_UserEvent:
    {
      if(State == ST_InactiveMode || State == ST_InactiveModePlaying)
        State = ST_WaitForPlayback;

      TRACEEXIT();
      return TMSCMDR_OK;
    }

    case TMSCMDR_Menu:
    {
      TRACEEXIT();
      return TMSCMDR_NotOK;
    }

    case TMSCMDR_Stop:
    {
      State = (State==ST_ActiveOSD || State==ST_ActionMenu || State==ST_ActionSubMenu) ? ST_Exit : ST_ExitNoSave;

      TRACEEXIT();
      return TMSCMDR_OK;
    }
  }

  TRACEEXIT();
  return TMSCMDR_UnknownFunction;
}

// ----------------------------------------------------------------------------
//                            Reset-Funktionen
// ----------------------------------------------------------------------------
void ClearOSD(bool EnterNormal)
{
  TRACEENTER();

  if(rgnPlayState)
  {
    TAP_Osd_Delete(rgnPlayState);
    rgnPlayState = 0;
  }
  if(rgnTextState)
  {
    TAP_Osd_Delete(rgnTextState);
    rgnTextState = 0;
  }

  if(rgnSegmentText)
  {
    TAP_Osd_Delete(rgnSegmentText);
    rgnSegmentText = 0;
  }

  if(rgnActionMenu)
  {
    TAP_Osd_Delete(rgnActionMenu);
    rgnActionMenu = 0;
  }

  if(rgnSegmentList)
  {
    TAP_Osd_Delete(rgnSegmentList);
    rgnSegmentList = 0;
  }

  if(rgnInfoBar)
  {
    TAP_Osd_Delete(rgnInfoBar);
    rgnInfoBar = 0;
  }

  if(rgnInfoBarMini)
  {
    TAP_Osd_Delete(rgnInfoBarMini);
    rgnInfoBarMini = 0;
  }
  TAP_Osd_Sync();

  CloseLogMC();
  if(EnterNormal)
  {
    dword SysState, SysSubState;
    TAP_GetState(&SysState, &SysSubState);
    if(SysSubState == 0) TAP_EnterNormal();
  }
  TRACEEXIT();
}

void Cleanup(bool DoClearOSD)
{
  TRACEENTER();

//  RecFileSize = 0;
  LastTotalBlocks = 0;
  MediaFileMode = FALSE;
  LinearTimeMode = FALSE;
  JumpRequestedSegment = 0xFFFF;
  JumpRequestedBlock = (dword) -1;
  JumpRequestedTime = 0;
  JumpPerformedTime = 0;
  NrBookmarks = 0;
  if(TimeStamps)
  {
    TAP_MemFree(TimeStamps);
    TimeStamps = NULL;
  }
//  LastTimeStamp = NULL;
//  NrTimeStamps = 0;
  OSDMode = DefaultOSDMode;
  ShowSegmentText = FALSE;
  UndoResetStack();
  if (DoClearOSD) ClearOSD(TRUE);
  CloseLogMC();

  TRACEEXIT();
}

void CleanupCut(void)
{
  TRACEENTER();
  if (DeleteCutFiles)
  {
    if (DeleteCutFiles != 2)
      system("sh " TAPFSROOT LOGDIR "/DeleteCuts.sh &");
    else
      system("sh " TAPFSROOT LOGDIR "/DeleteCuts.sh --recursive &");
  }
  TRACEEXIT();
}

// ----------------------------------------------------------------------------
//                             INI-Funktionen
// ----------------------------------------------------------------------------
void LoadINI(void)
{
  long                  SpecialEndMode;
  TRACEENTER();
  INILOCATION IniFileState;

  HDD_TAP_PushDir();
  TAP_Hdd_ChangeDir(LOGDIR);
  IniFileState = INIOpenFile(INIFILENAME, PROGRAM_NAME);
  if((IniFileState != INILOCATION_NotFound) && (IniFileState != INILOCATION_NewFile))
  {
    AutoOSDPolicy       =                INIGetInt("AutoOSDPolicy",              0,   0,    1)   ==   1;
    DefaultOSDMode      =     (tOSDMode) INIGetInt("DefaultOSDMode",    MD_FullOSD,   0,    3);
    DefaultMinuteJump   =                INIGetInt("DefaultMinuteJump",          0,   0,   99);
    ShowRebootMessage   =                INIGetInt("ShowRebootMessage",          1,   0,    1)   !=   0;
    MaxNavDiscrepancy   =                INIGetInt("MaxNavDiscrepancy",       5000,   0,  86400000);       // = 24 Stunden
    AskBeforeEdit       =                INIGetInt("AskBeforeEdit",              1,   0,    1)   !=   0;
    CutFileMode         = (tCutSaveMode) INIGetInt("CutFileMode",          CM_Both,   0,    2);
    SaveCutBak          =                INIGetInt("SaveCutBak",                 1,   0,    1)   !=   0;
    DeleteCutFiles      =                INIGetInt("DeleteCutFiles",             1,   0,    2);
    SpecialEndMode      =                INIGetInt("SpecialEndMode",             1,   0,    2);
      DisableSpecialEnd =                (SpecialEndMode == 0);
      ForceSpecialEnd   =                (SpecialEndMode == 2);
    DoiCheckTest        =  (tiCheckMode) INIGetInt("DoiCheckTest",        IM_ROEnd,   0,    4);
    CheckFSAfterCut     = (tCheckFSMode) INIGetInt("CheckFSAfterCut",      FM_Auto,   0,    3);
    InodeMonitoring     =                INIGetInt("InodeMonitoring",            0,   0,    1)   ==   1;
    StripMode           =                INIGetInt("StripMode",                  1,   0,    3);

    Overscan_X          =                INIGetInt("Overscan_X",                50,   0,  100);
    Overscan_Y          =                INIGetInt("Overscan_Y",                25,   0,  100);
    SegmentList_X       =                INIGetInt("SegmentList_X",             50,   0,  ScreenWidth - _SegmentList_Background_Gd.width);
    SegmentList_Y       =                INIGetInt("SegmentList_Y",             82,   0,  ScreenHeight - _SegmentList_Background_Gd.height);

    RCUMode             =     (tRCUMode) INIGetInt("RCUMode",              RC_auto,   0,    5);
    DirectSegmentsCut   =                INIGetInt("DirectSegmentsCut",          0,   0,    1)   ==   1;
    DisableSleepKey     =                INIGetInt("DisableSleepKey",            0,   0,    1)   ==   1;
  }
  INICloseFile();
  if (!AutoOSDPolicy && DefaultOSDMode == MD_NoOSD)
    DefaultOSDMode = MD_FullOSD;
//  if (DisableSpecialEnd) ForceSpecialEnd = FALSE;

  if(IniFileState == INILOCATION_NewFile)
    SaveINI();

  if (RCUMode == RC_auto)
  {
    switch(GetSysID())
    {
      case 22010:    // TMS-2100
      case 22121:    // SRP-2410
      case 22122:    // SRP-2410M
      case 22570:    // SRP-2401CI+ Eco
      case 42561:    // CRP-2401CI+ Conax
        RCUMode = RC_SRP2410;
        break;
      case 22120:    // SRP-2401CI+
      case 22130:    // SRP-2401CI+
        RCUMode = RC_SRP2401;
        break;
      case 42031:    // CRP-2401CI+
        RCUMode = RC_CRP;
        break;
      default:
        RCUMode = RC_SRP2401;
    }
  }
  HDD_TAP_PopDir();

  TRACEEXIT();
}

void SaveINI(void)
{
  TRACEENTER();
  FILE *f = NULL;

  if ((f = fopen(TAPFSROOT LOGDIR "/" INIFILENAME, "wb")) != NULL)
  {
    fprintf(f, "[MovieCutter]\r\n");
    fprintf(f, "%s=%d\r\n",  "AutoOSDPolicy",       AutoOSDPolicy       ?  1  :  0);
    fprintf(f, "%s=%d\r\n",  "DefaultOSDMode",      DefaultOSDMode);
    fprintf(f, "%s=%d\r\n",  "DefaultMinuteJump",   DefaultMinuteJump);
    fprintf(f, "%s=%d\r\n",  "ShowRebootMessage",   ShowRebootMessage   ?  1  :  0);
    fprintf(f, "%s=%lu\r\n", "MaxNavDiscrepancy",   MaxNavDiscrepancy);
    fprintf(f, "%s=%d\r\n",  "AskBeforeEdit",       AskBeforeEdit       ?  1  :  0);
    fprintf(f, "%s=%d\r\n",  "CutFileMode",         CutFileMode);
    fprintf(f, "%s=%d\r\n",  "SaveCutBak",          SaveCutBak          ?  1  :  0);
    fprintf(f, "%s=%d\r\n",  "DeleteCutFiles",      DeleteCutFiles);
    fprintf(f, "%s=%d\r\n",  "SpecialEndMode",      DisableSpecialEnd ? 0 : (ForceSpecialEnd ? 2 : 1));
    fprintf(f, "%s=%d\r\n",  "DoiCheckTest",        DoiCheckTest);
    fprintf(f, "%s=%d\r\n",  "CheckFSAfterCut",     CheckFSAfterCut);
    fprintf(f, "%s=%d\r\n",  "InodeMonitoring",     InodeMonitoring     ?  1  :  0);
    fprintf(f, "%s=%d\r\n",  "StripMode",           StripMode);
    fprintf(f, "%s=%lu\r\n", "Overscan_X",          Overscan_X);
    fprintf(f, "%s=%lu\r\n", "Overscan_Y",          Overscan_Y);
    fprintf(f, "%s=%lu\r\n", "SegmentList_X",       SegmentList_X);
    fprintf(f, "%s=%lu\r\n", "SegmentList_Y",       SegmentList_Y);
    fprintf(f, "%s=%d\r\n",  "RCUMode",             RCUMode);
    fprintf(f, "%s=%d\r\n",  "DirectSegmentsCut",   DirectSegmentsCut   ?  1  :  0);
    fprintf(f, "%s=%d\r\n",  "DisableSleepKey",     DisableSleepKey     ?  1  :  0);
    fclose(f);
    HDD_SetFileDateTime(INIFILENAME, TAPFSROOT LOGDIR, 0);
  }
  TRACEEXIT();
}


// ----------------------------------------------------------------------------
//                         SegmentMarker-Funktionen
// ----------------------------------------------------------------------------
void CountSelectedSegments()
{
  int i;
  TRACEENTER();

  NrSelectedSegments = 0;
  for(i = 0; i < NrSegmentMarker - 1; i++)
  {
    if(SegmentMarker[i].Selected) NrSelectedSegments++;
  }
  TRACEEXIT();
}
  
void ImportBookmarksToSegments(void)
{
  char LogString[512];
  dword newBlock;
  int i;

  TRACEENTER();

  if (NrBookmarks > 0)
  {
    // first, delete all present segment markers (*CW*)
//    UndoResetStack();
    DeleteAllSegmentMarkers();

    // second, add a segment marker for each bookmark
    WriteLogMCf(PROGRAM_NAME, "Importing %d of %d bookmarks", min(NrBookmarks, NRSEGMENTMARKER-2), NrBookmarks);

    for(i = 0; i < min(NrBookmarks, NRSEGMENTMARKER-2); i++)
    { 
      newBlock = Bookmarks[i];
      TAP_SPrint(LogString, sizeof(LogString), "Bookmark %d @ %lu", i + 1, newBlock);
      // Erlaube keinen neuen SegmentMarker zu knapp am Anfang oder Ende oder über totalBlock
      if ((newBlock > SegmentMarker[0].Block + 3*BlocksOneSecond) && (newBlock + 3*BlocksOneSecond < SegmentMarker[NrSegmentMarker-1].Block))
        AddSegmentMarker(&newBlock, TRUE, FALSE);
      else
        TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), " -> ignored! (Too close to begin/end.)");
      WriteLogMC(PROGRAM_NAME, LogString);
    }

    SetCurrentSegment();
    OSDSegmentListDrawList(FALSE);
    OSDInfoDrawProgressbar(TRUE, TRUE);
//    OSDRedrawEverything();
  }
  TRACEEXIT();
}

void AddDefaultSegmentMarker(void)
{
  TRACEENTER();

  ResetSegmentMarkers();
  SegmentMarker[1].Block   = PlayInfo.totalBlock;
  SegmentMarker[1].Timems  = NavGetBlockTimeStamp(SegmentMarker[1].Block);
  SegmentMarker[1].Percent = 100.0;
  NrSegmentMarker = 2;

  TRACEEXIT();
}

int AddSegmentMarker(dword *pNewBlock, bool MoveToIFrame, bool RejectSmallSegments)
{
  char                  StartTime[16];
  dword                 newTime;
  int                   ret = -1;
  int                   i, j;

  TRACEENTER();

  if(!pNewBlock || *pNewBlock > PlayInfo.totalBlock  /*!PLAYINFOVALID() || ((int)*pNewBlock < 0)*/)   // PlayInfo ist sicher, da in Z.804 und im ActionMenu überprüft, Prüfung für newBlock nun restriktiver
  {
    WriteLogMC(PROGRAM_NAME, "AddSegmentMarker: newBlock > totalBlock - darf nicht auftreten!");
    TRACEEXIT();
    return -1;
  }

  if(NrSegmentMarker >= NRSEGMENTMARKER)
  {
    WriteLogMC(PROGRAM_NAME, "AddSegmentMarker: SegmentMarker list is full!");
    ShowErrorMessage(LangGetString(LS_ListIsFull), NULL);
//    OSDRedrawEverything();
    TRACEEXIT();
    return -1;
  }

  newTime = NavGetBlockTimeStamp(*pNewBlock);

  // Verschiebe Segment bis zum nächsten I-Frame
  if (MoveToIFrame)
  {
    if (!LinearTimeMode && newTime)
    {
//TAP_PrintNet("AddSegment: Verschiebe SegmentMarker. WunschPos=%lu, MarkerPos=%lu, Differenz=%ld.\n", *pNewBlock, LastTimeStamp->BlockNr - (CUTPOINTSEARCHRADIUS/9024), LastTimeStamp->BlockNr - (CUTPOINTSEARCHRADIUS/9024) - *pNewBlock);
      *pNewBlock = 0;
      newTime = 0;
      if (LastTimeStamp->BlockNr > (dword)(CUTPOINTSEARCHRADIUS/9024) + 1)
      {
        *pNewBlock = LastTimeStamp->BlockNr - (CUTPOINTSEARCHRADIUS/9024) - 1;
        newTime = LastTimeStamp->Timems;
      }
    }
/*    else if (!MediaFileMode && VideoDetected)  // bei MediaFileMode ist isHD nicht bekannt, bei Radio keine I-Frame-Verschiebung
    {
      *pNewBlock = FindNextIFrame(PlaybackName, AbsPlaybackDir, *pNewBlock, HDVideo);
      newTime = NavGetBlockTimeStamp(*pNewBlock);
    }  */
  }

/*  if((newTime == 0) && (*pNewBlock > 3 * BlocksOneSecond))
  {
    TRACEEXIT();
    return -1;
  } */

  //Find the point where to insert the new marker so that the list stays sorted
  for(i = 1; i < NrSegmentMarker; i++)
  {
    if(SegmentMarker[i].Block >= *pNewBlock)
    {
      if(SegmentMarker[i].Block == *pNewBlock)
      {
        TRACEEXIT();
        return -1;
      }

      // Erlaube kein Segment mit weniger als 3 Sekunden
      if (RejectSmallSegments && ((i > 1 && (*pNewBlock <= SegmentMarker[i-1].Block + 3*BlocksOneSecond)) || (i < NrSegmentMarker-2 && (*pNewBlock + 3*BlocksOneSecond >= SegmentMarker[i].Block))))
      {
        TRACEEXIT();
        return -1;
      }

      for(j = NrSegmentMarker; j > i; j--)
        memcpy(&SegmentMarker[j], &SegmentMarker[j - 1], sizeof(tSegmentMarker));

      SegmentMarker[i].Block = *pNewBlock;
      SegmentMarker[i].Timems = newTime;
      SegmentMarker[i].Percent = ((float)*pNewBlock / PlayInfo.totalBlock) * 100.0;
      SegmentMarker[i].Selected = FALSE;
      SegmentMarker[i].pCaption = NULL;
      NrSegmentMarker++;

      MSecToTimeString(SegmentMarker[i].Timems, StartTime);
      WriteLogMCf(PROGRAM_NAME, "New marker @ block = %lu, time = %s, percent = %1.1f%%", *pNewBlock, StartTime, SegmentMarker[i].Percent);

      if(ActiveSegment + 1 >= i) ActiveSegment++;
      ret = i;
      break;
    }
  }

  TRACEEXIT();
  return ret;
}

int FindSegmentWithBlock(dword curBlock)
{
  int                   SegmentIndex;
  int                   i;

  TRACEENTER();
  SegmentIndex = 0;
  
  if (NrSegmentMarker > 2)
  {
    for(i = 0; i < NrSegmentMarker - 1; i++)
      if(SegmentMarker[i + 1].Block > curBlock)
      {
        SegmentIndex = i;
        break;
      }
  }
  TRACEEXIT();
  return SegmentIndex;
}

int FindNearestSegmentMarker(dword curBlock)
{
  int                   NearestMarkerIndex;
  long                  MinDelta;
  int                   i;

  TRACEENTER();

  NearestMarkerIndex = -1;
  if(NrSegmentMarker > 2)   // at least one segment marker present
  {
    MinDelta = 0x7fffffff;
    for(i = 1; i < NrSegmentMarker - 1; i++)
    {
      if(labs(SegmentMarker[i].Block - curBlock) < MinDelta)
      {
        MinDelta = labs(SegmentMarker[i].Block - curBlock);
        NearestMarkerIndex = i;
      }
      else
        break;
    }
//    if (NearestMarkerIndex == 0) NearestMarkerIndex = 1;
//    if (NearestMarkerIndex == NrSegmentMarker-1) NearestMarkerIndex = NrSegmentMarker - 2;
  }

  TRACEEXIT();
  return NearestMarkerIndex;
}

bool MoveSegmentMarker(int MarkerIndex, dword *pNewBlock, bool MoveToIFrame, bool RejectSmallSegments)
{
  dword newTime;
  bool ret = FALSE;

  TRACEENTER();

  if(*pNewBlock > PlayInfo.totalBlock  /*!PLAYINFOVALID() || ((int)newBlock < 0)*/)   // PlayInfo ist sicher, da in Z.804 überprüft, Prüfung für newBlock nun restriktiver
  {
    WriteLogMC(PROGRAM_NAME, "MoveSegmentMarker: newBlock > totalBlock - darf nicht auftreten!");
    TRACEEXIT();
    return FALSE;
  }

  if((MarkerIndex > 0) && (MarkerIndex < NrSegmentMarker - 1))
  {
    // neue Zeit ermitteln
    newTime = NavGetBlockTimeStamp(*pNewBlock);

    // Verschiebe Segment bis zum nächsten I-Frame
    if (MoveToIFrame)
    {
      if (!LinearTimeMode && newTime)
      {
//TAP_PrintNet("MoveSegment: Verschiebe SegmentMarker. WunschPos=%lu, MarkerPos=%lu, Differenz=%ld.\n", *pNewBlock, LastTimeStamp->BlockNr - (CUTPOINTSEARCHRADIUS/9024), LastTimeStamp->BlockNr - (CUTPOINTSEARCHRADIUS/9024) - *pNewBlock);
        *pNewBlock = 0;
        newTime = 0;
        if (LastTimeStamp->BlockNr > (dword)(CUTPOINTSEARCHRADIUS/9024) + 1)
        {
          *pNewBlock = LastTimeStamp->BlockNr - (CUTPOINTSEARCHRADIUS/9024) - 1;
          newTime = LastTimeStamp->Timems;
        }
      }
/*    else if (!MediaFileMode && VideoDetected)  // bei MediaFileMode ist isHD nicht bekannt, bei Radio keine I-Frame-Verschiebung
      {
        *pNewBlock = FindNextIFrame(PlaybackName, AbsPlaybackDir, *pNewBlock, HDVideo);
        newTime = NavGetBlockTimeStamp(*pNewBlock);
      }  */
    }

    if (SegmentMarker[MarkerIndex].Block == *pNewBlock)
    {
      TRACEEXIT();
      return FALSE;
    }
    // Erlaube kein Segment mit weniger als 3 Sekunden
    if (RejectSmallSegments && ((MarkerIndex > 1 && (*pNewBlock <= SegmentMarker[MarkerIndex-1].Block + 3*BlocksOneSecond)) || (MarkerIndex < NrSegmentMarker-2 && (*pNewBlock + 3*BlocksOneSecond >= SegmentMarker[MarkerIndex+1].Block))))
    {
      TRACEEXIT();
      return FALSE;
    }

//    if((newTime != 0) || (newBlock <= 3 * BlocksOneSecond))
    {
      SegmentMarker[MarkerIndex].Block = *pNewBlock;
      SegmentMarker[MarkerIndex].Timems = newTime;
      SegmentMarker[MarkerIndex].Percent = ((float)*pNewBlock / PlayInfo.totalBlock) * 100.0;
      ret = TRUE;
    }
  }

  TRACEEXIT();
  return ret;
}

bool DeleteSegmentMarker(int MarkerIndex, bool FreeCaption)
{
  int i;
  bool ret = FALSE;

  TRACEENTER();

  if((MarkerIndex > 0) && (MarkerIndex < NrSegmentMarker - 1))
  {
    if (FreeCaption && SegmentMarker[MarkerIndex].pCaption)
      TAP_MemFree(SegmentMarker[MarkerIndex].pCaption);

    for(i = MarkerIndex; i < NrSegmentMarker - 1; i++)
      memcpy(&SegmentMarker[i], &SegmentMarker[i + 1], sizeof(tSegmentMarker));

    memset(&SegmentMarker[NrSegmentMarker - 1], 0, sizeof(tSegmentMarker));
    NrSegmentMarker--;

    SegmentMarker[NrSegmentMarker - 1].Selected = FALSE;          // the very last marker (no segment)
//    if(NrSegmentMarker <= 2) SegmentMarker[0].Selected = FALSE;  // there is only one segment (from start to end)
    if(ActiveSegment >= MarkerIndex) ActiveSegment--;
    if(ActiveSegment >= NrSegmentMarker - 1) ActiveSegment = NrSegmentMarker - 2;
    JumpRequestedSegment = 0xFFFF;
    ret = TRUE;
  }

  TRACEEXIT();
  return ret;
}

void DeleteAllSegmentMarkers()
{
  int i;
  TRACEENTER();

  if (NrSegmentMarker > 2)
  {
    UndoResetStack();
    for (i = 0; i < NrSegmentMarker; i++)
      if (SegmentMarker[i].pCaption)
      {
        TAP_MemFree(SegmentMarker[i].pCaption);
        SegmentMarker[i].pCaption = NULL;
      }
    memcpy(&SegmentMarker[1], &SegmentMarker[NrSegmentMarker-1], sizeof(tSegmentMarker));
//    memset(&SegmentMarker[2], 0, (NrSegmentMarker-2) * sizeof(tSegmentMarker));
    NrSegmentMarker = 2;
  }
  SegmentMarker[0].Selected = FALSE;
  ActiveSegment = 0;
  JumpRequestedSegment = 0xFFFF;

  TRACEEXIT();
}

static void ResetSegmentMarkers(void)
{
  int i;
  TRACEENTER();

  for (i = 0; i < NrSegmentMarker; i++)
    if (SegmentMarker[i].pCaption)
      TAP_MemFree(SegmentMarker[i].pCaption);
  memset(SegmentMarker, 0, NRSEGMENTMARKER * sizeof(tSegmentMarker));
  NrSegmentMarker = 0;

  TRACEEXIT();
}

bool SelectSegmentMarker(void)
{
  int                   VisibleSegment;
  bool                  ret = FALSE;
  TRACEENTER();

  if (NrSegmentMarker >= 2)
  {
    if (JumpRequestedSegment != 0xFFFF)
      VisibleSegment = JumpRequestedSegment;
    else if (JumpRequestedBlock != (dword) -1)
      VisibleSegment = FindSegmentWithBlock(JumpRequestedBlock);
    else
      VisibleSegment = ActiveSegment;

    SegmentMarker[VisibleSegment].Selected = !SegmentMarker[VisibleSegment].Selected;
    ret = SegmentMarker[VisibleSegment].Selected;
  }
  TRACEEXIT();
  return ret;
}

void ChangeSegmentText(void)
{
  word CurrentSegment;

  if (JumpRequestedSegment != 0xFFFF)
    CurrentSegment = JumpRequestedSegment;
  else
    CurrentSegment = ActiveSegment;
  if(CurrentSegment >= NrSegmentMarker - 1)
    CurrentSegment = NrSegmentMarker - 2;

  Playback_Pause();

  // Neuen Puffer anlegen für die Tastatureingabe
  char *pNewCaption = (char*)TAP_MemAlloc(MAXCAPTIONLENGTH);  pNewCaption[0] = '\0';
  if (SegmentMarker[CurrentSegment].pCaption)
  {
    strncpy(pNewCaption, SegmentMarker[CurrentSegment].pCaption, MAXCAPTIONLENGTH);
    pNewCaption[MAXCAPTIONLENGTH-1] = '\0';
  }

  // OSD-Tastatur anzeigen
  OSDMenuKeyboard_Setup(LangGetString(LS_EnterText), pNewCaption, MAXCAPTIONLENGTH-1, TRUE);
  #ifdef MC_MULTILANG
    if (LangStrings)
      OSDMenuKeyboard_SetLegendStrings(LangGetString(LS_KeybSpace), LangGetString(LS_KeybDelete1), LangGetString(LS_KeybDelete2), LangGetString(LS_KeybOriginal), LangGetString(LS_KeybClearAll), LangGetString(LS_KeybCopy), LangGetString(LS_KeybPaste), LangGetString(LS_KeybCancel), LangGetString(LS_KeybSave));
  #endif
  OSDMenuKeyboard_Show();
  while (OSDMenuKeyboard_isVisible())
  {
    TAP_SystemProc();
    TAP_Sleep(1);
  }
  
  // pCaption auf den neuen Puffer verlinken
  if (OSDMenuKeyboard_Saved())
  {
    UndoAddEvent(TRUE, SegmentMarker[CurrentSegment].Block, SegmentMarker[CurrentSegment].Block, SegmentMarker[CurrentSegment].Selected, SegmentMarker[CurrentSegment].pCaption);
    if (*pNewCaption)
      SegmentMarker[CurrentSegment].pCaption = pNewCaption;
    else
    {
      SegmentMarker[CurrentSegment].pCaption = NULL;
      TAP_MemFree(pNewCaption);
    }
  }
  OSDSegmentListDrawList(FALSE);
  OSDSegmentTextDraw(TRUE);
  TRACEEXIT();
}


// ----------------------------------------------------------------------------
//                           Bookmark-Funktionen
// ----------------------------------------------------------------------------
void ExportSegmentsToBookmarks(void)
{
  int i;

  TRACEENTER();

  if (NrSegmentMarker > 2 && !MediaFileMode)
  {
    // first, delete all present bookmarks
//    UndoResetStack();
    DeleteAllBookmarks();
    NrBookmarks = 0;

    // second, add a bookmark for each SegmentMarker
    WriteLogMCf(PROGRAM_NAME, "Exporting %d of %d segment markers", min(NrSegmentMarker-2, 48), NrSegmentMarker-2);

    for(i = 1; i <= min(NrSegmentMarker-2, 48); i++)
    { 
      Bookmarks[NrBookmarks] = SegmentMarker[i].Block;
      NrBookmarks++;
    }

    if (!SaveBookmarks(Bookmarks, NrBookmarks, FALSE))
    {
      WriteLogMC(PROGRAM_NAME, "ExportSegmentsToBookmarks: SaveBookmarks() failed!");
      if (!ReadBookmarks(Bookmarks, &NrBookmarks))
        WriteLogMC(PROGRAM_NAME, "ExportSegmentsToBookmarks: -> ReadBookmarks() failed!");
    }
    OSDInfoDrawProgressbar(TRUE, TRUE);
//    OSDRedrawEverything();
  }
  TRACEEXIT();
}

int AddBookmark(dword newBlock, bool RejectSmallScenes)
{
  int ret = -1;
  int i, j;

  TRACEENTER();

  if(newBlock > PlayInfo.totalBlock  /*!PLAYINFOVALID() || ((int)newBlock < 0)*/)   // PlayInfo ist sicher, da in Z.804 überprüft, Prüfung für newBlock nun restriktiver
  {
    WriteLogMC(PROGRAM_NAME, "AddBookmark: newBlock > totalBlock - darf nicht auftreten!");
    TRACEEXIT();
    return -1;
  }

  if(NrBookmarks >= 48)
  {
    WriteLogMC(PROGRAM_NAME, "AddBookmark: Bookmark list is full!");
    TRACEEXIT();
    return -1;
  }

  //Find the point where to insert the new marker so that the list stays sorted
  if (NrBookmarks == 0)
  {
    Bookmarks[0] = newBlock;
    ret = 0;
  }
  else if (newBlock > Bookmarks[NrBookmarks - 1])
  {
    // Erlaube keinen Abschnitt mit weniger als 3 Sekunden
    if (RejectSmallScenes && (newBlock <= Bookmarks[NrBookmarks-1] + 3*BlocksOneSecond))
    {
      TRACEEXIT();
      return -1;
    }
    Bookmarks[NrBookmarks] = newBlock;
    ret = NrBookmarks;
  }
  else
  {
    for(i = 0; i < NrBookmarks; i++)
    {
      if(Bookmarks[i] >= newBlock)
      {
        if(Bookmarks[i] == newBlock)
        {
          TRACEEXIT();
          return -1;
        }

        // Erlaube keinen Abschnitt mit weniger als 3 Sekunden
        if (RejectSmallScenes && ((i > 0 && (newBlock <= Bookmarks[i-1] + 3*BlocksOneSecond)) || (newBlock + 3*BlocksOneSecond >= Bookmarks[i])))
        {
          TRACEEXIT();
          return -1;
        }

        for(j = NrBookmarks; j > i; j--)
          Bookmarks[j] = Bookmarks[j - 1];

        Bookmarks[i] = newBlock;
        ret = i;
        break;
      }
    }
  }
  NrBookmarks++;
  if (!SaveBookmarks(Bookmarks, NrBookmarks, FALSE))
  {
    ret = -1;
    WriteLogMC(PROGRAM_NAME, "AddBookmark: SaveBookmarks() failed!");
    if (!ReadBookmarks(Bookmarks, &NrBookmarks))
      WriteLogMC(PROGRAM_NAME, "AddBookmark: -> ReadBookmarks() failed!");
  }

  TRACEEXIT();
  return ret;
}

int FindNearestBookmark(dword curBlock)
{
  int                   NearestBookmarkIndex;
  long                  MinDelta;
  int                   i;

  TRACEENTER();

  NearestBookmarkIndex = -1;
  if(NrBookmarks > 0)
  {
    MinDelta = 0x7fffffff;
    for(i = 0; i < NrBookmarks; i++)
    {
      if(labs(Bookmarks[i] - curBlock) < MinDelta)
      {
        MinDelta = labs(Bookmarks[i] - curBlock);
        NearestBookmarkIndex = i;
      }
    }
  }

  TRACEEXIT();
  return NearestBookmarkIndex;
}

bool MoveBookmark(int BookmarkIndex, dword newBlock, bool RejectSmallScenes)
{
  int                   i;
  bool                  ret = FALSE;
  TRACEENTER();

  if(newBlock > PlayInfo.totalBlock  /*!PLAYINFOVALID() || ((int)newBlock < 0)*/)   // PlayInfo ist sicher, da in Z.804 überprüft, Prüfung für newBlock nun restriktiver
  {
    WriteLogMC(PROGRAM_NAME, "MoveBookmark: newBlock > totalBlock - darf nicht auftreten!");
    TRACEEXIT();
    return FALSE;
  }

  if ((BookmarkIndex >= 0) && (BookmarkIndex < NrBookmarks))
  {
    if (Bookmarks[BookmarkIndex] == newBlock)
    {
      TRACEEXIT();
      return FALSE;
    }
    
    // Erlaube keinen Abschnitt mit weniger als 3 Sekunden
    if (RejectSmallScenes && ((BookmarkIndex > 0 && (newBlock <= Bookmarks[BookmarkIndex-1] + 3*BlocksOneSecond)) || (BookmarkIndex < NrBookmarks-1 && (newBlock + 3*BlocksOneSecond >= Bookmarks[BookmarkIndex+1]))))
    {
      TRACEEXIT();
      return FALSE;
    }
    Bookmarks[BookmarkIndex] = newBlock;

    // Verschiebe das Bookmark an die richtige Stelle, falls die Sortierung nicht passt (z.B. bei Rückgängig)
    i = BookmarkIndex;
    while ((i > 0) && (Bookmarks[i-1] > newBlock))
    {
      Bookmarks[i] = Bookmarks[i-1];
      Bookmarks[i-1] = newBlock;
      i--;
    }
    while ((i < NrBookmarks-1) && (Bookmarks[i+1] < newBlock))
    {
      Bookmarks[i] = Bookmarks[i+1];
      Bookmarks[i+1] = newBlock;
      i++;
    }

    ret = SaveBookmarks(Bookmarks, NrBookmarks, FALSE);
    if (!ret)
    {
      WriteLogMC(PROGRAM_NAME, "MoveBookmark: SaveBookmarks() failed!");
      if (!ReadBookmarks(Bookmarks, &NrBookmarks))
        WriteLogMC(PROGRAM_NAME, "MoveBookmark: -> ReadBookmarks() failed!");
    }
  }

  TRACEEXIT();
  return ret;
}

bool DeleteBookmark(int BookmarkIndex)
{
  bool ret = FALSE;
  int i;

  TRACEENTER();

  if ((BookmarkIndex >= 0) && (BookmarkIndex < NrBookmarks))
  {
    for(i = BookmarkIndex; i < NrBookmarks - 1; i++)
      Bookmarks[i] = Bookmarks[i + 1];
    Bookmarks[NrBookmarks - 1] = 0;
    NrBookmarks--;

    ret = SaveBookmarks(Bookmarks, NrBookmarks, FALSE);
    if (!ret)
    {
      WriteLogMC(PROGRAM_NAME, "DeleteBookmark: SaveBookmarks() failed!");
      if (!ReadBookmarks(Bookmarks, &NrBookmarks))
        WriteLogMC(PROGRAM_NAME, "DeleteBookmark: -> ReadBookmarks() failed!");
    }
  }

  TRACEEXIT();
  return ret;
}

bool DeleteAllBookmarks(void)
{
  bool ret = FALSE;
  int i;
  TRACEENTER();

  if (NrBookmarks > 0)
  {
    UndoResetStack();
    for(i = 0; i < NrBookmarks; i++)
      Bookmarks[i] = 0;
  }
  NrBookmarks = 0;

  ret = SaveBookmarks(Bookmarks, NrBookmarks, FALSE);
  if (!ret)
    WriteLogMC(PROGRAM_NAME, "DeleteAllBookmarks: -> SaveBookmarks() failed!");

  TRACEEXIT();
  return ret;
}

// ----------------------------------------------------------------------------
//                          Rückgängig-Funktionen
// ----------------------------------------------------------------------------
void UndoAddEvent(bool Segment, dword PreviousBlock, dword NewBlock, bool SegmentWasSelected, char *pPrevCaption)
{
  tUndoEvent* NextAction;
  TRACEENTER();

  if (!(Segment == FALSE && PreviousBlock == 0 && NewBlock == 0))
  {
    UndoLastItem++;
    if (UndoLastItem >= NRUNDOEVENTS)
      UndoLastItem = 0;
    NextAction = &UndoStack[UndoLastItem];
    #ifdef FULLDEBUG
      TAP_PrintNet("MovieCutter: UndoAddEvent %d (%s, PrevBlock=%lu, NewBlock=%lu)\n", UndoLastItem, (Segment) ? "Segment" : "Bookmark", PreviousBlock, NewBlock);
    #endif

    NextAction->Segment            = Segment;
    NextAction->PrevBlockNr        = PreviousBlock;
    NextAction->NewBlockNr         = NewBlock;
    NextAction->SegmentWasSelected = SegmentWasSelected;
    if (NextAction->pPrevCaption)
      TAP_MemFree(NextAction->pPrevCaption);
    NextAction->pPrevCaption       = pPrevCaption;
  }
  TRACEEXIT();
}

bool UndoLastAction(void)
{
  tUndoEvent* LastAction;
  int i;

  TRACEENTER();

  if (UndoLastItem >= NRUNDOEVENTS)
  {
    TRACEEXIT();
    return FALSE;
  }
  LastAction = &UndoStack[UndoLastItem];
  #ifdef FULLDEBUG
    TAP_PrintNet("MovieCutter: UndoLastAction %d (%s, PrevBlock=%lu, NewBlock=%lu)\n", UndoLastItem, (LastAction->Segment) ? "Segment" : "Bookmark", LastAction->PrevBlockNr, LastAction->NewBlockNr);
  #endif

  if (LastAction->Segment)
  {
    if ((LastAction->NewBlockNr == 0) && (LastAction->PrevBlockNr != 0))
    {
      i = AddSegmentMarker(&LastAction->PrevBlockNr, FALSE, FALSE);
      if (i > 0)
      {
        SegmentMarker[i].Selected = LastAction->SegmentWasSelected;
        SegmentMarker[i].pCaption = LastAction->pPrevCaption;
      }
    }
    else
    {
      for(i = 0; i < NrSegmentMarker - 1; i++)
        if(SegmentMarker[i].Block == LastAction->NewBlockNr) break;
      if((i < NrSegmentMarker - 1) && (SegmentMarker[i].Block == LastAction->NewBlockNr))
      {
        if (LastAction->NewBlockNr == LastAction->PrevBlockNr)
        {
          if (SegmentMarker[i].pCaption)
            TAP_MemFree(SegmentMarker[i].pCaption);
          SegmentMarker[i].pCaption = LastAction->pPrevCaption;
        }
        else if (LastAction->NewBlockNr != 0)
        {
          if (LastAction->PrevBlockNr != 0)
            MoveSegmentMarker(i, &LastAction->PrevBlockNr, FALSE, FALSE);
          else
            DeleteSegmentMarker(i, TRUE);
        }
      }
    }
  }
  else
  {
    if (LastAction->NewBlockNr != 0)
    {
      if(LastAction->NewBlockNr == (dword) -1)  LastAction->NewBlockNr = 0;
      for(i = 0; i < NrBookmarks; i++)
        if(Bookmarks[i] == LastAction->NewBlockNr) break;
      if((i < NrBookmarks) && (Bookmarks[i] == LastAction->NewBlockNr))
      {
        if (LastAction->PrevBlockNr != 0)
        {
          if(LastAction->PrevBlockNr == (dword) -1)  LastAction->PrevBlockNr = 0;
          MoveBookmark(i, LastAction->PrevBlockNr, FALSE);  // BUG (fixed): Beim Undo einer BM-Löschung wird ein evtl. an Stelle 0 existierender BM (fälschlich) verschoben! + falsche Reihenfolge!!
        }
        else
          DeleteBookmark(i);    // BUG (fixed): Beim Undo einer BM-Verschiebung von 0 wird das BM nicht auf 0 zurückgeschoben, sondern gelöscht!
      }
    }
    else if (LastAction->PrevBlockNr != 0)
    {
      if(LastAction->PrevBlockNr == (dword) -1)  LastAction->PrevBlockNr = 0;
      AddBookmark(LastAction->PrevBlockNr, FALSE);    // BUG (fixed): Beim Undo einer BM-Verschiebung auf 0 wird das dortige BM nicht verschoben, sondern ein neues erzeugt!
    }
    else
    {
      TRACEEXIT();
      return FALSE;
    }
  }

  LastAction->Segment = FALSE;
  LastAction->PrevBlockNr = 0;
  LastAction->NewBlockNr = 0;
  LastAction->pPrevCaption = NULL;

  if(UndoLastItem > 0)
    UndoLastItem--;
  else
    UndoLastItem = NRUNDOEVENTS - 1;

  TRACEEXIT();
  return TRUE;
}

void UndoResetStack(void)
{
  int i;
  TRACEENTER();
  #ifdef FULLDEBUG
    TAP_PrintNet("MovieCutter: UndoResetStack()\n");
  #endif

  for (i = 0; i < NRUNDOEVENTS; i++)
    if (UndoStack[i].pPrevCaption)
      TAP_MemFree(UndoStack[i].pPrevCaption);
  memset(UndoStack, 0, NRUNDOEVENTS * sizeof(tUndoEvent));
  UndoLastItem = 0;

  TRACEEXIT();
}

// ----------------------------------------------------------------------------
//                           CutFile-Funktionen
// ----------------------------------------------------------------------------
bool CutFileDecodeBin(FILE *fCut, __off64_t *OutSavedSize)
{
  byte                  Version;
  int                   SavedNrSegments = 0;
  bool                  ret = FALSE;

  TRACEENTER();
  ResetSegmentMarkers();
  ActiveSegment = 0;
  if (OutSavedSize) *OutSavedSize = 0;

  if (fCut)
  {
    // Check correct version of cut-file
    Version = fgetc(fCut);
    switch (Version)
    {
      case 1:
      {
        tCutHeader1 CutHeader;
        rewind(fCut);
        ret = (fread(&CutHeader, sizeof(CutHeader), 1, fCut) == 1);
        if (ret)
        {
          if (OutSavedSize) *OutSavedSize = CutHeader.RecFileSize;
          SavedNrSegments = CutHeader.NrSegmentMarker;
//          ActiveSegment = CutHeader.ActiveSegment;
        }
        break;
      }
      case 2:
      {
        rewind(fCut);
        tCutHeader2 CutHeader;
        ret = (fread(&CutHeader, sizeof(CutHeader), 1, fCut) == 1);
        if (ret)
        {
          if (OutSavedSize) *OutSavedSize = CutHeader.RecFileSize;
          SavedNrSegments = CutHeader.NrSegmentMarker;
//          ActiveSegment = CutHeader.ActiveSegment;
        }
        break;
      }
      default:
        WriteLogMC(PROGRAM_NAME, "CutFileDecodeBin: .cut version mismatch!");
    }

    if (ret)
    {
      SavedNrSegments = min(SavedNrSegments, NRSEGMENTMARKER);
      while (fread(&SegmentMarker[NrSegmentMarker], sizeof(tSegmentMarker)-4, 1, fCut))
      {
        SegmentMarker[NrSegmentMarker].pCaption = NULL;
        NrSegmentMarker++;
      }
      if (NrSegmentMarker < SavedNrSegments)
        WriteLogMC(PROGRAM_NAME, "CutFileDecodeBin: Unexpected end of file!");
    }
  }

  // Wenn cut-File Version 1 hat, dann ermittle neue TimeStamps und vergleiche diese mit den alten (DEBUG)  [COMPATIBILITY LAYER]
  if (ret && (Version == 1) && (RecFileSize == *OutSavedSize) && !LinearTimeMode)
  {
    int i;
    dword oldTime, newTime;
    char oldTimeStr[12], newTimeStr[16];

    WriteLogMC(PROGRAM_NAME, "Import of old cut file version (compare old and new TimeStamps!)");
    for (i = 0; i < NrSegmentMarker; i++)
    {
      oldTime = SegmentMarker[i].Timems;
      newTime = NavGetBlockTimeStamp(SegmentMarker[i].Block);
      SegmentMarker[i].Timems = (newTime) ? newTime : oldTime;
      SecToTimeString(oldTime, oldTimeStr);
      MSecToTimeString(newTime, newTimeStr);
      WriteLogMCf(PROGRAM_NAME, " %s%2d.)  BlockNr=%8lu   oldTimeStamp=%s   newTimeStamp=%s", (labs(oldTime*1000-newTime) > 1000) ? "!!" : "  ", i+1, SegmentMarker[i].Block, oldTimeStr, newTimeStr);
    }
  }

  TRACEEXIT();
  return ret;
}

bool CutFileDecodeTxt(FILE *fCut, __off64_t *OutSavedSize)
{
  char                 *Buffer = NULL;
  size_t                BufSize = 0;
  __off64_t             SavedSize = 0;
  int                   Version = 3;
  int                   SavedNrSegments = 0;
  bool                  HeaderMode=FALSE, SegmentsMode=FALSE;
  char                  TimeStamp[16];
  char                 *c, Selected;
  int                   p, ReadBytes;
  bool                  ret = FALSE;

  TRACEENTER();
  ResetSegmentMarkers();
  ActiveSegment = 0;
  if (OutSavedSize) *OutSavedSize = 0;

  if (fCut)
  {
    // Check the first line
    if (getline(&Buffer, &BufSize, fCut) >= 0)
    {
      if ((strncmp(Buffer, "[MCCut3]", 8)==0) || ((strncmp(Buffer, "[MCCut4]", 8)==0) && ((Version = 4))))
      {
        HeaderMode = TRUE;
        ret = TRUE;
      }
    }

    while (ret && (getline(&Buffer, &BufSize, fCut) >= 0))
    {
      //Interpret the following characters as remarks: //
      c = strstr(Buffer, "//");
      if(c) *c = '\0';

      // Remove line breaks in the end
      p = strlen(Buffer);
      while (p && (Buffer[p-1] == '\r' || Buffer[p-1] == '\n' || Buffer[p-1] == ';'))
        Buffer[--p] = '\0';

      // Kommentare und Sektionen
      switch (Buffer[0])
      {
        case '\0':
          continue;

        case '%':
        case ';':
        case '#':
        case '/':
          continue;

        // Neue Sektion gefunden
        case '[':
        {
          HeaderMode = FALSE;
          // Header überprüfen
          if ((SavedSize <= 0) || (SavedNrSegments < 0))
          {
            ret = FALSE;
            break;
          }
          if (strcmp(Buffer, "[Segments]") == 0)
            SegmentsMode = TRUE;
          continue;
        }
      }

      // Header einlesen
      if (HeaderMode)
      {
        char            Name[50];
        __off64_t       Value;

        if (sscanf(Buffer, "%49[^= ] = %lld", Name, &Value) == 2)
        {
          if (strcmp(Name, "RecFileSize") == 0)
          {
            SavedSize = Value;
            if (OutSavedSize) *OutSavedSize = SavedSize;
          }
          else if (strcmp(Name, "NrSegmentMarker") == 0)
            SavedNrSegments = (int)Value;
//          else if (strcmp(Name, "ActiveSegment") == 0)
//            ActiveSegment = (int)Value;
        }
      }

      // Segmente einlesen
      else if (SegmentsMode)
      {
        __off64_t       Value;

        //[Segments]
        //#Nr. ; Sel ; StartBlock ; StartTime ; Percent
        if (sscanf(Buffer, "%*i ; %c ; %lld ; %15[^;\r\n] ; %f%%%n", &Selected, &Value, TimeStamp, &SegmentMarker[NrSegmentMarker].Percent, &ReadBytes) >= 3)
        {
          SegmentMarker[NrSegmentMarker].Selected = (Selected == '*');
          SegmentMarker[NrSegmentMarker].Block    = ((Version >= 4) ? CalcBlockSize(Value) : (dword)Value);
          SegmentMarker[NrSegmentMarker].Timems   = (TimeStringToMSec(TimeStamp));
          SegmentMarker[NrSegmentMarker].pCaption = NULL;
          while (Buffer[ReadBytes] && (Buffer[ReadBytes] == ' ' || Buffer[ReadBytes] == ';'))  ReadBytes++;
          if (Buffer[ReadBytes])
          {
            SegmentMarker[NrSegmentMarker].pCaption = (char*)TAP_MemAlloc(strlen(&Buffer[ReadBytes]) + 1);
            strcpy(SegmentMarker[NrSegmentMarker].pCaption, &Buffer[ReadBytes]);
          }
          NrSegmentMarker++;
        }
      }
    }
    free(Buffer);

    if (ret)
    {
      if (NrSegmentMarker != SavedNrSegments)
        WriteLogMCf(PROGRAM_NAME, "CutFileDecodeTxt: Invalid number of segments read (%d of %d)!", NrSegmentMarker, SavedNrSegments);
    }
    else
      WriteLogMC(PROGRAM_NAME, "CutFileDecodeTxt: Invalid cut file format!");
  }
  TRACEEXIT();
  return ret;
}

bool CutDecodeFromBM(void)
{
  int                   End = 0, Start, i;
  bool                  ret = FALSE;

  TRACEENTER();

  ResetSegmentMarkers();
  ActiveSegment = 0;
  if (Bookmarks[NRBOOKMARKS - 2] == TAPID)       // ID im vorletzen Bookmark-Dword (-> neues SRP-Format und CRP-Format auf SRP)
    End = NRBOOKMARKS - 2;
  else if (Bookmarks[NRBOOKMARKS - 1] == TAPID)  // ID im letzten Bookmark-Dword (-> CRP- und altes SRP-Format)
    End = NRBOOKMARKS - 1;

  if(End)
  {
    ret = TRUE;
    NrSegmentMarker = Bookmarks[End - 1];
    if (NrSegmentMarker > NRSEGMENTMARKER) NrSegmentMarker = NRSEGMENTMARKER;

    Start = End - NrSegmentMarker - 5;
    for (i = 0; i < NrSegmentMarker; i++)
    {
      SegmentMarker[i].Block = Bookmarks[Start + i];
      SegmentMarker[i].Selected = ((Bookmarks[End-5+(i/32)] & (1 << (i%32))) != 0);
      SegmentMarker[i].Timems = NavGetBlockTimeStamp(SegmentMarker[i].Block);
      SegmentMarker[i].Percent = 0;
      SegmentMarker[i].pCaption = NULL;
    }
  }

  TRACEEXIT();
  return ret;
}

bool CutFileLoad(void)
{
  FILE                 *fCut = NULL;
  char                  AbsCutName[FBLIB_DIR_SIZE];
  byte                  Version;
  __off64_t             SavedSize;
  int                   i;
  bool                  ret = FALSE;

  TRACEENTER();

  // Schaue zuerst im Cut-File nach
//  if (CutFileMode != CM_InfOnly)
  {
    GetCutNameFromRec(PlaybackName, AbsPlaybackDir, AbsCutName);
    fCut = fopen(AbsCutName, "rb");
    if(fCut)
    {
      Version = fgetc(fCut);
      if (Version == '[')
      {
        fseek(fCut, 6, SEEK_SET);
        Version = fgetc(fCut) - '0';
      }
      rewind(fCut);

      #ifdef FULLDEBUG
        WriteLogMCf(PROGRAM_NAME, "CutFileLoad: Importing cut-file version %hhu", Version);
      #endif
      switch (Version)
      {
        case 1:
        case 2:
        {
          ret = CutFileDecodeBin(fCut, &SavedSize);
          break;
        }
        case 3:
        case 4:
        {
          ret = CutFileDecodeTxt(fCut, &SavedSize);
          break;
        }
      }
      fclose(fCut);
      if (!ret)
        WriteLogMC(PROGRAM_NAME, "CutFileLoad: Failed to read cut-info from .cut!");
    }
//    else
//      if (CutFileMode == CM_CutOnly)
//        WriteLogMC(PROGRAM_NAME, "CutFileLoad: Failed to open .cut!");

    // Check, if size of rec-File has been changed
    if (ret && (RecFileSize != SavedSize))
    {
      WriteLogMC(PROGRAM_NAME, "CutFileLoad: .cut file size mismatch!");
/*      TRACEEXIT();
      return FALSE; */

      // Sonderfunktion: Import von Cut-Files mit unpassender Aufnahme-Größe
      if ((NrSegmentMarker > 2) && (TimeStamps != NULL))
      {
        char            LogString[512], curTimeStr[16];
        dword           Offsetms;
        tTimeStamp     *CurTimeStamp;

        WriteLogMC(PROGRAM_NAME, "CutFileLoad: Importing timestamps only, recalculating block numbers..."); 
        SegmentMarker[0].Block = 0;
        SegmentMarker[0].Timems = 0;  // NavGetBlockTimeStamp(0);
//        SegmentMarker[0].Selected = FALSE;
        if (SegmentMarker[NrSegmentMarker-1].Block == PlayInfo.totalBlock)
          SegmentMarker[NrSegmentMarker - 1].Block = 0;

        Offsetms = 0;
        CurTimeStamp = TimeStamps;
        for (i = 1; i <= NrSegmentMarker-2; i++)
        {
          if (Version == 1)
            SegmentMarker[i].Timems = SegmentMarker[i].Timems * 1000;

          // Wenn ein Bookmark gesetzt ist, dann verschiebe die SegmentMarker so, dass der erste auf dem Bookmark steht
          if (i == 1 && NrBookmarks > 0)
          {
            Offsetms = NavGetBlockTimeStamp(Bookmarks[0]) - SegmentMarker[1].Timems;
            MSecToTimeString(SegmentMarker[i].Timems + Offsetms, curTimeStr);
            WriteLogMCf(PROGRAM_NAME, "Bookmark found! - First segment marker will be moved to time %s. (Offset=%d ms)", curTimeStr, (int)Offsetms);
          }

          MSecToTimeString(SegmentMarker[i].Timems, curTimeStr);
          TAP_SPrint(LogString, sizeof(LogString), "%2u.)  oldTimeStamp=%s   oldBlock=%lu", i, curTimeStr, SegmentMarker[i].Block);
          if (Offsetms > 0)
          {
            SegmentMarker[i].Timems += Offsetms;
            MSecToTimeString(SegmentMarker[i].Timems, curTimeStr);
            TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), "  -  movedTimeStamp=%s", curTimeStr);
          }

          if ((SegmentMarker[i].Timems <= CurTimeStamp->Timems) || (CurTimeStamp >= TimeStamps + NrTimeStamps-1))
          {
            if (DeleteSegmentMarker(i, TRUE)) i--;
            TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), "  -->  Smaller than previous TimeStamp or end of nav reached. Deleted!");
          }
          else
          {
            while ((CurTimeStamp->Timems < SegmentMarker[i].Timems) && (CurTimeStamp < TimeStamps + NrTimeStamps-1))
              CurTimeStamp++;
            if (CurTimeStamp->Timems > SegmentMarker[i].Timems)
              CurTimeStamp--;
        
            if (CurTimeStamp->BlockNr < PlayInfo.totalBlock)
            {
              SegmentMarker[i].Block = CurTimeStamp->BlockNr;
              SegmentMarker[i].Timems = NavGetBlockTimeStamp(SegmentMarker[i].Block);
//              SegmentMarker[i].Selected = FALSE;
              MSecToTimeString(SegmentMarker[i].Timems, curTimeStr);
              TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), "  -->  newBlock=%lu   newTimeStamp=%s", SegmentMarker[i].Block, curTimeStr);
            }
            else
            {
              if (DeleteSegmentMarker(i, TRUE)) i--;
              TAP_SPrint(&LogString[strlen(LogString)], sizeof(LogString)-strlen(LogString), "  -->  TotalBlocks exceeded. Deleted!");
            }
          }
          WriteLogMC(PROGRAM_NAME, LogString);
        }
      }
      else
      {
//        WriteLogMC(PROGRAM_NAME, "CutFileLoad: Two or less timestamps imported, or NavTimeStamps=NULL!"); 
        ResetSegmentMarkers();
      }
    }
  }

  // sonst schaue in der inf
  if (!ret && (CutFileMode != CM_CutOnly))
  {
    ret = CutDecodeFromBM();
    #ifdef FULLDEBUG
    if (ret)
      WriteLogMCf(PROGRAM_NAME, "CutFileLoad: Imported segments from Bookmark-area.");
    #endif
//    if (!ret && CutFileMode == CM_InfOnly)
//      WriteLogMC(PROGRAM_NAME, "CutFileLoad: Failed to read segments from RAM!");

/*    if (!ret)
    {
      if (infData_Get2(PlaybackName, AbsPlaybackDir, INFFILETAG, &CutBlockSize, &CutBlock))
        ret = CutFileDecode(CutBlock, CutBlockSize, &SavedSize);
      else
        WriteLogMC(PROGRAM_NAME, "CutFileLoad: failed to read cut-info from .inf!");
      TAP_MemFree(CutBlock);
    }  */
  }

  if (ret && NrSegmentMarker > 0)
  {
    // erstes Segment auf 0 setzen?
    if (SegmentMarker[0].Block != 0)
    {
      #ifdef FULLDEBUG
        WriteLogMCf(PROGRAM_NAME, "CutFileLoad: Erster Segment-Marker %lu ist ungleich 0!", SegmentMarker[0].Block);
      #endif
      SegmentMarker[0].Block = 0;
      SegmentMarker[0].Timems = 0;
    }
    
    // Wenn letzter Segment-Marker ungleich TotalBlock ist -> anpassen
    if (SegmentMarker[NrSegmentMarker - 1].Block != PlayInfo.totalBlock)
    {
      #ifdef FULLDEBUG
        WriteLogMCf(PROGRAM_NAME, "CutFileLoad: Letzter Segment-Marker %lu ist ungleich TotalBlock %lu!", SegmentMarker[NrSegmentMarker - 1].Block, PlayInfo.totalBlock);
      #endif
      SegmentMarker[NrSegmentMarker - 1].Block = PlayInfo.totalBlock;
      dword newTime = NavGetBlockTimeStamp(SegmentMarker[NrSegmentMarker - 1].Block);
      SegmentMarker[NrSegmentMarker - 1].Timems = (newTime) /*? newTime : (dword)(1000 * (60*PlayInfo.duration + PlayInfo.durationSec))*/;
    }

    // Prozent-Angaben neu berechnen (müssen künftig nicht mehr in der .cut gespeichert werden)
    for (i = NrSegmentMarker-1; i >= 0; i--)
    {
      if ((i < NrSegmentMarker-1) && (SegmentMarker[i].Block >= PlayInfo.totalBlock))
      {
        WriteLogMCf(PROGRAM_NAME, "SegmentMarker %d (%lu): TotalBlocks exceeded. -> Deleting!", i, SegmentMarker[i].Block);
        DeleteSegmentMarker(i, TRUE);
      }
      else
        SegmentMarker[i].Percent = ((float)SegmentMarker[i].Block / PlayInfo.totalBlock) * 100.0;
    }

    // Markierungen und ActiveSegment prüfen, ggf. korrigieren
    SegmentMarker[NrSegmentMarker - 1].Selected = FALSE;         // the very last marker (no segment)
    if(NrSegmentMarker <= 2) SegmentMarker[0].Selected = FALSE;  // there is only one segment (from start to end)
//    if(ActiveSegment >= NrSegmentMarker - 1) ActiveSegment = NrSegmentMarker - 2;
  }

  // Wenn zu wenig Segmente -> auf Standard zurücksetzen
  if (NrSegmentMarker < 2)
  {
    if(ret) WriteLogMC(PROGRAM_NAME, "CutFileLoad: Less than two timestamps imported -> resetting!"); 
    ResetSegmentMarkers();
//    ActiveSegment = 0;
    ret = FALSE;
  }

  TRACEEXIT();
  return ret;
}
  
bool CutFileSave(void)
{
  return CutFileSave2(SegmentMarker, NrSegmentMarker, PlaybackName);
}
bool CutFileSave2(tSegmentMarker SegmentMarker[], int NrSegmentMarker, const char* RecFileName)
{
  FILE                 *fCut = NULL;
  char                  AbsCutName[FBLIB_DIR_SIZE], TimeStamp[16];
  int                   i;
  bool                  ret = TRUE;

  TRACEENTER();

  if ((CutFileMode != CM_InfOnly) && SegmentMarker && (NrSegmentMarker > 2 || SegmentMarker[0].pCaption))
  {
    // neues CutFile speichern
    if (!HDD_GetFileSizeAndInode2(RecFileName, AbsPlaybackDir, NULL, &RecFileSize))
      WriteLogMC(PROGRAM_NAME, "CutFileSave: Could not detect size of recording!"); 

    GetCutNameFromRec(RecFileName, AbsPlaybackDir, AbsCutName);
    fCut = fopen(AbsCutName, "wb");
    if(fCut)
    {
      ret = (fprintf(fCut, "[MCCut3]\r\n") > 0) && ret;
      ret = (fprintf(fCut, "RecFileSize=%llu\r\n", RecFileSize) > 0) && ret;
      ret = (fprintf(fCut, "NrSegmentMarker=%d\r\n", NrSegmentMarker) > 0) && ret;
      ret = (fprintf(fCut, "ActiveSegment=%d\r\n\r\n", ActiveSegment) > 0) && ret;  // sicher!?
      ret = (fprintf(fCut, "[Segments]\r\n") > 0) && ret;
      ret = (fprintf(fCut, "#Nr ; Sel ; StartBlock ;     StartTime ; Percent ; Caption\r\n") > 0) && ret;
      for (i = 0; i < NrSegmentMarker; i++)
      {
        MSecToTimeString(SegmentMarker[i].Timems, TimeStamp);
        ret = (fprintf(fCut, "%3d ;  %c  ; %10lu ;%14s ;  %5.1f%% ; %s\r\n", i, (SegmentMarker[i].Selected ? '*' : '-'), SegmentMarker[i].Block, TimeStamp, SegmentMarker[i].Percent, (SegmentMarker[i].pCaption ? SegmentMarker[i].pCaption : "")) > 0) && ret;
      }
//      ret = (fflush(fCut) == 0) && ret;
      ret = (fclose(fCut) == 0) && ret;
      HDD_SetFileDateTime(&AbsCutName[1], "", 0);
    }
    else
    {
      WriteLogMC(PROGRAM_NAME, "CutFileSave: failed to open .cut!");
      ret = FALSE;
    }
  }
  else  // nicht gewünschte Cut-Files löschen
    CutFileDelete();
  TRACEEXIT();
  return ret;
}

bool CutEncodeToBM(tSegmentMarker SegmentMarker[], int NrSegmentMarker, dword Bookmarks[], int NrBookmarks)
{
  int                   End, Start, i;
  bool                  ret = TRUE;

  TRACEENTER();
  if (!Bookmarks)
  {
    TRACEEXIT();
    return FALSE;
  }
  memset(&Bookmarks[NrBookmarks], 0, (NRBOOKMARKS - min(NrBookmarks, NRBOOKMARKS)) * sizeof(dword));

  if ((CutFileMode != CM_CutOnly) && SegmentMarker && (NrSegmentMarker > 2))
  {
    End   = (GetSystemType() == ST_TMSC) ? NRBOOKMARKS-1 : NRBOOKMARKS-2;  // neu: auf dem SRP das vorletzte Dword nehmen -> CRP-kompatibel
    Start = End - NrSegmentMarker - 5;
    if (Start >= NrBookmarks)
    {
      Bookmarks[End]   = TAPID;  // Magic
      Bookmarks[End-1] = NrSegmentMarker;
      for (i = 0; i < NrSegmentMarker; i++)
      {
        Bookmarks[Start+i] = SegmentMarker[i].Block;
        Bookmarks[End-5+(i/32)] = (Bookmarks[End-5+(i/32)] & ~(1 << (i%32))) | (SegmentMarker[i].Selected ? 1 << (i%32) : 0);
      }
    }
    else
    {
      WriteLogMCf(PROGRAM_NAME, "CutEncodeToBM: Error! Not enough space to store segment markers. (NrSegmentMarker=%d, NrBookmarks=%d)", NrSegmentMarker, NrBookmarks);
      ret = FALSE;
    }
  }

  TRACEEXIT();
  return ret;
}

bool CutSaveToBM(bool ReadBMBefore)
{
  TRACEENTER();
  if (CutFileMode != CM_CutOnly && !MediaFileMode)
  {
    if (ReadBMBefore && !ReadBookmarks(Bookmarks, &NrBookmarks))
      WriteLogMC(PROGRAM_NAME, "CutSaveToBM: ReadBookmarks() failed!");

    if (CutEncodeToBM(SegmentMarker, NrSegmentMarker, Bookmarks, NrBookmarks))
    {
      if (SaveBookmarks(Bookmarks, NrBookmarks, TRUE))
      {
        TRACEEXIT();
        return TRUE;
      }
      else
        WriteLogMC(PROGRAM_NAME, "CutSaveToBM: SaveBookmarks() failed!");
    }
  }
  TRACEEXIT();
  return FALSE;
}

bool CutSaveToInf(tSegmentMarker SegmentMarker[], int NrSegmentMarker, const char* RecFileName)
{
  char                  AbsInfName[FBLIB_DIR_SIZE];
  FILE                 *fInf = NULL;
  byte                 *Buffer = NULL;
  TYPE_Bookmark_Info   *BookmarkInfo = NULL;
  size_t               InfSize, BytesRead;
  bool                  ret = FALSE;
  
  TRACEENTER();
  if (CutFileMode != CM_CutOnly)
  {
    //Calculate inf header size
    InfSize = ((GetSystemType()==ST_TMSC) ? sizeof(TYPE_RecHeader_TMSC) : sizeof(TYPE_RecHeader_TMSS));
/*    switch (GetSystemType())
    {
      case ST_TMSS:  InfSize = sizeof(TYPE_RecHeader_TMSS); break;
      case ST_TMSC:  InfSize = sizeof(TYPE_RecHeader_TMSC); break;
      case ST_TMST:  InfSize = sizeof(TYPE_RecHeader_TMST); break;
      default:       InfSize = 0; break;
    } */

    //Allocate and clear the buffer
    Buffer = (byte*) TAP_MemAlloc(InfSize);
    if(Buffer) 
      memset(Buffer, 0, InfSize);
    else
    {
      WriteLogMC(PROGRAM_NAME, "CutSaveToInf: Failed to allocate the memory!");
      TRACEEXIT();
      return FALSE;
    }

    //Read inf
    TAP_SPrint(AbsInfName, sizeof(AbsInfName), "%s/%s.inf", AbsPlaybackDir, RecFileName);
    fInf = fopen(AbsInfName, "r+b");
    if(!fInf)
    {
      WriteLogMC(PROGRAM_NAME, "CutSaveToInf: Failed to open the inf file!");
      TAP_MemFree(Buffer);
      TRACEEXIT();
      return FALSE;
    }

    //Read and Decode inf
    BytesRead = fread(Buffer, 1, InfSize, fInf);
    if (BytesRead > 0)
    {
      switch (GetSystemType())
      {
        case ST_TMSS:  BookmarkInfo = &(((TYPE_RecHeader_TMSS*)Buffer)->BookmarkInfo); break;
        case ST_TMSC:  BookmarkInfo = &(((TYPE_RecHeader_TMSC*)Buffer)->BookmarkInfo); break;
        case ST_TMST:  BookmarkInfo = &(((TYPE_RecHeader_TMST*)Buffer)->BookmarkInfo); break;
        default:       break;
      }

      //Calculate the new bookmarks
      if (BookmarkInfo && CutEncodeToBM(SegmentMarker, NrSegmentMarker, BookmarkInfo->Bookmarks, BookmarkInfo->NrBookmarks))
      {
        //Write the new inf
        fseek(fInf, 0, SEEK_SET);
        if (fwrite(Buffer, 1, InfSize, fInf) == InfSize)
          ret = TRUE;
      }
    }
//    ret = (fflush(fInf) == 0) && ret;
    ret = (fclose(fInf) == 0) && ret;
    TAP_MemFree(Buffer);
    HDD_SetFileDateTime(&AbsInfName[1], "", 0);

    if (!ret)
      WriteLogMC(PROGRAM_NAME, "CutSaveToInf: Failed to write the new inf file!");
  }
  TRACEEXIT();
  return ret;
}

void CutFileDelete(void)
{
  char                  CutName[FBLIB_DIR_SIZE];

  TRACEENTER();

//  HDD_ChangeDir(PlaybackDir);
  GetCutNameFromRec(PlaybackName, "", CutName);
  HDD_Delete2(&CutName[1], AbsPlaybackDir, FALSE);

  TRACEEXIT();
}

void CutDumpList(void)
{
  char                  TimeString[16];
  int                   i;

  TRACEENTER();

  WriteLogMC(PROGRAM_NAME, "Seg      Block Time        Pct Sel Act");
  for(i = 0; i < NrSegmentMarker; i++)
  {
    MSecToTimeString(SegmentMarker[i].Timems, TimeString);
    WriteLogMCf(PROGRAM_NAME, "%02d: %010d %s %03d %3s %3s %s", i, (int)SegmentMarker[i].Block, TimeString, (int)SegmentMarker[i].Percent, SegmentMarker[i].Selected ? "yes" : "no", (i == ActiveSegment ? "*" : ""), SegmentMarker[i].pCaption);
  }

  TRACEEXIT();
}


// ----------------------------------------------------------------------------
//                              OSD-Funktionen
// ----------------------------------------------------------------------------
void CreateOSD(void)
{
  TRACEENTER();

  if(!rgnInfoBar && !rgnInfoBarMini && OSDMode != MD_NoOSD)  // Workaround: Wenn Play-Leiste eingeblendet ist (SysSubState == SUBSTATE_PvrPlayingSearch) wird diese sonst nicht ausgeblendet
  {
//    TAP_ExitNormal();
    TAP_EnterNormalNoInfo();
    TAP_ExitNormal();
  }

  if ((OSDMode != MD_MiniOSD) && (OSDMode != MD_NoOSD))
  {
    if(rgnPlayState)
    {
      TAP_Osd_Delete(rgnPlayState);
      rgnPlayState = 0;
    }
    if(rgnTextState)
    {
      TAP_Osd_Delete(rgnTextState);
      rgnTextState = 0;
    }
  }

  if(rgnSegmentText)
  {
    TAP_Osd_Delete(rgnSegmentText);
    rgnSegmentText = 0;
  }

  if (OSDMode != MD_MiniOSD)
  {
    if(rgnInfoBarMini)
    {
      TAP_Osd_Delete(rgnInfoBarMini);
      rgnInfoBarMini = 0;
    }
  }

  if ((OSDMode != MD_FullOSD) && (OSDMode != MD_NoSegmentList))
  {
    if(rgnInfoBar)
    {
      TAP_Osd_Delete(rgnInfoBar);
      rgnInfoBar = 0;
    }
  }

  if (OSDMode != MD_FullOSD)
  {
    if(rgnSegmentList)
    {
      TAP_Osd_Delete(rgnSegmentList);
      rgnSegmentList = 0;
    }
  }

  if (rgnStripProgBar)
  {
    TAP_Osd_Delete(rgnStripProgBar);
    rgnStripProgBar = 0;
  }
/*  if ((OSDMode == MD_NoOSD) && RecStrip_active)
    OSDRecStripProgressBar();  */

  if (OSDMode == MD_FullOSD)
    if(!rgnSegmentList)
      rgnSegmentList = TAP_Osd_Create(SegmentList_X, SegmentList_Y, _SegmentList_Background_Gd.width, _SegmentList_Background_Gd.height, 0, 0);

  if ((OSDMode == MD_FullOSD) || (OSDMode == MD_NoSegmentList))
    if(!rgnInfoBar)
      rgnInfoBar = TAP_Osd_Create(0, ScreenHeight - 160, ScreenWidth, 160, 0, 0);

  if (OSDMode == MD_MiniOSD)
    if(!rgnInfoBarMini)
      rgnInfoBarMini = TAP_Osd_Create(Overscan_X, ScreenHeight - Overscan_Y - 38 - 32, ScreenWidth - 2*Overscan_X, 38, 0, 0);

  if (ShowSegmentText)
    OSDSegmentTextDraw(TRUE);

  TRACEEXIT();
}

void OSDRedrawEverything(void)
{
  TRACEENTER();

  CreateOSD();
  OSDInfoDrawBackground();
  OSDInfoDrawRecName();
  SetCurrentSegment();
  OSDSegmentListDrawList(FALSE);
  OSDInfoDrawProgressbar(TRUE, FALSE);
  OSDInfoDrawPlayIcons(TRUE, FALSE);
  OSDInfoDrawMinuteJump(FALSE);
  OSDInfoDrawBookmarkMode(FALSE);
  OSDInfoDrawCurrentPlayTime(TRUE);
  OSDInfoDrawClock(TRUE);
  TAP_Osd_Sync();

  TRACEEXIT();
}

/* bool isLargeSegment(int Seg)
{
  return(
    ((Seg < NrSegmentMarker-2 || DisableSpecialEnd) && (SegmentMarker[Seg+1].Block - SegmentMarker[Seg].Block + 22 > 475949))
  || ((Seg == NrSegmentMarker-2) && ForceSpecialEnd && (SegmentMarker[Seg].Block + 11 > 475949))
  || ((Seg == NrSegmentMarker-2) && (SegmentMarker[Seg+1].Block - SegmentMarker[Seg].Block + 22 > 475949) && (SegmentMarker[Seg].Block + 11 > 475949))
  );
} */
bool isLargeSegment(dword SegStartBlock, dword SegEndBlock, bool isLastSegment, bool KeepCut)
{
  return(
    ((!isLastSegment || (DisableSpecialEnd && KeepCut)) && (SegEndBlock - SegStartBlock + 22 > 475949))
  || ( isLastSegment && ForceSpecialEnd && KeepCut      && (SegStartBlock + 11 > 475949))
  || ( isLastSegment && KeepCut                         && (SegEndBlock - SegStartBlock + 22 > 475949) && (SegStartBlock + 11 > 475949))
  );
}

// Segment-Liste
// -------------
void OSDSegmentListDrawList(bool DoSync)
{
  const int             RegionWidth        = _SegmentList_Background_Gd.width;
  const int             TextFieldStart_X   =    5,   EndTextField_X  = 152;
  const int             TextFieldStart_Y   =   29,   TextFieldHeight =  26,   TextFieldDist = 2;
  const int             Scrollbar_X        =  154,   Scrollbar_Y     =  40,  /*ScrollbarWidth = 10,*/    ScrollbarHeight = 256;
  const int             BelowTextArea_Y    =  307;
  const int             NrWidth     =  FM_GetStringWidth("99.", &Calibri_12_FontData);
  const int             DashWidth   =  FM_GetStringWidth(" - ", &Calibri_12_FontData);
  const int             TimeWidth   =  FM_GetStringWidth("99:99:99", &Calibri_12_FontData);

  int                   CurrentSegment;
  int                   ScrollButtonHeight, ScrollButtonPos;
  char                  StartTime[12], EndTime[12], OutStr[5];
  char                  PageNrStr[8], *PageStr;
  int                   p, NrPages;
  int                   Start;
  dword                 PosX, PosY, UseColor;
  int                   i;

  TRACEENTER();

  if(rgnSegmentList)
  {
    if (JumpRequestedSegment != 0xFFFF)
      CurrentSegment = JumpRequestedSegment;
    else
      CurrentSegment = ActiveSegment;
    if(CurrentSegment >= NrSegmentMarker - 1)
      CurrentSegment = NrSegmentMarker - 2;

    // Hintergrund, Überschrift, Buttons
    TAP_Osd_PutGd(rgnSegmentList, 0, 0, &_SegmentList_Background_Gd, FALSE);
    FM_PutString(rgnSegmentList, 2, 2, RegionWidth-3, LangGetString(LS_SegmentList), COLOR_White, ColorLightBackground, &Calibri_14_FontData, TRUE, ALIGN_CENTER);
    TAP_Osd_PutGd(rgnSegmentList, EndTextField_X - 2*19 - 3, BelowTextArea_Y + 6, &_Button_Up_small_Gd, TRUE);
    TAP_Osd_PutGd(rgnSegmentList, EndTextField_X - 19,       BelowTextArea_Y + 6, &_Button_Down_small_Gd, TRUE);

    // Seitenzahl
    NrPages = ((NrSegmentMarker - 2) / 10) + 1;
    p       = (CurrentSegment / 10) + 1;
    PageStr = LangGetString(LS_PageStr);
    TAP_SPrint(PageNrStr, sizeof(PageNrStr), "%d/%d", p, NrPages);

    PosX = TextFieldStart_X + 11;
    PosY = BelowTextArea_Y + 3;
    FM_PutString(rgnSegmentList, PosX, PosY, EndTextField_X - 2*19 - 3, PageStr,   COLOR_White, ColorLightBackground, &Calibri_10_FontData, FALSE, ALIGN_LEFT);
    PosX += FM_GetStringWidth(PageStr, &Calibri_10_FontData) + 5;

    FM_PutString(rgnSegmentList, PosX, PosY, EndTextField_X - 2*19 - 3, PageNrStr, COLOR_White, ColorLightBackground, &Calibri_10_FontData, FALSE, ALIGN_LEFT);

    // Segmente
    if(NrSegmentMarker > 2)
    {
//      if(CurrentSegment >= 10)                          TAP_Osd_PutGd(rgnSegmentList, 62, 23, &_Button_Up2_Gd, TRUE);
//      if((CurrentSegment < 10) && (NrSegmentMarker>11)) TAP_Osd_PutGd(rgnSegmentList, 88, 23, &_Button_Down2_Gd, TRUE);

      CountSelectedSegments();
      Start = (CurrentSegment / 10) * 10;
      for(i = 0; i < min(10, (NrSegmentMarker - Start) - 1); i++)
      {
        if((Start + i) == CurrentSegment)
        {
//          if((SegmentMarker[p+i + 1].Timems - SegmentMarker[p+i].Timems) < 60001)
//            TAP_Osd_PutGd(rgnSegmentList, TextFieldStart_X, TextFieldStart_Y + i*(TextFieldHeight+TextFieldDist), &&_Selection_Red_Gd, FALSE);
//          else
          TAP_Osd_PutGd(rgnSegmentList, TextFieldStart_X, TextFieldStart_Y + i*(TextFieldHeight+TextFieldDist), &_Selection_Blue_Gd, FALSE);
        }

        SecToTimeString((SegmentMarker[Start + i].Timems) / 1000, StartTime);
        SecToTimeString((SegmentMarker[Start + i + 1].Timems) / 1000, EndTime);

        PosX = TextFieldStart_X;
        PosY = TextFieldStart_Y + i*(TextFieldHeight+TextFieldDist) + 3;
        UseColor = (SegmentMarker[Start + i].Selected) ? COLOR_Yellow : COLOR_White;
        if (SegmentMarker[Start+i].Selected || ((NrSelectedSegments == 0) && (Start+i == CurrentSegment)))
          if(isLargeSegment(SegmentMarker[Start+i].Block, SegmentMarker[Start+i+1].Block, (Start+i == NrSegmentMarker-2), TRUE))
            UseColor = RGB(250, 139, 18);
        if (Start + i + 1 < 100)
          TAP_SPrint(OutStr, sizeof(OutStr), "%d.", Start + i + 1);
        else
          TAP_SPrint(OutStr, sizeof(OutStr), "%02d.", (Start + i + 1) % 100);
        FM_PutString(rgnSegmentList, PosX, PosY, PosX + NrWidth,    OutStr,                                                               UseColor, COLOR_None, &Calibri_12_FontData, FALSE, ALIGN_RIGHT);
        PosX += NrWidth;
        FM_PutString(rgnSegmentList, PosX, PosY, PosX + TimeWidth,  (Start+i == 0) ? LangGetString(LS_BeginStr) : StartTime,              UseColor, COLOR_None, &Calibri_12_FontData, FALSE, ALIGN_RIGHT);
        PosX += TimeWidth;
        FM_PutString(rgnSegmentList, PosX, PosY, PosX + DashWidth,  "-",                                                                  UseColor, COLOR_None, &Calibri_12_FontData, FALSE, ALIGN_CENTER);
        PosX += DashWidth;
        FM_PutString(rgnSegmentList, PosX, PosY, EndTextField_X+10, (Start+i == NrSegmentMarker-2) ? LangGetString(LS_EndStr) : EndTime,  UseColor, COLOR_None, &Calibri_12_FontData, FALSE, ALIGN_LEFT);

        if (SegmentMarker[Start+i].pCaption && *SegmentMarker[Start+i].pCaption)
          TAP_Osd_PutGd(rgnSegmentList, EndTextField_X - _Icon_SegmentText_Gd.width + (NrPages>1 ? 0 : 5), PosY + 5, &_Icon_SegmentText_Gd, TRUE);
      }
    }

    // Scrollbalken
    if (NrPages > 1)
    {
      TAP_Osd_PutGd(rgnSegmentList, EndTextField_X, TextFieldStart_Y, &_SegmentList_ScrollBar_Gd, FALSE);
      ScrollButtonHeight = (ScrollbarHeight / NrPages);
      ScrollButtonPos    = Scrollbar_Y + (ScrollbarHeight * (p - 1) / NrPages);
//      TAP_Osd_Draw3dBoxFill(rgnSegmentList, Scrollbar_X+2, ScrollButtonPos, ScrollbarWidth-4, ScrollButtonHeight, COLOR_Yellow, COLOR_Red, COLOR_DarkGray);
      for (i = 0; i < ScrollButtonHeight; i++)
        TAP_Osd_PutGd(rgnSegmentList, Scrollbar_X, ScrollButtonPos + i, &_SegmentList_ScrollButton_Gd, FALSE);
    }

    if(DoSync) TAP_Osd_Sync();
  }

  OSDSegmentTextDraw(FALSE);
  TRACEEXIT();
}

// SetCurrentSegment
// -----------------
void SetCurrentSegment(void)
{
  bool                  DoDraw = FALSE;
  int                   VisibleSegment = -1;
  static dword          VisibleBlock = 0;

  TRACEENTER();

  if (!PLAYINFOVALID())
  {
    TRACEEXIT();
    return;
  }

  if (((JumpRequestedBlock != (dword) -1)) && !JumpRequestedTime)
  {
    VisibleSegment = FindSegmentWithBlock(JumpRequestedBlock);
    if (ActiveSegment != VisibleSegment)
      DoDraw = TRUE;
    ActiveSegment = VisibleSegment;
  }

  if (!JumpPerformedTime || (labs(TAP_GetTick() - JumpPerformedTime) >= 200))
  {
    if (JumpPerformedTime) DoDraw = TRUE;
    JumpPerformedTime = 0;

    if (!JumpRequestedTime)
    {
      JumpRequestedBlock = (dword) -1;
      JumpRequestedSegment = 0xFFFF;
    }

    if (VisibleSegment < 0)
    {
      if(NrSegmentMarker > 2)
      {
        // Stabilisierung der springenden Segment-Anzeige beim Segment-Wechsel während Wiedergabe (neu)
        if (DoDraw || (TrickMode!=TRICKMODE_Normal && TrickMode!=TRICKMODE_Forward && TrickMode!=TRICKMODE_Slow) || (PlayInfo.currentBlock > VisibleBlock) || (PlayInfo.currentBlock + 100 < VisibleBlock))
          VisibleBlock = PlayInfo.currentBlock;
        VisibleSegment = FindSegmentWithBlock(VisibleBlock);
        if(ActiveSegment != VisibleSegment)
          DoDraw = TRUE;
        ActiveSegment = VisibleSegment;
      }
      else
        ActiveSegment = 0;
    }
  }

  if(DoDraw)
  {
    OSDSegmentListDrawList(FALSE);
    OSDInfoDrawProgressbar(TRUE, FALSE);
  }
  TRACEEXIT();
}

// Fortschrittsbalken
// ------------------
void OSDInfoDrawProgressbar(bool Force, bool DoSync)
{
  const dword           ColorProgressBar      = RGB(250, 139,  18);
  const dword           ColorWarnedSegment    = RGB(255, 100,  60);
  const dword           ColorActiveSegment    = RGB( 73, 206, 239);
  const dword           ColorActiveSegWarning = RGB(180, 175, 235);
  const dword           ColorSegmentSelection = COLOR_Blue;
  const dword           ColorSelectionWarning = COLOR_DarkMagenta;
  const dword           ColorCurrentPos       = RGB(157,   8,  13);
  const dword           ColorCurrentPosMark   = RGB(255, 111, 114);
  dword                 UseSelectionColor;

  word                  OSDRegion = 0;
  int                   FrameWidth, FrameHeight, FrameLeft, FrameTop;
  int                   ProgBarWidth, ProgBarHeight, ProgBarLeft, ProgBarTop;

  static dword          LastDraw = 0;
  static dword          LastPos = 999;
  static dword          VisibleBlock = 0;
  dword                 pos;
  dword                 curPos, curWidth, nextPos;
  int                   NearestMarker;
  int                   i, j;

  TRACEENTER();

  if(Force || (labs(TAP_GetTick() - LastDraw) > ((TrickMode == TRICKMODE_Rewind || TrickMode == TRICKMODE_Forward) ? 10 : 20)))
  {
    if((int)PlayInfo.totalBlock <= 0)  // für die Anzeige der Progressbar reicht es, wenn totalBlock gesetzt ist, currentBlock wird später geprüft!
    {
      TRACEEXIT();
      return;
    }

    if(rgnInfoBar)
    {
      OSDRegion     = rgnInfoBar;
      FrameWidth    = ScreenWidth - 2*Overscan_X - 2;                      FrameHeight   = 32;
      FrameLeft     = Overscan_X + 1;                                      FrameTop      = InfoBarLine2_Y + 1;
      ProgBarWidth  = FrameWidth - 15;                                     ProgBarHeight = 10;
      ProgBarLeft   = FrameLeft + 10;                                      ProgBarTop    = FrameTop + (FrameHeight-ProgBarHeight)/2;
    }
    else if (rgnInfoBarMini)
    {
      OSDRegion     = rgnInfoBarMini;
      int TimeWidth = FM_GetStringWidth("9:99:99", &Calibri_12_FontData) + 1;
      FrameWidth    = GetOSDRegionWidth(rgnInfoBarMini) - 4 - TimeWidth;   FrameHeight   = GetOSDRegionHeight(rgnInfoBarMini) - 6;
      FrameLeft     = 2;                                                   FrameTop      =  3;
      ProgBarWidth  = FrameWidth - 15;                                     ProgBarHeight = 10;
      ProgBarLeft   = FrameLeft + 10;                                      ProgBarTop    = FrameTop + (FrameHeight-ProgBarHeight)/2;
    }
    else
    {
      TRACEEXIT();
      return;
    }

//    if (!JumpRequestedTime && !JumpPerformedTime)
//      JumpRequestedBlock = (dword) -1;

    // Stabilisierung der vor- und zurückspringenden Abspielposition bei kurzen Wiedergaben (nicht unbedingt nötig)
    if ((JumpRequestedBlock != (dword) -1))
      VisibleBlock = JumpRequestedBlock;
    else
      if (Force || (TrickMode!=TRICKMODE_Normal && TrickMode!=TRICKMODE_Forward && TrickMode!=TRICKMODE_Slow) || (PlayInfo.currentBlock > VisibleBlock) || (PlayInfo.currentBlock + 100 < VisibleBlock))
        VisibleBlock = PlayInfo.currentBlock;

    pos = (dword)((float)VisibleBlock * ProgBarWidth / PlayInfo.totalBlock);
    if(Force || (pos != LastPos))
    {
      // Draw the background
      TAP_Osd_FillBox(OSDRegion, FrameLeft, FrameTop, ProgBarWidth + 15, FrameHeight, ColorLightBackground);
      TAP_Osd_FillBox(OSDRegion, ProgBarLeft, ProgBarTop, ProgBarWidth, ProgBarHeight, ColorProgressBar);
      for (i = 0; i <= 10; i++) {
        for (j = 1; j <= 4; j++) {
          TAP_Osd_PutPixel(OSDRegion, ProgBarLeft + ((i * (ProgBarWidth-1))/10), FrameTop + FrameHeight - j, COLOR_White);
        }
      }
      FM_PutString (OSDRegion, FrameLeft + 1, FrameTop - 2,                                                                               ProgBarLeft, LangGetString(LS_S), ((BookmarkMode) ? COLOR_Gray : RGB(255,180,30)), COLOR_None, &Calibri_10_FontData, FALSE, ALIGN_LEFT);
      FM_PutString (OSDRegion, FrameLeft,     FrameTop + FrameHeight + 1 - FM_GetStringHeight(LangGetString(LS_B), &Calibri_10_FontData), ProgBarLeft, LangGetString(LS_B), ((BookmarkMode) ? RGB(60,255,60) : COLOR_Gray),  COLOR_None, &Calibri_10_FontData, FALSE, ALIGN_LEFT);

      NearestMarker = (BookmarkMode) ? FindNearestBookmark(VisibleBlock) : FindNearestSegmentMarker(VisibleBlock);

      // For each Segment
      nextPos = (dword)((float)SegmentMarker[0].Block * ProgBarWidth / PlayInfo.totalBlock);  // Idealfall: 0
      for(i = 0; i < NrSegmentMarker - 1; i++)
      {
//        if ((SegmentMarker[i].Block > SegmentMarker[i+1].Block) || (SegmentMarker[i+1].Block > PlayInfo.totalBlock))
//          continue;

        // Calculate marker position
        curPos = min(nextPos, (dword)ProgBarWidth + 1);

        // Draw the segment marker
        if((i >= 1) /* && (SegmentMarker[i].Block <= PlayInfo.totalBlock) */)
        {
          if (!BookmarkMode)
          {
            if (i == NearestMarker)
              TAP_Osd_PutGd(OSDRegion, ProgBarLeft + curPos - _SegmentMarker_current_Gd.width/2, ProgBarTop - _SegmentMarker_current_Gd.height, &_SegmentMarker_current_Gd, TRUE);
            else
              TAP_Osd_PutGd(OSDRegion, ProgBarLeft + curPos - _SegmentMarker_Gd.width/2,         ProgBarTop - _SegmentMarker_Gd.height, &_SegmentMarker_Gd, TRUE);
          }
          else
            TAP_Osd_PutGd(OSDRegion,   ProgBarLeft + curPos - _SegmentMarker_gray_Gd.width/2,    ProgBarTop - _SegmentMarker_gray_Gd.height, &_SegmentMarker_gray_Gd, TRUE);
        }

        // Calculate segment position and width
        curPos = min(nextPos, (dword)ProgBarWidth);
        nextPos = (dword)((float)SegmentMarker[i+1].Block * ProgBarWidth / PlayInfo.totalBlock);
        curWidth = min(nextPos - curPos, (dword)ProgBarWidth + 1 - curPos);

        // Fill the active segment
        if ((NrSegmentMarker > 2) && (i == ActiveSegment))
          TAP_Osd_FillBox(OSDRegion, ProgBarLeft + curPos, ProgBarTop, curWidth, ProgBarHeight, ColorActiveSegment);

        // Fill too large segements with warning color
        UseSelectionColor = ColorSegmentSelection;
        if (NrSegmentMarker > 2)
        {
          if (isLargeSegment(SegmentMarker[i].Block, SegmentMarker[i+1].Block, (i == NrSegmentMarker-2), TRUE))
          {
            TAP_Osd_FillBox(OSDRegion, ProgBarLeft + curPos, ProgBarTop, curWidth, ProgBarHeight, ((i == ActiveSegment) ? ColorActiveSegWarning : ColorWarnedSegment));
            UseSelectionColor = ColorSelectionWarning;
          }
        }

        // Draw the selection
        if(SegmentMarker[i].Selected)
          TAP_Osd_DrawRectangle(OSDRegion, ProgBarLeft + curPos, ProgBarTop, curWidth, ProgBarHeight, 2, UseSelectionColor);

        // Draw requested jump
        if ((JumpRequestedSegment != 0xFFFF) && (i == JumpRequestedSegment))
        {
          if (SegmentMarker[i].Selected)
            TAP_Osd_DrawRectangle(OSDRegion, ProgBarLeft + curPos, ProgBarTop, curWidth, ProgBarHeight, 1, ColorActiveSegment);
          else
          {
            TAP_Osd_DrawRectangle(OSDRegion, ProgBarLeft + curPos, ProgBarTop, curWidth, ProgBarHeight, 2, ((i!=ActiveSegment) ? ColorActiveSegment : RGB(50, 150, 190)));
//            if ((JumpRequestedSegment == ActiveSegment) && (curWidth > 2))
//              TAP_Osd_DrawRectangle(OSDRegion, ProgBarLeft + curPos + 1, ProgBarTop + 1, curWidth - 2, ProgBarHeight - 2, 1, COLOR_Gray);
          }
        }
      }

      // Draw the Bookmarks
      for(i = 0; i < NrBookmarks; i++)
      {
//        if(Bookmarks[i] <= PlayInfo.totalBlock)
//        {
          curPos     = min((dword)((float)Bookmarks[i] * ProgBarWidth / PlayInfo.totalBlock), (dword)ProgBarWidth + 1);
          if (BookmarkMode)
          {
            if (i == NearestMarker)
              TAP_Osd_PutGd(OSDRegion, ProgBarLeft + curPos - _BookmarkMarker_current_Gd.width/2, ProgBarTop + ProgBarHeight, &_BookmarkMarker_current_Gd, TRUE);
            else
              TAP_Osd_PutGd(OSDRegion, ProgBarLeft + curPos - _BookmarkMarker_Gd.width/2,         ProgBarTop + ProgBarHeight, &_BookmarkMarker_Gd, TRUE);
          }
          else
            TAP_Osd_PutGd(OSDRegion,   ProgBarLeft + curPos - _BookmarkMarker_gray_Gd.width/2,    ProgBarTop + ProgBarHeight, &_BookmarkMarker_gray_Gd, TRUE);
//        }
      }

      // Draw the current position
      if((int)PlayInfo.currentBlock >= 0)
      {
        curPos = ProgBarLeft + min(pos, (dword)ProgBarWidth + 1);
        for(j = 0; j <= ProgBarHeight; j++)
          TAP_Osd_PutPixel(OSDRegion, curPos,     ProgBarTop + j,                 ColorCurrentPos);
//        TAP_Osd_PutGd(OSDRegion, curPos - _PositionMarker_Gd.width/2, ProgBarTop + ProgBarHeight + 1, &_PositionMarker_Gd, TRUE);
        TAP_Osd_PutPixel  (OSDRegion, curPos,     ProgBarTop + ProgBarHeight + 1, ColorCurrentPosMark);
        for(j = -1; j <= 1; j++)
          TAP_Osd_PutPixel(OSDRegion, curPos + j, ProgBarTop + ProgBarHeight + 2, ColorCurrentPosMark);
        for(j = -2; j <= 2; j++)
          TAP_Osd_PutPixel(OSDRegion, curPos + j, ProgBarTop + ProgBarHeight + 3, ColorCurrentPosMark);
      }
      LastPos = pos;
      if(DoSync) TAP_Osd_Sync();
    }
    LastDraw = TAP_GetTick();
  }
  TRACEEXIT();
}


// Info-Bereich
// ------------
void OSDInfoDrawBackground(void)
{
  TYPE_GrData*          ColorButtons[]       = {&_Button_red_Gd,          &_Button_Green_Gd,     &_Button_Yellow_Gd,     &_Button_Blue_Gd};
  char*                 ColorButtonStrings[] = {LangGetString(LS_Delete), LangGetString(LS_Add), LangGetString(LS_Move), LangGetString(LS_Select)};
  int                   ColorButtonLengths[4];
  TYPE_GrData*          BelowButtons[]       = {&_Button_recall_Gd,     &_Button_vf_Gd,               &_Button_ProgPlusMinus_Gd,  &_Button_menu_Gd,            &_Button_Exit_Gd,       &_Button_white_Gd};
  char*                 BelowButtonStrings[] = {LangGetString(LS_Undo), LangGetString(LS_ChangeMode), LangGetString(LS_FastNav),  LangGetString(LS_PauseMenu), LangGetString(LS_Exit), LangGetString(LS_OSD)};
  int                   BelowButtonLengths[6];

  int                   PosX, PosY, ButtonDist;
  int                   i;

  TRACEENTER();

  if(rgnInfoBar)
  {
    // Draw the background
    TAP_Osd_FillBox(rgnInfoBar, 0,  0, ScreenWidth, 45, ColorInfoBarTitle);
    TAP_Osd_FillBox(rgnInfoBar, 0, 45, ScreenWidth, GetOSDRegionHeight(rgnInfoBar) - 45, ColorDarkBackground);

    // Draw the title area
    TAP_Osd_PutGd(rgnInfoBar, Overscan_X + 12, 11, &_Icon_Playing_Gd, TRUE);
    TAP_Osd_DrawRectangle(rgnInfoBar, ScreenWidth - Overscan_X - InfoBarRightAreaWidth - 2, 12, 1, 22, 1, RGB(92,93,93));

    // Draw the color buttons area
    const int FrameWidth = ScreenWidth - 2*Overscan_X - InfoBarRightAreaWidth - InfoBarModeAreaWidth - 4;

    TAP_Osd_FillBox(rgnInfoBar, Overscan_X, InfoBarLine1_Y, FrameWidth, InfoBarLine1Height, ColorInfoBarDarkSub);

    ButtonDist = 6;
    for (i = 0; i < 4; i++)
    {
      ColorButtonLengths[i] = FM_GetStringWidth(ColorButtonStrings[i], &Calibri_12_FontData);
      ButtonDist += ColorButtons[i]->width + 3 + ColorButtonLengths[i];
    }
    ButtonDist = (FrameWidth - ButtonDist - 4) / 4;
    ButtonDist = min(ButtonDist, 20);

    PosY = InfoBarLine1_Y + 5;
    PosX = Overscan_X + 6;
    for (i = 0; i < 4; i++)
    {
      TAP_Osd_PutGd(rgnInfoBar, PosX, PosY + 1, ColorButtons[i], TRUE);
      PosX += _Button_red_Gd.width + 3;
      FM_PutString(rgnInfoBar, PosX, PosY, PosX + max(ColorButtonLengths[i] + ButtonDist, 0), ColorButtonStrings[i], COLOR_White, ColorInfoBarDarkSub, &Calibri_12_FontData, TRUE, ALIGN_LEFT);
      PosX += max(ColorButtonLengths[i] + ButtonDist, 0);
    }

    // Draw Sub-backgrounds
//    TAP_Osd_FillBox(rgnInfoBar, Overscan_X + FrameWidth + 2, InfoBarLine1_Y, InfoBarModeAreaWidth, InfoBarLine1Height, ColorInfoBarLightSub);
//    TAP_Osd_FillBox(rgnInfoBar, ScreenWidth - Overscan_X - InfoBarRightAreaWidth, InfoBarLine1_Y, InfoBarRightAreaWidth, InfoBarLine1Height, ColorInfoBarDarkSub);

    // Draw border of ProgressBar
    TAP_Osd_DrawRectangle(rgnInfoBar, Overscan_X, InfoBarLine2_Y, ScreenWidth - 2 * Overscan_X, 34, 1, COLOR_Gray);

    // Draw the button usage info
    ButtonDist = 6;
    for (i = 0; i < 6; i++)
    {
      BelowButtonLengths[i] = FM_GetStringWidth(BelowButtonStrings[i], &Calibri_10_FontData);
      ButtonDist += BelowButtons[i]->width + 2 + BelowButtonLengths[i];
    }
    ButtonDist = (ScreenWidth - (int)(2*Overscan_X) - ButtonDist - 74) / 6;
    ButtonDist = min(ButtonDist, 20);

    PosY = InfoBarLine3_Y;
    PosX = Overscan_X + 6;
    for (i = 0; i < 5 /*6*/; i++)
    {
      TAP_Osd_PutGd(rgnInfoBar, PosX, PosY, BelowButtons[i], TRUE);
//      if (BelowButtons[i] == &_Button_Exit_Gd)
//        PosX += BelowButtons[i]->width + 3;
//      else
        PosX += BelowButtons[i]->width + 2;
      FM_PutString(rgnInfoBar, PosX, PosY + 1, PosX + max(BelowButtonLengths[i] + ButtonDist, 0), BelowButtonStrings[i], COLOR_White, ColorDarkBackground, &Calibri_10_FontData, TRUE, ALIGN_LEFT);
      PosX += BelowButtonLengths[i] + ButtonDist;
    }
//TAP_Osd_DrawRectangle(rgnInfoBar, Overscan_X+6, InfoBarLine3_Y, PosX - Overscan_X-6, 20, 2, COLOR_Gray);
  }

  if(rgnInfoBarMini)
  {
    // Draw border of ProgressBar
    TAP_Osd_FillBox(rgnInfoBarMini, 0, 0, GetOSDRegionWidth(rgnInfoBarMini), GetOSDRegionHeight(rgnInfoBarMini), ColorLightBackground);
    TAP_Osd_DrawRectangle(rgnInfoBarMini, 0, 0, GetOSDRegionWidth(rgnInfoBarMini), GetOSDRegionHeight(rgnInfoBarMini), 2, COLOR_Gray);
  }

  TRACEEXIT();
}

void OSDInfoDrawRecName(void)
{
  const int             FrameLeft    = Overscan_X + 45,                                                    FrameTop    =  0;  // 10
  const int             FrameWidth   = ScreenWidth - Overscan_X - InfoBarRightAreaWidth - FrameLeft - 2,   FrameHeight = 45;  // 26
  const int             TimeWidth    = FM_GetStringWidth("99:99 h", &Calibri_12_FontData) + 1,             TimeLeft    = FrameLeft + FrameWidth - TimeWidth - 2;
  int                   TimeTop      = FrameTop + 13,                                                      TitleLeft   = FrameLeft + 2;
  int                   TitleWidth   = FrameWidth - TimeWidth - (TitleLeft - FrameLeft) - 12;
  int                   TitleHeight, TitleTop;
  char                  TitleStr[MAX_FILE_NAME_SIZE];
  char                  TimeStr[8];
  dword                 TimeVal, Hours, Minutes, Seconds;

  #ifdef MC_UNICODE
    tFontDataUC        *UseTitleFont = NULL;
  #else
    tFontData          *UseTitleFont = NULL;
  #endif
  char                 *LastSpace = NULL;
  char                 *EndOfName = NULL;
  int                   EndOfNameWidth;

  TRACEENTER();

  if(rgnInfoBar)
  {
//    TAP_Osd_FillBox(rgnInfoBar, FrameLeft, FrameTop, FrameWidth, FrameHeight, COLOR_Blue);

    // Dateiname in neuen String kopieren
    TAP_SPrint(TitleStr, sizeof(TitleStr), PlaybackName);
    #ifndef MC_UNICODE
      if(isUTFToppy()) StrToISO(PlaybackName, TitleStr);
      TitleStr[sizeof(TitleStr)-1] = '\0';
    #endif

    // Passende Schriftgröße ermitteln
    UseTitleFont = &Calibri_14_FontData;
    if(FM_GetStringWidth(TitleStr, UseTitleFont) + 10 > (dword)TitleWidth)
    {
      UseTitleFont = &Calibri_12_FontData;
      TimeTop--;
      TitleLeft -= 2;
      TitleWidth += 2;
    }

    // Vertikale Schriftposition berechnen
    TitleHeight = FM_GetStringHeight(TitleStr, UseTitleFont);
    TitleTop = FrameTop + (FrameHeight - TitleHeight) / 2 - 1;

    // Titel ausgeben
    if(FM_GetStringWidth(TitleStr, UseTitleFont) > (dword)TitleWidth)
    {
      // Falls immernoch zu lang, Titel in der Mitte kürzen
      LastSpace = strrchr(TitleStr, ' ');
      if (LastSpace)
      {
        LastSpace[0] = '*';
        EndOfName = strrchr(TitleStr, ' ');
        LastSpace[0] = ' ';
      }
      if (EndOfName)
        EndOfName++;
      else
      {
        if (LastSpace)
          EndOfName = LastSpace + 1;
        else
          EndOfName = &TitleStr[strlen(TitleStr) - 12];
      }
      EndOfNameWidth = FM_GetStringWidth(EndOfName, UseTitleFont);
      FM_PutString(rgnInfoBar, TitleLeft,                                 TitleTop, TitleLeft + TitleWidth - EndOfNameWidth, TitleStr,  COLOR_White, ColorInfoBarTitle, UseTitleFont, TRUE, ALIGN_LEFT);
      FM_PutString(rgnInfoBar, TitleLeft + TitleWidth - EndOfNameWidth-1, TitleTop, TitleLeft + TitleWidth,                  EndOfName, COLOR_White, ColorInfoBarTitle, UseTitleFont, FALSE, ALIGN_LEFT);
    }
    else
      FM_PutString(rgnInfoBar, TitleLeft,                                 TitleTop, TitleLeft + TitleWidth,                  TitleStr,  COLOR_White, ColorInfoBarTitle, UseTitleFont, TRUE, ALIGN_LEFT);

    TAP_Osd_DrawRectangle(rgnInfoBar, TimeLeft - 4, 12, 1, 22, 1, RGB(92,93,93));

//    if(PLAYINFOVALID())  // bei ungültiger PlayInfo lautet schlimmstenfalls die Zeitanzeige 99:99
//    {
      TimeVal = PlayInfo.duration * 60 + PlayInfo.durationSec;
      if (TimeVal >= 3600)
      {
        Hours    = (int)(TimeVal / 3600);
        Minutes  = (int)(TimeVal / 60) % 60;
        if (Hours >= 100) {Hours = 99; Minutes = 99;}
        TAP_SPrint(TimeStr, 8, "%lu:%02lu h", Hours, Minutes);
      }
      else if (TimeVal >= 60)
      {
        Minutes  = (int)(TimeVal / 60) % 60;
        Seconds  = TimeVal % 60;
        if (Minutes < 10)
          TAP_SPrint(TimeStr, 7, "%lu:%02lu m", Minutes, Seconds);
        else
          TAP_SPrint(TimeStr, 7, "%lu min", Minutes);
      }
      else
      {
        Seconds  = TimeVal % 60;
        TAP_SPrint(TimeStr, 7, "%lu sek", Seconds);
      }
      FM_PutString(rgnInfoBar, TimeLeft, TimeTop, TimeLeft + TimeWidth - 1, TimeStr, COLOR_White, ColorInfoBarTitle, &Calibri_12_FontData, FALSE, ALIGN_CENTER);
//    }
  }
  TRACEEXIT();
}

void OSDInfoDrawPlayIcons(bool Force, bool DoSync)
{
  static TYPE_TrickMode  LastTrickMode = TRICKMODE_Slow;
  static byte            LastTrickModeSpeed = 99;
//  static bool            LastNoOSDMode = TRUE;

  TRACEENTER();

  if((Force && (rgnInfoBar || DoSync)) || (TrickMode != LastTrickMode) || (TrickModeSpeed != LastTrickModeSpeed))
  {
    if(rgnInfoBar)
    {
      const int            ButtonWidth = _Button_Play_Inactive_Gd.width,         ButtonDist = 3;
      const int            FrameWidth  = 5 * ButtonWidth + 4 * ButtonDist;
      const int            FrameLeft   = ScreenWidth - Overscan_X - FrameWidth,  FrameTop = 1;
      const int            ButtonTop   = FrameTop + 20;
      char                 SpeedText[6];
      int                  PosX, TextPosX=0;

      TAP_Osd_FillBox(rgnInfoBar, FrameLeft - 6, FrameTop, min(FrameWidth + 13, ScreenWidth-FrameLeft+6), ButtonTop - FrameTop, ColorInfoBarTitle);  // müsste eigentlich 7 nach links gehen...

      PosX = FrameLeft;
      TAP_Osd_PutGd(rgnInfoBar, PosX, ButtonTop, ((TrickMode == TRICKMODE_Rewind)  ? &_Button_Rwd_Active_Gd   : &_Button_Rwd_Inactive_Gd),   TRUE);
      if (TrickMode == TRICKMODE_Rewind) TextPosX = PosX; 

      PosX += ButtonWidth + ButtonDist;
      TAP_Osd_PutGd(rgnInfoBar, PosX, ButtonTop, ((TrickMode == TRICKMODE_Pause)   ? &_Button_Pause_Active_Gd : &_Button_Pause_Inactive_Gd), TRUE);

      PosX += ButtonWidth + ButtonDist;
      TAP_Osd_PutGd(rgnInfoBar, PosX, ButtonTop, ((TrickMode == TRICKMODE_Normal)  ? &_Button_Play_Active_Gd  : &_Button_Play_Inactive_Gd),  TRUE);

      PosX += ButtonWidth + ButtonDist;
      TAP_Osd_PutGd(rgnInfoBar, PosX, ButtonTop, ((TrickMode == TRICKMODE_Slow)    ? &_Button_Slow_Active_Gd  : &_Button_Slow_Inactive_Gd),  TRUE);
      if (TrickMode == TRICKMODE_Slow) TextPosX = PosX; 

      PosX += ButtonWidth + ButtonDist;
      TAP_Osd_PutGd(rgnInfoBar, PosX, ButtonTop, ((TrickMode == TRICKMODE_Forward) ? &_Button_Ffwd_Active_Gd  : &_Button_Ffwd_Inactive_Gd),  TRUE);
      if (TrickMode == TRICKMODE_Forward) TextPosX = PosX;

      if (TrickMode == TRICKMODE_Rewind || TrickMode == TRICKMODE_Forward || TrickMode == TRICKMODE_Slow)
      {
        TAP_SPrint(SpeedText, sizeof(SpeedText), ((TrickMode == TRICKMODE_Slow) ? "1/%dx" : "%dx"), (1 << TrickModeSpeed));
        FM_PutString(rgnInfoBar, TextPosX - 7, FrameTop, TextPosX + ButtonWidth + 6 , SpeedText, COLOR_White, COLOR_None, &Calibri_12_FontData, FALSE, ALIGN_CENTER);
      }
    }
    else
    {
      const int            RegionWidth  = _PlayState_Background_Gd.width;
      const int            RegionHeight = _PlayState_Background_Gd.height;
      const int            IconLeft = 6,   IconTop = 6;

      char                 SpeedText[6];
      TYPE_GrData         *ActiveSymbol = NULL;
      int                  PosX, PosX2;

      if(rgnTextState)
      {
        TAP_Osd_Delete(rgnTextState);
        rgnTextState = 0;
      }
      if(!rgnPlayState)
        rgnPlayState = TAP_Osd_Create(ScreenWidth - Overscan_X - RegionWidth - 20, Overscan_Y + 20, RegionWidth, RegionHeight, 0, 0);
      LastPlayStateChange = TAP_GetTick();
      if(LastPlayStateChange == 0) LastPlayStateChange = 1;

      TAP_Osd_PutGd(rgnPlayState, 0, 0, &_PlayState_Background_Gd, FALSE);
      switch(TrickMode)
      {
        case TRICKMODE_Normal:   ActiveSymbol = &_Icon_Playing_Gd;  break;
        case TRICKMODE_Forward:  ActiveSymbol = &_Icon_Ffwd_Gd;     break;
        case TRICKMODE_Rewind:   ActiveSymbol = &_Icon_Rwd_Gd;      break;
        case TRICKMODE_Slow:     ActiveSymbol = &_Icon_Slow_Gd;     break;
        case TRICKMODE_Pause:    ActiveSymbol = &_Icon_Pause_Gd;    break;
        default:                 return;
      }
      TAP_Osd_PutGd(rgnPlayState, IconLeft, IconTop, ActiveSymbol, TRUE);

      if (TrickMode == TRICKMODE_Rewind || TrickMode == TRICKMODE_Forward || TrickMode == TRICKMODE_Slow)
      {
        TAP_SPrint(SpeedText, sizeof(SpeedText), "%dx", (1 << TrickModeSpeed));
        if (TrickMode == TRICKMODE_Slow)
        {
          PosX  = IconLeft + 6;
          PosX2 = PosX + FM_GetStringWidth("1", &Calibri_10_FontData);
          FM_PutString(rgnPlayState, PosX, IconTop + 5, PosX2, "1",       COLOR_White, COLOR_None, &Calibri_10_FontData, FALSE, ALIGN_LEFT);
          PosX  = PosX2 - 1;
          PosX2 = PosX + FM_GetStringWidth("/", &Calibri_10_FontData);
          FM_PutString(rgnPlayState, PosX, IconTop + 6, PosX2, "/",       COLOR_White, COLOR_None, &Calibri_10_FontData, FALSE, ALIGN_LEFT);
          PosX  = PosX2;
          PosX2 = PosX + FM_GetStringWidth(SpeedText, &Calibri_12_FontData);
          FM_PutString(rgnPlayState, PosX, IconTop + 7, PosX2, SpeedText, COLOR_White, COLOR_None, &Calibri_12_FontData, FALSE, ALIGN_LEFT);
        }
        else
          FM_PutString(rgnPlayState, IconLeft + 7, IconTop + 8, RegionWidth - 1, SpeedText,  COLOR_White, COLOR_None, &Calibri_12_FontData, FALSE, ALIGN_LEFT);
      }
//      TAP_Osd_Sync();
      OSDInfoDrawCurrentPlayTime(TRUE);
    }

    LastTrickMode = TrickMode;
    LastTrickModeSpeed = TrickModeSpeed;
    if(DoSync) TAP_Osd_Sync();
  }

//  LastNoOSDMode = (OSDMode == MD_NoOSD);
  TRACEEXIT();
}

void OSDInfoDrawCurrentPlayTime(bool Force)
{
  static dword          LastSec = (dword)-1;
  static dword          VisibleBlock = 0;
  dword                 Time;
  float                 Percent;
  dword                 PercentWidth;
  char                  TimeString[12];
  char                  PercentString[12];

  TRACEENTER();
  if (((int)PlayInfo.totalBlock <= 0) /*|| ((int)PlayInfo.currentBlock < 0)*/)   // wenn currentBlock nicht gesetzt, schlimmstenfalls zu hohe Prozentzahl
  {
    TRACEEXIT();
    return;
  }

  // Stabilisierung der vor- und zurückspringenden Zeit-Anzeige während Wiedergabe (neu)
  if (JumpRequestedBlock != (dword) -1)
    VisibleBlock = JumpRequestedBlock;
  else
    if (Force || (TrickMode!=TRICKMODE_Normal && TrickMode!=TRICKMODE_Forward && TrickMode!=TRICKMODE_Slow) || (PlayInfo.currentBlock > VisibleBlock) || (PlayInfo.currentBlock + 100 < VisibleBlock))
      VisibleBlock = PlayInfo.currentBlock;

  // Nur neu zeichnen, wenn sich die Sekunden-Zahl geändert hat
  Time = NavGetBlockTimeStamp(VisibleBlock) / 1000;
  if (Force || (Time != LastSec))
  {
    SecToTimeString(Time, TimeString);
    Percent = ((float)VisibleBlock / PlayInfo.totalBlock) * 100.0;
    if (abs(Percent) >= 999) Percent = 999.0;
    TAP_SPrint(PercentString, sizeof(PercentString), "%1.1f%%", Percent);

    if(rgnInfoBar)
    {
      const int Frame1Width = 78,                                                   Frame2Width = InfoBarRightAreaWidth - Frame1Width - 1;
      const int Frame1Left  = ScreenWidth - Overscan_X - InfoBarRightAreaWidth,     Frame2Left  = Frame1Left + Frame1Width + 1;
      PercentWidth = (dword)((float)PlayInfo.currentBlock * InfoBarRightAreaWidth / PlayInfo.totalBlock);
      PercentWidth = min(PercentWidth, InfoBarRightAreaWidth + 1);

      TAP_Osd_FillBox      (rgnInfoBar, Frame1Left,               InfoBarLine1_Y, InfoBarRightAreaWidth, InfoBarLine1Height, ColorInfoBarDarkSub);
      TAP_Osd_DrawRectangle(rgnInfoBar, Frame1Left + Frame1Width, InfoBarLine1_Y + 6, 1, 17, 1, RGB(92,93,93));

      TAP_Osd_FillBox      (rgnInfoBar, Frame1Left,   InfoBarLine1_Y + InfoBarLine1Height - 2, PercentWidth, 2, COLOR_Gray);
      FM_PutString         (rgnInfoBar, Frame1Left+1, InfoBarLine1_Y + 5, Frame1Left + Frame1Width - 1, PercentString, COLOR_White, ColorInfoBarDarkSub, &Calibri_12_FontData,     FALSE, ALIGN_CENTER);
      #ifdef MC_UNICODE
        FM_PutString       (rgnInfoBar, Frame2Left,   InfoBarLine1_Y + 5, Frame2Left + Frame2Width - 1, TimeString,    COLOR_White, ColorInfoBarDarkSub, &Courier_New_13_FontData, FALSE, ALIGN_CENTER);
      #else
        FM_PutString       (rgnInfoBar, Frame2Left,   InfoBarLine1_Y + 5, Frame2Left + Frame2Width - 1, TimeString,    COLOR_White, ColorInfoBarDarkSub, &Calibri_12_FontData,     FALSE, ALIGN_CENTER);
      #endif
    }
    if(rgnInfoBarMini)
    {
      const int FrameWidth = FM_GetStringWidth("9:99:99", &Calibri_12_FontData) + 1,   FrameHeight = GetOSDRegionHeight(rgnInfoBarMini) - 6;
      const int FrameLeft  = GetOSDRegionWidth(rgnInfoBarMini) - 2 - FrameWidth,           FrameTop = 3;

      TAP_Osd_FillBox(rgnInfoBarMini, FrameLeft, FrameTop, FrameWidth, FrameHeight, ColorLightBackground);
      FM_PutString(rgnInfoBarMini, FrameLeft, FrameTop + 6, FrameLeft + FrameWidth - 1, TimeString, COLOR_White, ColorLightBackground, &Calibri_12_FontData, FALSE, ALIGN_RIGHT);
    }
    if(rgnPlayState)
    {
      const int ProgBarWidth  = _PlayState_Background_Gd.width - 10,   ProgBarHeight = 3;
      const int ProgBarLeft   = 5,                                     ProgBarTop    = _PlayState_Background_Gd.height - ProgBarHeight - 5;

      const int PlayTimeWidth = FM_GetStringWidth("9:99:99", &Calibri_10_FontData) + 1,        PlayTimeHeight = FM_GetStringHeight("9:99:99", &Calibri_10_FontData);
      const int PlayTimeLeft  = _PlayState_Background_Gd.width - PlayTimeWidth - ProgBarLeft + 1,  PlayTimeTop    = ProgBarTop - PlayTimeHeight - 1;

      FM_PutString(rgnPlayState, PlayTimeLeft, PlayTimeTop, PlayTimeLeft + PlayTimeWidth - 1, TimeString, COLOR_White, ColorDarkBackground, &Calibri_10_FontData, FALSE, ALIGN_RIGHT);

      PercentWidth = (dword)((float)PlayInfo.currentBlock * ProgBarWidth / PlayInfo.totalBlock);
      PercentWidth = min(PercentWidth, (dword)ProgBarWidth + 1);
      TAP_Osd_FillBox(rgnPlayState, ProgBarLeft, ProgBarTop, ProgBarWidth, ProgBarHeight, COLOR_Gray);
      TAP_Osd_FillBox(rgnPlayState, ProgBarLeft, ProgBarTop, PercentWidth, ProgBarHeight, RGB(250,0,0));
    }

    LastSec = Time;
  }
  TRACEEXIT();
}

void OSDInfoDrawBookmarkMode(bool DoSync)
{
  const int             FrameWidth = InfoBarModeAreaWidth;
  const int             FrameLeft  = ScreenWidth - Overscan_X - InfoBarRightAreaWidth - FrameWidth - 2;
  TRACEENTER();

  if(rgnInfoBar)
  {
    TAP_Osd_FillBox(rgnInfoBar, FrameLeft, InfoBarLine1_Y, FrameWidth, InfoBarLine1Height, ColorInfoBarLightSub);
    FM_PutString (rgnInfoBar, FrameLeft, InfoBarLine1_Y + 7, FrameLeft + FrameWidth-1, ((BookmarkMode ? LangGetString(LS_Bookmarks) : LangGetString(LS_Segments))), ((BookmarkMode) ? RGB(60,255,60) : RGB(250,139,18)), ColorInfoBarLightSub, &Calibri_10_FontData, FALSE, ALIGN_CENTER);
    if(DoSync) TAP_Osd_Sync();
  }

  TRACEEXIT();
}

void OSDInfoDrawClock(bool Force)
{
  const int             FrameWidth  = FM_GetStringWidth("99:99", &Calibri_14_FontData) + 1;
  const int             FrameLeft   = ScreenWidth - FrameWidth - Overscan_X + 1 - 6;
  const int             FrameTop    = InfoBarLine3_Y - 3;

  word                  mjd;
  byte                  hour, min, sec;
  static byte           LastMin = 99;
  char                  Time[8];

  TRACEENTER();

  if(rgnInfoBar)
  {
    TAP_GetTime(&mjd, &hour, &min, &sec);
    if((min != LastMin) || Force)
    {
      TAP_SPrint(Time, sizeof(Time), "%02d:%02d", hour, min);
      FM_PutString (rgnInfoBar, FrameLeft, FrameTop, FrameLeft + FrameWidth - 1, Time, COLOR_White, ColorDarkBackground, &Calibri_14_FontData, FALSE, ALIGN_RIGHT);
      LastMin = min;
    }
  }

  TRACEEXIT();
}

void OSDInfoDrawMinuteJump(bool DoSync)
{
  const int             FrameLeft  = ScreenWidth - Overscan_X - InfoBarRightAreaWidth + 4;
  const int             FrameTop   = 4,                                                            ButtonTop   = FrameTop + 17;
  const int             FrameWidth = _Button_SkipLeft_Gd.width + _Button_SkipRight_Gd.width + 1,   FrameHeight = ButtonTop - FrameTop + _Button_SkipLeft_Gd.height;
  char InfoStr[5];

  TRACEENTER();

  if(rgnInfoBar)
  {
    TAP_Osd_FillBox(rgnInfoBar, FrameLeft, FrameTop, FrameWidth, FrameHeight, ColorInfoBarTitle);
    if(BookmarkMode || MinuteJump)
    {
      TAP_Osd_PutGd(rgnInfoBar, FrameLeft,                                 ButtonTop, &_Button_SkipLeft_Gd,  TRUE);
      TAP_Osd_PutGd(rgnInfoBar, FrameLeft + 1 + _Button_SkipLeft_Gd.width, ButtonTop, &_Button_SkipRight_Gd, TRUE);

      if (MinuteJump)
        TAP_SPrint(InfoStr, sizeof(InfoStr), "%d'", MinuteJump);
      else
        TAP_SPrint(InfoStr, sizeof(InfoStr), LangGetString(LS_BM));
      FM_PutString(rgnInfoBar, FrameLeft + 1, FrameTop, FrameLeft + FrameWidth - 1, InfoStr, COLOR_White, COLOR_None, &Calibri_10_FontData, FALSE, ALIGN_CENTER);
    }
    if(DoSync) TAP_Osd_Sync();
  }

  TRACEEXIT();
}

void OSDTextStateWindow(int MessageID)
{
  const int RegionWidth = 200;
  const int RegionHeight = 30;
  const int TextPositionY = 5;

  char  MessageStr[512];
  char *pMessageStr;
  bool  NoMiniOSD;

  TRACEENTER();

  if((OSDMode == MD_NoOSD) || (OSDMode == MD_MiniOSD && (MessageID==LS_MinuteJumpActive || MessageID==LS_MinuteJumpDisabled)))
  {
    if(rgnPlayState)
    {
      TAP_Osd_Delete(rgnPlayState);
      rgnPlayState = 0;
    }
    if(!rgnTextState)
      rgnTextState = TAP_Osd_Create(ScreenWidth - Overscan_X - RegionWidth - 20, Overscan_Y + 20, RegionWidth, RegionHeight, 0, 0);
    LastPlayStateChange = TAP_GetTick();
    if(LastPlayStateChange == 0) LastPlayStateChange = 1;

    TAP_Osd_FillBox(rgnTextState, 0, 0, RegionWidth, RegionHeight, ColorLightBackground);
    TAP_Osd_DrawRectangle(rgnTextState, 0, 0, RegionWidth, RegionHeight, 2, COLOR_Gray);

    pMessageStr = LangGetString(MessageID);
    NoMiniOSD = FALSE;
    switch(MessageID)
    {
      case LS_MovieCutterActive:
      {
        TAP_SPrint(MessageStr, sizeof(MessageStr), "%s (%s)", pMessageStr, LangGetString((BookmarkMode) ? LS_BM : LS_S));
        pMessageStr = MessageStr;
        NoMiniOSD = TRUE;
        break;
      }
      case LS_MinuteJumpActive:
      {
        TAP_SPrint(MessageStr, sizeof(MessageStr), pMessageStr, MinuteJump);
        pMessageStr = MessageStr;
        NoMiniOSD = TRUE;
        break;
      }
      case LS_MinuteJumpDisabled:
      {
        NoMiniOSD = TRUE;
        break;
      }  
    }
    FM_PutString(rgnTextState, 2, TextPositionY, RegionWidth-3, pMessageStr, RGB(255, 255, 224), ColorLightBackground, &Calibri_12_FontData, TRUE, ALIGN_CENTER);
    TAP_Osd_Sync();

    if (OSDMode == MD_NoOSD && !NoMiniOSD)
    {
      OSDMode = MD_MiniOSD;
      OSDRedrawEverything();
      OSDMode = MD_NoOSD;
    }
  }
  TRACEEXIT();
}

// Segment-Text-Fenster
void OSDSegmentTextDraw(bool Force)
{
  const int             RegionHeight = 100;
  const int             RegionWidth = (rgnSegmentList) ? (ScreenWidth - Overscan_X - SegmentList_X - _SegmentList_Background_Gd.width - 15) : (ScreenWidth - 2*Overscan_X - 100);
  const int             RegionLeft  = (rgnSegmentList) ? (ScreenWidth - RegionWidth - (int)Overscan_X) : ((ScreenWidth-RegionWidth) / 2);
  const int             RegionTop   = (rgnInfoBar) ? (ScreenHeight - 160 - RegionHeight - 2) : (ScreenHeight - Overscan_Y - 38 - 32 - RegionHeight - 8);
  const int             SpaceWidth  = FM_GetStringWidth(" ", &Calibri_12_FontData);
  static word           LastCurSegment = 0;
  char                  curLine[512], StartTime[12], EndTime[12];
  word                  CurrentSegment;

  TRACEENTER();

  if (ShowSegmentText)
  {
    if (JumpRequestedSegment != 0xFFFF)
      CurrentSegment = JumpRequestedSegment;
    else
      CurrentSegment = ActiveSegment;
    if(CurrentSegment >= NrSegmentMarker - 1)
      CurrentSegment = NrSegmentMarker - 2;

    if (!rgnSegmentText)
      rgnSegmentText = TAP_Osd_Create(RegionLeft, RegionTop, RegionWidth, RegionHeight, 0, 0);

    if (Force || (CurrentSegment != LastCurSegment))
    {
      TAP_Osd_DrawRectangle(rgnSegmentText, 0, 0, RegionWidth, RegionHeight, 2, ColorInfoBarLightSub);
      TAP_Osd_FillBox(rgnSegmentText, 2, 2, RegionWidth-4, RegionHeight-4, ColorDarkBackground);
      TAP_Osd_FillBox(rgnSegmentText, 2, 2, RegionWidth-4, FM_GetStringHeight(LangGetString(LS_SegmentNr), &Calibri_14_FontData)+2, ColorInfoBarLightSub);

      TAP_SPrint(curLine, sizeof(curLine), LangGetString(LS_SegmentNr), CurrentSegment+1);
      FM_PutString(rgnSegmentText, 10, 1, RegionWidth-10, curLine, RGB(216, 137, 18), ColorInfoBarLightSub, &Calibri_14_FontData, TRUE, ALIGN_LEFT);
      SecToTimeString((SegmentMarker[CurrentSegment].Timems) / 1000, StartTime);
      SecToTimeString((SegmentMarker[CurrentSegment + 1].Timems) / 1000, EndTime);
      TAP_SPrint(curLine, sizeof(curLine), "%s - %s", StartTime, EndTime);
      FM_PutString(rgnSegmentText, RegionWidth/2-80, 4, RegionWidth/2+80, curLine, COLOR_White, ColorInfoBarLightSub, &Calibri_12_FontData, TRUE, ALIGN_CENTER);
      SecToTimeString((SegmentMarker[CurrentSegment + 1].Timems - SegmentMarker[CurrentSegment].Timems) / 1000, EndTime);
      if ((SegmentMarker[CurrentSegment + 1].Timems - SegmentMarker[CurrentSegment].Timems) / 1000 < 3600)
        TAP_SPrint(curLine, sizeof(curLine), "%s min", &EndTime[2]);
      else
        TAP_SPrint(curLine, sizeof(curLine), "%s h", EndTime);
      FM_PutString(rgnSegmentText, RegionWidth-100, 4, RegionWidth-15-_Icon_SegmentText_Gd.width, curLine, COLOR_White, ColorInfoBarLightSub, &Calibri_12_FontData, TRUE, ALIGN_RIGHT);
      TAP_Osd_PutGd(rgnSegmentText, RegionWidth-_Icon_SegmentText_Gd.width-5, 9, &_Icon_SegmentText_Gd, TRUE);

      if (CurrentSegment != LastCurSegment)
        SegmentTextStartLine = 0;
      if (SegmentMarker[CurrentSegment].pCaption)
      {
        char *pFullText = SegmentMarker[CurrentSegment].pCaption;
        int curWidth = 0, TokenWidth = 0, i = 0;

        curLine[0] = '\0';
        char *curToken = strtok(pFullText, " \t\r\n");

        while (i < SegmentTextStartLine+3)
        {
          if (curToken)
            TokenWidth = FM_GetStringWidth(curToken, &Calibri_12_FontData);
          if (!curToken || ((curWidth + (TokenWidth + ((curWidth>0) ? SpaceWidth : 0))) >= RegionWidth-15))
          {
            // Zeile ausgeben und zurücksetzen
            if (i >= SegmentTextStartLine)
            {
              if (!*curLine && curToken)
                TAP_SPrint(curLine, sizeof(curLine), "%s", curToken); 
              FM_PutString(rgnSegmentText, 10, 31+(i-SegmentTextStartLine)*21, RegionWidth-5, curLine, RGB(192, 192, 192), ColorDarkBackground, &Calibri_12_FontData, TRUE, ALIGN_LEFT);
            }
            curWidth = 0;
            curLine[0] = '\0';
            i++;
          }
          if (curToken)  // Token an Zeile anfügen
          {
            strcat(curLine, (*curLine) ? &curToken[-1] : curToken);
            curWidth += (TokenWidth + ((curWidth>0) ? SpaceWidth : 0));
          }
          else
            break;

          // Eingabe-String reparieren (nächstes Token holen und 0 davor entfernen)
          if ((curToken = strtok(NULL, (i < SegmentTextStartLine+3) ? " \t\r\n" : "")) != NULL)
          {
            int j = 0;
            while ((curToken-j > pFullText) && (curToken[-j-1] == ' '))
              j++;
            if (curToken-j > pFullText)
              curToken[-j-1] = ' ';
          }
        }

        if (SegmentTextStartLine > 0)
          TAP_Osd_PutGd(rgnSegmentText, RegionWidth-_Button_Up_small_Gd.width-3, 31, &_Button_Up_small_Gd, TRUE);
        if (curToken)
          TAP_Osd_PutGd(rgnSegmentText, RegionWidth-_Button_Down_small_Gd.width-3, RegionHeight-_Button_Down_small_Gd.height-5, &_Button_Down_small_Gd, TRUE);

        if (i <= SegmentTextStartLine)
          ShowSegmentText = FALSE;
      }
      else if (SegmentTextStartLine > 0)
        ShowSegmentText = FALSE;

      TAP_Osd_Sync();
      LastCurSegment = CurrentSegment;
    }
  }
  if (!ShowSegmentText && rgnSegmentText)
  {
    TAP_Osd_Delete(rgnSegmentText);
    rgnSegmentText = 0;
    TAP_Osd_Sync();
  }
  TRACEEXIT();
}

// RecStrip-Fortschrittsanzeige
void OSDRecStripProgressBar(void)
{
  const int             RegionWidth = 150;
  const int             RegionHeight = 30;
  static int            LastPercent = 0;
  char                  ProgressStr[10];

  TRACEENTER();

  // Progress-OSD einblenden
  if (!rgnStripProgBar)
  {
    dword SysState, SysSubState;
    TAP_GetState(&SysState, &SysSubState);
    if(SysSubState != 0)
    {
      TAP_ExitNormal();
      TAP_EnterNormalNoInfo();
    }
//    TAP_ExitNormal();
    rgnStripProgBar = TAP_Osd_Create(Overscan_X, Overscan_Y + 16, RegionWidth, RegionHeight, 0, 0);
    TAP_Osd_FillBox(rgnStripProgBar, 0, 0, RegionWidth, RegionHeight, ColorLightBackground);
    TAP_Osd_DrawRectangle(rgnStripProgBar, 0, 0, RegionWidth, RegionHeight, 2, COLOR_Gray);
    FM_PutString(rgnStripProgBar, 5, 2, RegionWidth-3, LangGetString(LS_StrippingMsg), RGB(255, 255, 224), ColorLightBackground, &Calibri_10_FontData, TRUE, ALIGN_LEFT);
    TAP_Osd_FillBox(rgnStripProgBar, 5, RegionHeight-10, 100, 5, COLOR_Gray);
    TAP_Osd_Sync();
  }

  if (rgnStripProgBar && (RecStrip_Percent != LastPercent))
  {
    TAP_Osd_FillBox(rgnStripProgBar, 5, RegionHeight-10, min(RecStrip_Percent, 100), 5, COLOR_Red);
    TAP_SPrint(ProgressStr, sizeof(ProgressStr), "%3d %%", RecStrip_Percent);
    FM_PutString(rgnStripProgBar, 108, 12, RegionWidth-5, ProgressStr, RGB(255, 255, 224), ColorLightBackground, &Calibri_10_FontData, FALSE, ALIGN_RIGHT);
    TAP_Osd_Sync();
  }

  TRACEEXIT();
}


// ----------------------------------------------------------------------------
//                          ActionMenu-Funktionen
// ----------------------------------------------------------------------------
void ActionMenuDraw(void)
{
  const int             TextFieldStart_X  =   7,   TextFieldStart_Y  = 4;
  const int             TextFieldHeight   =  26,   TextFieldDist     = 2;
  const int             ShortButtonLeft   =  _ActionMenu9_Gd.width - TextFieldStart_X - _Button_Sleep_small_Gd.width - 5;
  const dword           Color_Inactive    =  RGB(120, 120, 120);
  const dword           Color_Warning     =  RGB(250, 139, 18);
  TYPE_GrData*          LowerButtons[]    =  {&_Button_Down_Gd, &_Button_Up_Gd, &_Button_vf_Gd, &_Button_Ok_Gd, &_Button_Exit_Gd};

  char                  TempStr[128];
  char                 *DisplayStr;
  dword                 DisplayColor;
  dword                 PosX, PosY;
  int                   i, j;

  TRACEENTER();
  CountSelectedSegments();

  // Region erzeugen und Hintergrund zeichnen
  if(!rgnActionMenu)
  {
    rgnActionMenu = TAP_Osd_Create(((ScreenWidth - _ActionMenu9_Gd.width) / 2) +29, 70, _ActionMenu9_Gd.width, _ActionMenu9_Gd.height, 0, 0);
//    ActionMenuItem = 0;
    jfs_fsck_present = HDD_Exist2("jfs_fsck", FSCKPATH);
    if(jfs_fsck_present)
      chmod(FSCKPATH "/jfs_fsck", 0777);
  }
  TAP_Osd_PutGd(rgnActionMenu, 0, 0, &_ActionMenu9_Gd, FALSE);

  // Buttons unterhalb der Liste zeichnen
  PosX = TextFieldStart_X + 12;
  PosY = TextFieldStart_Y + (TextFieldHeight + TextFieldDist) * MI_NrMenuItems + 2;
  for(i = 0; i < (int)(sizeof(LowerButtons) / sizeof(TYPE_GrData*)) ; i++)
  {
    TAP_Osd_PutGd(rgnActionMenu, PosX, PosY, LowerButtons[i], TRUE);
    PosX += (LowerButtons[i]->width + 5);
  }

  // ShortCut-Buttons zeichnen
  for (i = 1; i <= 9; i++)
  {
    TAP_Osd_PutGd(rgnActionMenu, ShortButtonLeft, TextFieldStart_Y + (TextFieldHeight + TextFieldDist) * i + 4, ShortButtons[i-1], TRUE);
  }

  // Grünen Auswahlrahmen zeichnen
  TAP_Osd_PutGd(rgnActionMenu, TextFieldStart_X, TextFieldStart_Y + (TextFieldHeight + TextFieldDist) * ActionMenuItem, &_ActionMenu_Bar_Gd, FALSE);

  // Menü-Einträge zeichnen
  for (i = 0; i < MI_NrMenuItems; i++)
  {
    DisplayStr = NULL;
    DisplayColor = (ActionMenuItem == i ? COLOR_Black : COLOR_White);
    switch(i)
    {
      case MI_SelectFunction:
      {
        DisplayStr = LangGetString(LS_SelectFunction);
/*        #ifndef MC_UNICODE
          char *p = DisplayStr;
          while (p && *p)
          {
            if (p[0] == 'â' || p[0] == '€' || p[0] == '•')  p[0] = '-';
            p++;
          }
        #endif  */
        break;
      }
      case MI_SaveSegments:
      case MI_DeleteSegments:
      {
        if (i == MI_SaveSegments)
        {
          if (NrSelectedSegments == 0)
            DisplayStr = LangGetString(LS_SaveCurSegment);
          else if (NrSelectedSegments == 1)
            DisplayStr = LangGetString(LS_Save1Segment);
          else
          {
            TAP_SPrint(TempStr, sizeof(TempStr), LangGetString(LS_SaveNrSegments), NrSelectedSegments);
            DisplayStr = TempStr;
          }
        }
        else
        {
          if (NrSelectedSegments == 0)
            DisplayStr = LangGetString(LS_DeleteCurSegment);
          else if (NrSelectedSegments == 1)
            DisplayStr = LangGetString(LS_Delete1Segment);
          else
          {
            TAP_SPrint(TempStr, sizeof(TempStr), LangGetString(LS_DeleteNrSegments), NrSelectedSegments);
            DisplayStr = TempStr;
          }
        }

        if (NrSegmentMarker <= 2)
          DisplayColor = Color_Inactive;
        else
        {
          for(j = 0; j < NrSegmentMarker - 1; j++)
          {
            if (SegmentMarker[j].Selected || ((NrSelectedSegments == 0) && (j == ActiveSegment)))
              if (isLargeSegment(SegmentMarker[j].Block, SegmentMarker[j+1].Block, (j==NrSegmentMarker-2), (i==MI_SaveSegments)))
              {
                DisplayColor = Color_Warning;
                break;
              }
          }
        }
        break;
      }
      case MI_CopySegSplitMovie:
      {
        if (!BookmarkMode)
        {
          if (NrSelectedSegments == 0)
            DisplayStr = LangGetString(LS_CopyCurSegment);
          else if (NrSelectedSegments == 1)
            DisplayStr = LangGetString(LS_Copy1Segment);
          else
          {
            TAP_SPrint(TempStr, sizeof(TempStr), LangGetString(LS_CopyNrSegments), NrSelectedSegments);
            DisplayStr = TempStr;
          }
          if (RecStrip_active && RecStrip_DoCut)
          {
            char *p;
            TAP_SPrint(TempStr, sizeof(TempStr), DisplayStr);
            p = strchr(TempStr, ':');
            if (p) *p = '\0';
            TAP_SPrint(&TempStr[strlen(TempStr)], sizeof(TempStr)-strlen(TempStr), " (%d %%)", RecStrip_Percent);
            DisplayStr = TempStr;
          }
          if (!RecStrip_present || RecStrip_active || (NrSegmentMarker <= 2))
            DisplayColor = Color_Inactive;
        }
        else
        {
          DisplayStr = LangGetString(LS_SplitMovie);
          if (PlayInfo.currentBlock == 0)
            DisplayColor = Color_Inactive;
          else if (isLargeSegment(PlayInfo.currentBlock, SegmentMarker[NrSegmentMarker-1].Block, TRUE, TRUE))
            DisplayColor = Color_Warning;
        }
        break;
      }
      case MI_SelectEvOddSegments:
      {
        if (ActionMenuEvenOdd == 1)
          DisplayStr = (NrSegmentMarker == 4) ? LangGetString(LS_SelectPadding) : LangGetString(LS_SelectOddSegments);
        else
          DisplayStr = (NrSegmentMarker == 4) ? LangGetString(LS_SelectMiddle) : LangGetString(LS_SelectEvenSegments);
        if (DirectSegmentsCut)
        {
          if (ActionMenuEvenOdd == 1)
            DisplayStr = (NrSegmentMarker == 4) ? LangGetString(LS_RemovePadding) : LangGetString(LS_DeleteOddSegments);
          else
            DisplayStr = LangGetString(LS_DeleteEvenSegments);
        }
        if (NrSegmentMarker <= 2) DisplayColor = Color_Inactive;
        break;
      }
      case MI_ClearAll:
      {
        if (BookmarkMode)
        {
          DisplayStr = LangGetString(LS_DeleteAllBookmarks);
          if (NrBookmarks <= 0) DisplayColor = Color_Inactive;
        }
        else
        {
          if (NrSelectedSegments > 0)
            DisplayStr = LangGetString(LS_UnselectAll);
          else
          {
            DisplayStr = LangGetString(LS_ClearSegmentList);
            if (NrSegmentMarker <= 2) DisplayColor = Color_Inactive;
          }
        }
        break;
      }
      case MI_RenameMovie:
      {
        DisplayStr = LangGetString(LS_RenameMovie);
        break;
      }
      case MI_ImExportBookmarks:
      {
        if (!BookmarkMode)
        {
          DisplayStr = LangGetString(LS_ImportBM);
          if (NrBookmarks <= 0) DisplayColor = Color_Inactive;
        }
        else
        {
          DisplayStr = LangGetString(LS_ExportToBM);
          if (NrSegmentMarker <= 2 || MediaFileMode) DisplayColor = Color_Inactive;
        }
        break;
      }
      case MI_RecStripCheckFS:
      {
        if (!BookmarkMode)
        {
          if (RecStrip_active && !RecStrip_DoCut)
          {
            TAP_SPrint(TempStr, sizeof(TempStr), "%s (%d %%)", LangGetString(LS_StripRec), RecStrip_Percent);
            DisplayStr = TempStr;
          }
          else
            DisplayStr = (isStripped) ? LangGetString(LS_StrippedRec) : LangGetString(LS_StripRec);
          DisplayColor = (RecStrip_present && !RecStrip_active) ? ((isStripped) ? RGB(80, 240, 80) : RGB(255, 150, 150)) : Color_Inactive;
        }
        else
        {
          DisplayStr = LangGetString(LS_FileSystemCheck);
          DisplayColor = (jfs_fsck_present) ? ((CheckFSAfterCut && InodeMonitoring) ? RGB(80, 240, 80) : RGB(255, 150, 150)) : Color_Inactive;
        }
        break;
      }
      case MI_ExitMC:
      {
        DisplayStr = LangGetString(LS_ExitMC);
        break;
      }
      case MI_NrMenuItems:
        break;
    }

    if (DisplayColor == Color_Inactive)
      TAP_Osd_FillBox(rgnActionMenu, ShortButtonLeft,    TextFieldStart_Y + (TextFieldHeight + TextFieldDist) * i + 4, _Button_Sleep_small_Gd.width, _Button_Sleep_small_Gd.height, ColorInfoBarDarkSub);
    if (DisplayStr && (i < MI_NrMenuItems))
      FM_PutString(rgnActionMenu, TextFieldStart_X + 10, TextFieldStart_Y + (TextFieldHeight + TextFieldDist) * i + 1, ShortButtonLeft, DisplayStr, DisplayColor, COLOR_None, &Calibri_14_FontData, TRUE, ((i==0) ? ALIGN_CENTER : ALIGN_LEFT));
  }

  TAP_Osd_Sync();
  TRACEEXIT();
}

void ActionSubMenuDraw(void)
{
  const int             TextFieldStart_X  =   5,   TextFieldStart_Y  = 5;
  const int             TextFieldHeight   =  26,   TextFieldDist     = 2;
  const int             SubMenuHeight     =  2*TextFieldStart_Y + 4*(TextFieldHeight+TextFieldDist);
  const int             SubMenuWidth      =  2*TextFieldStart_X + _SubMenu_Bar_Gd.width;
  const int             ShortButtonLeft   =  TextFieldStart_X + _SubMenu_Bar_Gd.width - _Button_Sleep_small_Gd.width - 5;
  int                   i;

  TRACEENTER();
//  CountSelectedSegments();

  // Region erzeugen und Hintergrund zeichnen
  if(!rgnActionSubMenu)
  {
    rgnActionSubMenu = TAP_Osd_Create(((ScreenWidth - _ActionMenu9_Gd.width) / 2) +29 +86, 70+TextFieldStart_Y + 3*(TextFieldHeight+TextFieldDist) +2, SubMenuWidth, SubMenuHeight, 0, 0);
  }
  TAP_Osd_FillBox(rgnActionSubMenu, 0, 0, SubMenuWidth, SubMenuHeight, ColorLightBackground);
  TAP_Osd_DrawRectangle(rgnActionSubMenu, 0, 0, SubMenuWidth, SubMenuHeight, 2, COLOR_Gray);

  // Rahmen und ShortCut-Buttons zeichnen
  for (i = 0; i < 4; i++)
  {
    TAP_Osd_FillBox(rgnActionSubMenu, TextFieldStart_X, TextFieldStart_Y + (TextFieldHeight+TextFieldDist) * i, _SubMenu_Bar_Gd.width, TextFieldHeight, ColorInfoBarDarkSub);
    TAP_Osd_PutGd(rgnActionSubMenu,   ShortButtonLeft,  TextFieldStart_Y + (TextFieldHeight+TextFieldDist) * i + 4, ShortButtons[i], TRUE);
  }

  // Grünen Auswahlrahmen zeichnen
  TAP_Osd_PutGd  (rgnActionSubMenu,   TextFieldStart_X, TextFieldStart_Y + (TextFieldHeight+TextFieldDist) * ActionSubMenuItem, &_SubMenu_Bar_Gd, FALSE);
//  for (i = 0; i < _SubMenu_Bar_Gd.width; i+=2)
//    TAP_Osd_PutGd(rgnActionSubMenu, TextFieldStart_X + i, TextFieldStart_Y + (TextFieldHeight+TextFieldDist) * ActionSubMenuItem, &_SubMenu_Bar_Gd, FALSE);
//    TAP_Osd_Copy(rgnActionMenu, rgnActionSubMenu, 7 +10, 4 + (TextFieldHeight+TextFieldDist)*3, 2, TextFieldHeight, TextFieldStart_X + i, TextFieldStart_Y + (TextFieldHeight+TextFieldDist) * ActionSubMenuItem, FALSE); 

  // Menü-Einträge zeichnen
  for (i = 0; i < 4; i++)
  {
    FM_PutString(rgnActionSubMenu, TextFieldStart_X + 5, TextFieldStart_Y + (TextFieldHeight+TextFieldDist) * i + 1, ShortButtonLeft, LangGetString(LS_CopySeparate+i), COLOR_White, COLOR_None, &Calibri_14_FontData, TRUE, ALIGN_LEFT);
  }

  TAP_Osd_Sync();
  TRACEEXIT();
}

bool ActionMenuItemInactive(int MenuItem)
{
  return (((MenuItem==MI_SaveSegments||MenuItem==MI_DeleteSegments||MenuItem==MI_SelectEvOddSegments) && NrSegmentMarker<=2) || (MenuItem==MI_CopySegSplitMovie && BookmarkMode && PlayInfo.currentBlock==0) || (MenuItem==MI_CopySegSplitMovie && !BookmarkMode && (NrSegmentMarker<=2 || !RecStrip_present || RecStrip_active)) || (MenuItem==MI_ClearAll && !((BookmarkMode && NrBookmarks>0) || (!BookmarkMode && (NrSegmentMarker>2 || NrSelectedSegments>0)))) || (MenuItem==MI_ImExportBookmarks && ((!BookmarkMode && NrBookmarks<=0) || (BookmarkMode && (NrSegmentMarker<=2 || MediaFileMode))) ) || (MenuItem==MI_RecStripCheckFS && ((!BookmarkMode && (!RecStrip_present||RecStrip_active)) || (BookmarkMode && !jfs_fsck_present))));
}

void ActionMenuDown(void)
{
  TRACEENTER();

  do
  {
    if(ActionMenuItem >= (MI_NrMenuItems - 1))
      ActionMenuItem = 1;
    else
      ActionMenuItem++;
  } while (ActionMenuItemInactive(ActionMenuItem));
  ActionMenuDraw();

  TRACEEXIT();
}

void ActionMenuUp(void)
{
  TRACEENTER();

  do
  {
    if(ActionMenuItem > 1)
      ActionMenuItem--;
    else
      ActionMenuItem = MI_NrMenuItems - 1;
  } while (ActionMenuItemInactive(ActionMenuItem));
  ActionMenuDraw();

  TRACEEXIT();
}

void ActionMenuRemove(void)
{
  TRACEENTER();

  if(rgnActionSubMenu)
  {
    TAP_Osd_Delete(rgnActionSubMenu);
    rgnActionSubMenu = 0;
  }
  if(rgnActionMenu)
  {
    TAP_Osd_Delete(rgnActionMenu);
    rgnActionMenu = 0;
  }
  OSDRedrawEverything();
//  TAP_Osd_Sync();

  TRACEEXIT();
}


// ----------------------------------------------------------------------------
//                            Playback-Funktionen
// ----------------------------------------------------------------------------
bool SetPlaybackSpeed(TYPE_TrickMode newTrickMode, byte newTrickModeSpeed)
{
  TRACEENTER();
//  HDD_TAP_PushDir();

  int i = 0;
  do {
    Appl_SetPlaybackSpeed(newTrickMode, newTrickModeSpeed, TRUE);
    if (i > 0) TAP_SystemProc();
    if (!isPlaybackRunning()) break;
    i++;
  } while ((i < 100) && (TrickMode != newTrickMode || TrickModeSpeed != newTrickModeSpeed));
//TAP_PrintNet("TrickMode=%d, Speed=%d - Iterationen: %d\n", newTrickMode, newTrickModeSpeed, i);

//  HDD_TAP_PopDir();
  TRACEEXIT();
  return (TrickMode == newTrickMode && TrickModeSpeed == newTrickModeSpeed);
}

void Playback_Faster(void)
{
  TRACEENTER();

  switch(TrickMode)
  {
    case TRICKMODE_Pause:
    {
      // 1/16xFWD
//      Playback_Slow();
      TrickModeSpeed = 3;
      TrickMode = TRICKMODE_Slow;
//      Appl_SetPlaybackSpeed(TRICKMODE_Slow, TrickModeSpeed, TRUE);
      break;
    }

    case TRICKMODE_Rewind:
    {
      if(TrickModeSpeed > 2)
      {
        // 64xRWD down to 2xRWD
        TrickModeSpeed -= 2;
//        Appl_SetPlaybackSpeed(TRICKMODE_Rewind, TrickModeSpeed, TRUE);
      }
      else
      {
        Playback_Normal();
        TRACEEXIT();
        return;
/*        // 1/16xFWD
        TrickModeSpeed = 4;
        Appl_SetPlaybackSpeed(TRICKMODE_Slow, TrickModeSpeed, TRUE);
        TrickMode = TRICKMODE_Slow; */
      }
      break;
    }

    case TRICKMODE_Slow:
    {
      if (TrickModeSpeed > 1)
      {
        // 1/16xFWD up to 1/2xFWD
        TrickModeSpeed--;
//        Appl_SetPlaybackSpeed(TRICKMODE_Slow, TrickModeSpeed, TRUE);
      }
      else
      {
        Playback_Normal();
        TRACEEXIT();
        return;
      }
      break;
    }

    case TRICKMODE_Normal:
    {
      // 2xFWD
      Playback_FFWD();
      TRACEEXIT();
      return;
    }

    case TRICKMODE_Forward:
    {
      // 2xFWD to 64xFWD
      Playback_FFWD();
      TRACEEXIT();
      return;
    }
  }
//  Appl_SetPlaybackSpeed(TrickMode, TrickModeSpeed, TRUE);
//  isPlaybackRunning();
  SetPlaybackSpeed(TrickMode, TrickModeSpeed);
  OSDInfoDrawPlayIcons(FALSE, TRUE);

  TRACEEXIT();
}

void Playback_Slower(void)
{
  TRACEENTER();

  switch(TrickMode)
  {
    case TRICKMODE_Forward:
    {
      if(TrickModeSpeed > 2)
      {
        // 64xFWD down to 2xFWD
        TrickModeSpeed -= 2;
//        Appl_SetPlaybackSpeed(TRICKMODE_Forward, TrickModeSpeed, TRUE);
      }
      else
      {
        // 1xFWD
        Playback_Normal();
        TRACEEXIT();
        return;
      }
      break;
    }

    case TRICKMODE_Normal:
    {
      Playback_RWD();
      // 1/2xFWD
//      Playback_Slow();
      TRACEEXIT();
      return;
    }

    case TRICKMODE_Slow:
    {
      // 1/2xFWD to 1/16xFWD
      if(TrickModeSpeed < 3)
      {
        TrickModeSpeed++;
//        Appl_SetPlaybackSpeed(TRICKMODE_Slow, TrickModeSpeed, TRUE);
      }
      else
      {
        // 1xRWD
//        TrickModeSpeed = 0;
//        Playback_RWD();
        TRACEEXIT();
        return;
      }
      break;
    }

    case TRICKMODE_Pause:
    {
      // 2xRWD
      Playback_RWD();
      TRACEEXIT();
      return;
    }

    case TRICKMODE_Rewind:
    {
      // 2xRWD down to 64xRWD
      Playback_RWD();
      TRACEEXIT();
      return;
    }
  }
//  Appl_SetPlaybackSpeed(TrickMode, TrickModeSpeed, TRUE);
//  isPlaybackRunning();
  SetPlaybackSpeed(TrickMode, TrickModeSpeed);
  OSDInfoDrawPlayIcons(FALSE, TRUE);

  TRACEEXIT();
}

void Playback_Normal(void)
{
  TRACEENTER();

  TrickMode = TRICKMODE_Normal;
  TrickModeSpeed = 1;
  Appl_SetPlaybackSpeed(TrickMode, TrickModeSpeed, TRUE);
  isPlaybackRunning();
//  SetPlaybackSpeed(TrickMode, TrickModeSpeed);
  OSDInfoDrawPlayIcons(FALSE, TRUE);

  TRACEEXIT();
}

void Playback_Pause(void)
{
  TRACEENTER();

  TrickMode = TRICKMODE_Pause;
  TrickModeSpeed = 0;
  Appl_SetPlaybackSpeed(TrickMode, TrickModeSpeed, TRUE);
  isPlaybackRunning();
//  SetPlaybackSpeed(TrickMode, TrickModeSpeed);
  OSDInfoDrawPlayIcons(FALSE, TRUE);

  TRACEEXIT();
}

void Playback_FFWD(void)
{
  TRACEENTER();

  //Appl_SetPlaybackSpeed(1, 1, true) 2x FFWD
  //Appl_SetPlaybackSpeed(1, 2, true) 4x FFWD
  //Appl_SetPlaybackSpeed(1, 3, true) 8x FFWD
  //Appl_SetPlaybackSpeed(1, 4, true) 16x FFWD
  //Appl_SetPlaybackSpeed(1, 5, true) 32x FFWD
  //Appl_SetPlaybackSpeed(1, 6, true) 64x FFWD

  if (TrickMode != TRICKMODE_Forward) 
    TrickModeSpeed = 0;

  if(TrickModeSpeed < 6)
  {
    TrickMode = TRICKMODE_Forward;
    TrickModeSpeed++;
    if (TrickModeSpeed < 6) TrickModeSpeed++;
//    Appl_SetPlaybackSpeed(TrickMode, TrickModeSpeed, TRUE);
//    isPlaybackRunning();
    SetPlaybackSpeed(TrickMode, TrickModeSpeed);
    OSDInfoDrawPlayIcons(FALSE, TRUE);
  }
  else
    Playback_Normal();

  TRACEEXIT();
}

void Playback_RWD(void)
{
  TRACEENTER();

  //Appl_SetPlaybackSpeed(2, 1, true) 2x RWD
  //Appl_SetPlaybackSpeed(2, 2, true) 4x RWD
  //Appl_SetPlaybackSpeed(2, 3, true) 8x RWD
  //Appl_SetPlaybackSpeed(2, 4, true) 16x RWD
  //Appl_SetPlaybackSpeed(2, 5, true) 32x RWD
  //Appl_SetPlaybackSpeed(2, 6, true) 64x RWD

  if (TrickMode != TRICKMODE_Rewind)
    TrickModeSpeed = 0;

  if(TrickModeSpeed < 6)
  {
    TrickMode = TRICKMODE_Rewind;
    TrickModeSpeed++;
    if (TrickModeSpeed < 6) TrickModeSpeed++;
//    Appl_SetPlaybackSpeed(TrickMode, TrickModeSpeed, TRUE);
//    isPlaybackRunning();
    SetPlaybackSpeed(TrickMode, TrickModeSpeed);
    OSDInfoDrawPlayIcons(FALSE, TRUE);
  }
  else
    Playback_Normal();

  TRACEEXIT();
}

void Playback_Slow(void)
{
  TRACEENTER();

  //Appl_SetPlaybackSpeed(3, 1, true) 1/2x Slow
  //Appl_SetPlaybackSpeed(3, 2, true) 1/4x Slow
  //Appl_SetPlaybackSpeed(3, 3, true) 1/8x Slow
  //Appl_SetPlaybackSpeed(3, 4, true) 1/16x Slow

  if (TrickMode != TRICKMODE_Slow)
    TrickModeSpeed = 1;
  else
    if(TrickModeSpeed < 3) TrickModeSpeed++;
    else TrickModeSpeed = 1;
  TrickMode = TRICKMODE_Slow;
//  Appl_SetPlaybackSpeed(TrickMode, TrickModeSpeed, TRUE);
//  isPlaybackRunning();
  SetPlaybackSpeed(TrickMode, TrickModeSpeed);
  OSDInfoDrawPlayIcons(FALSE, TRUE);

  TRACEEXIT();
}

void Playback_SetJumpNavigate(bool pJumpRequest, bool pNavRequest, bool pBackwards)
{
  static const dword    FastNavSteps[] = {12, 8, 4, 2, 1, 0, 1, 2, 4, 8, 12};
  static const int      NrFastNavSteps = 5;
  static int            LastDirection = 0;
  static dword          LastNavTime = 0;

  TRACEENTER();

  if (pJumpRequest && (NrSegmentMarker <= 2))
  {
    TRACEEXIT();
    return;
  }

  // Die jeweils andere Anforderung zurücksetzen
  if (!pNavRequest)
    JumpRequestedBlock = (dword) -1;

  if (!pJumpRequest)
  {
    if (JumpRequestedSegment != 0xFFFF)
    {
      JumpRequestedSegment = 0xFFFF;
      OSDSegmentListDrawList(FALSE);
    }
  }

  // neues Jump-Segment setzen
  if (pJumpRequest)
  {
    if (JumpRequestedSegment == 0xFFFF)
      JumpRequestedSegment = ActiveSegment;

    if (pBackwards)
    {
      if (JumpRequestedSegment > 0)
        JumpRequestedSegment--;
      else
        JumpRequestedSegment = NrSegmentMarker - 2;
    }
    else
    {
      if (JumpRequestedSegment < (NrSegmentMarker - 2))
        JumpRequestedSegment++;
      else
        JumpRequestedSegment = 0;
    }
    OSDSegmentListDrawList(FALSE);
  }
  
  // neue Navigations-Position setzen
  if (pNavRequest)
  {
    dword BlockJumpWidth;

    if (JumpRequestedBlock == (dword) -1)
      JumpRequestedBlock = PlayInfo.currentBlock;

    if (labs(TAP_GetTick() - LastNavTime) >= 500)
      LastDirection = 0;

    if (pBackwards)
    {
      if (LastDirection <= 0)
        LastDirection = max(LastDirection - 1, -NrFastNavSteps);
      else
        LastDirection = min(-LastDirection + 1, -1);
      BlockJumpWidth = (((float)PlayInfo.totalBlock / 100.0) * FastNavSteps[LastDirection + NrFastNavSteps]);

      if (JumpRequestedBlock >= BlockJumpWidth)
        JumpRequestedBlock = JumpRequestedBlock - BlockJumpWidth;
      else
        JumpRequestedBlock = 0;
    }
    else
    {
      if (LastDirection >= 0)
        LastDirection = min(LastDirection + 1, NrFastNavSteps);
      else
        LastDirection = max(-LastDirection - 1, 1);
      BlockJumpWidth = (((float)PlayInfo.totalBlock / 100.0) * FastNavSteps[LastDirection + NrFastNavSteps]);

      if (JumpRequestedBlock + BlockJumpWidth <= BlockNrLast10Seconds)
        JumpRequestedBlock = JumpRequestedBlock + BlockJumpWidth;
      else
        JumpRequestedBlock = (PlayInfo.currentBlock < BlockNrLast10Seconds) ? BlockNrLast10Seconds : (dword) -1;
    }
    LastNavTime = TAP_GetTick();
  }

  // Zeit der Aktion erfassen
  if (pJumpRequest || JumpRequestedBlock != (dword) -1)
  {
    JumpRequestedTime = TAP_GetTick();
    if (!JumpRequestedTime) JumpRequestedTime = 1;
  }

  // OSD aktualisieren
  OSDInfoDrawProgressbar(TRUE, TRUE);
  if (OSDMode == MD_NoOSD)
  {
    LastPlayStateChange = TAP_GetTick();
    if(!LastPlayStateChange) LastPlayStateChange = 1;
    OSDMode = MD_MiniOSD;
    OSDRedrawEverything();
    OSDMode = MD_NoOSD;
  }
  TRACEEXIT();
}

void Playback_JumpForward(void)
{
  dword                 NrJumpBlocks, JumpToBlock;

  TRACEENTER();
//  if(PLAYINFOVALID())  // Prüfung von currentBlock nun restriktiver
  if ((PlayInfo.currentBlock < BlockNrLastSecond) && (60*PlayInfo.duration + PlayInfo.durationSec != 0))
  {
    NrJumpBlocks = (PlayInfo.totalBlock / (60*PlayInfo.duration + PlayInfo.durationSec)) * MinuteJump*60;
    JumpToBlock  = min(PlayInfo.currentBlock + NrJumpBlocks, BlockNrLastSecond);

    if(TrickMode == TRICKMODE_Pause) Playback_Normal();
    TAP_Hdd_ChangePlaybackPos(JumpToBlock);
    JumpRequestedSegment = 0xFFFF;
    JumpRequestedBlock = (dword) -1;
    if (OSDMode == MD_NoOSD)
      OSDInfoDrawPlayIcons(TRUE, TRUE);
  }
  TRACEEXIT();
}

void Playback_JumpBackward(void)
{
  dword                 NrJumpBlocks = 0, JumpToBlock;

  TRACEENTER();
//  if(PLAYINFOVALID())  // Prüfung von currentBlock nun restriktiver
  if (60*PlayInfo.duration + PlayInfo.durationSec != 0)
    NrJumpBlocks = (PlayInfo.totalBlock / (60*PlayInfo.duration + PlayInfo.durationSec)) * MinuteJump*60;
  if (((int)PlayInfo.currentBlock >= 0) && (PlayInfo.currentBlock >= NrJumpBlocks))
  {
    JumpToBlock = PlayInfo.currentBlock - NrJumpBlocks;

    if(TrickMode == TRICKMODE_Pause) Playback_Normal();
    TAP_Hdd_ChangePlaybackPos(JumpToBlock);
    JumpRequestedSegment = 0xFFFF;
    JumpRequestedBlock = (dword) -1;
    if (OSDMode == MD_NoOSD)
      OSDInfoDrawPlayIcons(TRUE, TRUE);
  }
  TRACEEXIT();
}

void Playback_JumpNextSegment(void)
{
  TRACEENTER();

  if(NrSegmentMarker > 2)
  {
    if (ActiveSegment < NrSegmentMarker - 2)
      ActiveSegment++;
    else
      ActiveSegment = 0;

    if(TrickMode == TRICKMODE_Pause) Playback_Normal();
    TAP_Hdd_ChangePlaybackPos(SegmentMarker[ActiveSegment].Block);
    JumpRequestedSegment = 0xFFFF;
    JumpRequestedBlock = (dword) -1;
//    JumpRequestedTime = 0;
    JumpPerformedTime = TAP_GetTick();
    if(!JumpPerformedTime) JumpPerformedTime = 1;

    OSDSegmentListDrawList(TRUE);
    if (OSDMode == MD_NoOSD)
      OSDInfoDrawPlayIcons(TRUE, TRUE);
  }

  TRACEEXIT();
}

void Playback_JumpPrevSegment(void)
{
  TRACEENTER();

  const dword ThreeSeconds = (60*PlayInfo.duration + PlayInfo.durationSec != 0) ? (PlayInfo.totalBlock * 3 / (60*PlayInfo.duration + PlayInfo.durationSec)) : 0;

  if(NrSegmentMarker >= 2)
  {
    if (PlayInfo.currentBlock < (SegmentMarker[ActiveSegment].Block + ThreeSeconds))
    {
      if (ActiveSegment > 0)
        ActiveSegment--;
      else
        ActiveSegment = NrSegmentMarker - 2;
    }

    if(TrickMode == TRICKMODE_Pause) Playback_Normal();
    TAP_Hdd_ChangePlaybackPos(SegmentMarker[ActiveSegment].Block);
    JumpRequestedSegment = 0xFFFF;
    JumpRequestedBlock = (dword) -1;
//    JumpRequestedTime = 0;
    JumpPerformedTime = TAP_GetTick();
    if(!JumpPerformedTime) JumpPerformedTime = 1;

    OSDSegmentListDrawList(TRUE);
    if (OSDMode == MD_NoOSD)
      OSDInfoDrawPlayIcons(TRUE, TRUE);
  }

  TRACEEXIT();
}

void Playback_JumpNextBookmark(void)
{
  int i;

  TRACEENTER();

  if ((NrBookmarks > 0) && (PlayInfo.currentBlock > Bookmarks[NrBookmarks-1]))
  {
    if(TrickMode == TRICKMODE_Pause) Playback_Normal();
    TAP_Hdd_ChangePlaybackPos(0);   // Bookmarks[0]
    JumpRequestedSegment = 0xFFFF;
    JumpRequestedBlock = (dword) -1;
    if (OSDMode == MD_NoOSD)
      OSDInfoDrawPlayIcons(TRUE, TRUE);
    TRACEEXIT();
    return;
  }
  
  for(i = 0; i < NrBookmarks; i++)
  {
    if(Bookmarks[i] > PlayInfo.currentBlock)
    {
      if(TrickMode == TRICKMODE_Pause) Playback_Normal();
      TAP_Hdd_ChangePlaybackPos(Bookmarks[i]);
      JumpRequestedSegment = 0xFFFF;
      JumpRequestedBlock = (dword) -1;
      if (OSDMode == MD_NoOSD)
        OSDInfoDrawPlayIcons(TRUE, TRUE);
      TRACEEXIT();
      return;
    }
  }

  TRACEEXIT();
}

void Playback_JumpPrevBookmark(void)
{
  TRACEENTER();

  const dword           ThreeSeconds = (60*PlayInfo.duration + PlayInfo.durationSec != 0) ? (PlayInfo.totalBlock * 3 / (60*PlayInfo.duration + PlayInfo.durationSec)) : 0;
  dword                 JumpToBlock = PlayInfo.currentBlock;
  int                   i;

  if (NrBookmarks == 0)
    JumpToBlock = 0;
  else if (PlayInfo.currentBlock < Bookmarks[0] + ThreeSeconds)
  {
    if (PlayInfo.currentBlock >= ThreeSeconds)
      JumpToBlock = 0;
    else
      JumpToBlock = Bookmarks[NrBookmarks - 1];
  }
  else
    for(i = NrBookmarks - 1; i >= 0; i--)
    {
      if((Bookmarks[i] + ThreeSeconds) <= PlayInfo.currentBlock)
      {
        JumpToBlock = Bookmarks[i];
        break;
      }
    }
  if(TrickMode == TRICKMODE_Pause) Playback_Normal();
  TAP_Hdd_ChangePlaybackPos(JumpToBlock);
  JumpRequestedSegment = 0xFFFF;
  JumpRequestedBlock = (dword) -1;
  if (OSDMode == MD_NoOSD)
    OSDInfoDrawPlayIcons(TRUE, TRUE);

  TRACEEXIT();
}


// ----------------------------------------------------------------------------
//                        MovieCutter-Funktionalität
// ----------------------------------------------------------------------------
void MovieCutterSaveSegments(void)
{
  TRACEENTER();

  if (NrSegmentMarker > 2)
  {
    WriteLogMC(PROGRAM_NAME, "[Action 'Save segments' started...]");
    MovieCutterProcess(TRUE, FALSE);
  }

  TRACEEXIT();
}

void MovieCutterDeleteSegments(void)
{
  TRACEENTER();

  if (NrSegmentMarker > 2)
  {
    WriteLogMC(PROGRAM_NAME, "[Action 'Delete segments' started...]");
    MovieCutterProcess(FALSE, FALSE);
  }

  TRACEEXIT();
}

void MovieCutterSplitMovie(void)
{
  dword newBlock = PlayInfo.currentBlock;
  TRACEENTER();

  if (PlayInfo.currentBlock > 0)
  {
    WriteLogMC(PROGRAM_NAME, "[Action 'Split movie' started...]");
    if (AddSegmentMarker(&newBlock, TRUE, FALSE) > 0)
      MovieCutterProcess(TRUE, TRUE);
  }
  TRACEEXIT();
}

void MovieCutterSelectEvOddSegments(void)
{
  if (NrSegmentMarker <= 2) return;

  TRACEENTER();

  int i;
  for(i = 0; i < NrSegmentMarker-1; i++)
    SegmentMarker[i].Selected = ((i & 1) == !ActionMenuEvenOdd);

  OSDSegmentListDrawList(FALSE);
  OSDInfoDrawProgressbar(TRUE, TRUE);
//  OSDRedrawEverything();

  if (DirectSegmentsCut)
  {
    WriteLogMC(PROGRAM_NAME, "[Action 'Delete even/odd segments' selected...]");
    ActionMenuDraw();
    if (!AskBeforeEdit || ShowConfirmationDialog(LangGetString(LS_AskConfirmation)))
    {
      ActionMenuRemove();
      MovieCutterDeleteSegments();
      TRACEEXIT();
      return;
    }
  }

  State = ST_ActionMenu;
  ActionMenuEvenOdd = !ActionMenuEvenOdd;
  ActionMenuDraw();
  TRACEEXIT();
}

void MovieCutterUnselectAll(void)
{
  TRACEENTER();

  int i;
  for(i = 0; i < NrSegmentMarker-1; i++)
    SegmentMarker[i].Selected = FALSE;

  OSDSegmentListDrawList(FALSE);
  OSDInfoDrawProgressbar(TRUE, TRUE);
//  OSDRedrawEverything();

  if (NrSegmentMarker > 2)
  {
    State = ST_ActionMenu;
    ActionMenuDraw();
  }
  TRACEEXIT();
}

void MovieCutterDeleteFile(void)
{
  TRACEENTER();

//  NoPlaybackCheck = FALSE;
  if (isPlaybackRunning())
  {
//    NoPlaybackCheck = TRUE;
    TAP_Hdd_StopTs();
  }
//  HDD_ChangeDir(PlaybackDir);
//  NoPlaybackCheck = TRUE;
  CutFileDelete();
  HDD_Delete2(PlaybackName, AbsPlaybackDir, TRUE);
//  NoPlaybackCheck = FALSE;

  Cleanup(TRUE);
  State = AutoOSDPolicy ? ST_WaitForPlayback : ST_InactiveMode;

  TRACEEXIT();
}

bool MovieCutterRenameFile(void)
{
  char                  NewName[MAX_FILE_NAME_SIZE], TempNameISO[MAX_FILE_NAME_SIZE];
  char                 *ExtensionStart, *p;
  dword                 LastPlaybackPos;
  int                   i;
  bool                  ret = FALSE;

  TRACEENTER();
  HDD_TAP_PushDir();
  WriteLogMCf(PROGRAM_NAME, "[Action 'Rename movie' started...]");

#if STACKTRACE == TRUE
  TAP_PrintNet("DEBUG: VOR der Umwandlung in ISO:\n");
  TAP_PrintNet("DEBUG: PlaybackName = [%x] '%s'\n", (PlaybackName[0] >= 0x20 ? 0 : PlaybackName[0]), PlaybackName);
#endif

  ExtensionStart = strrchr(PlaybackName, '.');
  if (!ExtensionStart) ExtensionStart = "";
  if (isUTFToppy() && PlaybackName[0] >= 0x20)
    StrToISO(PlaybackName, TempNameISO);
  else
    strcpy(TempNameISO, PlaybackName);
  p = strrchr(TempNameISO, '.');
  if(p) p[0] = '\0';

#if STACKTRACE == TRUE
  TAP_PrintNet("DEBUG: NACH der Umwandlung in ISO:\n");
  TAP_PrintNet("DEBUG: TempNameISO  = [%x] '%s'\n", (TempNameISO[0] >= 0x20 ? 0 : TempNameISO[0]), TempNameISO);
#endif

  while (!ret)
  {
    // OSD-Tastatur anzeigen
    OSDMenuKeyboard_Setup(LangGetString(LS_RenameMovie), TempNameISO, sizeof(TempNameISO) - strlen(ExtensionStart) - 4 - 1, FALSE);
    #ifdef MC_MULTILANG
      if (LangStrings)
        OSDMenuKeyboard_SetLegendStrings(LangGetString(LS_KeybSpace), LangGetString(LS_KeybDelete1), LangGetString(LS_KeybDelete2), LangGetString(LS_KeybOriginal), LangGetString(LS_KeybClearAll), LangGetString(LS_KeybCopy), LangGetString(LS_KeybPaste), LangGetString(LS_KeybCancel), LangGetString(LS_KeybSave));
    #endif
    OSDMenuKeyboard_Show();
    while (OSDMenuKeyboard_isVisible())
    {
      TAP_SystemProc();
      TAP_Sleep(1);
    }
    OSDRedrawEverything();

    // Eingabe überprüfen und ggf. wieder in UTF-8 umwandeln
    if ((TempNameISO[0] >= 0x20) || (TempNameISO[0] && TempNameISO[1]))
    {
      // Wenn Name ungültig, Meldung anzeigen
      if (strchr(TempNameISO, '/'))
        ShowErrorMessage(LangGetString(LS_NameInvalid), NULL);
      else
      {
        char            TempNameBuffer[MAX_FILE_NAME_SIZE + 1]; TempNameBuffer[0] = '\5';
        char           *TempNameFull;
        char           *PlayNameNoTypeChar, *TempNameNoTypeChar;  // *TempNameWithTypeChar;

#if STACKTRACE == TRUE
  TAP_PrintNet("DEBUG: VOR Rück-Umwandlung in UTF-8:\n");
  TAP_PrintNet("DEBUG: TempNameISO  = [%x] '%s'\n", (TempNameISO[0] >= 0x20 ? 0 : TempNameISO[0]), TempNameISO);
#endif

        TempNameFull = (TempNameISO[0] >= 0x20) ? &TempNameBuffer[1] : TempNameBuffer;
        strcpy(TempNameFull, TempNameISO);
        if (isUTFToppy() && TempNameISO[0] >= 0x20)
          ConvertUTFStr(NewName, TempNameFull, sizeof(NewName) - strlen(ExtensionStart) - 4, TRUE);
        else
          strcpy(NewName, TempNameFull);  // TempNameBuffer wenn ISO-Dateiname immer mit TypeChar (0x05) beginnen soll
        strcat(NewName, ExtensionStart);
        strcat(TempNameFull, ExtensionStart);

        PlayNameNoTypeChar = (PlaybackName[0] >= 0x20) ? PlaybackName : &PlaybackName[1];
        TempNameNoTypeChar = &TempNameBuffer[1];  // (TempNameFull[0] >= 0x20) ? TempNameFull : &TempNameFull[1];
//        TempNameWithTypeChar = (TempNameFull[0] >= 0x20) ? TempNameBuffer : TempNameFull;

#if STACKTRACE == TRUE
  TAP_PrintNet("DEBUG: NACH der Rück-Umwandlung in UTF-8:\n");
  TAP_PrintNet("DEBUG: TempNameBuf  = [%x] '%s'\n", (TempNameBuffer[0] >= 0x20 ? 0 : TempNameBuffer[0]), TempNameBuffer);
  TAP_PrintNet("DEBUG: TempNameFull = [%x] '%s'\n", (TempNameFull[0]   >= 0x20 ? 0 : TempNameFull[0]),   TempNameFull);
  TAP_PrintNet("DEBUG: NewName      = [%x] '%s'\n", (NewName[0]        >= 0x20 ? 0 : NewName[0]),        NewName);
#endif

        // Wenn neuer Name nicht leer, aber verändert
        if (*NewName && (strcmp(NewName, PlaybackName) != 0) /*&& (strcmp(TempNameNoTypeChar, PlayNameNoTypeChar) != 0)*/)
        {
          // ggf. Rückfrage, ob existierende Datei überschrieben werden soll
          if ((!HDD_Exist2(NewName, AbsPlaybackDir) && !HDD_Exist2(TempNameBuffer, AbsPlaybackDir) && !HDD_Exist2(TempNameNoTypeChar, AbsPlaybackDir))
            || ShowConfirmationDialog(LangGetString(LS_NameAlreadyExists)))
              ret = TRUE;
        }
        else
          break;
      }
    }
    else
      break;
  }

  // Jetzt kann die Aufnahme umbenannt werden...
  if (ret)
  {
    WriteLogMCf(PROGRAM_NAME, "New file name: %s", NewName);

    // Playback stoppen
    LastPlaybackPos = PlayInfo.currentBlock;
    if (isPlaybackRunning())
      TAP_Hdd_StopTs();

    // Umbenennen
    HDD_Delete2(NewName, AbsPlaybackDir, TRUE);
    HDD_Rename2(PlaybackName, NewName, AbsPlaybackDir, TRUE, TRUE);

    char OldCutBak[FBLIB_DIR_SIZE+4], NewCutBak[FBLIB_DIR_SIZE+4];
    GetCutNameFromRec(PlaybackName, AbsPlaybackDir, OldCutBak); strcat(OldCutBak, ".bak");
    GetCutNameFromRec(NewName, AbsPlaybackDir, NewCutBak); strcat(NewCutBak, ".bak");
    rename(PlaybackName, NewName);

    // Playback wieder starten
    ret = HDD_Exist2(NewName, AbsPlaybackDir);
    if (ret)
    {
      WriteLogMC(PROGRAM_NAME, "Renaming successful!");
      HDD_StartPlayback2(NewName, AbsPlaybackDir, MediaFileMode);
    }
    else
    {
      WriteLogMC(PROGRAM_NAME, "Renaming has failed!");
      HDD_StartPlayback2(PlaybackName, AbsPlaybackDir, MediaFileMode);
    }

    // auf das Starten des Playbacks warten
    i = 0;
    do {
      TAP_SystemProc();
      i++;
    } while ((i < 2000) && (!isPlaybackRunning() || (int)PlayInfo.totalBlock <= 0 || (int)PlayInfo.currentBlock < 0));

    if (isPlaybackRunning())
    {
      PlaybackRepeatSet(TRUE);
      if(LastPlaybackPos >= 1000)
        TAP_Hdd_ChangePlaybackPos(LastPlaybackPos);
    }

    // neuen Namen übernehmen
    if (ret && isPlaybackRunning())
    {
      strcpy(PlaybackName, NewName);
//      OSDInfoDrawRecName();
      OSDRedrawEverything();
    }
    else
      ShowErrorMessage(LangGetString(LS_RenamingFailed), NULL);

    // Prüfen, ob das Playback wieder gestartet wurde
    if((int)PlayInfo.totalBlock <= 0)
    {
      WriteLogMC(PROGRAM_NAME, "MovieCutterRenameFile: Error restarting the playback!");
      State = ST_UnacceptedFile;
      LastTotalBlocks = PlayInfo.totalBlock;
      ClearOSD(TRUE);
      ret = FALSE;
    }
    else
    {
      if (SegmentMarker[NrSegmentMarker - 1].Block != PlayInfo.totalBlock)
      {
        #ifdef FULLDEBUG
          WriteLogMCf(PROGRAM_NAME, "MovieCutterRenameFile: Nach Playback-Restart neues TotalBlock %lu (vorher %lu)!", SegmentMarker[NrSegmentMarker - 1].Block, PlayInfo.totalBlock);
        #endif
        SegmentMarker[NrSegmentMarker - 1].Block = PlayInfo.totalBlock;
      }
    }
  }
  HDD_TAP_PopDir();
  TRACEEXIT();
  return ret;
}

void MovieCutterProcess(bool KeepCut, bool SplitMovie)  // Splittet am linken SegmentMarker (ActiveSegment)
{
//  int                   NrSelectedSegments;
  bool                  isMultiSelect, CutEnding;
  int                   WorkingSegment;
  int                   maxProgress; 
  char                  CutFileName[MAX_FILE_NAME_SIZE];
  char                  TempFileName[MAX_FILE_NAME_SIZE];
  char                  MessageString[512];
  char                 *pCutCaption, *pSourceCaption;
  tTimeStamp            CutStartPoint, BehindCutPoint;
  dword                 DeltaBlock, DeltaTime;
  dword                 CurPlayPosition;
  char                  InodeNrs[768];
  __ino64_t             InodeNr = 0;
  int                   icheckErrors = -2;
  int                   i, j;
  tResultCode           ret = RC_Error;

  TRACEENTER();

//  NoPlaybackCheck = TRUE;
  #ifdef FULLDEBUG
    char CurDir[FBLIB_DIR_SIZE];
    HDD_TAP_GetCurrentDir(CurDir);
    WriteLogMCf(PROGRAM_NAME, "MovieCutterProcess(KeepCut=%d, SplitMovie=%d): CurrentDir='%s'", KeepCut, SplitMovie, CurDir);
  #endif
//  HDD_ChangeDir(PlaybackDir);

  UndoResetStack();
  CutDumpList();
  CutSaveToBM(FALSE);

  // Lege ein Backup der .cut-Datei an
  if (SaveCutBak && (CutFileMode != CM_InfOnly))
  {
    CutFileSave();
    char AbsCutName[FBLIB_DIR_SIZE], AbsBackupCutName[FBLIB_DIR_SIZE];
    GetCutNameFromRec(PlaybackName, AbsPlaybackDir, AbsCutName);
    TAP_SPrint(AbsBackupCutName, sizeof(AbsBackupCutName), "%s.bak", AbsCutName);    
//    if (HDD_Exist2(&AbsBackupCutName[1], "")) HDD_Delete2(&AbsBackupCutName[1], "", FALSE);
    rename(AbsCutName, AbsBackupCutName);
  }
  CutFileSave();
//  TAP_SPrint(CommandLine, sizeof(CommandLine), "cp \"%s\" \"%s.bak\"", AbsCutName, AbsCutName);
//  system(CommandLine);

  // Zähle die ausgewählten Segmente
  CountSelectedSegments();
  isMultiSelect = (!SplitMovie && (NrSelectedSegments > 0));
  if (!isMultiSelect) NrSelectedSegments = 1;

  maxProgress = NrSelectedSegments;
  OSDMenuSaveMyRegion(rgnSegmentList);
  TAP_SPrint(MessageString, sizeof(MessageString), LangGetString(LS_Cutting), 0, maxProgress);
  OSDMenuProgressBarShow(PROGRAM_NAME, MessageString, 0, maxProgress + ((CheckFSAfterCut==FM_Always) ? 1 : 0), NULL);
  CurPlayPosition = PlayInfo.currentBlock;

// Aufnahmenfresser-Test und Ausgabe
if (DoiCheckTest >= 3)
{
  icheckErrors = 0;
  if (!HDD_CheckInode(PlaybackName, AbsPlaybackDir, FALSE, InodeMonitoring /*, "Original" */))
    icheckErrors++;
}
if (HDD_GetFileSizeAndInode2(PlaybackName, AbsPlaybackDir, &InodeNr, NULL))
  TAP_SPrint(InodeNrs, sizeof(InodeNrs), "%llu", InodeNr);

  for(i = NrSegmentMarker - 2; i >= 0 /*-1*/; i--)
  {
    if(isMultiSelect)
    {
      //If one or more segments have been selected, work with them.
      WorkingSegment = i;

      // Process the ending at last (*experimental!*)
/*      if (i == NrSegmentMarker - 2) {
        if (SegmentMarker[i].Selected) NrSelectedSegments--;
        continue;
      }
      if (i == -1)
      {
        WorkingSegment = NrSegmentMarker - 2;
        NrSelectedSegments = 1;
      }  */
    }
    else
      //If no segment has been selected, use the active segment and break the loop
      WorkingSegment = ActiveSegment;

    if(!isMultiSelect || SegmentMarker[WorkingSegment].Selected)
    {
//      NoPlaybackCheck = FALSE;
      if (isPlaybackRunning())
      {
//        NoPlaybackCheck = TRUE;
        TAP_Hdd_StopTs();
      }
//      NoPlaybackCheck = TRUE;
//      HDD_ChangeDir(PlaybackDir);

      WriteLogMCf(PROGRAM_NAME, "Processing segment %d", WorkingSegment);

      // Ermittlung der Schnittpositionen
      CutStartPoint.BlockNr  = SegmentMarker[WorkingSegment].Block;
      CutStartPoint.Timems   = SegmentMarker[WorkingSegment].Timems;
      BehindCutPoint.BlockNr = SegmentMarker[SplitMovie ? NrSegmentMarker-1 : WorkingSegment+1].Block;
      BehindCutPoint.Timems  = SegmentMarker[SplitMovie ? NrSegmentMarker-1 : WorkingSegment+1].Timems;
      pCutCaption =  KeepCut ? SegmentMarker[WorkingSegment].pCaption : NULL;
      pSourceCaption         = NULL;
//      if ((NrSegmentMarker == 3) && !SplitMovie)
//        pSourceCaption = SegmentMarker[!WorkingSegment].pCaption;

      // NEUE Spezial-Behandlung beim Schneiden des Endes
      CutEnding = FALSE;
      if (SplitMovie || (WorkingSegment == NrSegmentMarker - 2))
      {
//        if (!DisableSpecialEnd && (ForceSpecialEnd || (KeepCut && CutStartPoint.BlockNr + 11 <= 475949) || (KeepCut && CutStartPoint.BlockNr + 475949 + 11 < BehindCutPoint.BlockNr)))
//        if (!DisableSpecialEnd && KeepCut && (ForceSpecialEnd ||  (CutStartPoint.BlockNr + 11 <= 475949) || (CutStartPoint.BlockNr + 475949 + 11 < BehindCutPoint.BlockNr) ))
        if (!DisableSpecialEnd && KeepCut && (ForceSpecialEnd || ((CutStartPoint.BlockNr + 11 <= 475949) && (CutStartPoint.BlockNr + 475949 - 11 < BehindCutPoint.BlockNr))))  // (statt -11 eigentlich -2*CUTPOINTSEARCHRADIUS/9024)
        {
          //letztes Segment soll geschnitten werden -> speichere stattdessen den vorderen Teil der Aufnahme und tausche mit dem Original
          CutEnding = TRUE;
          CutStartPoint.BlockNr  = 0;
          CutStartPoint.Timems   = 0;
          BehindCutPoint.BlockNr = SegmentMarker[WorkingSegment].Block;
          BehindCutPoint.Timems  = SegmentMarker[WorkingSegment].Timems;
          pCutCaption = pSourceCaption;
          pSourceCaption = SegmentMarker[WorkingSegment].pCaption;
          WriteLogMC(PROGRAM_NAME, "(* special mode for cut ending *)");
        }
        else if (KeepCut)
          BehindCutPoint.BlockNr = 0xFFFFFFFF;  //letztes Segment soll gespeichert werden -> versuche bis zum tatsächlichen Ende zu gehen
        else
          BehindCutPoint.BlockNr = 0xFFFFFFFF;  //Truncate Ending!
      }

      // Ermittlung des Dateinamens für das CutFile
      GetNextFreeCutName(PlaybackName, CutFileName, AbsPlaybackDir, NrSelectedSegments - 1);
      if (CutEnding)
      {
        TAP_SPrint(TempFileName, sizeof(TempFileName), PlaybackName);
        char *p = strrchr(TempFileName, '.');
        TAP_SPrint(((p) ? p : &TempFileName[strlen(TempFileName)]), 10, "_temp%s", ((p) ? &PlaybackName[p-TempFileName] : ""));
        HDD_Delete2(TempFileName, AbsPlaybackDir, TRUE);
      }

      //Flush the caches *experimental*
      sync();
      TAP_Sleep(1);
//      HDD_ChangeDir(PlaybackDir);

      // Schnittoperation
      ret = MovieCutter(PlaybackName, ((CutEnding) ? TempFileName : CutFileName), AbsPlaybackDir, &CutStartPoint, &BehindCutPoint, (KeepCut || CutEnding), HDVideo, pCutCaption, pSourceCaption);

      if (HDD_GetFileSizeAndInode2(((CutEnding) ? TempFileName : CutFileName), AbsPlaybackDir, &InodeNr, NULL))
        TAP_SPrint(&InodeNrs[strlen(InodeNrs)], sizeof(InodeNrs)-strlen(InodeNrs), " %llu", InodeNr);

// Aufnahmenfresser-Test und Ausgabe
if (DoiCheckTest >= 3)
{
  if (ret && (KeepCut || CutEnding))
    if (!HDD_CheckInode(((CutEnding) ? TempFileName : CutFileName), AbsPlaybackDir, (DoiCheckTest==IM_RWBetween), InodeMonitoring /*, "CutFile" */))
      icheckErrors++;
  if (!HDD_CheckInode(PlaybackName, AbsPlaybackDir, (DoiCheckTest==IM_RWBetween), InodeMonitoring /*, "RestFile" */))
    icheckErrors++;
}
//if (!KeepCut && !CutEnding)
//  HDD_Delete2(CutFileName, AbsPlaybackDir, TRUE);  // wenn diese Zeile gelöscht wird, stattdessen das TRUE im MovieCutter-Aufruf wieder rausnehmen

// INFplus
if (KeepCut || CutEnding)
{
  __ino64_t OldInodeNr, NewInodeNr;
  char CommandLine[FBLIB_DIR_SIZE], InfFileName[MAX_FILE_NAME_SIZE];
  TAP_SPrint(InfFileName, sizeof(InfFileName), "%s.inf", PlaybackName);
  if (HDD_GetFileSizeAndInode2(InfFileName, AbsPlaybackDir, &OldInodeNr, NULL))
  {
    TAP_SPrint(InfFileName, sizeof(InfFileName), "%010llu.INF+", OldInodeNr);
    if (HDD_Exist2(InfFileName, TAPFSROOT "/ProgramFiles/Settings/INFplus"))
    {
      TAP_SPrint(InfFileName, sizeof(InfFileName), "%s.inf", (CutEnding) ? TempFileName : CutFileName);
      if (HDD_GetFileSizeAndInode2(InfFileName, AbsPlaybackDir, &NewInodeNr, NULL))
      {
        TAP_SPrint(CommandLine, sizeof(CommandLine), "%s %s/ProgramFiles/Settings/INFplus/%010llu.INF+ %s/ProgramFiles/Settings/INFplus/%010llu.INF+", (KeepCut) ? "cp" : "mv", TAPFSROOT, OldInodeNr, TAPFSROOT, NewInodeNr);
        system(CommandLine);
      }
    }
  }
}

      // Das erzeugte CutFile wird zum neuen SourceFile
      if (CutEnding)
      {
        RecFileSize = 0;
        if(HDD_Exist2(TempFileName, AbsPlaybackDir))
          if(!HDD_GetFileSizeAndInode2(TempFileName, AbsPlaybackDir, NULL, &RecFileSize))
            RecFileSize = 0;

        if (ret && RecFileSize > 0)
        {
          if (KeepCut)
          {
            WriteLogMCf(PROGRAM_NAME, "Renaming the end-segment file '%s' to '%s'", PlaybackName, CutFileName);
            HDD_Rename2(PlaybackName, CutFileName, AbsPlaybackDir, TRUE, FALSE);
          }
          else
            HDD_Delete2(PlaybackName, AbsPlaybackDir, TRUE);
          if (!HDD_Exist2(PlaybackName, AbsPlaybackDir))
          {
            WriteLogMCf(PROGRAM_NAME, "Renaming original recording '%s' back to '%s'", TempFileName, PlaybackName);
            HDD_Rename2(TempFileName, PlaybackName, AbsPlaybackDir, TRUE, FALSE);
          }
        }
      }

      // Überprüfung von Existenz und Größe der geschnittenen Aufnahme
      RecFileSize = 0;
      if(HDD_Exist2(PlaybackName, AbsPlaybackDir))
        if(!HDD_GetFileSizeAndInode2(PlaybackName, AbsPlaybackDir, NULL, &RecFileSize))
          RecFileSize = 0;
      WriteLogMCf(PROGRAM_NAME, "Size of the new playback file (after cut): %llu", RecFileSize);

      // Wiedergabe wird neu gestartet
      if (RecFileSize > 0)
      {
//        TAP_Hdd_PlayTs(PlaybackName);
        HDD_StartPlayback2(PlaybackName, AbsPlaybackDir, MediaFileMode);
        PlayInfo.totalBlock = 0;
        j = 0;
        while ((j < 2000) && (!isPlaybackRunning() || (int)PlayInfo.totalBlock <= 0 || (int)PlayInfo.currentBlock < 0))  // 2000 ~ 30 sek. (750 ~ 10 sek.)
        {
          TAP_SystemProc();
          j++;
        }
        #ifdef FULLDEBUG        
          WriteLogMCf(PROGRAM_NAME, "Playback re-started (j=%d, isPlaybackRunning=%d, TotalBlock=%lu, CurrentBlock=%lu)", j, isPlaybackRunning(), PlayInfo.totalBlock, PlayInfo.currentBlock);
        #endif
        if (PlayInfo.playMode == PLAYMODE_Playing || PlayInfo.playMode == 8)
          PlaybackRepeatSet(TRUE);
//        HDD_ChangeDir(PlaybackDir);

        WriteLogMCf(PROGRAM_NAME, "Reported new totalBlock = %lu", PlayInfo.totalBlock);
      }

      //Bail out if the cut failed
      if((ret == RC_Error) || (RecFileSize == 0) || ((int)PlayInfo.totalBlock <= 0))
      {
        WriteLogMC(PROGRAM_NAME, "MovieCutterProcess: Cutting process failed or totalBlock is 0!");
        State = ST_UnacceptedFile;
        OSDMenuProgressBarDestroyNoOSDUpdate();
        ShowErrorMessage(LangGetString(LS_CutHasFailed), NULL);
        break;
      }

//WriteLogMC("DEBUG", "Cut-List VOR Anpassung:");
//CutDumpList();

      // Anpassung der verbleibenden Segmente
      SegmentMarker[WorkingSegment].Selected = FALSE;
      if (WorkingSegment > 0)
        DeleteSegmentMarker(WorkingSegment, TRUE);  // das erste Segment kann nicht gelöscht werden
      else
      {
        if(SegmentMarker[0].pCaption) TAP_MemFree(SegmentMarker[0].pCaption);
        SegmentMarker[0].pCaption = SegmentMarker[1].pCaption;
        DeleteSegmentMarker(1, FALSE);
      }
      NrSelectedSegments--;

//      if (CutEnding) {
//        DeltaBlock = CalcBlockSize(CutFileSize);
//        DeltaTime = (!LinearTimeMode) ? (TimeStamps[NrTimeStamps-1].Timems - BehindCutPoint.Timems) : NavGetBlockTimeStamp(DeltaBlock);
//      } else {
//        DeltaBlock = BehindCutPoint.BlockNr - CutStartPoint.BlockNr;
//        DeltaTime = BehindCutPoint.Timems - CutStartPoint.Timems;
//      }
      if (CutEnding)
        CutStartPoint = BehindCutPoint;
      DeltaBlock = BehindCutPoint.BlockNr - CutStartPoint.BlockNr;
      DeltaTime = BehindCutPoint.Timems - CutStartPoint.Timems;

      if (SplitMovie)
      {
        // nachfolgende Segmente in neues CutFile verschieben
        int CutNrSegmentMarker = 1;
        tSegmentMarker *CutSegmentMarker = (tSegmentMarker*) TAP_MemAlloc(NRSEGMENTMARKER * sizeof(tSegmentMarker));
        if (CutSegmentMarker)
        {
          CutSegmentMarker[0].Block = 0;  CutSegmentMarker[0].Timems = 0;  CutSegmentMarker[0].Percent = 0;
          CutSegmentMarker[0].Selected = SegmentMarker[WorkingSegment - 1].Selected;
          CutSegmentMarker[0].pCaption = SegmentMarker[WorkingSegment - 1].pCaption;
        }

        for (j = 1; j < NrSegmentMarker; j++)
        {
          if (SegmentMarker[j].Block + 11 >= CutStartPoint.BlockNr)
          {
            if (CutSegmentMarker)
            {
              CutSegmentMarker[CutNrSegmentMarker].Block    = SegmentMarker[j].Block  - CutStartPoint.BlockNr;
              CutSegmentMarker[CutNrSegmentMarker].Timems   = SegmentMarker[j].Timems - min(SegmentMarker[j].Timems, CutStartPoint.Timems);
              CutSegmentMarker[CutNrSegmentMarker].Selected = SegmentMarker[j].Selected;
              CutSegmentMarker[CutNrSegmentMarker].Percent  = ((float)CutSegmentMarker[CutNrSegmentMarker].Block / (SegmentMarker[NrSegmentMarker-1].Block - CutStartPoint.BlockNr)) * 100.0;
              CutSegmentMarker[CutNrSegmentMarker].pCaption = SegmentMarker[j].pCaption;
              CutNrSegmentMarker++;
            }
            if (j == NrSegmentMarker - 1)
            {
              SegmentMarker[j].Block = CutStartPoint.BlockNr;
              SegmentMarker[j].Timems = CutStartPoint.Timems;
            }
            else
              if (DeleteSegmentMarker(j, FALSE))  // der letzte wird nicht gelöscht
                j--;
          }
          else
            SegmentMarker[j].Percent = ((float)SegmentMarker[j].Block / PlayInfo.totalBlock) * 100.0;
        }

        if (CutSegmentMarker)
        {
          CutFileSave2(CutSegmentMarker, CutNrSegmentMarker, CutFileName);
          CutSaveToInf(CutSegmentMarker, CutNrSegmentMarker, CutFileName);
        }
        else
          WriteLogMC(PROGRAM_NAME, "Error calculating new segments: No cut file for second part generated!");
        
        for (j = 1; j < CutNrSegmentMarker; j++)  // pCaption von 0 darf nicht freigegeben werden!
          if (CutSegmentMarker[j].pCaption)
            TAP_MemFree(CutSegmentMarker[j].pCaption);
        TAP_MemFree(CutSegmentMarker);
      }
      else
      {
        for(j = NrSegmentMarker - 1; j > 0; j--)
        {
          if(j == WorkingSegment)
          {
            // das aktuelle Segment auf die tatsächliche Schnittposition setzen
            SegmentMarker[WorkingSegment].Block  = CutStartPoint.BlockNr;
            SegmentMarker[WorkingSegment].Timems = CutStartPoint.Timems;
          }
          else if(SegmentMarker[j].Block + 11 >= BehindCutPoint.BlockNr)
          {
            // nachfolgende Semente verschieben
            SegmentMarker[j].Block -= DeltaBlock;
            SegmentMarker[j].Timems -= min(DeltaTime, SegmentMarker[j].Timems);  // NavGetBlockTimeStamp(SegmentMarker[j].Block)
          }
          SegmentMarker[j].Percent = ((float)SegmentMarker[j].Block / PlayInfo.totalBlock) * 100.0;
        }
      }

      // das letzte Segment auf den gemeldeten TotalBlock-Wert prüfen
      if(SegmentMarker[NrSegmentMarker-1].Block != PlayInfo.totalBlock)
      {
        // PROBLEM!! nav ist nicht neu geladen... -> darf nur beim letzten Segment aus der nav gelesen werden. (eigentlich sowieso unnötig)
        if (SplitMovie || (WorkingSegment == NrSegmentMarker - 1))
        {
          #ifdef FULLDEBUG
            WriteLogMCf(PROGRAM_NAME, "MovieCutterProcess: Korrigiere letzten Segment-Marker von %lu (%lu) zu TotalBlock %lu (%lu)!", SegmentMarker[NrSegmentMarker-1].Block, SegmentMarker[NrSegmentMarker-1].Timems, PlayInfo.totalBlock, NavGetBlockTimeStamp(PlayInfo.totalBlock));
          #endif
          SegmentMarker[NrSegmentMarker-1].Block = PlayInfo.totalBlock;
          dword newTime = NavGetBlockTimeStamp(SegmentMarker[NrSegmentMarker-1].Block);
          SegmentMarker[NrSegmentMarker-1].Timems = (newTime) /*? newTime : (dword)(1000 * (60*PlayInfo.duration + PlayInfo.durationSec))*/;
          SegmentMarker[NrSegmentMarker-1].Percent = 100.0;
        }
        else
        {
          #ifdef FULLDEBUG
            WriteLogMCf(PROGRAM_NAME, "MovieCutterProcess: Letzter Segment-Marker %lu (%lu) ist ungleich TotalBlock %lu!", SegmentMarker[NrSegmentMarker-1].Block, SegmentMarker[NrSegmentMarker-1].Timems, PlayInfo.totalBlock);
          #endif
        }
      }

//WriteLogMC("DEBUG", "Cut-List NACH Anpassung:");
CutDumpList();

      // Letzte Playback-Position anpassen
      if(CurPlayPosition)
      {
//        if (!CutEnding)
        {
          if (CurPlayPosition >= BehindCutPoint.BlockNr)
            CurPlayPosition -= DeltaBlock;
          else if (CurPlayPosition >= CutStartPoint.BlockNr)
            CurPlayPosition = /*(WorkingSegment < NrSegmentMarker - 1) ?*/ CutStartPoint.BlockNr;
          CalcLastSeconds();
          if (CurPlayPosition > BlockNrLastSecond)
            CurPlayPosition = 0;
        }
//        else
//          if (CurPlayPosition >= BehindCutPoint.BlockNr)
//            CurPlayPosition = 0;
      }

      // Wenn Spezial-Crop-Modus, nochmal testen, ob auch mit der richtigen rec weitergemacht wird
      if(CutEnding)
      {
        if(/*(ret==RC_Warning) || !HDD_Exist2(PlaybackName, PlaybackDir) ||*/ HDD_Exist2(TempFileName, AbsPlaybackDir))
        {
          State = ST_UnacceptedFile;
          WriteLogMC(PROGRAM_NAME, "Error processing the last segment: Renaming failed!");
          OSDMenuProgressBarDestroyNoOSDUpdate();
          ShowErrorMessage(LangGetString(LS_CutHasFailed), NULL);
          break;
        }
      }

      //Bail out if the currentBlock could not be detected
      if((int)PlayInfo.currentBlock < 0)
      {
        WriteLogMC(PROGRAM_NAME, "MovieCutterProcess: Restarting playback failed! CurrentBlock not detected...");
        State = ST_UnacceptedFile;
        OSDMenuProgressBarDestroyNoOSDUpdate();
        ShowErrorMessage(LangGetString(LS_CutHasFailed), NULL);
        break;
      }

//      ReadBookmarks(Bookmarks, &NrBookmarks);
      CutSaveToBM(TRUE);

      JumpRequestedSegment = 0xFFFF;
      JumpRequestedBlock = (dword) -1;
      NrBookmarks = 0;
      OSDSegmentListDrawList(FALSE);
      OSDInfoDrawProgressbar(TRUE, TRUE);

      if (OSDMenuProgressBarIsVisible())
      {
        OSDMenuSaveMyRegion(rgnSegmentList);
        TAP_SPrint(MessageString, sizeof(MessageString), LangGetString(LS_Cutting), maxProgress - NrSelectedSegments, maxProgress);
        OSDMenuProgressBarShow(PROGRAM_NAME, MessageString, maxProgress - NrSelectedSegments, maxProgress + ((CheckFSAfterCut==FM_Always) ? 1 : 0), NULL);
      }
    }
    if ((NrSelectedSegments <= 0 /* && !SegmentMarker[NrSegmentMarker-2].Selected*/) || (NrSegmentMarker <= 2))
      break;
  }
  CutFileSave();
//  CutDumpList();
//  TAP_Osd_Sync();
//  ClearOSD(FALSE);
//  OSDRedrawEverything();

/*  // Playback-Position wiederherstellen
  if (isPlaybackRunning())
  {
    if(TrickMode == TRICKMODE_Pause) Playback_Normal();
    if (CurPlayPosition > 0)
      TAP_Hdd_ChangePlaybackPos(CurPlayPosition);
  } */

  // Flush the caches *experimental*
//  sync();
//  TAP_Sleep(1);

  // Check the modified Inodes with jfs_icheck
  if (DoiCheckTest == IM_ROEnd || DoiCheckTest == IM_RWEnd)
  {
    if (OSDMenuProgressBarIsVisible())
      OSDMenuProgressBarShow(PROGRAM_NAME, LangGetString(LS_CheckingFileSystem), maxProgress - NrSelectedSegments, maxProgress + ((CheckFSAfterCut==FM_Always) ? 1 : 0), NULL);
    icheckErrors = HDD_CheckInodes(InodeNrs, AbsPlaybackDir, (DoiCheckTest==IM_RWEnd), InodeMonitoring);
  }

  // Save HDD mount point in list of suspect devices
  if (icheckErrors && (InodeMonitoring || CheckFSAfterCut == FM_Shutdown))
  {
    char MountPoint[FBLIB_DIR_SIZE];
    HDD_FindMountPointDev2(AbsPlaybackDir, MountPoint, NULL);
    strcat(MountPoint, ";");
    if (strstr(SuspectHDDs, MountPoint) == 0)
      TAP_SPrint(&SuspectHDDs[strlen(SuspectHDDs)], SIZESUSPECTHDDS-strlen(SuspectHDDs), MountPoint);
    NrAllSuspectInodes += max(icheckErrors, 0);
    #ifdef FULLDEBUG
      WriteLogMCf(PROGRAM_NAME, "NrAllSuspectInodes=%d, SuspectHDDs='%s'", NrAllSuspectInodes, SuspectHDDs);
    #endif
  }

  // Check file system consistency and show a warning
  if (((CheckFSAfterCut == FM_Always) || (CheckFSAfterCut == FM_Auto && icheckErrors)) && jfs_fsck_present)
  {
    WriteLogMCf(PROGRAM_NAME, "Inodes-Check mit fsck: %s", InodeNrs);
    if (CheckFSAfterCut == FM_Always)
      CheckFileSystem(AbsPlaybackDir, maxProgress, maxProgress + 1, maxProgress + 1, TRUE, TRUE, TRUE, TRUE, icheckErrors, InodeNrs);
    else
      CheckFileSystem(AbsPlaybackDir, maxProgress, maxProgress, maxProgress, TRUE, TRUE, TRUE, TRUE, icheckErrors, InodeNrs);
//    if (CurPlayPosition > 0)
//      TAP_Hdd_ChangePlaybackPos(CurPlayPosition);
  }
  else if ((DoiCheckTest == IM_Never) || !jfs_fsck_present)
  {
    WriteLogMC(PROGRAM_NAME, "Cut files have not been verified!");
    if(OSDMenuProgressBarIsVisible()) OSDMenuProgressBarDestroyNoOSDUpdate();
    ShowErrorMessage(LangGetString(LS_FilesNotVerified), LangGetString(LS_Warning));
  }
  else if (icheckErrors != 0)
  {
    char SuspectFilesStr[12];
    if (icheckErrors >= 0)
      TAP_SPrint(SuspectFilesStr, sizeof(SuspectFilesStr), "%d", icheckErrors);
    else if (icheckErrors == -1)
      TAP_SPrint(SuspectFilesStr, sizeof(SuspectFilesStr), "1 or more");
    else
      TAP_SPrint(SuspectFilesStr, sizeof(SuspectFilesStr), "?");

    WriteLogMCf(PROGRAM_NAME, "%s suspect files found with icheck-Test!", SuspectFilesStr);
    if(OSDMenuProgressBarIsVisible()) OSDMenuProgressBarDestroyNoOSDUpdate();
    TAP_SPrint(MessageString, sizeof(MessageString), LangGetString(LS_SuspectFilesFound), SuspectFilesStr);
    ShowErrorMessage(MessageString, LangGetString(LS_Warning));
  }
//  HDD_ChangeDir(PlaybackDir);

  if (OSDMenuProgressBarIsVisible())
  {
    TAP_SPrint(MessageString, sizeof(MessageString), LangGetString(LS_Cutting), maxProgress - NrSelectedSegments, maxProgress);
    OSDMenuProgressBarShow(PROGRAM_NAME, MessageString, maxProgress, maxProgress, NULL);
    OSDMenuProgressBarDestroyNoOSDUpdate();
//    TAP_Osd_Sync();
  }

  if (isPlaybackRunning())
  {
    if(TrickMode == TRICKMODE_Pause) Playback_Normal();
    if (CurPlayPosition > 0)
      TAP_Hdd_ChangePlaybackPos(CurPlayPosition);
    PlaybackRepeatSet(OldRepeatMode);
  }

//  Cleanup(FALSE);
  if (State == ST_UnacceptedFile)
  {
    LastTotalBlocks = PlayInfo.totalBlock;
    WriteLogMC(PROGRAM_NAME, "MovieCutterProcess() finished!");
    ClearOSD(TRUE);
  }
  else
  {
    State = ST_WaitForPlayback;
    WriteLogMC(PROGRAM_NAME, "MovieCutterProcess() successfully completed!");
//    ClearOSD(TRUE);
  }
//  NoPlaybackCheck = FALSE;

  TRACEEXIT();
}


// ----------------------------------------------------------------------------
//                              Hilfsfunktionen
// ----------------------------------------------------------------------------

// altes Verhalten von isPlaybackRunning():
// - holt IMMER die PlayInfo, liefert TRUE wenn Playing, bei ungültigem currentBlock wird dieser 0 gesetzt und TRUE zurückgegeben, bei NoPlaybackCheck wird TRUE zurückgegeben
// neues Verhalten von isPlaybackRunning():
// - bei NoPlaybackCheck wird TRUE zurückgegeben und KEINE PlayInfo gelesen, liefert TRUE wenn Playing, bei ungültigem currentBlock wird TRUE zurückgegeben und NICHT 0 gesetzt
// Grund: currentBlock kann ungültig werden, wenn man z.B. zum Anfang springt, das ist aber trotzdem laufende Wiedergabe
bool isPlaybackRunning(void)
{
  TRACEENTER();

  TAP_Hdd_GetPlayInfo(&PlayInfo);

//  if(NoPlaybackCheck) {
//    WriteLogMC(PROGRAM_NAME, "************NoPlaybackCheck");
//    TRACEEXIT();
//    return TRUE;  // WARUM!?
//  }

//  if((int)PlayInfo.currentBlock < 0) PlayInfo.currentBlock = 0;   *** kritisch ***

  if (PlayInfo.playMode == PLAYMODE_Playing || PlayInfo.playMode == 8)
  {
    TrickMode = (TYPE_TrickMode)PlayInfo.trickMode;
    TrickModeSpeed = PlayInfo.speed;
  }

  TRACEEXIT();
  return (PlayInfo.playMode == PLAYMODE_Playing || PlayInfo.playMode == 8);
}

void CalcLastSeconds(void)
{
  TRACEENTER();
  dword InfDurationSec = 60*PlayInfo.duration + PlayInfo.durationSec;
  if((int)PlayInfo.totalBlock > 0  /*PLAYINFOVALID()*/ && InfDurationSec)  // wenn nur totalBlock gesetzt ist, kann (und muss!) die Berechnung stattfinden
  {
    BlocksOneSecond      = PlayInfo.totalBlock / InfDurationSec;
    BlockNrLastSecond    = PlayInfo.totalBlock - min(PlayInfo.totalBlock, BlocksOneSecond);
    BlockNrLast10Seconds = PlayInfo.totalBlock - min(PlayInfo.totalBlock, (10 * PlayInfo.totalBlock / InfDurationSec));
  }
  else
  {
    BlocksOneSecond = 0; BlockNrLastSecond = PlayInfo.totalBlock; BlockNrLast10Seconds = PlayInfo.totalBlock;
  }
  TRACEEXIT();
}

void CheckLastSeconds(void)
{
  static bool LastSecondsPaused = FALSE;

  TRACEENTER();

  if(PlayInfo.currentBlock > BlockNrLastSecond)
  {
    if ((TrickMode != TRICKMODE_Pause) && !LastSecondsPaused)
      Playback_Pause();
    LastSecondsPaused = TRUE;
  }
  else if(PlayInfo.currentBlock > BlockNrLast10Seconds)
  {
    if (TrickMode == TRICKMODE_Forward)
      Playback_Normal();
  }
  else
    LastSecondsPaused = FALSE;

  TRACEEXIT();
}


// ----------------------------------------------------------------------------
//                            NAV-Funktionen
// ----------------------------------------------------------------------------
dword NavGetBlockTimeStamp(dword PlaybackBlockNr)
{
  TRACEENTER();

  if(LinearTimeMode || !TimeStamps)
  {
    TRACEEXIT();
    return ((dword) (((float)PlaybackBlockNr / PlayInfo.totalBlock) * (60*PlayInfo.duration + PlayInfo.durationSec)) * 1000);
  }

/*  if(TimeStamps == NULL)
  {
    WriteLogMC(PROGRAM_NAME, "Someone is trying to get a timestamp while the array is empty!");
    TRACEEXIT();
    return 0;
  } */

  if(LastTimeStamp->BlockNr < PlaybackBlockNr)
  {
    // Search the TimeStamp-Array in forward direction
    while((LastTimeStamp->BlockNr < PlaybackBlockNr) && (LastTimeStamp < TimeStamps + NrTimeStamps-1))
      LastTimeStamp++;
    if(LastTimeStamp->BlockNr > PlaybackBlockNr)
      LastTimeStamp--;
  }
  else if(LastTimeStamp->BlockNr > PlaybackBlockNr)
  {
    // Search the TimeStamp-Array in reverse direction
    while((LastTimeStamp->BlockNr > PlaybackBlockNr) && (LastTimeStamp > TimeStamps))
      LastTimeStamp--;
    if(LastTimeStamp->BlockNr > PlaybackBlockNr)
    {
      TRACEEXIT();
      return 0;
    }
  }

  TRACEEXIT();
  return LastTimeStamp->Timems;
}

// ----------------------------------------------------------------------------
//                           Compatibility-Layer
// ----------------------------------------------------------------------------
// Die Implementierung berücksichtigt (aus Versehen) auch negative Sprünge und Überlauf.
// SOLLTE eine nav-Datei einen Überlauf beinhalten, wird dieser durch PatchOldNavFile korrigiert.
bool PatchOldNavFile(const char *RecFileName, const char *AbsDirectory, bool isHD)
{
  FILE                 *fSourceNav = NULL;
  FILE                 *fNewNav = NULL;
  char                  NavFileName[MAX_FILE_NAME_SIZE];
  char                  BakFileName[MAX_FILE_NAME_SIZE];
  tnavSD                NavBuffer[2], *CurNavRec = &NavBuffer[0];
  char                  AbsFileName[FBLIB_DIR_SIZE];
  bool                  ret = FALSE;

  TRACEENTER();

//  HDD_ChangeDir(PlaybackDir);
  TAP_SPrint(NavFileName, sizeof(NavFileName), "%s.nav", RecFileName);
  TAP_SPrint(BakFileName, sizeof(BakFileName), "%s.nav.bak", RecFileName);

  //If nav already patched -> exit function
  if (HDD_Exist2(BakFileName, AbsDirectory))
  {
    TRACEEXIT();
    return FALSE;
  }
  WriteLogMC(PROGRAM_NAME, "Checking source nav file (possibly older version with incorrect Times)...");

  //Rename the original nav file to bak
  HDD_Rename2(NavFileName, BakFileName, AbsDirectory, FALSE, FALSE);

  //Open the original nav
  TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s/%s", AbsDirectory, BakFileName);
  fSourceNav = fopen(AbsFileName, "rb");
  if(!fSourceNav)
  {
    WriteLogMC(PROGRAM_NAME, "PatchOldNavFile() E1a01.");
    TRACEEXIT();
    return FALSE;
  }

  //Create and open the new source nav
  TAP_SPrint(AbsFileName, sizeof(AbsFileName), "%s/%s", AbsDirectory, NavFileName);
  fNewNav = fopen(AbsFileName, "wb");
  if(!fNewNav)
  {
    fclose(fSourceNav);
    WriteLogMC(PROGRAM_NAME, "PatchOldNavFile() E1a02.");
    TRACEEXIT();
    return FALSE;
  }

  // Versuche, nav-Dateien aus Timeshift-Aufnahmen zu unterstützen ***experimentell***
  dword FirstDword = 0;
  fread(&FirstDword, 4, 1, fSourceNav);
  if(FirstDword == 0x72767062)  // 'bpvr'
    fseek(fSourceNav, 1056, SEEK_SET);
  else
    rewind(fSourceNav);

  //Loop through the nav
  dword Difference = 0;
  dword LastTime = 0;
  size_t navsCount = 0;

  while (fread(CurNavRec, sizeof(tnavSD) * (isHD ? 2 : 1), 1, fSourceNav))
  {
    ret = TRUE;

    if (CurNavRec->Timems - LastTime >= 3000)
    {
      Difference += (CurNavRec->Timems - LastTime) - 1000;
      WriteLogMCf(PROGRAM_NAME, "  - Gap found at nav record nr. %u:  Offset=%llu, TimeStamp(before)=%lu, TimeStamp(after)=%lu, GapSize=%lu", navsCount /*(ftell(fSourceNav)/sizeof(tnavSD) - navsRead + i) / ((isHD) ? 2 : 1)*/, ((off_t)(CurNavRec->PHOffsetHigh) << 32) | CurNavRec->PHOffset, CurNavRec->Timems, CurNavRec->Timems-Difference, CurNavRec->Timems-LastTime);
    }
    LastTime = CurNavRec->Timems;
    CurNavRec->Timems -= Difference;
    navsCount++;

    ret = (fwrite(CurNavRec, sizeof(tnavSD) * (isHD ? 2 : 1), 1, fNewNav)) && ret;
  }

//  ret = (fflush(fNewNav) == 0) && ret;
  ret = (fclose(fNewNav) == 0) && ret;
  fclose(fSourceNav);
  HDD_SetFileDateTime(&AbsFileName[1], "", 0);

  //On error rename the bak file back to original
  if (!ret)
  {
    WriteLogMC(PROGRAM_NAME, "PatchOldNavFile() E1a03.");
    HDD_Delete2(NavFileName, AbsDirectory, FALSE);
    HDD_Rename2(BakFileName, NavFileName, AbsDirectory, FALSE, FALSE);
    TRACEEXIT();
    return FALSE;
  }

  TRACEEXIT();
  return (Difference > 0);
}


// ----------------------------------------------------------------------------
//                           System-Funktionen
// ----------------------------------------------------------------------------
bool CheckFileSystem(char *MountPath, dword ProgressStart, dword ProgressEnd, dword ProgressMax, bool DoFix, bool Quick, bool NoOkInfo, bool ErrorMessage, int SuspectFiles, char *InodeNrs)
{
  char                  ErrorStrFmt[512], SuspectFilesStr[12];
  dword                 OldSysState, OldSysSubState;
  bool                  ret = FALSE;
  
  TRACEENTER();
  TAP_GetState(&OldSysState, &OldSysSubState);

  // ProgressBar initialisieren
  OSDMenuSaveMyRegion(rgnSegmentList);
//  ClearOSD(FALSE);
  HDDCheck_InitProgBar(ProgressStart, ProgressEnd, ProgressMax, rgnSegmentList, LangGetString(LS_Warning), LangGetString(LS_CheckingFileSystem));

  // Suspekte Dateien in den ErrorString einsetzen
//  TAP_SPrint(ErrorStrFmt, sizeof(ErrorStrFmt), "Suspekt: %d", SuspectFiles);
//  strncat(ErrorStrFmt, LangGetString(LS_CheckFSFailed), sizeof(ErrorStrFmt) - strlen(ErrorStrFmt) - 1);
//  ErrorStrFmt[sizeof(ErrorStrFmt) - 1] = '\0';
  if (SuspectFiles >= 0)
    TAP_SPrint(SuspectFilesStr, sizeof(SuspectFilesStr), "%d", SuspectFiles);
  else
    TAP_SPrint(SuspectFilesStr, sizeof(SuspectFilesStr), "?");
  TAP_SPrint(ErrorStrFmt, sizeof(ErrorStrFmt), LangGetString(LS_CheckFSFailed), SuspectFilesStr);

//  MountPath = (AbsPlaybackDir[0]) ? AbsPlaybackDir : TAPFSROOT;
  ret = HDD_CheckFileSystem(MountPath, NULL, ((ErrorMessage) ? &ShowErrorMessage : NULL), DoFix, Quick, InodeMonitoring, NoOkInfo, InodeNrs, LangGetString(LS_CheckFSSuccess), ErrorStrFmt, LangGetString(LS_CheckFSAborted));

  // Prüfen, ob das Playback wieder gestartet wurde
  if (DoFix && (State==ST_ActiveOSD || State==ST_ActionMenu || State==ST_ActionSubMenu) && (LastTotalBlocks > 0) && (RecFileSize > 0))
  {
    if(!isPlaybackRunning() || (int)PlayInfo.totalBlock <= 0)
    {
      WriteLogMC(PROGRAM_NAME, "CheckFileSystem: Error restarting the playback!");
      State = ST_UnacceptedFile;
      LastTotalBlocks = PlayInfo.totalBlock;
      ClearOSD(TRUE);
    }
    else
    {
      if (SegmentMarker[NrSegmentMarker - 1].Block != PlayInfo.totalBlock)
      {
        #ifdef FULLDEBUG
          WriteLogMCf(PROGRAM_NAME, "CheckFileSystem: Nach Playback-Restart neues TotalBlock %lu (vorher %lu)!", SegmentMarker[NrSegmentMarker - 1].Block, PlayInfo.totalBlock);
        #endif
        SegmentMarker[NrSegmentMarker - 1].Block = PlayInfo.totalBlock;
      }
//      OSDRedrawEverything();
    }
  }
  else if (OldSysSubState != 0) 
    TAP_EnterNormalNoInfo();
  CloseLogMC();

  TRACEEXIT();
  return ret;
}
