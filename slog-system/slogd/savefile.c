/*
 * simple logd for the project log modules.
 *
 */

#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <signal.h>
#include <sys/klog.h>
#include <poll.h>
#include <syslog.h>
#include "libslog.h"
#include "msgqueue.h"

//static pthread_mutex_t output_lock;

static pthread_t wlog_tid;
//static char file_name[128] =  DEFAULT_PATH_FILE_LOG;

static int copyfile(int old_fd, int new_fd)
{
	int ret = 0;
	int rd_len = 0;
	int wt_len = 0;
	char log_buf[LOG_BUF_MAX_LEN] = {0};
	char *ptr = NULL;

	lseek(old_fd, 0L, SEEK_SET);
	lseek(new_fd, 0L, SEEK_SET);
	
	while(rd_len = read(old_fd, log_buf, LOG_BUF_MAX_LEN))
	{
		if((rd_len == -1) && (errno != EINTR))
			break;
		else if(rd_len>0)
		{
			ptr=log_buf;
			while(wt_len = write(new_fd, ptr, rd_len))
			{
				if((wt_len == -1) && (errno != EINTR))
					break;
				else if(wt_len == rd_len)
					break;//go to the next new buffer.
				else if( wt_len > 0)
				{
					ptr += wt_len;
					rd_len -= wt_len;
				}
			}
			if(wt_len == -1)
			{
				ret = -1;
				break;//OOP...
			}
		}
	}

	return ret;
}

static int checkfilesize(int *pfd, long int max_size, char * file_name)
{
	long size = lseek(*pfd, 0, SEEK_END);
	if (size > max_size)
	{
		char new_file[256] = {0};
		//The fast solution for rollback file, but it will not be nice for end-user who use the tail -f filename to monitor the log file.
		{
			close(*pfd);
			sprintf(new_file, "%s.bk",file_name);
			rename(file_name, new_file); 
			*pfd = open(file_name,O_CREAT|O_APPEND|O_RDWR,0644);
			sync();//fsync(fd);
		}
		
		//solution 2, copy the old file to backup, and empty the old file. It still keep the old file I-node, so end-user can use "tail -f filename" to monitor the log file
		// {
		// 	sprintf(new_file, "%s.bk",file_name);
		// 	int new_fd = open(new_file,O_CREAT|O_APPEND|O_RDWR,0644);
		// 	if (new_fd != -1)
		// 	{
		// 		copyfile(*pfd, new_fd);
		// 		close(new_fd);
		// 	}
		// 	//maybe we should check the return.
		// 	int ret = ftruncate(*pfd, 0);
		// 	if (ret == -1)
		// 	{
		// 		printf("ftruncate failedd. %s\n", strerror(errno));
		// 		exit(0);
		// 	}
		// 	lseek(*pfd, 0L, SEEK_SET);//rewind(fp);
		// 	fsync(*pfd);
		// }
	}
	
}

static void writefile(int *pfd, const char *msg, int len, char *file_name, long int file_max_size)
{
	static int cnt = 100;
	int wt_len = 0;
	const char *pwt = msg;
	while((wt_len = write(*pfd, pwt, len))!=0)
	{
		if((wt_len == -1)&&(errno!=EINTR))
			break;
		else if(wt_len == len)
			break;
		else if(wt_len > 0)
		{
			len -= wt_len;
			pwt += wt_len;
		}
	}
	//printf("[%s %d] save to file(len=%d):%s",__func__,__LINE__, len, msg);

	if (cnt-- == 0)
	{
		cnt = 100;
		checkfilesize(pfd, file_max_size, file_name);
		fsync(*pfd);
	}

}

static void writelog_fun(void * data)
{
	slogd_cfg_s *pCfg = (slogd_cfg_s *)data;
	prctl(PR_SET_NAME, "writelogfile");
	int msqid = getQid();
	if (msqid == -1)
	{
		printf("create the msqueue failed.[%s %d]\n", __func__,__LINE__);
		return;
	}

	int fd = open(pCfg->slog_file, O_CREAT|O_APPEND|O_RDWR,0644);
	if (fd < 0)
	{
		fprintf(stdout," ****** open file(%s) fail:%s  ***** \n", pCfg->slog_file, strerror(errno));
		return ;
	}

	while (1)
	{
		log_msgbuf_s msgbuf;
		if(rcvMsg(msqid, &msgbuf, 1) > 0)
		{
#ifdef SUPPORT_SYSLOG
			if(pCfg->send2syslog == 1)  //send to syslogd
			{
				//syslog();
			}
#endif
			if((pCfg->mod_entry[msgbuf.mtype - 1]).store_flag)  //save to file:
			{
				//max_size = pCfg->file_max_size * 1024;
				writefile(&fd, msgbuf.pbuff, msgbuf.len, pCfg->slog_file, pCfg->file_max_size * 1024);
			}
			//else
			//	printf("debug:%s",__func__,__LINE__, msgbuf.pbuff);
#ifdef SUPPORT_SENT2REMOTE
			if(strlen(pCfg->.send2remote_flag) > 0)  //send to remoteIP:
			{
				//sendto();
			}
#endif
			free(msgbuf.pbuff);
		}
		else
		{
			printf("Bye the thread...\n");
			//break;
		}
	}

	close(fd);
}


//for dump kernel log to file:
static void klogd_open(void)
{
	/* "Open the log. Currently a NOP" */
	klogctl(1, NULL, 0);
}

static void klogd_setloglevel(int lvl)
{
	/* "printk() prints a message on the console only if it has a loglevel
	 * less than console_loglevel". Here we set console_loglevel = lvl. */
	klogctl(8, NULL, lvl);
}

static int klogd_read(char *bufp, int len)
{
	return klogctl(2, bufp, len);
}
# define READ_ERROR "klogctl(2) error"

static void klogd_close(void)
{
	/* FYI: cmd 7 is equivalent to setting console_loglevel to 7
	 * via klogctl(8, NULL, 7). */
	klogctl(7, NULL, 0); /* "7 -- Enable printk's to console" */
	klogctl(0, NULL, 0); /* "0 -- Close the log. Currently a NOP" */
}
void overlapping_strcpy(char *dst, const char *src)
{
	/* Cheap optimization for dst == src case -
	 * better to have it here than in many callers.
	 */
	if (dst != src) {
		while ((*dst = *src) != '\0') {
			dst++;
			src++;
		}
	}
}

//the function refers to klogd.
static void kernellog_fun(void * data)
{
	slogd_cfg_s *pCfg = (slogd_cfg_s *)data;
	// prctl(PR_SET_NAME, "kernellogfile");
	// FILE * fp = open("/proc/tc3162/dbg_msg", O_RDONLY);//fopen("/dev/kmsg", "r");
	// //int fd = fileno(fp);
	// //struct pollfd pfd[1];
	// //int timeout = 10*1000;//ms
	// int bytes_read = 0;
	char log_buffer[LOG_BUF_MAX_LEN] = {0};
	// int cfd = -1;
	int used = 0;
	klogd_open();
	openlog("kernel", 0, LOG_KERN);
	
	while (1)
	{
		int n;
		int priority;
		char *start;

		/* "2 -- Read from the log." */
		start = log_buffer + used;
		n = klogd_read(start, LOG_BUF_MAX_LEN-1 - used);
		if (n < 0) {
			if (errno == EINTR)
				continue;
			perror("read failed:");
			break;
		}
		start[n] = '\0';

		/* Process each newline-terminated line in the buffer */
		start = log_buffer;
		while (1) {
			char *newline = strchrnul(start, '\n');

			if (*newline == '\0') {
				/* This line is incomplete */

				/* move it to the front of the buffer */
				overlapping_strcpy(log_buffer, start);
				used = newline - start;
				if (used < LOG_BUF_MAX_LEN-1) {
					/* buffer isn't full */
					break;
				}
				/* buffer is full, log it anyway */
				used = 0;
				newline = NULL;
			} else {
				*newline++ = '\0';
			}

			/* Extract the priority */
			//printf("LOG_INFO=%d, SLOG_LEVEL_INFO=%d, LOG_EMERG=%d\n", LOG_INFO, SLOG_LEVEL_INFO, LOG_EMERG);
			if (*start == '<') {
				start++;
				if (*start)
					priority = strtoul(start, &start, 10);
				if (*start == '>')
					start++;
			}
			/* Log (only non-empty lines) */
			if (start && pCfg->mod_entry[SLOG_MOD_KERN].store_flag == 1)
				slog_printf(SLOG_MOD_KERN, priority, "%s", start);

			if (!newline)
				break;
			start = newline;
		}
		
		
	}
	klogd_close();
}


static void start_thread(void * fun, void * data)
{
	
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED); 
	if(pthread_create(&wlog_tid, &attr, (void *)fun, data) != 0){
		fprintf(stderr, "pthread_create call_boot error!!\n");
		pthread_attr_destroy(&attr);
		exit(0);
	}
}

void start_writelog(void * data)
{
	start_thread(writelog_fun, data);
}

void start_kernellog(void * data)
{
	start_thread(kernellog_fun, data);
}
