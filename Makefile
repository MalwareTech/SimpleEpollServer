CPP_FILES := $(wildcard src/*.cpp)
OBJ_FILES := $(addprefix obj/,$(notdir $(CPP_FILES:.cpp=.o)))
CXX_FLAGS := -g -Wall -O3 -std=c++11
LD_FLAGS  :=

server: $(OBJ_FILES)
	g++ $(LD_FLAGS) -o $@ $^

obj/%.o: src/%.cpp
	g++ $(CXX_FLAGS) -c -o $@ $<

#
clean: 
	$(RM) bin/server obj/*.o
