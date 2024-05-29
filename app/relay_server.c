/*
step1. lib 디렉토리로 이동하여 공유 라이브러리 생성:

	cd lib
	make

step2. app 디렉토리로 이동하여 실행 파일 생성:

	cd ../app
	make

step3. 실행
	main.exe 와 mylib.dll 이 같은 디렉토리에 위치하도록 하거나,
	mylib.dll을 시스템 PATH 에 포함시킨 후 main.exe 를 실행하면 된다.

	NOTE:
		먼저 Windows OS 의 [네트워크 카드] 속성 설정에서 IP 주소를 설정해야 한다.

		$ ./relay_server.exe 9101 "Intel(R) Wi-Fi 6E AX211 160MHz"
		$ ./relay_server.exe 9101 Intel\(R\)\ Wi-Fi\ 6E\ AX211\ 160MHz
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <ws2tcpip.h>
#include <Ws2ipdef.h>
#include <windows.h>

#include "ejt_rtk_lib.h"

#define BUF_SIZE 	(64)
#define MAX_CLIENTS	(1024 * 2)


/********************************************************************************/
/* */
/********************************************************************************/
enum EN_PACKET_TYPE {
	EN_UNKNOW,
	EN_BASE,
	EN_ROVER,
};

const char* STR_THREAD_TYPE[] = {
	"UNKNOW",
	"BASE  ",
	"ROVER ",
};

struct app_client_info {
	enum EN_PACKET_TYPE type;
	char ip[INET_ADDRSTRLEN];
	char dev_name[BUF_SIZE];
	uint8_t rtk_status;

	struct app_client_info *next;
};

struct app_client_info	*g_head;



/********************************************************************************/
/*  */
/********************************************************************************/
void app_add_client(struct app_client_info *new_client)
{
	struct app_client_info *curr = g_head;
	struct app_client_info *prev = NULL;

	printf("%s:L%d\n", __func__, __LINE__);
	while (curr != NULL) {
		if (strcmp(curr->ip, new_client->ip) == 0 && strcmp(curr->dev_name, new_client->dev_name) == 0) {
			if (prev == NULL)
				g_head = curr->next;
			else
				prev->next = curr->next;
			free(curr);
			break;
		}
		prev = curr;
		curr = curr->next;
		printf("%s:L%d\n", __func__, __LINE__);
	}

	new_client->next = g_head;
	g_head = new_client;
}


void app_free_list(void)
{
	printf("%s:L%d\n", __func__, __LINE__);
	struct app_client_info *curr = g_head;
	while (curr != NULL) {
		struct app_client_info *temp = curr;
		curr = curr->next;
		printf("%s:L%d\n", __func__, __LINE__);
		free(temp);
	}
}

void app_print_all_clients()
{
	printf("%s:L%d\n", __func__, __LINE__);
	struct app_client_info *curr = g_head;
	while (curr != NULL) {
		printf("[APP]TYPE:[%-8s], IP_ADDR:[%-16s], DEV_NAME:[%-15s] RTK_STATUS:[0x%-7x]\n",
				STR_THREAD_TYPE[curr->type],
				curr->ip,
				curr->dev_name,
				curr->rtk_status
		);
		printf("%s:L%d\n", __func__, __LINE__);
		curr = curr->next;
	}
	return;
}


/********************************************************************************/
/* */
/********************************************************************************/
int main(int argc, char *argv[])
{
	int ret;
	int port;
	const char *if_name;

	setvbuf(stdout, NULL, _IONBF, 0);

	if (argc != 3) {
		print_network_adapters();

		fprintf(stderr, "Usage: %s port_number \"network_interface_name\"\n", argv[0]);
		return 1;
	}

	port = atoi(argv[1]);
	if_name = argv[2];

	/* Call .dll library */
	init_rtk_lib(if_name, port);

	Sleep(5);

	int n;

	while (1) {
		n = api_get_client_count();
		if (n <= 0)
			continue;

		printf("[APP]N = %d\n", n);


		Sleep(2000);	/* milliseconds */
		printf("\033[2J");
		printf("\033[H");

/* FIXME: panic... */
#if 1
		if (n > 0) {
			for (int i = 0; i < n; i++) {
				struct app_client_info *new_client = (struct app_client_info *)calloc(1, sizeof(struct app_client_info));
				if (new_client == NULL) {
					perror("Memory allocation failed");
					goto exit;
				}

				app_add_client(new_client);
			}

			api_get_client_data(g_head);

			app_print_all_clients();

			//app_free_list();

			printf("\033[2J");
			printf("\033[H");
		}
		Sleep(5000);	/* milliseconds */
#endif
	}

exit:
	return 0;
}
