#include <stdio.h>
#include <stdlib.h>
#include <libnet.h>
#include <pcap.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>


#ifdef __linux__
	#define th_dport dest
	#define th_sport source
	#define th_seq seq
#endif


#define couleur(param) printf("\033[%sm",param)


static FILE *fic = NULL;


void usage(void) {
	couleur("31");
	printf("Michel Dubois -- network -- (c) 2013\n\n");
	couleur("0");
	printf("Syntaxe: network <interface> <ip> <port> <num>\n");
	printf("\t<interface> -> name of the network interface (like eth0)\n");
	printf("\t<ip> -> dotted ip address of the destination\n");
	printf("\t<port> -> tcp port destination\n");
	printf("\t<num> -> sample size\n");
}


int isNotRoot(void) {
	int result = 0;
	if( (getuid() != 0) || (geteuid() != 0) ) {
		result = 1;
	} else {
		result = 0;
	}
	return(result);
}


void printPacket(u_char *arg, const struct pcap_pkthdr *header, const u_char *packet) {
	struct ip *iph = (struct ip *)(packet + LIBNET_ETH_H);
	struct tcphdr *tcph = (struct tcphdr *)(packet + LIBNET_ETH_H + LIBNET_IPV4_H);
	int *counter = (int *)arg;

	printf("### (%d) TCP ACK %s:%d -> %s:%d (%d bytes)\t", ++(*counter), inet_ntoa(iph->ip_src), ntohs(tcph->th_sport), inet_ntoa(iph->ip_dst), ntohs(tcph->th_dport), header->len);
	fprintf(fic, "%u\n", ntohl(tcph->th_seq));
	printf("tcp seq num = %u\n", ntohl(tcph->th_seq));
}


libnet_t *create_libnet_handle(libnet_t *l, char *interface) {
	char errbuf[LIBNET_ERRBUF_SIZE];

	l = libnet_init(LIBNET_RAW4, interface, errbuf);
	if ( l == NULL ) {
		fprintf(stderr, "libnet_init() failed: %s\n", errbuf);
		exit(EXIT_FAILURE);
	}
	return l;
}


pcap_t *create_pcap_handle(pcap_t *p, char *interface, char *dest, char *port) {
	char errbuf[PCAP_ERRBUF_SIZE];
	bpf_u_int32 net, mask;
	struct bpf_program fp; //compiled filter
//	char *filter = "(tcp[13] == 0x14) || (tcp[13] == 0x12)"; //we capture only TCP packets with ACK and RST flags
	char *filter = malloc(128 * sizeof(char));
	sprintf(filter, "(tcp[13] == 0x12) and host %s and port %s", dest, port);

	p = pcap_open_live(interface, BUFSIZ, 0, 512, errbuf);
	if ( p == NULL ) {
		fprintf(stderr, "pcap_open_live() failed: %s\n", errbuf);
		exit(EXIT_FAILURE);
	}
	if (pcap_datalink(p) != DLT_EN10MB) {
		fprintf(stderr, "Device %s doesn't provide Ethernet headers - not supported\n", interface);
		exit(EXIT_FAILURE);
	}
	if (pcap_setnonblock(p, 1, errbuf) == -1) {
		fprintf (stderr, "Error setting nonblocking: %s\n", errbuf);
		exit(EXIT_FAILURE);
	}
	if (pcap_lookupnet(interface, &net, &mask, errbuf) == -1) {
		fprintf (stderr, "Net lookup error: %s\n", errbuf);
		exit(EXIT_FAILURE);
	}
	if (pcap_compile (p, &fp, filter, 0, mask) == -1) {
		fprintf (stderr, "BPF error: %s\n", pcap_geterr(p));
		exit(EXIT_FAILURE);
	}
	if (pcap_setfilter (p, &fp) == -1) {
		fprintf (stderr, "Error setting BPF: %s\n", pcap_geterr(p));
		exit(EXIT_FAILURE);
	}
	pcap_freecode(&fp);
	return p;
}


void hostping(libnet_t *l, char *dest, int nbr) {
	u_int32_t ip_dest, ip_src;
	u_int16_t seq = 1, id;
	libnet_ptag_t icmp = LIBNET_PTAG_INITIALIZER;
	char payload[] = "Hello World";
	int bytes = 0, i = 0;

	libnet_seed_prand(l);
	id = (u_int16_t)libnet_get_prand(LIBNET_PR16);

	ip_src = libnet_get_ipaddr4(l);
	ip_dest = libnet_name2addr4(l, dest, LIBNET_DONT_RESOLVE);

	printf("### ping %s -> %s\n", libnet_addr2name4(ip_src, LIBNET_DONT_RESOLVE), libnet_addr2name4(ip_dest, LIBNET_DONT_RESOLVE));
	icmp = libnet_build_icmpv4_echo(ICMP_ECHO, 0, 0, id, seq, (u_int8_t*)payload, sizeof(payload), l, 0);
	libnet_autobuild_ipv4(LIBNET_IPV4_H + LIBNET_ICMPV4_ECHO_H + sizeof(payload), IPPROTO_ICMP, ip_dest, l);

	for (i=0; i<nbr; i++) {
		icmp = libnet_build_icmpv4_echo(ICMP_ECHO, 0, 0, id, seq+i, (u_int8_t*)payload, sizeof(payload), l, icmp);
		bytes = libnet_write(l);
		printf("### ping: %d bytes written\n", bytes);
		sleep(1);
	}
}


libnet_ptag_t initiate_tcp(libnet_t *l, char *dest, char *port) {
	libnet_ptag_t tcp = LIBNET_PTAG_INITIALIZER;

	tcp = libnet_build_tcp(
		libnet_get_prand(LIBNET_PRu16),		// Source port
		atoi(port),							// Destination port
		libnet_get_prand(LIBNET_PRu32),		// Sequence number
		0,									// ACK number
		TH_SYN,								// Control flags
		libnet_get_prand(LIBNET_PRu16),		// Windows size
		0,									// Checksum (0 for libnet to autofill) 
		0,									// Urgent pointer
		LIBNET_TCP_H,						// Total length of the TCP packet
		NULL,								// Payload
		0,									// Payload size
		l,									// Libnet handle
		0);									// Protocol tag to modify an existing header, 0 to build a new one

	libnet_build_ipv4(
		LIBNET_TCP_H + LIBNET_IPV4_H,		// length
		0,									// TOS
		libnet_get_prand(LIBNET_PRu16),		// IP ID
		0,									// IP Frag
		64,									// TTL
		IPPROTO_TCP,						// Protocol
		0,									// Checksum
		libnet_get_ipaddr4(l),				// Source IP
		libnet_name2addr4(l, dest, LIBNET_DONT_RESOLVE), // Destination IP
		NULL,								// Payload
		0,									// Payload size
		l,									// Libnet handle
		0);									// Protocol tag to modify an existing header, 0 to build a new one 
	return tcp;	
}


libnet_ptag_t tcp_syn(libnet_t *l, libnet_ptag_t tcp, char *dport, u_int32_t sport) {
	tcp = libnet_build_tcp(
		sport,
		atoi(dport),
		libnet_get_prand(LIBNET_PRu32),
		0,
		TH_SYN,
		libnet_get_prand(LIBNET_PRu16),
		0,
		0,
		LIBNET_TCP_H,
		NULL,
		0,
		l,
		tcp);
	return tcp;	
}


libnet_ptag_t tcp_rst(libnet_t *l, libnet_ptag_t tcp, char *dport, u_int32_t sport) {
	tcp = libnet_build_tcp(
		sport,
		atoi(dport),
		libnet_get_prand(LIBNET_PRu32),
		0,
		TH_RST,
		libnet_get_prand(LIBNET_PRu16),
		0,
		0,
		LIBNET_TCP_H,
		NULL,
		0,
		l,
		tcp);
	return tcp;	
}


void send_sniff(libnet_t *l, pcap_t *p, char *dest, char *dport, int nbr) {
	libnet_ptag_t tcp;
	int bytes = 0, i = 0, count = 0;
	u_int32_t sport = 0;

	tcp = initiate_tcp(l, dest, dport);

	for (i=0; i<nbr; i++) {
		sport = libnet_get_prand(LIBNET_PRu16);
		tcp = tcp_syn(l, tcp, dport, sport);
		bytes = libnet_write(l);
		printf("### TCP SYN sent %s:%u -> %s:%s (%d bytes)\n", libnet_addr2name4(libnet_get_ipaddr4(l), LIBNET_DONT_RESOLVE), sport, dest, dport, bytes);
		pcap_loop(p, 1, printPacket, (u_char *)&count);
//		The RST packet is send by the libnet_destroy function
		tcp = tcp_rst(l, tcp, dport, sport);
		bytes = libnet_write(l);
		printf("### TCP RST sent %s:%u -> %s:%s (%d bytes)\n", libnet_addr2name4(libnet_get_ipaddr4(l), LIBNET_DONT_RESOLVE), sport, dest, dport, bytes);
		printf("\n");
		usleep(10000); // 0.01 second
	}

}


int main(int argc, char *argv[]) {
	libnet_t *l = NULL;
	pcap_t *p = NULL;

	if (isNotRoot()) {
		usage();
		couleur("31");
		printf("\nINFO: you need to be root\n");
		couleur("0");
		exit(EXIT_FAILURE);
	}

	switch (argc) {
		case 3:
			l = create_libnet_handle(l, argv[1]);
			hostping(l, argv[2], 5);
			libnet_destroy(l);
			exit(EXIT_SUCCESS);
			break;
		case 5:
			fic = fopen("result.dat", "w");
			if (fic != NULL) {
				printf("INFO: file create\n");
				l = create_libnet_handle(l, argv[1]);
				p = create_pcap_handle(p, argv[1], argv[2], argv[3]);
				send_sniff(l, p, argv[2], argv[3], atoi(argv[4]));
				libnet_destroy(l);
				pcap_close(p);
				fclose(fic);
				printf("INFO: file close\nINFO: data saved in result.dat\n");
				exit(EXIT_SUCCESS);
			} else {
				printf("INFO: open error\n");
				exit(EXIT_FAILURE);
			}
			break;
		default:
			usage();
			exit(EXIT_FAILURE);
			break;	
		}
}


