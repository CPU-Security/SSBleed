LKM=mdu_kernel
make clean
make
sudo mknod /dev/${LKM} c 223 0
sudo chmod 666 /dev/${LKM}
sudo insmod ${LKM}.ko