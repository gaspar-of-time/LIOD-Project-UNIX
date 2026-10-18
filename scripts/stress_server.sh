#!/usr/bin/bash
if [ ! -d "/tmp/stress-ng" ]; then
#make it if it doesn't
  mkdir "/tmp/stress-ng"
fi

stress-ng --cpu-load 70 --vm 5 --vm-bytes 10% --hdd 5 --hdd-bytes 10% --temp-path /tmp/stress-ng
