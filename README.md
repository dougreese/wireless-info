wireless-info
=============

Small utility to obtain wireless info from Linux kernel using ioctl

Based on utilies in [Wireless Tools for Linux](http://www.hpl.hp.com/personal/Jean_Tourrilhes/Linux/Tools.html) (iwconfig, etc.).  Useful sample code snippets also found on Stack Overflow.  If you see code you originally posted on SO, feel free to contact for attribution.  Used as a learning experience in obtaining wireless info from Linux kernel using ioctl.  Yes, using ioctl is deprecated in favor of Netlink now, but some systems do not have full Netlink support as configured (with all required kernel modules); notably embedded Linux systems.

Optionally, monitor for changes in wireless status using small subset of Netlink messages.  This requires libnetlink be installed, and one of the easiest ways to get that is to install iproute-dev, at least on Debian based distros.  

Building is easy without a Makefile:

```
gcc -o wireless-info wireless-info.c /usr/lib/libnetlink.a
gcc -o wname wname.c
```

