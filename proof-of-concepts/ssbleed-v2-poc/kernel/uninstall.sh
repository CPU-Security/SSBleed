LKM=mdu_kernel
sudo rmmod ${LKM}.ko || true
if [ -e "/dev/${LKM}" ]; then sudo rm /dev/${LKM}; fi