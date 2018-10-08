//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include <stdio.h>
#include <setupapi.h>

#include "usb_low.h"
#include "fbus_low.h"
#include "misc.h"

//#include "EXECryptor.h"

extern	bool			abort_req;

const  GUID winUsbGuid = {0x58D07210 ,0x27C1 , 0x11DD , 0xBD , 0x0B , 0x08 ,0x00 ,0x20 ,0x0C ,0x9a ,0x66};

//---------------------------------------------------------------------------

#pragma package(smart_init)

// Print last error
void FbusLow_GetError(char *lpszFunction)
{
	LPVOID lpMsgBuf;
	char   lpDisplayBuf[300];
	DWORD  dw = GetLastError();

	FormatMessage(	FORMAT_MESSAGE_ALLOCATE_BUFFER |
					FORMAT_MESSAGE_FROM_SYSTEM,
					NULL,
					dw,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					(LPTSTR) &lpMsgBuf,
					0, NULL );

	sprintf(lpDisplayBuf,"%s failed with error %d: %s",lpszFunction, dw, lpMsgBuf);
	//OutputDebugString(lpDisplayBuf);
}

HANDLE FbusLow_open_one_device(IN     HDEVINFO                    	HardwareDeviceInfo,
								IN     PSP_INTERFACE_DEVICE_DATA   	DeviceInfoData,
								IN	   char 						*devName)
{
	PSP_INTERFACE_DEVICE_DETAIL_DATA     functionClassDeviceData = NULL;
	ULONG                                predictedLength = 0;
	ULONG                                requiredLength = 0;
	HANDLE								 hOut = INVALID_HANDLE_VALUE;

    //
	// allocate a function class device data structure to receive the
    // goods about this particular device.
    //
	SetupDiGetInterfaceDeviceDetail (
            HardwareDeviceInfo,
			DeviceInfoData,
			NULL, // probing so no output buffer yet
            0, // probing so output buffer length of zero
            &requiredLength,
            NULL); // not interested in the specific dev-node

    
	predictedLength = requiredLength;
	// sizeof (SP_FNCLASS_DEVICE_DATA) + 512;

	functionClassDeviceData =(_SP_DEVICE_INTERFACE_DETAIL_DATA_A *) malloc (predictedLength);
    functionClassDeviceData->cbSize = sizeof (SP_INTERFACE_DEVICE_DETAIL_DATA);

    //
    // Retrieve the information from Plug and Play.
	//
	if (! SetupDiGetInterfaceDeviceDetail (
			   HardwareDeviceInfo,
			   DeviceInfoData,
               functionClassDeviceData,
               predictedLength,
               &requiredLength,
			   NULL))
	{
		//OutputDebugString("err details");

		free( functionClassDeviceData );
		return INVALID_HANDLE_VALUE;
    }

	strcpy( devName,functionClassDeviceData->DevicePath) ;
	//OutputDebugString( "Attempting to open %s\n", devName );

    hOut = CreateFile (
				  functionClassDeviceData->DevicePath,
				  GENERIC_READ | GENERIC_WRITE,
                  FILE_SHARE_READ | FILE_SHARE_WRITE,
				  NULL, // no SECURITY_ATTRIBUTES structure
                  OPEN_EXISTING, // No special create flags
                  0x60000080, // No special attributes
                  NULL); // No template file

	if (INVALID_HANDLE_VALUE == hOut)
	{
		//char buff[200];
		//sprintf(buff,"FAILED to open %s\n", devName);
		//OutputDebugString(buff);
	}

	free( functionClassDeviceData );
	return hOut;
}

// Enumerate USB devices and open
HANDLE FbusLow_enum_and_open(void)
{
	HANDLE 						hOut = INVALID_HANDLE_VALUE;
	HDEVINFO 					hDevInfo;
	SP_DEVINFO_DATA 			DeviceInfoData;
	SP_INTERFACE_DEVICE_DATA 	deviceDevData;
	DWORD 						i;
	char 						completeDeviceName[256];

	deviceDevData.cbSize = sizeof (SP_INTERFACE_DEVICE_DATA);

	// Create a HDEVINFO with all present devices.
	hDevInfo = SetupDiGetClassDevs((LPGUID)&winUsbGuid,
		   0, // Enumerator
		   0,
		   DIGCF_PRESENT | DIGCF_INTERFACEDEVICE );

	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		   // Insert error handling here.
		   return INVALID_HANDLE_VALUE;
	}

	// Enumerate through all devices in Set.
	DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	for (i=0;SetupDiEnumDeviceInfo(hDevInfo,i,
		   &DeviceInfoData);i++)
	   {
		   DWORD DataT;
		   LPTSTR buffer = NULL;
		   DWORD buffersize = 0;

		   //
		   // Call function with null to begin with,
		   // then use the returned buffer size (doubled)
		   // to Alloc the buffer. Keep calling until
		   // success or an unknown failure.
		   //
		   //  Double the returned buffersize to correct
		   //  for underlying legacy CM functions that
		   //  return an incorrect buffersize value on
		   //  DBCS/MBCS systems.
		   //
		   while (!SetupDiGetDeviceRegistryProperty(
			   hDevInfo,
			   &DeviceInfoData,
			   SPDRP_DEVICEDESC,
			   &DataT,
			   (PBYTE)buffer,
			   buffersize,
			   &buffersize))
		   {
			   if (GetLastError() ==
				   ERROR_INSUFFICIENT_BUFFER)
			   {
				   // Change the buffer size.
				   if (buffer) LocalFree(buffer);
				   // Double the size to avoid problems on
				   // W2k MBCS systems per KB 888609.
				   buffer = (char *)LocalAlloc(LPTR,buffersize * 2);
			   }
			   else
			   {
				   // Insert error handling here.
				   break;
			   }
		   }
		   //OutputDebugString(buffer);

		   // Check for match
		   if(strcmp(buffer,"mcHF bootloader, M0NKA 2014") == 0)
		   {
				if (buffer) LocalFree(buffer);

				// Get interface detail
				if(SetupDiEnumDeviceInterfaces(hDevInfo,0,(LPGUID)&winUsbGuid,i,&deviceDevData))
				{
					//OutputDebugString("about to open...");

					// Open device
					hOut = FbusLow_open_one_device (hDevInfo, &deviceDevData, completeDeviceName);
					if(hOut != INVALID_HANDLE_VALUE)
					{
						//OutputDebugString("openned");

						//  Cleanup
						SetupDiDestroyDeviceInfoList(hDevInfo);

						// Found
						return hOut;
					}
				}
		   }

		   if (buffer) LocalFree(buffer);
	   }

	   //  Cleanup
	   SetupDiDestroyDeviceInfoList(hDevInfo);

	   return INVALID_HANDLE_VALUE;
}
// Try to open device
//
short FbusLow_open_scni_device(HANDLE *hPipe)
{
	HANDLE 	hSCNI = INVALID_HANDLE_VALUE;
	USHORT	InBuffer1 = 0x100;
	USHORT	OuBuffer1 = 0x000;
	ULONG	InBuffer2;
	UCHAR	OuBuffer2[100];

	ULONG	nBytes;

	// Open device
	hSCNI = FbusLow_enum_and_open();
	if(hSCNI == INVALID_HANDLE_VALUE)
		return 1;

	//OutputDebugString("dev opened");

	// First call - ??
	if(DeviceIoControl(hSCNI,0x350C068,&InBuffer1,2,&OuBuffer1,2,&nBytes,NULL) == 0)
	{
		//OutputDebugString( "call1 err\n" );
		CloseHandle(hSCNI);
		return 2;
	}
	//DebugPrintInt("res:",OuBuffer1);

	// Set menu
	InBuffer2 = 0x00000001;

	// GetDescriptor (Device)
	if(DeviceIoControl(hSCNI,0x350C004,&InBuffer2,4,OuBuffer2,0x12,&nBytes,NULL) == 0)
	{
		//OutputDebugString( "GetDescriptor (Device) err\n" );
		CloseHandle(hSCNI);
		return 3;
	}
	//DebugPrintHexArray(OuBuffer2,nBytes,"GetDescriptor (Device):");

	// Update call menu
    InBuffer2 = 0x00000002;

	// GetDescriptor (Configuration) - to get size
	if(DeviceIoControl(hSCNI,0x350C004,&InBuffer2,4,OuBuffer2,9,&nBytes,NULL) == 0)
	{
		//OutputDebugString( "GetDescriptor (Configuration) size err\n" );
		CloseHandle(hSCNI);
		return 4;
	}
	//DebugPrintHexArray(OuBuffer2,nBytes,"GetDescriptor (Configuration):");

	// GetDescriptor (Configuration) - full
	if(DeviceIoControl(hSCNI,0x350C004,&InBuffer2,4,OuBuffer2,OuBuffer2[2],&nBytes,NULL) == 0)
	{
		//OutputDebugString( "GetDescriptor (Configuration) full err\n" );
		CloseHandle(hSCNI);
		return 5;
	}
	//DebugPrintHexArray(OuBuffer2,nBytes,"GetDescriptor (Configuration):");

	// Some init ?
	if(DeviceIoControl(hSCNI,0x350C03C,NULL,0,NULL,0,&nBytes,NULL) == 0)
	{
		//OutputDebugString( "cmd x err\n" );
		CloseHandle(hSCNI);
		return 6;
	}

	// Return handle
	*hPipe = hSCNI;

	return 0;
}

// Close FBUS device
void FbusLow_close_scni_device(HANDLE hDev)
{
	if(hDev != INVALID_HANDLE_VALUE) CloseHandle(hDev);
}

// Send command using control pipe
short FbusLow_SendCommandViaControlPipe(HANDLE hDev,UCHAR VendorCmd,UCHAR *out_d,ULONG out_s,bool encrypt,USHORT wValue)
{
	ULONG		nBytes = 0;
	UCHAR 		transfer[20];
	OVERLAPPED	lo_w;
	ULONG		err;

	//OutputDebugString("NSS: FbusLow_SendCommandViaControlPipe in");

	if(hDev == INVALID_HANDLE_VALUE)
		return 1;

	// Encrypt payload
	if((encrypt) && (out_s))
	{
		//DebugPrintHexArray(temp + 8,out_s,"out data plain:");
		//RC4_Crypt(temp + 8,out_s);
		//DebugPrintHexArray(temp + 8,out_s,"out data encr:");
	}

	// Prepare command
	memset(transfer,0,sizeof(transfer));

	transfer[1] = 0x40;
	transfer[2] = VendorCmd;

	// Extra arguments needed for eMMC speed init
	transfer[3] = wValue & 0xFF;
	transfer[4] = wValue >> 8;

	transfer[7] = out_s & 0xFF;
	transfer[8] = out_s >> 8;

	//DebugPrintHexArray(transfer,9,"out transfer:");

	memset(&lo_w, 0, sizeof(lo_w));
	lo_w.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	ResetEvent(lo_w.hEvent);

	DeviceIoControl(hDev,0x350C03A,transfer,9,out_d,out_s,NULL,&lo_w);

	err = GetLastError();
	if (err != ERROR_IO_PENDING)
	{
		if(err != ERROR_SUCCESS)
		{
			CloseHandle(lo_w.hEvent);
			//DebugPrintInt( "FbusLow_SendCommandViaControlPipe->not pending error",err);
			return 2;
		}
	}

	while (1)
	{
		//OutputDebugString( "wait...");
		if(abort_req)
		{
			CancelIo(hDev);
			CloseHandle(lo_w.hEvent);
			//OutputDebugString( "FbusLow_SendCommandViaControlPipe->User abort!");
			return 100;
		}

		if(WaitForSingleObject(lo_w.hEvent,500) != 0)
			continue;	// wait more

		//OutputDebugString( "get");

		if (GetOverlappedResult(hDev, &lo_w, &nBytes, FALSE))
			break;

		//OutputDebugString( "check");

		err = GetLastError();
		if(err != ERROR_IO_INCOMPLETE)
		{
			CloseHandle(lo_w.hEvent);
			//DebugPrintInt("FbusLow_SendCommandViaControlPipe->err diff than incomplete:",err);
			return 3;
		}
	}
	CloseHandle(lo_w.hEvent);

	//OutputDebugString("NSS: FbusLow_SendCommandViaControlPipe out");
	return 0;
}

// Send command using control pipe
short FbusLow_ReadDataViaControlPipe(HANDLE hDev,UCHAR VendorCmd,UCHAR *out_d,ULONG out_s,bool encrypt)
{
	ULONG		nBytes = 0;
	UCHAR 		transfer[20];
	OVERLAPPED	lo_w;
	ULONG		err;

	if(hDev == INVALID_HANDLE_VALUE)
		return 1;

	// Prepare command
	memset(transfer,0,sizeof(transfer));

	transfer[1] = 0xC0;
	transfer[2] = VendorCmd;

	transfer[7] = out_s & 0xFF;
	transfer[8] = out_s >> 8;

	//DebugPrintHexArray(transfer,9,"out transfer:");

	memset(&lo_w, 0, sizeof(lo_w));
	lo_w.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	ResetEvent(lo_w.hEvent);

	DeviceIoControl(hDev,0x350C03A,transfer,9,out_d,out_s,&nBytes,&lo_w);

	/*if (GetLastError() != ERROR_IO_PENDING)
	{
		OutputDebugString( "FbusLow_ReadDataViaControlPipe->err 2");
		CloseHandle(lo_w.hEvent);
		return 2;
	}

	WaitForSingleObject(lo_w.hEvent, 10000); // orig timeout 1000, use proper loop here ???
	if(!GetOverlappedResult(hDev, &lo_w, &nBytes, FALSE))
	{
		OutputDebugString( "FbusLow_ReadDataViaControlPipe->err 3");
		CloseHandle(lo_w.hEvent);
		return 3;
	}
	CloseHandle(lo_w.hEvent);*/

	err = GetLastError();
	if (err != ERROR_IO_PENDING)
	{
		if(err != ERROR_SUCCESS)
		{
			CloseHandle(lo_w.hEvent);
			//OutputDebugString( "FbusLow_ReadDataViaControlPipe->err 2");
			return 2;
		}
	}

	while (1)
	{
		//OutputDebugString( "wait...");
		if(abort_req)
		{
			CancelIo(hDev);
			CloseHandle(lo_w.hEvent);
			//OutputDebugString( "FbusLow_ReadDataViaControlPipe->User abort!");
			return 100;
		}

		if(WaitForSingleObject(lo_w.hEvent,500) != 0)
			continue;	// wait more

		//OutputDebugString( "get");

		if (GetOverlappedResult(hDev, &lo_w, &nBytes, FALSE))
			break;

		//OutputDebugString( "check");

		err = GetLastError();
		if(err != ERROR_IO_INCOMPLETE)
		{
			CancelIo(hDev);
			CloseHandle(lo_w.hEvent);
			//DebugPrintInt("FbusLow_ReadDataViaControlPipe->err diff than incomplete:",err);
			return 3;
		}
	}
	CloseHandle(lo_w.hEvent);

	//DebugPrintInt("ret",nBytes);

	// All bytes here ?
	if(nBytes != out_s)
	{
		//OutputDebugString( "FbusLow_ReadDataViaControlPipe->err 4");
		return 4;
	}

	// Decrypt payload
	if((encrypt) && (nBytes))
	{
		//DebugPrintHexArray(temp + 8,out_s,"out data plain:");
		//RC4_Crypt(temp + 8,out_s);
		//DebugPrintHexArray(temp + 8,out_s,"out data encr:");
	}

	return 0;
}

// Send data via bulk pipe
short FbusLow_WriteBulkPipe(HANDLE hDev,UCHAR Pipe,UCHAR *out_d,ULONG out_s,bool encrypt)
{
	ULONG		nBytes = 0;
	UCHAR 		transfer[2];
	OVERLAPPED	lo_w;

	if(hDev == INVALID_HANDLE_VALUE)
		return 1;

	// Encrypt data
	if(encrypt)
	{
		//DebugPrintHexArray(temp + 8,out_s,"out data plain:");
		//RC4_Crypt(temp + 8,out_s);
		//DebugPrintHexArray(temp + 8,out_s,"out data encr:");
	}

	// Load arguments
	transfer[0] = 0;      // ??
	transfer[1] = Pipe;

	memset(&lo_w, 0, sizeof(lo_w));
	lo_w.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	DeviceIoControl(hDev,0x3508021,transfer,2,out_d,out_s,NULL,&lo_w);

	if (GetLastError() != ERROR_IO_PENDING)
	{
		//OutputDebugString( "err 2");
		CloseHandle(lo_w.hEvent);
		return 2;
	}

	WaitForSingleObject(lo_w.hEvent, 10000);  // orig timeout 1000, use proper loop here ???
	if(!GetOverlappedResult(hDev, &lo_w, &nBytes, FALSE))
	{
		//OutputDebugString( "err 3");
		CloseHandle(lo_w.hEvent);
		return 3;
	}
	CloseHandle(lo_w.hEvent);

	return 0;
}

// Read data via bulk pipe
short FbusLow_ReadBulkPipe(HANDLE hDev,UCHAR Pipe,UCHAR *out_d,ULONG out_s,bool encrypt)
{
	ULONG		nBytes = 0;
	UCHAR 		transfer[2];
	OVERLAPPED	lo_w;

	if(hDev == INVALID_HANDLE_VALUE)
		return 1;

	// Load arguments
	transfer[0] = 0;      // ??
	transfer[1] = Pipe;

	memset(&lo_w, 0, sizeof(lo_w));
	lo_w.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	DeviceIoControl(hDev,0x350401E,transfer,2,out_d,out_s,NULL,&lo_w);

	if (GetLastError() != ERROR_IO_PENDING)
	{
		//OutputDebugString( "err 2");
		CloseHandle(lo_w.hEvent);
		return 2;
	}

	WaitForSingleObject(lo_w.hEvent, 1000);
	if(!GetOverlappedResult(hDev, &lo_w, &nBytes, FALSE))
	{
		//OutputDebugString( "err 3");
		CloseHandle(lo_w.hEvent);
		return 3;
	}
	CloseHandle(lo_w.hEvent);

	//DebugPrintInt("read bulk ret",nBytes);

	// All bytes here ?
	if(nBytes != out_s)
		return 4;

	// Decrypt payload
	if((encrypt) && (nBytes))
	{
		//DebugPrintHexArray(temp + 8,out_s,"out data plain:");
		//RC4_Crypt(temp + 8,out_s);
		//DebugPrintHexArray(temp + 8,out_s,"out data encr:");
	}

	return 0;
}

// Complete device write transfer
short FbusLow_DeviceWrite(HANDLE hDev,UCHAR menu,UCHAR *out_d,ULONG out_s)
{
	UCHAR	buff[20];

	// Clear buffer
	memset(buff,0,8);

	// Insert bulk transfer details - size, encryption key, etc
	buff[0] = out_s >> 24;
	buff[1] = out_s >> 16;
	buff[2] = out_s >>  8;
	buff[3] = out_s >>  0;

	// Details of transfer via control transfer
	if(FbusLow_SendCommandViaControlPipe(hDev,menu,buff,8,false,0) != 0)
	{
		//OutputDebugString("error control");
		return 1;
	}

	// Device write - control+bulk
	if(FbusLow_WriteBulkPipe(hDev,0x01,out_d,out_s,false) != 0)
	{
		//OutputDebugString("error bulk");
		return 2;
	}

	return 0;
}

// Complete device read transfer - count determined by host
short FbusLow_DeviceRead(HANDLE hDev,UCHAR menu,UCHAR *out_d,ULONG out_s,ULONG *ret)
{
	UCHAR	buff[20];
	ULONG	bulk_size;

	// Clear buffer
	memset(buff,0,8);

	// Insert bulk transfer details - size, encryption key, etc
	buff[0] = out_s >> 24;
	buff[1] = out_s >> 16;
	buff[2] = out_s >>  8;
	buff[3] = out_s >>  0;

	// Details of transfer via control transfer
	if(FbusLow_SendCommandViaControlPipe(hDev,menu,buff,8,false,0) != 0)
	{
		//OutputDebugString("NSS: error control");
		return 1;
	}

	// Read the bulk data
	if(FbusLow_ReadBulkPipe(hDev,0x82,out_d,out_s,false) != 0)
	{
		//OutputDebugString("NSS: error bulk");
		return 2;
	}

	*ret = out_s;
	return 0;
}

// Complete device read transfer - count determined by device
short FbusLow_DeviceReadA(HANDLE hDev,UCHAR menu,UCHAR *out_d,ULONG out_s,ULONG *ret)
{
	UCHAR	buff[20];
	ULONG	bulk_size;
	UCHAR	*p_buff = out_d;
	bool	need_free = false;

	// Get details of transfer via control transfer
	if(FbusLow_ReadDataViaControlPipe(hDev,menu,buff,8,false) != 0)
	{
		//OutputDebugString("FbusLow_DeviceReadA->error control");
		return 1;
	}

	// Get size
	bulk_size  = buff[0] << 24;
	bulk_size |= buff[1] << 16;
	bulk_size |= buff[2] <<  8;
	bulk_size |= buff[3] <<  0;

	// No bulk part, still success
	if(bulk_size == 0)
	{
		// Copy control buffer
		memcpy(out_d,buff,8);

		*ret = bulk_size;
		return 0;
	}

	// Test buffer size
	if(bulk_size > out_s)
	{
		// Allocate buffer and read in it
		p_buff = (UCHAR *)malloc(bulk_size);
		need_free = true;
	}

	// Read the bulk data
	if(FbusLow_ReadBulkPipe(hDev,0x82,p_buff,bulk_size,false) != 0)
	{
		//OutputDebugString("FbusLow_DeviceReadA->error bulk");
		return 2;
	}

	// Copy and free
	if(need_free)
	{
		memcpy(out_d,p_buff,out_s);
		free(p_buff);
	}

	*ret = bulk_size;
	return 0;
}

// Open device in UART mode
short FbusLow_OpenUartConnection(HANDLE *hPipe)
{
	short res;

	// Open device
	res = FbusLow_open_scni_device(hPipe);
	if(res == 0)
	{
		// Set UART mode
		if(FbusLow_SendCommandViaControlPipe(*hPipe,0x30,NULL,0,false,0) != 0)
			return 5;
	}

	return res;
}

// Close device in UART mode
void FbusLow_CloseUartConnection(HANDLE hPipe)
{
	// Exit UART mode
	FbusLow_SendCommandViaControlPipe(hPipe,0x31,NULL,0,false,0);

	// Close device
	FbusLow_close_scni_device(hPipe);
}

// Open device in Flash mode
short FbusLow_OpenFlashConnection(HANDLE *hPipe)
{
	short res;

	// Open device
	res = FbusLow_open_scni_device(hPipe);
	if(res == 0)
	{
		// Set Flash mode
		if(FbusLow_SendCommandViaControlPipe(*hPipe,0x40,NULL,0,false,0) != 0)
			return 5;
	}

	return res;
}

// Close device in Flash mode
void FbusLow_CloseFlashConnection(HANDLE hPipe)
{
	// Exit Flash mode
	FbusLow_SendCommandViaControlPipe(hPipe,0x41,NULL,0,false,0);

	// Close device
	FbusLow_close_scni_device(hPipe);
}

// Open device in SDIO mode
short FbusLow_OpenSdioConnection(HANDLE *hPipe)
{
	unsigned short res,a;
	UCHAR key[16];

	srand(time(NULL));

	// Open device
	res = FbusLow_open_scni_device(hPipe);
	if(res == 0)
	{
		// Generate RND word and pass as wValue in control transfer
		a = rand();
		//DebugPrintInt("rnd val:",a);

		// Set Flash mode
		if(FbusLow_SendCommandViaControlPipe(*hPipe,0x50,NULL,0,false,a) != 0)
			return 5;
	}

	return res;
}

// Close device in SDIO mode
short FbusLow_CloseSdioConnection(HANDLE hPipe)
{
	short res;

	// Exit Flash mode
	res = FbusLow_SendCommandViaControlPipe(hPipe,0x51,NULL,0,false,0);

	// Close device
	FbusLow_close_scni_device(hPipe);

	return res;
}

// Wait high level on Tx line
short FbusLow_WaitHigh(HANDLE hDeadPipe,ULONG ackhdelay)
{
	unsigned char finished,buff[8];
	unsigned long i;

	for (i=0,finished = 0;i<ackhdelay && !finished;i++)
	{
		if(abort_req)
			return false;

		//finished = (AT91F_PIO_GetInput(AT91C_BASE_PIOA) & PHONE_TX);

		// Read status
		if(FbusLow_ReadDataViaControlPipe(hDeadPipe,0xB9,buff,sizeof(buff),false) != 0)
			return false;

		//DebugPrintHexArray(buff,8,"");

		// Tx status
		finished = buff[4];

		if (!finished)
			continue;

		//finished = (AT91F_PIO_GetInput(AT91C_BASE_PIOA) & PHONE_TX);

		// Read status
		if(FbusLow_ReadDataViaControlPipe(hDeadPipe,0xB9,buff,sizeof(buff),false) != 0)
			return false;

		//DebugPrintHexArray(buff,8,"");

		// Tx status
		finished = buff[4];
	}

	return (finished);
}

// Wait low level on Tx line
short FbusLow_WaitLow(HANDLE hDeadPipe,ULONG ackhdelay)
{
	unsigned char finished,buff[8];
	unsigned long i;

	for (i=0,finished = 0;i<ackhdelay && !finished;i++)
	{
    	if(abort_req)
			return false;
			
		//finished = !(AT91F_PIO_GetInput(AT91C_BASE_PIOA) & PHONE_TX);

		// Read status
		if(FbusLow_ReadDataViaControlPipe(hDeadPipe,0xB9,buff,sizeof(buff),false) != 0)
			return false;

		//DebugPrintHexArray(buff,8,"");

		// Tx status
		finished = !buff[4];

		if (!finished)
			continue;

		//finished = !(AT91F_PIO_GetInput(AT91C_BASE_PIOA) & PHONE_TX);

		// Read status
		if(FbusLow_ReadDataViaControlPipe(hDeadPipe,0xB9,buff,sizeof(buff),false) != 0)
			return false;

		//DebugPrintHexArray(buff,8,"");

		// Tx status
		finished = !buff[4];
	}

	return (finished);
}

short FbusLow_SendToPhone(HANDLE hDeadPipe,UCHAR *buffer,ULONG ulSize,ULONG del_l,ULONG del_h)
{
	UCHAR buff[10];
	ULONG ret;

	// Send data part
	if(FbusLow_DeviceWrite(hDeadPipe,0x4A,buffer,ulSize) != 0)
		return 1;

	// Wait low
	if (!FbusLow_WaitLow(hDeadPipe,del_l))
		return 2;

	buff[0] = 0;

	// Dummy byte
	if(FbusLow_DeviceWrite(hDeadPipe,0x4A,buff,1) != 0)
		return 3;

	// Wait high
	if (!FbusLow_WaitHigh(hDeadPipe,del_h))
		return 4;

	return 0;
}



