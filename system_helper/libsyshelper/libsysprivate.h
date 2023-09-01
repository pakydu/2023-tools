#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include "libslog.h"
#include "libhttpc.h"
#include "libsyshelper.h"

struct transfer_handle{
	struct httpc_input *input;
	rsp_cb  cb;
	void * data;
};

