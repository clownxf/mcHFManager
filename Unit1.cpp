//$$---- Form CPP ----
//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include <dir.h>

#include "Unit1.h"
#include "Worker.h"
#include "Ini.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm1 *Form1;

AnsiString 	sAppPath;
PVOID  		xWnd = NULL;
Worker		*aTh = NULL;

extern bool	loc_abort_req;

//---------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormActivate(TObject *Sender)
{
	char 					buffer[260];
	CIni 					m_ini;
	AnsiString 				s;

	Worker	*aThread = new Worker(true);

	xWnd = (PVOID)this;

	this->Caption = "mcHF Manager V 0.3";

	// overwritten by the open dialog !
	getcwd(buffer, MAXPATH);
	sAppPath = buffer;
	sAppPath += "\\";

	// Set ini file
	s = sAppPath + "mcHFManager.ini";
	m_ini.SetPathName(s.c_str());

	// Load position
	this->Left    = m_ini.GetInt("StartUp", "Left",100);
	this->Top     = m_ini.GetInt("StartUp", "Top",100);

	// Start thread
	aThread->Priority = tpNormal;
	aThread->Resume();
	aTh = aThread;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormDestroy(TObject *Sender)
{
	CIni 		m_ini;
	AnsiString 	s;

	// Killing thread
	SetRequest(1);   							// Set suspend request
	while(GetStatus());     					// Wait till msg is received
	TerminateThread((HANDLE)aTh->Handle,false);	// Hardcore kill

	// Set ini file
	s = sAppPath + "mcHFManager.ini";
	m_ini.SetPathName(s.c_str());

	// Save form possition
	m_ini.WriteInt("StartUp", "Left", this->Left);
	m_ini.WriteInt("StartUp", "Top", this->Top);
}
//---------------------------------------------------------------------------
// Check device
void __fastcall TForm1::BitBtn1Click(TObject *Sender)
{
	// Thread busy ?
	if(GetStatus())
		return;

	// Send request
	SetRequest(2);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::BitBtn2Click(TObject *Sender)
{
	Application->Terminate();	
}
//---------------------------------------------------------------------------
// Update DSP Firmware
void __fastcall TForm1::BitBtn3Click(TObject *Sender)
{
	// Thread busy ?
	if(GetStatus())
		return;

	// Send request
	SetRequest(3);
}
//---------------------------------------------------------------------------
// Select file
void __fastcall TForm1::BitBtn4Click(TObject *Sender)
{
	Memo1->Text = Memo1->Text + "---------------------------------------------------\r\n";
	Memo1->Text = Memo1->Text + "Open file...";
	Edit1->Text = "";

	// Init open dialog
	OpenDialog1->Filter = "BIN Files (*.bin)|*.BIN";
	OpenDialog1->InitialDir = sAppPath;

	if(!OpenDialog1->Execute())
	{
		Memo1->Text = Memo1->Text + "Cancelled.\r\n";
		return;
	}

	Edit1->Text = OpenDialog1->FileName;
	Memo1->Text = Memo1->Text + "Done.\r\n";
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Memo1Change(TObject *Sender)
{
	Memo1->Perform(WM_VSCROLL,SB_BOTTOM,SB_ENDSCROLL);
	Sleep(100);
}
//---------------------------------------------------------------------------
// Update UI Firmware
void __fastcall TForm1::BitBtn5Click(TObject *Sender)
{
	// Thread busy ?
	if(GetStatus())
		return;

	// Send request
	SetRequest(4);
}
//---------------------------------------------------------------------------
// Abort operation
void __fastcall TForm1::BitBtn6Click(TObject *Sender)
{
	loc_abort_req = true;
}
//---------------------------------------------------------------------------

