/*
 *	system normally library APIs.
 *	Copyright 2023 @inspur
*/


#include <stdio.h>
#include <errno.h>

#include "libslog.h"
#include "libsyshelper.h"


#define GREEN_LED_1    63    //ON OR OFF
#define GREEN_LED_2    62    //BLINK OR OFF
#define GREEN_LED_3    61    //SLOW BLINK OR OFF
#define GREEN_LED_4    60    //VERY SLOW BLINK OR OFF

#define RED_LED_1    68    //ON OR OFF
#define RED_LED_2    69    //BLINK OR OFF
#define RED_LED_3    70    //SLOW BLINK OR OFF
#define RED_LED_4    71    //VERY SLOW BLINK OR OFF

#define LED_CTRL_ON    "on %d"
#define LED_CTRL_OFF   "off %d"
#define LED_CTRL_FILE   "/proc/tc3162/led_test"

//led control
int sys_led_ctrl(LED_NO_E led, LED_ACTION_E act)
{
	if ( led >= LED_NO_MAX || act >= LED_ACTION_MAX)
	{
		return -1;
	}

	int fd = open(LED_CTRL_FILE, O_WRONLY|O_APPEND);
	if (fd == -1)
	{
		perror("fopen failed: "LED_CTRL_FILE);
		return -1;
	}
	char buf[128] = {0};
	int ret = 0;
	slog_printf(0, 0, "set the led: %d to %d", led, act);

	if (led == LED_NO_GREEN)
	{
		for (int i = GREEN_LED_1; i >= GREEN_LED_4; i--)
		{
			sprintf(buf, LED_CTRL_OFF, i);
			write(fd, buf, strlen(buf));
		}

		switch (act)
		{
			case LED_ACTION_OFF:
				// sprintf(buf, LED_CTRL_OFF, GREEN_LED_1);
				// fwrite(buf, strlen(buf), 1, fp);
				break;

			case LED_ACTION_ON:
				sprintf(buf, LED_CTRL_ON, GREEN_LED_1);
				write(fd, buf, strlen(buf));
				break;

			case LED_ACTION_BLINK:
				sprintf(buf, LED_CTRL_ON, GREEN_LED_2);
				write(fd, buf, strlen(buf));
				break;

			case LED_ACTION_BLINK_SLOW:
				sprintf(buf, LED_CTRL_ON, GREEN_LED_3);
				write(fd, buf, strlen(buf));
				break;

			case LED_ACTION_BLINK_VERY_SLOW:
				sprintf(buf, LED_CTRL_ON, GREEN_LED_4);
				write(fd, buf, strlen(buf));
				break;

			default:
				printf("unkonw action: %d\n", act);
				ret = -1;
				break;
		}
	}
	else if (led == LED_NO_RED)
	{
		for (int i = RED_LED_1; i <= RED_LED_4; i++)
		{
			sprintf(buf, LED_CTRL_OFF, i);
			write(fd, buf, strlen(buf));
		}

		switch (act)
		{
			case LED_ACTION_OFF:
				// sprintf(buf, LED_CTRL_OFF, RED_LED_1);
				// fwrite(buf, strlen(buf), 1, fp);
				break;

			case LED_ACTION_ON:
				sprintf(buf, LED_CTRL_ON, RED_LED_1);
				write(fd, buf, strlen(buf));
				break;

			case LED_ACTION_BLINK:
				sprintf(buf, LED_CTRL_ON, RED_LED_2);
				write(fd, buf, strlen(buf));
				break;

			case LED_ACTION_BLINK_SLOW:
				sprintf(buf, LED_CTRL_ON, RED_LED_3);
				write(fd, buf, strlen(buf));
				break;

			case LED_ACTION_BLINK_VERY_SLOW:
				sprintf(buf, LED_CTRL_ON, RED_LED_4);
				write(fd, buf, strlen(buf));
				break;

			default:
				printf("unkonw action: %d\n", act);
				ret = -1;
				break;
		}
	}
	else if (led == LED_NO_MIX)
	{
		for (int i = GREEN_LED_1; i >= GREEN_LED_4; i--)
		{
			sprintf(buf, LED_CTRL_OFF, i);
			write(fd, buf, strlen(buf));
		}
		
		for (int i = RED_LED_1; i <= RED_LED_4; i++)
		{
			sprintf(buf, LED_CTRL_OFF, i);
			write(fd, buf, strlen(buf));
		}

		switch (act)
		{
			case LED_ACTION_OFF:
				// sprintf(buf, LED_CTRL_OFF, RED_LED_1);
				// fwrite(buf, strlen(buf), 1, fp);
				break;

			case LED_ACTION_ON:
				sprintf(buf, LED_CTRL_ON, RED_LED_1);
				write(fd, buf, strlen(buf));
				sprintf(buf, LED_CTRL_ON, GREEN_LED_1);
				write(fd, buf, strlen(buf));
				break;
			case LED_ACTION_BLINK:
				sprintf(buf, LED_CTRL_ON, RED_LED_1);
				write(fd, buf, strlen(buf));
				sprintf(buf, LED_CTRL_ON, GREEN_LED_2);
				write(fd, buf, strlen(buf));
				break;

			case LED_ACTION_BLINK_SLOW:
				sprintf(buf, LED_CTRL_ON, RED_LED_1);
				write(fd, buf, strlen(buf));
				sprintf(buf, LED_CTRL_ON, GREEN_LED_3);
				write(fd, buf, strlen(buf));
				break;

			case LED_ACTION_BLINK_VERY_SLOW:
				sprintf(buf, LED_CTRL_ON, RED_LED_1);
				write(fd, buf, strlen(buf));
				sprintf(buf, LED_CTRL_ON, GREEN_LED_4);
				write(fd, buf, strlen(buf));
				break;

			default:
				printf("unkonw action: %d\n", act);
				ret = -1;
				break;
		}
	}
	close(fd);

	return ret;
}
