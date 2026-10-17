#!/usr/bin/bash
if [ "$EUID" -ne 0 ]
  then echo "Please run as root"
  exit
fi
# stop if anything fails
set -e
# create service user
useradd -r -U LIOD

# create directory for storing the daemon and scripts
mkdir -p /opt/LIOD/bin
mkdir /opt/LIOD/scripts

# own the directory
chown -R LIOD:LIOD /opt/LIOD

echo "Copying files to /opt\n"
cp ../bin/LIODd /opt/LIOD/bin/
cp ./stress_server.sh /opt/LIOD/scripts/
cp ./check_api.sh /opt/LIOD/scripts/

# make sure the files are also owned
chown -R LIOD:LIOD /opt/LIOD

echo "Copying services to /etc/systemd/system\n"
cp ./stress_server.service /etc/systemd/system/
cp ./LIODd.service /etc/systemd/system/
echo "Done.\n"