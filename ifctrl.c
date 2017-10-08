
#include <stdio.h> 
#include <sys/types.h>      
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "ifctrl.h"


int set_ifname(struct ifreq* ifr, char* ifname){
	size_t if_name_len = strlen(ifname);
	if (if_name_len > sizeof(ifr->ifr_name)){
		printf("Interface name is too long!\n");
		return -1;
	}
	memcpy(ifr->ifr_name, ifname, if_name_len);
	return 0;
}

int ifctrl_get_ifindex(char* ifname){
	struct ifreq ifr;


	int sock;
	if (( sock = socket( AF_INET, SOCK_DGRAM, 0 ) ) < 0 ){
		printf("socket error\n");
		return -1;
	}


	if (set_ifname(&ifr, ifname) != 0) return -1;

	if (ioctl(sock,SIOCGIFINDEX,&ifr)==-1) {
	    printf("%s",strerror(errno));
		return -1;
	}

	return ifr.ifr_ifindex;
}


int ifctrl_get_ifflags(char* ifname, short* flags){
	struct ifreq ifr;
	if (set_ifname(&ifr, ifname) != 0) return -1;

	int sock;
	if (( sock = socket( AF_INET, SOCK_DGRAM, 0 ) ) < 0 ){
		printf("socket error\n");
		return -1;
	}


	if (ioctl(sock,SIOCGIFFLAGS,&ifr)==-1) {
	    printf("%s",strerror(errno));
		return -1;
	}

	*flags = ifr.ifr_flags;	
	return 0;
}

int ifctrl_set_ifflags(char* ifname, short flags){
	struct ifreq ifr;
	if (set_ifname(&ifr, ifname) != 0) return -1;

	ifr.ifr_flags = flags;
	int sock;
	if (( sock = socket( AF_INET, SOCK_DGRAM, 0 ) ) < 0 ){
		printf("socket error\n");
		return -1;
	}


	if (ioctl(sock,SIOCSIFFLAGS,&ifr)==-1) {
	    printf("%s",strerror(errno));
		return -1;
	}
	return 0;
}


int ifctrl_is_up(char* ifname){
	short flags;
	if (ifctrl_get_ifflags(ifname, &flags) != 0){
		return -1;
	}

	return flags & IFF_UP;
} 

int ifctrl_get_wireless_mode(char* ifname){
	struct iwreq ifr;
	size_t if_name_len = strlen(ifname);

	if (if_name_len > sizeof(ifr.ifr_name)){
		printf("Interface name is too long!\n");
		return -1;
	}
	memcpy(ifr.ifr_name, ifname, if_name_len);

	int sock;
	if (( sock = socket( AF_INET, SOCK_DGRAM, 0 ) ) < 0 ){
		printf("socket error\n");
		return -1;
	}


	if (ioctl(sock,SIOCGIWMODE,&ifr)==-1) {
	    printf("%s\n",strerror(errno));
		return -1;
	}
	
	return ifr.u.mode;
}


int ifctrl_startstop(char* ifname, int on){
	struct ifreq ifr;
	if (set_ifname(&ifr, ifname) != 0) return -1;
	
	short flags;
	if (ifctrl_get_ifflags(ifname, &flags) != 0) return -1;

	if(on){
		flags = flags | IFF_UP;
		printf("Starting device...\n");
	} else {
		flags = flags & !IFF_UP;
		printf("Stopping device...\n");
	}

	if (ifctrl_set_ifflags(ifname, flags) != 0) return -1;
	if (on) printf("Device started.\n"); else printf("Device stopped.\n");

	return 0;
} 


int ifctrl_set_wireless_mode(char* ifname, int mode){
	struct iwreq ifr;
	size_t if_name_len = strlen(ifname);

	int sock;
	if (( sock = socket( AF_INET, SOCK_DGRAM, 0 ) ) < 0 ){
		printf("socket error\n");
		return -1;
	}

	if (ifctrl_startstop(ifname, 0) == -1) return -1;

	if (if_name_len > sizeof(ifr.ifr_name)){
		printf("Interface name is too long!\n");
		return -1;
	}
	memcpy(ifr.ifr_name, ifname, if_name_len);


	printf("Changing wireless mode...\n");
	ifr.u.mode = mode;

	if (ioctl(sock,SIOCSIWMODE,&ifr)==-1) {
	    printf("%s\n",strerror(errno));
		return -1;
	}
	
	if (ifctrl_startstop(ifname, 1) == -1) return -1;


	printf("Wireless mode changed.\n");
	return 0;
}