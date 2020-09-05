# CallMon
CallMon is a system call monitoring tool that works on Windows 10 versions 2004+ using PsAltSystemCallHandlers. 

## Usage
 * <b>CallMon requires driver signature enforcement (DSE) to be disabled. </b>
 * Download release [here](https://github.com/DownWithUp/CallMon/releases/tag/v1.0.0) (or download and build from source)
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

# Rust Driver Version
Optionally, there is a version of the AltCall.sys driver written in Rust. The sources and binary are included only in the repository and not in the release. I highly recommended reading not-matthias' (his code was the foundation for the Rust version) [blog post](https://not-matthias.github.io/kernel-driver-with-rust/) on building Windows drivers in Rust. In addition, I will mention that I worked on this to better my Rust skills and not to make a memory safe driver. I heavily used "unsafe" Rust code, and kernel interactions in themselves can always go awire. <br>
### Build
If you are not already on the nightly channel, change to it using:<br>
<code>rustup toolchain install nightly</code><br>
Override using:<br>
<code>rustup override set nightly</code><br>
### C VS. Rust
Besides, the obvious syntax differences, I also made some design changes:<br>
* Rust version uses ProbeForRead instead of MmHighestUserAddress and MmIsAddressValid check for stack pointer.
* Rust version ~~has no remove process IOCTL handling function (possibly coming soon?)~~ now has support for removing processes!
