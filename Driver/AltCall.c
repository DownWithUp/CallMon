#include <ntifs.h>
#include <windef.h>
#include <intrin.h>
#include "Extras.h"

#pragma warning(disable : 4201)

#define ProcessAltSystemCallInformation 0x64
#define RTL_WALK_USER_MODE_STACK        0x00000001
#define IOCTL_ADD_PROCESS               0x550000
#define IOCTL_REMOVE_PROCESS            0x550002
#define IOCTL_INIT                      0x550004

typedef DWORD64 QWORD;
typedef UINT16  WORD;
typedef UCHAR   BYTE;
typedef NTSTATUS(NTAPI* PsRegisterAltSystemCallHandler)(PVOID HandlerFunction, LONG HandlerIndex);
typedef NTSTATUS(NTAPI* ZwSetInformationProcess)(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation,
    ULONG ProcessInformationLength);
typedef NTSTATUS(NTAPI* PsSuspendProcess)(PEPROCESS Process);
typedef NTSTATUS(NTAPI* PsResumeProcess)(PEPROCESS Process);


// Globals
PsRegisterAltSystemCallHandler  pPsRegisterAltSystemCallHandler;
ZwSetInformationProcess         pZwSetInformationProcess;
PsSuspendProcess                pPsSuspendProcess;
PsResumeProcess                 pPsResumeProcess;
HANDLE                          hGlobalPipe = 0;

NTSTATUS DriverUnload(PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);

    return(STATUS_ACCESS_DENIED);
}

BOOLEAN MyHandler(PKTRAP_FRAME Frame)
{
    IO_STATUS_BLOCK ioBlock;
    TOTAL_PACKET    totalPacket;
    NTSTATUS        ntRet;

    if (hGlobalPipe)
    {
        totalPacket.CustomHeader.ProcessId = (ULONG64)PsGetProcessId(PsGetCurrentProcess());
        if (((QWORD)Frame->Rsp < (QWORD)MmHighestUserAddress) && (MmIsAddressValid((PVOID)Frame->Rsp)))
        {
            memmove(&totalPacket.CustomHeader.StackData, (PVOID)Frame->Rsp, sizeof(totalPacket.CustomHeader.StackData));
        }
        memmove(&totalPacket.Frame, Frame, sizeof(KTRAP_FRAME));
        ntRet = ZwWriteFile(hGlobalPipe, 0, NULL, NULL, &ioBlock, &totalPacket, sizeof(totalPacket), NULL, NULL);
    }
    return(TRUE);
}

NTSTATUS AddProcess(DWORD PID)
{
    OBJECT_ATTRIBUTES   objAttr;
    CLIENT_ID           clientId;
    HANDLE              hProcess;
    QWORD               qwPID;


    InitializeObjectAttributes(&objAttr, NULL, OBJ_KERNEL_HANDLE, 0, 0);
    clientId.UniqueProcess = (HANDLE)PID;
    clientId.UniqueThread = 0;
    hProcess = 0;
    if (NT_SUCCESS(ZwOpenProcess(&hProcess, PROCESS_ALL_ACCESS, &objAttr, &clientId)))
    {
        qwPID = (QWORD)clientId.UniqueProcess;
        if (NT_SUCCESS(pZwSetInformationProcess(hProcess, (PROCESSINFOCLASS)ProcessAltSystemCallInformation, &qwPID, 1)))
        {
            ZwClose(hProcess);
            return(STATUS_SUCCESS);
        }
    }
    return(STATUS_UNSUCCESSFUL);
}

// ETHREAD is version dependent
typedef struct _ETHREAD
{
    BYTE Random[0x4E8];
    LIST_ENTRY ThreadListEntry;
}ETHREAD;

NTSTATUS RemoveProcess(DWORD PID)
{
    OBJECT_ATTRIBUTES   objAttr;
    PLIST_ENTRY         pThreadHead;
    PLIST_ENTRY         pThreadNext;
    PEPROCESS           pProcess;
    CLIENT_ID           clientId;
    PETHREAD            pThread;
    NTSTATUS            ntRet;
    HANDLE              hProcess;
    QWORD               qwPID;


    InitializeObjectAttributes(&objAttr, NULL, OBJ_KERNEL_HANDLE, 0, 0);
    clientId.UniqueProcess = (HANDLE)PID;
    clientId.UniqueThread = 0;
    hProcess = 0;
    ntRet = STATUS_UNSUCCESSFUL;
    if (NT_SUCCESS(ZwOpenProcess(&hProcess, PROCESS_ALL_ACCESS, &objAttr, &clientId)))
    {
        qwPID = (QWORD)clientId.UniqueProcess;
        
        if (NT_SUCCESS(ObReferenceObjectByHandleWithTag(hProcess, PROCESS_ALL_ACCESS, *PsProcessType, KernelMode, 0x75537350, &pProcess, NULL)))
        {
            ntRet = pPsSuspendProcess(pProcess);
            if (NT_SUCCESS(ntRet))
            {
                /*
                    Because Microsoft doesn't document the structure of EPROCESS or KTHREAD 
                    members and offsets here could change in a new release.
                    See: https://docs.microsoft.com/en-us/windows-hardware/drivers/kernel/eprocess
                */
                pThreadHead = (PLIST_ENTRY)((BYTE*)pProcess + 0x5E0); //EPROCESS.ThreadListHead
                pThreadNext = pThreadHead->Flink;

                while (pThreadNext != pThreadHead)
                {
                    pThread = (PETHREAD)((BYTE*)pThreadNext - 0x4E8);
                    _interlockedbittestandreset((PLONG)pThread, 0x1D);
                    pThreadNext = pThreadNext->Flink;
                }
               
                ntRet = pPsResumeProcess(pProcess);
            }
            ObDereferenceObjectWithTag(pProcess, 0x75537350);
        }
        ZwClose(hProcess);
    }
    return(ntRet);
}

NTSTATUS DeviceDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PIO_STACK_LOCATION  pStack;
    OBJECT_ATTRIBUTES   objPipe;
    IO_STATUS_BLOCK     ioBlock;
    UNICODE_STRING      usPipe;
    NTSTATUS            ntRet;
    UNREFERENCED_PARAMETER(DeviceObject);

    pStack = IoGetCurrentIrpStackLocation(Irp);
    ntRet = STATUS_SUCCESS;
    if (pStack->MajorFunction == IRP_MJ_DEVICE_CONTROL)
    {
        switch (pStack->Parameters.DeviceIoControl.IoControlCode)
        {
        case IOCTL_ADD_PROCESS:
            if (pStack->Parameters.DeviceIoControl.InputBufferLength == 4)
            {
                ntRet = AddProcess(*(DWORD*)Irp->AssociatedIrp.SystemBuffer);
            }
            else
            {
                ntRet = STATUS_BUFFER_TOO_SMALL;
            }
            break;
        case IOCTL_REMOVE_PROCESS:
            if (pStack->Parameters.DeviceIoControl.InputBufferLength == 4)
            {
                ntRet = RemoveProcess(*(DWORD*)Irp->AssociatedIrp.SystemBuffer);
            }
            else
            {
                ntRet = STATUS_BUFFER_TOO_SMALL;
            }
            break;
        case IOCTL_INIT:
            if (hGlobalPipe)
            {
                ZwClose(hGlobalPipe);
                hGlobalPipe = 0;
            }
            if (!hGlobalPipe)
            {
                RtlInitUnicodeString(&usPipe, L"\\Device\\NamedPipe\\CallMonPipe");
                InitializeObjectAttributes(&objPipe, &usPipe, OBJ_KERNEL_HANDLE, 0, 0);
                if (NT_SUCCESS(ZwCreateFile(&hGlobalPipe, FILE_WRITE_DATA | SYNCHRONIZE, &objPipe, &ioBlock, NULL, 0, 0, FILE_OPEN, 
                                            FILE_SYNCHRONOUS_IO_NONALERT | FILE_DELETE_ON_CLOSE, NULL, 0)))
                {
                    ntRet = STATUS_SUCCESS;
                }
                else
                {
                    ntRet = STATUS_PIPE_BROKEN;
                }
            }
            break;
        default:
            break;
        }

    }
    Irp->IoStatus.Status = ntRet;
    IofCompleteRequest(Irp, IO_NO_INCREMENT);
    return(ntRet);
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    UNICODE_STRING  usRoutine;
    UNICODE_STRING  usDevice;  
    UNICODE_STRING  usSymLink;
    PDEVICE_OBJECT  pDeviceObj;

    UNREFERENCED_PARAMETER(RegistryPath);

    RtlInitUnicodeString(&usRoutine, L"PsRegisterAltSystemCallHandler");
    pPsRegisterAltSystemCallHandler = (PsRegisterAltSystemCallHandler)MmGetSystemRoutineAddress(&usRoutine);
    RtlInitUnicodeString(&usRoutine, L"ZwSetInformationProcess");
    pZwSetInformationProcess = (ZwSetInformationProcess)MmGetSystemRoutineAddress(&usRoutine);
    RtlInitUnicodeString(&usRoutine, L"PsSuspendProcess");
    pPsSuspendProcess = (PsSuspendProcess)MmGetSystemRoutineAddress(&usRoutine);
    RtlInitUnicodeString(&usRoutine, L"PsResumeProcess");
    pPsResumeProcess = (PsResumeProcess)MmGetSystemRoutineAddress(&usRoutine);

    if ((pPsRegisterAltSystemCallHandler) && (pZwSetInformationProcess) && (pPsSuspendProcess) && (pPsResumeProcess))
    {
        DriverObject->DriverUnload = DriverUnload;
        for (int i = 0;  i < IRP_MJ_MAXIMUM_FUNCTION; i++)
        {
            DriverObject->MajorFunction[i] = DeviceDispatch;
        }
        RtlInitUnicodeString(&usDevice, L"\\Device\\CallMon");
        if (NT_SUCCESS(IoCreateDevice(DriverObject, 0, &usDevice, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &pDeviceObj)))
        {
            RtlInitUnicodeString(&usSymLink, L"\\DosDevices\\CallMon");
            if (NT_SUCCESS(IoCreateSymbolicLink(&usSymLink, &usDevice)))
            {
                pDeviceObj->Flags |= DO_BUFFERED_IO;
                pPsRegisterAltSystemCallHandler((PVOID)MyHandler, 1);
                return(STATUS_SUCCESS);
            }
        }
    }
    return(STATUS_UNSUCCESSFUL);
}
