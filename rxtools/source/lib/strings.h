/*
 * Copyright (C) 2016 dukesrg
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

#ifndef STRINGS_H
#define STRINGS_H

#define STR(X, Y) static const char *const Y = X;

STR("Enabled", S_ENABLED)
STR("Disabled", S_DISABLED)
STR("|", S_PARENT_SEPARATOR)

STR("Japan", S_JAPAN)
STR("North America", S_NORTH_AMERICA)
STR("Europe/Australia", S_EUROPE_AUSTRALIA)
STR("China", S_CHINA)
STR("South Korea", S_SOUTH_KOREA)
STR("Taiwan", S_TAIWAN)
STR("Unknown", S_UNKNOWN)

//STR("Booting %ls...", SF_BOOTING)
STR("rxTools GUI", S_RXTOOLS_GUI)
STR("rxMode SysNAND", S_RXTOOLS_SYSNAND)
STR("rxMode EmuNAND", S_RXTOOLS_EMUNAND)
STR("Pasta mode", S_PASTA_MODE)

STR("Generating %ls XORpad", SF_GENERATING_XORPAD)
STR("SD", S_SD)
STR("NCCH", S_NCCH)

STR("Decrypting %16s", SF_DECRYPTING)

STR("Dumping %ls", SF_DUMPING)
STR("Injecting %ls", SF_INJECTING)
STR("Copying %ls to %ls", SF2_COPYING)
STR("Dumping %ls %ls partition", SF2_DUMPING_PARTITION)
STR("Injecting %ls %ls partition", SF2_INJECTING_PARTITION)
STR("Copying %ls partition from %ls to %ls", SF3_COPYING_PARTITION)
STR("Dumping %ls file %ls", SF2_DUMPING_FILE)
STR("Injecting %ls file %ls", SF2_INJECTING_FILE)

STR("SysNAND", S_SYSNAND)
STR("EmuNAND", S_EMUNAND)
STR("EmuNAND1", S_EMUNAND1)
STR("EmuNAND2", S_EMUNAND2)

STR("TWL", S_TWL)
STR("AGB_SAVE", S_AGB_SAVE)
STR("FIRM0", S_FIRM0)
STR("FIRM1", S_FIRM1)
STR("CTR", S_CTR)
STR("NCSD5", S_NCSD5)
STR("NCSD6", S_NCSD6)
STR("NCSD7", S_NCSD7)
STR("TWLN", S_TWLN)
STR("TWLP", S_TWLP)
STR("TWL2", S_TWL2)
STR("TWL3", S_TWL3)
STR("CTRNAND", S_CTRNAND)
STR("CTR1", S_CTR1)
STR("CTR2", S_CTR2)
STR("CTR3", S_CTR3)

//STR("Error reading %ls!", SF_ERROR_READING)
STR("Failed to %ls %ls!", SF_FAILED_TO)
STR("mount", S_MOUNT) //don't need a translation
//STR("read", S_READ)
//STR("write", S_WRITE)
STR("load", S_LOAD)
//STR("save", S_SAVE)
STR("file system", S_FILE_SYSTEM) //don't need a translation
//STR("font", S_FONT)
//STR("theme", S_THEME)
//STR("translation", S_TRANSLATION)
//STR("GUI", S_GUI)
STR("This file with a valid key must exist in the SD card root in order to boot rxMode on New 3DS.", S_NO_KTR_KEYS)
STR("This file with a valid key must exist in the SD card root in order to decrypt newer titles on firmware version below 7.0.0 and to use newer gamecard saves on firmware below 6.0.0.", S_NO_KEYX25)
STR("Press %ls to %ls.", SF2_PRESS_BUTTON_ACTION)
STR("any button", S_ANY_BUTTON)
STR("continue", S_CONTINUE)
STR("reboot", S_REBOOT)
STR("cancel", S_CANCEL)
STR("Completed!", S_COMPLETED)
STR("Canceled!", S_CANCELED)
STR("[A]", S_BUTTON_A)
STR("[B]", S_BUTTON_B)
STR("[X]", S_BUTTON_X)
STR("[Y]", S_BUTTON_Y)
STR("[L]", S_BUTTON_L)
STR("[R]", S_BUTTON_R)
STR("[UP]", S_BUTTON_UP)
STR("[DOWN]", S_BUTTON_DOWN)
STR("[LEFT]", S_BUTTON_LEFT)
STR("[RIGHT]", S_BUTTON_RIGHT)
STR("[SELECT]", S_BUTTON_SELECT)
STR("[START]", S_BUTTON_START)

#endif
