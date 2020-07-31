#pragma once
#include <winternl.h>
#pragma comment(lib, "ntdll.lib")

#define GLOBAL_STACK_MEM_SIZE   0x400000
#define IOCTL_ADD_PROCESS       0x550000
#define IOCTL_REMOVE_PROCESS    0x550002
#define IOCTL_INIT              0x550004
#define MIN_HEIGHT              200              
#define MIN_WIDTH               475

// Globals
HANDLE  hGlobalDriver       = INVALID_HANDLE_VALUE;
HANDLE  hGlobalPipe         = INVALID_HANDLE_VALUE;
HANDLE  hGlobalListenThread = INVALID_HANDLE_VALUE;
PVOID   pGlobalStackMem     = NULL;
HWND    hGlobalHWND         = 0;
BOOL    bGlobalSizes        = FALSE;
LONG    dx1, dx2, dx3, dx4, dx5;

typedef BOOL(WINAPI* IsNTAdmin)(DWORD Reserved, LPVOID pReserved);
typedef NTSTATUS(NTAPI* NtLoadDriver)(PUNICODE_STRING DriverServiceName);

typedef struct _STACK_CHUNK
{
    ULONG64 Row;
    ULONG64 StackData[0x10];
} STACK_CHUNK, *PSTACK_CHUNK;

typedef struct _CUSTOM_HEADER
{
    ULONG64 ProcessId;
    ULONG64 StackData[0x10];
} CUSTOM_HEADER, *PCUSTOM_HEADER;

typedef CCHAR KPROCESSOR_MODE;
typedef UCHAR KIRQL;

typedef struct _KTRAP_FRAME {

    //
    // Home address for the parameter registers.
    //

    ULONG64 P1Home;
    ULONG64 P2Home;
    ULONG64 P3Home;
    ULONG64 P4Home;
    ULONG64 P5;

    //
    // Previous processor mode (system services only) and previous IRQL
    // (interrupts only).
    //

    KPROCESSOR_MODE PreviousMode;

    KIRQL PreviousIrql;

    //
    // Page fault load/store indicator.
    //

    UCHAR FaultIndicator;

    //
    // Exception active indicator.
    //
    //    0 - interrupt frame.
    //    1 - exception frame.
    //    2 - service frame.
    //

    UCHAR ExceptionActive;

    //
    // Floating point state.
    //

    ULONG MxCsr;

    //
    //  Volatile registers.
    //
    // N.B. These registers are only saved on exceptions and interrupts. They
    //      are not saved for system calls.
    //

    ULONG64 Rax;
    ULONG64 Rcx;
    ULONG64 Rdx;
    ULONG64 R8;
    ULONG64 R9;
    ULONG64 R10;
    ULONG64 R11;

    //
    // Gsbase is only used if the previous mode was kernel.
    //
    // GsSwap is only used if the previous mode was user.
    //

    union {
        ULONG64 GsBase;
        ULONG64 GsSwap;
    };

    //
    // Volatile floating registers.
    //
    // N.B. These registers are only saved on exceptions and interrupts. They
    //      are not saved for system calls.
    //

    M128A Xmm0;
    M128A Xmm1;
    M128A Xmm2;
    M128A Xmm3;
    M128A Xmm4;
    M128A Xmm5;

    //
    // First parameter, page fault address, context record address if user APC
    // bypass.
    //

    union {
        ULONG64 FaultAddress;
        ULONG64 ContextRecord;
    };

    //
    //  Debug registers.
    //

    ULONG64 Dr0;
    ULONG64 Dr1;
    ULONG64 Dr2;
    ULONG64 Dr3;
    ULONG64 Dr6;
    ULONG64 Dr7;

    //
    // Special debug registers.
    //

    struct {
        ULONG64 DebugControl;
        ULONG64 LastBranchToRip;
        ULONG64 LastBranchFromRip;
        ULONG64 LastExceptionToRip;
        ULONG64 LastExceptionFromRip;
    };

    //
    //  Segment registers
    //

    USHORT SegDs;
    USHORT SegEs;
    USHORT SegFs;
    USHORT SegGs;

    //
    // Previous trap frame address.
    //

    ULONG64 TrapFrame;

    //
    // Saved nonvolatile registers RBX, RDI and RSI. These registers are only
    // saved in system service trap frames.
    //

    ULONG64 Rbx;
    ULONG64 Rdi;
    ULONG64 Rsi;

    //
    // Saved nonvolatile register RBP. This register is used as a frame
    // pointer during trap processing and is saved in all trap frames.
    //

    ULONG64 Rbp;

    //
    // Information pushed by hardware.
    //
    // N.B. The error code is not always pushed by hardware. For those cases
    //      where it is not pushed by hardware a dummy error code is allocated
    //      on the stack.
    //

    union {
        ULONG64 ErrorCode;
        ULONG64 ExceptionFrame;
    };

    ULONG64 Rip;
    USHORT SegCs;
    UCHAR Fill0;
    UCHAR Logging;
    USHORT Fill1[2];
    ULONG EFlags;
    ULONG Fill2;
    ULONG64 Rsp;
    USHORT SegSs;
    USHORT Fill3;
    ULONG Fill4;
} KTRAP_FRAME, * PKTRAP_FRAME;

typedef struct _TOTAL_PACKET
{
    CUSTOM_HEADER CustomHeader;
    KTRAP_FRAME Frame;
} TOTAL_PACKET, * PTOTAL_PACKET;


/*
    Taken from https://github.com/hfiref0x/WinObjEx64 GUI's code
*/
INT CListView_InsertColumn(HWND ListViewHWND, INT ColumnIndex, INT SubItemIndex, INT OrderIndex, LPWSTR Text, INT Width)
{
    LVCOLUMN lvColumn;

    lvColumn.mask = LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH | LVCF_ORDER;
    lvColumn.cx = (Width);
    lvColumn.pszText = Text;
    lvColumn.iSubItem = SubItemIndex;
    lvColumn.iOrder = OrderIndex;

    return(ListView_InsertColumn(ListViewHWND, ColumnIndex, &lvColumn));
}

BOOL ObtainDevice()
{
    hGlobalDriver = CreateFileA("\\\\.\\CallMon", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    if (hGlobalDriver != INVALID_HANDLE_VALUE)
    {
        return(TRUE);
    }
    return(FALSE);
}

BOOL CreatePipe()
{
    hGlobalPipe = CreateNamedPipeA("\\\\.\\pipe\\CallMonPipe", PIPE_ACCESS_INBOUND, PIPE_READMODE_BYTE | PIPE_WAIT, 1, 0x1000, 0x1000, INFINITE, NULL);
    if (hGlobalPipe != INVALID_HANDLE_VALUE)
    {
        return(TRUE);
    }
    return(FALSE);
}

BOOL AddProcess(WORD ProcessId)
{
    if (hGlobalDriver != INVALID_HANDLE_VALUE)
    {
        if (ProcessId)
        {
            if (DeviceIoControl(hGlobalDriver, IOCTL_ADD_PROCESS, &ProcessId, sizeof(ProcessId), NULL, 0, NULL, NULL))
            {
                return(TRUE);
            }
        }
    }
    return(FALSE);
}

BOOL GetDriverPrivilege()
{
    TOKEN_PRIVILEGES  tokenPrivs;
    HANDLE            hToken;
    LUID              luid;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
    {
        if (LookupPrivilegeValueA(NULL, "SeLoadDriverPrivilege", &luid))
        {
            tokenPrivs.PrivilegeCount = 1;
            tokenPrivs.Privileges[0].Luid = luid;
            tokenPrivs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            if (AdjustTokenPrivileges(hToken, FALSE, &tokenPrivs, sizeof(tokenPrivs), NULL, NULL))
            {
                CloseHandle(hToken);
                return(TRUE);
            }
        }
    }
    return(FALSE);
}

BOOL LoadDriver(HANDLE hDriver)
{
    UNICODE_STRING  usDriver;
    NtLoadDriver    pNtLoadDriver;
    NTSTATUS        ntRet;
    DWORD           dwData;
    WCHAR           wzDriver[MAX_PATH];
    HKEY            hKey;

    GetFinalPathNameByHandleW(hDriver, (LPWSTR)&wzDriver, MAX_PATH * sizeof(WCHAR), FILE_NAME_NORMALIZED | VOLUME_NAME_NT);
    RegCreateKeyA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\AltCall.sys", &hKey);
    RegSetValueExW(hKey, L"ImagePath", 0, REG_SZ, (LPWSTR)&wzDriver, lstrlenW((LPCWSTR)&wzDriver) * sizeof(WCHAR));
    dwData = 0x1;
    RegSetValueExA(hKey, "Type", 0, REG_DWORD, (BYTE*)&dwData, sizeof(dwData));
    RtlInitUnicodeString(&usDriver, L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Services\\AltCall.sys");
    if (pNtLoadDriver = (NtLoadDriver)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtLoadDriver"))
    {
        ntRet = pNtLoadDriver(&usDriver);
        if ((!ntRet) || (ntRet == 0xC000010E)) // STATUS_IMAGE_ALREADY_LOADED
        {
            return(TRUE);
        }
        else if (ntRet == 0xC0000428)
        {
            MessageBoxA(0, "You must disable driver signature enforcement (DSE) before using this program!\n"
                "This is possible by running windows in test mode.", "Error", MB_ICONERROR);
        }
    }
    return(FALSE);
}
