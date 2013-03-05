all: cfg.cpp dce.cpp pre.cpp dce.h main.cpp
	g++ -c cfg.cpp 
	g++ -c dce.cpp
	g++ -c pre.cpp
	g++ -c main.cpp 
	g++ cfg.o dce.o pre.o main.o -o dce
clean:
	rm -rf *.o dce dce.dSYM/ 
