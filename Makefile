CPPFLAGS=-Wall -pedantic -std=c++0x
CC=g++
FILES= UDPPinger.cpp main.cpp

all:
	$(CC) $(CPPFLAGS) -o udppinger $(FILES)

clean:
	rm -f udppinger