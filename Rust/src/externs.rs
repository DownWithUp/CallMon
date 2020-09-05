use winapi::km::wdm::*;
use winapi::km::wdm::IO_PRIORITY::KPRIORITY_BOOST;
use winapi::um::winnt::ACCESS_MASK;
use winapi::shared::ntdef::*;
use winapi::shared::basetsd::SIZE_T;
//crate::defines
use crate::PCLIENT_ID;
use crate::POBJECT_HANDLE_INFORMATION;

extern "system" {

    pub fn ProbeForRead(Address: PVOID, Length: SIZE_T, Alignment: ULONG);

    pub fn IoCreateDevice(DriverObject: PDRIVER_OBJECT, DeviceExtension: ULONG, DeviceName: PUNICODE_STRING, 
        DeviceType: ULONG, DeviceCharacteristics: ULONG, Exclusive: BOOLEAN, DeviceObject: *mut*mut DEVICE_OBJECT) -> NTSTATUS;

    pub fn IoCreateSymbolicLink(SymbolicLinkName: PUNICODE_STRING, DeviceName: PUNICODE_STRING) -> NTSTATUS;

    pub fn MmGetSystemRoutineAddress(SystemRoutineName: PUNICODE_STRING) -> PVOID;
    
    pub fn IofCompleteRequest(Irp : PIRP, PriorityBoost: KPRIORITY_BOOST);

    pub fn ZwSetInformationProcess(ProcessHandle: HANDLE, ProcessInformationClass: ULONG, ProcessInformation: PVOID,
        ProcessInformationLength: ULONG) -> NTSTATUS;
        
    pub fn PsSuspendProcess(Process: PEPROCESS) -> NTSTATUS;

    pub fn PsResumeProcess(Process: PEPROCESS) -> NTSTATUS;

    pub fn ZwCreateFile(FileHandle: PHANDLE, AccessMask: ACCESS_MASK, ObjectAttributes: POBJECT_ATTRIBUTES, IoStatusBlock: PIO_STATUS_BLOCK,
        AllocationSize: PLARGE_INTEGER, FileAttributes: ULONG, ShareAccess: ULONG, CreateDisposition: ULONG, CreateOptions: ULONG, EaBuffer: PVOID,
        EaLength: ULONG) -> NTSTATUS;
    
    pub fn ZwOpenProcess(ProcessHandle: PHANDLE, DesiredAccess: ACCESS_MASK, ObjectAttributes: POBJECT_ATTRIBUTES, 
        ClientId: PCLIENT_ID) -> NTSTATUS;

    pub fn ZwWriteFile(FileHandle: HANDLE, Event: HANDLE, ApcRoutine: PVOID, ApcContext: PVOID, IoStatusBlock: PIO_STATUS_BLOCK,
        Buffer: PVOID, Length: ULONG, ByteOffset: PLARGE_INTEGER, Key: PULONG) -> NTSTATUS;
    
    pub fn ZwClose(Handle: HANDLE) -> NTSTATUS;

    pub fn memmove(Destination: PVOID, Source: PVOID, Size: SIZE_T) -> PVOID; // cdecl

    pub fn PsGetProcessId(Process: PEPROCESS) -> HANDLE;

    pub fn ObReferenceObjectByHandle(Handle: HANDLE, DesiredAccess: ACCESS_MASK, ObjectType: PVOID, AccessMode: KPROCESSOR_MODE,
        Object: *mut PVOID, HandleInformation: POBJECT_HANDLE_INFORMATION) -> NTSTATUS;

    pub fn PsGetCurrentProcess() -> PEPROCESS;

    pub fn ObDereferenceObject(Object: PVOID) -> NTSTATUS;
}
