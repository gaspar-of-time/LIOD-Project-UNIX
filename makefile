all: ./bin/LIODClient ./bin/LIODd

./bin/LIODClient: ./include/cpp-tui/cpptui.hpp ./include/cpp-httplib/httplib.h ./src/clientMain.cpp ./bin
	g++ -I ./include/cpp-tui/ -I ./include/cpp-httplib/ ./src/clientMain.cpp -o ./bin/LIODClient

./bin:
	mkdir ./bin

./bin/LIODd: ./include/cpp-sys-stats/include/linux-system-usage.hpp ./include/cpp-httplib/httplib.h ./src/serverMain.cpp ./bin
	g++ -I ./include/cpp-sys-stats/include -I ./include/cpp-httplib/ ./src/serverMain.cpp -o ./bin/LIODd
