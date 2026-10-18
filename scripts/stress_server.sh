#!/usr/bin/bash
stress-ng --cpu 2 --cpu-load 70% --vm 2 --vm-bytes 70% --dccp 1 --hdd 1 --hdd-bytes 70%
