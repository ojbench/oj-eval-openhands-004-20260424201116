code: main.cpp
	g++ -std=c++14 -O2 -o code main.cpp

clean:
	rm -f code *.dat

.PHONY: clean
