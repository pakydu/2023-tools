/*
 * simple logd for the project log modules.
 *
 */

#include "msgqueue.h"

int getQid()
{
	int msqid = -1;
	
	FILE * fp = fopen(KEY_ID_PATH, "a");
	if (fp != NULL)
		fclose(fp);
    key_t key=ftok(KEY_ID_PATH, KEY_ID_PRJ_ID);
	if(key==-1)
	{
		perror("create the key failed.");
		return -1;
	}

	msqid = msgget(key, IPC_CREAT|0666);
	if(msqid==-1)
	{
		perror("create the qid failed");
		return -1;
	}

	return msqid;
}

#if 0  //referenc
/*
 *查看、设置、删除消息队列
 *msqid:消息队列的标识符id
 *cmd:对消息队列执行的操作,IPC_STAT,IPC_SET,IPC_RMID
 *buf:描述指定消息队列的结构体, 
 */
int msgctl(int msqid, int cmd, struct msqid_ds *buf);

/*
 *描述消息队列的结构体
 */
struct msqid_ds {
	struct ipc_perm msg_perm;     /* Ownership and permissions */
	time_t          msg_stime;    /* 最后发送消息时间 */
	time_t          msg_rtime;    /* 最后接收消息时间 */
	time_t          msg_ctime;    /* 最后修改时间 */
	unsigned long   __msg_cbytes; /* 当前使用消息队列空间大小 (nonstandard) */
	msgqnum_t       msg_qnum;     /* 当前消息个数 */
	msglen_t        msg_qbytes;   /* 消息队列最大长度 */
    pid_t           msg_lspid;    /* 最后发送消息的PID */
    pid_t           msg_lrpid;    /* 最后接收消息的PID */
};
#endif

void dump_msgqueue_stat(int msqid)
{
	struct msqid_ds msq_buf = {0};
	msgctl(msqid, IPC_STAT, &msq_buf);
	printf("msq current attr: __msg_cbytes:%ld, msg_qnum:%d, msg_qbytes=%d\n", msq_buf.__msg_cbytes, msq_buf.msg_qnum, msq_buf.msg_qbytes);

}

void set_msgqueue_maxbytes(int msqid, long max_bytes)
{
	struct msqid_ds msq_buf = {0};
	msgctl(msqid, IPC_STAT, &msq_buf);
	printf("msq old attr: __msg_cbytes:%ld, msg_qnum:%d, msg_qbytes=%d\n", msq_buf.__msg_cbytes, msq_buf.msg_qnum, msq_buf.msg_qbytes);

	//change the max size
	msq_buf.msg_qbytes = max_bytes;//1024 * 1024;
	msgctl(msqid, IPC_SET, &msq_buf);

	msgctl(msqid, IPC_STAT, &msq_buf);
	printf("msq new attr: __msg_cbytes:%ld, msg_qnum:%d, msg_qbytes=%d\n", msq_buf.__msg_cbytes, msq_buf.msg_qnum, msq_buf.msg_qbytes);

}
int sendMsg(int msqid, log_msgbuf_s *pMsg, int block)
{
	int msgflg = IPC_NOWAIT;
	if (block)
		msgflg = 0;
	return msgsnd(msqid, pMsg, sizeof(log_msgbuf_s)- sizeof(pMsg->mtype), msgflg);
}

int rcvMsg(int msqid, log_msgbuf_s *pMsg, int block)
{
	int msgflg = IPC_NOWAIT;
	if (block)
		msgflg = 0;
	int ret = msgrcv(msqid, pMsg, sizeof(log_msgbuf_s) - sizeof(pMsg->mtype), 0, msgflg);
	if (ret == -1)
	{
		perror("msgrcv error:");
	}
	return ret;
}
