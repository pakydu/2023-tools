#include "libsysprivate.h"


//call back for download fw.
static void download_fw_job(void *data)
{
	struct transfer_handle * pTran = (struct transfer_handle *)data;
	if (pTran == NULL)
	{
		printf("input the paramters error...\n");
		return ;
	}
	slog_printf(SLOG_MOD_SYSHELPER, SLOG_LEVEL_NOTICE, "start download the FW:%s", pTran->input->file_name);

	char * pdata = (char *) pTran->data;
	char err_text[128] = {0};
	rsp_cb * cb = pTran->cb;
	const char *rest_fmt = "{\"type\":\"%s\",\"filepath\":\"%s\",\"result\":\"%s\", \"detail\": \"%s\"}";
	int ret = 0;

	if (httpc_transferfile(eHTTPDOWNLOAD, pTran->input, err_text) == 0)
	{
		//printf("download fw OK\n");
		ret = 0;//ok
	}
	else
	{
		//printf("download fw failed\n");
		ret = 1;//failed
	}

	sprintf(pdata, rest_fmt, "download", pTran->input->file_name, ret == 0 ? "success":"failed");
	slog_printf(SLOG_MOD_SYSHELPER, SLOG_LEVEL_NOTICE, rest_fmt, "download", pTran->input->file_name, ret == 0 ? "success":"failed", ret == 0 ? "success":err_text);

	if (ret == 0)
	{
		ret = sys_upgrade_firmware(pTran->input->file_name, 5 /* NP_NO_HEADER*/, true); //upgrade success.
		sprintf(pdata, rest_fmt, "upgrade", pTran->input->file_name, ret == 0 ? "success":"failed");
	}

	remove(pTran->input->file_name);

	pTran->cb(pTran->data);

}

#define AP_UPGRADE_FW_NAME "/tmp/upgrade_fw"
void sys_upgrade_fw(struct httpc_input *input, rsp_cb * cb, char * result_json)
{
	strcpy(input->file_name, AP_UPGRADE_FW_NAME);
	static struct transfer_handle transfer_data;
	transfer_data.input = input;
	transfer_data.cb    = cb;
	transfer_data.data  = result_json;

	//next try to start new thread to do the job
	start_pthread(download_fw_job, &transfer_data);
}