# CallMon
CallMon is a system call monitoring tool that works on Windows 10 versions 2004+ using PsAltSystemCallHandlers. 

## Architecture


## Performance Impacts
Because the system call handler function is called everytime a targeted process preforms a call (and in the context of the targeted process), heavy API usage programs will experience a drop in performance due to the transfer of data back to the CallMon GUI process.

## Resources
[0xcpu's Research on AltSyscallHandlers](https://github.com/0xcpu/WinAltSyscallHandler)
