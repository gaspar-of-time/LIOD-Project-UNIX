#!/usr/bin/bash
if [ ! -d "/tmp/stress-ng" ]; then
#make it if it doesn't
  mkdir "/tmp/stress-ng"
fi

stress-ng --vm 1 --vm-bytes 50% --hdd 1 --hdd-bytes 50% --temp-path /tmp/stress-ng
