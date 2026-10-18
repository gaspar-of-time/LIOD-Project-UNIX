#!/usr/bin/bash
LOG="/var/log/log_api.log" # file may need to exist prior to runtime, unsure
FIFO="/tmp/obligitoryFifo"
# check if the log exists
if [-ne "$LOG"]; then
#make it if it doesn't
  touch $LOG
fi

#forever read the line of the fifo
while true
do
    if read line <$FIFO; then
        if [[ "$line" != '' ]]; then
        echo $line >> "$LOG"
        exit
        fi
    fi
done