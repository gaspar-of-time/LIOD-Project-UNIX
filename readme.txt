General Plan

use c++ and cpp-tui (since we need networking I feel c++ will be the best option, not using ncurses since it doesn't have existing graph tools)

can't use netcat on a local dashboard since it needs to use an agent/api
need unique user for agent/api

last 10 minutes of data on graph
update every 5 seconds? if so then use queue or array or vector of length 120
monitor cpu, memory, disk usage, disk traffic, network traffic
notify when user adjustable threshold is passed

use tcp for networking using sys/socket
see https://codezup.com/c-networking-guide/
