/*
 *	system normally library APIs.
 *	Copyright 2023 @inspur
*/

#include "libsysprivate.h"

/*   remove the left space   */
static char * l_trim(char * szOutput, const char *szInput)
{
	for( ; *szInput != '\0' && isspace(*szInput); ++szInput);  //check the string from left to right.

	return strcpy(szOutput, szInput);
}

/*   remove the right space   */
static char *r_trim(char *szOutput, const char *szInput)
{
	char *p = NULL;
	strcpy(szOutput, szInput);
	for(p = szOutput + strlen(szOutput) - 1; p >= szOutput && isspace(*p); --p); //check the string from right to left.

	*(++p) = '\0';
	return szOutput;
}

/*   remove the right and left space   */
static char * a_trim(char * szOutput, const char * szInput)
{
	char *p = NULL;
	l_trim(szOutput, szInput);//check the string from left to right.
	for(p = szOutput + strlen(szOutput) - 1;p >= szOutput && isspace(*p); --p);//check the string from right to left.

	*(++p) = '\0';
	return szOutput;
}


static void getslog_filename(char *file_name)
{
	char *cfg_name = "/etc/slogd.cfg";//
	strcpy(file_name, "/var/tmp/slogd.log");//set the default file name in the beginning.
	FILE *fp;
	char buf_i[256], buf_o[256];
	char *buf = NULL;
	char keyname[128], keyval[128];
	if((fp=fopen(cfg_name,"r"))==NULL)
	{
		printf("openfile [%s] error [%s]\n",cfg_name, strerror(errno));
		return;
	}
	
	fseek(fp, 0, SEEK_SET);
	while(!feof(fp) && fgets(buf_i, 256, fp)!=NULL)
	{
		l_trim(buf_o, buf_i);
		if( strlen(buf_o) <= 0 )
			continue;
		buf = NULL;
		buf = buf_o;
		if( buf[0] == '#' )
		{
			continue;
		}
		else
		{
			//try to the a section: [xxx]
			char tmp_buf[256] = {0};
			r_trim(tmp_buf, buf);
			char *c = NULL;

			if( (c = (char*)strchr(buf, '=')) == NULL )
				continue;
			memset( keyname, 0, sizeof(keyname) );
			sscanf( buf, "%[^=|^ |^\t]", keyname );//end of the "=" or "\t"
			if( strstr(keyname, "slogfile") != NULL )
			{
				sscanf( ++c, "%[^\n]", keyval );//begin with "="
				char *KeyVal_o = (char *)malloc(strlen(keyval) + 1);
				if(KeyVal_o != NULL)
				{
					memset(KeyVal_o, 0, strlen(keyval) + 1);
					a_trim(KeyVal_o, keyval);
					if(KeyVal_o && strlen(KeyVal_o) > 0)
						strcpy(keyval, KeyVal_o);
					free(KeyVal_o);
					KeyVal_o = NULL;
					printf("key=%s , value=%s\n", keyname, keyval);
					strcpy(file_name,keyval);
					break;
				}
				else
				{
					continue;
				}
			}
		}
	}

	fclose( fp );
}


//call back for upload log file.
static void upload_slog_job(void *data)
{
	struct transfer_handle * pTran = (struct transfer_handle *)data;
	if (pTran == NULL)
	{
		printf("input the paramters error...\n");
		return ;
	}
	slog_printf(SLOG_MOD_SYSHELPER, SLOG_LEVEL_NOTICE, "start upload the slog:%s", pTran->input->file_name);

	char * pdata = (char *) pTran->data;
	char err_text[128] = {0};
	rsp_cb * cb = pTran->cb;
	const char *rest_fmt = "{\"type\":\"%s\",\"filepath\":\"%s\",\"result\":\"%s\", \"detail\": \"%s\"}";
	int ret = 0;

	if (httpc_transferfile(eHTTPUPLOAD, pTran->input, err_text) == 0)
	{
		//printf("upload syslog file OK\n");
		ret = 0;//ok
	}
	else
	{
		//printf("upload syslog file failed\n");
		ret = 1;//failed
	}

	sprintf(pdata, rest_fmt, "upload", pTran->input->file_name, ret == 0 ? "success":"failed", ret == 0 ? "success":err_text);
	slog_printf(SLOG_MOD_SYSHELPER, SLOG_LEVEL_NOTICE, "upload slog : %s", pdata);

	remove(pTran->input->file_name);

	pTran->cb(pTran->data);


	//send the backup file?
	char file_name[128] = {0};
	char tmp_file[128]  = {0};
	getslog_filename(file_name);
	sprintf(tmp_file, "%s.bk",pTran->input->file_name);
	char bk_file[256] = {0};
	sprintf(bk_file, "%s.bk", file_name);
	printf("bk_file:%s, tmp_file:%s\n", bk_file, tmp_file);
	if(copyfile(bk_file, tmp_file) != -1)
	{
		strcpy(pTran->input->file_name, tmp_file);
		if (httpc_transferfile(eHTTPUPLOAD, pTran->input, err_text) == 0)
		{
			printf("upload syslog file OK\n");
			ret = 0;//ok
		}
		else
		{
			printf("upload syslog file failed, err_tex=%s\n", err_text);
			ret = 1;//failed
		}
		printf("backup file:%s\n", pTran->input->file_name);
		remove(pTran->input->file_name);
		// sprintf(pdata, rest_fmt, "upload", pTran->input->file_name, ret == 0 ? "success":"failed");
		// slog_printf(SLOG_MOD_SYSHELPER, SLOG_LEVEL_NOTICE, "upload slog : %s", pdata);
		// pTran->cb(pTran->data);
	}
}

void sys_upload_slog(struct httpc_input *input, rsp_cb * cb, char * result_json)
{
	char file_name[128] = {0};
	char tmp_file[128]  = {0};
	getslog_filename(file_name);
	char mac[8] = {0};
	get_mac_by_interface("br0", mac);
	sprintf(tmp_file, "/tmp/%02X-%02X-%02X-%02X-%02X-%02X_%s", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], basename(file_name));
	copyfile(file_name, tmp_file);

	strcpy(input->file_name, tmp_file);

	//next try to start new thread to do the job
	static struct transfer_handle transfer_data;
	transfer_data.input = input;
	transfer_data.cb    = cb;
	transfer_data.data  = result_json;
	printf("file:%s\n", input->file_name);

	start_pthread(upload_slog_job, &transfer_data);

}