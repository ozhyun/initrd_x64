/*******************************************************
Copyright (C) abloomy.
All rights reserved.
Describle	:	 For building and verifying image
Author		:	 ou,zhongyun	 
Date		:	 2013.02.20
*******************************************************/
#ifndef __PKGUTIL_H__
#define __PKGUTIL_H__

#include <openssl/ssl.h>
#include <openssl/des.h>
#include <openssl/md5.h>
#include <openssl/err.h>

#define RSA_SIG_LEN			128

#define IMG_VERSION			1

#define IMG_HAS_SIGNATURE		1
#define IMG_NO_SIGNATURE		0

#define IMG_ENCRYPT			1
#define IMG_NOT_ENCRYPT		0


struct image_header {
	unsigned char version;
	unsigned char hdr_len;
	unsigned char data_encrypt;
	unsigned char has_sig;
	unsigned char md5[MD5_DIGEST_LENGTH];
	unsigned char signature[RSA_SIG_LEN];
};

int data_2_hexstr(char **info, void *data, int datalen);

char *imghdr_2_str(const struct image_header *imghdr, char *str, int strlen);

int get_file_md5(const char *file, unsigned char *md5);

int get_file_img_hdr(const char *infile, struct image_header *ih);

int create_img(const char *infile, const char *outfile, int enc, int sig);

int verify_img(const char *infile);

int unpkg_img(const char *infile, const char *outfile);

int extract_img(const char *infile, const char *outdir);

#endif  //#ifndef __PKGUTIL_H__
