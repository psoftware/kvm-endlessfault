FRONTEND_CPP_FILES := $(wildcard frontend/*.cpp)
FRONTEND_OBJ_FILES = $(patsubst frontend/%.cpp,frontend/%.o,$(FRONTEND_CPP_FILES))

BACKEND_CPP_FILES := $(wildcard backend/*.cpp)
BACKEND_OBJ_FILES = $(patsubst backend/%.cpp,backend/%.o,$(BACKEND_CPP_FILES))


## -- linking

all: kvm

kvm: kvm.o $(FRONTEND_OBJ_FILES) $(BACKEND_OBJ_FILES)
	g++ kvm.o $(FRONTEND_OBJ_FILES) $(BACKEND_OBJ_FILES) -o kvm

## -- compilazione

kvm.o: kvm.cpp
	g++ -c kvm.cpp -o kvm.o

frontend/%.o: frontend/%.cpp frontend/%.h
	g++ -c -o $@ $<

backend/%.o: backend/%.cpp backend/%.h
	g++ -c -o $@ $<

clean:
	rm -f *.o frontend/*.o
	rm kvm
