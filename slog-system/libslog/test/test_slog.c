/*
 * simple logd for the project log modules.
 *
 */

#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <signal.h>
#include <poll.h>

#include "libslog.h"




int main(int argc, char **argv)
{
	printf(" test the slog api....\n");
	if (argc > 3)
	{
		printf("set the log cfg:\n");
		slog_ctl(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), "[%s %d] ",__func__,__LINE__);
	}
	else
	{
	while(1)
	{
		slog_printf(SLOG_MOD_KERN, 1, "test 111111111111111111111111111111111111111111111111111111110");
		//sleep(1);
		slog_printf(SLOG_MOD_FWUPGRADE, 2, "test 222222222222222222222222222222222222222222222222222222222222222222222222220");
		//sleep(2);
		slog_printf(SLOG_MOD_CAPWAP, 3, "test 3333333333333333333333333333333333333333333333333333333333333333333333333333333330");
		//sleep(1);
		slog_printf(SLOG_MOD_MAX, 4, "test 444444444444444444444444444444444444444444444444444444444444444444444444444440");
		//sleep(1);
		slog_printf(SLOG_MOD_MAX, 5, "test 555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555");
		//sleep(1);
		slog_printf(SLOG_MOD_MAX, 6, "test 6666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666660");
		//sleep(5);
	}
	}

	return 0;
}


