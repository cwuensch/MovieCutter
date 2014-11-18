#define _GNU_SOURCE
#include                <string.h>
#include                <stdio.h>
#include                <stdlib.h>
#include                "tap.h"
#include                "libFireBird.h"
#include                "../../../../../FireBirdLib/flash/FBLib_flash.h"

#define PROGRAM_NAME    "ChannelListTAP"
#define VERSION         "V0.1"

TAP_ID                  (0x8E0A4271);
TAP_PROGRAM_NAME        (PROGRAM_NAME" "VERSION);
TAP_AUTHOR_NAME         ("chris86");
TAP_DESCRIPTION         ("Import/Export of Sat, Transponder, Service, Favorites lists");
TAP_ETCINFO             (__DATE__);

#define EXPORTFILENAME   "ProgramFiles/Channels.dat"

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

void DeleteServices(void)
{
  int CountT, CountR;

  CountT = 0;
  while(FlashServiceGetTotal(SVC_TYPE_Tv) > 0)
  {
    FlashServiceDel(SVC_TYPE_Tv, 0);
    CountT++;
  }

  CountR = 0;
  while(FlashServiceGetTotal(SVC_TYPE_Radio) > 0)
  {
    FlashServiceDel(SVC_TYPE_Radio, 0);
    CountR++;
  }

  TAP_PrintNet("%d TV and %d radio services have been deleted.\n", CountT, CountR);
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




typedef struct
{
  char                  Magic[6];     // TFchan
  short                 FileVersion;  // 1
  SYSTEM_TYPE           SystemType;
  unsigned long         FileSize;

  int                   SatellitesOffset;
  int                   TranspondersOffset;
  int                   TVServicesOffset;
  int                   RadioServicesOffset;
  int                   FavoritesOffset;
  int                   ServiceNamesOffset;
  int                   ProviderNamesOffset;
  int                   Padding;

  int                   NrSatellites;
  int                   NrTransponders;
  int                   NrTVServices;
  int                   NrRadioServices;
  int                   ServiceNamesLength;
  int                   ProviderNamesLength;
  int                   NrFavGroups;
  int                   NrSvcsPerFavGroup;
} tExportHeader;


bool ExportSettings_TMSC()
{
  tExportHeader         FileHeader;
  FILE                 *fExportFile = NULL;
  int                   i;
  bool                  ret = FALSE;

  TRACEENTER();

  fExportFile = fopen(TAPFSROOT "/" EXPORTFILENAME, "wb");
  if(fExportFile)
  {
    ret = TRUE;

    // Write the file header
    memset(&FileHeader, 0, sizeof(FileHeader));
    strncpy(FileHeader.Magic, "TFchan", 6);
    FileHeader.FileVersion = 1;
    FileHeader.SystemType = GetSystemType();

/*    // Detect the size of the blocks
    Offset = sizeof(tExportHeader);

    FileHeader.SatellitesOffset = Offset;
    FileHeader.NrSatellites = FlashSatTablesGetTotal();
    Offset += FileHeader.NrSatellites * sizeof(TYPE_SatInfo_TMSC);

    FileHeader.TranspondersOffset = Offset;
    for(i = 0; i < FileHeader.NrSatellites; i++)
      FileHeader.NrTransponders += FlashTransponderTablesGetTotal(i);
    Offset += FileHeader.NrTransponders * sizeof(TYPE_TpInfo_TMSC);

    TAP_Channel_GetTotalNum(&FileHeader.NrTVServices, &FileHeader.NrRadioServices);
    FileHeader.TVServicesOffset = Offset;
    Offset += FileHeader.NrTVServices * sizeof(TYPE_Service_TMSC);
    FileHeader.RadioServicesOffset = Offset;
    Offset += FileHeader.NrRadioServices * sizeof(TYPE_Service_TMSC);

    FileHeader.FavoritesOffset = Offset;
    FlashFavoritesGetParameters(&NrGroups, &NrSvcsPerGroup);
    Offset += ((NrSvcsPerGroup==100) ? sizeof(tFavorites) : sizeof(tFavorites1050));

    FileHeader.ServiceNamesOffset = Offset;
    Offset += 65004;

    FileHeader.ProviderNamesOffset = Offset;
    Offset += 5380;
    FileHeader.FileSize = Offset;  */

    // Now write the data blocks to the file
    ret = ret & fwrite(&FileHeader, sizeof(tExportHeader), 1, fExportFile);
    {
      TYPE_SatInfo_TMSC *p;
      FileHeader.SatellitesOffset = ftell(fExportFile);
      p = (TYPE_SatInfo_TMSC*)FIS_vFlashBlockSatInfo();
      if(p)
      {
        FileHeader.NrSatellites = FlashSatTablesGetTotal();
        ret = ret & fwrite(p, sizeof(TYPE_SatInfo_TMSC), FileHeader.NrSatellites, fExportFile);
      }
    }
    {
      TYPE_TpInfo_TMSC *p;
      FileHeader.TranspondersOffset = ftell(fExportFile);
      p = (TYPE_TpInfo_TMSC*)(FIS_vFlashBlockTransponderInfo());
      if(p)
      {
        for(i = 0; i < FileHeader.NrSatellites; i++)
          FileHeader.NrTransponders += FlashTransponderTablesGetTotal(i);
        ret = ret & fwrite(p, sizeof(TYPE_TpInfo_TMSC), FileHeader.NrTransponders, fExportFile);
      }
    }
    {
      TYPE_Service_TMSC *p;
      FileHeader.TVServicesOffset = ftell(fExportFile);
      p = (TYPE_Service_TMSC*)(FIS_vFlashBlockTVServices());
      if(p)
      {
        int Muell;
        TAP_Channel_GetTotalNum(&FileHeader.NrTVServices, &Muell);
//        p[1].NameLock = 0;  // TEST des RoboChannel Flags (--> klappt!)
        ret = ret & fwrite(p, sizeof(TYPE_Service_TMSC), FileHeader.NrTVServices, fExportFile);
      }
    }
    {
      TYPE_Service_TMSC *p;
      FileHeader.RadioServicesOffset = ftell(fExportFile);
      p = (TYPE_Service_TMSC*)(FIS_vFlashBlockRadioServices());
      if(p)
      {
        int Muell;
        TAP_Channel_GetTotalNum(&Muell, &FileHeader.NrRadioServices);
        ret = ret & fwrite(p, sizeof(TYPE_Service_TMSC), FileHeader.NrRadioServices, fExportFile);
      }
    }
    {
      tFavorites FavGroup;
      FileHeader.FavoritesOffset = ftell(fExportFile);
      FlashFavoritesGetParameters(&FileHeader.NrFavGroups, &FileHeader.NrSvcsPerFavGroup);
      for (i = 0; i < FileHeader.NrFavGroups; i++)
      {
        memset(&FavGroup, 0, sizeof(tFavorites));
        FlashFavoritesGetInfo(i, &FavGroup);
        ret = ret & fwrite(&FavGroup, sizeof(tFavorites), 1, fExportFile);
      }
    }
    {
      char *p1, *p2;
      FileHeader.ServiceNamesOffset = ftell(fExportFile);
      p1 = (char*)(FIS_vFlashBlockServiceName());
      p2 = (char*)(FIS_vFlashBlockProviderInfo());
      if(p1)
      {
        if(p2)
          FileHeader.ServiceNamesLength = p2 - p1;  // 40000 / 39996 ***  ?
        ret = ret & fwrite(p1, 1, FileHeader.ServiceNamesLength, fExportFile);
      }
      FileHeader.ProviderNamesOffset = ftell(fExportFile);
      if(p2)
      {
        FileHeader.ProviderNamesLength = 21 * 256;  // ***  5380 ?
        ret = ret & fwrite(p2, 1, FileHeader.ProviderNamesLength, fExportFile);
      }
    }
    FileHeader.FileSize = ftell(fExportFile);
    fclose(fExportFile);

    fExportFile = fopen(TAPFSROOT "/" EXPORTFILENAME, "r+b");
    if(fExportFile)
    {
      ret = ret & fwrite(&FileHeader, sizeof(tExportHeader), 1, fExportFile);
      fclose(fExportFile);
    }
  }
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
  TAP_Hdd_ChangeDir("/");
//  if(!TAP_Hdd_Exist(EXPORTFILENAME))
  {
    ExportSettings_TMSC();
  }
//  else
  {
    //DeleteTimers();
    //DeleteFavourites();
    //DeleteServices();
    //DeleteTransponder(1);
    //ImportTransponder();
  }

  return 0;
}
