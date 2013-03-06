all: cfg.cpp dce.cpp pre.cpp dce.h main.cpp
	g++ -g -c cfg.cpp 
	g++ -g -c dce.cpp
	g++ -g -c pre.cpp
	g++ -g -c main.cpp 
	g++ -g cfg.o dce.o pre.o main.o -o dce
clean:
	rm -rf *.o dce dce.dSYM/ 
