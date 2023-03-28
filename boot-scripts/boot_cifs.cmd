setenv myip        192.168.0.14
setenv serverip    192.168.0.4
setenv netmask     255.255.255.0
setenv gatewayip   192.168.0.4
setenv hostname    myhost
setenv mountpath   rootfs
setenv bootargs    console=ttyS0,115200 earlyprintk rootdelay=1 root=/dev/cifs rw cifsroot=//$serverip/$mountpath,username=root,password=toor,port=1445 ip=$myip:$serverip:$gatewayip:$netmask:$hostname::off

fatload mmc 0 $kernel_addr_r Image
fatload mmc 0 $fdt_addr_r nanopi-neo-plus2.dtb

booti $kernel_addr_r - $fdt_addr_r
