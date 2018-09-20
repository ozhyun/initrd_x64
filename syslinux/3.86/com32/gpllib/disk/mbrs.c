/* ----------------------------------------------------------------------- *
 *
 *   Copyright 2009 Pierre-Alexandre Meyer
 *
 *   This file is part of Syslinux, and is made available under
 *   the terms of the GNU General Public License version 2.
 *
 * ----------------------------------------------------------------------- */

#include <disk/common.h>
#include <disk/geom.h>
#include <disk/read.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * get_mbr_string - return a string describing the boot code
 * @label:		first four bytes of the MBR
 * @buffer:		pre-allocated buffer
 * @buffer_size:	@buffer size
 **/
void get_mbr_string(const uint32_t label, char *buffer, const int buffer_size)
{
    /* 2 bytes are usually enough to identify the MBR */
    uint16_t s_label = label >> 16;

    switch (s_label) {
    case 0x0000:
    case 0xfa33:
    case 0xfab8:
    case 0xfabe:
	strncpy(buffer, "No bootloader", buffer_size - 1);
	break;
    case 0x0ebe:
	strncpy(buffer, "ThinkPad", buffer_size - 1);
	break;
    case 0x31c0:
	strncpy(buffer, "Acer 3", buffer_size - 1);
	break;
    case 0x33c0:
	strncpy(buffer, "Windows", buffer_size - 1);
	break;
    case 0x33ff:
	strncpy(buffer, "HP/Gateway", buffer_size - 1);
	break;
    case 0xb800:
	strncpy(buffer, "PloP", buffer_size - 1);
	break;
    case 0xea05:
	strncpy(buffer, "XOSL", buffer_size - 1);
	break;
    case 0xea1e:
	strncpy(buffer, "Truecrypt Boot Loader", buffer_size - 1);
	break;
    case 0xeb04:
	strncpy(buffer, "Solaris", buffer_size - 1);
	break;
    case 0xeb48:
	strncpy(buffer, "Grub", buffer_size - 1);
	break;
    case 0xeb4c:
	strncpy(buffer, "Grub2 (v1.96)", buffer_size - 1);
	break;
    case 0xeb63:
	strncpy(buffer, "Grub2 (v1.97)", buffer_size - 1);
	break;
    case 0xeb5e:
	/* We need more than 2 bytes */
	if (((label >> 8) & 0xff) == 0x00)
	    strncpy(buffer, "fbinst", buffer_size - 1);
	else if (((label >> 8) & 0xff) == 0x80)
	    strncpy(buffer, "Grub4Dos", buffer_size - 1);
	else
	    strncpy(buffer, "Unknown mbr", buffer_size - 1);
	break;
    case 0xfa31:
	/* We need more than 2 bytes */
	if (((label >> 8) & 0xff) == 0xc9)
	    strncpy(buffer, "Master Boot LoaDeR", buffer_size - 1);
	else if (((label >> 8) & 0xff) == 0xc0)
	    strncpy(buffer, "Syslinux", buffer_size - 1);
	else
	    strncpy(buffer, "Unknown mbr", buffer_size - 1);
	break;
    case 0xfaeb:
	strncpy(buffer, "Lilo", buffer_size - 1);
	break;
    case 0xfc31:
	strncpy(buffer, "Testdisk", buffer_size - 1);
	break;
    case 0xfc33:
	strncpy(buffer, "GAG", buffer_size - 1);
	break;
    case 0xfceb:
	strncpy(buffer, "BootIT NG", buffer_size - 1);
	break;
    default:
	strncpy(buffer, "Unknown mbr", buffer_size - 1);
	break;
    }

    buffer[buffer_size - 1] = '\0';
}

/**
 * get_mbr_id - return the first four bytes of the MBR
 * @d:		driveinfo struct describing the drive
 **/
uint32_t get_mbr_id(const struct driveinfo *d)
{
    char mbr[SECTOR * sizeof(char)];

    if (read_mbr(d->disk, &mbr) == -1)
	return -1;
    else {
	uint32_t mbr_id;
	/* Reverse the opcodes */
	mbr_id = (*(uint8_t *) (mbr + 3));
	mbr_id += (*(uint8_t *) (mbr + 2) << 8);
	mbr_id += (*(uint8_t *) (mbr + 1) << 16);
	mbr_id += (*(uint8_t *) mbr) << 24;
	return mbr_id;
    }
}
