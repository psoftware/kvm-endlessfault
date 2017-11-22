COMM_CFLAGS=-std=gnu++14 -g -no-pie
LD_FLAGS=-pthread
ELFPROG_CFLAGS=-nostdlib -Ttext=0x100000 -fno-asynchronous-unwind-tables -m64 -Wl,--build-id=none -no-pie -Wl,-fuse-ld=gold

FRONTEND_CPP_FILES := $(wildcard frontend/*.cpp)
FRONTEND_OBJ_FILES = $(patsubst frontend/%.cpp,frontend/%.o,$(FRONTEND_CPP_FILES))

BACKEND_CPP_FILES := $(wildcard backend/*.cpp)
BACKEND_OBJ_FILES = $(patsubst backend/%.cpp,backend/%.o,$(BACKEND_CPP_FILES))

ELF_OBJ_FILES = elf/elf32.o elf/elf64.o elf/estrattore.o elf/interp.o
ELF_HEADER_FILES := $(wildcard elf/*.h)


## -- linking

all: kvm build/boot64 build/prog_prova build/keyboard_program build/ricorsivo

kvm: kvm.o bootloader/Bootloader.o bootloader/bootloader_code.o $(FRONTEND_OBJ_FILES) $(BACKEND_OBJ_FILES) $(ELF_OBJ_FILES)
	g++ kvm.o bootloader/Bootloader.o bootloader/bootloader_code.o $(FRONTEND_OBJ_FILES) $(BACKEND_OBJ_FILES) $(ELF_OBJ_FILES) -o kvm $(LD_FLAGS)

build/prog_prova: target/prog_prova.c target/prog_prova.s
	gcc $(ELFPROG_CFLAGS) target/prog_prova.c target/prog_prova.s -o build/prog_prova

build/keyboard_program: target/keyboard_program.s
	gcc $(ELFPROG_CFLAGS) target/keyboard_program.s -o build/keyboard_program

build/ricorsivo: target/ricorsivo.c target/prog_prova.s
	gcc $(ELFPROG_CFLAGS) target/ricorsivo.c target/prog_prova.s -o build/ricorsivo
## --compilazione

kvm.o: kvm.cpp
	g++ -c kvm.cpp -o kvm.o $(COMM_CFLAGS)

#boot.o: boot.cpp boot.h
#	g++ -c boot.cpp -o boot.o $(COMM_CFLAGS)

frontend/%.o: frontend/%.cpp frontend/%.h
	g++ -c -o $@ $< $(COMM_CFLAGS)

backend/%.o: backend/%.cpp backend/%.h
	g++ -c -o $@ $< $(COMM_CFLAGS)

elf/%.o: elf/%.cpp $(ELF_HEADER_FILES)
	g++ -c -o $@ $< $(COMM_CFLAGS)

bootloader/Bootloader.o: bootloader/Bootloader.cpp bootloader/Bootloader.h
	g++ -c bootloader/Bootloader.cpp -o bootloader/Bootloader.o $(COMM_CFLAGS)

bootloader/bootloader_code.o: bootloader/bootloader_code.cpp
	g++ -c bootloader/bootloader_code.cpp -o bootloader/bootloader_code.o $(COMM_CFLAGS)

build/boot64: bootloader/boot64.s
	g++ -m32 -nostdlib -fno-exceptions -g -fno-rtti -fno-stack-protector -mno-red-zone -gdwarf-2 -fpic -m32 -Ttext=0 bootloader/boot64.s -o build/boot64 -Wl,-fuse-ld=gold
clean:
	rm -f *.o frontend/*.o backend/*.o elf/*.o
	rm -f kvm
	rm -f build/*
	rm -f bootloader/*.o
