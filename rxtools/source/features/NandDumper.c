/*
 * Copyright (C) 2015 The PASTA Team
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
#include "nand.h"

#define BUF1 (void*)0x21000000
#define PROGRESS_WIDTH	16

static size_t getNandSize()
{
	return getMpInfo() == MPINFO_KTR ? 0x4D800000 : 0x3AF00000;
}

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

void NandDumper(){
	ConsoleSetTitle(strings[STR_DUMP], strings[STR_NAND]);
	File myFile;
	int isEmuNand = SYS_NAND;
	if(checkEmuNAND() && (isEmuNand = NandSwitch()) == UNK_NAND) return;
	isEmuNand--;
	ConsoleInit();
	ConsoleSetTitle(strings[STR_DUMP], strings[STR_NAND]);
	unsigned char* buf = BUF1;
	unsigned int nsectors = NAND_SECTOR_SIZE;  //sectors in a row
	wchar_t tmpstr[STR_MAX_LEN];
	wchar_t ProgressBar[41] = {0,};
	for(int i=0; i<PROGRESS_WIDTH; i++)
		wcscat(ProgressBar, strings[STR_PROGRESS]);
	unsigned int progress = 0;
	wchar_t filename[_MAX_LFN];
	swprintf(filename, _MAX_LFN, L"rxTools/nand/%lsNAND.bin",
		isEmuNand ? L"EMU" : L"");
	if(FileOpen(&myFile, filename, 1)){
		print(strings[STR_DUMPING], isEmuNand ? strings[STR_EMUNAND] : strings[STR_SYSNAND], filename);
		ConsoleShow();
		int x, y;
		ConsoleGetXY(&x, &y);
		y += 16 * 6;
		x += 8 * 2;

		DrawString(&bottomScreen, ProgressBar, x, y, ConsoleGetTextColor(), ConsoleGetBackgroundColor());
		swprintf(tmpstr, STR_MAX_LEN, strings[STR_PRESS_BUTTON_ACTION], strings[STR_BUTTON_B], strings[STR_CANCEL]);
		DrawString(&bottomScreen, tmpstr, x, y + 16 * 2, ConsoleGetTextColor(), ConsoleGetBackgroundColor());

		for(int count = 0; count < getNandSize()/NAND_SECTOR_SIZE/nsectors; count++){

			if(isEmuNand) tmio_readsectors(TMIO_DEV_SDMC, count*nsectors, nsectors, buf);
			else tmio_readsectors(TMIO_DEV_NAND, count*nsectors, nsectors, buf);

			FileWrite(&myFile, buf, nsectors*NAND_SECTOR_SIZE, count*NAND_SECTOR_SIZE*nsectors);
			TryScreenShot();
			if((count % (int)(getNandSize()/NAND_SECTOR_SIZE/nsectors/PROGRESS_WIDTH)) == 0 && count != 0){
				DrawString(&bottomScreen, strings[STR_PROGRESS_OK], x+(16*(progress++)), y, ConsoleGetTextColor(), ConsoleGetBackgroundColor());
			}
			unsigned int pad = GetInput();
			if (pad & keys[KEY_B].mask) {
				FileClose(&myFile);
				goto end;
			}
		}
		if(isEmuNand){
			tmio_readsectors(TMIO_DEV_SDMC, checkEmuNAND()/NAND_SECTOR_SIZE, 1, buf);
			FileWrite(&myFile, buf, NAND_SECTOR_SIZE, 0);
		}
		FileClose(&myFile);
		print(strings[STR_COMPLETED]);
		ConsoleShow();
	}else{
		print(strings[STR_FAILED]);
		ConsoleShow();
	}

end:
	print(strings[STR_PRESS_BUTTON_ACTION], strings[STR_BUTTON_A], strings[STR_CONTINUE]);
	ConsoleShow();
	WaitForButton(keys[KEY_A].mask);
}

void DumpNandPartitions(){
	ConsoleSetTitle(strings[STR_DUMP], strings[STR_NAND_PARTITIONS]);
	int isEmuNand = SYS_NAND;
	if(checkEmuNAND() && (isEmuNand = NandSwitch()) == UNK_NAND) return;
	isEmuNand--;
	ConsoleInit();
	ConsoleSetTitle(strings[STR_DUMP], strings[STR_NAND_PARTITIONS]);
	print(strings[STR_PROCESSING], isEmuNand ? strings[STR_EMUNAND] : strings[STR_SYSNAND]);
	wchar_t* p_name[] = {
		L"twln.bin", L"twlp.bin", L"agb_save.bin",
		L"firm0.bin", L"firm1.bin", L"ctrnand.bin"
	};
	wchar_t* p_descr[] = { strings[STR_TWLN], strings[STR_TWLP], strings[STR_AGB_SAVE], strings[STR_FIRM0], strings[STR_FIRM1], strings[STR_CTRNAND] };
	nand_partition_index p_idx[] = { NAND_PARTITION_TWLN, NAND_PARTITION_TWLP, NAND_PARTITION_AGB_SAVE, NAND_PARTITION_FIRM0, NAND_PARTITION_FIRM1, NAND_PARTITION_CTRNAND};
	int sect_row = 0x80;

	wchar_t tmp[_MAX_LFN];
	for(int i = 3; i < sizeof(p_idx) / sizeof(p_idx[0]); i++){		//Cutting out twln, twlp and agb_save. Todo: Properly decrypt them
		File out;
		swprintf(tmp, _MAX_LFN, L"rxTools/nand/%ls%ls", isEmuNand ? L"emu_" : L"", p_name[i]);
		FileOpen(&out, tmp, 1);
		print(strings[STR_DUMPING], p_descr[i], tmp);
		ConsoleShow();

		size_t size = GetNANDPartition(isEmuNand ? EMUNAND : SYSNAND, p_idx[i])->sectors_count;
		for(int j = 0; j < size; j += sect_row){
			swprintf(tmp, _MAX_LFN, L"%08X / %08X", j * NAND_SECTOR_SIZE, size * NAND_SECTOR_SIZE);
			int x, y;
			ConsoleGetXY(&x, &y);
			y += 16 * 3;
			x += 8 * 2;
			ConsoleShow();
			DrawString(&bottomScreen, tmp, x, y, ConsoleGetTextColor(), ConsoleGetBackgroundColor());
			nand_readsectors(j, sect_row, BUF1, isEmuNand ? EMUNAND0 : SYSNAND, p_idx[i]);
			FileWrite(&out, BUF1, sect_row * NAND_SECTOR_SIZE, j * NAND_SECTOR_SIZE);
		}
		FileClose(&out);
	}
	print(strings[STR_PRESS_BUTTON_ACTION], strings[STR_BUTTON_A], strings[STR_CONTINUE]);
	ConsoleShow();
	WaitForButton(keys[KEY_A].mask);
}

void GenerateNandXorpads(){
	aes_ctr ctr;
	const char *filename = "0:rxTools/nand.fat16.xorpad";
	nand_partition *partition = GetNANDPartition(SYSNAND, NAND_PARTITION_CTR);
	size_t size_mb = (partition->sectors_count * NAND_SECTOR_SIZE + ((1 << 20) - 1)) >> 20;

	statusInit(size_mb, lang(SF_GENERATING), lang(S_FAT16_XORPAD));
	GetNANDCTR(&ctr);
	aes_add_ctr(&ctr, partition->first_sector * NAND_SECTOR_SIZE / AES_BLOCK_SIZE);
	CreatePad(&ctr, &(aes_key){NULL, 0, partition->keyslot, 0}, size_mb, filename, 0);
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
		if (*(uint32_t*)((uint8_t*)BUF1 + 0x100) == 'HCCN') {
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

void RebuildNand(){
	char* p_name[] = { "twln.bin", "twlp.bin", "agb_save.bin", "firm0.bin", "firm1.bin", "ctrnand.bin" };
	wchar_t* p_descr[] = { strings[STR_TWLN], strings[STR_TWLP], strings[STR_AGB_SAVE], strings[STR_FIRM0], strings[STR_FIRM1], strings[STR_CTRNAND] };
	nand_partition_index p_idx[] = { NAND_PARTITION_TWLN, NAND_PARTITION_TWLP, NAND_PARTITION_AGB_SAVE, NAND_PARTITION_FIRM0, NAND_PARTITION_FIRM1, NAND_PARTITION_CTRNAND};
	int sect_row = 0x1; //Slow, ok, but secure

	ConsoleInit();
	int isEmuNand = checkEmuNAND();
	ConsoleSetTitle(strings[STR_INJECT], strings[STR_NAND_PARTITIONS]);
	if(!isEmuNand){
		print(strings[STR_NO_EMUNAND]);
		print(strings[STR_PRESS_BUTTON_ACTION], strings[STR_BUTTON_A], strings[STR_CANCEL]);
		ConsoleShow();
		WaitForButton(keys[KEY_A].mask);
		return;
	}
	print(strings[STR_PROCESSING], isEmuNand ? strings[STR_EMUNAND] : strings[STR_SYSNAND]);
	const size_t wtmpLen = 256;
	wchar_t wtmp[wtmpLen];
	for(int i = 3; i < sizeof(p_idx) / sizeof(p_idx[0]); i++){ //Cutting out twln, twlp and agb_save. Todo: Properly decrypt them
		File out;
		swprintf(wtmp, wtmpLen, L"rxTools/nand/%ls%ls", isEmuNand ? L"emu_" : L"", p_name[i]);
		if(FileOpen(&out, wtmp, 0)){
			print(strings[STR_INJECTING], wtmp, p_descr[i]);
			ConsoleShow();

			size_t size = GetNANDPartition(isEmuNand ? EMUNAND : SYSNAND, p_idx[i])->sectors_count;
			for(int j = 0; j < size; j += sect_row){
				swprintf(wtmp, wtmpLen, L"%08X / %08X", j*NAND_SECTOR_SIZE, size * NAND_SECTOR_SIZE);
				int x, y;
				ConsoleGetXY(&x, &y);
				y += 16 * 3;
				x += 8 * 2;
				DrawString(&bottomScreen, wtmp, x, y, ConsoleGetTextColor(), ConsoleGetBackgroundColor());

				FileRead(&out, BUF1, sect_row*NAND_SECTOR_SIZE, j*NAND_SECTOR_SIZE);
				if (isEmuNand)
					nand_writesectors(j, sect_row, BUF1, EMUNAND, p_idx[i]);
			}
			FileClose(&out);
		}
	}
	print(strings[STR_PRESS_BUTTON_ACTION], strings[STR_BUTTON_A], strings[STR_CONTINUE]);
	ConsoleShow();
	WaitForButton(keys[KEY_A].mask);
}