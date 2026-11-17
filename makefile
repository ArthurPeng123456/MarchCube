CXX = g++
CXXFLAGS = -Wall -Wextra -O2 -g -std=c++11 -I include

march_objects = build/marching_cubes.o build/generator.o build/utilities.o build/main_march.o

all: main_march

main_march: $(march_objects)
	$(CXX) $(CXXFLAGS) $(march_objects) -o $@

build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build:
	mkdir -p build

build/utilities.o: include/utilities.hpp include/types.hpp
build/main_march.o: include/generator.hpp include/marching_cubes.hpp include/utilities.hpp
build/marching_cubes.o: include/marching_cubes.hpp include/types.hpp
build/generator.o: include/generator.hpp include/types.hpp

.PHONY: all clean

clean:
	rm -f build/*.o main_march main_march.exe