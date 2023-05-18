# /bin/bash

ifup eth0
mount /dev/mmcblk0p1 /mnt
rm /mnt/soc_system.rbf
rm /mnt/soc_system.dtb
scp jjl2247@micro24.ee.columbia.edu:Downloads/embedded_lab3/lab3-hw/output_files/soc_system.rbf /mnt
scp jjl2247@micro24.ee.columbia.edu:Downloads/embedded_lab3/lab3-hw/soc_system.dtb /mnt
sync
