//$$---- Thread CPP ----
//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include <stdio.h>
#include <stdlib.h>

#include "Worker.h"
#pragma package(smart_init)

#include "UpdateFlash.h"
#include "fbus_low.h"
#include "misc.h"

extern  PVOID  			xWnd;
extern  AnsiString 		sAppPath;

bool					abort_req = false;
bool					loc_abort_req = false;

SafeQueueHandler 		aSQH;

//---------------------------------------------------------------------------

__fastcall Worker::Worker(bool CreateSuspended)
	: TThread(CreateSuspended)
{
}
//---------------------------------------------------------------------------
void __fastcall Worker::Execute()
{
	TForm1 *W;
	UCHAR	menu;

	try
	{
		aSQH.Init();

		// Get active window ptr
		W =(TForm1 *)xWnd;
		if (!W)
			this->Terminate();

		// Working thread body, do not check Terminated flag
		while(1)
		{
			menu = aSQH.Get();

			// Check request
			switch(menu)
			{
				case 1:
				{
					// Ask that is ready to suspend
					aSQH.Set(0);

					this->Suspend();

					// Never here
					break;
				}

				// Check device
				case 2:
				{
					W->BitBtn1->Enabled = false;

					// Process
					this->CheckDevice(W);

					W->BitBtn1->Enabled = true;

					// Clear flags
					aSQH.Set(0);

					break;
				}

				// Reflash DSP firmware
				case 3:
				{
					W->BitBtn3->Enabled = false;

					// Process
					this->UpdateDSPFirmware(W);

					W->BitBtn3->Enabled = true;

					// Clear flags
					aSQH.Set(0);

					break;
				}

				// Reflash UI firmware
				case 4:
				{
					W->BitBtn5->Enabled = false;

					// Process
					this->UpdateUIFirmware(W);

					W->BitBtn5->Enabled = true;

					// Clear flags
					aSQH.Set(0);

					break;
				}

				default:
					break;
			}

			Sleep(300);
		}
	}

	catch(...)
	{
		OutputDebugString("Worker->Exception");
	}
}
//---------------------------------------------------------------------------
void SetRequest(UCHAR req)
{
	 aSQH.Set(req);
}

//---------------------------------------------------------------------------

UCHAR GetStatus(void)
{
	 return aSQH.Get();
}
//---------------------------------------------------------------------------
// Check device
void __fastcall Worker::CheckDevice(TForm1 *W)
{
	HANDLE	hMtiPipe = INVALID_HANDLE_VALUE;
	UCHAR	buff[200];
	char	temp[100],*sn,*desc;

	loc_abort_req = false;

	W->Memo1->Text = W->Memo1->Text + "---------------------------------------------------\r\n";
	W->Memo1->Text = W->Memo1->Text + "Looking for mcHF bootloader...";

	// Open interface
	if(FbusLow_open_scni_device(&hMtiPipe) != 0)
	{
		W->Memo1->Text = W->Memo1->Text + "Not found!\r\n";
		return;
	}
	W->Memo1->Text = W->Memo1->Text + "Done.\r\n";
	W->Memo1->Text = W->Memo1->Text + "Reading details...";

	// Get device details command
	if(FbusLow_SendCommandViaControlPipe(hMtiPipe,0x10,NULL,0,false,0) != 0)
	{
		W->Memo1->Text = W->Memo1->Text + "Error1!\r\n";
		FbusLow_close_scni_device(hMtiPipe);
		return;
	}

	// Read data
	if(FbusLow_ReadDataViaControlPipe(hMtiPipe,0x90,buff,128,false) != 0)
	{
		W->Memo1->Text = W->Memo1->Text + "Error2!\r\n";
		FbusLow_close_scni_device(hMtiPipe);
		return;
	}
	//DebugPrintHexArray(buff,128,"device data:");
	W->Memo1->Text = W->Memo1->Text + "Done.\r\n";

	// Load string descriptor
	desc = ((char *)buff + 64);
	//OutputDebugString(desc);

	W->Memo1->Text = W->Memo1->Text + "---------------------\r\n";
	W->Memo1->Text = W->Memo1->Text + "DSP Details:\r\n";

	// Is correct device ?
	if(strstr(desc,"mcHF bootloader, M0NKA 2014") == 0)
	{
		W->Memo1->Text = W->Memo1->Text + "Incorrect device, exit!\r\n";
		FbusLow_close_scni_device(hMtiPipe);
		return;
	}

	// Build type
	if(buff[20] == 1)
		buff[20] = 'd';
	else
		buff[20] = 'r';

	// Convert build
	sprintf(temp,"%d.%d.%d.%d %c",buff[16],buff[17],buff[18],buff[19],buff[20]);

	// Convert SN
	//sn = ReturnHexArray(buff + 22,10);
	sn = (char *)(buff + 25);

	// Print details
	W->Memo1->Text = W->Memo1->Text + "Dev ID:\t" + (AnsiString)desc + "\r\n";
	W->Memo1->Text = W->Memo1->Text + "Build:\t" + (AnsiString)(temp) + "\r\n";
	W->Memo1->Text = W->Memo1->Text + "Dev SN:\t" + (AnsiString)(sn) + "\r\n";

	// Show HW version
	sprintf(temp,"%d.%d",buff[14],buff[15]);
	W->Memo1->Text = W->Memo1->Text + "HW Ver:\t" + (AnsiString)(temp) + "\r\n";

	// UI CPU (rev 0.8)
	if((buff[33] == 0x73) && (buff[34] == 0xF2))
	{
		W->Memo1->Text = W->Memo1->Text + "---------------------\r\n";
		W->Memo1->Text = W->Memo1->Text + "UI CPU Details:\r\n";

		// Convert build
		sprintf(temp,"%d.%d.%d.%d",buff[35],buff[36],buff[37],buff[38]);
		W->Memo1->Text = W->Memo1->Text + "Build:\t" + (AnsiString)(temp) + "\r\n";

		desc = ((char *)buff + 39);
		W->Memo1->Text = W->Memo1->Text + "Dev ID:\t" + (AnsiString)desc + "\r\n";
	}

	//free(sn);

	// Close
	FbusLow_close_scni_device(hMtiPipe);
}
//---------------------------------------------------------------------------
// Reflash DSP
void __fastcall Worker::UpdateDSPFirmware(TForm1 *W)
{
	HANDLE	hDeadPipe = INVALID_HANDLE_VALUE;
	UCHAR   temp[1000];
	UCHAR	in[512];
	USHORT 	a,i;
	ULONG	s,m;
	UCHAR	*pFile;

	AnsiString asPath;

	loc_abort_req = false;

	W->Memo1->Text = W->Memo1->Text + "---------------------------------------------------\r\n";
	W->Memo1->Text = W->Memo1->Text + "DSP Firmware update:\r\n";
	W->Memo1->Text = W->Memo1->Text + "Check file selection...";

	// Check name
	asPath = W->Edit1->Text;
	if(asPath == "")
	{
		W->Memo1->Text = W->Memo1->Text + "Nothing selected!\r\n";
		return;
	}

	// Open
	if(!AnyFileOpen(asPath.c_str(),&s,&m))
	{
		W->Memo1->Text = W->Memo1->Text + "Error Open!\r\n";
		return;
	}

	pFile = (UCHAR *)s;

	W->Memo1->Text = W->Memo1->Text + "Done.\r\n";
	W->Memo1->Text = W->Memo1->Text + "Trying to establish connection...";

	// Open interface
	if(FbusLow_OpenSdioConnection(&hDeadPipe) != 0)
	{
		W->Memo1->Text = W->Memo1->Text + "Error!\r\n";
		FbusLow_CloseSdioConnection(hDeadPipe);
		free(pFile);
		return;
	}
	W->Memo1->Text = W->Memo1->Text + "Done.\r\n";
	W->Memo1->Text = W->Memo1->Text + "Send erase command...";

	// Erase sectors
	if(FbusLow_SendCommandViaControlPipe(hDeadPipe,0x4D,NULL,0,false,a) != 0)
	{
		W->Memo1->Text = W->Memo1->Text + "Error!\r\n";
		FbusLow_CloseSdioConnection(hDeadPipe);
		free(pFile);
		return;
	}
	W->Memo1->Text = W->Memo1->Text + "Done.\r\n";
	W->Memo1->Text = W->Memo1->Text + "Waiting erase completion...";

	W->pg1->Max 		= 15;
	W->pg1->Min	 		= 0;
	W->pg1->Step 		= 1;
	W->pg1->Position 	= 0;

	// Wait complete status
	for(i = 0; i < 15; i++)
	{
    	if(loc_abort_req)
			break;

		W->pg1->Position = i;
		Sleep(1000);

		// Read responce
		if(FbusLow_ReadDataViaControlPipe(hDeadPipe,0xDD,in,32,false) != 0)
		{
			W->Memo1->Text = W->Memo1->Text + ".";
		}
		else
		{
			//DebugPrintHexArray(in,32,"");
			break;
		}
	}

	if(loc_abort_req)
	{
		W->Memo1->Text = W->Memo1->Text + "User abort.\r\n";
		FbusLow_CloseSdioConnection(hDeadPipe);
		free(pFile);
		return;
	}

	// Check erase result
	if(in[0] == 0)
		W->Memo1->Text = W->Memo1->Text + "Done.\r\n";
	else
	{
		W->Memo1->Text = W->Memo1->Text + "Error!\r\n";
		FbusLow_CloseSdioConnection(hDeadPipe);
		free(pFile);
		return;
	}

	W->Memo1->Text = W->Memo1->Text + "Write firmware...";

	W->pg1->Max 		= m/512;
	W->pg1->Min	 		= 0;
	W->pg1->Step 		= 1;
	W->pg1->Position 	= 0;

	for(i = 0; i < m/512; i++)
	{
		if(loc_abort_req)
			break;

        W->pg1->Position = i;

		memcpy(temp,pFile + i*512,512);
		//DebugPrintHexArray(temp,8,"");

		if(FbusSdio_WriteBlock(hDeadPipe,(0x08010000 + i*512),temp) != 0)
		{
			W->Memo1->Text = W->Memo1->Text + "Error!\r\n";
			FbusLow_CloseSdioConnection(hDeadPipe);
			free(pFile);
			return;
		}
	}

	if(loc_abort_req)
	{
		W->Memo1->Text = W->Memo1->Text + "User abort.\r\n";
		FbusLow_CloseSdioConnection(hDeadPipe);
		free(pFile);
		return;
	}

	W->Memo1->Text = W->Memo1->Text + "Done.\r\n";
	free(pFile);

	W->Memo1->Text = W->Memo1->Text + "Restart to normal mode...";

	// Close
	if(FbusLow_CloseSdioConnection(hDeadPipe) != 0)
	{
		W->Memo1->Text = W->Memo1->Text + "Error!\r\n";
		return;
	}

	W->Memo1->Text = W->Memo1->Text + "Done.\r\n";
}
//---------------------------------------------------------------------------
// Reflash UI firmware
void __fastcall Worker::UpdateUIFirmware(TForm1 *W)
{
	HANDLE	hDeadPipe = INVALID_HANDLE_VALUE;
	UCHAR   temp[1000];
	UCHAR	in[512];
	USHORT 	a,i;
	ULONG	s,m;
	UCHAR	*pFile;

	AnsiString asPath;

	loc_abort_req = false;

	W->Memo1->Text = W->Memo1->Text + "---------------------------------------------------\r\n";
	W->Memo1->Text = W->Memo1->Text + "UI Firmware update:\r\n";
	W->Memo1->Text = W->Memo1->Text + "Check file selection...";

	// Check name
	asPath = W->Edit1->Text;
	if(asPath == "")
	{
		W->Memo1->Text = W->Memo1->Text + "Nothing selected!\r\n";
		return;
	}

	// Open
	if(!AnyFileOpen(asPath.c_str(),&s,&m))
	{
		W->Memo1->Text = W->Memo1->Text + "Error Open!\r\n";
		return;
	}

	pFile = (UCHAR *)s;

	W->Memo1->Text = W->Memo1->Text + "Done.\r\n";
	W->Memo1->Text = W->Memo1->Text + "Trying to establish connection...";

	// Open interface
	if(FbusLow_OpenSdioConnection(&hDeadPipe) != 0)
	{
		W->Memo1->Text = W->Memo1->Text + "Error!\r\n";
		FbusLow_CloseSdioConnection(hDeadPipe);
		free(pFile);
		return;
	}
	W->Memo1->Text = W->Memo1->Text + "Done.\r\n";
	W->Memo1->Text = W->Memo1->Text + "Send erase command...";

	// Erase sectors
	if(FbusLow_SendCommandViaControlPipe(hDeadPipe,0x7E,NULL,0,false,a) != 0)
	{
		W->Memo1->Text = W->Memo1->Text + "Error!\r\n";
		FbusLow_CloseSdioConnection(hDeadPipe);
		free(pFile);
		return;
	}
	W->Memo1->Text = W->Memo1->Text + "Done.\r\n";
	W->Memo1->Text = W->Memo1->Text + "Waiting erase completion...";

	W->pg1->Max 		= 25;
	W->pg1->Min	 		= 0;
	W->pg1->Step 		= 1;
	W->pg1->Position 	= 0;

	// Wait complete status
	for(i = 0; i < 25; i++)
	{
		if(loc_abort_req)
			break;

		//W->Memo1->Text = W->Memo1->Text + ".";
		W->pg1->Position = i;
		Sleep(1000);

		// Read responce
		if(FbusLow_ReadDataViaControlPipe(hDeadPipe,0xEE,in,32,false) != 0)
		{
			//
		}
		else
		{
			//DebugPrintHexArray(in,32,"");

			if(in[0] == 0x22)
				break;
		}
	}

	if(loc_abort_req)
	{
		W->Memo1->Text = W->Memo1->Text + "User abort.\r\n";
		FbusLow_CloseSdioConnection(hDeadPipe);
		free(pFile);
		return;
	}

	// Check erase result
	if(in[0] == 0x22)
		W->Memo1->Text = W->Memo1->Text + "Done.\r\n";
	else
	{
		W->Memo1->Text = W->Memo1->Text + "Error!\r\n";
		FbusLow_CloseSdioConnection(hDeadPipe);
		free(pFile);
		return;
	}

	W->pg1->Max 		= m/512;
	W->pg1->Min	 		= 0;
	W->pg1->Step 		= 1;
	W->pg1->Position 	= 0;

	W->Memo1->Text = W->Memo1->Text + "Write firmware...";

	for(i = 0; i < m/512; i++)
	{
		if(loc_abort_req)
			break;

		memcpy(temp,pFile + i*512,512);
		//DebugPrintHexArray(temp,8,"");

		if(FbusSdio_WriteUIBlock(hDeadPipe,(0x08020000 + i*512),temp) != 0)
		{
			W->Memo1->Text = W->Memo1->Text + "Error!\r\n";
			FbusLow_CloseSdioConnection(hDeadPipe);
			free(pFile);
			return;
		}

		W->pg1->Position = i;
	}

	if(loc_abort_req)
	{
		W->Memo1->Text = W->Memo1->Text + "User abort.\r\n";
		FbusLow_CloseSdioConnection(hDeadPipe);
		free(pFile);
		return;
	}

	W->Memo1->Text = W->Memo1->Text + "Done.\r\n";
	free(pFile);

	W->Memo1->Text = W->Memo1->Text + "Restart to normal mode...";

	// Close
	if(FbusLow_CloseSdioConnection(hDeadPipe) != 0)
	{
		W->Memo1->Text = W->Memo1->Text + "Error!\r\n";
		return;
	}

	W->Memo1->Text = W->Memo1->Text + "Done.\r\n";
}
//---------------------------------------------------------------------------
