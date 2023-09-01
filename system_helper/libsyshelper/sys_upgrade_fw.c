#include "libsysprivate.h"

//fw type:
#define UG_ROMFILE				1
#define UG_TCLINUX				4
#define UG_TCLINUX_ALLINONE		5   //not supported.

// #if defined(TCSUPPORT_CMCCV2)
// #define TRX_SIZE 	 sizeof(struct trx_header)
// #endif

//upgrade type: 
#define NO_OPERATION  			0
#define NO_HEADER				1
#define HTML_HEADER				2
#if defined(TCSUPPORT_CT_PHONEAPP)
#define PHONE_NO_HEADER			3
#endif
#define CTCDBUS_NO_HEADER		4
#define NP_NO_HEADER			5

#define TRX_MAGIC2				0x32524448	/* "for tclinux" */
#define CHECKSUM_TEMP_BUF_SIZE 	4096


#define DEVICEPARASTATIC_PATH	"/etc/deviceParaStatic.conf"

typedef enum check_image_status {
	IMG_CHECK_OK            = 0,    //image is correct and matched the board.
	IMG_BAD_DATA            = -1,   //image data is not correct.
	IMG_TYPE_NOT_SUPPORT    = -2,   //image type is not matched the board.
	IMG_DATA_SIZE_ERR       = -3,   //image size too big or small
	IMG_NO_HEADER           = -4,   //image didn't have header.
	IMG_OTHER_ERR           = -5    //other unknow error.
} Check_Image_Status_E;

//from the header file:install_bsp/tools/trx/trx.h
struct trx_header {
        unsigned int magic;                     /* "HDR0" */
        unsigned int header_len;    /*Length of trx header*/
        unsigned int len;                       /* Length of file including header */
        unsigned int crc32;                     /* 32-bit CRC from flag_version to end of file */
        unsigned char version[32];  /*firmware version number*/
        unsigned char customerversion[32];  /*firmware version number*/
//      unsigned int flag_version;      /* 0:15 flags, 16:31 version */
#if 0
        unsigned int reserved[44];      /* Reserved field of header */
#else
        unsigned int kernel_len;        //kernel length
        unsigned int rootfs_len;        //rootfs length
        unsigned int romfile_len;       //romfile length
        #if 0
        unsigned int reserved[42];  /* Reserved field of header */
        #else
        unsigned char Model[32];
        unsigned int decompAddr;//kernel decompress address
#if 1 //defined(TCSUPPORT_CMCCV2)
        unsigned int openjdk_len;      /*openjdk length*/
        unsigned int osgi_len;          /*osgi length*/
        unsigned int imageflag;         /*tclinux+openjdk+osgi 1;tclinux+openjdk 2;tclinux+osgi 3*/
        unsigned int reserved[29];  /* Reserved field of header */
#else

        unsigned int saflag;
        unsigned int saflen;
        unsigned int reserved[30];  /* Reserved field of header */
#endif
#endif
#if defined(TCSUPPORT_SECURE_BOOT_V2)
        SECURE_HEADER_V2 sHeader;
#elif defined(TCSUPPORT_SECURE_BOOT_V1) || defined(TCSUPPORT_SECURE_BOOT_FLASH_OTP)
        SECURE_HEADER_V1 sHeader;
#endif
#endif
};


static const uint32_t crc_32_tab[] = { /* CRC polynomial 0xedb88320 */
0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

#define UPDC32(octet,crc) (crc_32_tab[((crc) ^ (octet)) & 0xff] ^ ((crc) >> 8))

static uint32_t calculate_crc32(int imagefd, int32_t len)
{
	int32_t length=len;	
	int check_len=0, i=0;
	char flag=0;/*for stop loop*/
	unsigned char *buf;
	uint32_t crc;
	int res = 0;
	
      	crc = 0xFFFFFFFF;
	
	/*Because read 1 byte at a time will spent much time, 
		we read a bigger size at a time and use this space to 
		do checksum. */
   	buf = (unsigned char *)malloc(CHECKSUM_TEMP_BUF_SIZE);
	if(buf == NULL)
		return 0;
 	while(flag == 0){
		/*decide add length*/
		if(length <= CHECKSUM_TEMP_BUF_SIZE){
			check_len=length;
			flag = 1;	
		}else{
			check_len = CHECKSUM_TEMP_BUF_SIZE;
		}

		length -= check_len;
		res = read(imagefd, buf, check_len); 
		
    		for(i=0;i < check_len;i++)
    		{
    			crc = UPDC32(*(buf+i), crc);	
		}  
	}  	

    free(buf);
    return ((uint32_t)crc);
}

//not use the blapi_system_xxx, just use the linux system informaiton.
/*
blapi_cmd system set_boot_flag tclinux_slave/tclinux

[do_blapi_system_get_mtdsize]296: mtd_size = 42991616,mtd_erasesize=131072
# cat /proc/mtd
dev:    size   erasesize  name
mtd0: 00080000 00020000 "bootloader"
mtd1: 00040000 00020000 "romfile"
mtd2: 002fbb28 00020000 "kernel"
mtd3: 01210000 00020000 "rootfs"
mtd4: 02900000 00020000 "tclinux"
mtd5: 002fbb28 00020000 "kernel_slave"
mtd6: 01210000 00020000 "rootfs_slave"
mtd7: 02900000 00020000 "tclinux_slave"
mtd8: 02200000 00020000 "java"
mtd9: 02900000 00020000 "yaffs"
mtd10: 01e00000 00020000 "data"
mtd11: 00240000 00020000 "reservearea"
# df -f
df: invalid option -- 'f'
BusyBox v1.00 (2023.08.05-07:21+0000) multi-call binary

Usage: df [-k] [FILESYSTEM ...]

# df
Filesystem           1k-blocks      Used Available Use% Mounted on
/dev/mtdblock6           18560     18560         0 100% /
/dev/mtdblock10          30720     11944     18776  39% /data
# cat /proc/mounts
/dev/root / squashfs ro,relatime 0 0
proc /proc proc rw,relatime 0 0
none /tmp ramfs rw,relatime 0 0
devpts /dev/pts devpts rw,relatime,mode=600,ptmxmode=000 0 0
sysfs /sys sysfs rw,relatime 0 0
none /sys/kernel/debug debugfs rw,relatime 0 0
/dev/mtdblock10 /data jffs2 rw,relatime 0 0
# cat /proc/cmdline
sdram_conf=0x00108893 vendor_name=ECONET Technologies Corp. product_name=xPON ONU ethaddr=c8:ef:bc:a0:05:f0 snmp_sysobjid=1.2.3.4.5 country_code=ff ether_gpio=0c power_gpio=0b0b username=root password=Pon521 dsl_gpio=0b internet_gpio=02 multi_upgrade_gpio=0b020400000000000000000000000000 onu_type=2 qdma_init=33 root=/dev/mtdblock6 ro console=ttyS0,115200n8 earlycon bootflag=1 serdes_sel=0 tclinux_info=0x14ffa07,0x2024,0x2f9a04,0x2fbb20,0x1203ba5,0x14fef33,0x2024,0x2f9a04,0x2fbb20,0x12030d3
#

*/

static int get_current_boot_flag(char * flag)
{
	char root_dev[128] = {0};
	#if 0  //get the rootfs from mount point.
	find_path_mountpoint("/", root_dev);
	#else  //get the rootfs from cmdline.
	find_rootfs_path(root_dev);
	#endif
	//printf(" root_dev=%s\n", root_dev);
	if (strcmp(root_dev, "/dev/mtdblock4") == 0)
	{
		*flag = 1;
	}
	else
	{
		*flag = 0;
	}
}
static int set_boot_flag(const char * img)
{
	char cmd_buf[128] = {0};
	sprintf(cmd_buf, "/userfs/bin/blapi_cmd system set_boot_flag %s", img);
	system(cmd_buf);

	return 0;
}

static int get_mtdsize(char *mtd, int *mtd_size)
{
	/*
	# cat /proc/mtd
		dev:    size   erasesize  name
		mtd0: 00080000 00020000 "bootloader"
		mtd1: 00040000 00020000 "romfile"
		mtd2: 002fbb28 00020000 "kernel"
		mtd3: 01210000 00020000 "rootfs"
		mtd4: 02900000 00020000 "tclinux"
		mtd5: 002fbb28 00020000 "kernel_slave"
		mtd6: 01210000 00020000 "rootfs_slave"
		mtd7: 02900000 00020000 "tclinux_slave"
		mtd8: 02200000 00020000 "java"
		mtd9: 02900000 00020000 "yaffs"
		mtd10: 01e00000 00020000 "data"
		mtd11: 00240000 00020000 "reservearea"
	*/
	char find_name[128] = {0};

	if (strcmp(mtd, "tclinux") == 0)
	{
		strcpy(find_name, "mtd4:");
	}
	else
	{
		strcpy(find_name, "mtd7:");
	}

	char line_buf[128] ={0};
	FILE *fp = fopen("/proc/mtd", "r");
	if (fp != NULL)
	{
		while( fgets(line_buf, sizeof(line_buf), fp) >0 )
		{
			if(strstr(line_buf, find_name) != NULL)
			{
				char *pFind = NULL;
				char *pNext = line_buf;
				pFind =strtok_r(pNext, " ", &pNext);//skip the first item.
				pFind = strtok_r(pNext, " ", &pNext);//second item.
				*mtd_size = strtoul(pFind, NULL, 16);
				break;
				
			}
		}
		fclose(fp);
	}

	return 0;
}

/*modify the return value 
before-------return trx_header_len if success
now---------return 0 if success
*/
static Check_Image_Status_E image_check_tc(int imagefd, char firmware_type, char *file_path,char *mtd, unsigned  int length)
{
	char img_buf[sizeof(struct trx_header)];

	struct trx_header *trx = (struct trx_header *) img_buf;
	int mtd_size = 0;
	int fd, buflen;
	int ret = -1;
	int readmtd = 1;

	unsigned char curmac[6] = {0};
	int addr = 0;
	int size = 0;
	char bootFlag = -1;

	if(firmware_type == UG_ROMFILE)
	{
		strcpy(mtd,"romfile");
	}
	else if(firmware_type == UG_TCLINUX)
	{
		get_current_boot_flag(&bootFlag);
		if (bootFlag == 1)
		{
			strncpy(mtd, "tclinux", 31);
		}
		else
		{
			strncpy(mtd, "tclinux_slave", 31);
		}
		slog_printf(0,1,"bootFlag:%s", mtd);

	}
	else
	{
		//printf("image_check_tc:not support this type\n");
		slog_printf(0,1,"image_check_tc:not support this type:%d\n", firmware_type);

		return -1;

	}
	
	if ( readmtd )
	{
		ret = get_mtdsize(mtd, &mtd_size);
		if(ret < 0)
			return -1;
	}
	
	switch(firmware_type)
	{
		case UG_ROMFILE:
			/*Check the romfile image is vaildly or not.*/
			close(imagefd);/*first close,because in function 'check_romfile' will open the file*/
			ret = -1;//check_romfile(file_path);
			if(ret < 0)
			{
				// printf("image_check_tc:romfile check failed!\n");
				slog_printf(0,1,"image_check_tc:romfile check failed!\n");
				return -1;
			}
			break;
			
		case UG_TCLINUX:
			/*Check the tclinux is vaildly or not.*/
			buflen = read(imagefd, img_buf, sizeof(struct trx_header));
			/*max len check*/
			if(mtd_size < trx->len)
			{
				// printf("Image too big for partition: %s\n", mtd);
				slog_printf(0,1,"Image too big for partition: %s\n", mtd);

				return -1;
			}
			/*mix len check*/
			if (buflen < sizeof(struct trx_header)) 
			{
				// printf("Could not get image header, file too small (%d bytes)\n", buflen);
				slog_printf(0,1,"Could not get image header, file too small (%d bytes)\n", buflen);
				return -1;
			}
			/*magic check*/
			if ((trx->magic != TRX_MAGIC2) || (trx->len < sizeof(struct trx_header)))
			{

				printf("filesystem: Bad trx header\n");
				slog_printf(0,1,"filesystem: Bad trx header\n");
				return -1;
			}
			/*crc check*/
			if(trx->crc32 != calculate_crc32(imagefd, (trx->len-sizeof(struct trx_header))))
			{
				printf("crc32 check fail\n");
				slog_printf(0,1,"crc32 check fail\n");
				return -1;
			}
			char *tmp = NULL;
			tmp = strstr(trx->Model, "\r\n");
			if (tmp != NULL)
				*tmp = '\0';
			tmp = strstr(trx->Model, "\n");
			if (tmp != NULL)
				*tmp = '\0';
			tmp = strstr(trx->customerversion, "\r\n");
			if (tmp != NULL)
				*tmp = '\0';
			tmp = strstr(trx->customerversion, "\n");
			if (tmp != NULL)
				*tmp = '\0';
			// printf("[%s %d]trx->Model:%s, trx->customerversion:%s\n", __func__,__LINE__, trx->Model, trx->customerversion);
			slog_printf(0,1,"Image Model:%s, customerversion:%s, imageflag:%d\n", trx->Model, trx->customerversion, trx->imageflag);
			if (strlen(trx->customerversion) != 0)
			{
				char upgrade_force[32] = {0};
				int check_model = 1; //by default, we will check the FW's module, you can set the upgrade_force to 0 bypass the check.
				if(check_model)
				{
					char ModelName[32] = {0};//"IF2243";//hardcode.
					int tmp_ret = get_profile_key_val( "ModelName=", ModelName, sizeof(ModelName), DEVICEPARASTATIC_PATH);
					if (tmp_ret != -1)//if(cfg_obj_get_object_attr(DEVICEINFO_DEVPARASTATIC_NODE, "ModelName", 0, ModelName, sizeof(ModelName)) > 0)
					{
						slog_printf(0,1,"Do the model check. Image Model:%s, board Model:%s\n", trx->Model, ModelName);
						if (strcmp(ModelName, trx->Model) != 0)
						{
							slog_printf(0,1,"Image Model:%s, board Model:%s. The Model is not matched!!!\n", trx->Model, ModelName);
							return IMG_TYPE_NOT_SUPPORT;
						}
					}
				}
			}
			else //for old FW, didn't check the model name!!
			{
				slog_printf(0,1,"skip the model check. Image Model:%s, customerversion:%s\n", trx->Model, trx->customerversion);
			}
			break;
		default:
			slog_printf(0,1,"Image firmware_type:%s not matched!!!\n", firmware_type);
			return IMG_TYPE_NOT_SUPPORT;
	}

	return IMG_CHECK_OK;
}

// static uint32_t determine_type1(char *file_path, unsigned char *firmware_type)
// {
// 	struct stat buf;

// 	if(stat(AP_UPGRADE_FW_NAME, &buf) == 0)
// 	{
// 		*firmware_type = UG_TCLINUX;
// 		strncpy(file_path, AP_UPGRADE_FW_NAME, 128);
// 	}

// 	return buf.st_size;
// }


static int upgarde_fw(const char * file_name, int type, bool bootflag)
{
	unsigned int offset=0 , length=0;
	unsigned char firmware_type;
	//char file_path[128] = {0};
	char cmd_buf[256] = {0};
	char img[32] = {0};
	Check_Image_Status_E header_len=0;
	//type = NP_NO_HEADER
	if ( type == NP_NO_HEADER)//
	{
		struct stat buf;
		if(stat(file_name, &buf) == 0)
		{
			firmware_type = UG_TCLINUX;
			//strncpy(file_path, file_name, 128);
			length = buf.st_size;
		}
		offset = 0;
	}

	if (NO_OPERATION != type)
		slog_printf(0,1,"sys_upgrade_firmware format type:%d, file_path:%s\n", type, file_name);
		// sys_upgrade_firmware format type:2, web_upgrade_flag:1

	
	// blapi_system_secure_upgrade(256, offset, file_path, firmware_type);
	/*check the firmware.*/
	int fd = open(file_name, O_RDONLY);
	if(fd < 0){
		unlink(file_name);
		return -1;
	}
	else
	{

		//lseek(fd,offset+blapi_system_secure_trx(256, offset, file_path, firmware_type),SEEK_CUR);
		lseek(fd, 0, SEEK_SET);
		header_len = image_check_tc(fd,firmware_type,file_name,img,length);

		slog_printf(0,1,"check the image resutl: %d(%s)\n", header_len, (header_len == IMG_CHECK_OK)? "success!!!":"failed!!!");

		if (header_len != IMG_CHECK_OK) //if( header_len == -1)
		{
			if(UG_ROMFILE != firmware_type)
			{
				close(fd);
			}
			unlink(file_name);
			return -1;
		}
		if(UG_ROMFILE != firmware_type)
		{
			close(fd);
		}
	}


	printf("ONU is ready to upgrade!\n");

	printf("Please Waiting,CPE is erasing flash......\n");

	snprintf(cmd_buf, sizeof(cmd_buf), "/userfs/bin/mtd -f write %s %d %d %s",file_name, length, offset ,img);
	slog_printf(0,1,"cmd_buf:%s\n", cmd_buf);
	system(cmd_buf);
	
	set_boot_flag(img);

	if(bootflag)
	{
		system("reboot -d 5 &");
		slog_printf(0,1,"---> to reboot by upgrade.\n");
		return 0;
	}

	return 0;

}

int sys_upgrade_firmware(const char * file_name, int type, bool bootflag)
{
	int retval=0;
	char msg_value[32] = {0};
	int ret = -1;

	if (NO_OPERATION != type)
		slog_printf(0,1,"----------------------> start the Firmware upgrading...\n");

	retval=upgarde_fw(file_name, type, bootflag);
	if(retval == 0)
	{
		/*set firmware upgrade status*/
		slog_printf(0,1,"Image has been upgraded, next waiting reboot.\n");
	}
	else
	{
		/*set firmware upgrade status*/
		slog_printf(0,1,"Image has been upgraded FAIL.\n");
	}

	slog_printf(0,1,"----------------------> end the Firmware upgrading: retval=%d...\n", retval);
	sync();

	return retval;
}