#ifndef __MOVIECUTTERLIB__
#define __MOVIECUTTERLIB__

#define TIMECODEARRAYSIZE  94         //94 = 2 blocks * 9024 bytes per block / 192 Bytes per packet
#define TEMPCUTNAME        "__tempcut__.ts"
//#define FULLDEBUG        1

bool  MovieCutter(char *SourceFileName, dword CutPointA, dword CutPointB, bool KeepSource, bool KeepCut);
bool  GetPCR(char *FileName, dword Block, int PacketSize, dword *PCR);
dword DeltaPCR(dword FirstPCR, dword SecondPCR);
void  WriteLogMC(char *ProgramName, char *s);
bool  is192ByteTS(char *FileName);

#endif
