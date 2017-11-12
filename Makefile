COMM_CFLAGS=-std=gnu++14
LD_FLAGS=-pthread
ELFPROG_CFLAGS=-nostdlib -Ttext=0x100000 -fno-asynchronous-unwind-tables -m64 -Wl,--build-id=none

FRONTEND_CPP_FILES := $(wildcard frontend/*.cpp)
FRONTEND_OBJ_FILES = $(patsubst frontend/%.cpp,frontend/%.o,$(FRONTEND_CPP_FILES))

BACKEND_CPP_FILES := $(wildcard backend/*.cpp)
BACKEND_OBJ_FILES = $(patsubst backend/%.cpp,backend/%.o,$(BACKEND_CPP_FILES))

ELF_OBJ_FILES = elf/elf32.o elf/elf64.o elf/estrattore.o elf/interp.o
ELF_HEADER_FILES := $(wildcard elf/*.h)


## -- linking

all: kvm build/caric build/prog_prova build/keyboard_program

kvm: kvm.o boot.o $(FRONTEND_OBJ_FILES) $(BACKEND_OBJ_FILES) $(ELF_OBJ_FILES)
	g++ kvm.o boot.o $(FRONTEND_OBJ_FILES) $(BACKEND_OBJ_FILES) $(ELF_OBJ_FILES) -o kvm $(LD_FLAGS)

build/caric: $(ELF_OBJ_FILES) elf/main.o
	g++ $(ELF_OBJ_FILES) elf/main.o -o build/caric $(LD_FLAGS)

build/prog_prova: target/prog_prova.c target/prog_prova.s
	gcc $(ELFPROG_CFLAGS) target/prog_prova.c target/prog_prova.s -o build/prog_prova

build/keyboard_program: target/keyboard_program.s
	gcc $(ELFPROG_CFLAGS) target/keyboard_program.s -o build/keyboard_program

## -- compilazione

kvm.o: kvm.cpp
	g++ -c kvm.cpp -o kvm.o $(COMM_CFLAGS)

boot.o: boot.cpp boot.h
	g++ -c boot.cpp -o boot.o $(COMM_CFLAGS)

frontend/%.o: frontend/%.cpp frontend/%.h
	g++ -c -o $@ $< $(COMM_CFLAGS)

backend/%.o: backend/%.cpp backend/%.h
	g++ -c -o $@ $< $(COMM_CFLAGS)

elf/%.o: elf/%.cpp $(ELF_HEADER_FILES)
	g++ -c -o $@ $< $(COMM_CFLAGS)

clean:
	rm -f *.o frontend/*.o backend/*.o elf/*.o
	rm -f kvm
	rm -f build/*
