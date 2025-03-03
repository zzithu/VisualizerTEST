


all:
	g++ -o main.exe main.cpp -Isrc/include -Lsrc/lib -lSDL3




clean:
	rm main.exe
	rm main.o 