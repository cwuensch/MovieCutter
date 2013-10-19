#ifndef __MOVIECUTTERLIB__
#define __MOVIECUTTERLIB__

#define TIMECODEARRAYSIZE  94         // 94 packets = 2 blocks a 47 packets (9024 Bytes per block, 192 Bytes per packet)
#define TEMPCUTNAME        "__tempcut__.ts"
//#define FULLDEBUG        1

bool  MovieCutter(char *SourceFileName, dword CutPointA, dword CutPointB, bool KeepSource, bool KeepCut, unsigned int LeaveNamesOut);
bool  GetPCRPID(char *FileName, word *PCRPID);
bool  GetPCR(char *FileName, dword Block, int PacketSize, word PCRPID, dword *PCR);
dword DeltaPCR(dword FirstPCR, dword SecondPCR);
void  WriteLogMC(char *ProgramName, char *s);
bool  is192ByteTS(char *FileName);

#endif
