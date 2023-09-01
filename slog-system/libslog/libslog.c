/*
 * simple logd for the project log modules.
 *
 */

#include <sys/file.h>
#include <unistd.h>
#include <pthread.h>
#include "libslog.h"


//local static val:
static int client_fd = -1;
static struct sockaddr_un clAddr = {
	.sun_family = AF_UNIX,
	.sun_path[0]=0,
};

static struct sockaddr_un slogdAddr = {
	.sun_family = AF_UNIX,
	.sun_path = SLOG_PATH_SOCKET,
};

static socklen_t socklen  = sizeof(slogdAddr);
static char g_mod_name[32] = {0};
pid_t pid = -1;

static pthread_once_t once = PTHREAD_ONCE_INIT;
static int init_once()
{
	//just do once for the caller
	//init the globvals...
	prctl(PR_GET_NAME, g_mod_name);
	pid = getpid();
	client_fd=socket(AF_UNIX, SOCK_DGRAM, 0);

	return 0;
}

static void get_current_time(char *buf)
{
	time_t timer = time(NULL);;
	struct tm tmf;
	localtime_r(&timer, &tmf);
	sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", tmf.tm_year + 1900, tmf.tm_mon + 1, tmf.tm_mday,
            tmf.tm_hour, tmf.tm_min, tmf.tm_sec);
}


static int send2slogd(char * buf, int len)
{
	
	
	if(client_fd < 0)
	{
		printf("socket failure:%s\n",strerror(errno));
	}

	//memset(&clientAddr,0,sizeof(struct sockaddr_un));
	//clientAddr.sun_family = AF_UNIX;
	//clientAddr.sun_path[0]=0;
	//char tmpPath[] = "/tmp/unix_XXXX";
	//strcpy(clientAddr.sun_path, tmpPath);
	
	//snprintf(clientAddr.sun_path+1,strlen(SLOG_PATH_SOCKET)+1,"%s",SLOG_PATH_SOCKET);
	// if(connect(client_fd,(struct sockaddr *)&clientAddr,sizeof(struct sockaddr_un)) < 0)
	// {
	// 	printf("connect failure:%s\n",strerror(errno));
	// 	return -2;
	// }

	// int rv=write(client_fd,buf, len);
	

	int rv = sendto(client_fd, buf, len, 0, (void *)&slogdAddr, socklen);//just send, didn't connect the slogd.
	if(rv <= 0)
	{
		printf("write failure:%s\n",strerror(errno));
		return -3;
	}

	return 0;
}

void slog_printf (SLOG_MOD_NAME_E mod_index, SLOG_LEVEL_E level, const char *format,...)
{
	int ret = 0;
	int len = 0;
	//int write_level = 0;
	unsigned int headlen = 0;
	unsigned int totallen = 0;
	char time_str[32] = {0};
	char tmpbuf[LOG_BUF_MAX_LEN] = {0};

	va_list ptr ;
	char *ptmp = tmpbuf;
	
	if(NULL == format)
	{
		return ;
	}

	//init the glob-vals
	pthread_once(&once, (void *)init_once);

	// if (SLOG_MOD_MAX > mod_index)
	// {
	// 	strcpy(g_mod_name, pmode2str[mod_index]);
	// }

	// if (level >= SLOG_LEVEL_MAX)
	// 	level = SLOG_LEVEL_MAX -1;

	get_current_time(time_str);
	//pid_t pid = getpid();
	//pid_t tid = gettid();

	//fill the header and perfix
	slog_header_s tmp_header = {0};//slogd will use it to filter the log message...
	tmp_header.slog_type = SLOG_TYPE_LOG;//log message.
	tmp_header.mod_index = mod_index;
	tmp_header.debug_level = level;
	headlen = sizeof(tmp_header);
	memcpy((char *)tmpbuf, &tmp_header, headlen);//install the header
	headlen += sprintf((char *)(tmpbuf+headlen), "%s %10s[%8d][%8s]: ", time_str, 
		(SLOG_MOD_MAX > mod_index)? pmode2str[mod_index]: g_mod_name, 
		pid, 
		pleve2str[level]);//install the perfix
	ptmp += headlen;
	
	va_start(ptr, format);
	len = vsnprintf(ptmp, (sizeof(tmpbuf) - headlen - 1), format, ptr);//keep the last char to '\n'
	if (len < 0)
	{
		printf(" vsnprintf error.\n");
		va_end(ptr);
		return ;
	}
	va_end(ptr);

	tmpbuf[LOG_BUF_MAX_LEN - 1] = '\0';
	totallen = strlen(ptmp) + headlen;
	tmpbuf[totallen++] = '\n';

	//sendto slogd:
	ret = send2slogd(tmpbuf, totallen);
	if(0 != ret)
	{
		perror(" send error.\n");
		return ;
	}
	
	return ;
}

void slog_ctl(slogtype_e type, SLOG_MOD_NAME_E mod_index, SLOG_LEVEL_E level, const char *format,...)
{
	int ret = 0;
	int len = 0;
	//int write_level = 0;
	unsigned int headlen = 0;
	unsigned int totallen = 0;
	char time_str[32] = {0};
	char tmpbuf[LOG_BUF_MAX_LEN] = {0};

	va_list ptr ;
	char *ptmp = tmpbuf;
	
	if(NULL == format)
	{
		return ;
	}

	//init the glob-vals
	pthread_once(&once, (void *)init_once);

	//fill the header and perfix
	slog_header_s tmp_header = {0};//slogd will use it to filter the log message...
	tmp_header.slog_type = type;// message.
	tmp_header.mod_index = mod_index;
	tmp_header.debug_level = level;
	headlen = sizeof(tmp_header);
	memcpy((char *)tmpbuf, &tmp_header, headlen);//install the header
	ptmp += headlen;
	
	va_start(ptr, format);
	len = vsnprintf(ptmp, (sizeof(tmpbuf) - headlen - 1), format, ptr);//keep the last char to '\n'
	if (len < 0)
	{
		printf(" vsnprintf error.\n");
		va_end(ptr);
		return ;
	}
	va_end(ptr);

	tmpbuf[LOG_BUF_MAX_LEN - 1] = '\0';
	totallen = strlen(ptmp) + headlen;
	tmpbuf[totallen++] = '\n';

	//sendto slogd:
	ret = send2slogd(tmpbuf, totallen);
	if(0 != ret)
	{
		perror(" send error.\n");
		return ;
	}
	
	return ;
}