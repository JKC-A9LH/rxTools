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
 * You should have received a copy of the GNU General Public Licenselan
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include "menu.h"
#include "crypto.h"
#include "fs.h"
#include "console.h"
#include "draw.h"
#include "hid.h"
#include "screenshot.h"
#include "firm.h"
#include "configuration.h"
#include "log.h"
#include "AdvancedFileManager.h"
#include "lang.h"
#include "theme.h"
#include "mpcore.h"
#include "strings.h"

static const wchar_t *const keyX16path = L"0:key_0x16.bin";
static const wchar_t *const keyX1Bpath = L"0:key_0x1B.bin";
static const wchar_t *const keyX25path = L"0:slot0x25KeyX.bin";
static const wchar_t *const rxRootPath = L"0:rxTools/%ls";
static const wchar_t *const fontPath = L"sys/font.bin";
static const wchar_t *const menuPath = L"sys/gui.json";

static const wchar_t *const jsonPattern = L"*.json";

#define JSON_SIZE	0x2000
#define JSON_TOKENS	(JSON_SIZE >> 3) //should be enough

static void setConsole() {
	ConsoleSetXY(15, 20);
	ConsoleSetWH(bottomScreen.w - 30, bottomScreen.h - 80);
	ConsoleSetBorderColor(BLUE);
	ConsoleSetTextColor(RGB(0, 141, 197));
	ConsoleSetBackgroundColor(TRANSPARENT);
	ConsoleSetSpecialColor(BLUE);
	ConsoleSetSpacing(2);
	ConsoleSetBorderWidth(3);
}

static void install() {
	f_mkdir(L"rxTools");
	f_mkdir(L"rxTools/nand");
	InstallConfigData();
}

bool initKeyX(uint_fast8_t slot, const wchar_t *path) {
	uint8_t buf[AES_BLOCK_SIZE];
	FIL f;

	if (!FileOpen(&f, path, false) ||
		(FileRead2(&f, buf, AES_BLOCK_SIZE) != AES_BLOCK_SIZE &&
		(FileClose(&f) || true)
	)) return false;
	FileClose(&f);
	setup_aeskeyX(slot, buf);
	return true;
}

static _Noreturn void mainLoop() {
	uint32_t pad;

	while (true) {
		pad = InputWait();
		if (pad & keys[KEY_DDOWN].mask)
			MenuNextSelection();

		if (pad & keys[KEY_DUP].mask)
			MenuPrevSelection();

		if (pad & keys[KEY_A].mask) {
			MenuSelect();
		}

		if (pad & keys[KEY_B].mask) {
			MenuClose();
		}

		MenuShow();
	}
}

__attribute__((section(".text.start"), noreturn)) void _start()
{

	wchar_t langDir[_MAX_LFN + 1];
	wchar_t themeDir[_MAX_LFN + 1];
	wchar_t path[_MAX_LFN + 1];
	size_t size;
	void *fontBuf;
	int r;
	FIL f;
	uint32_t pad;

	// Enable TMIO IRQ
	*(volatile uint32_t *)0x10001000 = 0x00010000;

	preloadStringsA();

	// init screen buffer addresses from const vectors
	bottomScreen.addr = (uint8_t*)*(uint32_t*)bottomScreen.addr;
	top1Screen.addr = (uint8_t*)*(uint32_t*)top1Screen.addr;
	top2Screen.addr = (uint8_t*)*(uint32_t*)top2Screen.addr;

	if (!FSInit()) {
		DrawInfo(NULL, lang(S_REBOOT, -1), lang(SF_FAILED_TO, -1), lang(S_MOUNT, -1), lang(S_FILE_SYSTEM, -1));
		Shutdown(true);		
	}

	setConsole();

	readCfg();

	swprintf(path, _MAX_LFN + 1, rxRootPath, fontPath);
	if (!FileOpen(&f, path, false) ||
		((fontBuf = __builtin_alloca(f.fsize)) &&
		FileRead2(&f, fontBuf, f.fsize) != f.fsize &&
		(FileClose(&f) || true)
	)) {
		fontIsLoaded = 0;
 		DrawInfo(NULL, lang(S_CONTINUE, -1), lang(SF_FAILED_TO, -1), lang(S_LOAD, -1), path);
	} else {
		fontIsLoaded = 1;
		FileClose(&f);
		font16.addr = fontBuf;
		font24 = (FontMetrics){12, 24, 24, font16.dwstart, font16.addr + font16.dw * font16.h * 0x10000 / (8 * sizeof(font24.addr[0]))};

		if (!(swprintf(langDir, _MAX_LFN + 1, rxRootPath, L"lang") > 0 &&
			(size = FileMaxSize(langDir, jsonPattern)) &&
			langInit(&(Json){__builtin_alloca(size), size, __builtin_alloca((size >> 2) * sizeof(jsmntok_t)), size >> 2}, langDir, jsonPattern) &&
			langLoad(cfgs[CFG_LANGUAGE].val.s, LANG_SET) &&
			swprintf(path, _MAX_LFN + 1, L"%ls/%s%ls", langDir, cfgs[CFG_LANGUAGE].val.s, wcsrchr(jsonPattern, L'.'))
		)) DrawInfo(NULL, lang(S_CONTINUE, -1), lang(SF_FAILED_TO, -1), lang(S_LOAD, -1), path);
	 		
		preloadStringsU();
	}
	
	r = loadStrings();

	swprintf(path, _MAX_LFN + 1, rxRootPath, menuPath);
	if (!(size = FileSize(path)) ||
		!menuLoad(&(Json){__builtin_alloca(size), size, __builtin_alloca((size >> 2) * sizeof(jsmntok_t)), size >> 2}, path)
	) DrawInfo(NULL, lang(S_CONTINUE, -1), lang(SF_FAILED_TO, -1), lang(S_LOAD, -1), path);

	if (getMpInfo() != MPINFO_KTR || (
		(initKeyX(0x16, keyX16path) || (DrawInfo(lang(S_NO_KTR_KEYS, -1), lang(S_CONTINUE, -1), lang(SF_FAILED_TO, -1), lang(S_LOAD, -1), keyX16path) && false)) &&
		(initKeyX(0x1B, keyX1Bpath) || (DrawInfo(lang(S_NO_KTR_KEYS, -1), lang(S_CONTINUE, -1), lang(SF_FAILED_TO, -1), lang(S_LOAD, -1), keyX1Bpath) && false))
	)) install();

	if (!(swprintf(themeDir, _MAX_LFN + 1, rxRootPath, L"theme") > 0 &&
		(size = FileMaxSize(themeDir, jsonPattern)) &&
		themeInit(&(Json){__builtin_alloca(size), size, __builtin_alloca((size >> 2) * sizeof(jsmntok_t)), size >> 2}, themeDir, jsonPattern) &&
		themeLoad(cfgs[CFG_THEME].val.s, THEME_SET) &&
		swprintf(path, _MAX_LFN + 1, L"%ls/%s%ls", themeDir, cfgs[CFG_THEME].val.s, wcsrchr(jsonPattern, L'.')) > 0
	)) DrawInfo(NULL, lang(S_CONTINUE, -1), lang(SF_FAILED_TO, -1), lang(S_LOAD, -1), path);

	pad = GetInput();

	if (~pad & keys[cfgs[CFG_GUI_FORCE].val.i].mask) {
		if (pad & keys[cfgs[CFG_EMUNAND_FORCE].val.i].mask)
			rxMode(1);
		else if (pad & keys[cfgs[CFG_SYSNAND_FORCE].val.i].mask)
			rxMode(0);
		else if (pad & keys[cfgs[CFG_PASTA_FORCE].val.i].mask)
			PastaMode();
		else switch (cfgs[CFG_BOOT_DEFAULT].val.i) {
			case BOOT_EMUNAND:
				rxMode(1);
			case BOOT_SYSNAND:
				rxMode(0);
			case BOOT_PASTA:
				PastaMode();
		}
	}
	if (r < 0) { //Fallback to Emu- or SysNAND if UI should be loaded but theme is not available
 		DrawInfo(NULL, lang(S_CONTINUE, -1), lang(SF_FAILED_TO, -1), lang(S_LOAD, -1), path);
		rxMode(checkEmuNAND() ? 1 : 0);
	}

	if (sysver < 7 && !initKeyX(0x25, keyX25path))
		DrawInfo(lang(S_NO_KEYX25, -1), lang(S_CONTINUE, -1), lang(SF_FAILED_TO, -1), lang(S_LOAD, -1), keyX25path);

	MenuInit();
	MenuShow();
	mainLoop();
}
