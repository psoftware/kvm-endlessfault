COMM_CFLAGS=-std=gnu++14 -g
LD_FLAGS=-pthread
ELFPROG_CFLAGS=-nostdlib -Ttext=0x200000 -fno-asynchronous-unwind-tables -m64 -Wl,--build-id=none -no-pie -Wl,-fuse-ld=gold -g

FRONTEND_CPP_FILES := $(wildcard frontend/*.cpp)
FRONTEND_OBJ_FILES = $(patsubst frontend/%.cpp,frontend/%.o,$(FRONTEND_CPP_FILES))

BACKEND_CPP_FILES := $(wildcard backend/*.cpp)
BACKEND_OBJ_FILES = $(patsubst backend/%.cpp,backend/%.o,$(BACKEND_CPP_FILES))

DEBUGSERVER_OBJ_FILES = debug-server/DebugServer.o debug-server/net_wrapper.o debug-server/messages.o

ELF_OBJ_FILES = elf/elf32.o elf/elf64.o elf/estrattore.o elf/interp.o
ELF_HEADER_FILES := $(wildcard elf/*.h)

GDBSERVER_OBJ_FILES = gdbserver/gdbserver.o
GDBSERVER_HEADER_FILES := $(wildcard gdbserver/*.h)

NETLIB_CPP_FILES := $(wildcard netlib/*.cpp)
NETLIB_OBJ_FILES = $(patsubst netlib/%.cpp,netlib/%.o,$(NETLIB_CPP_FILES))


## -- linking

all: kvm build build/boot64 build/prog_prova build/keyboard_program build/ricorsivo debug-client/debug_client

build:
	mkdir -p build/

kvm: kvm.o bootloader/Bootloader.o $(FRONTEND_OBJ_FILES) $(BACKEND_OBJ_FILES) $(ELF_OBJ_FILES) $(GDBSERVER_OBJ_FILES) $(DEBUGSERVER_OBJ_FILES) $(NETLIB_OBJ_FILES)
	g++ kvm.o bootloader/Bootloader.o $(FRONTEND_OBJ_FILES) $(BACKEND_OBJ_FILES) $(ELF_OBJ_FILES) $(GDBSERVER_OBJ_FILES) $(DEBUGSERVER_OBJ_FILES) $(NETLIB_OBJ_FILES) -o kvm $(LD_FLAGS)

build/prog_prova: target/prog_prova.c target/lib.s target/lib.h
	gcc $(ELFPROG_CFLAGS) target/prog_prova.c target/lib.s -o build/prog_prova

build/keyboard_program: target/keyboard_program.s
	gcc $(ELFPROG_CFLAGS) target/keyboard_program.s -o build/keyboard_program

build/ricorsivo: target/ricorsivo.c target/lib.s target/lib.h
	gcc $(ELFPROG_CFLAGS) target/ricorsivo.c target/lib.s -o build/ricorsivo

debug-client/debug_client: debug-client/debug_client.o debug-server/net_wrapper.o debug-server/messages.o backend/ConsoleLog.o
	g++ debug-client/debug_client.o debug-server/net_wrapper.o debug-server/messages.o backend/ConsoleLog.o -o debug-client/debug_client $(COMM_CFLAGS)
#--ompilazione

kvm.o: kvm.cpp backend/*.h frontend/*.h bootloader/Bootloader.h gdbserver/gdbserver.h netlib/migration.h netlib/commlib.h
	g++ -c kvm.cpp -o kvm.o $(COMM_CFLAGS)

frontend/%.o: frontend/%.cpp frontend/%.h
	g++ -c -o $@ $< $(COMM_CFLAGS)

backend/%.o: backend/%.cpp backend/%.h
	g++ -c -o $@ $< $(COMM_CFLAGS)

netlib/%.o: netlib/%.cpp netlib/%.h
	g++ -c -o $@ $< $(COMM_CFLAGS)

debug-server/messages.o: debug-server/messages.c debug-server/messages.h
	gcc -c debug-server/messages.c -o debug-server/messages.o

debug-server/%.o: debug-server/%.cpp debug-server/%.h 
	g++ -c -o $@ $< $(COMM_CFLAGS)

elf/%.o: elf/%.cpp $(ELF_HEADER_FILES)
	g++ -c -o $@ $< $(COMM_CFLAGS)

gdbserver/%.o: gdbserver/%.cpp $(GDBSERVER_HEADER_FILES)
	g++ -c -o $@ $< $(COMM_CFLAGS)

bootloader/Bootloader.o: bootloader/Bootloader.cpp bootloader/Bootloader.h
	g++ -c bootloader/Bootloader.cpp -o bootloader/Bootloader.o $(COMM_CFLAGS)

debug-client/debug_client.o: debug-client/debug_client.cpp debug-server/net_wrapper.h 
	g++ -c debug-client/debug_client.cpp -o debug-client/debug_client.o $(COMM_CFLAGS)

build/boot64: bootloader/boot64.S
	g++ -m32 -nostdlib -fno-exceptions -g -fno-rtti -fno-stack-protector -mno-red-zone -gdwarf-2 -fpic -m32 -Ttext=0x2000 -no-pie bootloader/boot64.S -o build/boot64 -Wl,-fuse-ld=gold
clean:
	rm -f *.o frontend/*.o backend/*.o elf/*.o debug-server/*.o debug-client/*.o netlib/*.o
	rm -f kvm debug-client/debug_client
	rm -f build/*
	rm -f bootloader/*.o
