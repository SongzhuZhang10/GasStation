#ifndef __COMPUTER_H__
#define __COMPUTER_H__

#include "rt.h"
#include "common.h"
#include "pump_data.h"
#include <list>

/**
 * To avoid linker tools error LNK2005/LNK1169 (i.e., symbol was defined more than once.),
 * the header file must not include the declarations of variables that are used by the
 * function definitions inside the implementation file.
 */
void setupComputer();
void recordTxn(const unique_ptr<PumpData>& pump_data_ptr);

void printTxn(const CustomerRecord& record, int position, int txn_id);

UINT __stdcall readPump(void* args);
UINT __stdcall readTank(void* args);
UINT __stdcall printTxnHistory(void* args);

class TxnListPrinter
{
private:
	list<CustomerRecord>* lst;
	size_t last_size = 0;
	unique_ptr<CMutex> mutex;

public:
	TxnListPrinter(list<CustomerRecord>& lst);

	void printNew();
};

#endif // __COMPUTER_H__
