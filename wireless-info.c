/*
    Simple utility to pull wireless stats from Linux kernel using
    ioctl, similar to iwconfig

    Copyright (C) 2014 Doug Reese

    Used as a learning experience in obtaining wireless info from 
    the Linux kernel using ioctl. 

    Major insight and functionality gleaned from Jean Tourrilhes' 
    Wireless Tools for Linux
    http://www.hpl.hp.com/personal/Jean_Tourrilhes/Linux/Tools.html

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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <linux/wireless.h>

/* Some usefull constants */
#define KILO	1e3
#define MEGA	1e6
#define GIGA	1e9

/* For doing log10/exp10 without libm */
#define LOG10_MAGIC	1.25892541179

/* no libm */
#define WE_NOLIBM

/*
 * Returns a socket to use for ioctl calls
 */ 
int get_socket() 
{
  int sock = -1;
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    return 0;
  }
 
  return sock;
}

/*
 * Compare two ethernet addresses 
 * (from wireless tools)
 */
static inline int
iw_ether_cmp(const struct ether_addr* eth1, const struct ether_addr* eth2)
{
  return memcmp(eth1, eth2, sizeof(*eth1));
}

/*
 * Convert a value in milliWatt to a value in dBm.
 * (from wireless tools)
 */
int iw_mwatt2dbm(int	in)
{
#ifdef WE_NOLIBM
  /* Version without libm : slower */
  double	fin = (double) in;
  int		  res = 0;

  /* Split integral and floating part to avoid accumulating rounding errors */
  while(fin > 10.0) {
    res += 10;
    fin /= 10.0;
  }

  /* Eliminate rounding errors, take ceil */
  while(fin > 1.000001)	{
    res += 1;
    fin /= LOG10_MAGIC;
  }
  return(res);
#else	/* WE_NOLIBM */
  /* Version with libm : faster */
  return((int) (ceil(10.0 * log10((double) in))));
#endif	/* WE_NOLIBM */
}

/*
 * Display an Ethernet address in readable format.
 * (from wireless tools)
 */
void iw_ether_ntop(const struct ether_addr *eth, char *buf)
{
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
	  eth->ether_addr_octet[0], eth->ether_addr_octet[1],
	  eth->ether_addr_octet[2], eth->ether_addr_octet[3],
	  eth->ether_addr_octet[4], eth->ether_addr_octet[5]);
}

/*
 * Display an Wireless Access Point Socket Address in readable format.
 * Note : 0x44 is an accident of history, that's what the Orinoco/PrismII
 * chipset report, and the driver doesn't filter it.
 * (from wireless tools)
 */ 
char * iw_sawap_ntop(const struct sockaddr *sap, char *buf)
{
  const struct ether_addr ether_zero = {{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }};
  const struct ether_addr ether_bcast = {{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }};
  const struct ether_addr ether_hack = {{ 0x44, 0x44, 0x44, 0x44, 0x44, 0x44 }};
  const struct ether_addr * ether_wap = (const struct ether_addr *) sap->sa_data;

  if(!iw_ether_cmp(ether_wap, &ether_zero))
    sprintf(buf, "Not-Associated");
  else
    if(!iw_ether_cmp(ether_wap, &ether_bcast))
      sprintf(buf, "Invalid");
    else
      if(!iw_ether_cmp(ether_wap, &ether_hack))
	sprintf(buf, "None");
      else
	iw_ether_ntop(ether_wap, buf);
  return(buf);
}

/*
 * Output a bitrate with proper scaling
 * (from wireless tools)
 */
void iw_print_bitrate(char *buffer, int	buflen, int bitrate)
{
  double	rate = bitrate;
  char		scale;
  int		  divisor;

  if(rate >= GIGA) {
    scale = 'G';
    divisor = GIGA;
  } else {
    if(rate >= MEGA) {
      scale = 'M';
      divisor = MEGA;
    } else {
      scale = 'k';
      divisor = KILO;
    }
  }
  snprintf(buffer, buflen, "%g %cb/s", rate / divisor, scale);
}

/*
 * Output a txpower with proper conversion
 * (from wireless tools)
 */
void iw_print_txpower(char *buffer, int	buflen, struct iw_param *txpower)
{
  int		dbm;

  /* Check if disabled */
  if(txpower->disabled) {
    snprintf(buffer, buflen, "off");
  } else {
    /* Check for relative values */
    if(txpower->flags & IW_TXPOW_RELATIVE) {
      snprintf(buffer, buflen, "%d", txpower->value);
    } else {
      /* Convert everything to dBm */
      if(txpower->flags & IW_TXPOW_MWATT)
        dbm = iw_mwatt2dbm(txpower->value);
      else
        dbm = txpower->value;

      /* Display */
      snprintf(buffer, buflen, "%d dBm", dbm);
    }
  }
}
 
/*
 * Checks to see if interface is wireless
 */ 
int check_wireless(const char* ifname, char* protocol) 
{
  int sock = get_socket();
  struct iwreq pwrq;
  memset(&pwrq, 0, sizeof(pwrq));
  strncpy(pwrq.ifr_name, ifname, IFNAMSIZ);
 
  if (ioctl(sock, SIOCGIWNAME, &pwrq) != -1) {
    if (protocol) strncpy(protocol, pwrq.u.name, IFNAMSIZ);
    close(sock);
    return 1;
  }
 
  close(sock);
  return 0;
}

/*
 * Retrieves/prints wireless interface ESSID
 */
int wireless_essid(const char* ifname) 
{
  int sock = get_socket();
  struct iwreq wrq;
  char nickname[IW_ESSID_MAX_SIZE + 2]; 
  char *essid = NULL;

  memset(&wrq, 0, sizeof(wrq));
  strncpy(wrq.ifr_name, ifname, IFNAMSIZ);

  wrq.u.essid.pointer = nickname;
  wrq.u.essid.length  = IW_ESSID_MAX_SIZE + 2;
  wrq.u.essid.flags   = 0;

  if (ioctl(sock, SIOCGIWESSID, &wrq) < 0) {
    perror("Could not get ESSID");
    close(sock);
    return;
  }
  close(sock);

  essid = (char *)malloc(wrq.u.essid.length + 1);
  memset(essid, 0, wrq.u.essid.length + 1);
  strncpy(essid, wrq.u.essid.pointer, wrq.u.essid.length);
  
  printf("ESSID: %s\n", essid);
  free(essid);
} 
 
/*
 * Retrieves/prints wireless interface access point
 */
int wireless_ap(const char* ifname) 
{
  int sock = get_socket();
  struct iwreq wrq;
  struct sockaddr addr;

  memset(&wrq, 0, sizeof(wrq));
  memset(&addr, 0, sizeof(addr));
  strncpy(wrq.ifr_name, ifname, IFNAMSIZ);

  if (ioctl(sock, SIOCGIWAP, &wrq) < 0) {
    perror("Could not get bitrate");
    close(sock);
    return;
  }
  close(sock);

  char buffer[256];
  printf("Access Point: %s\n", iw_sawap_ntop(&wrq.u.ap_addr, buffer));
} 

/*
 * Retrieves/prints wireless interface bitrate
 */
int wireless_bitrate(const char* ifname) 
{
  int sock = get_socket();
  struct iwreq wrq;
  struct sockaddr addr;

  memset(&wrq, 0, sizeof(wrq));
  memset(&addr, 0, sizeof(addr));
  strncpy(wrq.ifr_name, ifname, IFNAMSIZ);

  if (ioctl(sock, SIOCGIWRATE, &wrq) < 0) {
    perror("Could not get access point");
    close(sock);
    return;
  }
  close(sock);

  char buffer[256];
  iw_print_bitrate(buffer, sizeof(buffer), wrq.u.bitrate.value);
  printf("Bit Rate: %s\n", buffer);
} 

/*
 * Retrieves/prints wireless interface transmit power
 */
int wireless_txpower(const char* ifname) 
{
  int sock = get_socket();
  struct iwreq wrq;
  struct sockaddr addr;

  memset(&wrq, 0, sizeof(wrq));
  memset(&addr, 0, sizeof(addr));
  strncpy(wrq.ifr_name, ifname, IFNAMSIZ);

  if (ioctl(sock, SIOCGIWTXPOW, &wrq) < 0) {
    perror("Could not get transmit power");
    close(sock);
    return;
  }
  close(sock);

  char buffer[256];
  iw_print_txpower(buffer, sizeof(buffer), &wrq.u.txpower);
  printf("Transmit Power: %s\n", buffer);
} 

/*
 * Retrieves/prints wireless interface stats
 */
int wireless_stats(const char* ifname) 
{
  int sock = get_socket();
  struct iwreq wrq;
  struct iw_statistics stats;
  
  memset(&wrq, 0, sizeof(wrq));
  memset(&stats, 0, sizeof(stats));
  strncpy(wrq.ifr_name, ifname, IFNAMSIZ);

  wrq.u.data.pointer = &stats;
  wrq.u.data.length  = sizeof(struct iw_statistics);
  wrq.u.data.flags   = 1;

  if (ioctl(sock, SIOCGIWSTATS, &wrq) < 0) {
    perror("Could not get stats");
    close(sock);
    return 0;
  }
  close(sock);

  printf("Status: %x\n", stats.status);

  if (!(stats.qual.updated & IW_QUAL_QUAL_INVALID)) {
    printf("Quality: %d\n", stats.qual.qual);
  } else {
    printf("Quality not reported\n");
  }

  /*
   * TODO: test for different types of stats (RCPI vs. dBm)
   * (see wireless tools iwlib.c iw_print_stats(...)
   */

  /* signal level */
  if (!(stats.qual.updated & IW_QUAL_LEVEL_INVALID)) {
    int dblevel = stats.qual.level;
    dblevel -= 0x100;
    printf("Signal Level: %d dBm\n", dblevel);
  } else {
    printf("Signal Level not reported\n");
  }

  /* noise level */
  if (!(stats.qual.updated & IW_QUAL_NOISE_INVALID)) {
    int dblevel = stats.qual.noise;
    dblevel -= 0x100;
    printf("Noise Level: %d dBm\n", dblevel);
  } else {
    printf("Noise Level not reported\n");
  }
 
  /* discarded stats */
  printf("Rx invalid nwid: %d\n", stats.discard.nwid);
  printf("Rx invalid crypt: %d\n", stats.discard.code);
  printf("Rx invalid frag: %d\n", stats.discard.fragment);
  printf("Tx excessive retries: %d\n", stats.discard.retries);
  printf("Invalid misc: %d\n", stats.discard.misc);
  printf("Missed beacon: %d\n", stats.miss.beacon);

 
  printf("Updated: %x\n", stats.qual.updated);

 
  return 1; 
} 

/*
 * Retrieves/prints wireless interface ranges
 */
int wireless_range(const char *ifname) 
{
  int sock = get_socket();
  struct iwreq wrq;
  struct iw_range range;
  
  memset(&wrq, 0, sizeof(wrq));
  memset(&range, 0, sizeof(range));
  strncpy(wrq.ifr_name, ifname, IFNAMSIZ);

  wrq.u.data.pointer = &range;
  wrq.u.data.length  = sizeof(struct iw_range);
  wrq.u.data.flags   = 1;

  if (ioctl(sock, SIOCGIWRANGE, &wrq) < 0) {
    perror("Could not get range");
    return 0;
    close(sock);
  }
  close(sock);
 
  /* quality */
  printf("Max Quality: %d\n", range.max_qual.qual);

  /* see wireless tools wireless.22.h ~line 1022 */
  printf("Avg Quality: %d\n", range.avg_qual.qual);
 
  /* max signal level */
  if (!(range.max_qual.updated & IW_QUAL_LEVEL_INVALID)) {
    int dblevel = range.max_qual.level;
    dblevel -= 0x100;
    printf("Max Signal Level: %d dBm\n", dblevel);
  } else {
    printf("Max Signal Level not reported\n");
  }

  /* max noise level */
  if (!(range.max_qual.updated & IW_QUAL_NOISE_INVALID)) {
    int dblevel = range.max_qual.noise;
    dblevel -= 0x100;
    printf("Max Noise Level: %d dBm\n", dblevel);
  } else {
    printf("Max Noise Level not reported\n");
  }
 
  return 1;
}

/*
 * Main application
 */ 
int main(int argc, char const *argv[]) 
{
  struct ifaddrs *ifaddr, *ifa;
 
  if (getifaddrs(&ifaddr) == -1) {
    perror("getifaddrs");
    return -1;
  }
 
  /* Walk through linked list, maintaining head pointer so we
     can free list later */
  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    char protocol[IFNAMSIZ] = {0};
 
    if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_PACKET) 
      continue;
 
    if (check_wireless(ifa->ifa_name, protocol)) {
      printf("Interface %s is wireless: %s\n", ifa->ifa_name, protocol);

      wireless_essid(ifa->ifa_name);
      wireless_ap(ifa->ifa_name);
      wireless_bitrate(ifa->ifa_name);
      wireless_txpower(ifa->ifa_name);
      printf("--------\n");

      wireless_stats(ifa->ifa_name);
      printf("--------\n");

      wireless_range(ifa->ifa_name);
    } else {
      printf("interface %s is not wireless\n", ifa->ifa_name);
    }
    printf("========\n");

  }
 
  freeifaddrs(ifaddr);
  return 0;
}

/* vim: set expandtab tabstop=2 shiftwidth=2 softtabstop=2 autoindent smartindent: */
