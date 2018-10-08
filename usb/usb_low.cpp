//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <stdio.h>
#include <setupapi.h>

#include "nokia.h"
#include "usbdi.h"

#include "usb_low.h"
//#include "misc.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)

char completeDeviceName[256] = "";  //generated from the GUID registered by the driver itself

// FBUS connection publics
OVERLAPPED	lo_r;
OVERLAPPED	lo_w;
HANDLE		hEvent_r;
HANDLE		hEvent_w;

// Dead USB connection publics
HANDLE 		hEvent1_dead;
HANDLE 		hEvent2_dead;
//OVERLAPPED	lo_w_dead;
//OVERLAPPED	lo_r_dead;

extern	bool			abort_req;

// Print last error
void UsbLow_GetError(char *lpszFunction)
{
#ifdef DEBUG_BUILD
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
#endif
}

int UsbLow_GetDevicePath 		(	IN 	HDEVINFO                 	HardwareDeviceInfo,
									IN	PSP_INTERFACE_DEVICE_DATA	DeviceInfoData,
							   		IN 	char 						*devName)
{
	PSP_INTERFACE_DEVICE_DETAIL_DATA     functionClassDeviceData = NULL;
	ULONG                                predictedLength = 0;
	ULONG                                requiredLength = 0;

	//
	// allocate a function class device data structure to receive the
	// goods about this particular device.
	//
	if(!SetupDiGetInterfaceDeviceDetail (	HardwareDeviceInfo,
											DeviceInfoData,
											NULL, // probing so no output buffer yet
											0, // probing so output buffer length of zero
											&requiredLength,
											NULL))
	 {

		if(GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		{
			//UsbLow_GetError("OpenCompositeDevice->SetupDiGetInterfaceDeviceDetail 1");
			return 1;
        }
	 }

	predictedLength = requiredLength;
	//predictedLength =  1024;

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
		//UsbLow_GetError("OpenCompositeDevice->SetupDiGetInterfaceDeviceDetail 2");

		free( functionClassDeviceData );
		return 2;
	}
	//OutputDebugString(functionClassDeviceData->DevicePath);

	// Return path
	strcpy( devName,functionClassDeviceData->DevicePath) ;

	free( functionClassDeviceData );
	return 0;
}


HANDLE OpenOneDevice (IN       HDEVINFO                    	HardwareDeviceInfo,
					  IN       PSP_INTERFACE_DEVICE_DATA   	DeviceInfoData,
					  IN	   char 						*devName
					  )

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
	//OutputDebugString( devName );

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

// Enumerate USB devices
HANDLE OpenUsbDeviceA(LPGUID  pGuid, char *outNameBuf,bool skip_ui,char *pid)
{
	HANDLE 						hOut = INVALID_HANDLE_VALUE;
	HDEVINFO 					hDevInfo;
	SP_DEVINFO_DATA 			DeviceInfoData;
	SP_INTERFACE_DEVICE_DATA 	deviceDevData;
	DWORD 						i;
	AnsiString					asSelection = "";
	int 						index;

	// Get selection
	//index = Form1->sListBox2->ItemIndex;
	//if(index != -1)
	//{
	//	asSelection = Form1->sListBox2->Items->Strings[index];
		//OutputDebugString(asSelection.c_str());
	//}

	deviceDevData.cbSize = sizeof (SP_INTERFACE_DEVICE_DATA);

	// Create a HDEVINFO with all present devices.
	hDevInfo = SetupDiGetClassDevs(	pGuid,
									0, // Enumerator
									0,
									DIGCF_PRESENT | DIGCF_INTERFACEDEVICE );

	if(hDevInfo == INVALID_HANDLE_VALUE)
	{
		return hOut;
	}

	// Enumerate through all devices
	DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	for (i=0;SetupDiEnumDeviceInfo(hDevInfo,i,&DeviceInfoData);i++)
	{
		DWORD 	DataT;
		LPTSTR 	buffer = NULL;
		DWORD 	buffersize = 0;

		while (!SetupDiGetDeviceRegistryProperty(	hDevInfo,
													&DeviceInfoData,
													SPDRP_DEVICEDESC,
													&DataT,
													(PBYTE)buffer,
													buffersize,
													&buffersize))
		{
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
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

		// Do we open the first found, or user selection ?
		// Also on flashing protocol we ignore list box selection
		if((asSelection != "") && (!skip_ui))
		{
			if(strstr(asSelection.c_str(),buffer) == 0)
			{
				//OutputDebugString("skip this device");
                if (buffer) LocalFree(buffer);
				continue;
			}
		}
		//OutputDebugString("name:");
		//OutputDebugString(buffer);

		// Get interface detail
		if(SetupDiEnumDeviceInterfaces(	hDevInfo,
										0, 			// We don't care about specific PDOs
										pGuid,
										i,          // dev index ?
										&deviceDevData))
		{
			//OutputDebugString("about to open...");

			// Open device
			hOut = OpenOneDevice (hDevInfo, &deviceDevData, outNameBuf);
			if(hOut != INVALID_HANDLE_VALUE)
			{
				// Are we looking for specific PID ?
				// Case Lumia 900, where two devices with same USB name show up, just PID is different
				if(pid)
				{
					//OutputDebugString("NSS: Request to look for specific PID on usb open");

					// Do we have match ?
					if(strstr(outNameBuf,pid) == 0)
					{
						//OutputDebugString("NSS: no match, check next");

						CloseHandle(hOut);
						if (buffer) LocalFree(buffer);
						continue;
					}
				}

				//  Cleanup
				SetupDiDestroyDeviceInfoList(hDevInfo);
				if (buffer) LocalFree(buffer);

				return hOut;
			}
		}

		if (buffer) LocalFree(buffer);
	}

	//if ( GetLastError()!=NO_ERROR &&
	//		GetLastError()!=ERROR_NO_MORE_ITEMS )
	//{
	// Insert error handling here.
	//	   return hOut;
	//}

	//  Cleanup
	SetupDiDestroyDeviceInfoList(hDevInfo);

	return hOut;
}

HANDLE OpenUsbDevice(LPGUID  pGuid, char *outNameBuf,bool skip_ui)
{
   ULONG 					NumberDevices;
   HANDLE 					hOut = INVALID_HANDLE_VALUE;
   HDEVINFO                 hardwareDeviceInfo;
   SP_INTERFACE_DEVICE_DATA deviceInfoData;
   ULONG                    i;
   BOOLEAN                  done;
   PUSB_DEVICE_DESCRIPTOR   usbDeviceInst;
   PUSB_DEVICE_DESCRIPTOR	*UsbDevices = &usbDeviceInst;

   *UsbDevices = NULL;
   NumberDevices = 0;

   //
   // Open a handle to the plug and play dev node.
   // SetupDiGetClassDevs() returns a device information set that contains info on all
   // installed devices of a specified class.
   //
   hardwareDeviceInfo = SetupDiGetClassDevs (
						   pGuid,
						   NULL, // Define no enumerator (global)
						   NULL, // Define no
						   (DIGCF_PRESENT | // Only Devices present
							DIGCF_INTERFACEDEVICE)); // Function class devices.

   //
   // Take a wild guess at the number of devices we have;
   // Be prepared to realloc and retry if there are more than we guessed
   //
   NumberDevices = 4;
   done = FALSE;
   deviceInfoData.cbSize = sizeof (SP_INTERFACE_DEVICE_DATA);

	i = 0;
	while (!done)
	{
		NumberDevices *= 2;

		if (*UsbDevices)
		{
			*UsbDevices =(_USB_DEVICE_DESCRIPTOR *)realloc (*UsbDevices, (NumberDevices * sizeof (USB_DEVICE_DESCRIPTOR)));
		}
		else
		{
			*UsbDevices = (_USB_DEVICE_DESCRIPTOR *)calloc (NumberDevices, sizeof (USB_DEVICE_DESCRIPTOR));
		}

		if(NULL == *UsbDevices)
		{
			// SetupDiDestroyDeviceInfoList destroys a device information set
			// and frees all associated memory.

			//OutputDebugString("err 1");

			SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
			return INVALID_HANDLE_VALUE;
		}

		usbDeviceInst = *UsbDevices + i;

//		DebugPrintInt("count",NumberDevices);
		for (; i < NumberDevices; i++)
		{
			//DebugPrintInt("check dev no",i);

			// SetupDiEnumDeviceInterfaces() returns information about device interfaces
			// exposed by one or more devices. Each call returns information about one interface;
			// the routine can be called repeatedly to get information about several interfaces
			// exposed by one or more devices.
			if (SetupDiEnumDeviceInterfaces	(hardwareDeviceInfo,
											0, // We don't care about specific PDOs
											pGuid,
											i,
											&deviceInfoData))
			{

				hOut = OpenOneDevice (hardwareDeviceInfo, &deviceInfoData, outNameBuf);
				if(hOut != INVALID_HANDLE_VALUE)
				{
					//OutputDebugString("open one device ok");
					done = TRUE;
					break;
				}
				//else
				//	OutputDebugString("open one device err");
			}
			else
			{
				if (ERROR_NO_MORE_ITEMS == GetLastError())
				{
					//OutputDebugString("no more items");
					done = TRUE;
					break;
				}
				//else
				//	GetError("enum err");
			}
		}
   }

   NumberDevices = i;

   // SetupDiDestroyDeviceInfoList() destroys a device information set
   // and frees all associated memory.
   SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
   free ( *UsbDevices );

   return hOut;
}

/*BOOL GetUsbDeviceFileName( LPGUID  pGuid, char *outNameBuf)
{
	HANDLE hDev = OpenUsbDevice( pGuid, outNameBuf );
	if ( hDev != INVALID_HANDLE_VALUE )
	{
		CloseHandle( hDev );
		return TRUE;
	}
	return FALSE;
} */

HANDLE open_dev(UCHAR ucDriverID,char *pid)
{
	HANDLE hDEV = INVALID_HANDLE_VALUE;
	char temp[50];

	sprintf(temp,"id: %d",ucDriverID);
	//OutputDebugString(temp);

	// ADSP driver
	if(ucDriverID == 1)
		hDEV = OpenUsbDevice( (LPGUID)&GUID_CLASS_HHKUSB_DRIVER, completeDeviceName,false);

	// ADL driver
	if(ucDriverID == 2)
	{
		hDEV = OpenUsbDeviceA((LPGUID)&GUID_CLASS_NMWCD_DRIVER, completeDeviceName,false,pid);
		if(hDEV != INVALID_HANDLE_VALUE)
		{
			//#if _DEBUG
			//OutputDebugString("name:");
			//OutputDebugString(completeDeviceName);
			//#endif

			// Check for valid device
			/*if((strstr(completeDeviceName,"&pid_04") == 0) &&
			   (strstr(completeDeviceName,"&pid_05") == 0) &&
				(strstr(completeDeviceName,"&pid_00") == 0))
			{
				CloseHandle(hDEV);
				return INVALID_HANDLE_VALUE;
			} */

			// Check for valid device
			if(strstr(completeDeviceName,"&vid_0421") != 0)
			{
				CloseHandle(hDEV);
				return INVALID_HANDLE_VALUE;
			}
		}
		//else
		//	OutputDebugString("err 44");
	}

	// BB5 Phone Parent Driver
	if(ucDriverID == 3)
	{
		// Open device
		//hDEV = OpenUsbDevice((LPGUID)&GUID_CLASS_BB5PARENT_DRIVER, completeDeviceName,true);
		hDEV = OpenUsbDeviceA((LPGUID)&GUID_CLASS_BB5PARENT_DRIVER, completeDeviceName,true,pid);
		if(hDEV != INVALID_HANDLE_VALUE)
		{
			//#if _DEBUG
			//OutputDebugString(completeDeviceName);
			//#endif

			// Check for valid device
			if(strstr(completeDeviceName,"_0106") == 0)
			{
				CloseHandle(hDEV);
				return INVALID_HANDLE_VALUE;
			}
		}
	}

	// JSON driver
	if(ucDriverID == 4)
	{
		hDEV = OpenUsbDeviceA((LPGUID)&GUID_CLASS_NOKIA_WIN_DRIVER_WP7, completeDeviceName,false,pid);
		if(hDEV != INVALID_HANDLE_VALUE)
		{
			//OutputDebugString("name:");
			//OutputDebugString(completeDeviceName);

			// Check for valid device
			if(strstr(completeDeviceName,"&vid_0421") != 0)

			//if(strstr(completeDeviceName,"&pid_05ED") != 0)
			//if(strstr(completeDeviceName,"&pid_065C") != 0)

			{
				CloseHandle(hDEV);
				return INVALID_HANDLE_VALUE;
			}
		}
		else
		{
			//OutputDebugString("not wp7, will try wp8 class...");

			// Maybe WP8 ?
			hDEV = OpenUsbDeviceA((LPGUID)&GUID_CLASS_NOKIA_WIN_DRIVER_WP8, completeDeviceName,false,pid);
			if(hDEV != INVALID_HANDLE_VALUE)
			{
				//OutputDebugString("name:");
				//OutputDebugString(completeDeviceName);

				// Check for valid device
				if(strstr(completeDeviceName,"&vid_0421") != 0)
				{
					CloseHandle(hDEV);
					return INVALID_HANDLE_VALUE;
				}
			}
		}
	}

	//if (hDEV == INVALID_HANDLE_VALUE) {
	//	OutputDebugString("Failed to open (%s) = %d", completeDeviceName, GetLastError());
	//}// else {
	//	OutputDebugString("DeviceName = (%s)\n", completeDeviceName);
	//}

	return hDEV;
}

// Open FBUS connection
short BB5UsbOpenConnectionA(HANDLE *hPipe,ULONG TimeOutVar,UCHAR dev)
{
	ULONG		i,nBytes,dummy;
	HANDLE		hDEV = INVALID_HANDLE_VALUE; // device handle

	//#if _DEBUG
	//OutputDebugString( "Open connection...\n" );
	//#endif

	// loop till device is plugged
	for(i = 0; i <= TimeOutVar;i++)
	{
		if(abort_req)
			return 100;

	   // exit if timeout expire
	   if(i == TimeOutVar){

		 //#if _DEBUG
		 //OutputDebugString( "Timeout reached! Exit.\n" );
		 //#endif

		 return 1;
	   }

	   // open device
	   hDEV = open_dev(dev,NULL);

	   // check for valid Handle
	   if(hDEV != INVALID_HANDLE_VALUE)
		 break;

	   //#if _DEBUG
	   //OutputDebugString( "Waiting...\n" );
	   //#endif

	   // Wait 1 sec
	   Sleep(300);
	}

	//#if _DEBUG
	//OutputDebugString( "Connection to ADL server established.\n" );
	//#endif

	lo_w.hEvent			= NULL;
	lo_w.Offset			= NULL;
	lo_w.OffsetHigh		= NULL;
	lo_w.Internal		= NULL;
	lo_w.InternalHigh	= NULL;

	lo_w.hEvent			= NULL;       // needs to be read ????
	lo_w.Offset			= NULL;
	lo_w.OffsetHigh		= NULL;
	lo_w.Internal		= NULL;
	lo_w.InternalHigh	= NULL;

	hEvent_w = CreateEvent(	NULL,    // default security attribute
							NULL,    // manual-reset event
							NULL,    // initial state = signaled
							NULL);   // unnamed event object


	hEvent_r = CreateEvent(	NULL,    // default security attribute
							NULL,    // manual-reset event
							NULL,    // initial state = signaled
							NULL);   // unnamed event object

	lo_w.hEvent			= hEvent_w;
	lo_r.hEvent			= hEvent_r;

	// Register event with driver
	if(DeviceIoControl(hDEV,NMWCD_REGISTER_AT_RESPONSE_EVENT,&hEvent_w,4,&dummy,4,&nBytes,NULL) == 0)
	{
		//#if _DEBUG
		//OutputDebugString( "Register event1 err\n" );
		//#endif
		CloseHandle(hDEV);
		return 2;
	}

	// Register event with driver
	if(DeviceIoControl(hDEV,NMWCD_REGISTER_DRIVER_READY_EVENT,&hEvent_r,4,&dummy,4,&nBytes,NULL) == 0)
	{
		//#if _DEBUG
		//OutputDebugString( "Register event2 err\n" );
		//#endif

		CloseHandle(hDEV);
		return 3;
	}
	*hPipe = hDEV;

	return 0;
}

// Close FBUS connection
void BB5UsbCloseConnection(HANDLE hPipe)
{
	if(hPipe    != INVALID_HANDLE_VALUE) CloseHandle(hPipe);
	if(hEvent_w != INVALID_HANDLE_VALUE) CloseHandle(hEvent_w);
	if(hEvent_r != INVALID_HANDLE_VALUE) CloseHandle(hEvent_r);
}

// Open dead USB connection
short BB5UsbOpenDeadConnection(HANDLE *hPipe)
{
	ULONG	i;
	HANDLE	hDEV = INVALID_HANDLE_VALUE;
	ULONG	TimeOutVar = 200;
	UCHAR	retBuf[768];
	ULONG	nBytes=0;

	//OutputDebugString( "Open connection...\n" );

	// loop till device is plugged
	for(i = 0; i <= TimeOutVar;i++)
	{
    	if(abort_req)
			return 100;

	   // exit if timeout expire
	   if(i == TimeOutVar)
	   {
			//OutputDebugString( "Timeout reached! Exit.\n" );
			return 1;
	   }

	   // open device for test
	   hDEV = open_dev(3,NULL);

	   // Exit on valid Handle
	   if(hDEV != INVALID_HANDLE_VALUE)
			break;

	   //OutputDebugString( "Waiting...\n" );

	   Sleep(1000);
	}

	//OutputDebugString( "Connection to rom established.\n" );

	hEvent1_dead = CreateEvent(
		NULL,			// default security attributes
		FALSE,			// manual-reset event
		FALSE,			// initial state is signaled
		NULL			// object name
		);

	hEvent2_dead = CreateEvent(
		NULL,			// default security attributes
		FALSE,			// manual-reset event
		FALSE,			// initial state is signaled
		NULL			// object name
		);

	// Register event with driver
	if(DeviceIoControl(hDEV,NMWCD_REGISTER_AT_RESPONSE_EVENT,&hEvent1_dead,4,retBuf,512,&nBytes,NULL) == 0)
	{
		//OutputDebugString( "err 2\n" );
		return 2;
	}

	// Register event with driver
	if(DeviceIoControl(hDEV,NMWCD_REGISTER_DRIVER_READY_EVENT,&hEvent2_dead,4,retBuf,512,&nBytes,NULL) == 0)
	{
		//OutputDebugString( "err 3\n" );
		return 3;
	}
	*hPipe = hDEV;

	return 0;
}

// Close dead USB connection
void BB5UsbCloseDeadConnection(HANDLE hPipe)
{
	if(hPipe        != INVALID_HANDLE_VALUE) CloseHandle(hPipe);
	if(hEvent1_dead != INVALID_HANDLE_VALUE) CloseHandle(hEvent1_dead);
	if(hEvent2_dead != INVALID_HANDLE_VALUE) CloseHandle(hEvent2_dead);
}

// Control transfer write - maybe need overlapped operation ?
short UsbLow_SendControlTransfer(HANDLE hPipe,UCHAR *out_d,ULONG out_s)
{
	ULONG	nBytes = 0;

	// Send data - non overlapped version
	if(DeviceIoControl(hPipe,NMWCD_EP0_PASSTHROUGH,out_d,out_s,NULL,0,&nBytes,NULL) == 0)
		return 1;

	return 0;
}

// Control transfer read  - maybe need overlapped operation ?
short UsbLow_ReadControlTransfer(HANDLE hPipe, UCHAR *out_d,ULONG out_s,UCHAR *pinBuf, UINT buffSize,ULONG *nBytesReturned)
{
	// Read - non overlapped version
	if(DeviceIoControl(hPipe,NMWCD_EP0_PASSTHROUGH,out_d,out_s,pinBuf,buffSize,nBytesReturned,NULL) == 0)
		return 1;

   return 0;
}

// Send command using control pipe - overlap version
short UsbLow_SendCommandViaControlPipe(HANDLE hPipe,UCHAR VendorCmd,UCHAR *out_d,ULONG out_s,bool encrypt)
{
	ULONG		nBytes = 0;
	UCHAR		*temp;
	OVERLAPPED	lo_w;
	ULONG		err;

	if(hPipe == INVALID_HANDLE_VALUE)
	{
		#ifdef DEBUG_BUILD
		//OutputDebugString( "UsbLow_SendCommandViaControlPipe->bad handle");
		#endif
		return 1;
	}

	temp = (UCHAR *)malloc(8 + out_s);
	memset(temp,        0,    8);
	memcpy(temp + 8,out_d,out_s);

	// Encrypt payload
	if(encrypt)
	{
		//DebugPrintHexArray(temp + 8,out_s,"out data plain:");
		//RC4_Crypt(temp + 8,out_s);
		//DebugPrintHexArray(temp + 8,out_s,"out data encr:");
	}

	*(temp + 0) = 0x40;
	*(temp + 1) = VendorCmd;

	*(temp + 6) = out_s & 0xFF;
	*(temp + 7) = out_s >> 8;

	//DebugPrintHexArray(temp,(out_s + 8),"cmd:");

    memset(&lo_w, 0, sizeof(lo_w));
	lo_w.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// Send data
	DeviceIoControl(hPipe,NMWCD_EP0_PASSTHROUGH,temp,(out_s + 8),NULL,0,&nBytes,&lo_w);

	err = GetLastError();
	if (err != ERROR_IO_PENDING)
	{
		if(err != ERROR_SUCCESS)
		{
			CloseHandle(lo_w.hEvent);

			#ifdef DEBUG_BUILD
			//OutputDebugString( "UsbLow_SendCommandViaControlPipe->not pending error");
			#endif

			free(temp);
			return 2;
        }
	}

	WaitForSingleObject(lo_w.hEvent, 5000);
	if(!GetOverlappedResult(hPipe, &lo_w, &nBytes, FALSE))
	{
		#ifdef DEBUG_BUILD
		//OutputDebugString( "UsbLow_SendCommandViaControlPipe->overlapped err");
        #endif

		CloseHandle(lo_w.hEvent);
		free(temp);
		return 3;
	}
	CloseHandle(lo_w.hEvent);

	free(temp);
	return 0;
}

// Read responce on command - under test, with overlap, if problem - uncomment old impl up
//
short UsbLow_GetDataViaControlPipe(HANDLE hPipe, UCHAR VendorCmd,UCHAR *pinBuf, UINT buffSize,ULONG expected,ULONG *nBytesReturned,bool encrypt)
{
	OVERLAPPED	lo_w;
	UCHAR 		vendor_read[8];
	ULONG		err;

	if(hPipe == INVALID_HANDLE_VALUE)
	{
		#ifdef DEBUG_BUILD
		//OutputDebugString( "UsbLow_GetDataViaControlPipe->handle invalid");
		#endif
		return 1;
	}

	memset(vendor_read,0,8);

	vendor_read[0] = 0xC0;
	vendor_read[1] = VendorCmd;

	vendor_read[6] = expected & 0xFF;
	vendor_read[7] = expected >> 8;

	//DebugPrintHexArray(vendor_read,8,"cmd:");

	memset(&lo_w, 0, sizeof(lo_w));
	lo_w.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	DeviceIoControl(hPipe,NMWCD_EP0_PASSTHROUGH,vendor_read,8,pinBuf,buffSize,nBytesReturned,&lo_w);

	err = GetLastError();
	if (err != ERROR_IO_PENDING)
	{
		if(err != ERROR_SUCCESS)
		{
			CloseHandle(lo_w.hEvent);

			#ifdef DEBUG_BUILD
			//DebugPrintInt( "UsbLow_GetDataViaControlPipe->not pending error",err);
            #endif
			return 2;
        }
	}

	while (1)
	{
		if(abort_req)
        {
			CancelIo(hPipe);
			CloseHandle(lo_w.hEvent);

			#ifdef DEBUG_BUILD
			//OutputDebugString( "UsbLow_GetDataViaControlPipe->User abort!");
			#endif
			return 100;
		}

		if(WaitForSingleObject(lo_w.hEvent,500) != 0)
			continue;	// wait more

		if (GetOverlappedResult(hPipe, &lo_w, nBytesReturned, FALSE))
			break;

		err = GetLastError();
		if(err != ERROR_IO_INCOMPLETE)
		{
			CloseHandle(lo_w.hEvent);

			#ifdef DEBUG_BUILD
			//DebugPrintInt("UsbLow_GetDataViaControlPipe->err diff than incomplete:",err);
			#endif
			return 3;
		}
	}
	CloseHandle(lo_w.hEvent);

	// Encrypt payload
	if(encrypt)
	{
		//DebugPrintHexArray(pinBuf,*nBytesReturned,"enc resp:");
		//RC4_Crypt(pinBuf,*nBytesReturned);
		//DebugPrintHexArray(pinBuf,*nBytesReturned,"dec resp:");
	}

	return 0;
}

// Read responce on command - test, overlapped
short UsbLow_GetDataViaControlPipeA(HANDLE hPipe, UCHAR VendorCmd,UCHAR *pinBuf, UINT buffSize,ULONG expected,ULONG *nBytesReturned,bool encrypt)
{
	UCHAR 		vendor_read[8];
	OVERLAPPED	lo_w;
	int			last;

	memset(vendor_read,0,8);

	vendor_read[0] = 0xC0;
	vendor_read[1] = VendorCmd;

	vendor_read[6] = expected & 0xFF;
	vendor_read[7] = expected >> 8;

	//DebugPrintHexArray(vendor_read,8,"cmd:");

	memset(&lo_w, 0, sizeof(lo_w));
	lo_w.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// Read - non overlapped version
	DeviceIoControl(hPipe,NMWCD_EP0_PASSTHROUGH,vendor_read,8,pinBuf,buffSize,nBytesReturned,&lo_w);

	last = GetLastError();
	if (last != ERROR_IO_PENDING)
	{
		if(last == ERROR_SUCCESS)
		{
			*nBytesReturned = buffSize;
			CloseHandle(lo_w.hEvent);
			return 0;
		}

		//OutputDebugString( "UsbLow_GetDataViaControlPipe->err 2");
		//DebugPrintInt("",last);
		CloseHandle(lo_w.hEvent);
		return 2;
	}

	WaitForSingleObject(lo_w.hEvent, 10000); // orig timeout 1000, use proper loop here ???
	if(!GetOverlappedResult(hPipe, &lo_w, nBytesReturned, FALSE))
	{
		//OutputDebugString( "UsbLow_GetDataViaControlPipe->err 3");
		CloseHandle(lo_w.hEvent);
		return 3;
	}
	CloseHandle(lo_w.hEvent);

	// All bytes here ?
	if(*nBytesReturned != buffSize)
		return 4;

	// Encrypt payload
	if(encrypt)
	{
		//DebugPrintHexArray(pinBuf,*nBytesReturned,"enc resp:");
		//RC4_Crypt(pinBuf,*nBytesReturned);
		//DebugPrintHexArray(pinBuf,*nBytesReturned,"dec resp:");
	}

	return 0;
}

bool BulkWriteToDevice(HANDLE hWrite, UCHAR *poutBuf, UINT buffSize,ULONG timeout)
{
	ULONG	nBytesWrite;

	if(poutBuf == NULL)
		return false;

	if(hWrite == INVALID_HANDLE_VALUE)
		return false;

	if(WriteFile(hWrite,poutBuf,buffSize,&nBytesWrite,&lo_w))
	{
		if(nBytesWrite == buffSize)
			return true;
	}

	if (GetLastError() != ERROR_IO_PENDING)
	{
		//OutputDebugString( "BulkWriteToDevice->err write1!");
		return false;
	}

	while (1)
	{
		// Timeout value:
		// orig 1000, for Infineon 5000, seems this function sucks donkey balls :(
		//
		if(WaitForSingleObject(hEvent_w,timeout) != 0)
		{
			CancelIo(hWrite);
			//OutputDebugString( "BulkWriteToDevice->err write2!");
			return false;
		}

		if (GetOverlappedResult(hWrite, &lo_w, &nBytesWrite, FALSE)) break;
		if (GetLastError() != ERROR_IO_INCOMPLETE)
		{
			//OutputDebugString( "BulkWriteToDevice->err write3!");
			return false;
		}
	}

	if(nBytesWrite != buffSize)
	{
		//OutputDebugString( "BulkWriteToDevice->err write4!");
		return false;
	}

	return true;
}

bool BulkReadFromDevice(HANDLE hWrite, UCHAR *pinBuf, UINT buffSize,ULONG *nBytesReturned)
{
	if(pinBuf == NULL)
		return false;
		
	if(hWrite == INVALID_HANDLE_VALUE) 
		return false;
						    
	if(ReadFile(hWrite,pinBuf,buffSize,nBytesReturned,&lo_r))
	{
		if(*nBytesReturned == buffSize)
			return true;
	}       

	if (GetLastError() != ERROR_IO_PENDING) 
	{
		//OutputDebugString( "BulkReadFromDevice->err read1!");
		return false;
	}

	while(1)
	{
		if(WaitForSingleObject(hEvent_r,5000) != 0)
		{
			CancelIo(hWrite);
			//OutputDebugString( "BulkReadFromDevice->err read2!");
			return false;
		}

		if (GetOverlappedResult(hWrite,&lo_r, nBytesReturned, FALSE)) break;
		if (GetLastError() != ERROR_IO_INCOMPLETE)
		{
			//OutputDebugString( "BulkReadFromDevice->err read3!");
			return false;
		}
	}

	return true;
}


