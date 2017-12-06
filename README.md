# README #

### Compilazione ###
`make`

kvm.cpp e tutti i file cpp contenuti in frontend, backend, elf, debug-server e gdbserver saranno compilati e linkati per generare l'eseguibile kvm.

### Avvio ###
L'avvio può essere compiuto nella console dalla quale si digita il comando usando
`./kvm patheseguibile`

oppure (CONSIGLIATO) si può usare
`./run patheseguibile`
che apre una nuova console e stampa l'output di log sulla console iniziale.

### GDB Server ###
E' possibile sfruttare gdb per debuggare il programma avviato sulla Virtual Machine.
Innanzitutto va abilitato il server nel file config.ini, nella sezione gdb-server, e, dopo aver avviato il Virtual Machine Monitor (usando run o kvm) va avviato il client gdb.
Nella console del debugger è necessario inserire i seguenti comandi:
`set architecture i386:x86-64`
`file patheseguibile`
`target remote 127.0.0.1:1234`

Se tutto è andato a buon fine sarà possibile digitare i normali comandi per le operazioni di debug (breakpoint, continue, step, next...).

Inoltre è disponibile il comando aggiuntivo (custom)
`monitor regs`
che permette di stampare il contenuto dei registri privilegiati (CR4, CR3, CR2, CR0, EFER).
