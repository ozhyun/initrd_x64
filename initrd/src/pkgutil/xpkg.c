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

enum prog_opt {
	OPT_ERR 		= 0,
	OPT_MD5 		= 1,
	OPT_IMGHDR 	= 2,
	OPT_CREATE 	= 3,
	OPT_VERIFY 	= 4,
	OPT_EXTRACT	= 5,
	OPT_UNPKG	= 6,
	OPT_HELP		= 7,
};

static void full_usage(const char *prog)
{
	printf("Usage: %s\n", prog);
	printf("\t md5 <file>                   print file's MD5\n");
	printf("\t imghdr <img>                 print img header\n");
	printf("\t create <infile>  <img>       "
						"create img from [infile], write into [img]\n");
	printf("\t verify <img>                 verify image file\n");
	printf("\t extract <img> <outdir>       "
						"extract image <img> into dir <outdir>\n");	
	printf("\t unpkg <img> <outfile>        "
						"unpkg image <img> into <outfile>\n");
	printf("\t -??                          print this usage\n");
}

static void usage(const char *prog)
{
	printf("Usage: %s\n", prog);
	printf("\t md5 <file>                   print file's MD5\n");
	printf("\t verify <img>                 verify image file\n");
	printf("\t extract <img> <outdir>       "
						"extract image <img> into dir <outdir>\n");	
}

static enum prog_opt get_opt(const char *option)
{
	enum prog_opt opt = OPT_ERR;

	if (option == NULL)
		opt = OPT_ERR;
	else if (strcmp(option, "md5") == 0)
		opt = OPT_MD5;
	else if (strcmp(option, "imghdr") == 0)
		opt = OPT_IMGHDR;
	else if (strcmp(option, "create") == 0)
		opt = OPT_CREATE;
	else if (strcmp(option, "verify") == 0)
		opt = OPT_VERIFY;
	else if (strcmp(option, "extract") == 0)
		opt = OPT_EXTRACT;
	else if (strcmp(option, "unpkg") == 0)
		opt = OPT_UNPKG;
	else if (strcmp(option, "-??") == 0)
		opt = OPT_HELP;

	return opt;
}

int main(int argc, char *argv[])
{
	int ret;
	char *prog;
	enum prog_opt opt;
	struct image_header img_hdr;
	unsigned char md5[MD5_DIGEST_LENGTH];
	char *md5str = NULL;
	char *infile = NULL;
	char *infile_basename = NULL;
	char *outfile = NULL;

	prog = basename(argv[0]);

	if (argc < 2) {
		usage(prog);
		return 1;
	}
	opt = get_opt(argv[1]);

	ret = 0;

	switch(opt) {
	case OPT_MD5:
		infile = argv[2];
		ret = get_file_md5(infile, md5);
		if (ret != 0) {
			printf("Cann't get md5 of file %s\n", argv[2]);
		}
		else {
			ret = data_2_hexstr(&md5str, md5, sizeof(md5));
			if (md5str == NULL) {
				printf("get md5 error\n");
			}
			else {
				printf("%s    %s\n", md5str, infile);
				free(md5str);
				md5str = NULL;
			}
		}
		break;

	case OPT_IMGHDR:
		infile = argv[2];
		ret = get_file_img_hdr(infile, &img_hdr);
		if (ret != 0) {
			printf("Cann't get img hdr from file %s\n", infile);
		} else {
			char buff[1024];
			imghdr_2_str(&img_hdr, buff, 1024);
			printf("\"%s\" IMG Hdr:\n%s\n", infile, buff);
		}
		break;
	
	case OPT_CREATE:
		infile = argv[2];
		outfile = argv[3];
		ret = create_img(infile, outfile, IMG_ENCRYPT, IMG_NO_SIGNATURE);
		if (ret != 0) {
			printf("create img %s from %s failed\n", outfile, infile);
		} else {
			printf("create img %s success\n", outfile);
		}
		break;

	case OPT_VERIFY:
		infile = argv[2];
		ret = verify_img(infile);
		if (ret != 0) {
			printf("Image %s be modified\n", infile);
		} else {
			printf("Image %s be verified success\n", infile);
		}
		break;
		
	case OPT_EXTRACT:
		infile = argv[2];
		infile_basename = basename(argv[2]);
		outfile = argv[3];
		ret = extract_img(infile, outfile);
		if (ret != 0) {
			printf("Extract Image %s into %s failed\n", infile_basename, outfile);
		} else {
			//printf("Extract Image %s into %s success\n", infile, outfile);
			printf("Extract Image %s success\n", infile_basename);
		}
		break;

	case OPT_UNPKG:
		infile = argv[2];
		infile_basename = basename(argv[2]);
		outfile = argv[3];
		ret = unpkg_img(infile, outfile);
		if (ret != 0) {
			printf("Unpkg Image %s into %s failed\n", infile_basename, outfile);
		} else {
			printf("Unpkg Image %s into %s success\n", infile_basename, outfile);
		}
		break;

	case OPT_HELP:
		full_usage(prog);
		break;

	default:
		usage(prog);
		break;
	}

	return ret;
}

