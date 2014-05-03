#ifndef __CWTAPAPILIB__
#define __CWTAPAPILIB__

#ifdef _MSC_VER
  #define __const const
#endif


// ============================================================================
//                               TAP-API-Lib
// ============================================================================
//#undef memcpy
//#undef memcmp
//#undef memset
#undef sprintf
//#define TAP_MemCpy    memcpy
//#define TAP_MemCmp    memcmp
//#define TAP_MemSet    memset
#define TAP_SPrint    snprintf

void HDD_Rename2(const char *FileName, const char *NewFileName, const char *Directory, bool RenameInfNav);
void HDD_Delete2(const char *FileName, const char *Directory, bool DeleteInfNav);
bool HDD_Exist2(char *FileName, const char *Directory);
bool HDD_GetFileSizeAndInode2(char *FileName, char *Directory, __ino64_t *OutCInode, __off64_t *OutFileSize);
bool HDD_StartPlayback2(char *FileName, const char *Directory);


#endif