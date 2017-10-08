#include <linux/wireless.h>
#include <linux/if_ether.h>

int ifctrl_get_ifindex(char* ifname);
int ifctrl_get_ifflags(char* ifname, short *flags);
int ifctrl_set_ifflags(char* ifname, short flags);
int ifctrl_is_up(char* ifname);
int ifctrl_get_wireless_mode(char* ifname);
int ifctrl_set_wireless_mode(char* ifname, int mode);
int ifctrl_startstop(char* ifname, int on);
