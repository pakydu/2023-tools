#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/prctl.h>
#include <ucontext.h>

#include "libsyshelper.h"

// #define BACKTRACE_STORE_DEFAULT_PATH "/var/tmp"
static char  backtrace_store_path[128] = "/var/tmp";

//we will just use the local addr2line in our pc.
// static void output_addrline(char addr[])
// {
// 	char cmd[256];
// 	char line[256];
// 	char addrline[32]={0,};
// 	char *str1, *str2;
// 	FILE* file;
// 	str1 = strchr(addr,'[');
// 	str2 = strchr(addr, ']');
// 	if(str1 == NULL || str2 == NULL)
// 	{
// 		return;
// 	}

// 	memcpy(addrline, str1 + 1, str2 -str1);
// 	snprintf(cmd, sizeof(cmd) - 1, "addr2line -e /proc/%d/exe %s ", getpid(), addrline);
// 	file = popen(cmd, "r");
// 	if(NULL != fgets(line, 256, file))
// 	{
// 		printf("%s\n", line);
// 	}
// 	pclose(file);
// }

/* The maps file looks something like this:
	b667b000-b667c000 rw-p 00000000 00:00 0
	b667c000-b6680000 r-xp 00000000 1f:06 555  /lib/libnl-genl-3.so.200.26.0
	b6680000-b668f000 ---p 00004000 1f:06 555  /lib/libnl-genl-3.so.200.26.0
	b668f000-b6690000 r--p 00003000 1f:06 555  /lib/libnl-genl-3.so.200.26.0
	b6690000-b6691000 rw-p 00004000 1f:06 555  /lib/libnl-genl-3.so.200.26.0
	b6691000-b66aa000 r-xp 00000000 1f:06 545  /lib/libnl-3.so.200.26.0
	b66aa000-b66ba000 ---p 00019000 1f:06 545  /lib/libnl-3.so.200.26.0
	b66ba000-b66bb000 r--p 00019000 1f:06 545  /lib/libnl-3.so.200.26.0
	b66bb000-b66bc000 rw-p 0001a000 1f:06 545  /lib/libnl-3.so.200.26.0
	b66bc000-b66c6000 r-xp 00000000 1f:06 426  /lib/libconfig.so.11.1.0
	b66c6000-b66d5000 ---p 0000a000 1f:06 426  /lib/libconfig.so.11.1.0
	b66d5000-b66d6000 r--p 00009000 1f:06 426  /lib/libconfig.so.11.1.0
	b66d6000-b66d7000 rw-p 0000a000 1f:06 426  /lib/libconfig.so.11.1.0
	b66d7000-b66fa000 r-xp 00000000 1f:06 314  /lib/ld-2.32.so
	b6701000-b6709000 rw-p 00000000 00:00 0
	b6709000-b670a000 r--p 00022000 1f:06 314  /lib/ld-2.32.so
	b670a000-b670b000 rw-p 00023000 1f:06 314  /lib/ld-2.32.so
	be1e7000-be208000 rw-p 00000000 00:00 0    [stack]
	be33a000-be33b000 r-xp 00000000 00:00 0    [sigpage]
	be33b000-be33c000 r--p 00000000 00:00 0    [vvar]
	be33c000-be33d000 r-xp 00000000 00:00 0    [vdso]
	ffff0000-ffff1000 r-xp 00000000 00:00 0    [vectors]
	#
	#
	# cat /proc/13818/task/13818/maps
 */
static int get_stack_range( unsigned int* min_addr, unsigned int* max_addr, pid_t pid, pid_t tid, int fd)
{
	int   nRetVal = -1;
	unsigned int   nAddrMin = 0;
	unsigned int   nAddrMax = 0;
	
	if( (min_addr != (unsigned int*)NULL) && (max_addr != (unsigned int*)NULL) )
	{
		char file_name[128];
		char buf[256] = {0};
		const char*    ptmpKey = "[stack]";
		char *pCur = NULL;
		char *pNext = NULL;


		sprintf(file_name, "/proc/%d/task/%d/maps", pid, tid);
		FILE *fp = fopen(file_name, "r");
		if (fp != NULL)
		{
			while(!feof(fp) && fgets(buf, 256, fp)!=NULL)
			{
				write(fd, "\t", 1); write(fd, buf, strlen(buf));//write to dump file.
				if(strstr(buf, ptmpKey) != NULL)
				{
					// be1e7000-be208000 rw-p 00000000 00:00 0    [stack]
					if ((pCur= strtok_r(buf, "-", &pNext))!= NULL)
					{
						nAddrMin = strtoll(pCur, NULL, 16);
						if ((pCur= strtok_r(pNext, "-", &pNext))!= NULL)
						{
							nAddrMax = strtoll(pCur, NULL, 16);
						}
					}
					//break;
				}
			}
			fclose(fp);
		}
	}

	if( (nAddrMin > 0) && (nAddrMax > 0) )
	{
		*min_addr = nAddrMin;
		*max_addr = nAddrMax;
		nRetVal = 0;
	}
	return( nRetVal);
}

/*
	// # uname -a
	// Linux Inspur-PON-AP 5.4.55 #2 SMP Wed Aug 30 15:10:33 CST 2023 armv7l unknown
	// # kill -l
	// 1) SIGHUP           2) SIGINT           3) SIGQUIT          4) SIGILL
	// 5) SIGTRAP          6) SIGABRT          7) SIGBUS           8) SIGFPE
	// 9) SIGKILL         10) SIGUSR1         11) SIGSEGV         12) SIGUSR2
	// 13) SIGPIPE         14) SIGALRM         15) SIGTERM         16) SIGSTKFLT
	// 17) SIGCHLD         18) SIGCONT         19) SIGSTOP         20) SIGTSTP
	// 21) SIGTTIN         22) SIGTTOU         23) SIGURG          24) SIGXCPU
	// 25) SIGXFSZ         26) SIGVTALRM       27) SIGPROF         28) SIGWINCH
	// 29) SIGIO           30) SIGPWR          31) SIGSYS
	// #
*/

static signal_ending_act_st g_ending_act_arry[] = {
	{ /* 0 */         NULL,       NULL }, //just fill it to match the array index.
	{ /*SIGHUP,*/   NULL,      NULL },
	{ /*SIGINT,*/   NULL,      NULL },
	{ /*SIGQUIT,*/  NULL,      NULL },
	{ /*SIGILL,*/   NULL,      NULL },
	{ /*SIGTRAP,*/  NULL,      NULL },
	{ /*SIGABRT,*/  NULL,      NULL },
	{ /*SIGBUS,*/   NULL,      NULL },
	{ /*SIGFPE,*/   NULL,      NULL },
	{ /*SIGKILL,*/  NULL,      NULL },
	{ /*SIGUSR1,*/  NULL,      NULL },
	{ /*SIGSEGV,*/  NULL,      NULL },
	{ /*SIGUSR2,*/  NULL,      NULL },
	{ /*SIGPIPE,*/  NULL,      NULL },
	{ /*SIGALRM,*/  NULL,      NULL },
	{ /*SIGTERM,*/  NULL,      NULL },
	{ /*SIGSTKFLT,*/NULL,      NULL },
	{ /*SIGCHLD,*/  NULL,      NULL },
	{ /*SIGCONT,*/  NULL,      NULL },
	{ /*SIGSTOP,*/  NULL,      NULL },
	{ /*SIGTSTP,*/  NULL,      NULL },
	{ /*SIGTTIN,*/  NULL,      NULL },
	{ /*SIGTTOU,*/  NULL,      NULL },
	{ /*SIGURG,*/   NULL,      NULL },
	{ /*SIGXCPU,*/  NULL,      NULL },
	{ /*SIGXFSZ,*/  NULL,      NULL },
	{ /*SIGVTALRM,*/NULL,      NULL },
	{ /*SIGPROF,*/  NULL,      NULL },
	{ /*SIGWINCH,*/ NULL,      NULL },
	{ /*SIGIO,*/    NULL,      NULL },
	{ /*SIGSYS,*/    NULL,      NULL },
};
// void dump_stack(char *sig, int fd)
// {
// 	void* stackArray[100];
// 	int stackSize = backtrace(stackArray, sizeof(stackArray) / sizeof(void*));
// 	#if 1
// 	char** stackSymbols = backtrace_symbols(stackArray, stackSize);
// 	if (stackSymbols != NULL)
// 	{
// 		printf("Program stack trace(size=%d):\n", stackSize);
// 		for (int i = 0; i < stackSize; i++)
// 		{
// 			printf("%s\n", stackSymbols[i]);
// 		}
// 		free(stackSymbols);
// 	}
// 	#else
// 	char file_name[128] = {0};
// 	//sprintf(file_name,"bt_%d_%d_%s.log", getpid(), gettid(), sig);
// 	//int fd = open(file_name, O_CREAT|O_RDWR, 0666);
// 	backtrace_symbols_fd(stackArray, stackSize, fd);
// 	//close(fd);
// 	#endif
// }

static void dump_memory(int fd, unsigned int start, unsigned int len)
{
	if ((start != (unsigned int*)NULL) && (len < 8*1024*1024)) //8M
	{
		char* pCur = (char*)start;
		char* pEnd = pCur + len;

		for (int i = 0; i < len; i++)
		{
			char cByte;
			if (pCur < pEnd)
			{
				if( (*pCur > 0x1F) && (*pCur < 0x7F) )
				{
					cByte = *pCur;
				}
				else
				{
					cByte = '.';//maybe use hex format.
				}
			}
			else
			{
				cByte = ' ';
			}
			write( fd, &cByte, 1 );
			if (i%16)
			{
				write(fd, "\n", 1);
			}
			pCur++;
		}
		write(fd, "\n", 1);
		
	}
	else
	{
		char buf[128] = {0};
		snprintf(buf, sizeof(buf)- 1, "Cannot dump buffer. Too many bytes: %lu\n", len); write(fd, buf, strlen(buf));
	}
}

/*
./buildroot-gcc1030-glibc232_kernel5_4/arm-buildroot-linux-gnueabi/sysroot/usr/include/sys/ucontext.h:
	typedef struct
	{
		unsigned long int __ctx(trap_no);
		unsigned long int __ctx(error_code);
		unsigned long int __ctx(oldmask);
		unsigned long int __ctx(arm_r0);
		unsigned long int __ctx(arm_r1);
		unsigned long int __ctx(arm_r2);
		unsigned long int __ctx(arm_r3);
		unsigned long int __ctx(arm_r4);
		unsigned long int __ctx(arm_r5);
		unsigned long int __ctx(arm_r6);
		unsigned long int __ctx(arm_r7);
		unsigned long int __ctx(arm_r8);
		unsigned long int __ctx(arm_r9);
		unsigned long int __ctx(arm_r10);
		unsigned long int __ctx(arm_fp);
		unsigned long int __ctx(arm_ip);
		unsigned long int __ctx(arm_sp);
		unsigned long int __ctx(arm_lr);
		unsigned long int __ctx(arm_pc);
		unsigned long int __ctx(arm_cpsr);
		unsigned long int __ctx(fault_address);
	} mcontext_t;

	//Userlevel context.
	typedef struct ucontext_t
	{
		unsigned long __ctx(uc_flags);
		struct ucontext_t *uc_link;
		stack_t uc_stack;
		mcontext_t uc_mcontext;
		sigset_t uc_sigmask;
		unsigned long __ctx(uc_regspace)[128] __attribute__((__aligned__(8)));
	} ucontext_t;
*/
static void dump_stack(int signo, siginfo_t *info, void *data)
{
	if( (info->si_signo != 15) || (info->si_code != 0) )
	{
		struct sigaction sigdef;
		memset(&sigdef, 0, sizeof(sigdef));
		sigaction(signo, &sigdef, NULL);//reset

		ucontext_t* ctx = (ucontext_t *)data;//for arm platform.
		unsigned int min_addr = 0;
		unsigned int max_addr = 0;
		int fd = -1;
		char file_name[128] = {0};
		char process_name[32] = {0};
		char thread_name[32] = {0};
		char buf[256] = {0};
		pid_t pid = getpid();
		pid_t tid = gettid();
		char time_str[32] = {0};

		//fill the dump file name:
		//prctl(PR_GET_NAME, process_name);
		get_threadname_bypid(pid, pid, process_name);
		get_threadname_bypid(pid, tid, thread_name);
		get_current_time2str(time_str);
		sprintf(file_name, "%s/bt_%s_%d_%d_%d_%s.bt", backtrace_store_path, process_name, pid, tid, signo, time_str);
		slog_printf(0, 0, "catched the sigal: %d, dump backtrace to %s", signo, file_name);

		fd = open(file_name, O_CREAT | O_WRONLY | O_SYNC, 0666);
		if( fd != -1 )
		{
			//dump the header information:
			snprintf(buf, sizeof(buf)- 1, "%s catched the signal: %d (%s) at %s\n", process_name, signo, strsignal(signo), time_str); write(fd, buf, strlen(buf));
			snprintf(buf, sizeof(buf)- 1, "\t Process(%s) ID: %d\n", process_name, pid); write(fd, buf, strlen(buf));
			snprintf(buf, sizeof(buf)- 1, "\t Thread(%s)  ID: %d\n", thread_name, tid); write(fd, buf, strlen(buf));
			write(fd, "\n", 1);

			// dump singal information
			snprintf(buf, sizeof(buf)- 1, "Signal information: \n"); write(fd, buf, strlen(buf));
			snprintf(buf, sizeof(buf)- 1, "\t siginfo->si_signo=%d, siginfo->si_code=%d, siginfo->si_errno=%d\n", info->si_signo, info->si_code, info->si_errno); write(fd, buf, strlen(buf));
			write(fd, "\n", 1);

			//dump regist information:
			snprintf(buf, sizeof(buf)- 1, "Register Context:\n"); write(fd, buf, strlen(buf));
			snprintf(buf, sizeof(buf)- 1, "\t trap_no:    0x%x\n", ctx->uc_mcontext.trap_no); write(fd, buf, strlen(buf));
			snprintf(buf, sizeof(buf)- 1, "\t error_code: 0x%x\n", ctx->uc_mcontext.error_code); write(fd, buf, strlen(buf));
			snprintf(buf, sizeof(buf)- 1, "\t oldmask:    0x%x\n ", ctx->uc_mcontext.oldmask); write(fd, buf, strlen(buf));
			snprintf(buf, sizeof(buf)- 1, "\t r0:         0x%x\n ", ctx->uc_mcontext.arm_r0); write(fd, buf, strlen(buf));
			snprintf(buf, sizeof(buf)- 1, "\t r1:         0x%x\n", ctx->uc_mcontext.arm_r1); write(fd, buf, strlen(buf));
			snprintf(buf, sizeof(buf)- 1, "\t r2:         0x%x\n", ctx->uc_mcontext.arm_r2); write(fd, buf, strlen(buf));
			snprintf(buf, sizeof(buf)- 1, "\t r3:         0x%x\n", ctx->uc_mcontext.arm_r3); write(fd, buf, strlen(buf));
			snprintf(buf, sizeof(buf)- 1, "\t r4:         0x%x\n", ctx->uc_mcontext.arm_r4); write(fd, buf, strlen(buf));
			snprintf(buf, sizeof(buf)- 1, "\t r5:         0x%x\n", ctx->uc_mcontext.arm_r5); write(fd, buf, strlen(buf));
			snprintf(buf, sizeof(buf)- 1, "\t r6:         0x%x\n", ctx->uc_mcontext.arm_r6); write(fd, buf, strlen(buf));
			snprintf(buf, sizeof(buf)- 1, "\t r7:         0x%x\n", ctx->uc_mcontext.arm_r7); write(fd, buf, strlen(buf));
			snprintf(buf, sizeof(buf)- 1, "\t r8:         0x%x\n", ctx->uc_mcontext.arm_r8); write(fd, buf, strlen(buf));
			snprintf(buf, sizeof(buf)- 1, "\t r9:         0x%x\n", ctx->uc_mcontext.arm_r9); write(fd, buf, strlen(buf));
			snprintf(buf, sizeof(buf)- 1, "\t r10:        0x%x\n", ctx->uc_mcontext.arm_r10); write(fd, buf, strlen(buf));
			snprintf(buf, sizeof(buf)- 1, "\t fp:         0x%x\n", ctx->uc_mcontext.arm_fp); write(fd, buf, strlen(buf));
			snprintf(buf, sizeof(buf)- 1, "\t ip:         0x%x\n", ctx->uc_mcontext.arm_ip); write(fd, buf, strlen(buf));
			snprintf(buf, sizeof(buf)- 1, "\t sp:         0x%x\n", ctx->uc_mcontext.arm_sp); write(fd, buf, strlen(buf));
			snprintf(buf, sizeof(buf)- 1, "\t lr:         0x%x\n", ctx->uc_mcontext.arm_lr); write(fd, buf, strlen(buf));
			snprintf(buf, sizeof(buf)- 1, "\t pc:         0x%x\n", ctx->uc_mcontext.arm_pc); write(fd, buf, strlen(buf));
			snprintf(buf, sizeof(buf)- 1, "\t cpsr:       0x%x\n", ctx->uc_mcontext.arm_cpsr); write(fd, buf, strlen(buf));
			snprintf(buf, sizeof(buf)- 1, "\t fault_address: 0x%x\n ", ctx->uc_mcontext.fault_address); write(fd, buf, strlen(buf));
			write(fd, "\n", 1);

			// dump backtrace
			void* pTrace[32];
			int len = (sizeof(pTrace))/(sizeof(pTrace[0]));
			char**   pSymbols = NULL;
			memset((char*)pTrace, 0, sizeof(pTrace) );
			len = backtrace( pTrace, len );
			snprintf(buf, sizeof(buf)- 1, "Backtrace(dep=%d)\n", len); write(fd, buf, strlen(buf));

			if( len > 0 )
			{
				pSymbols = backtrace_symbols( pTrace, len );
				if( pSymbols != (char**)NULL )
				{
					for( int i = 0; i < len; i++ )
					{
						const char* pszSymbolCur = pSymbols[i];

						if( pszSymbolCur != (char*)NULL )
						{
							snprintf(buf, sizeof(buf)- 1, "\t 0x%016x - %s\n", (unsigned int)(pTrace[i]), pszSymbolCur); write(fd, buf, strlen(buf));
						}
						else
						{
							snprintf(buf, sizeof(buf)- 1, "\t NULL - NULL\n"); write(fd, buf, strlen(buf));
						}
					}
					free(pSymbols);
					pSymbols = NULL;
				}
			}
			write(fd, "\n", 1);

			// dump maps and stack memory range:
			snprintf(buf, sizeof(buf)- 1, "dump the process maps:\n"); write(fd, buf, strlen(buf));

			if( get_stack_range( &min_addr, &max_addr, pid, tid, fd) == 0 )
			{
				unsigned int   sp_addr = ctx->uc_mcontext.arm_sp;
				unsigned int*  pStack_start = (unsigned int*)sp_addr;
				unsigned int len = max_addr - sp_addr;
				//try to dump the stack ram data:
				// dump_memory(fd, pStack_start, len);
			}
			else
			{
				snprintf(buf, sizeof(buf)- 1, "\t ERROR: unable to dump stack - could not get stack \n"); write(fd, buf, strlen(buf));
			}
			close(fd);
		}

		if (g_ending_act_arry[signo].cb != NULL)
		{
			g_ending_act_arry[signo].cb(g_ending_act_arry[signo].data);
		}
		kill( pid, info->si_signo );
		// kill(pid, SIGINT);
		//in the end, check the old file, just keep the last 5 files: ls -t /var/tmp/*.bt | sed -n '6,$p' | awk '{system("rm "$1)}'
		//char cmd_buf[256] = {0};
		//sprintf(cmd_buf, "ls -t %s/bt_*.bt | sed -n '6,$p' | awk '{system(\"rm \"$1)}'", backtrace_store_path);
		//system(cmd_buf);
	}
	else
	{

	}
}

/*
// SIGABRT  由调用abort函数产生，进程非正常退出
// SIGALRM  用alarm函数设置的timer超时或setitimer函数设置的interval timer超时
// SIGBUS  某种特定的硬件异常，通常由内存访问引起
// SIGCANCEL  由Solaris Thread Library内部使用，通常不会使用
// SIGCHLD  进程Terminate或Stop的时候，SIGCHLD会发送给它的父进程。缺省情况下该Signal会被忽略
// SIGCONT  当被stop的进程恢复运行的时候，自动发送
// SIGEMT  和实现相关的硬件异常
// SIGFPE  数学相关的异常，如被0除，浮点溢出，等等
// SIGFREEZE  Solaris专用，Hiberate或者Suspended时候发送
// SIGHUP  发送给具有Terminal的Controlling Process，当terminal被disconnect时候发送
// SIGILL  非法指令异常
// SIGINFO  BSD signal。由Status Key产生，通常是CTRL+T。发送给所有Foreground Group的进程
// SIGINT  由Interrupt Key产生，通常是CTRL+C或者DELETE。发送给所有ForeGround Group的进程
// SIGIO  异步IO事件
// SIGIOT  实现相关的硬件异常，一般对应SIGABRT
// SIGKILL  无法处理和忽略。中止某个进程
// SIGLWP  由Solaris Thread Libray内部使用
// SIGPIPE  在reader中止之后写Pipe的时候发送
// SIGPOLL  当某个事件发送给Pollable Device的时候发送
// SIGPROF  Setitimer指定的Profiling Interval Timer所产生
// SIGPWR  和系统相关。和UPS相关。
// SIGQUIT  输入Quit Key的时候（CTRL+\）发送给所有Foreground Group的进程
// SIGSEGV  非法内存访问
// SIGSTKFLT  Linux专用，数学协处理器的栈异常
// SIGSTOP  中止进程。无法处理和忽略。
// SIGSYS  非法系统调用
// SIGTERM  请求中止进程，kill命令缺省发送
// SIGTHAW  Solaris专用，从Suspend恢复时候发送
// SIGTRAP  实现相关的硬件异常。一般是调试异常
// SIGTSTP  Suspend Key，一般是Ctrl+Z。发送给所有Foreground Group的进程
// SIGTTIN  当Background Group的进程尝试读取Terminal的时候发送
// SIGTTOU  当Background Group的进程尝试写Terminal的时候发送
// SIGURG  当out-of-band data接收的时候可能发送
// SIGUSR1  用户自定义signal 1
// SIGUSR2  用户自定义signal 2
// SIGVTALRM  setitimer函数设置的Virtual Interval Timer超时的时候
// SIGWAITING  Solaris Thread Library内部实现专用
// SIGWINCH  当Terminal的窗口大小改变的时候，发送给Foreground Group的所有进程
// SIGXCPU  当CPU时间限制超时的时候
// SIGXFSZ  进程超过文件大小限制
// SIGXRES  Solaris专用，进程超过资源限制的时候发送
*/

/*
 install a signo, and it will dump the stack log into the folder: path is the folder, logfile will be save there.
 0 --> OK, -1 --> failed
*/
int signal_dump_bt2file(int signo, const char *path, signal_ending_act_st *act) 
{
	
	int ret = 0;
	struct sigaction sigMem;
	switch (signo)
	{
		case SIGABRT:
			sigMem.sa_flags = (SA_RESTART | SA_SIGINFO);
			sigMem.sa_sigaction = dump_stack;
			break;
		case SIGSEGV:
			sigMem.sa_flags = (SA_RESTART | SA_SIGINFO);
			sigMem.sa_sigaction = dump_stack;
			break;
		case SIGBUS:
			sigMem.sa_flags = (SA_RESTART | SA_SIGINFO);
			sigMem.sa_sigaction = dump_stack;
			break;

		default:
			ret = -1;
			printf("not supported for the signo:%d\n", signo);
			break;
	}

	if (ret == 0)
	{
		if (sigaction(signo, &sigMem, NULL) == 0)
		{
			if (path != NULL)
			{
				snprintf(backtrace_store_path, sizeof(backtrace_store_path) - 1, path);
			}
			//fill the ending action
			if (signo > 0 && signo <= SIGSYS && act != NULL)
			{
				g_ending_act_arry[signo].cb = act->cb;
				g_ending_act_arry[signo].data = act->data;
			}

			printf("[%s %d] install the signo %d successfully.\n", __func__, __LINE__, signo);
		}
		else
		{
			ret = -1;
		}
	}

	return 0;
}
