/*
 ** BASE	---> 	SERVER: 	0x23 0x23 .... 0xd 0xa		//## ... \r\n
 ** SERVER	---> 	BASE:
 **
 **
 ** ROVER 	--->	SERVER:		0x5b 0x5b .... 0x5d 0x5d	//[[ ... ]]
 ** SERVER	---> 	ROVER:
 **
 */
#include "linked_list.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>		/* for strncpy */
#include <unistd.h>		/* for close */
#include <arpa/inet.h>		/* for inet_ntoa */
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <netinet/in.h>
#include <net/if.h>

#define BUFFER_SIZE 1024
#define RTCM_SIZE 2048		/* Max RTCM size */

pthread_mutex_t		mutex	= PTHREAD_MUTEX_INITIALIZER;	/* else call pthread_mutex_init() */
pthread_cond_t		cond	= PTHREAD_COND_INITIALIZER;	/* else call pthread_cond_init() */

/********************************************************************************/
/* Define client types */
/********************************************************************************/
//typedef enum {
//    EN_UNKNOW,
//    EN_BASE,
//    EN_ROVER,
//} EN_PACKET_TYPE;
//
//typedef struct {
//    EN_PACKET_TYPE type;
//    const char* name;
//} packet_type_st;
//
//const packet_type_st STR_PACKET_TYPE[] = {
//    { EN_UNKNOW, "UNKNOW" },
//    { EN_BASE,   "BASE  " },
//    { EN_ROVER,  "ROVER " },
//};

const char* STR_THREAD_TYPE[] = {
	"UNKNOW",
	"BASE  ",
	"ROVER ",
};

enum EN_PACKET_TYPE {
	EN_UNKNOW,
	EN_BASE,
	EN_ROVER,
};

/********************************************************************************/
/*  */
/********************************************************************************/
struct server_info {
	char if_name[64];
	char ip[INET_ADDRSTRLEN];
	int socket;
};

struct client_info {
	int socket;
	char ip[INET_ADDRSTRLEN];
	enum EN_PACKET_TYPE type;
	struct sockaddr_in address;
	pthread_t thread;

	/****************************************/
	/* 750 Old Model 포맷 */
	/****************************************/
	//uint8_t stx[2];			/* index 0~1  (2 byte, fix값: "##") */
	uint8_t seq;				/* index 2    (1 byte)	*/
	char dev_name[64];			/* index 3~13 (11 byte) */
	//char  count[3];			/* 			*/
	char  count[4];				/* index 14~17 (4 byte)	*/

	short pkt_len;				/* index 18~19, 2 byte */
	/* NOTE:
	 ** 	section A + section B 의 길이다.
	 **
	 ** 	pkt_len = 	sizeof(short) +		//section A
	 **			a_msg_len +		//section A
	 **			sizeof(short) +		//section B
	 **			b_msg_len +		//section B
	 */

	/****************************************/
	/* section A */
	/****************************************/
	short a_msg_len;			/* index 20~21, 2 byte. 이 값은 바로 뒤의 a_rtcm[] 의 길이다 */
	char a_rtcm[BUFFER_SIZE];		/* index 22~a_msg_len 의 값 만큼 */

	/****************************************/
	/* section B */
	/****************************************/
	short b_msg_len;			/* index Y, 2 byte. 이 값은 바로 뒤의 b_rtcm[] 의 길이다 */
	char b_rtcm[BUFFER_SIZE];		/* index Y~b_msg_len 의 값 만큼 */

	char cs;				/*  */
	//uint8_t etx[2];			/* index Z~Z+1 (2 byte, fix값: "\r\n") */

	// struct client_info *next;
};

struct LinkedList* g_client_meta = NULL;
struct server_info g_server_info;

struct ThreadArgs {
    struct client_info *client;
    struct LinkedList* list_meta;
};

int Is_IP_and_name_same(void* l_data, void* r_data)
{
    struct client_info * l_struct = (struct client_info *)l_data;
    struct client_info * r_struct = (struct client_info *)r_data;

    if (strcmp(l_struct->dev_name, r_struct->dev_name) == 0 && strcmp(l_struct->ip, r_struct->ip) == 0)
    {
        return 1;
    }

    return 0;
}

int Is_socket_same(void* l_data, void* pFd)
{
    struct client_info * client = (struct client_info *)l_data;
    int fd = *(int*)pFd;

    if (client->socket == fd)
    {
        return 1;
    }

    return 0;
}

/********************************************************************************/
/* Add a new client to the linked list, removing any existing client with the same IP and device name */
/********************************************************************************/
void add_client(struct LinkedList* list_meta, struct client_info *new_client)
{
	pthread_mutex_lock(&mutex);

    // check if new_client is NULL or new_client is the first client
    if (new_client == NULL ) {
        pthread_mutex_unlock(&mutex);
        return;
    }

    void* to_delete = Add_to_linked_list_if(list_meta, new_client, Is_IP_and_name_same);
    struct client_info* to_delete_client = (struct client_info*)to_delete;
    if (to_delete_client != NULL)
    {
        close(to_delete_client->socket);
        free(to_delete_client);
    }

	pthread_mutex_unlock(&mutex);
}


/********************************************************************************/
/* Remove a client from the linked list */
/********************************************************************************/
void remove_client(struct LinkedList* list_meta, int s)
{
	pthread_mutex_lock(&mutex);

	void* to_delete = Delete_from_linked_list_if(list_meta, &s, Is_socket_same);
    struct client_info* to_delete_client = (struct client_info*)to_delete;
    if (to_delete_client != NULL)
    {
        close(to_delete_client->socket);
        free(to_delete_client);
    }

	pthread_mutex_unlock(&mutex);
}

// Clean up function for linked list
void CleanClient(void* data) {
    struct client_info* client = (struct client_info*)data;
    if (client != NULL)
    {
        close(client->socket);
        free(client);
    }
}

/********************************************************************************/
/* Clean up linked list */
/********************************************************************************/
void free_list(struct LinkedList* list_meta)
{
	pthread_mutex_lock(&mutex);

    Free_linked_list(list_meta, CleanClient);

	pthread_mutex_unlock(&mutex);
}

// /********************************************************************************/
// /* Find a client in the linked list by socket */
// /********************************************************************************/
// struct client_info *find_client(int socket)
// {
// 	pthread_mutex_lock(&mutex);
// 	struct client_info *curr = g_client_head;
// 	while (curr != NULL) {
// 		if (curr->socket == socket)
// 			return curr;
// 		curr = curr->next;
// 	}
// 	pthread_mutex_unlock(&mutex);
// 	return NULL;
// }

/********************************************************************************/
/* Print all clients in the linked list */
/********************************************************************************/
int print_all_clients()
{
	volatile int r_cnt = 0;
    struct LinkedList* node = g_client_meta->next;
	struct client_info *curr = NULL;
	while (node != NULL) {
        curr = (struct client_info *) node->data;
		printf("%-16s %-8s %-15s 0x%-12lX  %-5d     %-7d\n",
				curr->ip,
				STR_THREAD_TYPE[curr->type],
				curr->dev_name,
				curr->thread,
				curr->socket,
				ntohs(curr->address.sin_port)
		      );

		if (curr->type == EN_ROVER)
			r_cnt++;

		node = node->next;
	}
	return r_cnt;
}


/********************************************************************************/
/*  */
/********************************************************************************/
void print_status(enum EN_PACKET_TYPE pkt_type, int r_len, int w_len)
{
	/* 이전 테이블 지우기 */
	printf("\033[2J");

	/* 커서를 맨 위로 이동 */
	//printf("\033[H");

	/********************************************************************************/
	/* NOTE: START 반복 출력되는 부분 */
	/********************************************************************************/
	pthread_mutex_lock(&mutex);
	printf("--------------------------------------------------------------------------------\n");
	printf("                       ASCEN RTU C SERVER v1.0                                  \n");
	printf("                                                                                \n");
	printf("SERVER_IP: %s, IF: %s, SOCKET_FD: %d\n", g_server_info.ip, g_server_info.if_name, g_server_info.socket);
	printf("--------------------------------------------------------------------------------\n");
	printf("CLIENT_IP:       TYPE:    DEV_NAME:       THREAD_ID:    SOCKET_FD:  REMOTE_PORT:\n");
	int cnt_rover = print_all_clients(&cnt_rover);
	printf("--------------------------------------------------------------------------------\n");
	printf("SERVER RECV %d BYTE, from %s\n", r_len, STR_THREAD_TYPE[pkt_type]);
	printf("SERVER SEND %d BYTE to %d ROVERS\n", w_len, cnt_rover);
	printf("--------------------------------------------------------------------------------\n");
	//fflush(stdout);
	pthread_mutex_unlock(&mutex);
	/********************************************************************************/
	/* NOTE: END 반복 출력되는 부분 */
	/********************************************************************************/
}


/********************************************************************************/
/*  */
/********************************************************************************/
static uint8_t check_sum_xor(char* buffer, int start, int end)
{
	uint8_t xor = 0;
	for (int i = start; i < end; i++)
		xor ^= buffer[i];
	return xor;
}

/********************************************************************************/
/* RTCM 데이터를 Rover로 전달 */
/********************************************************************************/
static int create_rover_rtcm_pkt_2_send(struct client_info *client)
{
	int ret = 0;
	char rover_pkt[1024 * 3] = {0};
	pthread_t tid = pthread_self();

	/* STX */
	rover_pkt[0] = 0x5b;	/* [ */
	rover_pkt[1] = 0x5b;	/* [ */

	/* seq */
	rover_pkt[2] = client->seq;

	/* copy little-endian to big-endian */
	memcpy(&rover_pkt[3],  &client->count[2], 1);
	memcpy(&rover_pkt[4],  &client->count[1], 1);
	memcpy(&rover_pkt[5],  &client->count[0], 1);

	memcpy(&rover_pkt[6],  &client->pkt_len, 2);

	/* section a */
	memcpy(&rover_pkt[8],  				&client->a_msg_len,	2);
	memcpy(&rover_pkt[10], 				&client->a_rtcm, 	client->a_msg_len);

	/* section b */
	memcpy(&rover_pkt[10 + client->a_msg_len],	&client->b_msg_len,	2);
	memcpy(&rover_pkt[10 + client->a_msg_len + 2],	&client->b_rtcm,	client->b_msg_len);

	/*  */
	int idx_cs = 10 + client->a_msg_len + 2 + client->b_msg_len;
	rover_pkt[idx_cs] = check_sum_xor(rover_pkt, 1, idx_cs);

	/* ETX */
	rover_pkt[idx_cs + 1] = 0x5d;	/* ] */
	rover_pkt[idx_cs + 2] = 0x5d;	/* ] */

	rover_pkt[idx_cs + 3] = '\0';	/*  */

	int total_len= idx_cs + 3;

	/* scan linked list to relay rtcm */
	struct LinkedList *curr = g_client_meta->next;
	while (curr != NULL) {
		struct client_info *temp = (struct client_info *)(curr->data);

		/* 일반적으로 닫힌 소켓의 fd는 -1로 설정된다 */
		if (temp->type == EN_ROVER && temp->socket != -1) {
			ret = send(temp->socket, rover_pkt, total_len, 0);
#if ENABLE_DEBUG
			printf("[0x%lX] Server send %d byte RTCM packet to ROVER.\n", tid, ret);
			for (int i = 0; i < ret; i++)
				printf("%x ", rover_pkt[i]);
			printf("\n");
#endif
		}

		curr = curr->next;
	}

	return ret;
}

/********************************************************************************/
/* make base ack packet */
/********************************************************************************/
static int create_base_ack_pkt_2_send(struct client_info *client)
{
	int ret = 0;
	char buf[22] = {0};

	pthread_t tid = pthread_self();

	/* STX */
	buf[0] = 0x23;
	buf[1] = 0x23;
	buf[2] = client->seq;

	memcpy(&buf[3], &client->dev_name, 11);		/* device ID ID[11] */
	buf[14] = 0x20;					/* fixed dummy data: Space */

	/* copy little-endian to big-endian */
	memcpy(&buf[15], &client->count[2], 1);
	memcpy(&buf[16], &client->count[1], 1);
	memcpy(&buf[17], &client->count[0], 1);

	/* ACK/NACK */
	buf[18] = 0x01;					/* 1: ACK, 2:NACK */

	/* Check sum */
	uint8_t cs = check_sum_xor(buf, 1, 19);
	buf[19] = cs;

	/* ETX */
	buf[20] = '\r';
	buf[21] = '\n';

	/* send ack to base */
	ret = write(client->socket, buf, sizeof(buf));
#if ENABLE_DEBUG
	printf("[0x%lX] Server send %d byte ACK to BASE:", tid, ret);
	for (int i = 0; i < ret; i++)
		printf("0x%x ", buf[i]);
	printf("\n");
#endif

	return ret;
}



/********************************************************************************/
/* Just for test */
/********************************************************************************/
static enum EN_PACKET_TYPE check_packet_type_for_manual_test(const char *buffer, int r_len)
{
	enum EN_PACKET_TYPE type;

	/* NOTE: 타입 판단만 하고, 데이터를 Get 하지 말 것? */
	if (buffer[0] == '#' && buffer[strlen(buffer) - 2] == '\r' && buffer[strlen(buffer) - 1] == '\n') {
		type = EN_BASE;
	} else if (buffer[0] == '[' && buffer[strlen(buffer) - 2] == '\r' && buffer[strlen(buffer) - 1] == '\n') {
		type = EN_ROVER;
	} else {
		type = EN_UNKNOW;
	}

	return type;
}

/********************************************************************************/
/* Function to check buffer type */
/********************************************************************************/
static enum EN_PACKET_TYPE check_packet_type(struct client_info *client, const char *buffer, int r_len)
{
	if (buffer[0] == 0x23 && buffer[1] == 0x23 && buffer[r_len-2] == 0xd && buffer[r_len-1] == 0xa) {
		/* Base packet */
		client->type = EN_BASE;

		client->seq = buffer[2];				/* index[2]	:seq id		*/
		memcpy(client->dev_name,	&buffer[3], 11);	/* index[3:13]	:dev name	*/

		/* copy big-endian to little-endian */
		memcpy(&client->count[0],	&buffer[17], 1);	/* index[14:17]	:		*/
		memcpy(&client->count[1],	&buffer[16], 1);	/* index[14:17]	:		*/
		memcpy(&client->count[2],	&buffer[15], 1);	/* index[14:17]	:		*/
		memcpy(&client->count[3],	&buffer[14], 1);	/* index[14:17]	:		*/

		/*  */
		memcpy(&client->pkt_len, 	&buffer[18], 2);	/* index[18:19]	:		*/

		/* read section A */
		memcpy(&client->a_msg_len,	&buffer[20], 2);	/* index[20:21]	:		*/
		memcpy(client->a_rtcm,		&buffer[22], client->a_msg_len);

		/* read section B */
		memcpy(&client->b_msg_len,	&buffer[22 + client->a_msg_len], 2);
		memcpy(client->b_rtcm,		&buffer[22 + client->a_msg_len + 2], client->b_msg_len);

		/* read check-sum */
		client->cs = buffer[22 + client->a_msg_len + 2 + client->b_msg_len];
	} else if (buffer[0] == 0x5b && buffer[1] == 0x5b && buffer[r_len-2] == 0x5d && buffer[r_len-1] == 0x5d) {
		/* Rover packet */
		client->type = EN_ROVER;
		memcpy(client->dev_name,	&buffer[3], 11);	/* index[3:13]	:dev name	*/
	} else {
		/* Unknow packet */
		client->type = EN_UNKNOW;
	}

	return client->type;
}

/********************************************************************************/
/* Function to relay data between clients */
/********************************************************************************/
static void *client_thread(void *arg)
{
	struct ThreadArgs *thread_args = (struct ThreadArgs *)arg;
    struct client_info *client = thread_args->client;
    struct LinkedList* list_meta = thread_args->list_meta;

	char buffer[BUFFER_SIZE];
	pthread_t tid = pthread_self();

	/* linked-list에 추가하기 전에, 쓰레드가 먼저 실핼 될 수도 있으므로 초기화 */
	enum EN_PACKET_TYPE pkt_type  = EN_UNKNOW;

	int r_len = 0;
	int w_len = 0;

	while (1) {
		/* Read data from client */
		r_len = 0;
		w_len = 0;

		r_len = read(client->socket, buffer, BUFFER_SIZE);
		if (r_len > 0) {
			buffer[r_len] = '\0';

			/* Determine client type */
			pkt_type = check_packet_type(client, buffer, r_len);
			/* Base, Rover 중에 하나 */
			switch (pkt_type) {
			case EN_BASE:
				pthread_mutex_lock(&mutex);
				create_base_ack_pkt_2_send(client);
				w_len = create_rover_rtcm_pkt_2_send(client);
				pthread_mutex_unlock(&mutex);
				break;
			case EN_ROVER:
				break;
			default:
				break;
			}

			/* NOTE: thrad 정상작동 중 출력 */
			print_status(client->type, r_len, w_len);
		} else {
			/* Client disconnected */
			printf("[0x%lX] Client type %s disconnected\n", tid, STR_THREAD_TYPE[client->type]);

			remove_client(list_meta, client->socket);

			/* NOTE: thrad 종료되기전에 출력해야 한다 */
			print_status(client->type, r_len, w_len);

			pthread_exit(NULL);
		}
	}
}

/********************************************************************************/
/* */
/********************************************************************************/
int main(int argc, char *argv[])
{
	if (argc != 3) {
		printf("Usage: %s <if_name> <port>\n", argv[0]);
		printf("Ex:    %s eth0 9101\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	const char *interface = argv[1];
	int port = atoi(argv[2]);

	int server_socket;
	struct sockaddr_in server_addr, client_addr;
	socklen_t addr_size = sizeof(client_addr);
	struct ifreq ifr;
	char buffer[BUFFER_SIZE] = {0};

    struct LinkedList* list_meta = Init_linked_list();
    g_client_meta = list_meta;

	/* Create server socket */
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == -1) {
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}

	/* Get IPv4 IP address */
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, argv[1], IFNAMSIZ-1);
	ioctl(server_socket, SIOCGIFADDR, &ifr);

	g_server_info.socket = server_socket;
	strcpy(g_server_info.if_name, argv[1]);
	strcpy(g_server_info.ip, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));


	/* 인터페이스 바인딩 */
	if (setsockopt(server_socket, SOL_SOCKET, SO_BINDTODEVICE, interface, strlen(interface)) < 0) {
		perror("Setsockopt failed");
		exit(EXIT_FAILURE);
	}

	/* Set socket option to reuse address */
	int opt = 1;
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
		perror("Setsockopt failed");
		exit(EXIT_FAILURE);
	}

	/* Initialize server address structure */
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port);


	/* Bind the server socket */
	if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
		perror("Socket bind failed");
		exit(EXIT_FAILURE);
	}

	/* Listen for incoming connections */
	if (listen(server_socket, SOMAXCONN) == -1) {
		perror("Listen failed");
		exit(EXIT_FAILURE);
	}


	print_status(0, 0, 0);
	printf("Waiting for connections on port %d...\n", port);

	while (1) {
		/* Accept incoming connections */
		int new_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_size);
		if (new_socket == -1) {
			perror("Accept error");
			exit(EXIT_FAILURE);
		}

		printf("New connection, client socket FD is: %d, IP is: %s, PORT: %d\n",
				new_socket,
				inet_ntoa(client_addr.sin_addr),
				ntohs(client_addr.sin_port));

		/* Create a new client node */
		struct client_info *new_client = (struct client_info *)malloc(sizeof(struct client_info));
		if (new_client == NULL) {
			perror("Memory allocation failed");
			close(new_socket);
			continue;
		}

		new_client->socket = new_socket;
		/* Default: Unknown type */
		new_client->type = EN_UNKNOW;
		new_client->address = client_addr;
		// new_client->next = NULL;
		strcpy(new_client->ip, inet_ntoa(client_addr.sin_addr));

        struct ThreadArgs thread_args;
        thread_args.client = new_client;
        thread_args.list_meta = list_meta;

		/* Create a new thread for the client */
		if (pthread_create(&new_client->thread, NULL, client_thread, &thread_args) != 0) {
			perror("Thread creation failed");
			close(new_socket);
			free(new_client);
			continue;
		}

		/* Add the new client node to the linked list */
		add_client(list_meta, new_client);
	}

	/* Close server socket */
	close(server_socket);

	/* Clean up linked list */
	free_list(list_meta);

	/* 사용이 끝난 mutex 변수 해제. malloc로 동적으로 선언한 mutex 변수는 free()를 호출하기 전에, 꼭 pthread_mutex_destroy()를 먼저 호출해야한다. */
	pthread_mutex_destroy(&mutex);

	return 0;
}

