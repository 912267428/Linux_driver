#!/bin/bash

arm-linux-gnueabihf-gcc $1App.c -o $1App
chmod 777 $1App
sudo cp $1.ko $1App /home/rodney/linux/nfs/rootfs/lib/modules/4.1.15

#arm-linux-gnueabihf-gcc noblockioApp.c -o noblockioApp
#chmod 777 blockioApp
#sudo cp blockio.ko  /home/rodney/linux/nfs/rootfs/lib/modules/4.1.15