#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
       #include <sys/stat.h>
       #include <fcntl.h>
	   #include <execinfo.h>
#include <signal.h>
 #include <sys/time.h>
       #include <sys/resource.h>


#include "libhttpc.h"
#include "libsyshelper.h"

static int usage(const char *progname)
{
	fprintf(stderr, "Usage: %s 1/2  url\n"
		"1--> download, 2--> upload\n"
		"\n", progname);

	return 1;
}


//for now, just save the core dump into SD card.
static bool supportCoreDump()
{
   #ifdef SUPPORT_COREDUMP   //When you promotion, please disable it in makefile(SUPPORT_COREDUMP)
   return true;
   #else
   return false;
   #endif
}

static void setCoreDumpSize(long maxSize)
{
    if( !supportCoreDump() )
       return;
	struct rlimit rlim;
	if (maxSize > 0)
	{
		rlim.rlim_cur = (rlim_t)maxSize;
		rlim.rlim_max = (rlim_t)maxSize;
	}
	else
	{
		rlim.rlim_cur = RLIM_INFINITY;
		rlim.rlim_max = RLIM_INFINITY;
	}
	if ( 0 != setrlimit(RLIMIT_CORE, &rlim))
	{
		printf("ERROR changing RLIMIT_CORE limits\n");
	}

}

static void ending_cb(void *data)
{
	int *pInt = (int *)data;
	printf("I am ending act, I get the data is :%d\n", *pInt);
}

static signal_ending_act_st test_act;
static int x = 100;

static void install_signal_handle()
{
	test_act.cb = ending_cb;
	x = 200;
	test_act.data =  &x;
	signal_dump_bt2file(SIGABRT, NULL, &test_act);
	signal_dump_bt2file(SIGSEGV, NULL, &test_act);
}

void test3(int n)
{
	char *str;
	printf("in test3 [%d]\n", n);
	strcpy(str, "123");
}
void test2(int n)
{
	printf("in test2 [%d]\n", n);
	test3(3);
}
void test1(int n)
{
	printf("in test1 [%d]\n", n);
	test2(2);
}

int main(int argc, char **argv)
{
	install_signal_handle();
	while(1)
	{
		sleep(5);
		if (access("/data/sig11", F_OK) == 0)
		{
			test1(1);
		}
	}
	return 0;
}
