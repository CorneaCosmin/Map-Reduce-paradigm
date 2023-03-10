all: build

build: tema1.cpp
	g++ -Wall tema1.cpp -o tema1 -lm -ggdb3 -lpthread

run: tema1
	./tema1 $(arg1) $(arg2) $(arg3)

clean:
	rm tema1
