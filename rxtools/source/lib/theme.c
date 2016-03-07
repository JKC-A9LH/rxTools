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

#include <string.h>
#include <wchar.h>
#include "theme.h"

const wchar_t *themeDir = L"/rxTools/theme";
const wchar_t *themeFile = L"theme.json";
const wchar_t *themePath = L"%ls/%ls/%ls";
wchar_t themeName[_MAX_LFN + 1] = L"";

#define THEME_JSON_SIZE		0x2000
#define THEME_JSON_TOKENS	0x200

char jst[THEME_JSON_SIZE];
jsmntok_t tokt[THEME_JSON_TOKENS];
Json themeJson = {jst, THEME_JSON_SIZE, tokt, THEME_JSON_TOKENS};

themeStyle style, styleDefault = {
	L"",
	L"",
	L"",
	{WHITE, TRANSPARENT},
	{0, 0, 0, 24},
	{WHITE, TRANSPARENT},
	{BLACK, WHITE},
	{GREY, TRANSPARENT},
	{BLACK, GREY},
	{0, 24, 160, 0},
	{WHITE, TRANSPARENT},
	{160, 24, 0, 0},
	{WHITE, TRANSPARENT},
	{160, 24, 0, 16}
};

typedef enum {
	OBJ_NONE,
	OBJ_MENU,
	OBJ_CAPTION,
	OBJ_ITEMS,
	OBJ_DESCRIPTION,
	OBJ_VALUE
} objtype;

enum {
	TOP1,
	TOP2,
	BOTTOM,
	CAPTIONFG,
	CAPTIONBG,
	CAPTIONX,
	CAPTIONY,
	CAPTIONW,
	CAPTIONH,
	ITEMSX,
	ITEMSY,
	ITEMSW,
	ITEMSH,
	ITEMSCOLORFG,
	ITEMSCOLORBG,
	ITEMSSELECTEDFG,
	ITEMSSELECTEDBG,
	ITEMSDISABLEDFG,
	ITEMSDISABLEDBG,
	ITEMSUNSELECTEDFG,
	ITEMSUNSELECTEDBG,
	DESCRIPTIONX,
	DESCRIPTIONY,
	DESCRIPTIONW,
	DESCRIPTIONH,
	DESCRIPTIONFG,
	DESCRIPTIONBG,
	VALUEX,
	VALUEY,
	VALUEW,
	VALUEH,
	VALUEFG,
	VALUEBG,
	IDX_COUNT
};

int colorParse(int s, char *key, int *idx, int coloridx);

int themeParse(int s, objtype type, char *key, int *idx) {
	if (s == themeJson.count || key == NULL)
		return 0;
	if (themeJson.tok[s].type == JSMN_PRIMITIVE || themeJson.tok[s].type == JSMN_STRING)
		return 1;
	else if (themeJson.tok[s].type == JSMN_OBJECT) {
		int i, j = 0, k;
		bool isTarget = false;
		for (i = 0; i < themeJson.tok[s].size; i++) {
			j++;
			if (themeJson.tok[s+j+1].type == JSMN_OBJECT && strncmp(key, themeJson.js + themeJson.tok[s+j].start, themeJson.tok[s+j].end - themeJson.tok[s+j].start) == 0)
				isTarget = true;

			switch (themeJson.js[themeJson.tok[s+j].start]) {
				case 'b': //"bottomimg"
					idx[BOTTOM] = s + ++j;
					break;
				case 'c'://"color"
					switch(type) {
						case OBJ_CAPTION: //"color"
							j += colorParse(s+j+1, key, idx, CAPTIONFG);
							break;
						case OBJ_ITEMS:
							j += colorParse(s+j+1, key, idx, ITEMSCOLORFG);
							break;
						case OBJ_DESCRIPTION:
							j += colorParse(s+j+1, key, idx, DESCRIPTIONFG);
							break;
						case OBJ_VALUE:
							j += colorParse(s+j+1, key, idx, VALUEFG);
							break;
						case OBJ_MENU: //"caption"
							type = OBJ_CAPTION;
						default:
							j += themeParse(s+j+1, type, key, idx);
					}
					break;
				case 'd':
					switch(type) {
						case OBJ_ITEMS: //"disabled"
							j += colorParse(s+j+1, key, idx, ITEMSDISABLEDFG);
							break;
						default: //"description"
							j += themeParse(s+j+1, OBJ_DESCRIPTION, key, idx);
					}
					break;
				case 'i': //"items"
					j += themeParse(s+j+1, OBJ_ITEMS, key, idx);
					break;
				case 's': //"selected"
					j += colorParse(s+j+1, key, idx, ITEMSSELECTEDFG);
					break;
				case 'u': //"unselected"
					j += colorParse(s+j+1, key, idx, ITEMSUNSELECTEDFG);
					break;
/*				case 'h': //"hint"
					j += colorParse(s+j+1, key, idx, DESCRIPTIONFG);
					break;
*/				case 'v': //"value"
					j += themeParse(s+j+1, OBJ_VALUE, key, idx);
					break;
				case 't': //"topimg"
					if (themeJson.tok[s+j+1].type == JSMN_STRING)
						idx[TOP1] = idx[TOP2] = s + j + 1;
					else if (themeJson.tok[s+j+1].type == JSMN_ARRAY && themeJson.tok[s+j+1].size > 0) {
						idx[TOP1] = s + j + 2;
						if (themeJson.tok[s+j+1].size > 1)
							idx[TOP2] = s + j + 3;
					}
					j += themeParse(s+j+1, type, key, idx);
					break;
				case 'h': //"height"
					switch(type) {
						case OBJ_CAPTION:
							idx[CAPTIONH] = s + ++j;
							break;
						case OBJ_ITEMS:
							idx[ITEMSH] = s + ++j;
							break;
						case OBJ_DESCRIPTION:
							idx[DESCRIPTIONH] = s + ++j;
							break;
						case OBJ_VALUE:
							idx[VALUEH] = s + ++j;
							break;
						default:
							j++;
					}
					break;
				case 'w': //"width"
					switch(type) {
						case OBJ_CAPTION:
							idx[CAPTIONW] = s + ++j;
							break;
						case OBJ_ITEMS:
							idx[ITEMSW] = s + ++j;
							break;
						case OBJ_DESCRIPTION:
							idx[DESCRIPTIONW] = s + ++j;
							break;
						case OBJ_VALUE:
							idx[VALUEW] = s + ++j;
							break;
						default:
							j++;
					}
					break;
				case 'x': //"x"
					switch(type) {
						case OBJ_CAPTION:
							idx[CAPTIONX] = s + ++j;
							break;
						case OBJ_ITEMS:
							idx[ITEMSX] = s + ++j;
							break;
						case OBJ_DESCRIPTION:
							idx[DESCRIPTIONX] = s + ++j;
							break;
						case OBJ_VALUE:
							idx[VALUEX] = s + ++j;
							break;
						default:
							j++;
					}
					break;
				case 'y': //"y"
					switch(type) {
						case OBJ_CAPTION:
							idx[CAPTIONY] = s + ++j;
							break;
						case OBJ_ITEMS:
							idx[ITEMSY] = s + ++j;
							break;
						case OBJ_DESCRIPTION:
							idx[DESCRIPTIONY] = s + ++j;
							break;
						case OBJ_VALUE:
							idx[VALUEY] = s + ++j;
							break;
						default:
							j++;
					}
					break;
				case 'm': //"menu"
					type = OBJ_MENU;
				default:
					if (isTarget) { //target object - get member indexes and terminate
						themeParse(s+j+1, type, key, idx);
						return 0;
					} else {
						int localidx[IDX_COUNT] = {0};
						if ((k = themeParse(s+j+1, type, key, localidx)) == 0 || themeJson.js[themeJson.tok[s+j].start] == 'm') {
						//object is in target chain - set inherited indexes and terminate or apply root 'menu' for unset parameters
							for (int l = 0; l < IDX_COUNT; l++)
								if (idx[l] == 0)
									idx[l] = localidx[l];
							if (k == 0)
								return 0;
						}
					}
					j += k;
			}
		}
		return j + 1;
	} else if (themeJson.tok[s].type == JSMN_ARRAY) {
		int i, j = 0;
		for (i = 0; i < themeJson.tok[s].size; i++)
			j += themeParse(s+j+1, type, key, idx);
		return j + 1;
	}
	return 0;
}

int colorParse(int s, char *key, int *idx, int coloridx) {
	if (themeJson.tok[s].type == JSMN_STRING)
		idx[coloridx] = s;
	else if (themeJson.tok[s].type == JSMN_ARRAY && themeJson.tok[s].size > 0) {
		idx[coloridx] = s + 1;
		if (themeJson.tok[s].size > 1)
			idx[coloridx + 1] = s + 2;
	}
	return themeParse(s, OBJ_NONE, key, idx);
}

void setImg(wchar_t *path, int index) {
	if (index > 0) {
		wchar_t fn[_MAX_LFN + 1];
		fn[mbstowcs(fn, themeJson.js + themeJson.tok[index].start, themeJson.tok[index].end - themeJson.tok[index].start)] = 0;
		swprintf(path, _MAX_LFN, themePath, themeDir, themeName, fn);
	}
}

void setColor(uint32_t *color, int index) {
	if (index > 0)
		*color = strtoul(themeJson.js + themeJson.tok[index].start, NULL, 16);
}

void setInt(uint32_t *val, int index) {
	if (index > 0)
		*val = strtoul(themeJson.js + themeJson.tok[index].start, NULL, 0);
}

void themeStyleSet(char *key) {
	style = styleDefault;
	int idx[IDX_COUNT] = {0};
	themeParse(0, OBJ_NONE, key, idx);
	setImg(style.top1img, idx[TOP1]);
	setImg(style.top2img, idx[TOP2]);
	setImg(style.bottomimg, idx[BOTTOM]);
	setColor(&style.captionColor.fg, idx[CAPTIONFG]);
	setColor(&style.captionColor.bg, idx[CAPTIONBG]);
	setColor(&style.itemsColor.fg, idx[ITEMSCOLORFG]);
	setColor(&style.itemsColor.bg, idx[ITEMSCOLORBG]);
	setColor(&style.itemsSelected.fg, idx[ITEMSSELECTEDFG]);
	setColor(&style.itemsSelected.bg, idx[ITEMSSELECTEDBG]);
	setColor(&style.itemsDisabled.fg, idx[ITEMSDISABLEDFG]);
	setColor(&style.itemsDisabled.bg, idx[ITEMSDISABLEDBG]);
	setColor(&style.itemsUnselected.fg, idx[ITEMSUNSELECTEDFG]);
	setColor(&style.itemsUnselected.bg, idx[ITEMSUNSELECTEDBG]);
	setColor(&style.descriptionColor.fg, idx[DESCRIPTIONFG]);
	setColor(&style.descriptionColor.bg, idx[DESCRIPTIONBG]);
	setColor(&style.valueColor.fg, idx[VALUEFG]);
	setColor(&style.valueColor.bg, idx[VALUEBG]);
	setInt(&style.captionRect.x, idx[CAPTIONX]);
	setInt(&style.captionRect.y, idx[CAPTIONY]);
	setInt(&style.captionRect.w, idx[CAPTIONW]);
	setInt(&style.captionRect.h, idx[CAPTIONH]);
	setInt(&style.itemsRect.x, idx[ITEMSX]);
	setInt(&style.itemsRect.y, idx[ITEMSY]);
	setInt(&style.itemsRect.w, idx[ITEMSW]);
	setInt(&style.itemsRect.h, idx[ITEMSH]);
	setInt(&style.descriptionRect.x, idx[DESCRIPTIONX]);
	setInt(&style.descriptionRect.y, idx[DESCRIPTIONY]);
	setInt(&style.descriptionRect.w, idx[DESCRIPTIONW]);
	setInt(&style.descriptionRect.h, idx[DESCRIPTIONH]);
	setInt(&style.valueRect.x, idx[VALUEX]);
	setInt(&style.valueRect.y, idx[VALUEY]);
	setInt(&style.valueRect.w, idx[VALUEW]);
	setInt(&style.valueRect.h, idx[VALUEH]);
}

int themeLoad(char *name, themeSeek seek) {
	DIR dir;
	FILINFO fno;
	wchar_t *fn;
	wchar_t path[_MAX_LFN + 1];
	wchar_t pathfn[_MAX_LFN + 1];
	wchar_t targetfn[_MAX_LFN + 1];
	wchar_t prevfn[_MAX_LFN + 1] = L"";
	wchar_t lfn[_MAX_LFN + 1];
	fno.lfname = lfn;
	fno.lfsize = _MAX_LFN + 1;

	targetfn[mbstowcs(targetfn, name, _MAX_LFN + 1)] = 0;

	if (f_findfirst(&dir, &fno, themeDir, L"*") == FR_OK) {
		wcscpy(pathfn, *fno.lfname ? fno.lfname : fno.fname);
		do {
			fn = *fno.lfname ? fno.lfname : fno.fname;
			if (swprintf(path, _MAX_LFN + 1, themePath, themeDir, fn, themeFile) > 0 && f_stat(path, NULL) == FR_OK) {
				if (wcscmp(fn, targetfn) == 0) {
					if (seek == THEME_SET)
						wcscpy(pathfn, targetfn);
					else if (seek == THEME_NEXT) {
						while (f_findnext(&dir, &fno) == FR_OK && *fno.fname) {
							fn = *fno.lfname ? fno.lfname : fno.fname;
							if (swprintf(path, _MAX_LFN + 1, themePath, themeDir, fn, themeFile) > 0 && f_stat(path, NULL) == FR_OK) {
								wcscpy(pathfn, fn);
								break;
							}
						}
					} else if (seek == THEME_PREV && !*prevfn)
						continue;
					break;
				} else if (seek == THEME_PREV)
					wcscpy(prevfn, fn);
			}
		} while (f_findnext(&dir, &fno) == FR_OK && *fno.fname);
		if (seek == THEME_PREV && *prevfn)
			wcscpy(pathfn, prevfn);
		swprintf(path, _MAX_LFN + 1, themePath, themeDir, pathfn, themeFile);
		themeJson = (Json){jst, THEME_JSON_SIZE, tokt, THEME_JSON_TOKENS};
		if (jsonLoad(&themeJson, path) > 0) {
			wcscpy(themeName, pathfn);
			name[wcstombs(name, pathfn, 31)] = 0;
		}
	} else
		themeJson.count = 0;
	f_closedir(&dir);
	return themeJson.count;
}