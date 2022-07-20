game: main.cpp cellmap.h cellmap.cpp
	g++ main.cpp cellmap.cpp -o game -I include -L lib -l SDL2-2.0.0 -std=c++17 ${CCFLAGS} -O3 -DCIN

test: test.cpp cellmap.h cellmap.cpp
	g++ test.cpp cellmap.cpp -o test -I include -L lib -lgtest -std=c++17 ${CCFLAGS}
