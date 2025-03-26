


all:
	g++ -o visualizer.exe visualizer.cpp -Isrc/include -Lsrc/lib -lSDL3

debug:
	g++ -o visualizer.exe visualizer.cpp -Isrc/include -Lsrc/lib -lSDL3 -DDEBUG



clean:
	rm visualizer.exe
	rm visualizer.o 
	