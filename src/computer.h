#ifndef __COMPUTER_H__
#define __COMPUTER_H__

#include "rt.h"
#include "common.h"
#include "pump_controller.h"
#include <list>

/**
 * To avoid linker tools error LNK2005/LNK1169 (i.e., symbol was defined more than once.),
 * the header file must not include the declarations of variables that are used by the
 * function definitions inside the implementation file.
 */
void setupComputer();
void printTxn(const CustomerRecord& record, int position, int txn_id);
void exitComputer();
void writeTxnToPipe(const unique_ptr<PumpController>& pump_ctrl);

UINT __stdcall readTank(void* args);
UINT __stdcall printTxnHistory(void* args);
UINT __stdcall runPump(void* args);

class TxnListPrinter
{
private:
	/**
	 * Using a smart pointer would not be better here.
	 * This class does not own the list it's printing. It's just given a address reference to it.
	 * Thus, it's not this class's responsibility to delete the list when it's done.
	 * This is the responsibility of whoever owns the list that is passed to the class's constructor.
	 * It's considered good practice to use raw pointers or references when you want to merely observe
	 * an object (i.e., you don't need to control its lifetime).
	 * The reference to the list tells users of the ListPrinter class that the class will use the list
	 * but won't manage its memory.
	 */
	list<CustomerRecord>* lst;
	
	unique_ptr<CMutex> mutex;

public:
	TxnListPrinter(list<CustomerRecord>& lst);

	void printNew();
};

#endif // __COMPUTER_H__
