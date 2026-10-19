all: ./bin/LIODClient ./bin/LIODd

./bin/LIODClient: ./cpp-tui/cpptui.hpp ./cpp-httplib/httplib.h ./src/clientMain.cpp
	g++ -I ./cpp-tui/ -I ./cpp-httplib/ ./src/clientMain.cpp -o ./bin/LIODClient

./bin/LIODd: ./cpp-sys-stats/include/linux-system-usage.hpp ./cpp-httplib/httplib.h ./src/serverMain.cpp
	g++ -I ./cpp-sys-stats/include -I ./cpp-httplib/ ./src/serverMain.cpp -o ./bin/LIODd
