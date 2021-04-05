#include <ntddk.h>
#include <wdf.h>

//win10x64 19041 windbg offsets
#define KTHREAD_APCSTATE_PROCESS_OFFSET 0x0B8
#define KTHREAD_WAITLISTENTRY 0x0D8
#define EPROCESS_ACTIVEPROCESSLINKS 0x448
#define PRCB_DISPATCHERREADYLISTHEAD 0x7C80
#define EPROCCESS_IMAGEFILE 0x5A8

DRIVER_INITIALIZE DriverEntry;
PLIST_ENTRY RemovedEntry;
PLIST_ENTRY RemovedEntryFromList;

//Alternate way of getting currentprcb
//FORCEINLINE PLIST_ENTRY GetDispatcherList(VOID)
//{
//	return (PLIST_ENTRY)(__readgsqword(FIELD_OFFSET(KPCR, CurrentPrcb)) + PRCB_DISPATCHERREADYLISTHEAD);
//}

typedef BYTE*(NTAPI* KeQueryPrcbAddressFn)(
	__in ULONG ProcNum
	);

static PLIST_ENTRY GetDispatcherListPerProc(int proc) 
{
	KeQueryPrcbAddressFn ntQueryPrcb = NULL;
	UNICODE_STRING ntQueryPrcbName;

	// Get KeQueryPrcbAddress function address and execute
	RtlInitUnicodeString(&ntQueryPrcbName, L"KeQueryPrcbAddress");
	ntQueryPrcb = (KeQueryPrcbAddressFn)MmGetSystemRoutineAddress(&ntQueryPrcbName);
	return (PLIST_ENTRY)(ntQueryPrcb(proc) + PRCB_DISPATCHERREADYLISTHEAD);
}

static BOOLEAN RemoveEntityFromList(PLIST_ENTRY ListHead)
{
	PLIST_ENTRY	NextListEntry, PrevListEntry, tmpPtr;
	PETHREAD	CurrThread;
	PEPROCESS CurrProc;

	try {

		PrevListEntry = ListHead;
		NextListEntry = ListHead->Flink;
		while ((NextListEntry != ListHead) && NextListEntry) {

			// get __ETHREAD struct pointer from list
			CurrThread = (PETHREAD)((BYTE*)NextListEntry - KTHREAD_WAITLISTENTRY);
			tmpPtr = NextListEntry->Flink;

			// get thread's __EPROCESS struct from list
			CurrProc = *(PEPROCESS*)((BYTE*)CurrThread + KTHREAD_APCSTATE_PROCESS_OFFSET);
			const char* ProcessName = (const char*)((BYTE*)CurrProc + EPROCCESS_IMAGEFILE);
			if (strncmp("ALEX.exe", ProcessName, 14) == 0)
			{
				RemovedEntry = NextListEntry;
				RemovedEntryFromList = ListHead;
				RemoveEntryList(NextListEntry);
				return TRUE;
			}
			NextListEntry = tmpPtr;
		}

	} except(EXCEPTION_EXECUTE_HANDLER) {
	}

	return FALSE;
}

VOID Unload()
{
	InsertTailList(RemovedEntryFromList, RemovedEntry);
    DbgPrint("Driver Unloaded\n");
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{

    UNREFERENCED_PARAMETER(RegistryPath);
    DriverObject->DriverUnload = (PDRIVER_UNLOAD)Unload;
	//PLIST_ENTRY DispatcherList = GetDispatcherList();
	PLIST_ENTRY DispatcherList = GetDispatcherListPerProc(0);
	BOOLEAN fileStatus = FALSE;
	while(!fileStatus) {
		for (int i = 0; i < 31; i++) {
			fileStatus = RemoveEntityFromList(&DispatcherList[i]);
			if (fileStatus)
				break;
		}
	}
    DbgPrint("Finished!\n");
    return STATUS_SUCCESS;
}