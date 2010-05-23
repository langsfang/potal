/*
 * Created: 05/20/2010 10:04:24 PM
 * zhaoshicao@gmail.com
 * for : 
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

#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <ncurses.h>

#include "constant.h"
#include "typedef.h"

#define TIMER        2

#define _WIN_COLOR(win, _statments, _color)\
    do {\
        wattrset(win, COLOR_PAIR(_color));\
        {\
            _statments\
        }\
        wattrset(win, COLOR_PAIR(0));\
    } while (0)

struct WIN2 {
    WINDOW *t, *d[2];
};

extern int mode, turn, limit, nwin;
extern int myid, pot, rtime;
extern int need_update;
extern int allow_send_ready;

extern struct WIN2 *wplayer[MAX_PLAYER];
extern PLE_BASE playerinfo[MAX_PLAYER];
extern int seat[MAX_PLAYER];
extern int win[MAX_PLAYER];

extern struct WIN2 *wirc;
extern WINDOW *pirc;
extern char ircmsg[MAX_LEN+1];

extern WINDOW *wchip;
extern int chipin;

extern WINDOW *wcard[NUM_PUBCARDS];
extern CARD pubcard[NUM_PUBCARDS];

void init_vars();

#endif

