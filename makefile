CPP_FILES := $(wildcard *.cpp)
OBJ_FILES := $(addprefix ,$(notdir $(CPP_FILES:.cpp=.o)))

all: server removeObj

server: $(OBJ_FILES)
	g++ -o $@ $^

%.o: %.cpp
	g++ -c -o $@ $<

removeObj:
	rm *.o
