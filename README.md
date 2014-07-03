wireless-info
=============

Small utility to obtain wireless info from Linux kernel using ioctl

Based on untilies in Wireless Tools for Linux (iwconfig, etc.).  Useful sample code snippets also found on Stack Overflow.  If you see code you originally posted on SO, feel free to contact for attribution.  Used as a learning experience in obtaining wireless info from Linux kernel using ioctl.  Yes, using ioctl is deprecated in favor of Netlink now, but some systems do not support Netlink as configured; notably embedded Linux systems.

gcc -o wireless-info wireless-info.c
gcc -o wname wname.c



