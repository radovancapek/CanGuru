# SocketCAN CanGuru Lite driver

## Install module

Add file can.conf to /usr/lib/modules-load.d
~~~
can
can-dev
~~~
or execute commands
~~~
modprobe can
modprobe can-dev
~~~

~~~
insmod canguru_drv.ko
~~~

## Init network

~~~
ip link set can1 type can bitrate 500000 dbitrate 2000000 termination 120 fd on
ip link set up can1
~~~

~~~
ip link set down can1
~~~

~~~
ip -details -statistics link show can1
~~~


## Debug
Kernel config change
~~~
CONFIG_GDB_SCRIPTS=y
CONFIG_FRAME_POINTER=y

CONFIG_STRICT_KERNEL_RWX=n

CONFIG_KGDB_SERIAL_CONSOLE=y
CONFIG_KGDB=y
CONFIG_KGDB_KDB=y
CONFIG_KDB_DEFAULT_ENABLE=0x1
CONFIG_CONSOLE_POLL=y
~~~

### VM
Add kgdboc options to /etc/default/grub
~~~
GRUB_CMDLINE_LINUX_DEFAULT="loglevel=3 quiet nokaslr kgdboc=ttyS0,115200 kgdbwait"
~~~
Update grub
~~~
grub-mkconfig -o /boot/grub/grub.cfg
~~~

### VirtualBox

Enable COM1
* Mode: Host pipe
* Disable connect to exist pipe
* Path: /tmp/vm-serial-socket

### GDB
socat -d -d -d -d /tmp/vm-serial-socket PTY,link=/tmp/vm-serial-pty
