//$$---- Thread HDR ----
//---------------------------------------------------------------------------

#ifndef WorkerH
#define WorkerH
//---------------------------------------------------------------------------
#include <Classes.hpp>

#include "Unit1.h"
//---------------------------------------------------------------------------
class Worker : public TThread
{
private:
protected:
	void __fastcall Execute();
public:
	__fastcall Worker(bool CreateSuspended);

	void __fastcall CheckDevice(TForm1 *W);
	void __fastcall UpdateDSPFirmware(TForm1 *W);
	void __fastcall UpdateUIFirmware(TForm1 *W);
};

class Mutex
{
private:
    CRITICAL_SECTION lock;
public:
    Mutex(void)       {
		InitializeCriticalSection(&lock);
	}

	void Lock(void)   {
		 EnterCriticalSection(&lock);
	}

	void Unlock(void) {
		 LeaveCriticalSection(&lock);
	}

	~Mutex(void)       {
		DeleteCriticalSection(&lock);
	}
};

class SafeQueueHandler : private Mutex
{
private:

	UCHAR	ucRequest;

public:

	void Init(void)
	{
		Lock();
		ucRequest = 0;
		Unlock();
	}

	UCHAR Get(void)
	{
		UCHAR val;

		Lock();
		val = ucRequest;
		Unlock();

		return val;
	}

	void Set(UCHAR value)
	{
		Lock();
		ucRequest = value;
		Unlock();
	}
};

void  SetRequest(UCHAR req);
UCHAR GetStatus(void);
//---------------------------------------------------------------------------
#endif
