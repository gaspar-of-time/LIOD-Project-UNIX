#!/usr/bin/bash
# checks if test request goes through to API, attempts restarting if HTTP error is recieved or API hanging
# warning: unfinished!

# for following vars, change to match machine or if otherwise needed
SERVICE="serverMain.service"
URL="http://localhost:7777/" # endpoint to check
FIFO="/tmp/obligitoryFifo"

#stop script if anything outright fails
set -e

# grab timestap for log entries
ts() {
  date '+%f %T';
}
if [-ne "$FIFO"]; then
#make it if it doesn't
  mkfifo $FIFO
fi






# API is given 5 seconds to respond before it assumes API service is hanging
if curl -fsS --max-time 5 "$URL" > /dev/null 2>&1; then
  echo "$(ts) API RESPONDED TO TEST" >> "$FIFO"
else
  echo "$(ts) API FAILED TO RESPOND TO TEST, ATTEMPTING SERVICE RESTART" >> "$FIFO"
  systemctl restart "$SERVICE"
  sleep 5 # chose an arbitrary sleep time here. increase if API takes longer to restart fully
  if curl -fsS --max-time 5 "$URL" > /dev/null 2>&1; then
    echo "$(ts) API RESPONDED TO TEST, API RESTARTED SUCCESSFULLY" >> "$FIFO"
  else
    echo "$(ts) API FAILED TO RESPOND TO TEST, API FAILED TO RESTART CORRECTLY" >> "$FIFO"
  fi
fi
