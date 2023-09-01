/*
 * simple slog for the project log modules.
 *
 */

#ifndef _SLOG_MSGQUEUE_H_
#define _SLOG_MSGQUEUE_H_

#ifdef __cplusplus

extern"C"
{
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>


#define KEY_ID_PATH    "/var/tmp/slogd_keyid"
#define KEY_ID_PRJ_ID    12345

typedef struct _msgbuf
{
	long mtype;//消息类型，必须>0
	int len;
	char *pbuff;
}log_msgbuf_s;

int getQid();
void dump_msgqueue_stat(int msqid);
void set_msgqueue_maxbytes(int msqid, long max_bytes);
int sendMsg(int msqid, log_msgbuf_s *pMsg, int block);
int rcvMsg(int msqid, log_msgbuf_s *pMsg, int block);




#ifdef __cplusplus
}
#endif


#endif  /* _SLOG_MSGQUEUE_H_ */
