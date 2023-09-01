/*
 * system helper's APIs.
 *
 */
#ifndef _SYSHELPER_API_H_
#define _SYSHELPER_API_H_

#ifdef __cplusplus

extern"C"
{
#endif

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include "libhttpc.h"


typedef enum LED_NO {
	LED_NO_GREEN = 0,  //FOR GPIO 10
	LED_NO_RED  = 1,  //FOR GPIO 1
	LED_NO_MIX  = 2,  //mix red & green

	//the last member
	LED_NO_MAX
} LED_NO_E;

typedef enum LED_ACTION {
	LED_ACTION_OFF = 0,
	LED_ACTION_ON  = 1,
	LED_ACTION_BLINK  = 2,
	LED_ACTION_BLINK_SLOW  = 3,
	LED_ACTION_BLINK_VERY_SLOW = 4,

	//the last member
	LED_ACTION_MAX
} LED_ACTION_E;


//define some call back function type:
typedef void (*rsp_cb)(void *data);
typedef void (*thread_func)(void *data);

typedef void (*ending_act)(void *data);

//define the struct which will be used in signal_call's ending action.
typedef struct signal_ending_act {
	ending_act  cb;
	void* data;
} signal_ending_act_st;

//file APIs
int copyfile(char * src_name, char * des_name);
#define MAXGET_PROFILE_SIZE 128
int get_profile_key_val(char *keyname,char *str_return, int size,char *path);
int find_path_mountpoint(const char *mount_path, char *dev_name);
int find_rootfs_path(char *root_dev);
int cmd_dump(char *cmdbuf, char *outbuf, int buflen);

//network APIs
int get_mac_by_interface(const char* interface, char* mac);
int get_dev_first_mac(char * mac);
int bridge_has_port(const char *brname, const char *port);

//thread APIs
int start_pthread(thread_func func, void * data);

//led control
int sys_led_ctrl(LED_NO_E led, LED_ACTION_E act);

//system info
pid_t gettid();
void get_current_time2str(char *buf);
int signal_dump_bt2file(int signo, const char *path, signal_ending_act_st *act);
void get_threadname_bypid(pid_t pid, pid_t tid, char *name);


//other
void sys_upload_slog(struct httpc_input *input, rsp_cb * cb, char * result_json);
void sys_upgrade_fw(struct httpc_input *input, rsp_cb * cb, char * result_json);
int sys_upgrade_firmware(const char * file_name, int type, bool bootflag);



#ifdef __cplusplus
}
#endif


#endif  /* _SYSHELPER_API_H_ */
