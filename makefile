all: ./bin/LIODClient ./bin/LIODd

./bin/LIODClient: ./cpp-tui/cpptui.hpp ./cpp-httplib/httplib.h ./src/clientMain.cpp ./bin
	g++ -I ./cpp-tui/ -I ./cpp-httplib/ ./src/clientMain.cpp -o ./bin/LIODClient

./bin:
	mkdir ./bin

./bin/LIODd: ./cpp-sys-stats/include/linux-system-usage.hpp ./cpp-httplib/httplib.h ./src/serverMain.cpp ./bin
	g++ -I ./cpp-sys-stats/include -I ./cpp-httplib/ ./src/serverMain.cpp -o ./bin/LIODd
