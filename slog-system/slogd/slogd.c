/*
 * simple logd for the project log modules.
 *
 */

#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <signal.h>
#include <poll.h>
#include <sys/file.h>
#include "libslog.h"

#include "msgqueue.h"
#include "savefile.h"
#include "slogdcfg.h"


static slogd_cfg_s g_slogd_cfg = {0};


static int is_running(char *name)
{
	int ret = 0;
	int fd = open(name, O_RDONLY);
	if (fd != -1)
	{
		if(flock(fd, LOCK_EX|LOCK_NB))
		{
			//printf("lock failed!!!\n");
			ret = 1;
		}

		//close(fd);//don't close it!!!
	}

	return ret;
}

static void usage(char *name)
{
	printf("Usage: %s [-c cfgfile] [-r] [ -m \" mod_index  level store_flag\"]\n"
					"\t\t-h   help for the usage  \n"
            		"\t\t-c   input the config file name \n"
					"\t\t-r   reload the config file \n"
					"\t\t-m   config the module debug, use the string format to config the module index and level, such \" 2 4 \"\n", name); 
	return;
}

int main(int argc, char **argv)
{
	struct sockaddr_un sunx;
	int sock_fd;
	char cfg_name[128] = "/etc/slogd.cfg";

	//parse the input params:
	int opt = 0;
	int mod_index = 0, leve = 0, store_flag = 0;
	while ((opt = getopt(argc, argv, "rhc:m:")) != -1) 
	{
		switch (opt)
		{
			case 'r':
				printf("reload the slogd's config file\n");
				slog_ctl(SLOG_TYPE_CFG_RELOAD, 1, 1, "");
				return 0;
				break;

			case 'h':
				usage(argv[0]);
				return 0;
				break;

			case 'c':
				strncpy(cfg_name, optarg, sizeof(cfg_name) - 1);
				printf("cfg_name:%s\n", cfg_name);
				break;

			case 'm':
				sscanf(optarg, "%d %d", &mod_index, &leve);
				printf("config the mod: %s  to %d \n", pmode2str[mod_index], pleve2str[leve]);
				slog_ctl(SLOG_TYPE_CONTROL_MOD, mod_index, leve, "");
				return 0;
				break;

			default:
				usage(argv[0]);
				return -1;
		}
	}

	//make sure only one runnning...
	if(is_running(cfg_name))
	{
		printf("the %s is running...\n", argv[0]);
		return -1;
	}

	int msqid = getQid();
	if (msqid == -1)
	{
		printf("create the msqueue failed.\n");
		return -1;
	}
	set_msgqueue_maxbytes(msqid, 1024 * 1024);


	//init the cfg from config file.
	if(init_cfg(cfg_name, &g_slogd_cfg) != 0)
	{
		printf("load config file failed.\n");
		return -1;
	}

	memset(&sunx, 0, sizeof(sunx));
	sunx.sun_family = AF_UNIX;

	strcpy(sunx.sun_path, SLOG_PATH_SOCKET);
	

	sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	int flag = 1;
	setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

	

	unlink(sunx.sun_path);
	bind(sock_fd, (struct sockaddr *) &sunx, sizeof(sunx));
	chmod(SLOG_PATH_SOCKET, 0666);


	//start the write log thread:
	start_writelog(&g_slogd_cfg);
	
	//start the kernel log thread:
	start_kernellog(&g_slogd_cfg);


	struct pollfd pfd[1];
	int timeout = 10*1000;//ms
	int bytes_read = 0;
	char log_buf[LOG_BUF_MAX_LEN] = {0};
	int cfd = -1;
	
	while (1)
	{
		pfd[0].fd = sock_fd;
		pfd[0].events = POLLIN;
		memset(log_buf, 0, LOG_BUF_MAX_LEN);
		switch (poll(pfd, 1, timeout)) 
		{
			case -1:
				if (errno != EINTR)
					printf("[%s %d] error:%s\n",__func__,__LINE__, strerror(errno));
				break;
			case 0:
				//printf("[%s %d] timeout\n",__func__,__LINE__);
				break;
			default:
				bytes_read = recvfrom(pfd[0].fd, log_buf, LOG_BUF_MAX_LEN - 1, 0, NULL, NULL);
				if ( bytes_read <= 0 )
				{
					if ( (EAGAIN == errno) || (EINTR == errno))
					{
						perror("UnixRead read error:");
					}
				}
				else
				{
					slog_header_s tmp_header = {0};//slogd will use it to filter the log message...
					int headlen = sizeof(tmp_header);
					memcpy(&tmp_header, log_buf, headlen);

					switch (tmp_header.slog_type)
					{
						case SLOG_TYPE_LOG:
							if (g_slogd_cfg.mod_entry[tmp_header.mod_index].debug_leve >=  tmp_header.debug_level)
							{
								log_msgbuf_s msgbuf;
								msgbuf.mtype =  tmp_header.mod_index + 1; //the msgtype must bigger than zero!!!
								msgbuf.len = strlen((log_buf+headlen)) + 1;//include the last char '\0';
								msgbuf.pbuff = (char *)calloc(msgbuf.len, sizeof(char));
								memcpy(msgbuf.pbuff, (log_buf+headlen), msgbuf.len);
								//printf("[%s %d] mod_index=%d, level=%d,msgbuf.buff(len=%d):%s",__func__,__LINE__, tmp_header.mod_index, tmp_header.debug_level, msgbuf.len, msgbuf.pbuff);
								if (sendMsg(msqid, &msgbuf, 0) == -1)
								{
									perror("sendMsg failed:");
									dump_msgqueue_stat(msqid);
									//free the msg buffer
									free(msgbuf.pbuff);
								}
							}
							//else
							//	printf("[%s %d] drop!!! mod_index=%d, level=%d,text:%s",__func__,__LINE__, tmp_header.mod_index, tmp_header.debug_level,(log_buf+headlen));
							break;
						case SLOG_TYPE_CONTROL_MOD:
							g_slogd_cfg.mod_entry[tmp_header.mod_index].debug_leve =  tmp_header.debug_level;
							printf("[%s %d] controll message mod_index=%d, level=%d,text:%s",__func__,__LINE__, tmp_header.mod_index, tmp_header.debug_level,(log_buf+headlen));
							break;

						case SLOG_TYPE_CFG_RELOAD:
							if(init_cfg(cfg_name, &g_slogd_cfg) != 0)
							{
								printf("load config file failed.\n");
							}
							break;

						default:
							printf("didn't support the log type:%d\n",tmp_header.slog_type);
							break;
					}
				}
		}
		
	}
	unlink(KEY_ID_PATH);
	return 0;
}
