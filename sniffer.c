#include <stdio.h> 
#include <sys/types.h>      
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/if_packet.h>
#include <endian.h>
#include <stdbool.h>
#include <stdlib.h>
#include <linux/wireless.h>

struct ieee80211_radiotap_header {
        u_int8_t        it_version;     /* set to 0 */
        u_int8_t        it_pad;
        u_int16_t       it_len;         /* entire length */
        u_int32_t       it_present;     /* fields present */
} __attribute__((__packed__));


struct probe_request {
		char* SSID;
		char DA[6];
		char* vendor_ie;  
};

#define IEEE80211_STYPE_PROBE_REQ	0x0040
#define IEEE80211_FTYPE_MGMT		0x0000
#define IEEE80211_FCTL_FTYPE		0x000c
#define IEEE80211_FCTL_STYPE		0x00f0
static inline bool ieee80211_is_probe_req(__le16 fc)
{
	return (fc & htole16(IEEE80211_FCTL_FTYPE | IEEE80211_FCTL_STYPE)) ==
	       htole16(IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_PROBE_REQ);
}

char* bin2hex(unsigned char* bin, size_t size, char* delimiter){
	int i;
	int delim_len = 0;

	if (delimiter){
		delim_len = strlen(delimiter);
	} 
	
	char* hex = malloc(size * (2+delim_len) - delim_len + 1);


	if (!hex) return NULL;

	if (!delimiter){
		for (i = 0; i < size; i++){
			sprintf(hex + (2*i),"%.2X",bin[i]);
		}
	} else {
		for (i = 0; i < size -1; i++){
			sprintf(hex + ((2+delim_len)*i),"%.2X%s",bin[i],delimiter);
		}
		sprintf(hex + ((2+delim_len)*(size-1)),"%.2X",bin[size-1]);
	}
	return hex;
}


int set_ifname(struct ifreq* ifr, char* ifname){
	size_t if_name_len = strlen(ifname);
	if (if_name_len > sizeof(ifr->ifr_name)){
		printf("Interface name is too long!\n");
		return -1;
	}
	memcpy(ifr->ifr_name, ifname, if_name_len);
	return 0;
}

int ifindex_from_name(char* ifname, int socket){
	struct ifreq ifr;
	
	if (set_ifname(&ifr, ifname) != 0) return -1;

	if (ioctl(socket,SIOCGIFINDEX,&ifr)==-1) {
	    printf("%s",strerror(errno));
		return -1;
	}

	return ifr.ifr_ifindex;
}


int interface_get_flags(char* ifname, int socket, short* flags){
	struct ifreq ifr;
	if (set_ifname(&ifr, ifname) != 0) return -1;

	if (ioctl(socket,SIOCGIFFLAGS,&ifr)==-1) {
	    printf("%s",strerror(errno));
		return -1;
	}

	*flags = ifr.ifr_flags;	
	return 0;
}

int interface_set_flags(char* ifname, int socket, short flags){
	struct ifreq ifr;
	if (set_ifname(&ifr, ifname) != 0) return -1;

	ifr.ifr_flags = flags;

	if (ioctl(socket,SIOCSIFFLAGS,&ifr)==-1) {
	    printf("%s",strerror(errno));
		return -1;
	}
	return 0;
}


int interface_is_up(char* ifname, int socket){
	short flags;
	if (interface_get_flags(ifname, socket, &flags) != 0){
		return -1;
	}

	return flags & IFF_UP;
} 

int get_wireless_mode(char* ifname, int socket){
	struct iwreq ifr;
	size_t if_name_len = strlen(ifname);

	if (if_name_len > sizeof(ifr.ifr_name)){
		printf("Interface name is too long!\n");
		return -1;
	}
	memcpy(ifr.ifr_name, ifname, if_name_len);

	if (ioctl(socket,SIOCGIWMODE,&ifr)==-1) {
	    printf("%s\n",strerror(errno));
		return -1;
	}
	
	return ifr.u.mode;
}

int shutdown_if(char* ifname, int socket){
	struct ifreq ifr;
	if (set_ifname(&ifr, ifname) != 0) return -1;
	
	printf("Shutting down device...\n");
	short flags;
	if (interface_get_flags(ifname, socket, &flags) != 0) return -1;

	flags = flags & !IFF_UP;

	if (interface_set_flags(ifname, socket, flags) != 0) return -1;

	return 0;
}

int start_if(char* ifname, int socket){
	struct ifreq ifr;
	if (set_ifname(&ifr, ifname) != 0) return -1;
	
	printf("Starting device...\n");
	short flags;
	if (interface_get_flags(ifname, socket, &flags) != 0) return -1;

	flags = flags | IFF_UP;

	if (interface_set_flags(ifname, socket, flags) != 0) return -1;
	printf("Device started\n"); 
	return 0;
} 


int set_wireless_mode(char* ifname, int socket, int mode){
	struct iwreq ifr;
	size_t if_name_len = strlen(ifname);

	if (shutdown_if(ifname, socket) == -1) return -1;

	if (if_name_len > sizeof(ifr.ifr_name)){
		printf("Interface name is too long!\n");
		return -1;
	}
	memcpy(ifr.ifr_name, ifname, if_name_len);


	printf("Changing wireless mode...\n");
	ifr.u.mode = mode;

	if (ioctl(socket,SIOCSIWMODE,&ifr)==-1) {
	    printf("%s\n",strerror(errno));
		return -1;
	}
	
	if (start_if(ifname, socket) == -1) return -1;


	printf("Wireless mode changed.\n");
	return 0;
}


void parse_IE(unsigned char* buffer, size_t size){
		// parse Information Elements
		int pos = 0;
		while(pos < (size - 1)){
			u_int8_t element_id = buffer[pos];
			u_int8_t ie_len = buffer[pos+1];

			if (pos + ie_len + 1 >= size){
				printf("Malformed Information Elements\n");
				return;
			}

			u_int8_t vendor_id[3];
			switch(element_id){
				case 0: //SSID
					if (ie_len == 0){
						printf("\tSSID: Broadcast\n");
					} else {
						char* ssid = malloc(ie_len+1);
						if (!ssid) return;
						memcpy(ssid, buffer + pos + 2, ie_len);
						ssid[ie_len] = '\0';

						printf("\tSSID: %s\n", ssid);
					}
					break;
				case 221: // Vendor Specific
					
					memcpy(vendor_id, buffer + pos + 2, 3);
					u_int8_t* vendor_data = malloc(ie_len - 3);
					if(!vendor_data) return;
					
					memcpy(vendor_data, buffer + pos + 2 + 3, ie_len -3);
					char* vendor_hex = bin2hex(vendor_id, 3, ":");
					char* vendor_data_hex = bin2hex(vendor_data, ie_len -3, ":");

					printf("\tVendor Specific:\n");
					printf("\t\tVendor ID: %s\n", vendor_hex);
					printf("\t\tVendor Data: %s\n", vendor_data_hex);
					

					free(vendor_data);
					free(vendor_hex);
					free(vendor_data_hex);
					break;
				default:
					break;
			}					
				



			pos += ie_len + 2;
		}
}

void parse_80211(unsigned char* buffer, size_t size){

		u_int16_t frame_control = *(buffer);

		if (ieee80211_is_probe_req(frame_control)){
			printf("Probe Request:\n");

			char* raw = bin2hex(buffer, size, ":");
			printf("\n%s\n\n", raw);
			free(raw);


			u_int8_t sa[6];
			u_int8_t da[6];
			u_int8_t bssid[6];
			memcpy(da,buffer + 4, 6);
			memcpy(sa,buffer + 10, 6);
			memcpy(bssid,buffer + 16, 6);

			char* da_hex = bin2hex(da,6,":");
			char* sa_hex = bin2hex(sa,6,":");
			char* bssid_hex = bin2hex(bssid,6,":");

			printf("\tDA: %s\n",da_hex);
			printf("\tSA: %s\n",sa_hex);
			printf("\tBSSID: %s\n",bssid_hex);
			
			free(da_hex);
			free(sa_hex);

			// skip mac header
			// ignore FCS for IE parsing
			parse_IE(buffer + 24, size - 4 - 24);
		}

}


int main(int argc, char**argv){
	printf("Probe Request Sniffer started...\n");

	int sock = socket(AF_PACKET,SOCK_RAW, htons(ETH_P_ALL));
	if (sock == -1){
		printf("Socket creation failed! Are you root?\n");
		return -1;
	}

	char* ifname = "wlp7s0";


	if (interface_is_up(ifname,sock) != 1){
		printf("Interface %s is not running.\n", ifname);
		return -1;
	}

	int mode = get_wireless_mode(ifname,sock);
	if (mode == -1){
		return -1;
	}
	if (mode != IW_MODE_MONITOR){
		printf("Device must be in monitor mode!\n");
		if (set_wireless_mode(ifname, sock, IW_MODE_MONITOR) == -1){
			return -1;	
		} 
	}


	int ifindex = ifindex_from_name(ifname, sock);

	if (ifindex == -1){
		return -1;
	}


	struct sockaddr_ll sll;
	sll.sll_family = AF_PACKET;
	sll.sll_ifindex = ifindex;
	sll.sll_protocol = htons(ETH_P_ALL);

	if ((bind(sock, (struct sockaddr *)&sll, sizeof(sll))) == -1){
		perror("bind:");
		return -1;
	}

	unsigned char buffer[2048];
	while (1){
		int pos = 0;
		int size = recvfrom(sock,buffer, 2048, 0, NULL, NULL);
		
		if (size == -1){
			return -1;
		}

		if (size < sizeof(struct ieee80211_radiotap_header)){
			printf("Unable to parse frame!\n");
			continue;
		}

		//TODO Check if data has radiotap header

		struct ieee80211_radiotap_header* rheader;
		rheader = (struct ieee80211_radiotap_header*) buffer;
		
		// skip radiotap header
		pos += rheader->it_len;

		parse_80211(buffer + pos, size - pos);

	}



	return 0;
}