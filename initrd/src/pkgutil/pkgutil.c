/*******************************************************
Copyright (C) abloomy.
All rights reserved.
Describle	:	 For building and verifying image
Author		:	 ou,zhongyun	 
Date		:	 2013.02.20
*******************************************************/
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h> 
#include <signal.h>
#include <limits.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>

#include "pkgutil.h"

#define SIGNAME(s) strsignal(s)

#define DEFAUT_ENC_PASS	"Abl@Dev^KK"

static const int IMG_HDR_LEN = sizeof(struct image_header);


static int execute_cmd(const char *cmd)
{
	int ret;
	ret = system(cmd);
	if(ret == -1) {
		return -1;
	}
	else if(ret == 127) {
		return -2;
	}
	else if(ret != 0) {
		/*
		if(WIFSIGNALED(ret)) {
			return -3;
		}
		else{
			return -4;
		}
		*/

		return -4;
	}

	return 0;
}

int data_2_hexstr(char **info, void *data, int datalen)
{
	int i;
	char *buf;
	unsigned char *ptr;

	if (data==NULL || datalen<=0 || info== NULL)
		return 0;

	ptr = (unsigned char *)data;
	buf = malloc(datalen*2+1);
	if (buf == NULL)
		return -1;

	for (i=0; i<datalen; i++) {
		sprintf(buf+2*i, "%02x", *ptr);
		ptr++;
	}
	buf[datalen*2] = '\0';

	*info = buf;

	return datalen*2;
}

char *imghdr_2_str(const struct image_header *imghdr, char *str, int strlen)
{
	char *md5str = NULL;
	char *sigstr = NULL;
	int ret;

	ret = data_2_hexstr(&md5str, (void*)imghdr->md5, MD5_DIGEST_LENGTH);
	ret = data_2_hexstr(&sigstr, (void*)imghdr->signature, RSA_SIG_LEN);
	
	snprintf(str, strlen, 
			"    Version:  %d\n"
			"    HdrLEN:   %d\n"
			"    Encrypt:  %d\n"
			"    HasSig:   %d\n"
			"    MD5:      %s\n"
			"    SIG:      %s\n",
			imghdr->version, imghdr->hdr_len, 
			imghdr->data_encrypt, imghdr->has_sig,
			md5str ? md5str : "",
			sigstr ? sigstr : "");

	if (md5str) {
		free(md5str);
	}
	
	if (sigstr) {
		free(sigstr);
	}
	
	return 0;
}

int get_file_md5(const char *file, unsigned char *md5)
{
	unsigned char hash[MD5_DIGEST_LENGTH];
	MD5_CTX ctx;
	FILE *fd;
	char buff[1024];
	int len;

	if (file == NULL || md5 == NULL) {
		return -1;
	}

	fd = fopen(file, "r");
	if (fd == NULL) {
		printf("open file %s failed\n", file);
		return -2;
	}

	memset(hash, 0, MD5_DIGEST_LENGTH);

	MD5_Init(&ctx);

	while(!feof(fd) && !ferror(fd)) {
		len = fread(buff, 1, 1024, fd);
		if (len > 0) {
			MD5_Update(&ctx, buff, len);
		}
	}
	
	MD5_Final(hash, &ctx);
	memcpy(md5, hash, MD5_DIGEST_LENGTH);

	fclose(fd);
	return 0;
}


int get_file_img_hdr(const char *infile, struct image_header *ih)
{
	struct stat st;
	int ret, len;
	FILE *fd;

	if (infile == NULL || ih == NULL) {
		printf("%s:%d  invalid params\n", __FUNCTION__, __LINE__);
		return -1;
	}

	ret = stat(infile, &st);
	if (ret == -1) {
		printf("check file %s exist or not\n", infile);
		return -2;
	}

	if (st.st_size < IMG_HDR_LEN) {
		printf("file %s was too short\n", infile);
		return -3;
	}

	fd = fopen(infile, "r");
	if (fd == NULL) {
		printf("open file %s failed\n", infile);
		return -4;
	}

	len = fread(ih, 1, IMG_HDR_LEN, fd);
	if (len < IMG_HDR_LEN) {
		printf("file %s was too short\n", infile);
		fclose(fd);
		return -5;
	}

	fclose(fd);

	return 0;
}

int create_img(const char *infile, const char *outfile, int enc, int sig)
{
	struct image_header img_hdr;
	char cmd[1024];
	int ret;
	FILE *fdout;
	int wlen;

	memset(&img_hdr, 0, IMG_HDR_LEN);
	img_hdr.version = IMG_VERSION;
	img_hdr.hdr_len = (unsigned char)IMG_HDR_LEN;
	img_hdr.data_encrypt = enc;
	img_hdr.has_sig = sig; // Fix me: using signature
	
	ret = get_file_md5(infile, img_hdr.md5);
	if (ret != 0 ) {
		printf("Cann't get md5 of %s\n", infile);
		return -1;	
	}

	// Fix me: Should compute the signature here
	if (sig) {

	}
	
	fdout = fopen(outfile, "w+");
	if (fdout == NULL) {
		printf("%s:%d cann't open out file %s\n", __FUNCTION__, __LINE__, outfile);
		return -1;
	}

	wlen = fwrite(&img_hdr, sizeof(char), IMG_HDR_LEN, fdout);
	if (wlen != IMG_HDR_LEN) {
		printf("%s:%d write image header to %s failed\n", __FUNCTION__, __LINE__, outfile);
		fclose(fdout);
		return -2;
	}

	fclose(fdout);

	// encryp data
	snprintf(cmd, 1024, 
				"openssl enc -e -des -pass pass:%s -in %s >> %s", 
				DEFAUT_ENC_PASS, infile, outfile);
	ret = execute_cmd(cmd);

	return ret;
}

int verify_img(const char *infile)
{
	struct stat st;
	struct image_header img_hdr;
	int ret;
	int tmpfd;
	char cmd[1024];
	unsigned char md5[MD5_DIGEST_LENGTH];
	char tmpfile[128] = "/tmp/pkg/img.XXXXXX";


	if (infile == NULL) {
		printf("should input infile name\n");
		return -1;
	}

	ret = get_file_img_hdr(infile, &img_hdr);
	if (ret != 0) {
		printf("Cann't get img hdr from  file %s\n", infile);
		return -2;
	}

	// verify img header
	if (img_hdr.version != IMG_VERSION)
		return -3;

	if (img_hdr.hdr_len != IMG_HDR_LEN)
		return -4;

	tmpfd = mkstemp(tmpfile);
	if (tmpfd == -1) {
		printf("Cann't create tmp file\n");
		return -5;
	}
	close(tmpfd);

	
	ret = stat(infile, &st);
	if (ret == -1) {
		printf("check file %s exist or not\n", infile);
		return -6;
	}

	if (img_hdr.data_encrypt) {
		snprintf(cmd, 1024, 
					"tail -c %ld %s | "
					"openssl enc -d -des -pass pass:%s -out %s", 
					(st.st_size - IMG_HDR_LEN), infile, 
					DEFAUT_ENC_PASS, tmpfile);
	} else {
		snprintf(cmd, 1024, 
					"tail -c %ld %s > %s", 
					(st.st_size - IMG_HDR_LEN), infile, tmpfile);
	}

	ret = execute_cmd(cmd);
	if (ret != 0) {
		printf("cann't extract img %s\n", infile);
		ret = -7;
		goto Remove_TMP;
	}

	ret = get_file_md5(tmpfile, md5);
	if (ret != 0) {
		printf("get MD5 failed \n");
		ret = -8;
		goto Remove_TMP;
	}

	if (memcmp(md5, img_hdr.md5, MD5_DIGEST_LENGTH) != 0) {
		char *md5str1, *md5str2;

		data_2_hexstr(&md5str1, img_hdr.md5, MD5_DIGEST_LENGTH);
		data_2_hexstr(&md5str2, img_hdr.md5, MD5_DIGEST_LENGTH);
		
		printf("File be modified: Orig MD5: %s current MD5: %s\n", 
			md5str1, md5str2);

		ret = -9;
	}

	// Fix me: to check the signature
	if (img_hdr.has_sig) {

	}
	
Remove_TMP:
	unlink(tmpfile);
	
	return ret;
}


int unpkg_img(const char *infile, const char *outfile)
{
	int ret;
	char cmd[1024];
	struct stat st;
	struct image_header imghdr;

	if (infile == NULL || outfile == NULL) {
		printf("Input params invalied\n");
		return -1;
	}
	
	ret = verify_img(infile);
	if (ret != 0) {
		printf("file %s was a invalid image\n", infile);
		return -2;
	}

	ret = stat(infile, &st);
	if (ret == -1) {
		printf("check file %s exist or not\n", infile);
		return -2;
	}

	get_file_img_hdr(infile, &imghdr);

	if (imghdr.data_encrypt) {
		snprintf(cmd, 1024, 
					"tail -c %ld %s | "
					"openssl enc -d -des -pass pass:%s -out %s", 
					(st.st_size - IMG_HDR_LEN), infile, 
					DEFAUT_ENC_PASS, outfile);
	} else {
		snprintf(cmd, 1024, 
					"tail -c %ld %s > %s", 
					(st.st_size - IMG_HDR_LEN), infile, outfile);
	}

	ret = execute_cmd(cmd);
	if (ret != 0) {
		printf("unpkg img %s failed\n", infile);
	}
	
	return ret;
}


int extract_img(const char *infile, const char *outdir)
{
	int ret, tmpfd;
	char cmd[1024];
	char tmpfile[128] = "/tmp/pkg/img.XXXXXX";


	ret = verify_img(infile);
	if (ret != 0) {
		printf("file %s was a invalid image\n", infile);
		return -1;
	}
	
	tmpfd = mkstemp(tmpfile);
	if (tmpfd == -1) {
		printf("Cann't create tmp file\n");
		return -2;
	}
	close(tmpfd);

	ret = unpkg_img(infile, tmpfile);
	if (ret != 0) {
		printf("unpkg img %s failed\n", infile);
	}
	else {
		snprintf(cmd, 1024, "tar zxf %s -C %s", tmpfile, outdir);
		
		ret = execute_cmd(cmd);
		if (ret != 0) {
			printf("extract decrypted img %s(%s) failed\n", infile, tmpfile);
		}
	}

	// remove temp file
	unlink(tmpfile);

	return ret;
}

