/* ----------------------------------------------------------------------- *
 *
 *   Copyright 1998-2008 H. Peter Anvin - All Rights Reserved
 *   Copyright 2009 Intel Corporation; author: H. Peter Anvin
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, Inc., 53 Temple Place Ste 330,
 *   Boston MA 02111-1307, USA; either version 2 of the License, or
 *   (at your option) any later version; incorporated herein by reference.
 *
 * ----------------------------------------------------------------------- */

/*
 * syslinux.c - Linux installer program for SYSLINUX
 *
 * Hacked up for DOS.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "mystuff.h"

#include "syslinux.h"
#include "libfat.h"

const char *program = "syslinux";	/* Name of program */
uint16_t dos_version;

#ifdef DEBUG
# define dprintf printf
void pause(void)
{
    uint16_t ax;
    
    asm volatile("int $0x16" : "=a" (ax) : "a" (0));
}
#else
# define dprintf(...) ((void)0)
# define pause() ((void)0)
#endif

void __attribute__ ((noreturn)) usage(void)
{
    puts("Usage: syslinux [-sfmar][-d directory] <drive>: [bootsecfile]\n");
    exit(1);
}

void unlock_device(int);

void __attribute__ ((noreturn)) die(const char *msg)
{
    unlock_device(0);
    puts("syslinux: ");
    puts(msg);
    putchar('\n');
    exit(1);
}

void warning(const char *msg)
{
    puts("syslinux: warning: ");
    puts(msg);
    putchar('\n');
}

/*
 * read/write wrapper functions
 */
int creat(const char *filename, int mode)
{
    uint16_t rv;
    uint8_t err;

    dprintf("creat(\"%s\", 0x%x)\n", filename, mode);

    rv = 0x3C00;
    asm volatile ("int $0x21 ; setc %0"
		  : "=bcdm" (err), "+a" (rv)
		  : "c" (mode), "d" (filename));
    if (err) {
	dprintf("rv = %d\n", rv);
	die("cannot open ldlinux.sys");
    }

    return rv;
}

void close(int fd)
{
    uint16_t rv = 0x3E00;

    dprintf("close(%d)\n", fd);

    asm volatile ("int $0x21":"+a" (rv)
		  :"b"(fd));

    /* The only error MS-DOS returns for close is EBADF,
       and we really don't care... */
}

int rename(const char *oldname, const char *newname)
{
    uint16_t rv = 0x5600;	/* Also support 43FFh? */
    uint8_t err;

    dprintf("rename(\"%s\", \"%s\")\n", oldname, newname);

    asm volatile ("int $0x21 ; setc %0":"=bcdm" (err), "+a"(rv)
		  :"d"(oldname), "D"(newname));

    if (err) {
	dprintf("rv = %d\n", rv);
	warning("cannot move ldlinux.sys");
	return rv;
    }

    return 0;
}

ssize_t write_file(int fd, const void *buf, size_t count)
{
    uint16_t rv;
    ssize_t done = 0;
    uint8_t err;

    dprintf("write_file(%d,%p,%u)\n", fd, buf, count);

    while (count) {
	rv = 0x4000;
	asm volatile ("int $0x21 ; setc %0":"=abcdm" (err), "+a"(rv)
		      :"b"(fd), "c"(count), "d"(buf));
	if (err || rv == 0)
	    die("file write error");

	done += rv;
	count -= rv;
    }

    return done;
}

static inline __attribute__ ((const))
uint16_t data_segment(void)
{
    uint16_t ds;

    asm("movw %%ds,%0" : "=rm"(ds));
    return ds;
}

void write_device(int drive, const void *buf, size_t nsecs, unsigned int sector)
{
    uint16_t errnum;
    struct diskio dio;

    dprintf("write_device(%d,%p,%u,%u)\n", drive, buf, nsecs, sector);

    dio.startsector = sector;
    dio.sectors = nsecs;
    dio.bufoffs = (uintptr_t) buf;
    dio.bufseg = data_segment();

    /* Try FAT32-aware system call first */
    asm volatile("int $0x21 ; jc 1f ; xorw %0,%0\n"
		 "1:"
		 : "=a" (errnum)
		 : "a" (0x7305), "b" (&dio), "c" (-1), "d" (drive),
		   "S" (1), "m" (dio)
		 : "memory");

    /* If not supported, try the legacy system call (int2526.S) */
    if (errnum == 0x0001)
	errnum = int26_write_sector(drive, &dio);

    if (errnum) {
	dprintf("rv = %04x\n", errnum);
	die("sector write error");
    }
}

void read_device(int drive, const void *buf, size_t nsecs, unsigned int sector)
{
    uint16_t errnum;
    struct diskio dio;

    dprintf("read_device(%d,%p,%u,%u)\n", drive, buf, nsecs, sector);

    dio.startsector = sector;
    dio.sectors = nsecs;
    dio.bufoffs = (uintptr_t) buf;
    dio.bufseg = data_segment();

    /* Try FAT32-aware system call first */
    asm volatile("int $0x21 ; jc 1f ; xorw %0,%0\n"
		 "1:"
		 : "=a" (errnum)
		 : "a" (0x7305), "b" (&dio), "c" (-1), "d" (drive),
		   "S" (0), "m" (dio));

    /* If not supported, try the legacy system call (int2526.S) */
    if (errnum == 0x0001)
	errnum = int25_read_sector(drive, &dio);

    if (errnum) {
	dprintf("rv = %04x\n", errnum);
	die("sector read error");
    }
}

/* Both traditional DOS and FAT32 DOS return this structure, but
   FAT32 return a lot more data, so make sure we have plenty of space */
struct deviceparams {
    uint8_t specfunc;
    uint8_t devtype;
    uint16_t devattr;
    uint16_t cylinders;
    uint8_t mediatype;
    uint16_t bytespersec;
    uint8_t secperclust;
    uint16_t ressectors;
    uint8_t fats;
    uint16_t rootdirents;
    uint16_t sectors;
    uint8_t media;
    uint16_t fatsecs;
    uint16_t secpertrack;
    uint16_t heads;
    uint32_t hiddensecs;
    uint32_t hugesectors;
    uint8_t lotsofpadding[224];
} __attribute__ ((packed));

uint32_t get_partition_offset(int drive)
{
    uint8_t err;
    uint16_t rv;
    struct deviceparams dp;

    dp.specfunc = 1;		/* Get current information */

    rv = 0x440d;
    asm volatile ("int $0x21 ; setc %0"
		  :"=abcdm" (err), "+a"(rv), "=m"(dp)
		  :"b" (drive), "c" (0x0860), "d" (&dp));

    if (!err)
	return dp.hiddensecs;

    rv = 0x440d;
    asm volatile ("int $0x21 ; setc %0"
		  : "=abcdm" (err), "+a" (rv), "=m" (dp)
		  : "b" (drive), "c" (0x4860), "d" (&dp));

    if (!err)
	return dp.hiddensecs;

    die("could not find partition start offset");
}

struct rwblock {
    uint8_t special;
    uint16_t head;
    uint16_t cylinder;
    uint16_t firstsector;
    uint16_t sectors;
    uint16_t bufferoffset;
    uint16_t bufferseg;
} __attribute__ ((packed));

static struct rwblock mbr = {
    .special = 0,
    .head = 0,
    .cylinder = 0,
    .firstsector = 0,		/* MS-DOS, unlike the BIOS, zero-base sectors */
    .sectors = 1,
    .bufferoffset = 0,
    .bufferseg = 0
};

void write_mbr(int drive, const void *buf)
{
    uint16_t rv;
    uint8_t err;

    dprintf("write_mbr(%d,%p)", drive, buf);

    mbr.bufferoffset = (uintptr_t) buf;
    mbr.bufferseg = data_segment();

    rv = 0x440d;
    asm volatile ("int $0x21 ; setc %0" : "=bcdm" (err), "+a"(rv)
		  :"c"(0x0841), "d"(&mbr), "b"(drive), "m"(mbr));

    dprintf(" rv(0841) = %04x", rv);
    if (!err) {
	dprintf("\n");
	return;
    }

    rv = 0x440d;
    asm volatile ("int $0x21 ; setc %0" : "=bcdm" (err), "+a"(rv)
		  :"c"(0x4841), "d"(&mbr), "b"(drive), "m"(mbr));

    dprintf(" rv(4841) = %04x\n", rv);
    if (err)
	die("mbr write error");
}

void read_mbr(int drive, const void *buf)
{
    uint16_t rv;
    uint8_t err;

    dprintf("read_mbr(%d,%p)", drive, buf);

    mbr.bufferoffset = (uintptr_t) buf;
    mbr.bufferseg = data_segment();

    rv = 0x440d;
    asm volatile ("int $0x21 ; setc %0":"=abcdm" (err), "+a"(rv)
		  :"c"(0x0861), "d"(&mbr), "b"(drive), "m"(mbr));

    dprintf(" rv(0861) = %04x", rv);
    if (!err) {
	dprintf("\n");
	return;
    }

    rv = 0x440d;
    asm volatile ("int $0x21 ; setc %0":"=abcdm" (err), "+a"(rv)
		  :"c"(0x4861), "d"(&mbr), "b"(drive), "m"(mbr));

    dprintf(" rv(4841) = %04x\n", rv);
    if (err)
	die("mbr read error");

    dprintf("Bytes: %02x %02x %02x %02x %02x %02x %02x %02x\n",
	    ((const uint8_t *)buf)[0],
	    ((const uint8_t *)buf)[1],
	    ((const uint8_t *)buf)[2],
	    ((const uint8_t *)buf)[3],
	    ((const uint8_t *)buf)[4],
	    ((const uint8_t *)buf)[5],
	    ((const uint8_t *)buf)[6],
	    ((const uint8_t *)buf)[7]);
}

/* This call can legitimately fail, and we don't care, so ignore error return */
void set_attributes(const char *file, int attributes)
{
    uint16_t rv = 0x4301;

    dprintf("set_attributes(\"%s\", 0x%02x)\n", file, attributes);

    asm volatile ("int $0x21":"+a" (rv)
		  :"c"(attributes), "d"(file));
}

/*
 * Version of the read_device function suitable for libfat
 */
int libfat_xpread(intptr_t pp, void *buf, size_t secsize,
		  libfat_sector_t sector)
{
    read_device(pp, buf, 1, sector);
    return secsize;
}

static inline void get_dos_version(void)
{
    uint16_t ver;

    asm("int $0x21 ; xchgb %%ah,%%al"
	: "=a" (ver) 
	: "a" (0x3001)
	: "ebx", "ecx");
    dos_version = ver;

    dprintf("DOS version %d.%d\n", (dos_version >> 8), dos_version & 0xff);
}

/* The locking interface relies on static variables.  A massive hack :( */
static uint8_t lock_level, lock_drive;

static inline void set_lock_device(uint8_t device)
{
    lock_level  = 0;
    lock_drive = device;
}

static int do_lock(uint8_t level)
{
    uint16_t level_arg = lock_drive + (level << 8);
    uint16_t rv;
    uint8_t err;
#if 0
    /* DOS 7.10 = Win95 OSR2 = first version with FAT32 */
    uint16_t lock_call = (dos_version >= 0x070a) ? 0x484A : 0x084A;
#else
    uint16_t lock_call = 0x084A; /* MSDN says this is OK for all filesystems */
#endif

    dprintf("Trying lock %04x... ", level_arg);
    asm volatile ("int $0x21 ; setc %0"
		  : "=bcdm" (err), "=a" (rv)
		  : "a" (0x440d), "b" (level_arg),
		    "c" (lock_call), "d" (0x0001));
    dprintf("%s %04x\n", err ? "err" : "ok", rv);

    return err ? rv : 0;
}

void lock_device(int level)
{
    static int hard_lock = 0;
    int err;

    if (dos_version < 0x0700)
	return;			/* Win9x/NT only */

    if (!hard_lock) {
	/* Assume hierarchial "soft" locking supported */

	while (lock_level < level) {
	    int new_level = lock_level + 1;
	    err = do_lock(new_level);
	    if (err) {
		if (err == 0x0001) {
		    /* Try hard locking next */
		    hard_lock = 1;
		}
		goto soft_fail;
	    }

	    lock_level = new_level;
	}
	return;
    }

soft_fail:
    if (hard_lock) {
	/* Hard locking, only level 4 supported */
	/* This is needed for Win9x in DOS mode */
	
	err = do_lock(4);
	if (err) {
	    if (err == 0x0001) {
		/* Assume locking is not needed */
		return;
	    }
	    goto hard_fail;
	}

	lock_level = 4;
	return;
    }

hard_fail:
    die("could not lock device");
}

void unlock_device(int level)
{
    uint16_t rv;
    uint8_t err;
    uint16_t unlock_call;

    if (dos_version < 0x0700)
	return;			/* Win9x/NT only */

#if 0
    /* DOS 7.10 = Win95 OSR2 = first version with FAT32 */
    unlock_call = (dos_version >= 0x070a) ? 0x486A : 0x086A;
#else
    unlock_call = 0x086A;	/* MSDN says this is OK for all filesystems */
#endif

    if (lock_level == 4 && level > 0)
	return;			/* Only drop the hard lock at the end */

    while (lock_level > level) {
	uint8_t new_level = (lock_level == 4) ? 0 : lock_level - 1;
	uint16_t level_arg = (new_level << 8) + lock_drive;
	rv = 0x440d;
	dprintf("Trying unlock %04x... ", new_level);
	asm volatile ("int $0x21 ; setc %0"
		      : "=bcdm" (err), "+a" (rv)
		      : "b" (level_arg), "c" (unlock_call));
	dprintf("%s %04x\n", err ? "err" : "ok", rv);
	lock_level = new_level;
    }
}

/*
 * This function does any desired MBR manipulation; called with the device lock held.
 */
struct mbr_entry {
    uint8_t active;		/* Active flag */
    uint8_t bhead;		/* Begin head */
    uint8_t bsector;		/* Begin sector */
    uint8_t bcylinder;		/* Begin cylinder */
    uint8_t filesystem;		/* Filesystem value */
    uint8_t ehead;		/* End head */
    uint8_t esector;		/* End sector */
    uint8_t ecylinder;		/* End cylinder */
    uint32_t startlba;		/* Start sector LBA */
    uint32_t sectors;		/* Length in sectors */
} __attribute__ ((packed));

static void adjust_mbr(int device, int writembr, int set_active)
{
    static unsigned char sectbuf[512];
    int i;

    if (!writembr && !set_active)
	return;			/* Nothing to do */

    read_mbr(device, sectbuf);

    if (writembr) {
	memcpy(sectbuf, syslinux_mbr, syslinux_mbr_len);
	*(uint16_t *) (sectbuf + 510) = 0xaa55;
    }

    if (set_active) {
	uint32_t offset = get_partition_offset(device);
	struct mbr_entry *me = (struct mbr_entry *)(sectbuf + 446);
	int found = 0;

	dprintf("Searching for partition offset: %08x\n", offset);

	for (i = 0; i < 4; i++) {
	    if (me->startlba == offset) {
		me->active = 0x80;
		found++;
	    } else {
		me->active = 0;
	    }
	    me++;
	}

	if (found < 1) {
	    die("partition not found (-a is not implemented for logical partitions)");
	} else if (found > 1) {
	    die("multiple aliased partitions found");
	}
    }

    write_mbr(device, sectbuf);
}

int main(int argc, char *argv[])
{
    static unsigned char sectbuf[512];
    int dev_fd, fd;
    static char ldlinux_name[] = "@:\\ldlinux.sys";
    char **argp, *opt;
    int force = 0;		/* -f (force) option */
    struct libfat_filesystem *fs;
    libfat_sector_t s, *secp, sectors[65];	/* 65 is maximum possible */
    int32_t ldlinux_cluster;
    int nsectors;
    const char *device = NULL, *bootsecfile = NULL;
    const char *errmsg;
    int i;
    int writembr = 0;		/* -m (write MBR) option */
    int set_active = 0;		/* -a (set partition active) option */
    const char *subdir = NULL;
    int stupid = 0;
    int raid_mode = 0;

    dprintf("argv = %p\n", argv);
    for (i = 0; i <= argc; i++)
	dprintf("argv[%d] = %p = \"%s\"\n", i, argv[i], argv[i]);

    (void)argc;			/* Unused */

    get_dos_version();

    for (argp = argv + 1; *argp; argp++) {
	if (**argp == '-') {
	    opt = *argp + 1;
	    if (!*opt)
		usage();

	    while (*opt) {
		switch (*opt) {
		case 's':	/* Use "safe, slow and stupid" code */
		    stupid = 1;
		    break;
		case 'r':	/* RAID mode */
		    raid_mode = 1;
		    break;
		case 'f':	/* Force install */
		    force = 1;
		    break;
		case 'm':	/* Write MBR */
		    writembr = 1;
		    break;
		case 'a':	/* Set partition active */
		    set_active = 1;
		    break;
		case 'd':
		    if (argp[1])
			subdir = *++argp;
		    break;
		default:
		    usage();
		}
		opt++;
	    }
	} else {
	    if (bootsecfile)
		usage();
	    else if (device)
		bootsecfile = *argp;
	    else
		device = *argp;
	}
    }

    if (!device)
	usage();

    /*
     * Figure out which drive we're talking to
     */
    dev_fd = (device[0] & ~0x20) - 0x40;
    if (dev_fd < 1 || dev_fd > 26 || device[1] != ':' || device[2])
	usage();

    set_lock_device(dev_fd);

    lock_device(2);		/* Make sure we can lock the device */
    read_device(dev_fd, sectbuf, 1, 0);
    unlock_device(1);

    /*
     * Check to see that what we got was indeed an MS-DOS boot sector/superblock
     */
    if ((errmsg = syslinux_check_bootsect(sectbuf))) {
	unlock_device(0);
	puts(errmsg);
	putchar('\n');
	exit(1);
    }

    ldlinux_name[0] = dev_fd | 0x40;

    set_attributes(ldlinux_name, 0x00);
    fd = creat(ldlinux_name, 0);
    write_file(fd, syslinux_ldlinux, syslinux_ldlinux_len);
    close(fd);
    set_attributes(ldlinux_name, 0x07);	/* SYSTEM HIDDEN READONLY */

    /*
     * Now, use libfat to create a block map.  This probably
     * should be changed to use ioctl(...,FIBMAP,...) since
     * this is supposed to be a simple, privileged version
     * of the installer.
     */
    lock_device(2);
    fs = libfat_open(libfat_xpread, dev_fd);
    ldlinux_cluster = libfat_searchdir(fs, 0, "LDLINUX SYS", NULL);
    secp = sectors;
    nsectors = 0;
    s = libfat_clustertosector(fs, ldlinux_cluster);
    while (s && nsectors < 65) {
	*secp++ = s;
	nsectors++;
	s = libfat_nextsector(fs, s);
    }
    libfat_close(fs);

    /*
     * If requested, move ldlinux.sys
     */
    if (subdir) {
	char new_ldlinux_name[160];
	char *cp = new_ldlinux_name + 3;
	const char *sd;
	int slash = 1;

	new_ldlinux_name[0] = dev_fd | 0x40;
	new_ldlinux_name[1] = ':';
	new_ldlinux_name[2] = '\\';

	for (sd = subdir; *sd; sd++) {
	    char c = *sd;

	    if (c == '/' || c == '\\') {
		if (slash)
		    continue;
		c = '\\';
		slash = 1;
	    } else {
		slash = 0;
	    }

	    *cp++ = c;
	}

	/* Skip if subdirectory == root */
	if (cp > new_ldlinux_name + 3) {
	    if (!slash)
		*cp++ = '\\';

	    memcpy(cp, "ldlinux.sys", 12);

	    set_attributes(ldlinux_name, 0);
	    if (rename(ldlinux_name, new_ldlinux_name))
		set_attributes(ldlinux_name, 0x07);
	    else
		set_attributes(new_ldlinux_name, 0x07);
	}
    }

    /*
     * Patch ldlinux.sys and the boot sector
     */
    syslinux_patch(sectors, nsectors, stupid, raid_mode);

    /*
     * Write the now-patched first sector of ldlinux.sys
     */
    /* lock_device(3); -- doesn't seem to be needed */
    write_device(dev_fd, syslinux_ldlinux, 1, sectors[0]);

    /*
     * Muck with the MBR, if desired, while we hold the lock
     */
    adjust_mbr(dev_fd, writembr, set_active);

    /*
     * To finish up, write the boot sector
     */

    /* Read the superblock again since it might have changed while mounted */
    read_device(dev_fd, sectbuf, 1, 0);

    /* Copy the syslinux code into the boot sector */
    syslinux_make_bootsect(sectbuf);

    /* Write new boot sector */
    if (bootsecfile) {
	unlock_device(0);
	fd = creat(bootsecfile, 0x20);	/* ARCHIVE */
	write_file(fd, sectbuf, 512);
	close(fd);
    } else {
	write_device(dev_fd, sectbuf, 1, 0);
	unlock_device(0);
    }

    /* Done! */

    return 0;
}
