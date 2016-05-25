/*
 * Copyright (C) 2015-2016 The PASTA Team, dukesrg
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include "tmio/tmio.h"
#include "NandDumper.h"
#include "console.h"
#include "draw.h"
#include "lang.h"
#include "hid.h"
#include "fs.h"
#include "screenshot.h"
#include "padgen.h"
#include "aes.h"
#include "mpcore.h"
#include "ncch.h"
#include "CTRDecryptor.h"
#include "progress.h"
#include "strings.h"
#include "progress.h"

#define BUF1 (void*)0x21000000
#define BUF_SIZE 0x100000

int NandSwitch(){
	if(!checkEmuNAND()) return  0; //If No EmuNAND, we force to work on SysNAND
	ConsoleInit();
	print(strings[STR_CHOOSE], strings[STR_NAND]);
	print(strings[STR_BLANK_BUTTON_ACTION], strings[STR_BUTTON_X], strings[STR_SYSNAND]);
	print(strings[STR_BLANK_BUTTON_ACTION], strings[STR_BUTTON_Y], strings[STR_EMUNAND]);
	print(strings[STR_BLANK_BUTTON_ACTION], strings[STR_BUTTON_B], strings[STR_CANCEL]);

	ConsoleShow();
	while (1) {
        uint32_t pad_state = InputWait();
		if(pad_state & keys[KEY_X].mask) return SYS_NAND;
		if(pad_state & keys[KEY_Y].mask) return EMU_NAND;
		if(pad_state & keys[KEY_B].mask) return UNK_NAND;
    }
}

uint_fast8_t GenerateNandXorpad(nand_partition_index pidx, wchar_t *path) {
	aes_ctr ctr;
	nand_partition_entry *partition = GetNANDPartition(SYSNAND, pidx);
	size_t size = partition->sectors_count * NAND_SECTOR_SIZE;

	statusInit(size, STATUS_CANCELABLE | STATUS_WAIT, lang(SF_GENERATING_XORPAD), lang(*nand_partition_names[pidx]));
	GetNANDCTR(&ctr);
	aes_add_ctr(&ctr, partition->first_sector * NAND_SECTOR_SIZE / AES_BLOCK_SIZE);
	return CreatePad(&ctr, &(aes_key){NULL, 0, partition->keyslot, 0}, size, path, 0);
}

void DumpNANDSystemTitles(){
	ConsoleSetTitle(strings[STR_DUMP], strings[STR_SYSTEM_TITLES]);
	int isEmuNand = SYS_NAND;
	if(checkEmuNAND() && (isEmuNand = NandSwitch()) == UNK_NAND) return;
	isEmuNand--;
	ConsoleInit();
	ConsoleSetTitle(strings[STR_DUMP], strings[STR_SYSTEM_TITLES]);
	print(strings[STR_PROCESSING], isEmuNand ? strings[STR_EMUNAND] : strings[STR_SYSNAND]);
	wchar_t* outfolder = L"rxTools/titles";
	print(strings[STR_SYSTEM_TITLES_WARNING]);
	ConsoleShow();
	File pfile;
	wchar_t filename[_MAX_LFN];
	int nTitle = 0;
	unsigned int tot_size = 0x179000;
	f_mkdir (outfolder);
	for(int i = 0; i < tot_size; i++){
		nand_readsectors(i, 1, BUF1, isEmuNand ? EMUNAND : SYSNAND, NAND_PARTITION_CTRNAND);
		if (*(uint32_t*)((uint8_t*)BUF1 + 0x100) == NCCH_MAGIC) {
			ctr_ncchheader ncch;
			memcpy((void*)&ncch, BUF1, NAND_SECTOR_SIZE);
			swprintf(filename, _MAX_LFN, L"%ls/%ls%08X%08X.app",
				outfolder, isEmuNand ? L"emu_" : L"",
				*((unsigned int*)(BUF1 + 0x10C)), *((unsigned int*)(BUF1 + 0x108)));
			ConsoleInit();
			print(strings[STR_DUMPING], L"", filename);
			ConsoleShow();
			FileOpen(&pfile, filename, 1);
			for(int j = 0; j < ncch.contentsize; j++){
				nand_readsectors(i + j, 1, BUF1, isEmuNand ? EMUNAND : SYSNAND, NAND_PARTITION_CTRNAND);
				FileWrite(&pfile, BUF1, NAND_SECTOR_SIZE, j*NAND_SECTOR_SIZE);
			}
			FileClose(&pfile);
			i += ncch.contentsize;
			nTitle++;
		}
	}
	ConsoleInit();
	print(strings[STR_SYSTEM_TITLES_DECRYPT], nTitle);
	print(strings[STR_PRESS_BUTTON_ACTION], strings[STR_BUTTON_A], strings[STR_CONTINUE]);
	print(strings[STR_PRESS_BUTTON_ACTION], strings[STR_BUTTON_B], strings[STR_CANCEL]);
	ConsoleShow();
	while (1) {
        uint32_t pad_state = InputWait();
		if(pad_state & keys[KEY_A].mask){
			CTRDecryptor();
			break;
		}
		if(pad_state & keys[KEY_B].mask)
			break;
    }
}

uint_fast8_t DumpNand(nand_type type, wchar_t *path) {
	File f;
	uint8_t buf[BUF_SIZE];
	size_t block, size, offset = 0;
	nand_metrics *n = GetNANDMetrics(type);
	uint_fast8_t id = type == SYSNAND ? TMIO_DEV_NAND : TMIO_DEV_SDMC;
	if (!n || !(size = n->sectors_count) || !path || size * NAND_SECTOR_SIZE > FSFreeSpace(path) || !FileOpen(&f, path, 1))
		return 0;

	statusInit(size, STATUS_CANCELABLE | STATUS_WAIT, lang(SF_DUMPING), lang(*nand_names[type]));
	while (size) {
		block = size < BUF_SIZE / NAND_SECTOR_SIZE ? size : BUF_SIZE / NAND_SECTOR_SIZE;
		if (offset || n->format != NAND_FORMAT_GW) 
			tmio_readsectors(id, n->first_sector + offset, block, buf);
		else {
			tmio_readsectors(id, n->first_sector + n->sectors_count, 1, buf);
			if (block > 1)
				tmio_readsectors(id, n->first_sector + 1, block - 1, buf + NAND_SECTOR_SIZE);
		}
		FileWrite2(&f, buf, block * NAND_SECTOR_SIZE);
		size -= block;
		offset += block;
		if (!progressSetPos(offset))
			return 0;
	}
	FileClose(&f);
	return 1;
}

uint_fast8_t InjectNand(nand_type type, wchar_t *path) {
	File f;
	uint8_t buf[BUF_SIZE];
	size_t block, size, offset = 0;
	nand_metrics *n = GetNANDMetrics(type);
	uint_fast8_t id = type == SYSNAND ? TMIO_DEV_NAND : TMIO_DEV_SDMC;

	if (!n || !path || !FileOpen(&f, path, 0) || (size = n->sectors_count) * NAND_SECTOR_SIZE != FileGetSize(&f))
		return 0;

	statusInit(size, STATUS_WAIT, lang(SF_INJECTING), lang(*nand_names[type]));
	while (size) {
		block = FileRead2(&f, buf, BUF_SIZE) / NAND_SECTOR_SIZE;
		if (offset || n->format != NAND_FORMAT_GW) 
			tmio_writesectors(id, n->first_sector + offset, block, buf);
		else {
			tmio_writesectors(id, n->first_sector + n->sectors_count, 1, buf);
			if (block > 1)
				tmio_writesectors(id, n->first_sector + 1, block - 1, buf + NAND_SECTOR_SIZE);
		}
		size -= block;
		offset += block;
		if (!progressSetPos(offset))
			return 0;
	}
	FileClose(&f);
	return 1;
}

uint_fast8_t CopyNand(nand_type src, nand_type dst) {
	uint8_t buf[BUF_SIZE];
	size_t block, size, offset = 0;
	nand_metrics *srcnand = GetNANDMetrics(src), *dstnand = GetNANDMetrics(dst);
	uint_fast8_t srcid = src == SYSNAND ? TMIO_DEV_NAND : TMIO_DEV_SDMC, dstid = dst == SYSNAND ? TMIO_DEV_NAND : TMIO_DEV_SDMC;

	if (!srcnand || !dstnand || srcnand == dstnand || !(size = srcnand->sectors_count) || size != dstnand->sectors_count)
		return 0;

	statusInit(size, STATUS_WAIT, lang(SF2_COPYING), lang(*nand_names[src]), lang(*nand_names[dst]));
	while (size) {
		block = size < BUF_SIZE / NAND_SECTOR_SIZE ? size : BUF_SIZE / NAND_SECTOR_SIZE;
		if (offset || srcnand->format != NAND_FORMAT_GW) 
			tmio_readsectors(srcid, srcnand->first_sector + offset, block, buf);
		else {
			tmio_readsectors(srcid, srcnand->first_sector + srcnand->sectors_count, 1, buf);
			if (block > 1)
				tmio_readsectors(srcid, srcnand->first_sector + 1, block - 1, buf + NAND_SECTOR_SIZE);
		}
		if (offset || dstnand->format != NAND_FORMAT_GW) 
			tmio_writesectors(dstid, dstnand->first_sector + offset, block, buf);
		else {
			tmio_writesectors(dstid, dstnand->first_sector + dstnand->sectors_count, 1, buf);
			if (block > 1)
				tmio_writesectors(dstid, dstnand->first_sector + 1, block - 1, buf + NAND_SECTOR_SIZE);
		}
		size -= block;
		offset += block;
		if (!progressSetPos(offset))
			return 0;
	}
	return 1;
}

uint_fast8_t DumpPartition(nand_type type, nand_partition_index partition, wchar_t *path) {
	File f;
	uint8_t buf[BUF_SIZE];
	size_t block, size, offset = 0;
	nand_partition_entry *p = GetNANDPartition(type, partition);

	if (!p || !(size = p->sectors_count) || !path || size * NAND_SECTOR_SIZE > FSFreeSpace(path) || !FileOpen(&f, path, 1))
		return 0;

	statusInit(size, STATUS_CANCELABLE | STATUS_WAIT, lang(SF2_DUMPING_PARTITION), lang(*nand_names[type]), lang(*nand_partition_names[partition]));
	while (size) {
		block = size < BUF_SIZE / NAND_SECTOR_SIZE ? size : BUF_SIZE / NAND_SECTOR_SIZE;
		nand_readsectors(offset, block, buf, type, partition);
		FileWrite2(&f, buf, block * NAND_SECTOR_SIZE);
		size -= block;
		offset += block;
		if (!progressSetPos(offset))
			return 0;
	}
	FileClose(&f);
	return 1;
}

uint_fast8_t InjectPartition(nand_type type, nand_partition_index partition, wchar_t *path) {
	File f;
	uint8_t buf[BUF_SIZE];
	size_t block, size, offset = 0;
	nand_partition_entry *p = GetNANDPartition(type, partition);

	if (!p || !path || !FileOpen(&f, path, 0) || (size = p->sectors_count) * NAND_SECTOR_SIZE != FileGetSize(&f))
		return 0;

	statusInit(size, STATUS_WAIT, lang(SF2_INJECTING_PARTITION), lang(*nand_names[type]), lang(*nand_partition_names[partition]));
	while (size) {
		block = FileRead2(&f, buf, BUF_SIZE) / NAND_SECTOR_SIZE;
		nand_writesectors(offset, block, buf, type, partition);
		size -= block;
		offset += block;
		if (!progressSetPos(offset))
			return 0;
	}
	FileClose(&f);
	return 1;
}

uint_fast8_t CopyPartition(nand_type src, nand_partition_index partition, nand_type dst) {
	uint8_t buf[BUF_SIZE];
	size_t block, size, srcoffset, dstoffset;
	nand_metrics *srcnand = GetNANDMetrics(src), *dstnand = GetNANDMetrics(dst);
	uint_fast8_t srcid = src == SYSNAND ? TMIO_DEV_NAND : TMIO_DEV_SDMC, dstid = dst == SYSNAND ? TMIO_DEV_NAND : TMIO_DEV_SDMC;
	nand_partition_entry *srcpartition = GetNANDPartition(src, partition), *dstpartition = GetNANDPartition(dst, partition);

	if (!srcnand || !dstnand || !srcpartition || !dstpartition || src == dst || !(size = srcpartition->sectors_count) || size != dstpartition->sectors_count)
		return 0;

	statusInit(size, STATUS_WAIT, lang(SF3_COPYING_PARTITION), lang(*nand_partition_names[partition]), lang(*nand_names[src]), lang(*nand_names[dst]));
	srcoffset = srcpartition->first_sector;
	dstoffset = dstpartition->first_sector;
	while (size) {
		block = size < BUF_SIZE / NAND_SECTOR_SIZE ? size : BUF_SIZE / NAND_SECTOR_SIZE;
		if (srcoffset || srcnand->format != NAND_FORMAT_GW) 
			tmio_readsectors(srcid, srcnand->first_sector + srcoffset, block, buf);
		else {
			tmio_readsectors(srcid, srcnand->first_sector + srcnand->sectors_count, 1, buf);
			if (block > 1)
				tmio_readsectors(srcid, srcnand->first_sector + 1, block - 1, buf + NAND_SECTOR_SIZE);
		}
		if (dstoffset || dstnand->format != NAND_FORMAT_GW) 
			tmio_writesectors(dstid, dstnand->first_sector + dstoffset, block, buf);
		else {
			tmio_writesectors(dstid, dstnand->first_sector + dstnand->sectors_count, 1, buf);
			if (block > 1)
				tmio_writesectors(dstid, dstnand->first_sector + 1, block - 1, buf + NAND_SECTOR_SIZE);
		}
		size -= block;
		srcoffset += block;
		dstoffset += block;
		if (!progressSetPos(srcoffset))
			return 0;
	}
	return 1;
}

uint_fast8_t CopyFile(wchar_t *srcpath, wchar_t *dstpath) {
	uint8_t buf[BUF_SIZE];
	size_t size, block, offset = 0;
	File srcfile, dstfile;
	
	if (!FileOpen(&srcfile, srcpath, 0) || (
		(!(size = FileGetSize(&srcfile)) || size > FSFreeSpace(dstpath) || !FileOpen(&dstfile, dstpath, 1)) &&
		(FileClose(&srcfile) || 1)
	)) return 0;

	statusInit(size, STATUS_WAIT, lang(SF2_COPYING), srcpath, dstpath);
	while (size) {
		block = FileRead2(&srcfile, buf, BUF_SIZE);
		FileWrite2(&dstfile, buf, block);
		size -= block;
		offset += block;
		if (!progressSetPos(offset))
			return 0;
	}
	FileClose(&srcfile);
	FileClose(&dstfile);
	return 1;
}