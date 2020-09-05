/*
    Some defines and structs that are custom or was unable to find 
    in the winapi crate.
*/
#![allow(non_snake_case)]

use winapi::shared::ntdef::{ULONG, USHORT, UCHAR, KIRQL, HANDLE};
use winapi::um::winnt::M128A;
use winapi::km::wdm::KPROCESSOR_MODE;
use winapi::um::winnt::ACCESS_MASK;
pub type QWORD = u64;
pub const FILE_DEVICE_SECURE_OPEN: ULONG            = 0x00000100;
pub const FILE_DELETE_ON_CLOSE: ULONG               = 0x00001000;
pub const FILE_SYNCHRONOUS_IO_NONALERT:ULONG        = 0x00000020;
pub const FILE_OPEN:ULONG                           = 0x00000001;
pub const PagedPool: ULONG                          = 0x00000001;
pub const ProcessAltSystemCallInformation: ULONG    = 0x00000064;

pub const IOCTL_ADD_PROCESS: ULONG     = 0x550000;
pub const IOCTL_REMOVE_PROCESS: ULONG  = 0x550002;
pub const IOCTL_INIT: ULONG            = 0x550004;

pub struct State
{
    pub abc: ULONG,
} 

pub struct _CLIENT_ID
{
    pub UniqueProcess: HANDLE,
    pub UniqueThread: HANDLE,
}
pub type CLIENT_ID = _CLIENT_ID;
pub type PCLIENT_ID = *mut _CLIENT_ID;

pub struct _OBJECT_HANDLE_INFORMATION {
    pub HandleAttributes: ULONG,
    pub GrantedAccess: ACCESS_MASK,
} 
pub type OBJECT_HANDLE_INFORMATION = _OBJECT_HANDLE_INFORMATION;
pub type POBJECT_HANDLE_INFORMATION =  *mut _OBJECT_HANDLE_INFORMATION;

pub struct _CUSTOM_HEADER
{
    pub ProcessId: QWORD,
    pub StackData: [QWORD; 16],
} 
pub type CUSTOM_HEADER  = _CUSTOM_HEADER;
pub type PCUSTOM_HEADER = *mut _CUSTOM_HEADER;


// Unions for KTRAP_FRAME
pub union u1 
{
    pub GsBase: QWORD,
    pub GsSwap: QWORD,
}

pub union u2
{
    pub FaultAddress: QWORD,
    pub ContextRecord: QWORD,
}

pub union u3 
{
    pub ErrorCode: QWORD,
    pub ExceptionFrame: QWORD,
}

#[repr(C)]
pub struct _KTRAP_FRAME 
{
     //
     // Home address for the parameter registers.
     //     
    pub P1Home: QWORD,
    pub P2Home: QWORD,
    pub P3Home: QWORD,
    pub P4Home: QWORD,
    pub P5: QWORD,

    //
    // Previous processor mode (system services only) and previous IRQL
    // (interrupts only).
    //
     
    pub PreviousMode: KPROCESSOR_MODE,
     
    pub PreviousIrql: KIRQL,
     
    //
    // Page fault load/store indicator.
    // 
    pub FaultIndicator: UCHAR,
     
    //
    // Exception active indicator.
    //
    //    0 - interrupt frame.
    //    1 - exception frame.
    //    2 - service frame.
    // 
    pub ExceptionActive: UCHAR,
    
    //
    // Floating point state.
    //
 
    pub MxCsr: ULONG,
     
    //
    //  Volatile registers.
    //
    // N.B. These registers are only saved on exceptions and interrupts. They
    //      are not saved for system calls.
    // 
    pub Rax: QWORD,
    pub Rcx: QWORD,
    pub Rdx: QWORD,
    pub R8: QWORD,
    pub R9: QWORD,
    pub R10: QWORD,
    pub R11: QWORD,
 
    //
    // Gsbase is only used if the previous mode was kernel.
    //
    // GsSwap is only used if the previous mode was user.
    //
    pub u1: u1,
     
    //
    // Volatile floating registers.
    //
    // N.B. These registers are only saved on exceptions and interrupts. They
    //      are not saved for system calls.
    //
     
    pub Xmm0: M128A,
    pub Xmm1: M128A,
    pub Xmm2: M128A,
    pub Xmm3: M128A,
    pub Xmm4: M128A,
    pub Xmm5: M128A,
 
    //
    // First parameter, page fault address, context record address if user APC
    // bypass.
    //

    pub u2: u2, 
  
    //
    //  Debug registers.
    // 
    pub Dr0: QWORD,
    pub Dr1: QWORD,
    pub Dr2: QWORD,
    pub Dr3: QWORD,
    pub Dr6: QWORD,
    pub Dr7: QWORD,     
    //
    // Special debug registers.
    //

    //pub DebugRegs: _DEBUG_REGS,

    pub DebugControl: QWORD,
    pub LastBranchToRip: QWORD,
    pub LastBranchFromRip: QWORD,  
    pub LastExceptionToRip: QWORD,
    pub LastExceptionFromRip: QWORD,
         
     //
     //  Segment registers
     //
         
    pub SegDs: USHORT,
    pub SegEs: USHORT,
    pub SegFs: USHORT,
    pub SegGs: USHORT,
        
    //
    // Previous trap frame address.
    //
        
    pub TrapFrame: QWORD,
        
    //
    // Saved nonvolatile registers RBX, RDI and RSI. These registers are only
    // saved in system service trap frames.
    //

    pub Rbx: QWORD,
    pub Rdi: QWORD,
    pub Rsi: QWORD,
         
    //
    // Saved nonvolatile register RBP. This register is used as a frame
    // pointer during trap processing and is saved in all trap frames.
    // 
    pub Rbp: QWORD,
    
    //
    // Information pushed by hardware.
    //
    // N.B. The error code is not always pushed by hardware. For those cases
    //      where it is not pushed by hardware a dummy error code is allocated
    //      on the stack.
    //
    pub u3: u3, 

    pub Rip: QWORD,
    pub SegCs: USHORT,
    pub Fill0: UCHAR,
    pub Logging: UCHAR,
    pub Fill1: [USHORT;2],
    pub EFlags: ULONG,
    pub Fill2: ULONG,
    pub Rsp: QWORD,
    pub SegSs: USHORT,
    pub Fill3: USHORT,
    pub Fill4: ULONG,
}
pub type KTRAP_FRAME = _KTRAP_FRAME;
pub type PKTRAP_FRAME = *mut _KTRAP_FRAME;

pub struct _TOTAL_PACKET
{
    pub CustomHeader: CUSTOM_HEADER,
    /*
        The padding is here because for some reason, 
        the C version size of TOTAL_PACKET is 544
        while the rust version is 536. 
        In both versions the KTRAP_FRAME is 400
        and the CUSTOM_HEADER is 136 which should
        equal 536, but some optimization seems to occur
        which adds padding per struct in a struct.
    */
    CPadding: QWORD,
    pub Frame: KTRAP_FRAME,
}
pub type TOTAL_PACKET = _TOTAL_PACKET;
pub type PTOTAL_PACKET = *mut _TOTAL_PACKET;
