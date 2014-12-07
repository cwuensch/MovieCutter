#define _GNU_SOURCE
#include                <string.h>
#include                <stdio.h>
#include                <stdlib.h>
#include                "tap.h"
#include                "libFireBird.h"
#include                "../../../../../../Topfield/FireBirdLib/flash/FBLib_flash.h"

#define PROGRAM_NAME    "ChannelListTAP"
#define VERSION         "V0.1"

TAP_ID                  (0x8E0A4271);
TAP_PROGRAM_NAME        (PROGRAM_NAME" "VERSION);
TAP_AUTHOR_NAME         ("chris86");
TAP_DESCRIPTION         ("Import/Export of Sat, Transponder, Service, Favorites lists");
TAP_ETCINFO             (__DATE__);

#define EXPORTFILENAME   "ProgramFiles/Channels.dat"


#define PROVIDERNAMELENGTH  21
#define NRPROVIDERNAMES     256
#define SERVICENAMESLENGTH  39996    // 40000 / 40004 / 39996 ***  ?


#define SYSTYPE 7
#if (SYSTYPE == 5)    // ST_TMSS
  typedef TYPE_SatInfo_TMSS             TYPE_SatInfo_TMSx;
  typedef TYPE_TpInfo_TMSS              TYPE_TpInfo_TMSx;
  typedef TYPE_Service_TMSS             TYPE_Service_TMSx;
#elif (SYSTYPE == 7)  // ST_TMSC
  typedef TYPE_SatInfo_TMSC             TYPE_SatInfo_TMSx;
  typedef TYPE_TpInfo_TMSC              TYPE_TpInfo_TMSx;
  typedef TYPE_Service_TMSC             TYPE_Service_TMSx;
#elif (SYSTYPE == 6)  // ST_TMST
  typedef TYPE_SatInfo_TMST             TYPE_SatInfo_TMSx;
  typedef TYPE_TpInfo_TMST              TYPE_TpInfo_TMSx;
  typedef TYPE_Service_TMST             TYPE_Service_TMSx;
#endif


SYSTEM_TYPE                  CurSystemType;
int                          NrFavGroups = 0;
int                          NrFavsPerGroup = 0;
size_t                       SIZE_SatInfo_TMSx = 0;
size_t                       SIZE_TpInfo_TMSx  = 0;
size_t                       SIZE_Service_TMSx = 0;
size_t                       SIZE_Favorites = 0;


bool InitSystemType(void)
{
  bool ret = TRUE;

  CurSystemType = GetSystemType();
  switch (CurSystemType)
  {
    case ST_TMSS:
      SIZE_SatInfo_TMSx = sizeof(TYPE_SatInfo_TMSS);
      SIZE_TpInfo_TMSx  = sizeof(TYPE_TpInfo_TMSS);
      SIZE_Service_TMSx = sizeof(TYPE_Service_TMSS);
      break;
    
    case ST_TMSC:
      SIZE_SatInfo_TMSx = sizeof(TYPE_SatInfo_TMSC);
      SIZE_TpInfo_TMSx  = sizeof(TYPE_TpInfo_TMSC);
      SIZE_Service_TMSx = sizeof(TYPE_Service_TMSC);
      break;

    case ST_TMST:
      SIZE_SatInfo_TMSx = sizeof(TYPE_SatInfo_TMST);
      SIZE_TpInfo_TMSx  = sizeof(TYPE_TpInfo_TMST);
      SIZE_Service_TMSx = sizeof(TYPE_Service_TMST);
      break;

    default:
      TAP_PrintNet("Nicht unterstütztes System!");
      ret = FALSE;
      break;
  }

  FlashFavoritesGetParameters(&NrFavGroups, &NrFavsPerGroup);
  switch (NrFavsPerGroup)
  {
    case 50:
      SIZE_Favorites = sizeof(tFavorites1050);
      break;

    case 100:
      SIZE_Favorites = sizeof(tFavorites);
      break;

    default:
      TAP_PrintNet("Nicht unterstützte Favoriten-Struktur!");
      ret = FALSE;
      break;
  }

  return ret;
}

/*
bool FlashSatTablesDecode_ST_TMSx(void *Data, tFlashSatTable *SatTable)
{
  if(!Data || !SatTable)
    return FALSE;

  switch(CurSystemType)
  {
    case ST_TMSS: return FlashSatTablesDecode_ST_TMSS(Data, SatTable); break;
    case ST_TMST: return FlashSatTablesDecode_ST_TMST(Data, SatTable); break;
    case ST_TMSC: return FlashSatTablesDecode_ST_TMSC(Data, SatTable); break;
    default:      return FALSE;
  }
}

bool FlashTransponderTablesDecode_ST_TMSx(void *Data, tFlashTransponderTable *TransponderTable)
{
  if(!Data || !TransponderTable)
    return FALSE;

  switch(CurSystemType)
  {
    case ST_TMSS: return FlashTransponderTablesDecode_ST_TMSS(Data, TransponderTable); break;
    case ST_TMST: return FlashTransponderTablesDecode_ST_TMST(Data, TransponderTable); break;
    case ST_TMSC: return FlashTransponderTablesDecode_ST_TMSC(Data, TransponderTable); break;
    default:      return FALSE;
  }
}
bool FlashTransponderTablesEncode_ST_TMSx(void *Data, tFlashTransponderTable *TransponderTable)
{
  if(!Data || !TransponderTable)
    return FALSE;

  switch(CurSystemType)
  {
    case ST_TMSS: return FlashTransponderTablesEncode_ST_TMSS(Data, TransponderTable); break;
    case ST_TMST: return FlashTransponderTablesEncode_ST_TMST(Data, TransponderTable); break;
    case ST_TMSC: return FlashTransponderTablesEncode_ST_TMSC(Data, TransponderTable); break;
    default:      return FALSE;
  }
}

bool FlashServiceDecode_ST_TMSx(void *Data, tFlashService *Service)
{
  if(!Data || !Service)
    return FALSE;

  switch(CurSystemType)
  {
    case ST_TMSS: return FlashServiceEncode_ST_TMSS(Data, Service); break;
    case ST_TMST: return FlashServiceEncode_ST_TMST(Data, Service); break;
    case ST_TMSC: return FlashServiceEncode_ST_TMSC(Data, Service); break;
    default:      return FALSE;
  }
}
bool FlashServiceEncode_ST_TMSx(void *Data, tFlashService *Service)
{
  if(!Data || !Service)
    return FALSE;

  switch(CurSystemType)
  {
    case ST_TMSS: return FlashServiceEncode_ST_TMSS(Data, Service); break;
    case ST_TMST: return FlashServiceEncode_ST_TMST(Data, Service); break;
    case ST_TMSC: return FlashServiceEncode_ST_TMSC(Data, Service); break;
    default:      return FALSE;
  }
} */


void WriteFile(TYPE_File *f, char *Text)
{
  if(f) TAP_Hdd_Fwrite(Text, strlen(Text), 1, f);
}

void ExportTransponder(void)
{
  //[Transponder]
  //#SatName    Frq   SymbRate  Channel BW  TSID  ONWID NWID  Pilot FEC Modulation  System  Pol LPHP  ClockSync
  //Astra       10729 22000     0       0   1050  1     0     N     2/3 8PSK        DVBS2   V   0     N

  int                       NrSats, i, NrTransponder, j;
  bool                      ret;
  tFlashSatTable            SatTable;
  tFlashTransponderTable    TransponderTable;
  TYPE_File                *fSettings;
  char                      Log[512];

  //Create the Settings file
  if(!TAP_Hdd_Exist(EXPORTFILENAME)) TAP_Hdd_Delete(EXPORTFILENAME);
  TAP_Hdd_Create(EXPORTFILENAME, ATTR_NORMAL);
  fSettings = TAP_Hdd_Fopen(EXPORTFILENAME);
  WriteFile(fSettings, "[Transponder]\r\n");
  WriteFile(fSettings, "#SatName          Frq    SymbRate  Channel Bandw  TSID ONWID NWID Pilot FEC     Modulation  System  Pol LPHP  ClockSync\r\n");
  //                                                                                                      unknown     DVBS2   H

  NrSats = FlashSatTablesGetTotal();
  for(i = 0; i < NrSats; i++)
  {
    ret = FlashSatTablesGetInfo(i, &SatTable);
    if(ret)
    {
      NrTransponder = FlashTransponderTablesGetTotal(i);
      for(j = 0; j < SatTable.NrOfTransponders; j++)
      {
        ret = FlashTransponderTablesGetInfo(i, j, &TransponderTable);
        if(ret)
        {
          //Sat Name, Frq, SR, Channel, BW, TSID, ONWID, NWID, Pilot
          TAP_SPrint(Log, "%-17s %-6d %-9d %-6d %-7d %4.4x %4.4x  %4.4x %-5s ", SatTable.SatName, TransponderTable.Frequency, TransponderTable.SymbolRate, TransponderTable.ChannelNr, TransponderTable.Bandwidth,
                                                                                TransponderTable.TSID, TransponderTable.OriginalNetworkID, TransponderTable.NetworkID, TransponderTable.Pilot ? "Y" : "N");
          WriteFile(fSettings, Log);

          //FEC
          switch(TransponderTable.FEC)
          {
            case 0x0: TAP_SPrint(Log, "Auto    "); break;
            case 0x1: TAP_SPrint(Log, "1/2     "); break;
            case 0x2: TAP_SPrint(Log, "2/3     "); break;
            case 0x3: TAP_SPrint(Log, "3/4     "); break;
            case 0x4: TAP_SPrint(Log, "5/6     "); break;
            case 0x5: TAP_SPrint(Log, "7/8     "); break;
            case 0x6: TAP_SPrint(Log, "8/9     "); break;
            case 0x7: TAP_SPrint(Log, "3/5     "); break;
            case 0x8: TAP_SPrint(Log, "4/5     "); break;
            case 0x9: TAP_SPrint(Log, "9/10    "); break;
            case 0xf: TAP_SPrint(Log, "None    "); break;
            default:  TAP_SPrint(Log, "unknown "); break;
          }
          WriteFile(fSettings, Log);

          //Modulation Sat
          switch(TransponderTable.Modulation)
          {
            case 0x0: TAP_SPrint(Log, "Auto        "); break;
            case 0x1: TAP_SPrint(Log, "QPSK        "); break;
            case 0x2: TAP_SPrint(Log, "8PSK        "); break;
            case 0x3: TAP_SPrint(Log, "16QAM       "); break;
            default:  TAP_SPrint(Log, "unknown     "); break;
          }
          WriteFile(fSettings, Log);

          //Modulation Cable
          //switch(TransponderTable.Modulation)
          //{
          //  case 0x0: WriteFile(fTransponder, "16QAM;"); break;
          //  case 0x1: WriteFile(fTransponder, "32QAM;"); break;
          //  case 0x2: WriteFile(fTransponder, "64QAM;"); break;
          //  case 0x3: WriteFile(fTransponder, "128QAM;"); break;
          //  case 0x4: WriteFile(fTransponder, "256QAM;"); break;
          //  default: WriteFile(fTransponder, "unknown"); break;
          //}

          //System, Polarisation, LPHP, ClockSync
          TAP_SPrint(Log, "%-7s %-3s %-5d %s\r\n", TransponderTable.ModSystem == 0 ? "DVBS" : "DVBS2",
                                                   TransponderTable.Polarisation == 0 ? "V" : "H",
                                                   TransponderTable.LPHP, TransponderTable.ClockSync ? "Y" : "N");
          WriteFile(fSettings, Log);
        }
        else
        {
          //Failed to decode the transponder
          TAP_PrintNet("Failed to decode transponder %d\n", j);
        }
      }
    }
    else
    {
      //Failed to decode the sat
      TAP_PrintNet("Failed to decode sat %d\n", i);
    }
  }

  TAP_Hdd_Fclose(fSettings);
}

void ImportTransponder(void)
{
  tFlashSatTable            SatTable;
  tFlashTransponderTable    TransponderTable;
  FILE                     *fSetting;
  char                     *Line = NULL, *CurrentSOL, *p;
  size_t                    len = 0;
  int                       ParamCount, i, NrSats, SatIndex;
  char                      Params[50][50];
  bool                      DoParse;
  int                       Count;

  Count = 0;
  if(TAP_Hdd_Exist(EXPORTFILENAME))
  {
    fSetting = fopen(TAPFSROOT "/" EXPORTFILENAME, "r");
    if(fSetting)
    {
      DoParse = FALSE;

      while(getline(&Line, &len, fSetting) != -1)
      {
        //Interpret the following characters as remarks: ; # //
        p = strchr(Line, ';');  if(p) *p = '\0';
        p = strchr(Line, '#');  if(p) *p = '\0';
        p = strstr(Line, "//"); if(p) *p = '\0';

        //Remove CR and LF
        p = strchr(Line, '\r');  if(p) *p = '\0';
        p = strchr(Line, '\n');  if(p) *p = '\0';

        //Replace all TABs with SPACEs
        StrReplace(Line, "\t", " ");

        //Trim all spaces from the end of the line
        RTrim(Line);

        //Check if this is a block header
        if(Line[0] == '[') DoParse = strcmp(Line, "[Transponder]") == 0;

        if(DoParse)
        {
          //Separate the line into the parameters
          ParamCount = 0;
          memset(Params, 0, sizeof(Params));
          CurrentSOL = Line;
          while(CurrentSOL[0])
          {
            //Make wure, the line doesn't start with a SPACE
            while(CurrentSOL[0] == ' ') CurrentSOL++;

            //Find the next SPACE in the current line
            p = strchr(CurrentSOL, ' ');

            //If there is no SPACE, point to EOL
            if(p == NULL) p = CurrentSOL + strlen(CurrentSOL);

            strncpy(Params[ParamCount], CurrentSOL, p - CurrentSOL);
            ParamCount++;

            CurrentSOL = p;
          }

          //Check if enough parameter are available
          if(ParamCount == 15)
          {
            //Decode and add the transponder
            //Translate the SatName (parameter 0) into the sat index
            NrSats = FlashSatTablesGetTotal();
            SatIndex = -1;
            for(i = 0; i < NrSats; i++)
            {
              FlashSatTablesGetInfo(i, &SatTable);
              if(strcmp(SatTable.SatName, Params[0]) == 0)
              {
                SatIndex = i;
                break;
              }
            }

            if(SatIndex != -1)
            {
              //#SatName          Frq    SymbRate  Channel Bandw  TSID ONWID NWID Pilot FEC     Modulation  System  Pol LPHP  ClockSync
              //Astra             10729  22000     0      0       041a 0001  0000 N     2/3     8PSK        DVBS2   V   0     N

              memset(&TransponderTable, 0, sizeof(TransponderTable));
              TransponderTable.SatIndex = SatIndex;
              TransponderTable.Frequency = strtol(Params[1], NULL, 10);
              TransponderTable.SymbolRate = strtol(Params[2], NULL, 10);
              TransponderTable.ChannelNr = strtol(Params[3], NULL, 10);
              TransponderTable.Bandwidth = strtol(Params[4], NULL, 10);
              TransponderTable.TSID = strtol(Params[5], NULL, 16);
              TransponderTable.OriginalNetworkID = strtol(Params[6], NULL, 16);
              TransponderTable.NetworkID = strtol(Params[7], NULL, 16);
              TransponderTable.Pilot = (Params[8][0] == 'Y') || (Params[8][0] == 'y');

              switch(Params[9][0])
              {
                case 'A':
                case 'a': TransponderTable.FEC = 0; break;
                case '1': TransponderTable.FEC = 1; break;
                case '2': TransponderTable.FEC = 2; break;
                case '3':
                {
                  //Two cases here 3/4 and 3/5
                  if(Params[9][2] == '4')
                    TransponderTable.FEC = 3;
                  else
                    TransponderTable.FEC = 7;

                  break;
                }
                case '5': TransponderTable.FEC = 4; break;
                case '7': TransponderTable.FEC = 5; break;
                case '8': TransponderTable.FEC = 6; break;
                case '4': TransponderTable.FEC = 8; break;
                case '9': TransponderTable.FEC = 9; break;
                case 'N':
                case 'n': TransponderTable.FEC = 0xf; break;
              }

              switch(Params[10][0])
              {
                case 'A':
                case 'a': TransponderTable.Modulation = 0; break;
                case 'Q':
                case 'q': TransponderTable.Modulation = 1; break;
                case '8': TransponderTable.Modulation = 2; break;
                case '1': TransponderTable.Modulation = 3; break;
              }

              if(strcmp(Params[11], "DVBS") == 0)
                TransponderTable.ModSystem = 0;
              else
                TransponderTable.ModSystem = 1;

              TransponderTable.Polarisation = ((Params[12][0] == 'H') || (Params[12][0] == 'h')) ? 1 : 0;
              TransponderTable.LPHP = strtol(Params[13], NULL, 10);
              TransponderTable.ClockSync = (Params[14][0] == 'Y') || (Params[14][0] == 'y');

              if(FlashTransponderTablesAdd(SatIndex, &TransponderTable))
              {
                Count++;
              }
              else
              {
                TAP_PrintNet("Failed to add transponder '%s'\n", Line);
              }
            }
            else
            {
              TAP_PrintNet("Didn't find a matching sat for '%s'\n", Line);
            }
          }
          else
          {
            //Ignore block headers
            if((ParamCount > 0) && (Line[0] != '[')) TAP_PrintNet("Failed to import '%s'\n", Line);
          }
        }
      }
      free(Line);
      fclose(fSetting);
    }
  }

  TAP_PrintNet("%d transponders have been added\n", Count);
}

void DeleteTransponder(int SatIndex)
{
  int Count;

  Count = 0;
  while(FlashTransponderTablesGetTotal(SatIndex) > 0)
  {
    FlashTransponderTablesDel(SatIndex, 0);
    Count++;
  }

  TAP_PrintNet("%d transponder have been deleted.\n", Count);
}

void DeleteTimers(void)
{
  int Count;

  Count = 0;
  while(TAP_Timer_GetTotalNum() > 0)
  {
    TAP_Timer_Delete(0);
    Count++;
  }
  TAP_PrintNet("%d timer have been deleted.\n", Count);
}

int GetLengthOfServiceNames(void)
{
  int Result = 0;
  int i;
  char *p1, *p2;
  p1 = (char*)(FIS_vFlashBlockServiceName());
  p2 = (char*)(FIS_vFlashBlockProviderInfo());

  if(p1)
  {
    Result = SERVICENAMESLENGTH;
    if (p2 && (p2 > p1) && (p2-p1 < Result))
      Result = p2 - p1;

    for (i = 0; i < Result-1; i++)
    {
      if (!p1[i] && !p1[i+1])
      {
        Result = i+1;
        break;
      }
    }
  }
  return Result;
}

void DebugServiceNames(char* FileName)
{
  FILE *fOut = NULL;
  char fn[512];
  char *p = NULL;

  TAP_SPrint(fn, "%s/%s", TAPFSROOT, FileName);
  fOut = fopen(fn, "wb");
  p = (char*)(FIS_vFlashBlockServiceName());
  if(p && fOut)
  {
    fwrite(p, 1, SERVICENAMESLENGTH, fOut);
    fclose(fOut);
  }
}

void DeleteServiceNames(void)
{
  void  (*Appl_DeleteTvSvcName)(unsigned short, bool);
  void  (*Appl_DeleteRadioSvcName)(unsigned short, bool);
  Appl_DeleteTvSvcName    = (void*)FIS_fwAppl_DeleteTvSvcName();
  Appl_DeleteRadioSvcName = (void*)FIS_fwAppl_DeleteRadioSvcName();

  int nTVServices, nRadioServices, i;
  TAP_Channel_GetTotalNum(&nTVServices, &nRadioServices);

//  DebugServiceNames("vorher.dat");
//  char tmp[512];
  for (i = (nRadioServices - 1); i >= 0; i--)
  {
    Appl_DeleteRadioSvcName(i, FALSE);
//    TAP_SPrint(tmp, "Rad%ld.dat", i);
//    DebugServiceNames(tmp);
  }
  for (i = (nTVServices - 1); i >= 0; i--)
  {
    Appl_DeleteTvSvcName(i, FALSE);
//    TAP_SPrint(tmp, "TV%ld.dat", i);
//    DebugServiceNames(tmp);
  }
//  DebugServiceNames("nachher.dat");
}

bool DeleteAllSettings(void)
{
  bool ret = TRUE;

  {
    // Favourites
    char                 *p;
    p = (char*) FIS_vFlashBlockFavoriteGroup();

    if (p)
    {
      memset(p, 0, NrFavGroups * SIZE_Favorites);
    }
    else ret = FALSE;
  }

  {
    // Service Names
    char                 *p1, *p2;

    DeleteServiceNames();
//    p1 = (char*)(FIS_vFlashBlockServiceName());
//    p2 = (char*)(FIS_vFlashBlockProviderInfo());
//    if(p1 && p2 && (p2 > p1))
//      memset(p, 0, min(p2 - p1, SERVICENAMESLENGTH));
  }

  {
    // Provider Names
    char                 *p;

    p = (char*)(FIS_vFlashBlockProviderInfo());
    if(p)
      memset(p, 0, PROVIDERNAMELENGTH * NRPROVIDERNAMES);
  }

  {
    // TV Services
    TYPE_Service_TMSS      *p;
    word                   *nSvc;

    p    = (TYPE_Service_TMSS*)(FIS_vFlashBlockTVServices());
    nSvc = (word*)FIS_vnTvSvc();
    if (p && nSvc)
    {
      memset(p, 0, *nSvc * SIZE_Service_TMSx);
      *nSvc = 0;
    }

    // Radio Services
    p    = (TYPE_Service_TMSS*)(FIS_vFlashBlockRadioServices());
    nSvc = (word*)FIS_vnRadioSvc();
    if (p && nSvc)
    {
      memset(p, 0, *nSvc * SIZE_Service_TMSx);
      *nSvc = 0;
    }
    else ret = FALSE;
  }

  {
    // Transponders
    TYPE_TpInfo_TMSS *p;
    dword            *NrTransponders;
          
    p = (TYPE_TpInfo_TMSS*)(FIS_vFlashBlockTransponderInfo());
    if (p)
    {
      NrTransponders = (dword*)(p) - 1;
      memset(p, 0, *NrTransponders * SIZE_TpInfo_TMSx);
      *NrTransponders = 0;
    }
    else ret = FALSE;
  }

  {
    // Satellites
    TYPE_SatInfo_TMSS *p;
    p = (TYPE_SatInfo_TMSS*)FIS_vFlashBlockSatInfo();
    if (p)
      memset(p, 0, FlashSatTablesGetTotal() * SIZE_SatInfo_TMSx);
    else ret = FALSE;
  }

  return ret;
}



typedef struct
{
  char                  Magic[6];     // TFchan
  short                 FileVersion;  // 1
  SYSTEM_TYPE           SystemType;
  bool                  UTF8System;
  unsigned long         FileSize;

  int                   SatellitesOffset;
  int                   TranspondersOffset;
  int                   TVServicesOffset;
  int                   RadioServicesOffset;
  int                   FavoritesOffset;
  int                   ProviderNamesOffset;
  int                   ServiceNamesOffset;

  int                   NrSatellites;
  int                   NrTransponders;
  int                   NrTVServices;
  int                   NrRadioServices;
  int                   ProviderNamesLength;
  int                   ServiceNamesLength;
  int                   NrFavGroups;
  int                   NrSvcsPerFavGroup;
} tExportHeader;


bool ExportSettings()
{
  tExportHeader         FileHeader;
  FILE                 *fExportFile = NULL;
  int                   i;
  bool                  ret = FALSE;

  TRACEENTER();
  TAP_PrintNet("Starte Export...\n");

  fExportFile = fopen(TAPFSROOT "/" EXPORTFILENAME, "wb");
  if(fExportFile)
  {
    ret = TRUE;

    // Write the file header
    memset(&FileHeader, 0, sizeof(FileHeader));
    strncpy(FileHeader.Magic, "TFchan", 6);
    FileHeader.FileVersion = 1;
    FileHeader.SystemType = GetSystemType();
    FileHeader.UTF8System = isUTFToppy();

    // Now write the data blocks to the file
    ret = fwrite(&FileHeader, sizeof(tExportHeader), 1, fExportFile) && ret;
    {
      TYPE_SatInfo_TMSS *p;
      FileHeader.SatellitesOffset = ftell(fExportFile);
      p = (TYPE_SatInfo_TMSS*)FIS_vFlashBlockSatInfo();
      if(p)
      {
        FileHeader.NrSatellites = FlashSatTablesGetTotal();
        ret = fwrite(&FileHeader.NrSatellites, sizeof(FileHeader.NrSatellites), 1, fExportFile) && ret;
        FileHeader.SatellitesOffset = ftell(fExportFile);
        if (FileHeader.NrSatellites > 0)
          ret = fwrite(p, SIZE_SatInfo_TMSx, FileHeader.NrSatellites, fExportFile) && ret;
      }
    }
    TAP_PrintNet((ret) ? "Satellites ok\n" : "Satellites Fehler\n");

    {
      TYPE_TpInfo_TMSS *p;
      FileHeader.TranspondersOffset = ftell(fExportFile);
      p = (TYPE_TpInfo_TMSS*)(FIS_vFlashBlockTransponderInfo());
      if(p)
      {
        for(i = 0; i < FileHeader.NrSatellites; i++)
          FileHeader.NrTransponders += FlashTransponderTablesGetTotal(i);
        ret = fwrite(&FileHeader.NrTransponders, sizeof(FileHeader.NrTransponders), 1, fExportFile) && ret;
        FileHeader.TranspondersOffset = ftell(fExportFile);
        if (FileHeader.NrTransponders > 0)
          ret = fwrite(p, SIZE_TpInfo_TMSx, FileHeader.NrTransponders, fExportFile) && ret;
      }
    }
    TAP_PrintNet((ret) ? "Transponders ok\n" : "Transponders Fehler\n");

    {
      TYPE_Service_TMSS *p;
      FileHeader.TVServicesOffset = ftell(fExportFile);
      p = (TYPE_Service_TMSS*)(FIS_vFlashBlockTVServices());
      if(p)
      {
        int Muell;
        TAP_Channel_GetTotalNum(&FileHeader.NrTVServices, &Muell);
        ret = fwrite(&FileHeader.NrTVServices, sizeof(FileHeader.NrTVServices), 1, fExportFile) && ret;
        FileHeader.TVServicesOffset = ftell(fExportFile);
        if (FileHeader.NrTVServices > 0)
          ret = fwrite(p, SIZE_Service_TMSx, FileHeader.NrTVServices, fExportFile) && ret;
      }
    }
    TAP_PrintNet((ret) ? "TVServices ok\n" : "TVServices Fehler\n");

    {
      TYPE_Service_TMSS *p;
      FileHeader.RadioServicesOffset = ftell(fExportFile);
      p = (TYPE_Service_TMSS*)(FIS_vFlashBlockRadioServices());
      if(p)
      {
        int Muell;
        TAP_Channel_GetTotalNum(&Muell, &FileHeader.NrRadioServices);
        ret = fwrite(&FileHeader.NrRadioServices, sizeof(FileHeader.NrRadioServices), 1, fExportFile) && ret;
        FileHeader.RadioServicesOffset = ftell(fExportFile);
        if (FileHeader.NrRadioServices > 0)
          ret = fwrite(p, SIZE_Service_TMSx, FileHeader.NrRadioServices, fExportFile) && ret;
      }
    }
    TAP_PrintNet((ret) ? "RadioServices ok\n" : "RadioServices Fehler\n");

    {
      tFavorites FavGroup;
      FileHeader.NrFavGroups = NrFavGroups;
      FileHeader.NrSvcsPerFavGroup = NrFavsPerGroup;
      ret = fwrite(&FileHeader.NrFavGroups, sizeof(FileHeader.NrFavGroups), 1, fExportFile) && ret;
      FileHeader.FavoritesOffset = ftell(fExportFile);
      for (i = 0; i < FileHeader.NrFavGroups; i++)
      {
        memset(&FavGroup, 0, sizeof(tFavorites));
        FlashFavoritesGetInfo(i, &FavGroup);
        ret = fwrite(&FavGroup, sizeof(tFavorites), 1, fExportFile) && ret;
      }
    }
    TAP_PrintNet((ret) ? "Favorites ok\n" : "Favorites Fehler\n");

    {
      char *p1, *p2;
      p1 = (char*)(FIS_vFlashBlockServiceName());
      p2 = (char*)(FIS_vFlashBlockProviderInfo());
      int NrProviderNames = NRPROVIDERNAMES;

      ret = fwrite(&NrProviderNames, sizeof(NrProviderNames), 1, fExportFile) && ret;
      FileHeader.ProviderNamesOffset = ftell(fExportFile);
      if(p2)
      {
        FileHeader.ProviderNamesLength = PROVIDERNAMELENGTH * NRPROVIDERNAMES;  // ***  5380 ?
        ret = fwrite(p2, 1, FileHeader.ProviderNamesLength, fExportFile) && ret;
      }
      TAP_PrintNet((ret) ? "ProviderNames ok\n" : "ProviderNames Fehler\n");

      int NrServices = FileHeader.NrTVServices + FileHeader.NrRadioServices;
      ret = fwrite(&NrServices, sizeof(FileHeader.NrTVServices), 1, fExportFile) && ret;
      FileHeader.ServiceNamesOffset = ftell(fExportFile);
      if(p1)
      {
//        FileHeader.ServiceNamesLength = 40004;   // 40000 / 39996 ***  ?
//        if(p2)
//          FileHeader.ServiceNamesLength = p2 - p1;
        FileHeader.ServiceNamesLength = GetLengthOfServiceNames();
        ret = fwrite(p1, 1, FileHeader.ServiceNamesLength, fExportFile) && ret;
      }
      TAP_PrintNet((ret) ? "ServiceNames ok\n" : "ServiceNames Fehler\n");
    }

    FileHeader.FileSize = ftell(fExportFile);
    fclose(fExportFile);

    fExportFile = fopen(TAPFSROOT "/" EXPORTFILENAME, "r+b");
    if(fExportFile)
    {
      ret = fwrite(&FileHeader, sizeof(tExportHeader), 1, fExportFile) && ret;
      fclose(fExportFile);
    }
  }
  else
    TAP_PrintNet("Datei nicht gefunden\n");

  if (!ret)
    if(TAP_Hdd_Exist(EXPORTFILENAME)) TAP_Hdd_Delete(EXPORTFILENAME);
  TAP_PrintNet((ret) ? "Export erfolgreich\n" : "Export fehlgeschlagen\n");
  TRACEEXIT();
  return ret;
}



bool ImportSettings()
{
  tExportHeader         FileHeader;
  FILE                 *fImportFile = NULL;
  unsigned long         fs;
  int                   i;
  bool                  ret = FALSE;
  char                 *Buffer = NULL;

  TRACEENTER();
  TAP_PrintNet ("Starte Import...\n");

  fImportFile = fopen(TAPFSROOT "/" EXPORTFILENAME, "rb");
  if(fImportFile)
  {
    // Dateigröße bestimmen um Puffer zu allozieren
    fseek(fImportFile, 0, SEEK_END);
    fs = ftell(fImportFile);
    rewind(fImportFile);

    // Header prüfen
    if (  (fread(&FileHeader, sizeof(tExportHeader), 1, fImportFile))
       && (strncmp(FileHeader.Magic, "TFchan", 6) == 0)
       && (FileHeader.FileVersion == 1)
       && (FileHeader.FileSize == fs)
       && (FileHeader.SystemType == GetSystemType())
       && ((FileHeader.SystemType == ST_TMSS) || (FileHeader.SystemType == ST_TMSC) || (FileHeader.SystemType == ST_TMST)))
    {
      Buffer = (char*) TAP_MemAlloc(fs * sizeof(char));
      if (Buffer)
      {
        rewind(fImportFile);
        if (fread(Buffer, 1, fs, fImportFile) == fs)
        {
          ret = TRUE;

          // Now write the data blocks from the file to the RAM
          {
            TYPE_SatInfo_TMSS *p;

            p = (TYPE_SatInfo_TMSS*)FIS_vFlashBlockSatInfo();
//            fseek(fImportFile, FileHeader.SatellitesOffset, SEEK_SET);
            if (ret && p /*&& (fread(Buffer, SIZE_SatInfo_TMSx, FileHeader.NrSatellites, fImportFile) == (size_t)FileHeader.NrSatellites)*/)
            {
//              NrSatellites = FlashSatTablesGetTotal();
//              memset(p, 0, NrSatellites * SIZE_SatInfo_TMSx);
              memcpy(p, Buffer + FileHeader.SatellitesOffset, FileHeader.NrSatellites * SIZE_SatInfo_TMSx);
//              *NrSatellitesTest = FileHeader.NrSatellites;
            }
            else
              ret = FALSE;
          }
          TAP_PrintNet((ret) ? "Satellites ok\n" : "Satellites Fehler\n");

          {
            TYPE_TpInfo_TMSS *p;
            dword            *NrTransponders;
          
            p = (TYPE_TpInfo_TMSS*)(FIS_vFlashBlockTransponderInfo());
            NrTransponders = (dword*)(p) - 1;
//            memset(p, 0, *NrTransponders * SIZE_TpInfo_TMSx);
//            *NrTransponders = 0;
//            fseek(fImportFile, FileHeader.TranspondersOffset, SEEK_SET);
            if (ret && p /*&& (fread(Buffer, SIZE_TpInfo_TMSx, FileHeader.NrTransponders, fImportFile) == (size_t)FileHeader.NrTransponders)*/)
            {
              TAP_PrintNet("NrTransponders = %lu \n", *NrTransponders);
              memcpy(p, Buffer + FileHeader.TranspondersOffset, FileHeader.NrTransponders * SIZE_TpInfo_TMSx);
              *NrTransponders = FileHeader.NrTransponders;
            }
            else
              ret = FALSE;
          }
          TAP_PrintNet((ret) ? "Transponders ok\n" : "Transponders Fehler\n");

          {
            char*                 (*Appl_AddSvcName)(char const*);
            word                  (*Appl_SetProviderName)(char const*);
            TYPE_Service_TMSx      *p;
            word                   *nSvc;

            Appl_AddSvcName       = (void*)FIS_fwAppl_AddSvcName();
            Appl_SetProviderName  = (void*)FIS_fwAppl_SetProviderName();


            char* Buffer2 = Buffer + FileHeader.ServiceNamesOffset;   // (char*) TAP_MemAlloc(FileHeader.ServiceNamesLength * sizeof(char));
            char* Buffer3 = Buffer + FileHeader.ProviderNamesOffset;  // (char*) TAP_MemAlloc(FileHeader.ProviderNamesLength * sizeof(char));
//          if (Buffer2 && Buffer3)
//          {
            fseek(fImportFile, FileHeader.ProviderNamesOffset, SEEK_SET);
            if (ret /*&& (fread(Buffer3, 1, FileHeader.ProviderNamesLength, fImportFile) == (size_t)FileHeader.ProviderNamesLength)
                    && (fread(Buffer2, 1, FileHeader.ServiceNamesLength, fImportFile) == (size_t)FileHeader.ServiceNamesLength)*/)
            {
              p    = (TYPE_Service_TMSx*)(FIS_vFlashBlockTVServices());
              nSvc = (word*)FIS_vnTvSvc();
//              DeleteServiceNames(TRUE);
//              memset(p, 0, *nSvc * SIZE_Service_TMSx);
//              *nSvc = 0;

//              fseek(fImportFile, FileHeader.TVServicesOffset, SEEK_SET);
              if (ret && p && nSvc /*&& (fread(Buffer, SIZE_Service_TMSx, FileHeader.NrTVServices, fImportFile) == (size_t)FileHeader.NrTVServices)*/)
              {
                memcpy(p, Buffer + FileHeader.TVServicesOffset, FileHeader.NrTVServices * SIZE_Service_TMSx);
                *nSvc = FileHeader.NrTVServices;
                TAP_PrintNet("NrTVServices = %lu \n", *nSvc);

                TYPE_Service_TMSx* pServices;
                pServices = (TYPE_Service_TMSx*) (Buffer + FileHeader.TVServicesOffset);
                for (i = 0; i < FileHeader.NrTVServices; i++)
                {
//                  *nSvc = (word)(i+1);
                  if (Appl_AddSvcName)
                  {
                    if (pServices[i].NameOffset < (dword)FileHeader.ServiceNamesLength)
                    {
                      TAP_PrintNet("%s\n", &Buffer2[pServices[i].NameOffset]);
                      p[i].NameOffset = (dword)Appl_AddSvcName(&Buffer2[pServices[i].NameOffset]);
                    }
                    else
                      p[i].NameOffset = (dword)Appl_AddSvcName("***Dummy***");
                  }
                  else
                    ret = FALSE;
                  if (Appl_SetProviderName)
                  {
                    if (pServices[i].ProviderIdx * PROVIDERNAMELENGTH < FileHeader.ProviderNamesLength)
                      p[i].ProviderIdx = Appl_SetProviderName(&Buffer3[pServices[i].ProviderIdx * PROVIDERNAMELENGTH]);
                    else
                      p[i].ProviderIdx = Appl_SetProviderName("***Dummy***");
                  }
                  else
                    ret = FALSE;
                  p[i].NameLock = 0;
//                  memcpy(&p[i], &pServices[i], SIZE_Service_TMSx);
                }
              }
              else
                ret = FALSE;
              TAP_PrintNet((ret) ? "TVServices ok\n" : "TVServices Fehler\n");

              p    = (TYPE_Service_TMSx*)(FIS_vFlashBlockRadioServices());
              nSvc = (word*)FIS_vnRadioSvc();
//              DeleteServiceNames(FALSE);
//              memset(p, 0, *nSvc * SIZE_Service_TMSx);
//              *nSvc = 0;

//              fseek(fImportFile, FileHeader.RadioServicesOffset, SEEK_SET);
              if (ret && p && nSvc /*&& (fread(Buffer, SIZE_Service_TMSx, FileHeader.NrRadioServices, fImportFile) == (size_t)FileHeader.NrRadioServices)*/)
              {
                memcpy(p, Buffer + FileHeader.RadioServicesOffset, FileHeader.NrRadioServices * SIZE_Service_TMSx);
                *nSvc = FileHeader.NrRadioServices;
                TAP_PrintNet("NrRadioServices = %lu \n", *nSvc);

                TYPE_Service_TMSx* pServices;
                pServices = (TYPE_Service_TMSx*) (Buffer + FileHeader.RadioServicesOffset);
                for (i = 0; i < FileHeader.NrRadioServices; i++)
                {
//                  *nSvc = (word)(i+1);
                  if (Appl_AddSvcName)
                  {
                    if (pServices[i].NameOffset < (dword)FileHeader.ServiceNamesLength)
                    {
                      TAP_PrintNet("%s\n", &Buffer2[pServices[i].NameOffset]);
                      p[i].NameOffset = (dword)Appl_AddSvcName(&Buffer2[pServices[i].NameOffset]);
                    }
                    else
                      p[i].NameOffset = (dword)Appl_AddSvcName("***Dummy***");
                  }
                  else
                    ret = FALSE;
                  if (Appl_SetProviderName)
                  {
                    if (pServices[i].ProviderIdx * PROVIDERNAMELENGTH < FileHeader.ProviderNamesLength)
                      p[i].ProviderIdx = Appl_SetProviderName(&Buffer3[pServices[i].ProviderIdx * PROVIDERNAMELENGTH]);
                    else
                      p[i].ProviderIdx = Appl_SetProviderName("***Dummy***");
                  }
                  else
                    ret = FALSE;
                  p[i].NameLock = 0;
//                  memcpy(&p[i], &pServices[i], SIZE_Service_TMSx);
                }
              }
              else
                ret = FALSE;
            }
            else
              ret = FALSE;
//            }
//            else
//              ret = FALSE;
            Buffer2 = NULL;  // TAP_MemFree(Buffer2);
            Buffer3 = NULL;  // TAP_MemFree(Buffer3);
            TAP_PrintNet((ret) ? "RadioServices ok\n" : "RadioServices Fehler\n");
          }

          {
//            int                   NrGroups, NrSvcsPerGroup;
            tFavorites            FavGroup;
            char                 *p;

//            FlashFavoritesGetParameters(&NrGroups, &NrSvcsPerGroup);
            p = (char*) FIS_vFlashBlockFavoriteGroup();
//            memset(p, 0, NrGroups * ((NrSvcsPerGroup == 50) ? sizeof(tFavorites1050) : sizeof(tFavorites)));

//            word *NrFavGroupsTest = (word*)(p) - 1;
//            TAP_PrintNet("NrFavGroups = %u \n", *NrFavGroupsTest);

            fseek(fImportFile, FileHeader.FavoritesOffset, SEEK_SET);
            for (i = 0; i < FileHeader.NrFavGroups; i++)
            {
//              memset(&FavGroup, 0, sizeof(tFavorites));
              if (ret && (fread(&FavGroup, sizeof(tFavorites), 1, fImportFile)))
              {
                if (FavGroup.GroupName[0])
                {
                  // bescheuerter Workaround für zu strenge Prüfung in FlashFavoritesSetInfo()
                  (p + i * SIZE_Favorites)[0] = '*';
                  TAP_PrintNet("FavGroup %d: Name = '%s', Entries = %d\n", i, FavGroup.GroupName, FavGroup.NrEntries);
                  ret = ret && FlashFavoritesSetInfo(i, &FavGroup);
                }
              }
            }
          }
          TAP_PrintNet((ret) ? "Favorites ok\n" : "Favorites Fehler\n");
        }

        TAP_MemFree(Buffer);
      }
      else
        TAP_PrintNet ("Nicht genung Speicher!\n");
    }
    else
      TAP_PrintNet ("Header passt nicht!\n");
    fclose(fImportFile);
  }
  else
    TAP_PrintNet ("Datei nicht gefunden!\n");
//  if(ret)
//    FlashProgram();
  TAP_PrintNet((ret) ? "Import erfolgreich\n" : "Import fehlgeschlagen\n");
  TRACEEXIT();
  return ret;
}



dword TAP_EventHandler(word event, dword param1, dword param2)
{
  (void) event;
  (void) param2;

  return param1;
}

int TAP_Main(void)
{
  if (InitSystemType())
  {
    TAP_Hdd_ChangeDir("/");
    if(TAP_Hdd_Exist(EXPORTFILENAME))
    {
      DeleteTimers();
      DeleteAllSettings();
//      DeleteFavourites();
//      DeleteServices();
//      DeleteTransponder(1);
//      ImportTransponder();
      ImportSettings();
//      Appl_ImportChData("Settings.std");
    }
    else
    {
      ExportSettings();
//      Appl_ExportChData("Settings.std");
    }
  }
  return 0;
}
