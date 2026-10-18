#!/usr/bin/bash
if [ ! -d "/tmp/stress-ng" ]; then
#make it if it doesn't
  mkdir "/tmp/stress-ng"
fi

stress-ng --cpu 2 --cpu-load 70 --vm 2 --vm-bytes 70% --hdd 1 --hdd-bytes 70% --temp-path /tmp/stress-ng
