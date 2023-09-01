/*
 * simple slog for the project log modules.
 *
 */
#ifndef _SLOG_API_H_
#define _SLOG_API_H_

#ifdef __cplusplus

extern"C"
{
#endif

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <errno.h>
#include <pthread.h>


#ifndef SLOG_PATH_SOCKET
#define SLOG_PATH_SOCKET     "/var/tmp/slog"    //"/dev/log"
#endif

#define DEFAULT_PATH_FILE_LOG   "/var/tmp/slog.log"
#define LOG_BUF_MAX_LEN   (256+16)    //include the slog_header.

//LOG_TAG：
typedef enum {
	SLOG_MOD_KERN			= 0, 	/* kernel messages */
	SLOG_MOD_FWUPGRADE		= 1, 	/* firmware upgrade messages */
	SLOG_MOD_CAPWAP			= 2, 	/* capwap messages */
	SLOG_MOD_EVENTMGR		= 3, 	/* event manager */
	SLOG_MOD_LANHOST		= 4, 	/* lan host */

	/*THE END.... */
	SLOG_MOD_MAX                   /* just define it to global */
} SLOG_MOD_NAME_E;
static char *pmode2str[SLOG_MOD_MAX+1] = {
	 "KERNEL",
	 "FWUPGRADE",
	 "CAPWAP",
	 "EVENTMGR",
	 "LANHOST",

	 /*THE END.... */
	 "global"
};
//LOG_LEVEL:
typedef enum { 
	SLOG_LEVEL_EMERG	= 0,	/* system is unusable */
	SLOG_LEVEL_ALERT	= 1,	/* action must be taken immediately */
	SLOG_LEVEL_CRIT		= 2,	/* critical conditions */
	SLOG_LEVEL_ERR		= 3,	/* error conditions */
	ATOS_LOG_ERR		= 3,
	SLOG_LEVEL_WARNING	= 4,	/* warning conditions */
	SLOG_LEVEL_NOTICE	= 5,	/* normal but significant condition */
	SLOG_LEVEL_INFO		= 6,	/* informational */
	SLOG_LEVEL_DEBUG	= 7,	/* debug-level messages */
	ATOS_LOG_DEBUG		= 7,

	/*THE END.... */
	SLOG_LEVEL_MAX
} SLOG_LEVEL_E;
static char *pleve2str[SLOG_LEVEL_MAX+1] = {
	 "EMERG",
	 "ALERT",
	 "CRIT",
	 "ERR",
	 "WARNING",
	 "NOTICE",
	 "INFO",
	 "DEBUG",
	 ""
	};


static char *cfg_members[] = {
	"slogfile",
	"slogfile_size",
	"send2syslog",
	"send2remote",
	"debug_leve",
	"store_flag",
	NULL
};
//cfg file data format:
/*
[global]
slogfile = /var/tmp/slogd.log
#slogfile_size:  it will use 1024 as its base.
slogfile_size = 2048
#send2syslog:  1 --> call syslog() to send the message. 0 --> don't need.
send2syslog = 1
#send2remote: 192.168.1.2:514 send to remote IP 192.168.1.2:515 via UDP. //not support now.
send2remote = 192.168.1.2:514
debug_leve = 4
store_flag = 1

[KERNEL]
#SLOG_LEVEL_EMERG	= 0,	system is unusable
#SLOG_LEVEL_ALERT	= 1,	action must be taken immediately
#SLOG_LEVEL_CRIT	= 2,	critical conditions
#SLOG_LEVEL_ERR		= 3,	error conditions
#SLOG_LEVEL_WARNING	= 4,	warning conditions
#SLOG_LEVEL_NOTICE	= 5,	normal but significant condition
#SLOG_LEVEL_INFO	= 6,	informational
#SLOG_LEVEL_DEBUG	= 7,	debug-level messages
debug_leve = 4
store_flag = 0

[FWUPGRADE]
debug_leve = 6
store_flag = 1

[CAPWAP]
debug_leve = 5
store_flag = 1

# the other is for default cfg.
[other]
debug_leve = 4
store_flag = 1

*/

typedef struct slogd_ctl
{
	/*SLOG_MOD_NAME_E  mod_index; */ //we will use the arrary's index to touch every entry via pmode2str.
	SLOG_LEVEL_E     debug_leve;
	char             store_flag;         //1 --> save to file,  0 --> don't need to file, just call printf.
} slogd_ctl_s;

typedef struct slogd_cfg
{
	char     slog_file[128];
	char     send2syslog;        //1 --> call syslog() to send the message. 0 --> don't need.
	char     send2remote[128];   //192.168.1.2:514 send to remote IP 192.168.1.2:515 via UDP. //not support now.
	long int  file_max_size;
	slogd_ctl_s  mod_entry[SLOG_MOD_MAX+1];//last entry is the default cfg
} slogd_cfg_s;


typedef enum slogtype 
{
	SLOG_TYPE_LOG = 0,     //Debug log message
	SLOG_TYPE_CONTROL_MOD, //config some mod level.
	SLOG_TYPE_CFG_RELOAD,  //reload the cfg.
}slogtype_e;

#pragma pack(1)
typedef struct slog_header {
	char slog_type;//0 ---> log, 1 ---> control, 2 ---> reload the cfg
	char mod_index;
	char debug_level;
	char extend[13];
} slog_header_s;
#pragma pack()

//if you don't know your mod_index, you can use SLOG_MOD_MAX which will be match do it's program name.
void slog_printf(SLOG_MOD_NAME_E mod_index, SLOG_LEVEL_E pri_level, const char *format,...);
void slog_ctl(slogtype_e type, SLOG_MOD_NAME_E mod_index, SLOG_LEVEL_E level, const char *format,...);

#ifdef __cplusplus
}
#endif


#endif  /* _SLOG_API_H_ */
