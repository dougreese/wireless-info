/*
    Simple utility to pull wireless name from Linux kernel using ioctl
    
    Copyright (C) 2014 Doug Reese

    Used as a learning experience in obtaining wireless info from 
    the Linux kernel using ioctl.
    
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
    USA
*/

#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/wireless.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

int main(int argc, char **argv)
{
    int ret;
    struct ifreq req;
    struct sockaddr_in *addr;
    int s;

    if (argc != 2) {
        fprintf(stderr, "Need an interface name (like wlan0)\n");
        return 1;
    }

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        perror("Cannot open socket");
        return 1;
    }
    strncpy(req.ifr_name, argv[1], sizeof(req.ifr_name));
    ret = ioctl(s, SIOCGIWNAME, &req);
    if (ret < 0) {
        fprintf(stderr, "No wireless extension\n");
        return 1;
    }

    printf("%s\n", req.ifr_name);
    printf("%s\n", req.ifr_newname);
    return 0;
}
