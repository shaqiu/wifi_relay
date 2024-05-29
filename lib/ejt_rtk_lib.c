/*
	EJTech RTK relay server v1.0 lib
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <pthread.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>


/* Windows OS + GCC 에서는 필요없다 */
// #pragma comment(lib, "Ws2_32.lib")
// #pragma comment(lib, "Iphlpapi.lib")

#define BUFFER_SIZE 1024
#define RTCM_SIZE 2048		/* Max RTCM size */

pthread_mutex_t		mutex	= PTHREAD_MUTEX_INITIALIZER;	/* else call pthread_mutex_init() */
pthread_cond_t		cond	= PTHREAD_COND_INITIALIZER;	/* else call pthread_cond_init() */

/********************************************************************************/
/* Define client types */
/* TODO: 나중에 최적화 */
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

/********************************************************************************/
/* */
/********************************************************************************/
typedef struct app_client_info {
	enum EN_PACKET_TYPE type;
	char ip[INET_ADDRSTRLEN];
	char dev_name[BUFFER_SIZE];
	uint8_t rtk_status;

	struct app_client_info *next;
} app_client_info_st;

/********************************************************************************/
/* */
/********************************************************************************/
struct lib_client_info {
	int socket;
	char ip[INET_ADDRSTRLEN];
	enum EN_PACKET_TYPE type;
	struct sockaddr_in address;
	pthread_t thread;
	//ptw32_handle_t thread;
	uint8_t rtk_status;

	/****************************************/
	/* 750 old model(AK-920K) format */
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

	struct lib_client_info *next;
};

/********************************************************************************/
/* TODO: 나중에 없앨 것 */
/********************************************************************************/
struct lib_client_info *g_client_head = NULL;
struct server_info g_server_info;

/********************************************************************************/
/*  */
/********************************************************************************/
void print_network_adapters() {
	PIP_ADAPTER_INFO adapter_info;
	ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
	adapter_info = (IP_ADAPTER_INFO *)malloc(ulOutBufLen);
	if (GetAdaptersInfo(adapter_info, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
		free(adapter_info);
		adapter_info = (IP_ADAPTER_INFO *)malloc(ulOutBufLen);
	}

	if (GetAdaptersInfo(adapter_info, &ulOutBufLen) == NO_ERROR) {
		PIP_ADAPTER_INFO pAdapterInfo = adapter_info;
		printf("[LIB]--------------------------------------------------------------------------------\n");
		printf("[LIB]IP Address:           Network Adapter Name:\n");
		printf("[LIB]--------------------------------------------------------------------------------\n");
		while (pAdapterInfo) {
			printf("[LIB]%-16s      %s\n", pAdapterInfo->IpAddressList.IpAddress.String, pAdapterInfo->Description);
			pAdapterInfo = pAdapterInfo->Next;
		}
	} else {
		fprintf(stderr, "GetAdaptersInfo failed.\n");
	}
	free(adapter_info);
}

/********************************************************************************/
/* Add a new client to the linked list, removing any existing client with the same IP and device name */
/********************************************************************************/
void lib_add_client(struct lib_client_info *new_client)
{
	pthread_mutex_lock(&mutex);
	struct lib_client_info *curr = g_client_head;
	struct lib_client_info *prev = NULL;

	while (curr != NULL) {
		if (strcmp(curr->ip, new_client->ip) == 0 && strcmp(curr->dev_name, new_client->dev_name) == 0) {
			if (prev == NULL)
				g_client_head = curr->next;
			else
				prev->next = curr->next;
			closesocket(curr->socket);
			free(curr);
			break;
		}
		prev = curr;
		curr = curr->next;
	}

	new_client->next = g_client_head;
	g_client_head = new_client;
	pthread_mutex_unlock(&mutex);
}


/********************************************************************************/
/* Remove a client from the linked list */
/********************************************************************************/
void lib_remove_client(int socket)
{
	pthread_mutex_lock(&mutex);
	struct lib_client_info *curr = g_client_head;
	struct lib_client_info *prev = NULL;

	while (curr != NULL) {
		if (curr->socket == socket) {
			if (prev == NULL)
				g_client_head = curr->next;
			else
				prev->next = curr->next;
			closesocket(curr->socket);
			free(curr);
			pthread_mutex_unlock(&mutex);
			return;
		}
		prev = curr;
		curr = curr->next;
	}
	pthread_mutex_unlock(&mutex);
}

/********************************************************************************/
/* Clean up linked list */
/********************************************************************************/
void lib_free_list(void)
{
	pthread_mutex_lock(&mutex);
	struct lib_client_info *curr = g_client_head;
	while (curr != NULL) {
		struct lib_client_info *temp = curr;
		curr = curr->next;
		closesocket(temp->socket);
		free(temp);
	}
	pthread_mutex_unlock(&mutex);
}

/********************************************************************************/
/* Find a client in the linked list by socket */
/********************************************************************************/
struct lib_client_info *find_client(int socket)
{
	pthread_mutex_lock(&mutex);
	struct lib_client_info *curr = g_client_head;
	while (curr != NULL) {
		if (curr->socket == socket)
			return curr;
		curr = curr->next;
	}
	pthread_mutex_unlock(&mutex);
	return NULL;
}

#if 0
/* NOTE: 필요없다, 이 함수는 */
/********************************************************************************/
/* Find a client in the linked list by socket to update rtk status */
/********************************************************************************/
static int lib_find_rover_client_2_update(int socket, uint8_t status)
{
	printf("[LIB]%s(): socket=%d, status=%c\n", __func__, socket, status);
	struct lib_client_info *curr = g_client_head;
	while (curr != NULL) {
		if (curr->socket == socket) {
			curr->rtk_status = status;
			return 0;
		}
		curr = curr->next;
	}
	return -1;
}
#endif



/********************************************************************************/
/* Print all clients in the linked list */
/* TODO: 이 함수내부에는 lock 걸지 말 것 */
/********************************************************************************/
int lib_print_all_clients()
{
	volatile int cnt = 0;
	struct lib_client_info *curr = g_client_head;
	while (curr != NULL) {
		printf("[LIB]%-16s %-8s %-15s %-5d       %-7d       0x%-7x\n",
				curr->ip,
				STR_THREAD_TYPE[curr->type],
				curr->dev_name,
				curr->socket,
				ntohs(curr->address.sin_port),
				curr->rtk_status
		);

		cnt++;
		curr = curr->next;
	}
	return cnt;
}


/********************************************************************************/
/* Print all clients in the linked list */
/* TODO: 이 함수내부에는 lock 걸지 말 것 */
/********************************************************************************/
int lib_print_rover_clients()
{
	volatile int r_cnt = 0;
	struct lib_client_info *curr = g_client_head;
	while (curr != NULL) {
		printf("[LIB]%-16s %-8s %-15s %-5d       %-7d       0x%-7x\n",
				curr->ip,
				STR_THREAD_TYPE[curr->type],
				curr->dev_name,
				curr->socket,
				ntohs(curr->address.sin_port),
				curr->rtk_status
		);

		if (curr->type == EN_ROVER)
			r_cnt++;

		curr = curr->next;
	}
	return r_cnt;
}


/********************************************************************************/
/* NOTE: 여러 client중, 한 사람만 프린트하면 된다? 아니다? */
/********************************************************************************/
void lib_print_status(enum EN_PACKET_TYPE pkt_type, int r_len, int w_len)
{
	/* 이전 테이블 지우기 */
	//printf("\033[2J");

	/* 커서를 맨 위로 이동 */
	//printf("\033[H");

	pthread_mutex_lock(&mutex);
	/********************************************************************************/
	/* NOTE: START 반복 출력되는 부분 */
	/********************************************************************************/
	printf("[LIB]--------------------------------------------------------------------------------\n");
	printf("[LIB]                       EJTech RTK Relay Server v1.0                             \n");
	printf("[LIB]                                                                                \n");
	print_network_adapters(); /* Print network adapters before checking arguments */
	printf("[LIB]                                                                                \n");
	printf("[LIB]--------------------------------------------------------------------------------\n");
	printf("[LIB]SERVER_IF: %s\n", g_server_info.if_name);
	printf("[LIB]SERVER_IP: %s\n", g_server_info.ip);
	printf("[LIB]SOCKET_FD: %d\n", g_server_info.socket);
	printf("[LIB]--------------------------------------------------------------------------------\n");
	printf("[LIB]CLIENT_IP:       TYPE:    DEV_NAME:       SOCKET_FD:  REMOTE_PORT:  RTK_STATUS: \n");

	int cnt = lib_print_all_clients();

	printf("[LIB]--------------------------------------------------------------------------------\n");
	printf("[LIB]SERVER RECV %d BYTE, from %s\n", r_len, STR_THREAD_TYPE[pkt_type]);
	printf("[LIB]SERVER SEND %d BYTE to %d ROVERS\n", w_len, cnt);
	printf("[LIB]--------------------------------------------------------------------------------\n");
	/********************************************************************************/
	/* NOTE: END 반복 출력되는 부분 */
	/********************************************************************************/
	pthread_mutex_unlock(&mutex);
}









/********************************************************************************/
/* api 0:  return node counts */
/********************************************************************************/
int api_get_client_count(void)
{
	int n = -1;

	if (pthread_mutex_lock(&mutex) == 0) {	/* NOT EBUSY */
		n = lib_print_all_clients();
		pthread_mutex_unlock(&mutex);
	}

	return n;
}

/********************************************************************************/
/* api 1: return device ip, name, status */
/********************************************************************************/
int api_get_client_data(void *head)
{
	printf("%s:L%d\n", __func__, __LINE__);
	if (head == NULL) {
		printf("[LIB]Please provide valid list head\n");
		return -1;
	}

	if (pthread_mutex_lock(&mutex) == 0) {	/* NOT EBUSY */
		struct lib_client_info *curr = g_client_head;
		struct app_client_info *app_curr= (struct app_client_info *)head;

		while (curr != NULL && app_curr != NULL) {
			printf("%s:L%d\n", __func__, __LINE__);
			app_curr->type = curr->type;
			strcpy(app_curr->ip, curr->ip);
			strcpy(app_curr->dev_name, curr->dev_name);
			app_curr->rtk_status = curr->rtk_status;

			printf("%s:L%d\n", __func__, __LINE__);
			curr = curr->next;
			app_curr = app_curr->next;
		}

		pthread_mutex_unlock(&mutex);
	}
	return 0;
}



/********************************************************************************/
/* api 2: */
/********************************************************************************/
int api_get_device_status_by_name(char *name)
{
	int ret = -1;

	if (name == NULL) {
		printf("[LIB]Please provide valid pointer\n");
		return ret;
	}

	if (pthread_mutex_lock(&mutex) == 0) {	/* NOT EBUSY */
		/* TODO: return device status */

		ret = 0;
		pthread_mutex_unlock(&mutex);
	}
	return ret;
}

/********************************************************************************/
/* api 3: */
/********************************************************************************/
int api_get_device_status_by_ip(char *ip)
{
	int ret = -1;

	if (ip == NULL) {
		printf("[LIB]Please provide valid pointer\n");
		return ret;
	}

	if (pthread_mutex_lock(&mutex) == 0) {	/* NOT EBUSY */
		/* TODO: return device status */

		ret = 0;
		pthread_mutex_unlock(&mutex);
	}
	return ret;
}













/********************************************************************************/
/*  */
/********************************************************************************/
static uint8_t lib_check_sum_xor(char* buffer, int start, int end)
{
	uint8_t xor = 0;
	for (int i = start; i < end; i++)
		xor ^= buffer[i];
	return xor;
}

/********************************************************************************/
/* RTCM 데이터를 Rover로 전달 */
/********************************************************************************/
static int lib_create_rover_rtcm_pkt_2_send(struct lib_client_info *client)
{
	int ret = 0;
	char rover_pkt[1024 * 3] = {0};
	//pthread_t tid = pthread_self();

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
	rover_pkt[idx_cs] = lib_check_sum_xor(rover_pkt, 1, idx_cs);

	/* ETX */
	rover_pkt[idx_cs + 1] = 0x5d;	/* ] */
	rover_pkt[idx_cs + 2] = 0x5d;	/* ] */

	rover_pkt[idx_cs + 3] = '\0';	/*  */

	int total_len= idx_cs + 3;

	/* scan linked list to relay rtcm */
	struct lib_client_info *curr = g_client_head;
	while (curr != NULL) {
		struct lib_client_info *temp = curr;

		/********************************************************************************/
		/* 일반적으로 닫힌 소켓의 fd는 -1로 설정된다 */
		/********************************************************************************/
		if (temp->type == EN_ROVER && temp->socket != -1) {
			ret = send(temp->socket, rover_pkt, total_len, 0);
#if ENABLE_DEBUG
			printf("[LIB][0x%lX] Server send %d byte RTCM packet to ROVER.\n", tid, ret);
			for (int i = 0; i < ret; i++)
				printf("[LIB]%x ", rover_pkt[i]);
			printf("[LIB]\n");
#endif
		}

		curr = curr->next;
	}

	return ret;
}

/********************************************************************************/
/* make base ack packet */
/********************************************************************************/
static int lib_create_base_ack_pkt_2_send(struct lib_client_info *client)
{
	int ret = 0;
	char buf[22] = {0};

	//pthread_t tid = pthread_self();

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
	uint8_t cs = lib_check_sum_xor(buf, 1, 19);
	buf[19] = cs;

	/* ETX */
	buf[20] = '\r';
	buf[21] = '\n';

	/* send ack to base */
	ret = send(client->socket, buf, sizeof(buf), 0);
#if ENABLE_DEBUG
	printf("[LIB][0x%lX] Server send %d byte ACK to BASE:", tid, ret);
	for (int i = 0; i < ret; i++)
		printf("[LIB]0x%x ", buf[i]);
	printf("[LIB]\n");
#endif

	return ret;
}


#if 0
/********************************************************************************/
/* Just for test */
/********************************************************************************/
static enum EN_PACKET_TYPE parse_packet_type_for_manual_test(const char *buffer, int r_len)
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
#endif

/********************************************************************************/
/* Function to check buffer type */
/********************************************************************************/
static enum EN_PACKET_TYPE lib_parse_packet(struct lib_client_info *client, const char *buffer, int r_len)
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

		/* base인 경우는 0x7 로 Hard coding */
		client->rtk_status = 0x7;

	} else if (buffer[0] == 0x5b && buffer[1] == 0x5b && buffer[r_len-2] == 0x5d && buffer[r_len-1] == 0x5d) {
		/* Rover packet */
		client->type = EN_ROVER;
		memcpy(client->dev_name,	&buffer[3], 11);	/* index[3:13]	:dev name	*/

		/* update rover RTK status */
		if (r_len == 51)
			client->rtk_status = buffer[22];		/* 0x0 ~ 0x4 */
		else
			client->rtk_status = 0x2D;			/* '-' */
	} else {
		/* Unknow packet */
		client->type = EN_UNKNOW;
	}

	return client->type;
}

/********************************************************************************/
/* NOTE: 여러 client중, 한 사람만 프린트하면 된다? 아니다? */
/* NOTE: Base, Rover Thread 중에 하나다*/
/********************************************************************************/
void *lib_client_thread(void *arg)
{
	struct lib_client_info *client = (struct lib_client_info *)arg;
	char buffer[BUFFER_SIZE];
	//pthread_t tid = pthread_self();

	/* 첫 실행시 linked-list에 추가하기 전에, 쓰레드가 먼저 실핼 될 수도 있으므로 초기화한다 */
	//enum EN_PACKET_TYPE pkt_type  = EN_UNKNOW;

	int r_len;
	int w_len;

	while (1) {
		/* Read data from client */
		r_len = 0;
		w_len = 0;

		/* 첫 실행시 빼고, linked-list에 이미 등록된 Base/Rover 중에 하나일 것이다 */
		r_len = recv(client->socket, buffer, BUFFER_SIZE, 0);
		if (r_len > 0) {
			buffer[r_len] = '\0';

			/********************************************************************************/
			/* Determine client type (packet type) */
			/********************************************************************************/
			lib_parse_packet(client, buffer, r_len);

			/* Base, Rover 중에 하나 */
			switch (client->type) {
			case EN_BASE:
				pthread_mutex_lock(&mutex);
				lib_create_base_ack_pkt_2_send(client);
				w_len = lib_create_rover_rtcm_pkt_2_send(client);
				pthread_mutex_unlock(&mutex);

				break;
			case EN_ROVER:
				/* NOTE: 패킷해석 함수에서 이미 다 처리를 했기때문에, 여기서는 아무것도 하지 않는다 */
				break;
			default:
				break;
			}

			/********************************************************************************/
			/* NOTE: 여러 client중, 한 사람만 프린트하면 된다? 아니다? */
			/********************************************************************************/
			lib_print_status(client->type, r_len, w_len);
		} else {
			/* Client disconnected */
			printf("[LIB]Client type %s disconnected\n", STR_THREAD_TYPE[client->type]);

			lib_remove_client(client->socket);

			/* thread 종료되기전에 출력해야 한다 */
			lib_print_status(client->type, r_len, w_len);

			pthread_exit(NULL);
		}
	}
}



/********************************************************************************/
/*  */
/********************************************************************************/
typedef struct thread_param {
	char if_name[64];
	int port;
} thread_param_st;

void* lib_thread_gen_server(void *arg)
{
	if (arg == NULL) {
		printf("[LIB]Please check thread parameter!\n");
		return NULL;
	}

	thread_param_st *param = (thread_param_st*)arg;

	const char* if_name = param->if_name;
	int port = param->port;

	/* NOTE: Disable buffering for stdout. 이것을 실행하지 않으면, printf 문이 바로바로 출력되지 않는다 */
	setvbuf(stdout, NULL, _IONBF, 0);

	/*  */
	WSADATA wsaData;

	/*  */
	SOCKET server_socket;
	struct sockaddr_in server_addr, client_addr;
	int addr_size = sizeof(client_addr);
	pthread_t thread_id;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		fprintf(stderr, "WSAStartup failed. Error: %d\n", WSAGetLastError());
		free(param);
		return NULL;
	}

	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == INVALID_SOCKET) {
		fprintf(stderr, "Socket creation failed. Error: %d\n", WSAGetLastError());
	/*
		 * WSACleanup 함수는 Winsock 2 DLL(Ws2_32.dll)의 사용을 종료합니다.
		 * 작업이 성공한 경우 반환 값은 0입니다. 그렇지 않으면 SOCKET_ERROR 값이 반환되고 WSAGetLastError를 호출하여 특정 오류 번호를 검색할 수 있습니다.
		 *
		 * 다중 스레드 환경에서 WSACleanup 은 모든 스레드에 대한 Windows 소켓 작업을 종료합니다.
		 * */
		WSACleanup();
		free(param);
		return NULL;
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);

	// Get the IP address associated with the given network interface name
	PIP_ADAPTER_INFO adapter_info;
	ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
	adapter_info = (IP_ADAPTER_INFO *)malloc(ulOutBufLen);
	if (GetAdaptersInfo(adapter_info, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
		free(adapter_info);
		adapter_info = (IP_ADAPTER_INFO *)malloc(ulOutBufLen);
	}

	if (GetAdaptersInfo(adapter_info, &ulOutBufLen) == NO_ERROR) {
		PIP_ADAPTER_INFO pAdapterInfo = adapter_info;
		while (pAdapterInfo) {
			if (strcmp(pAdapterInfo->Description, if_name) == 0) {
				server_addr.sin_addr.s_addr = inet_addr(pAdapterInfo->IpAddressList.IpAddress.String);
				break;
			}
			pAdapterInfo = pAdapterInfo->Next;
		}
	}

	printf("[LIB]--------------------------------------------------------------------------------\n");
	printf("[LIB]SERVER IF  : %s\n", if_name);
	printf("[LIB]SERVER IP  : %s\n", inet_ntoa(server_addr.sin_addr));
	printf("[LIB]SERVER PORT: %d\n", port);
	printf("[LIB]--------------------------------------------------------------------------------\n");

	g_server_info.socket = server_socket;
	strcpy(g_server_info.if_name, if_name);
	strcpy(g_server_info.ip, inet_ntoa(server_addr.sin_addr));


	free(adapter_info);

	if (server_addr.sin_addr.s_addr == INADDR_NONE) {
		fprintf(stderr, "Invalid network interface name or no IP address found.\n");
		closesocket(server_socket);
		WSACleanup();
		free(param);
		return NULL;
	}

	if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
		fprintf(stderr, "Bind failed. Error: %d\n", WSAGetLastError());
		closesocket(server_socket);
		WSACleanup();
		free(param);
		return NULL;
	}

	if (listen(server_socket, 5) == SOCKET_ERROR) {
		fprintf(stderr, "Listen failed. Error: %d\n", WSAGetLastError());
		closesocket(server_socket);
		WSACleanup();
		free(param);
		return NULL;
	}

	printf("[LIB]Server listening on %s:%d\n", inet_ntoa(server_addr.sin_addr), port);

	/********************************************************************************/
	/* NOTE: 상태 출력은 메인 프로세스에서 하면 안된다 */
	/********************************************************************************/
	while (1) {
		/* Accept incoming connections */
		int new_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_size);
		if (new_socket == -1) {
			fprintf(stderr, "Accept failed. Error: %d\n", WSAGetLastError());
			closesocket(server_socket);
			continue;
		}

		printf("[LIB]New connection, client socket FD is: %d, IP is: %s, PORT: %d\n",
				new_socket,
				inet_ntoa(client_addr.sin_addr),
				ntohs(client_addr.sin_port));

		/* Create a new client node */
		struct lib_client_info *new_client = (struct lib_client_info *)calloc(1, sizeof(struct lib_client_info));
		if (new_client == NULL) {
			perror("Memory allocation failed");
			closesocket(new_socket);
			continue;
		}

		new_client->socket = new_socket;
		/* Default: Unknown type */
		new_client->type = EN_UNKNOW;
		new_client->address = client_addr;
		new_client->next = NULL;
		strcpy(new_client->ip, inet_ntoa(client_addr.sin_addr));

		/* Create a new thread for the client */
		if (pthread_create(&new_client->thread, NULL, lib_client_thread, new_client) != 0) {
			fprintf(stderr, "Failed to create thread. Error: %d\n", WSAGetLastError());
			closesocket(new_socket);
			free(new_client);
		} else {
			pthread_detach(thread_id); // Ensure resources are freed when thread exits
		}

		/* Add the new client node to the linked list */
		lib_add_client(new_client);
	}

	WSACleanup();
	free(param);
	return NULL;
}


/********************************************************************************/
/*  */
/********************************************************************************/
int init_rtk_lib(const char *if_name, int port)
{
	pthread_t th;
	thread_param_st *param = (thread_param_st *)calloc(1, sizeof(thread_param_st));;

	if (if_name == NULL) {
		printf("[LIB]Please provide the network card name\n");
		return -1;
	}

	if (port <= 0) {
		printf("[LIB]Please provide the port number\n");
		return -1;
	}

	strcpy(param->if_name, if_name);
	param->port = port;

	pthread_create(&th, NULL, lib_thread_gen_server, (void *)param);
	return 0;
}
