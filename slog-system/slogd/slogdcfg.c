/*
 * simple logd for the project log modules.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include "slogdcfg.h"
#include "libslog.h"


static slogd_cfg_s def_slogd_cfg= {
	.slog_file = "/var/tmp/slogd.log",
	.send2syslog = 0,
	.send2remote = "",
	.file_max_size = 2*1024,    //kbytes
	.mod_entry = {
		{/* SLOG_MOD_KERN,*/         SLOG_LEVEL_WARNING,     0 },
		{/* SLOG_MOD_FWUPGRADE,*/    SLOG_LEVEL_INFO,        1 },
		{/* SLOG_MOD_CAPWAP, */      SLOG_LEVEL_NOTICE,      1 },

		{/* SLOG_MOD_MAX,*/          SLOG_LEVEL_WARNING,     1 },
	}
} ;


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

/* find the setion's index, the default is global */
static SLOG_MOD_NAME_E find_mod_index(char *mod_name)
{
	SLOG_MOD_NAME_E i = SLOG_MOD_MAX;
	for (i = 0; i <= SLOG_MOD_MAX; i++)
	{
		if (strcasecmp(pmode2str[i], mod_name) == 0)
			break;
	}

	return i;
}

const char *find_cfg_key(char *keyname)
{
	char *pkeyname = NULL;
	for(int i = 0; cfg_members[i] != NULL; i++)
	{
		if (strcasecmp(cfg_members[i], keyname) == 0)
			pkeyname = cfg_members[i];
	}

	return pkeyname;
}
/*   init the cfg from file */
int init_cfg(char *cfg_name,  void *data)
{
	char keyname[32] = {0};
	char keyval[128] = {0};
	char *buf,*c;
	char buf_i[KEYVALLEN], buf_o[KEYVALLEN];

	//in the beginning, load the default config
	slogd_cfg_s *pcfg = (slogd_cfg_s*)data;
	memcpy(pcfg, &def_slogd_cfg, sizeof(slogd_cfg_s));

	//next rewrite them via config file.
	FILE *fp;
	if((fp=fopen(cfg_name,"r"))==NULL)
	{
		printf("openfile [%s] error [%s]\n",cfg_name, strerror(errno));
		return(-1);
	}
	
	fseek(fp, 0, SEEK_SET);
	while(!feof(fp) && fgets(buf_i, KEYVALLEN, fp)!=NULL)
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
			char tmp_buf[KEYVALLEN] = {0};
			SLOG_MOD_NAME_E mod_index;
			r_trim(tmp_buf, buf);

next_section:
			if (tmp_buf[0] == '[' && tmp_buf[strlen(tmp_buf)-1] == ']')//find a section
			{
				tmp_buf[strlen(tmp_buf)-1] = '\0';
				mod_index = find_mod_index(tmp_buf+1);

				//get the next line
				while(!feof(fp) && fgets(buf_i, KEYVALLEN, fp)!=NULL)
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
						r_trim(tmp_buf, buf);
						if (tmp_buf[0] == '[' && tmp_buf[strlen(tmp_buf)-1] == ']')//find next section!!!
						{
							goto  next_section;
						}
						if( (c = (char*)strchr(buf, '=')) == NULL )
							continue;
						memset( keyname, 0, sizeof(keyname) );
						sscanf( buf, "%[^=|^ |^\t]", keyname );//end of the "=" or "\t"
						if( find_cfg_key(keyname) != NULL )
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
								//printf("config for %s. key=%s , value=%s\n", pmode2str[mod_index], keyname, keyval);

								//fill the data.
								if (strcasecmp(keyname, "slogfile") == 0 && strlen(keyval) > 0)
								{
									strcpy(pcfg->slog_file,keyval);
								}
								else if (strcasecmp(keyname, "slogfile_size") == 0)
								{
									pcfg->file_max_size = atoi(keyval);
								}
								else if (strcasecmp(keyname, "send2syslog") == 0)
								{
									pcfg->send2syslog = atoi(keyval);
								}
								else if (strcasecmp(keyname, "send2remote") == 0)
								{
									strcpy(pcfg->send2remote,keyval);
								}
								else if (strcasecmp(keyname, "debug_leve") == 0)
								{
									((pcfg->mod_entry)[mod_index]).debug_leve = atoi(keyval);
								}
								else if (strcasecmp(keyname, "store_flag") == 0)
								{
									((pcfg->mod_entry)[mod_index]).store_flag = atoi(keyval);
								}
							}
							else
							{
								continue;
							}
						}
					}
				}
			}

		}
	}

	fclose( fp );

	//dump the current config
	slog_printf(0, 1, "global cfg: slog_file:%s, send2syslog:%d, send2remote:%s, file_max_size:%dKbytes, debug_leve:%d, store_flag:%d\n",pcfg->slog_file ,pcfg->send2syslog, pcfg->send2remote, pcfg->file_max_size, pcfg->mod_entry[SLOG_MOD_MAX].debug_leve, pcfg->mod_entry[SLOG_MOD_MAX].store_flag);
	for(int i = 0; i <= SLOG_MOD_MAX; i++)
	{
		if (pcfg->mod_entry[i].debug_leve == 0 && pcfg->mod_entry[i].store_flag == 0) //use the global to fill the default vale.
		{
			pcfg->mod_entry[i].debug_leve = pcfg->mod_entry[SLOG_MOD_MAX].debug_leve;
			pcfg->mod_entry[i].store_flag = pcfg->mod_entry[SLOG_MOD_MAX].store_flag;
		}
		slog_printf(0, 1, "%s(i=%d) cfg: debug_leve:%d, store_flag:%d\n", pmode2str[i], i, pcfg->mod_entry[i].debug_leve, pcfg->mod_entry[i].store_flag);
	}

	return(0);
}
