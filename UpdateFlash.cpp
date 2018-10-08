//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

//#include "usb_low.h"
#include "fbus_low.h"
#include "misc.h"

#include "UpdateFlash.h"
//---------------------------------------------------------------------------

#pragma package(smart_init)


// Write flash block routine
short FbusSdio_WriteBlock(HANDLE hPipe,ULONG ulSectNumber,UCHAR *data)
{
	UCHAR cmd[8];
	UCHAR temp[512];
	ULONG i;

	if(data == NULL)
		return 1;

	memcpy(temp,data,512);

	// Send sector via bulk pipe
	if(FbusLow_WriteBulkPipe(hPipe,0x01,temp,512,false) != 0)
	{
		return 2;
	}

	memset(cmd,0,8);

	i = ulSectNumber;
	//DebugPrintInt("sect:",i);

	// Send block number
	cmd[0] = i >> 24;
	cmd[1] = i >> 16;
	cmd[2] = i >>  8;
	cmd[3] = i >>  0;

	// Send command
	if(FbusLow_SendCommandViaControlPipe(hPipe,0x52,cmd,8,false,0) != 0)
	{
		return 3;
	}

	return 0;
}

// Write flash block routine
short FbusSdio_WriteUIBlock(HANDLE hPipe,ULONG ulSectNumber,UCHAR *data)
{
	UCHAR cmd[8];
	UCHAR temp[512];
	ULONG i;

	if(data == NULL)
		return 1;

	memcpy(temp,data,512);

	// Send sector via bulk pipe
	if(FbusLow_WriteBulkPipe(hPipe,0x01,temp,512,false) != 0)
	{
		return 2;
	}

	memset(cmd,0,8);

	i = ulSectNumber;
	//DebugPrintInt("sect:",i);

	// Send block number
	cmd[0] = i >> 24;
	cmd[1] = i >> 16;
	cmd[2] = i >>  8;
	cmd[3] = i >>  0;

	// Send command
	if(FbusLow_SendCommandViaControlPipe(hPipe,0x53,cmd,8,false,0) != 0)
	{
		return 3;
	}

	return 0;
}
