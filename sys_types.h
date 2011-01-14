/*
    GNU fdisk - a clone of Linux fdisk.

    Copyright (C) 2006
    Free Software Foundation, Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
*/

#ifndef SYS_TYPES_H_INCLUDED
#define SYS_TYPES_H_INCLUDED



#ifndef HACKS_H_INCLUDED
struct _SysType {
	unsigned char type;
	const TCHAR *name;
};
typedef struct _SysType SysType;
#endif

#ifdef UNICODE
#define N_(String) _T(String)
#else
#define N_(String) String
#endif


/* The system types for msdos  partition tables
 * Needed for lfdisk and some interface improvements
 */
SysType msdos_systypes[] = {
	/* TODO: I don't like some of the strings, change them */
	{ 0x00, N_("Empty") }, /* "Empty") */
	{ 0x01, N_("FAT12") }, 
	{ 0x02, N_("XENIX root") }, 
	{ 0x03, N_("XENIX usr") }, 
	{ 0x04, N_("Small FAT16") }, 
	{ 0x05, N_("Extended") }, /* "Extended") */
	{ 0x06, N_("FAT16") }, 
	{ 0x07, N_("HPFS/NTFS") }, 
	{ 0x08, N_("AIX") }, 
	{ 0x09, N_("AIX bootable") }, 
	{ 0x0a, N_("OS/2 boot mgr") }, 
	{ 0x0b, N_("FAT32") }, 
	{ 0x0c, N_("FAT32 LBA") }, 
	{ 0x0e, N_("FAT16 LBA") }, 
	{ 0x0f, N_("Extended LBA") }, 
	{ 0x10, N_("OPUS") }, 
	{ 0x11, N_("Hidden FAT12") }, 
	{ 0x12, N_("Compaq diag") }, 
	{ 0x14, N_("Hidd Sm FAT16") }, 
	{ 0x16, N_("Hidd FAT16") }, 
	{ 0x17, N_("Hidd HPFS/NTFS") }, 
	{ 0x18, N_("AST SmartSleep") }, 
	{ 0x1b, N_("Hidd FAT32") }, 
	{ 0x1c, N_("Hidd FAT32 LBA") }, 
	{ 0x1e, N_("Hidd FAT16 LBA") }, 
	{ 0x24, N_("NEC DOS") }, 
	{ 0x39, N_("Plan 9") }, 
	{ 0x3c, N_("PMagic recovery") }, 
	{ 0x40, N_("Venix 80286") }, 
	{ 0x41, N_("PPC PReP Boot") }, 
	{ 0x42, N_("SFS") }, 
	{ 0x4d, N_("QNX4.x") }, 
	{ 0x4e, N_("QNX4.x 2nd part") }, 
	{ 0x4f, N_("QNX4.x 3rd part") }, 
	{ 0x50, N_("OnTrack DM") }, 
	{ 0x51, N_("OnTrackDM6 Aux1") }, 
	{ 0x52, N_("CP/M") }, 
	{ 0x53, N_("OnTrackDM6 Aux3") }, 
	{ 0x54, N_("OnTrack DM6") }, 
	{ 0x55, N_("EZ Drive") }, 
	{ 0x56, N_("Golden Bow") }, 
	{ 0x5c, N_("Priam Edisk") }, 
	{ 0x61, N_("SpeedStor") }, 
	{ 0x63, N_("GNU HURD/SysV") }, 
	{ 0x64, N_("Netware 286") }, 
	{ 0x65, N_("Netware 386") }, 
	{ 0x70, N_("DiskSec MltBoot") }, 
	{ 0x75, N_("PC/IX") }, 
	{ 0x80, N_("Minix <1.4a") }, 
	{ 0x81, N_("Minix >1.4b") }, 
	{ 0x82, N_("Linux swap") }, /* FIXME: Or solaris */
	{ 0x83, N_("Linux") }, 
	{ 0x84, N_("OS/2 hidden C:") }, 
	{ 0x85, N_("Linux extended") }, 
	{ 0x86, N_("NTFS volume set") },
	{ 0x87, N_("NTFS volume set") }, 
	{ 0x88, N_("Linux plaintext") }, 
	{ 0x8e, N_("Linux LVM") }, 
	{ 0x93, N_("Amoeba") }, 
	/*This guys created a seperate partition for badblocks?! */
	{ 0x94, N_("Amoeba BBT") },  
	{ 0x9f, N_("BSD/OS") }, 
	{ 0xa0, N_("Thinkpad hib") }, 
	{ 0xa5, N_("FreeBSD") }, 
	{ 0xa6, N_("OpenBSD") }, 
	{ 0xa7, N_("NeXTSTEP") }, 
	{ 0xa8, N_("Darwin UFS") }, 
	{ 0xa9, N_("NetBSD") }, 
	{ 0xab, N_("Darwin boot") }, 
	{ 0xb7, N_("BSDI fs") }, 
	{ 0xb8, N_("BSDI swap") }, 
	/* Beware of the hidden wizard */
	{ 0xbb, N_("Boot Wizard Hid") },
	{ 0xbe, N_("Solaris boot") }, 
	{ 0xbf, N_("Solaris") }, 
	{ 0xc1, N_("DRDOS/2 FAT12") }, 
	{ 0xc4, N_("DRDOS/2 smFAT16") }, 
	{ 0xc6, N_("DRDOS/2 FAT16") }, 
	/* Reminds me of Rush - 2112 */
	{ 0xc7, N_("Syrinx") },  
	{ 0xda, N_("Non-FS data") },
	{ 0xdb, N_("CP/M / CTOS") }, 
	{ 0xde, N_("Dell Utility") }, 
	/* Should 0x20 be DontBootIt then? */
	{ 0xdf, N_("BootIt") },  
	{ 0xe1, N_("DOS access") }, 
	{ 0xe3, N_("DOS R/O") }, 
	/*I sense some strange déjà vu */
	{ 0xe4, N_("SpeedStor") },  
	{ 0xeb, N_("BeOS fs") }, 
	{ 0xee, N_("EFI GPT") }, 
	{ 0xef, N_("EFI FAT") }, 
	{ 0xf0, N_("Lnx/PA-RISC bt") }, 
	{ 0xf1, N_("SpeedStor") }, 
	{ 0xf2, N_("DOS secondary") }, 
	/* Are these guys trying for a Guinness record or something? */
	{ 0xf4, N_("SpeedStor") },  
	{ 0xfd, N_("Lnx RAID auto") }, 
	{ 0xfe, N_("LANstep") }, 
	{ 0xff, N_("XENIX BBT") },
	{ 0, NULL }
};

SysType bsd_systypes[] = {
	{ 0x00, N_("Unused") },
	{ 0x01, N_("Swap") },
	{ 0x02, N_("Version 6") },
	{ 0x03, N_("Version 7") },
	{ 0x04, N_("System V") },
	{ 0x05, N_("4.1BSD") },
	{ 0x06, N_("Eight Edition") },
	{ 0x07, N_("4.2BSD") },
/* It's this way in Linux fdisk, I guess it is alright */
#ifdef __alpha__
	{ 0x08, N_("ext2") },
#else
	{ 0x08, N_("MS-DOS") },
#endif
	{ 0x09, N_("4.4LFS") },
	{ 0x0a, N_("Unknown") },
	{ 0x0b, N_("HPFS") },
	{ 0x0c, N_("ISO-9660") },
	{ 0x0d, N_("Boot") },
	{ 0x0e, N_("ADOS") },
	{ 0x0f, N_("HFS") },
	{ 0x10, N_("AdvFS") },
	{ 0, NULL }
};

SysType sun_systypes[] = {
	{ 0x00, N_("Empty") },
	{ 0x01, N_("Boot") },
	{ 0x02, N_("SunOS root") },
	{ 0x03, N_("SunOS swap") },
	{ 0x04, N_("SunOS usr") },
	{ 0x05, N_("Whole disk") },
	{ 0x06, N_("SunOS stand") },
	{ 0x07, N_("SunOS var") },
	{ 0x08, N_("SunOS home") },
	{ 0x82, N_("Linux swap") },
	{ 0x83, N_("Linux") },
	{ 0x8e, N_("Linux LVM") },
	{ 0xfd, N_("Lnx RAID auto") },
	{ 0, NULL }
};

#endif
