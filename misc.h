#ifndef MiscH
#define MiscH

void   DebugPrintHexArray(UCHAR *pArray,UINT aSize,char *chMessage);
void   DebugPrintInt(char *text,UINT val);

char *ReturnHexArray(UCHAR *pArray,UINT aSize);

bool AnyFileOpen(char *nPath, ULONG *pBuffer, ULONG *bReaded);

#endif
