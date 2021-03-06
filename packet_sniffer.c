#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/if.h>
#include <netinet/if_ether.h>
#include <net/ethernet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#define ALL 0
#define HTTPMODE 1
#define DNSMODE 2
#define ICMPMODE 3
#define HTTPSMODE 4

#define ICMP 1
#define TCP 6
#define UDP 17

#define DNS 53
#define HTTP 80
#define HTTPS 443

#define BUF 65536 
#define ERR 1

#define MYETH "eth0"

void process_packet(unsigned char*, int);
void print_ethernet_header(unsigned char*, int);
void print_ip_header(unsigned char*, int);
void print_tcp_packet(unsigned char*, int);
void print_udp_packet(unsigned char*, int);
void print_icmp_packet(unsigned char*, int);
void print_data(unsigned char*, int);

int kbhit();
void print_cur(void);
void print_form(void);
void filter();
void init_ip();

void add_node();
void print_nodes(int, char*, char*, char*);
void free_nodes();

typedef struct _node{
    int data_size;
    unsigned char buffer[BUF]; 
    struct _node *next;
}node;

node* head;

struct sockaddr_in source, dest;
int raw_socket;
int tcp = 0, udp = 0, icmp = 0, total = 0;

FILE *log_file;

int main(){   
    head = (node*)malloc(sizeof(node));
    head->next = NULL;

    int saddr_size, data_size;
    struct sockaddr saddr;
    struct in_addr in;
    struct ifreq ethreq;
    unsigned char* buffer = (unsigned char*)malloc(BUF);

    log_file = fopen("log.txt", "w");
    if(!log_file){
        printf("ERR - Failed to create file.\n");
        return ERR;
    }

    raw_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if(raw_socket < 0){
        printf("ERR - Failed to create raw socket.\n");
        return ERR;
    }
    
    strncpy(ethreq.ifr_name, MYETH, IFNAMSIZ - 1);
    if(ioctl(raw_socket, SIOCGIFFLAGS, &ethreq) == -1){
        printf("ERR - Failed to do ioctl01.\n");
        close(raw_socket);
        return ERR;
    }

    ethreq.ifr_flags |= IFF_PROMISC;
    if(ioctl(raw_socket, SIOCSIFFLAGS, &ethreq) == -1){
        printf("ERR - Failed to do ioctl02.\n");
        close(raw_socket);
        return ERR;
    }

    printf("         KPU Computer Network Project(Team 8)\n");
    printf("            press return key to filter\n\n");
  
    while(1){
        print_cur();
        if(kbhit())
            break;
        
        saddr_size = sizeof(saddr);
        data_size = recvfrom(raw_socket, buffer, BUF, 0, &saddr, (socklen_t*)&saddr_size);
        if(data_size < 0){
            printf("ERR - Failed to get packets.\n");
            return ERR;
        }
        process_packet(buffer, data_size);
    }
    fclose(log_file);
    filter();

    free_nodes();

    ethreq.ifr_flags ^= IFF_PROMISC;
    ioctl(raw_socket, SIOCSIFFLAGS, &ethreq);
    
    close(raw_socket);
    
    printf("\n\n         Thank you for using this program\n");
 
    return 0;

}

void add_node(unsigned char* buffer, int size){
    node* new_node = (node*)malloc(sizeof(node));
    new_node->next = NULL;
    new_node->data_size = size;
    memcpy(new_node->buffer, buffer, sizeof(unsigned char) * size);
    // connect
    if(head->next){
        node* cur = head->next;
        while(cur->next){
            cur = cur->next;
        }
        cur->next = new_node;
    }
    else{
        head->next = new_node;
    }
}

void print_nodes(int option, char* ip_either, char* source_ip, char* dest_ip){
    log_file = fopen("filter.txt", "w");
    if(!log_file)
        return;
    
    node* cur = head->next;

    while(cur){
        struct iphdr* iph = (struct iphdr*)(cur->buffer + sizeof(struct ethhdr));
        if(ip_either){
            if(strcmp(inet_ntoa(*(struct in_addr *)&iph->saddr), ip_either) && strcmp(inet_ntoa(*(struct in_addr *)&iph->daddr), ip_either)){
                cur = cur->next;
                continue;
            }
        }
        else{
            if(source_ip){
                if(dest_ip){
                    if(strcmp(inet_ntoa(*(struct in_addr *)&iph->saddr), source_ip) || strcmp(inet_ntoa(*(struct in_addr *)&iph->daddr), dest_ip)){
                        cur = cur->next;
                        continue;
                    }
                }
                else{
                    if(strcmp(inet_ntoa(*(struct in_addr *)&iph->saddr), source_ip)){
                        cur = cur->next;
                        continue;
                    }
                }
            }
            else{
                if(dest_ip){
                    if(strcmp(inet_ntoa(*(struct in_addr *)&iph->daddr), dest_ip)){
                        cur = cur->next;
                        continue;
                    }
                }
            }
        }

        if(option == ICMPMODE || option == ALL){
            if(iph->protocol == ICMP)
                print_icmp_packet(cur->buffer, cur->data_size);
        }
        if(option == ALL || option == HTTPMODE || option == HTTPSMODE){
            if(iph->protocol == TCP){
                struct tcphdr* tcph = (struct tcphdr*)(cur->buffer + (iph->ihl * 4) + sizeof(struct ethhdr));
                if(option == HTTPMODE && !(ntohs(tcph->dest) == HTTP || ntohs(tcph->source) == HTTP)){
                    cur = cur->next;
                    continue;
                }
                if(option == HTTPSMODE && !(ntohs(tcph->dest) == HTTPS || ntohs(tcph->source) == HTTPS)){
                    cur = cur->next;
                    continue;
                }
                
                print_tcp_packet(cur->buffer, cur->data_size);
            }
        }
        if(option == DNSMODE || option == ALL){
            if(iph->protocol == UDP){
                struct udphdr* udph = (struct udphdr*)(cur->buffer + (iph->ihl * 4) + sizeof(struct ethhdr));
                if(option == DNSMODE && !(ntohs(udph->dest) == DNS || ntohs(udph->source) == DNS)){
                    cur = cur->next;
                    continue;
                }                    
                
                print_udp_packet(cur->buffer, cur->data_size);
            }
        }
        cur = cur->next;
    }
    fclose(log_file);
}

void free_nodes(){
    node* temp;
    node* cur = head->next;

    while(cur){
        temp = cur;
        cur = cur->next;
    }
    free(head);
}

void process_packet(unsigned char* buffer, int size){
    struct iphdr* iph = (struct iphdr*)(buffer + sizeof(struct ethhdr));
    
    total++; // Total number of packets captured unconditionally
   
    if(iph->protocol == ICMP){
        print_icmp_packet(buffer, size);
        add_node(buffer, size);
        icmp++;
    }
    else if(iph->protocol == TCP){
        print_tcp_packet(buffer, size);
        tcp++;
        add_node(buffer, size);
    }
    else if(iph->protocol == UDP){ 
        print_udp_packet(buffer, size);
        udp++;
        add_node(buffer, size);
    }
}

void print_ethernet_header(unsigned char* buffer, int size){
	struct ethhdr* eth = (struct ethhdr*)buffer;

	fprintf(log_file, "\nEthernet Header\n");
	fprintf(log_file, "   |-Destination Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X \n", eth->h_dest[0] , eth->h_dest[1] , eth->h_dest[2] , eth->h_dest[3] , eth->h_dest[4] , eth->h_dest[5] );
	fprintf(log_file, "   |-Source Address      : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X \n", eth->h_source[0] , eth->h_source[1] , eth->h_source[2] , eth->h_source[3] , eth->h_source[4] , eth->h_source[5] );
	fprintf(log_file, "   |-Protocol            : %u \n",(unsigned short)eth->h_proto);
}

void print_ip_header(unsigned char* buffer, int size){
	print_ethernet_header(buffer , size);
    
	unsigned short iphdrlen;
		
	struct iphdr* iph = (struct iphdr*)(buffer + sizeof(struct ethhdr));
	iphdrlen = iph->ihl * 4;

	memset(&source, 0, sizeof(source));
	source.sin_addr.s_addr = iph->saddr;
	
	memset(&dest, 0, sizeof(dest));
	dest.sin_addr.s_addr = iph->daddr;
	
    fprintf(log_file, "\nIP Header\n");
	fprintf(log_file, "   |-IP Version        : %d\n", (unsigned int)iph->version);
	fprintf(log_file, "   |-IP Header Length  : %d (bytes)\n", ((unsigned int)(iph->ihl)) * 4);
	fprintf(log_file, "   |-Type Of Service   : %d\n",(unsigned int)iph->tos);
	fprintf(log_file, "   |-IP Total Length   : %d  (bytes)\n", ntohs(iph->tot_len));
	fprintf(log_file, "   |-Identification    : %d\n", ntohs(iph->id));
	fprintf(log_file, "   |-TTL               : %d\n", (unsigned int)iph->ttl);
	fprintf(log_file, "   |-Protocol          : %d\n", (unsigned int)iph->protocol);
	fprintf(log_file, "   |-Checksum          : %d\n", ntohs(iph->check));
	fprintf(log_file, "   |-Source IP         : %s\n", inet_ntoa(source.sin_addr));
	fprintf(log_file, "   |-Destination IP    : %s\n", inet_ntoa(dest.sin_addr));
}

void print_tcp_packet(unsigned char* buffer, int size){
	unsigned short iphdrlen;
	
	struct iphdr* iph = (struct iphdr *)(buffer + sizeof(struct ethhdr));
	iphdrlen = iph->ihl * 4;
	
	struct tcphdr* tcph=(struct tcphdr*)(buffer + iphdrlen + sizeof(struct ethhdr));
			
	int header_size = sizeof(struct ethhdr) + iphdrlen + tcph->doff * 4;
	
	fprintf(log_file, "\n\n======================= TCP Packet =======================\n");	
		
	print_ip_header(buffer,size);
		
	fprintf(log_file, "\nTCP Header\n");
	fprintf(log_file, "   |-Source Port          : %u\n", ntohs(tcph->source));
	fprintf(log_file, "   |-Destination Port     : %u\n", ntohs(tcph->dest));
	fprintf(log_file, "   |-Sequence Number      : %u\n", ntohl(tcph->seq));
	fprintf(log_file, "   |-Acknowledge Number   : %u\n", ntohl(tcph->ack_seq));
	fprintf(log_file, "   |-Header Length        : %d (bytes)\n", (unsigned int)tcph->doff * 4);
	fprintf(log_file, "   |-Urgent Flag          : %d\n", (unsigned int)tcph->urg);
	fprintf(log_file, "   |-Acknowledgement Flag : %d\n", (unsigned int)tcph->ack);
	fprintf(log_file, "   |-Push Flag            : %d\n", (unsigned int)tcph->psh);
	fprintf(log_file, "   |-Reset Flag           : %d\n", (unsigned int)tcph->rst);
	fprintf(log_file, "   |-Synchronise Flag     : %d\n", (unsigned int)tcph->syn);
	fprintf(log_file, "   |-Finish Flag          : %d\n", (unsigned int)tcph->fin);
	fprintf(log_file, "   |-Window               : %d\n", ntohs(tcph->window));
	fprintf(log_file, "   |-Checksum             : %d\n", ntohs(tcph->check));
	fprintf(log_file, "   |-Urgent Pointer       : %d\n", tcph->urg_ptr);
	
	print_data(buffer + header_size, size - header_size);	

	fprintf(log_file, "\n===========================================================");
}

void print_udp_packet(unsigned char* buffer, int size){
	unsigned short iphdrlen;
	
	struct iphdr* iph = (struct iphdr*)(buffer + sizeof(struct ethhdr));
	iphdrlen = iph->ihl * 4;
	
	struct udphdr* udph = (struct udphdr*)(buffer + iphdrlen + sizeof(struct ethhdr));
	
	int header_size =  sizeof(struct ethhdr) + iphdrlen + sizeof udph;
	
	fprintf(log_file, "\n\n======================= UDP Packet =======================\n");
	
	print_ip_header(buffer,size);			
	
	fprintf(log_file, "\nUDP Header\n");
	fprintf(log_file, "   |-Source Port      : %d\n" , ntohs(udph->source));
	fprintf(log_file, "   |-Destination Port : %d\n" , ntohs(udph->dest));
	fprintf(log_file, "   |-UDP Length       : %d\n" , ntohs(udph->len));
	fprintf(log_file, "   |-UDP Checksum     : %d\n" , ntohs(udph->check));
	
	print_data(buffer + header_size, size - header_size);

	fprintf(log_file, "\n===========================================================");
}

void print_icmp_packet(unsigned char* buffer , int size){
	unsigned short iphdrlen;
	
	struct iphdr* iph = (struct iphdr*)(buffer + sizeof(struct ethhdr));
	iphdrlen = iph->ihl * 4;
	
	struct icmphdr *icmph = (struct icmphdr *)(buffer + iphdrlen + sizeof(struct ethhdr));
	
	int header_size =  sizeof(struct ethhdr) + iphdrlen + sizeof icmph;
	
	fprintf(log_file, "\n\n======================= ICMP Packet =======================\n");	
	
	print_ip_header(buffer , size);
			
	fprintf(log_file, "\n");
		
	fprintf(log_file, "ICMP Header\n");
	fprintf(log_file, "   |-Type : %d",(unsigned int)(icmph->type));
	if((unsigned int)(icmph->type) == 11)
		fprintf(log_file, "  (TTL Expired)\n");
	else if((unsigned int)(icmph->type) == ICMP_ECHOREPLY)
		fprintf(log_file, "  (ICMP Echo Reply)\n");
	fprintf(log_file, "   |-Code : %d\n", (unsigned int)(icmph->code));
	fprintf(log_file, "   |-Checksum : %d\n",ntohs(icmph->checksum));

	print_data(buffer + header_size, size - header_size);

	fprintf(log_file, "\n===========================================================");
}

void print_data(unsigned char* data , int size){
	int i , j;
    if(size != 0){
        fprintf(log_file, "\n------------------------- Payload --------------------------\n");
        for(i = 0; i < size; i++){
		    if(i != 0 && i % 16 == 0){
			    fprintf(log_file, "         ");
			    for(j = i - 16; j < i; j++){ //알파벳 또는 숫자일 경우 출력, 아닐경우 '.' 출력
    				if(data[j] >= 32 && data[j] <= 128)
	    				fprintf(log_file, "%c", (unsigned char)data[j]); 
		    		else 
                        fprintf(log_file, "."); 
			    }
			    fprintf(log_file, "\n");
		    } 
		
		    if(i % 16 == 0)
                fprintf(log_file, "   ");
		    fprintf(log_file, " %02X", (unsigned int)data[i]);
    
	    	if(i == size - 1){
		    	for(j = 0; j < 15 - i % 16; j++) //여백
			        fprintf(log_file, "   "); 
			    fprintf(log_file, "         ");
			
			    for(j = i - i % 16; j <= i ; j++){
				    if(data[j] >= 32 && data[j] <= 128) 
				        fprintf(log_file, "%c", (unsigned char)data[j]);
				    else 
				        fprintf(log_file, ".");
			    }
			    fprintf(log_file,  "\n" );
		    }
	    }
    }	
}

void print_cur(void){
    printf(" Total : %d  [Captured] ICMP : %d TCP : %d UDP : %d ", total, icmp, tcp, udp);
    if(total % 4 == 0)
        printf(".    \r");
    else if(total % 4 == 1)
        printf("..   \r");
    else if(total % 4 == 2)
        printf("...  \r"); 
    else if(total % 4 == 3)
        printf(".... \r");  
}

int kbhit(void){
    struct termio t;
    unsigned short flag;
    unsigned char min, time;
    char buf[10];
  
    ioctl(0, TCGETA, &t);    /* 표준입력 상태파악 */
  
    flag= t.c_lflag;         /* 값 변경 */
    min= t.c_cc[VMIN];
    time= t.c_cc[VTIME];
  
    t.c_lflag &= ~ICANON;     /* low 모드로 설정 */
    t.c_cc[VMIN] = 0;         /* read호출시 0개문자 읽어들임 */
    t.c_cc[VTIME]= 0;         /* 시간지연 없음 */
  
    ioctl(0, TCSETA, &t);     /* 상태변경 */
  
    if(read(0, buf, 9) <=  0) { /* read호출 */
        t.c_lflag = flag;      /* 원상태로 복구 */
        t.c_cc[VMIN] = min;
        t.c_cc[VTIME]= time;
        ioctl(0, TCSETA, &t);
     
       return 0;             /*키가 안눌러졌음 */
    }
    else {
        t.c_lflag = flag;     /* 원상태로 복구 */
        t.c_cc[VMIN]= min;
        t.c_cc[VTIME]= time;
        ioctl(0, TCSETA, &t);
    
        return 1;             /* 키가 눌러졌음을 알림 */
    }
}

void filter(){ 
    getchar();
    char input[50] = {0, };
    char temp[3][30] = {{0, }, {0, }, {0, }};
    char* ptr = NULL;
    char* source_ip = NULL;
    char* dest_ip = NULL;
    char* ip_either = NULL;
    
    int count = 0;
    // THe filtering form
    printf("\n+---------------------------------+\n");
    printf("| <option>                        |\n");
    printf("| <option> <ip>                   |\n");
    printf("| <option> <source ip> <dest ip>  |\n");
    printf("| <option> <source ip> all        |\n");
    printf("| <option> all <destination ip>   |\n");
    printf("+---------------------------------+\n");
    printf("press just return key to exit\n");
    printf("please enter the values >> ");
    scanf("%[^\n]", input); 
    
    // split the input
    ptr = strtok(input, " ");
    while(ptr){
        strcpy(temp[count], ptr);
        count++;
        ptr = strtok(NULL, " ");    
    }

    if(count == 0){
        return;
    }
    else if(count == 1){
        if(!strcmp(temp[0], "all")){
            print_nodes(ALL, ip_either, source_ip, dest_ip);
            filter();
        }
        else if(!strcmp(temp[0], "icmp")){
            print_nodes(ICMPMODE, ip_either, source_ip, dest_ip);
            filter();
        }
        else if(!strcmp(temp[0], "http")){
            print_nodes(HTTPMODE, ip_either, source_ip, dest_ip);
            filter();
        }
        else if(!strcmp(temp[0], "https")){
            print_nodes(HTTPSMODE, ip_either, source_ip, dest_ip);
            filter();
        }
        else if(!strcmp(temp[0], "dns")){
            print_nodes(DNSMODE, ip_either, source_ip, dest_ip);
            filter();
        }
        else{
            printf("ERR - The invalid value. The form is as follows:\n");
            filter();
        }
    }
    else if(count == 2){
        ip_either = temp[1];
        if(!strcmp(temp[0], "all")){
            print_nodes(ALL, ip_either, source_ip, dest_ip);
            filter();
        }
        else if(!strcmp(temp[0], "icmp")){
            print_nodes(ICMPMODE, ip_either, source_ip, dest_ip);
            filter();
        }
        else if(!strcmp(temp[0], "http")){
            print_nodes(HTTPMODE, ip_either, source_ip, dest_ip);
            filter();
        }
        else if(!strcmp(temp[0], "https")){
            print_nodes(HTTPSMODE, ip_either, source_ip, dest_ip);
            filter();
        }
        else if(!strcmp(temp[0], "dns")){
            print_nodes(DNSMODE, ip_either, source_ip, dest_ip);
            filter();
        }
        else{
            printf("ERR - The invalid value. The form is as follows:\n");
            filter();
        }
    }
    else if(count == 3){
        if(strcmp(temp[1], "all"))
            source_ip = temp[1];
        if(strcmp(temp[2], "all"))       
            dest_ip = temp[2];
        
        if(!strcmp(temp[0], "all")){
            print_nodes(ALL, ip_either, source_ip, dest_ip);
            filter();
        }
        else if(!strcmp(temp[0], "icmp")){
            print_nodes(ICMPMODE, ip_either, source_ip, dest_ip);
            filter();
        }
        else if(!strcmp(temp[0], "http")){
            print_nodes(HTTPMODE, ip_either, source_ip, dest_ip);
            filter();
        }
        else if(!strcmp(temp[0], "https")){
            print_nodes(HTTPSMODE, ip_either, source_ip, dest_ip);
            filter();
        }
        else if(!strcmp(temp[0], "dns")){
            print_nodes(DNSMODE, ip_either, source_ip, dest_ip);
            filter();
        }
        else{
            printf("ERR - The invalid value. The form is as follows:\n");
            filter();
        }
    }
    else{
        printf("ERR - Too many arguments. The form is as follows:\n");
        filter();
    }
}
