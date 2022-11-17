philosopher:	philosopher.cpp	philosopher.h
	g++	-std=c++11	-pthread	-o	philosopher	philosopher.cpp philosopher.h
clean:
	rm	philosopher

