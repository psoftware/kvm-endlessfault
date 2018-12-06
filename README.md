# README #

### Build ###
```shell
make
```

kvm.cpp and every other cpp file in frontend, backend, elf, debug-server and gdbserver folders will be compiled to generate the kvm executable file.
Example sources of target/ folder will be compiled but not linked to kvm.

### Run ###
The emulator needs an elf file to execute. The executable runs directly in long paged mode.
```shell
./kvm elfpath
```
Log output goes automatically into console.log file.

You can start the emulator in a new (xterm) console using
```bash
./run elfpath
```
The main console will be used as a log output.

Elf examples can be found in the build/ directory after building with make.
There are also some precompiled and more complex examples in esempi-io/ folder

### Parameters ####
```bash
./kvm -logfile file # -> to set the log file path
```

Other parameters can be set into the main folder .ini file.

### IO Emulation ###
Actually there are only a few emulated devices: video card (text-mode only), keyboard and serial ports. The emulation is very basic.

### GDB Server ###
This supervisor includes an almost complete implementation of a gdbstub.
You can debug the program running on the VM using a gdb client and all the common commands like step, next, continue, memory commands and many others.
The server must be enabled using the config.ini file.
It's possible to attach a gdb client to a running supervisor instance using this set of commands:

```
(gdb) set architecture i386:x86-64
(gdb) file elfpath
(gdb) target remote address:port
```

You can also use the custom gdb command

```
(gdb) monitor regs
```

to get the content of some low level registers (not listed with the standard register command) like CR4, CR3, CR2, CR0 and EFER.

### Live migration (experimental) ###
A virtual machine can be migrated to another instance of kvm-endlessfault running on another host (reachable by network address).
Start kvm-endlessfault as usual:

```
./run elfpath
```

Next, run another instance of kvm-endless fault, this time using run_migr script

```
./run_migr
```

Note: elf file must not be specified because this instance will host the migrated VMM.

Enter the (first) VMM internal console from another terminal and launch migrate command:

```
nc 127.0.0.1 9000
vmm console> migrate 127.0.0.1 8000
```
Telnet can be also used.\
Default internal console port is 9000.\
Default migration port is 8000.

#### Live migration without xterm window ####
Starting kvm-endlessfault instance:

```
./kvm <elfpath> -consoleport 9000
```

Starting the receiving kvm-endlessfault instance:
```
./kvm <elfpath> -migr -migrport 8000
```

Then, from another terminal:

```
nc 127.0.0.1 9000
vmm console> migrate 127.0.0.1 8000
```


### Developers ###
This project is developed by\
Antonio Le Caldare (kvm skeleton code, gdbstub, keyboard emulation, elf execution, live migration)\
Vincent Della Corte (video card text-mode emulation, code polish, comment review)\
Vincenzo Consales (elf loader, bootloader code, custom memory debug server)

as a project for Virtualization university course (prof. [@giuseppelettieri]( https://github.com/giuseppelettieri )).

### References ###
Part of kvm bootloading code (for protected, long and paged mode) is took from:\
https://github.com/dpw/kvm-hello-world

The INI reader class code is took from:\
https://github.com/benhoyt/inih

### Notes ###
Some variable names, comments and commit messages are written in Italian language due to the purpose of the project. Just open an issue instance if you find unreadable code or comments.