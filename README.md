# CallMon
CallMon is a system call monitoring tool that works on Windows 10 versions 2004+ using PsAltSystemCallHandlers. 

## Usage
 * <b>CallMon requires driver signature enforcement (DSE) to be disabled. </b>
 * Download release (or build from source)
 * Ensure both CallMon.exe and AltCall.sys are in the same directory
 * Run CallMon.exe as an administrator
 * Click on "Initialize"
 * Enter a process's ID in the text field and click "Add Process"

## Architecture
CallMon is comprised of a kernel driver (AltCall.sys) and a GUI application (CallMon.exe). Together, these programs work to provide API introspection for monitored processes.
The driver and GUI application communicate via a named pipe (\\\\.\pipe\CallMonPipe). The data passed by the driver to usermode consists of a custom header which contains the process id and stack information along with a KTRAP_FRAME structure received from the alt syscall handler function.

## Performance Impacts
Because the system call handler function is called everytime a targeted process preforms a call (and in the context of the targeted process), heavy API usage programs will experience a drop in performance due to the transfer of data back to the CallMon GUI process.

## Resources
[0xcpu's Research on AltSyscallHandlers](https://github.com/0xcpu/WinAltSyscallHandler)
