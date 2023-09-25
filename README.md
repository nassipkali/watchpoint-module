# watchpoint-module
1. Add layer to poky image:
```
bitbake-layers add-layer /path/to/layer
```
2. Add layer to build config in poky ./build/conf/bblayers.conf
```
BBLAYERS ?= " \
   /home/daulet/Workspace/poky/meta \
   /home/daulet/Workspace/poky/meta-poky \
   /home/daulet/Workspace/poky/meta-yocto-bsp \
   /path/to/layer \
   "
```
3. After launch poky image in qemu - run module
```
cd /lib/modules/$(uname -r)/extra
modprobe addr_mod
```
4. Set sysfs params:
```
cd /sys/kernel/watchpoint
echo "0x12345678" > watch_address
echo "1" > read_watch_enabled
```
5. Check result in dmesg
```
dmesg
```
