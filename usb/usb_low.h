//---------------------------------------------------------------------------

#ifndef usb_lowH
#define usb_lowH
//---------------------------------------------------------------------------

//#include "Main.h"
//#include "NssProWorker.h"

#define NMWCD_RESET_INT_PIPE				0x22200C
#define NMWCD_RESET_DATA_IN_PIPE			

#define NMWCD_SET_LOOPBACK_OFF				0x222010
#define NMWCD_SET_LOOPBACK_ON				0x222014

#define NMWCD_RESET_DATA_OUT_PIPE			0x222018
#define NMWCD_RESET_DEVICE					0x22201C

#define NMWCD_SET_XFER_SIZE					0x222020

#define NMWCD_READ							0x222024
#define NMWCD_WRITE							0x222028

#define NMWCD_QUERY_DEV_DESC_INFO			0x222030
#define NMWCD_QUERY_DMA_CHANNEL_AVAILABLE	0x222034
#define NMWCD_QUERY_IF_DESC_INFO			0x22203C
#define NMWCD_QUERY_MORE_DATA				0x000000

#define NMWCD_REGISTER_AT_RESPONSE_EVENT	0x222038
#define NMWCD_REGISTER_DRIVER_READY_EVENT	0x222044
#define NMWCD_REGISTER_REMOVE_EVENT			0x000000

#define NMWCD_EP0_PASSTHROUGH				0x222040


#define NUMBER_OF_SUPPORTED_DRIVERS			4

void UsbLow_GetError(char *lpszFunction);

HANDLE open_dev(UCHAR ucDriverID,char *pid);

short BB5UsbOpenConnectionA(HANDLE *hPipe,ULONG TimeOutVar,UCHAR dev);
void  BB5UsbCloseConnection(HANDLE hPipe);

short BB5UsbOpenDeadConnection(HANDLE *hPipe);
void  BB5UsbCloseDeadConnection(HANDLE hPipe);

short UsbLow_SendControlTransfer(HANDLE hPipe,UCHAR *out_d,ULONG out_s);
short UsbLow_ReadControlTransfer(HANDLE hPipe, UCHAR *out_d,ULONG out_s,UCHAR *pinBuf, UINT buffSize,ULONG *nBytesReturned);

short UsbLow_SendCommandViaControlPipe(HANDLE hPipe,UCHAR VendorCmd,UCHAR *out_d,ULONG out_s,bool encrypt);
short UsbLow_GetDataViaControlPipe(HANDLE hPipe, UCHAR VendorCmd,UCHAR *pinBuf, UINT buffSize,ULONG expected,ULONG *nBytesReturned,bool encrypt);
short UsbLow_GetDataViaControlPipeA(HANDLE hPipe, UCHAR VendorCmd,UCHAR *pinBuf, UINT buffSize,ULONG expected,ULONG *nBytesReturned,bool encrypt);

bool  BulkWriteToDevice(HANDLE hWrite, UCHAR *poutBuf, UINT buffSize,ULONG timeout);
bool  BulkReadFromDevice(HANDLE hWrite, UCHAR *pinBuf, UINT buffSize,ULONG *nBytesReturned);


#endif
