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

echo "Copying files to /opt/LIOD/\n"
cp ../bin/LIODd /opt/LIOD/bin/
cp ./stress_server.sh /opt/LIOD/scripts/
cp ./check_api.sh /opt/LIOD/scripts/
cp ./stress_server.service /opt/LIOD/scripts/
cp ./LIODd.service /opt/LIOD/scripts/
echo "Done.\n"
# make sure the files are also owned
chown -R LIOD:LIOD /opt/LIOD

echo "Enabling and starting systemd services\n"
systemctl enable --now /opt/LIOD/scripts/LIODd.service
systemctl enable --now /opt/LIOD/scripts/stress_server.service
echo "Done.\n"
