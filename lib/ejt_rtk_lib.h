#ifndef MYLIB_H
#define MYLIB_H

#ifdef __cplusplus
extern "C" {
#endif

/* */
__declspec(dllexport) void print_network_adapters();
__declspec(dllexport) int init_rtk_lib(const char *if_name, int port);

/* */
__declspec(dllexport) int api_get_client_count(void);
__declspec(dllexport) int api_get_client_data(void *head);

/* */
__declspec(dllexport) int api_get_device_status_by_name(char *name);
__declspec(dllexport) int api_get_device_status_by_ip(char *ip);

#ifdef __cplusplus
}
#endif

#endif // MYLIB_H

