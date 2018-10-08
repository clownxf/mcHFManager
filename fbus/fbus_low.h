//---------------------------------------------------------------------------

#ifndef fbus_lowH
#define fbus_lowH
//---------------------------------------------------------------------------

#define	MAX_BSI_VOLTAGE		0x20

short FbusLow_open_scni_device(HANDLE *hPipe);
void  FbusLow_close_scni_device(HANDLE hDev);

short FbusLow_SendCommandViaControlPipe(HANDLE hDev,UCHAR VendorCmd,UCHAR *out_d,ULONG out_s,bool encrypt,USHORT wValue);
short FbusLow_ReadDataViaControlPipe(HANDLE hDev,UCHAR VendorCmd,UCHAR *out_d,ULONG out_s,bool encrypt);

short FbusLow_WriteBulkPipe(HANDLE hDev,UCHAR Pipe,UCHAR *out_d,ULONG out_s,bool encrypt);
short FbusLow_ReadBulkPipe (HANDLE hDev,UCHAR Pipe,UCHAR *out_d,ULONG out_s,bool encrypt);

short FbusLow_DeviceWrite(HANDLE hDev,UCHAR menu,UCHAR *out_d,ULONG out_s);
short FbusLow_DeviceRead (HANDLE hDev,UCHAR menu,UCHAR *out_d,ULONG out_s,ULONG *ret);
short FbusLow_DeviceReadA(HANDLE hDev,UCHAR menu,UCHAR *out_d,ULONG out_s,ULONG *ret);

short FbusLow_OpenUartConnection(HANDLE *hPipe);
void  FbusLow_CloseUartConnection(HANDLE hPipe);

short FbusLow_OpenFlashConnection(HANDLE *hPipe);
void  FbusLow_CloseFlashConnection(HANDLE hPipe);

short FbusLow_OpenSdioConnection(HANDLE *hPipe);
short FbusLow_CloseSdioConnection(HANDLE hPipe);

short FbusLow_WaitHigh(HANDLE hDeadPipe,ULONG ackhdelay);
short FbusLow_WaitLow(HANDLE hDeadPipe,ULONG ackhdelay);

short FbusLow_SendToPhone(HANDLE hDeadPipe,UCHAR *buffer,ULONG ulSize,ULONG del_l,ULONG del_h);

#endif
