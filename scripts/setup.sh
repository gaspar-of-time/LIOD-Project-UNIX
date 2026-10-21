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

apt -y install stress-ng cron

# own the directory
chown -R LIOD:LIOD /opt/LIOD

echo "Copying files to /opt/LIOD/"
cp ../bin/LIODd /opt/LIOD/bin/
cp ./stress_server.sh /opt/LIOD/scripts/
cp ./check_api.sh /opt/LIOD/scripts/
cp ./log_api.sh /opt/LIOD/scripts/
cp ./stress_server.service /opt/LIOD/scripts/
cp ./LIODd.service /opt/LIOD/scripts/
chmod +x --recursive /opt/LIOD/
echo "Done.\n"
# make sure the files are also owned
chown -R LIOD:LIOD /opt/LIOD

echo "Enabling and starting systemd services"
systemctl enable --now /opt/LIOD/scripts/LIODd.service
systemctl enable --now /opt/LIOD/scripts/stress_server.service
echo "Done.\n"

echo "Adding cron job to root crontab"
echo "*/5 * * * * /opt/LIOD/scripts/check_api.sh & /opt/LIOD/scripts/log_api.sh" >> "/etc/crontab"
echo "Done.\n"

#Navarro Swap Script
# Auto configure swap at startup to use primary disk space
# Add to VM Advanced startup script
# Set swap file size in MB
SWAP_SIZE=2048

# Check if swap file already exists
if [[ ! -f /swapfile ]]; then
  #  Preallocates disk space for a file of a given size - Create swap file
  fallocate -l ${SWAP_SIZE}M /swapfile
  if [[ $? -ne 0 ]]; then
    echo "Error: Failed to create swap file."
    exit 1
  fi

  # Set correct permissions
  chmod 600 /swapfile

  # Format as swap
  mkswap /swapfile
  if [[ $? -ne 0 ]]; then
    echo "Error: Failed to format swap file."
    exit 1
  fi
fi

# Enable swap
swapon /swapfile
if [[ $? -ne 0 ]]; then
  echo "Error: Failed to enable swap."
  exit 1
fi

# Add swap to fstab for persistent mounting
echo "/swapfile swap swap defaults 0 0" | sudo tee -a /etc/fstab

# Verify swap is active
free -h

echo "Swap file setup complete." # write to logs