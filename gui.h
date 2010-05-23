/*
 * Created: 05/20/2010 10:10:43 PM
 * zhaoshicao@gmail.com
 * fangwei@mail.ustc.edu.cn
 * 
 * ====
 * This material is provided "as is", with absolutely no warranty expressed
 * or implied. Any use is at your own risk.
 *
 * Permission to use or copy this software for any purpose is hereby granted 
 * without fee, provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 *
 */

#ifndef _GUI_H_
#define _GUI_H_

#include <ncurses.h>

#include "client.h"

void init_curse();
void drawinfo();
void drawpubcard(WINDOW* w, int cdno);
void drawplayer(struct WIN2 *w, int plno);
void drawchip(WINDOW *w);
void drawirc(struct WIN2 *w);

#endif

