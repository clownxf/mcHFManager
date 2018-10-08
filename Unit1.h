//$$---- Form HDR ----
//---------------------------------------------------------------------------

#ifndef Unit1H
#define Unit1H
#include <Buttons.hpp>
#include <Classes.hpp>
#include <ComCtrls.hpp>
#include <Controls.hpp>
#include <Dialogs.hpp>
#include <StdCtrls.hpp>
//---------------------------------------------------------------------------
//#include <System.Classes.hpp>
//#include <Vcl.Controls.hpp>
//#include <Vcl.StdCtrls.hpp>
//#include <Vcl.Forms.hpp>
//#include <Vcl.Buttons.hpp>
//#include <Vcl.ComCtrls.hpp>
//#include <Vcl.Dialogs.hpp>

//---------------------------------------------------------------------------
class TForm1 : public TForm
{
__published:	// IDE-managed Components
	TPageControl *PageControl1;
	TTabSheet *TabSheet1;
	TBitBtn *BitBtn1;
	TMemo *Memo1;
	TBitBtn *BitBtn2;
	TBitBtn *BitBtn3;
	TBitBtn *BitBtn4;
	TEdit *Edit1;
	TOpenDialog *OpenDialog1;
	TBitBtn *BitBtn5;
	TBitBtn *BitBtn6;
	TProgressBar *pg1;
	void __fastcall FormActivate(TObject *Sender);
	void __fastcall FormDestroy(TObject *Sender);
	void __fastcall BitBtn1Click(TObject *Sender);
	void __fastcall BitBtn2Click(TObject *Sender);
	void __fastcall BitBtn3Click(TObject *Sender);
	void __fastcall BitBtn4Click(TObject *Sender);
	void __fastcall Memo1Change(TObject *Sender);
	void __fastcall BitBtn5Click(TObject *Sender);
	void __fastcall BitBtn6Click(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TForm1(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TForm1 *Form1;
//---------------------------------------------------------------------------
#endif
