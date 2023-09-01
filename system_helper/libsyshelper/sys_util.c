/*
 *	system normally library APIs.
 *	Copyright 2023 @inspur
*/


#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <mntent.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <sys/time.h>
#include <linux/if_bridge.h>

#include "libsysprivate.h"

#include<sys/syscall.h>
pid_t gettid() { return syscall(SYS_gettid); }

#define BUFFER_SIZE 512
int copyfile(char * src_name, char * des_name)
#if 1
{
	int ret = 0;
	int rd_len = 0;
	int wt_len = 0;
	char log_buf[256] = {0};
	char *ptr = NULL;

	int src_fd = open(src_name, O_RDONLY,0644);
	int des_fd = open(des_name, O_CREAT|O_APPEND|O_RDWR,0644);

	if (src_fd >= 0 && des_fd >= 0)
	{
		lseek(src_fd, 0L, SEEK_SET);
		lseek(des_fd, 0L, SEEK_SET);
	
		while(rd_len = read(src_fd, log_buf, 256))
		{
			if((rd_len == -1) && (errno != EINTR))
				break;
			else if(rd_len>0)
			{
				ptr=log_buf;
				while(wt_len = write(des_fd, ptr, rd_len))
				{
					if((wt_len == -1) && (errno != EINTR))
						break;
					else if(wt_len == rd_len)
						break;//go to the next new buffer.
					else if( wt_len > 0)
					{
						ptr += wt_len;
						rd_len -= wt_len;
					}
				}
				if(wt_len == -1)
				{
					ret = -1;
					break;//OOP...
				}
			}
		}
	}
	else
		ret = -1;

	if(src_fd >= 0)
		close(src_fd);

	if(des_fd >= 0)
		close(des_fd);

	return ret;
}
#else
{
	FILE *source_file, *dest_file;
	char buffer[BUFFER_SIZE];
	size_t bytes_read;

	source_file = fopen(old_name, "r");
	if (source_file == NULL)
	{
		return 1;
	}

	dest_file = fopen(new_name, "w");
	if (dest_file == NULL) 
	{
		fclose(source_file);
		return 1;
	}

	while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, source_file)) > 0)
	{
		fwrite(buffer, 1, bytes_read, dest_file);
	}

	fclose(source_file);
	fclose(dest_file);

	return 0;
}
#endif

int get_profile_key_val(char *keyname,char *str_return, int size,char *path)
{
	FILE *fp;
	char *str_key= NULL;
	char stream[MAXGET_PROFILE_SIZE]={0};
	int totalLength=0;
	char *p = NULL;

	fp=fopen(path,"r");
	if(fp==NULL){
		fprintf(stderr,"Can't open %s\n",path);
		return -1;
	}

	memset(str_return, 0, size);
	fseek(fp, 0, SEEK_SET);

	while(fgets(stream, MAXGET_PROFILE_SIZE, fp) != NULL)
	{
		str_key = strstr(stream,keyname);
		if(str_key == NULL || str_key != stream)
		{
			memset(stream, 0, sizeof(stream));
			continue;
		}	

		p = strtok(stream,"\r");
		while(p)
		{
			p = strtok(NULL,"\r");
		}

		p = strtok(stream,"\n");
		while(p)
		{
			p = strtok(NULL,"\n");
		}

		totalLength=strlen(stream)-strlen(keyname);
		if(size<totalLength+1){/*total length + '\0' should not less than buffer*/
			fprintf(stderr, "Too small buffer to catch the %s frome get_profile_str_new\n", keyname);
			fclose(fp);
			return -1;
		}
		else if(totalLength<0) {/*can't get a negative length string*/
			fprintf(stderr, "No profile string can get\n");
			fclose(fp);
			return -1;
		}
		else{
			strncpy(str_return, stream+strlen(keyname), totalLength);
			str_return[totalLength]= '\0';
			fclose(fp);
			return strlen(str_return);
		}
	}
	fclose(fp);
	fprintf(stderr,"File %s content %s is worng\n",path,keyname);
	return -1;
}

static void find_real_root_device_name(char *root_dev)
{
	DIR *dir;
	struct dirent *entry;
	struct stat statBuf, rootStat;
	char fileName[128] = {0};
	dev_t dev;

	if (stat("/", &rootStat) != 0)
		perror("could not stat '/'");
	else {
		/* This check is here in case they pass in /dev name */
		if ((rootStat.st_mode & S_IFMT) == S_IFBLK)
			dev = rootStat.st_rdev;
		else
			dev = rootStat.st_dev;

		dir = opendir("/dev");
		if (!dir)
			perror("could not open '/dev'");
		else {
			while((entry = readdir(dir)) != NULL) {
				const char *myname = entry->d_name;
				
				/* Must skip ".." since that is "/", and so we
				 * would get a false positive on ".."  */
				if (myname[0] == '.' && myname[1] == '.' && !myname[2])
					continue;

				

				/* if there is a link named /dev/root skip that too */
				if (strcmp(myname, "root")==0)
					continue;

				sprintf(fileName,"/dev/%s", myname);
				printf("fileName:%s\n", fileName);

				/* Some char devices have the same dev_t as block
				 * devices, so make sure this is a block device */
				if (stat(fileName, &statBuf) == 0 &&
						S_ISBLK(statBuf.st_mode)!=0 &&
						statBuf.st_rdev == dev)
				{
					strcpy(root_dev, fileName);
					break;
				}
			}
			closedir(dir);
		}
	}
}


int find_path_mountpoint(const char *mount_path, char *dev_name)
{
	struct mntent *m;
	int ret = -1;
	FILE *fp = NULL;
	fp = setmntent("/proc/mounts","r"); //open file for describing the mounted filesystems: /etc/fstab  or /etc/mtab
	if(!fp)
	{
		printf("error:%s\n",strerror(errno));
		ret = -1;
		return ret;
	}

	while ((m = getmntent(fp)))        //read next line
	{
		//printf("1Drive %s, name %s,type  %s,opt  %s\n", m->mnt_dir, m->mnt_fsname,m->mnt_type,m->mnt_opts );
		if (strcmp(m->mnt_dir, mount_path) == 0)
		{
			if (strcmp(m->mnt_fsname, "/dev/root") == 0)//convert it to block device
				find_real_root_device_name(dev_name);
			else
				strcpy(dev_name, m->mnt_fsname);
			//printf("2Drive %s, name %s(%s),type  %s,opt  %s\n", m->mnt_dir, m->mnt_fsname, dev_name, m->mnt_type,m->mnt_opts );
			ret = 0;
		}
	}
	
	endmntent(fp);

	return ret;
}

/*
# cat /proc/cmdline
sdram_conf=0x00108893 vendor_name=ECONET Technologies Corp. product_name=xPON ONU ethaddr=c8:ef:bc:a0:05:f0 snmp_sysobjid=1.2.3.4.5 country_code=ff ether_gpio=0c power_gpio=0b0b username=root password=Pon521 dsl_gpio=0b internet_gpio=02 multi_upgrade_gpio=0b020400000000000000000000000000 onu_type=2 qdma_init=33 root=/dev/mtdblock6 ro console=ttyS0,115200n8 earlycon bootflag=1 serdes_sel=0 tclinux_info=0x14ffa07,0x2024,0x2f9a04,0x2fbb20,0x1203ba5,0x14fef33,0x2024,0x2f9a04,0x2fbb20,0x12030d3
#
*/
#define LINUX_CMDLINE   "/proc/cmdline"
#define LINUX_CMDLINE_SIZE   1024
int find_rootfs_path(char *root_dev)
{
	int ret = -1;
	struct stat file_stat;
	char buf[LINUX_CMDLINE_SIZE] = {0};
	// if(stat(LINUX_CMDLINE, &file_stat) == 0)  //proc file system: the file size still is zero!!!
	// {
	// 	buf = (char *) calloc(file_stat.st_size, 0);
	// 	printf("[%s %d]file_stat.st_size:%ld\n", __func__, __LINE__, file_stat.st_size);
	// }

	FILE *fp = fopen(LINUX_CMDLINE, "r");
	if (fp != NULL)
	{
		fread(buf, LINUX_CMDLINE_SIZE, 1, fp);
		if (strlen(buf) > 0)
		{
			char *pTmp = buf;
			char *pFind = NULL;
			char *pNext = NULL;
			while((pFind = strtok_r(pTmp, " ", &pNext)) != NULL)
			{
				if (strncmp(pFind, "root=", strlen("root=")) == 0)
				{
					strcpy(root_dev, pFind + strlen("root="));
					ret = 0;
					break;
				}
				pTmp = pNext;
			}
		}
		ret = 0;
		fclose(fp);
	}

	return ret;
}


int start_pthread(thread_func func, void * data)
{
	int ret = 0;
	
	pthread_t p_msg_hdl_Handle;
	pthread_attr_t p_msg_hdl_attr;
	/*create thread*/
	ret = pthread_attr_init(&p_msg_hdl_attr);
	if ( ret != 0 )
	{
		printf("Thread(p_msg_hdl_attr) attribute creation fail.\n");
		return -1;
	}

	ret = pthread_attr_setdetachstate(&p_msg_hdl_attr, PTHREAD_CREATE_DETACHED);
	if ( ret != 0 )
	{
		printf("Thread(p_msg_hdl_attr):Set attribute fail.\n");
		return -1;
	}

	ret = pthread_create(&p_msg_hdl_Handle, &p_msg_hdl_attr, (void *)func, data);
	if ( ret != 0 )
	{
		printf("Thread(p_msg_hdl_attr):Create thread fail!");
		return -1;
	}

	pthread_attr_destroy(&p_msg_hdl_attr);

	return 0;
}

int get_mac_by_interface(const char* interface, char* mac)
{
	int sock;
	struct ifreq ifr;
	int result = 0;

	sock = socket(PF_PACKET, SOCK_RAW, 0);
	if (sock < 0) {
		return 0;
	}
	
	memset(&ifr, 0, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, interface);
	
	if (!ioctl(sock, SIOCGIFHWADDR, &ifr)) {
		result = ((ifr.ifr_hwaddr.sa_family == ARPHRD_EUI64) ? 8 : 6);
		memcpy(mac, ifr.ifr_hwaddr.sa_data, result);
	}
	
	close(sock);
	return result;
}

/*
 check bridge has interface.
*/
#define MAX_PORTS	1024
int bridge_has_port(const char *brname, const char *port)
{
	int ret = 0;
	int i, err, count;
	struct ifreq ifr;
	char ifname[IFNAMSIZ];
	int ifindices[MAX_PORTS];
	int fd = -1;
	unsigned long args[4] = { BRCTL_GET_PORT_LIST,
				  (unsigned long)ifindices, MAX_PORTS, 0 };

	memset(ifindices, 0, sizeof(ifindices));
	strncpy(ifr.ifr_name, brname, IFNAMSIZ);
	ifr.ifr_data = (char *) &args;

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return 0;

	err = ioctl(fd, SIOCDEVPRIVATE, &ifr);
	if (err < 0) {
		printf("list ports for bridge:'%s' failed: %s\n", brname, strerror(errno));
		return 0;
	}

	for (i = 0; i < MAX_PORTS; i++) {
		if (!ifindices[i])
			continue;

		if (!if_indextoname(ifindices[i], ifname)) {
			// printf("can't find name for ifindex:%d\n",ifindices[i]);
			continue;
		}
		// printf("find name:%s for ifindex:%d\n",ifname, ifindices[i]);
		if (strcmp(port, ifname) == 0)
		{
			ret = 1;
			break;
		}
	}

	close(fd);

	return ret;
}

/*
	get data from the popen(cmd)
*/
int cmd_dump(char *cmdbuf, char *outbuf, int buflen)
{
	FILE *fp    = NULL;
	int  ilen   = 0;
	int  lentmp = 0;
	char tmpbuf[256];
	if(cmdbuf == NULL || outbuf == NULL)
	{
		return -1;
	}
	
	fp = popen(cmdbuf,"r");
	if(NULL == fp)
	{
		return -1;
	}

	while(1)
	{
		memset(tmpbuf,0,sizeof(tmpbuf));
		if(NULL == fgets(tmpbuf,sizeof(tmpbuf),fp))
		{
			break;
		}
		lentmp = strlen(tmpbuf);

		if((ilen + lentmp + 1) > buflen)
		{
			break;
		}
		memcpy(outbuf + ilen, tmpbuf, lentmp);
		ilen += lentmp;
	}
	
	pclose(fp);
	*(outbuf + ilen) = '\0';
	return ilen;
}

/*
	get the device first macaddress string format, the MFG set a mac-range for the device.
*/
int get_dev_first_mac(char * mac)
{
	int ret = -1;
	const char *cmd = "blapi_cmd system get_EtherAddr | grep EtherAddr | awk '{print $3,$4}'";
	char buf[128] = {0};
	if (cmd_dump(cmd, buf, sizeof(buf)) > 0)
	{
		//c8:ef:bc:a0:05:f0 c8:ef:bc:a0:05:f1
		char *ptmp = strstr(buf, " ");
		if (ptmp != NULL)
		{
			*ptmp = '\0';
			strcpy(mac, buf);
			ret = 0;
		}
	}

	return ret;
}

/*
	get the system date string format.
*/
void get_current_time2str(char *buf)
{
	time_t timer = time(NULL);;
	struct tm tmf;
	localtime_r(&timer, &tmf);
	sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", tmf.tm_year + 1900, tmf.tm_mon + 1, tmf.tm_mday,
            tmf.tm_hour, tmf.tm_min, tmf.tm_sec);
}

void get_threadname_bypid(pid_t pid, pid_t tid, char *name)
{
	if (name != NULL)
	{
		char  file_name[128] = {0};
		int   fd = -1;
		char buf[128] = {0};

		snprintf(file_name, sizeof(file_name), "/proc/%d/task/%d/comm", pid, tid);
		fd = open( file_name, O_RDONLY, 0666 );
		if( fd != -1 )
		{
			read(fd, buf, sizeof(buf));
			char *ptmp = strstr(buf, "\n");
			if( ptmp != NULL )
			{
				*ptmp = '\0';
			}
			strcpy(name, buf);
			close(fd);
		}
	}
}