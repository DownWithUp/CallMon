#![no_std]
#![feature(alloc_error_handler)]
#![allow(non_snake_case)]
#![feature(asm)]

extern crate alloc;

use crate::{string::create_unicode_string, externs::IofCompleteRequest, externs::MmGetSystemRoutineAddress, 
    externs::PsGetCurrentProcess, externs::PsGetProcessId, externs::ZwClose, externs::ZwCreateFile, externs::ZwOpenProcess, 
    externs::ZwSetInformationProcess, externs::ProbeForRead, externs::ZwWriteFile, externs::memmove, 
    externs::ObReferenceObjectByHandle, externs::ObDereferenceObject, externs::PsSuspendProcess, externs::PsResumeProcess};
pub mod externs;
pub mod log;
pub mod string;
pub mod defines;

use core::panic::PanicInfo;
use winapi::um::winnt::PROCESS_ALL_ACCESS;
use winapi::km::wdm::KPROCESSOR_MODE::KernelMode;
use winapi::km::wdm::*;
use winapi::shared::ntdef::*;
use winapi::shared::ntstatus::*;
use defines::*;

// Globals
static mut hGlobalPipe: HANDLE = 0 as HANDLE;

// When using the alloc crate it seems like it does some unwinding. Adding this
// export satisfies the compiler but may introduce undefined behaviour when a
// panic occurs.
#[no_mangle]
pub extern "system" fn __CxxFrameHandler3(_: *mut u8, _: *mut u8, _: *mut u8, _: *mut u8) -> i32 { unimplemented!() }
// Hack, explanation can be found here: https://github.com/Trantect/win_driver_example/issues/4
#[export_name = "_fltused"]
static _FLTUSED: i32 = 0;

#[global_allocator]
static GLOBAL: kernel_alloc::KernelAlloc = kernel_alloc::KernelAlloc;

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! 
{ 
    loop {};
}

extern "system" fn DriverUnloadFunction(DriverObject: &mut DRIVER_OBJECT)
{
    kernel_print::kernel_println!("Unload attempt");
    return;
}

pub extern "system" fn MyHandler(Frame: PKTRAP_FRAME) -> BOOLEAN
{
    unsafe
    {
        use core::ptr::null_mut;
        use core::mem::zeroed;
        use core::mem::transmute;

        let mut totalPacket: TOTAL_PACKET = core::mem::zeroed::<TOTAL_PACKET>();
        let mut ioBlock: IO_STATUS_BLOCK = core::mem::zeroed::<IO_STATUS_BLOCK>();

        totalPacket.CustomHeader.ProcessId = PsGetProcessId(PsGetCurrentProcess()) as QWORD;
        let mut TryProbe = ||
        {
            if (*Frame).Rsp as QWORD != 0
            {
                ProbeForRead((*Frame).Rsp as PVOID, (totalPacket.CustomHeader.StackData.len() * core::mem::size_of::<QWORD>()), 4);
                memmove(transmute(&mut totalPacket.CustomHeader.StackData), (*Frame).Rsp as PVOID, 
                    (totalPacket.CustomHeader.StackData.len() * core::mem::size_of::<QWORD>()));
            }
        };
        TryProbe();
        memmove(transmute(&mut totalPacket.Frame), Frame as PVOID, core::mem::size_of::<KTRAP_FRAME>());
        ZwWriteFile(hGlobalPipe, null_mut(), null_mut(), null_mut(), &mut ioBlock, transmute(&mut totalPacket), 
            core::mem::size_of::<TOTAL_PACKET>() as ULONG, null_mut(), null_mut());

        return TRUE;
    }
}

fn AddProcess(PID: DWORD) -> NTSTATUS
{
    use core::ptr::null_mut;

    unsafe
    {
        let mut objAttr: OBJECT_ATTRIBUTES = core::mem::zeroed::<OBJECT_ATTRIBUTES>();     
        let mut clientId: CLIENT_ID = core::mem::zeroed::<CLIENT_ID>();
        InitializeObjectAttributes(&mut objAttr, null_mut(), OBJ_KERNEL_HANDLE, null_mut(), null_mut()); 
        clientId.UniqueProcess = PID as HANDLE;
        clientId.UniqueThread = 0 as HANDLE;
        let mut hProcess: HANDLE = 0 as HANDLE;
        if NT_SUCCESS(ZwOpenProcess(&mut hProcess, PROCESS_ALL_ACCESS, &mut objAttr, &mut clientId))
        {
            let mut NewPID: QWORD = PID.into();
            
            if NT_SUCCESS(ZwSetInformationProcess(hProcess, ProcessAltSystemCallInformation, &mut NewPID as *mut QWORD as PVOID, 1))
            {
                ZwClose(hProcess);
                return STATUS_SUCCESS;
            }
        }
    }
    return STATUS_UNSUCCESSFUL;
}

fn RemoveProcess(PID: DWORD) -> NTSTATUS
{
    use core::ptr::null_mut;
    let mut ntRet: NTSTATUS = STATUS_SUCCESS;

    unsafe
    {
        let mut objAttr: OBJECT_ATTRIBUTES = core::mem::zeroed::<OBJECT_ATTRIBUTES>();     
        let mut clientId: CLIENT_ID = core::mem::zeroed::<CLIENT_ID>();
        InitializeObjectAttributes(&mut objAttr, null_mut(), OBJ_KERNEL_HANDLE, null_mut(), null_mut()); 
        clientId.UniqueProcess = PID as HANDLE;
        clientId.UniqueThread = 0 as HANDLE;
        let mut hProcess: HANDLE = 0 as HANDLE;
        if NT_SUCCESS(ZwOpenProcess(&mut hProcess, PROCESS_ALL_ACCESS, &mut objAttr, &mut clientId))
        {
            let mut NewPID: QWORD = PID.into();       
            let mut pProcess: PEPROCESS = null_mut();


            if NT_SUCCESS(ObReferenceObjectByHandle(hProcess, PROCESS_ALL_ACCESS, null_mut(), KernelMode, &mut pProcess, null_mut()))
            {
                ntRet = PsSuspendProcess(pProcess);
                let mut pThreadHead: PLIST_ENTRY = null_mut();
                let mut pThreadNext: PLIST_ENTRY = null_mut();

                pThreadHead = (pProcess as QWORD + 0x5E0) as PLIST_ENTRY;
                pThreadNext = (*pThreadHead).Flink;
                let mut pThread: PVOID = null_mut();
                while pThreadNext != pThreadHead
                {
                    pThread = pThreadNext as PVOID;
                    pThread = (pThread as QWORD - 0x4E8) as PVOID;
                    /*
                        asm block to replace the _interlockedbittestandreset macro
                        lock btr dword ptr [rax], 1Dh
                    */
                    asm!("lock btr dword ptr [{0}], 0x1D", inout(reg) pThread);
                    pThreadNext = (*pThreadNext).Flink;
                }

                ntRet = PsResumeProcess(pProcess);
                ObDereferenceObject(pProcess);
            }

            ZwClose(hProcess);
        
        }
    }
    return STATUS_SUCCESS;
}

fn PreformInit() -> NTSTATUS
{
    use core::ptr::null_mut;
    use core::mem::zeroed;
    
    let mut ntRet: NTSTATUS = STATUS_SUCCESS;
    unsafe
    {
        if hGlobalPipe != 0 as HANDLE
        {
            ZwClose(hGlobalPipe);
            hGlobalPipe = 0 as HANDLE;
        }
        else
        {
            let mut objPipe: OBJECT_ATTRIBUTES = core::mem::zeroed::<OBJECT_ATTRIBUTES>();
            let mut usPipe = create_unicode_string(obfstr::wide!("\\Device\\NamedPipe\\CallMonPipe"));
            InitializeObjectAttributes(&mut objPipe, &mut usPipe, OBJ_KERNEL_HANDLE, null_mut(), null_mut());
            let mut ioBlock: IO_STATUS_BLOCK = core::mem::zeroed::<IO_STATUS_BLOCK>();
            if NT_SUCCESS(ZwCreateFile(&mut hGlobalPipe, FILE_WRITE_DATA | SYNCHRONIZE, &mut objPipe, &mut ioBlock, null_mut(), 0, 0, FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT | FILE_DELETE_ON_CLOSE, NULL, 0))
            {
                ntRet = STATUS_SUCCESS;
            }
            else
            {
                ntRet = STATUS_PIPE_BROKEN;
            }
        }
    }
    return ntRet;
}

extern "system" fn DeviceDispatch(DeviceObject: &mut DEVICE_OBJECT, Irp: &mut IRP) -> NTSTATUS
{
    let pStack: &mut IO_STACK_LOCATION;
    let mut ntRet: NTSTATUS = STATUS_SUCCESS;

    unsafe
    {
        let FunctionCode = (*(*Irp.Tail.Overlay().__bindgen_anon_2.__bindgen_anon_1.CurrentStackLocation())).MajorFunction;
        let Stack = (*Irp.Tail.Overlay().__bindgen_anon_2.__bindgen_anon_1.CurrentStackLocation());
        let IOCTL = (*Stack).Parameters.DeviceIoControl().IoControlCode as u32;
        if FunctionCode == IRP_MJ::DEVICE_CONTROL as u8
        {
            match IOCTL
            {
                IOCTL_ADD_PROCESS =>
                {
                    if (*Stack).Parameters.DeviceIoControl().InputBufferLength as DWORD == 4
                    {
                        let mut buf = (*Irp.AssociatedIrp.SystemBuffer_mut());
                        let PID: &mut DWORD = &mut *(buf as *mut DWORD);
                        ntRet = AddProcess(*PID);
                    }
                    else 
                    {
                        ntRet = STATUS_BUFFER_TOO_SMALL;
                    }
                },
                IOCTL_REMOVE_PROCESS =>
                {
                    if (*Stack).Parameters.DeviceIoControl().InputBufferLength as DWORD == 4
                    {
                        let mut buf = (*Irp.AssociatedIrp.SystemBuffer_mut());
                        let PID: &mut DWORD = &mut *(buf as *mut DWORD);
                        ntRet = RemoveProcess(*PID);
                    }
                    else 
                    {
                        ntRet = STATUS_BUFFER_TOO_SMALL;
                    }

                },
                IOCTL_INIT => ntRet = PreformInit(),
                _ => kernel_print::kernel_println!("Invalid IOCTL (Might not be supported in rust version)"),
            } 
        }
        let a = Irp.IoStatus.__bindgen_anon_1.Status_mut();
        *a = ntRet;
        IofCompleteRequest(Irp, 0);
    }
    return STATUS_SUCCESS;
}

#[no_mangle]
pub extern "system" fn DriverEntry(DriverObject: &mut DRIVER_OBJECT, RegistryPath: PUNICODE_STRING) -> NTSTATUS 
{
    use core::mem::transmute;

    DriverObject.DriverUnload = Some(DriverUnloadFunction);
    let usDevice = create_unicode_string(obfstr::wide!("\\Device\\CallMon\0"));
    unsafe
    {
        use winapi::km::wdm::DEVICE_TYPE::FILE_DEVICE_UNKNOWN;
        let mut outDevice: *mut DEVICE_OBJECT = core::ptr::null_mut();

        if NT_SUCCESS(IoCreateDevice(DriverObject, 0, &usDevice, FILE_DEVICE_UNKNOWN,  FILE_DEVICE_SECURE_OPEN, FALSE, &mut outDevice))
        {
            kernel_print::kernel_println!("Finished initializing!");
            let usSymLink = create_unicode_string(obfstr::wide!("\\DosDevices\\CallMon"));
            if NT_SUCCESS(IoCreateSymbolicLink(&usSymLink, &usDevice))
            {
                (*outDevice).Flags |= DEVICE_FLAGS::DO_BUFFERED_IO as u32;
                for i in 0..IRP_MJ::MAXIMUM_FUNCTION as usize
                {
                    DriverObject.MajorFunction[i] = Some(DeviceDispatch);
                }
                let mut usPsRegisterAltSystemCallHandler = create_unicode_string(obfstr::wide!("PsRegisterAltSystemCallHandler"));
                type PsRegisterAltSystemCallHandler = extern "system" fn(HandlerRoutine: PVOID, HandlerIndex: LONG) -> NTSTATUS;
                let pPsRegisterAltSystemCallHandler: PsRegisterAltSystemCallHandler = transmute(MmGetSystemRoutineAddress(&mut usPsRegisterAltSystemCallHandler));
                if NT_SUCCESS(pPsRegisterAltSystemCallHandler(transmute(MyHandler as PVOID), 1))
                {
                    return STATUS_SUCCESS;
                }
            }
        }
    }
    return STATUS_UNSUCCESSFUL
}
