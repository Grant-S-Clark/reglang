e exe:
	g++ *.cpp
a asan:
	g++ -fsanitize=address *.cpp
r run:
	./a.out
q quick:
	g++ *.cpp
	./a.out
c clean:
	rm a.out
