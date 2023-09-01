#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <libslog.h>
#include "libhttpc.h"

// const char *url = "https://www.baidu.com2";
// const char *save_file = "save_file.txt";

/*
    TR143 upload statistics info
*/
struct timeval g_sysTimeTCPOpenRequest;
struct timeval g_sysTimeTCPOpenResponse;
struct timeval g_sysTimeROM;
struct timeval g_sysTimeBOM;
struct timeval g_sysTimeEOM;
unsigned int g_BOMOutOctet;
unsigned int g_EOMOutOctet;
unsigned int g_diagstate;

int add_http_header(struct http_client *httpc, char *hdr)
{
	struct curl_slist *header_list_p = NULL;

	if ( NULL == httpc || NULL == hdr )
		return -1;
	header_list_p = curl_slist_append(httpc->header_list, hdr);
	if ( NULL == header_list_p )
		return -1;

	httpc->header_list = header_list_p;
	return 0;
}


static int http_file_transfer_header_init(struct http_client *httpc)
{
	if ( NULL == httpc )
		return -1;

	httpc->header_list = NULL;
	add_http_header(httpc, "Accept: */*");
	add_http_header(httpc, "User-Agent: free-httpclient 1.1");
	add_http_header(httpc, "Content-Type: multipart/form-data");
	add_http_header(httpc, "Expect:");

	return 0;
}

static size_t http_get_response(void *buffer, size_t size, size_t nmemb, void *userdata)
{
	size_t new_buf_len = 0;
	char *new_buf = NULL;
	struct http_client *httpc = (struct http_client *)userdata;

	if ( RESP_MODE_FILE== httpc->http_resp_mode )
	{
		new_buf_len = fwrite(buffer, size, nmemb, httpc->fp_http_file);
	}
	else
	{
		new_buf_len = size * nmemb;
		new_buf = realloc(httpc->recvBuffer, httpc->recvBufferLength + new_buf_len + 1);

		if ( NULL == new_buf )
			return 0;
		memcpy(new_buf + httpc->recvBufferLength, buffer, new_buf_len);
		httpc->recvBuffer = new_buf;
		httpc->recvBufferLength += new_buf_len;
		httpc->recvBuffer[httpc->recvBufferLength] = 0;
	}

	return new_buf_len;
}

static size_t http_send_request(void *buffer, size_t size, size_t nmemb, void *userdata)
{
	size_t buf_len = 0;
	struct http_client *httpc = (struct http_client *)userdata;

	buf_len = fread(buffer, size, nmemb, httpc->fp_http_file);
	return buf_len;
}

// static int curl_sockopt_call(void *clientp,
//                                      curl_socket_t curlfd,
//                                      curlsocktype purpose)
// {
// 	char w_ip_v4[32] = "192.168.10.1", w_ip_v6[40] = {0};
// 	struct sockaddr_in v4_addr;
// 	struct sockaddr_in6 v6_addr;
// 	int bind_addr_state = 0;

// 	/* bind tr69 wan. */
// 	bzero(&v4_addr, sizeof(v4_addr));
// 	v4_addr.sin_family = AF_INET;
// 	if ( 1 != inet_pton(AF_INET, w_ip_v4, &v4_addr.sin_addr) ) 
// 		printf("[%s]:inet_pton AF_INET failed.\n", __FUNCTION__);
// 	if ( bind(curlfd, (struct sockaddr *)&v4_addr, sizeof(v4_addr)) < 0 )
// 		printf("[%s]:bind v4 addr failed.\n", __FUNCTION__);
// 	else
// 	{
// 		printf("[%s]:bind v4 addr OK.\n", __FUNCTION__);
// 		bind_addr_state |= BIND_V4_OK;
// 	}
	
// 	return CURL_SOCKOPT_OK;
// }


static int curl_debug_cb(CURL *handle, curl_infotype type,
                  unsigned char *data, size_t size,
                  void *userdata)
{
	FILE *fp = fopen("/proc/tc3162/dbg_msg", "w");

	if ( NULL != fp )
	{
		fwrite(data, size, 1, fp);
		fclose(fp);
	}

	// slog_printf(SLOG_MOD_MAX, SLOG_LEVEL_EMERG, "%s", data);

	return 0;
}

CURL *http_client_init(struct http_client *httpc)
{
	CURL *curl = NULL;
	char *http_username = NULL;
	char *http_password = NULL;
	char *ssl_cert_path = NULL;
	char *ssl_key_path = NULL;
	char *ssl_keypasswd = NULL;
	char *ssl_ca_cert_path = NULL;
	char *http_url = NULL;
	int32_t conn_mode = CONN_TYPE_IPV4;//get_connect_mode();
	char result[128] = {0};
	int ret = -1;
	char replaceDomainName[8] = {0};

	if ( NULL == httpc )
		return NULL;

	http_username = httpc->username;
	http_password = httpc->password;
	// if ( SSL_MODE_SINGLE_AT_S == get_ssl_client_mode()
	// 	|| SSL_MODE_BOTH == get_ssl_client_mode() )
	// {
	// 	ssl_cert_path = httpc->ssl_cert_path;
	// 	ssl_key_path = httpc->ssl_key_path;
	// 	ssl_keypasswd = httpc->ssl_keypasswd;
	// }

	// if ( SSL_MODE_SINGLE_AT_C == get_ssl_client_mode()
	// 	|| SSL_MODE_BOTH == get_ssl_client_mode() )
	// 	ssl_ca_cert_path = httpc->ssl_ca_cert_path;

	http_url = httpc->url;


	if ( NULL == http_url )
	{
		printf("\n[%s]:error url is NULL.\n", __FUNCTION__);
		return NULL;
	}

	curl = curl_easy_init();
	if ( !curl )
	{
		printf("\n[%s]:error curl_easy_init failed.\n", __FUNCTION__);
		return NULL;
	}

	// printf("\n[%s]:http user & psw [%s]:[%s] \n", __FUNCTION__
	// 			, (http_username ? http_username : "")
	// 			, (http_password ? http_password : "") );

	curl_easy_setopt(curl, CURLOPT_URL, http_url);
	curl_easy_setopt(curl, CURLOPT_USERNAME, http_username ? http_username : "");
	curl_easy_setopt(curl, CURLOPT_PASSWORD, http_password ? http_password : "");
	curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
	if ( RESP_MODE_NONE != httpc->http_resp_mode )
	{
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_get_response);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, httpc);
	}
	if ( REQ_MODE_NONE != httpc->http_req_mode )
	{
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, http_send_request);
		curl_easy_setopt(curl, CURLOPT_READDATA, httpc);
	}
	//curl_easy_setopt(curl, CURLOPT_SOCKOPTFUNCTION, curl_sockopt_call);

	if ( CONN_TYPE_IPV6 == conn_mode )
		curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V6);
	else
		curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

	if ( 0 == httpc->time_out )
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 8);
	else
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, httpc->time_out);

	if (0)//debug ...
	{
		curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, curl_debug_cb);
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	}
	curl_easy_setopt(curl, CURLOPT_COOKIEFILE, httpc->cookies_file);
	curl_easy_setopt(curl, CURLOPT_COOKIEJAR, httpc->cookies_file);
	curl_easy_setopt(httpc->curl, CURLOPT_FAILONERROR, 1);
	curl_easy_setopt(httpc->curl, CURLOPT_ERRORBUFFER, httpc->error_buf);

// #if defined(TCSUPPORT_CWMP_OPENSSL)
// 	if ( ssl_cert_path )
// 		curl_easy_setopt(curl, CURLOPT_SSLCERT, ssl_cert_path);
// 	if ( ssl_key_path )
// 		curl_easy_setopt(curl, CURLOPT_SSLKEY, ssl_key_path);
// 	if ( ssl_keypasswd )
// 		curl_easy_setopt(curl, CURLOPT_SSLKEYPASSWD, ssl_keypasswd);
// 	if ( ssl_ca_cert_path )
// 		curl_easy_setopt(curl, CURLOPT_CAINFO, ssl_ca_cert_path);
// 	if ( 1 == get_acs_ssl_verify() )
// 		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1);
// 	else
// 		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
// 	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
// #endif

	return curl;
}


int http_header_free(struct http_client *httpc)
{
	if ( httpc && httpc->header_list )
	{
		curl_slist_free_all(httpc->header_list);
		httpc->header_list = NULL;
	}

	return 0;
}



// static int httptask_process_task_fail(struct http_client *httpc, int httpCode, int curl_code)
// {
// 	int fileType;

// 	if ( NULL == httpc )
// 		return -1;

// 	switch (httpCode)
// 	{
// 		case HTTP_200_OK:
// 		printf("[%s %d]\n",__func__,__LINE__);
// 		break;
// 		case HTTP_401_UNAUTHORIZED:
// 		printf("[%s %d]\n",__func__,__LINE__);
// 		break;
// 		case HTTP_405_METHOD_NOT_ALLOWED:
// 		printf("[%s %d]\n",__func__,__LINE__);
// 		break;
// 		case HTTP_404_NOTFOUND:
// 		printf("[%s %d]\n",__func__,__LINE__);
// 		break;
// 		default:
// 		printf("[%s %d] other code: %d\n",__func__,__LINE__, httpCode);
// 		break;
// 	}	

// 	return 0;
// }

int httpc_transferfile(httpfileType type, struct httpc_input * pinput, char * err_text)
{
	httpfileType actionType = type; 
	const char *file_name = pinput->file_name;
	CURL *curl = NULL;
	CURLcode curl_code;

	struct stat file_info;
	int httpCode = 0, ret = 0;
	struct http_client http_c;
	bzero(&http_c, sizeof(http_c));

	http_c.username= pinput->username;
	http_c.password = pinput->passwd;
	http_c.url = pinput->url;//"http://192.168.56.1/up/1.txt";
	http_c.cookies_file = CURL_TRANSFER_COOKIES_FILE;
	if (actionType == eHTTPDOWNLOAD) //download
	{
		http_c.cookies_file = CURL_TRANSFER_COOKIES_FILE;
		http_c.http_resp_mode = RESP_MODE_FILE;
		http_c.fp_http_file = fopen(file_name, "wb");
		curl_easy_setopt(curl, CURLOPT_FTP_RESPONSE_TIMEOUT, 12000000);
	}
	else if (actionType == eHTTPUPLOAD) //download
	{
		http_c.http_req_mode = REQ_MODE_FILE;
		http_c.fp_http_file = fopen(file_name, "rb");
		stat(file_name, &file_info);
	}
	else
	{
		//printf("[%s]:Unknown file transfer type.\n", __FUNCTION__);
		if (err_text)
		{
			sprintf(err_text, "not support transfer type");
		}
			
		ret = -1;
		goto http_task_fail;
	}

	if ( NULL == http_c.fp_http_file
		&& NULL != file_name )
	{
		//printf("[%s]:fp is NULL, file is [%s].\n", __FUNCTION__, file_name);
		if (err_text)
		{
			sprintf(err_text, "local fp error");
		}
		ret = -2;
		goto http_task_fail;
	}

	http_file_transfer_header_init(&http_c);
	if ( NULL == (curl = http_client_init(&http_c)) )
	{
		//printf("[%s]:http_client_init failed.\n", __FUNCTION__);
		if (err_text)
		{
			sprintf(err_text, "http client init failed");
		}
		ret = -3;
		goto http_task_fail;
	}
	http_c.curl = curl;
	http_c.time_out = pinput->time_out == 0 ? HTTP_RSP_DEFAULT_TIMEOUT : pinput->time_out;	
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, http_c.time_out);

	//for upload via POST.
	curl_mime *mime = NULL;
	curl_mimepart *part;
	if (actionType == eHTTPUPLOAD)
	{
		add_http_header(&http_c, "SOAPAction:");

		#ifdef USE_HTTP_PUT
		curl_easy_setopt(curl, CURLOPT_PUT, 1);
		curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)file_info.st_size);
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
		#else  //default by POST.
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        mime = curl_mime_init(curl);
        part = curl_mime_addpart(mime);
        curl_mime_name(part, "");
		curl_mime_filedata(part, file_name);
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
		curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)file_info.st_size);
		#endif

		curl_easy_setopt(curl, CURLOPT_FTP_RESPONSE_TIMEOUT, 12000000);
	}

	if(CURLE_OK != curl_easy_setopt(curl, CURLOPT_HTTPHEADER, http_c.header_list))
	{
		//printf("[%s]:curl_easy_setopt CURLOPT_HTTPHEADER failed.\n", __FUNCTION__);
		if (err_text)
		{
			sprintf(err_text, "set http header failed");
		}
		ret = -4;
		goto http_task_fail;
	}
	curl_code = curl_easy_perform(http_c.curl);
	http_header_free(&http_c);
	//printf("[%s %d]curl_code:%s\n",__func__,__LINE__, curl_easy_strerror(curl_code));

	if ( 0 != http_c.error_buf[0] )
		printf("[%s]:LibCurl Error: %s.\n", __FUNCTION__, http_c.error_buf);

	
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

	if ( CURLE_OK != curl_code
		|| ( HTTP_200_OK != httpCode && HTTP_204_NOCONTENT != httpCode ))
	{
		//printf("[%s]:sending http message failed: %d, curl_code:%d.\n", __FUNCTION__, httpCode, curl_code);
		if (err_text)
		{
			sprintf(err_text, "http client send failed, httpCode=%d,curl_code=%d", httpCode, curl_code);
		}
		ret = -5;
		goto http_task_fail;
	}

http_task_fail:
	// httptask_process_task_fail(&http_c, httpCode, curl_code);

	if (http_c.fp_http_file != NULL)
	{
		fclose(http_c.fp_http_file);
		http_c.fp_http_file = NULL;
	}
	http_header_free(&http_c);
	if (mime != NULL)
		curl_mime_free(mime);

	return ret;
}
