/*
步骤1. 进入lib目录并生成共享库：

    cd lib
    make

步骤2. 进入app目录并生成可执行文件：

    cd ../app
    make

步骤3. 运行
    确保main.exe和mylib.dll位于同一目录中，或者将mylib.dll添加到系统PATH中，然后运行main.exe。

    注意：
        首先在Windows操作系统的[网络适配器]属性设置中配置IP地址。

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
