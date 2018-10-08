//---------------------------------------------------------------------------

#ifndef UpdateFlashH
#define UpdateFlashH
//---------------------------------------------------------------------------

short FbusSdio_WriteBlock(HANDLE hPipe,ULONG ulSectNumber,UCHAR *data);
short FbusSdio_WriteUIBlock(HANDLE hPipe,ULONG ulSectNumber,UCHAR *data);

#endif
