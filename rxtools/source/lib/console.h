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

#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdarg.h>
#include <wchar.h>
#include "draw.h"

#define CONSOLE_SIZE			(0x4000)
#define CONSOLE_WIDTH			(320 - 16 * 2)
#define CONSOLE_HEIGHT			(240 - 16 * 3)
#define CONSOLE_MAX_LINES		(int)(CONSOLE_HEIGHT / 16 - 4)
#define CONSOLE_MAX_LINE_LENGTH		(int)(CONSOLE_WIDTH / 8 - 4)
#define CONSOLE_MAX_TITLE_LENGTH	(int)(320 / 8)
#define CONSOLE_X			((320 - CONSOLE_WIDTH) / 2)
#define CONSOLE_Y			((240 - CONSOLE_HEIGHT) / 2)

void ConsoleInit();
void ConsoleShow();
void ConsoleFlush();
void ConsoleAddText(wchar_t* str);
void ConsoleSetBackgroundColor(Color color);
Color ConsoleGetBackgroundColor();
void ConsoleSetBorderColor(Color color);
Color ConsoleGetBorderColor();
void ConsoleSetTextColor(Color color);
Color ConsoleGetTextColor();
void ConsoleSetXY(int x, int y);
void ConsoleGetXY(int *x, int *y);
void ConsoleSetWH(int width, int height);
void ConsoleSetTitle(const wchar_t *format, ...);
void ConsoleSetBorderWidth(int width);
int ConsoleGetBorderWidth(int width);
void ConsoleSetSpecialColor(Color color);
Color ConsoleGetSpecialColor();
void ConsoleSetSpacing(int space);
int ConsoleGetSpacing();

void print(const wchar_t *format, ...);
void vprint(const wchar_t *format, va_list va);

#endif
