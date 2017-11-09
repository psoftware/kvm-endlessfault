COMM_CFLAGS=-std=gnu++14
LD_FLAGS=-pthread
ELFPROG_CFLAGS=-nostdlib -Ttext=0xfffff000 -fno-asynchronous-unwind-tables -m32

FRONTEND_CPP_FILES := $(wildcard frontend/*.cpp)
FRONTEND_OBJ_FILES = $(patsubst frontend/%.cpp,frontend/%.o,$(FRONTEND_CPP_FILES))

BACKEND_CPP_FILES := $(wildcard backend/*.cpp)
BACKEND_OBJ_FILES = $(patsubst backend/%.cpp,backend/%.o,$(BACKEND_CPP_FILES))

ELF_OBJ_FILES = elf/elf32.o elf/elf64.o elf/estrattore.o elf/interp.o


## -- linking

all: kvm caric prog_prova

kvm: kvm.o $(FRONTEND_OBJ_FILES) $(BACKEND_OBJ_FILES) $(ELF_OBJ_FILES)
	g++ kvm.o $(FRONTEND_OBJ_FILES) $(BACKEND_OBJ_FILES) $(ELF_OBJ_FILES) -o kvm $(LD_FLAGS)

caric: $(ELF_OBJ_FILES) elf/main.o
	g++ $(ELF_OBJ_FILES) elf/main.o -o caric $(LD_FLAGS)

prog_prova: elf/prog_prova.c elf/prog_prova.s
	gcc $(ELFPROG_CFLAGS) elf/prog_prova.c elf/prog_prova.s -o prog_prova

## -- compilazione

kvm.o: kvm.cpp
	g++ -c kvm.cpp -o kvm.o $(COMM_CFLAGS)

frontend/%.o: frontend/%.cpp frontend/%.h
	g++ -c -o $@ $< $(COMM_CFLAGS)

backend/%.o: backend/%.cpp backend/%.h
	g++ -c -o $@ $< $(COMM_CFLAGS)

elf/%.o: elf/%.cpp elf/%.h
	g++ -c -o $@ $< $(COMM_CFLAGS)

clean:
	rm -f *.o frontend/*.o backend/*.o elf/*.o
	rm -f kvm caric prog_prova
