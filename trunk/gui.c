/*
 * Created: 05/20/2010 10:16:02 PM
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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ncurses.h>
#include <string.h>

#include "constant.h"
#include "typedef.h"
#include "card.h"

#include "client.h"
#include "gui.h"

#define _max_three(res, a, b, c)\
    do {\
        int _max_three_t;\
        res = (_max_three_t = ((a) > (b)) ? (a) : (b)) > (c) ? _max_three_t : c;\
    } while (0)

#define VLINE        0 
#define HLINE        0

static const char *fill  =  "++++++++++++++++++++++++++++++++++++++++\n"
                            "++++++++++++++++++++++++++++++++++++++++\n"
                            "++++++++++++++++++++++++++++++++++++++++\n"
                            "++++++++++++++++++++++++++++++++++++++++\n"
                            "++++++++++++++++++++++++++++++++++++++++\n"
                            "++++++++++++++++++++++++++++++++++++++++\n"
                            "++++++++++++++++++++++++++++++++++++++++\n"
                            "++++++++++++++++++++++++++++++++++++++++\n"
                            "++++++++++++++++++++++++++++++++++++++++\n"
                            "++++++++++++++++++++++++++++++++++++++++\n";
static const char *empty =  "                                        \n"
                            "                                        \n"
                            "                                        \n"
                            "                                        \n"
                            "                                        \n"
                            "                                        \n"
                            "                                        \n"
                            "                                        \n"
                            "                                        \n"
                            "                                        \n";

#ifdef NCURSESW
static const char *kind[4] = {
    "♥ ", "♦ ", "♣ ", "♠ "};
#else
static const char *kind[4] = {
    "H", "D", "C", "S"};
#endif

static const char *num[13] = {
    "2", "3", "4", "5", "6", "7", "8",
    "9", "10", "J", "Q", "K", "A"};

struct layout {
    int player_height, player_width;
    int pubcard_height, pubcard_width;
    int chip_height, chip_width;
    int irc_width;
};

static const struct layout min_layout = {9, 14, 3, 5, 3, 7, 10};
static struct layout layout = {12, 16, 8, 10, 3, 7, 20};

static void drawcard(WINDOW *w, int height, int width, CARD card, int status)
{
    if (w == NULL) return;

    int color = 1;
    mvwprintw(w, 0, 0, empty);

    switch (status) {
        case 0:
            mvwprintw(w, 0, 0, fill);
            break;
        case 1:
            switch (card.c) {
                case 0: /* HEART, red */
                    color = 1;
                    break;
                case 1: /* DIAMOND, cyan */
                    color = 6;
                    break;
                case 2: /* CLUB, blue */
                    color = 3;
                    break;
                case 3: /* SPADE, green */
                    color = 2;
                    break;
                default:
                    goto fn_exit;
            }
            _WIN_COLOR(w,
                    mvwprintw(w, (height-1)/2, width/2-2, "%s%s", 
                        kind[card.c], num[card.i]);
                    , color);
            break;
        default:
            break;
    }

fn_exit:    
    box(w, VLINE, HLINE);
    wrefresh(w);
}

void drawinfo()
{
    int startx = (COLS-layout.irc_width)/2-5;
    int starty = layout.player_height/2+LINES/4-layout.pubcard_height/4;

    curs_set(0);

    mvprintw(starty, startx, empty);
    mvprintw(starty, startx, "pot: %d\n", pot);
    refresh();
}

static void initpubcard(WINDOW **w, int cdno)
{
    int startx, starty;
    int height = layout.pubcard_height, 
        width  = layout.pubcard_width;

    startx = (COLS-layout.irc_width)/2-NUM_PUBCARDS*width/2+width*cdno;
    starty = LINES/2-height/2; 
    
    *w = newwin(height, width, starty, startx);
}

void drawpubcard(WINDOW *w, int cdno)
{
    int startx, starty;
    int height = layout.pubcard_height, 
        width  = layout.pubcard_width;

    if (w == NULL) return;
    curs_set(0);

    int status = 1;
    if (pubcard[cdno].c == 4) {
        status = 2;
    }

    drawcard(w, height, width, pubcard[cdno], status);
}

static void initplayer(struct WIN2 **w, int plno)
{
    int startx, starty;
    int height = layout.player_height, 
        width  = layout.player_width;

    switch (plno) {
        case 0:
            startx = (COLS-layout.irc_width)/2-width;
            starty = LINES-height;
            break;
        case 1: case 2: 
            startx = 0;
            starty = height/2+(2-plno)*(LINES-height)/2+(LINES-height)/4;
            break;
        case 7: case 8:
            startx = COLS-layout.irc_width-width;
            starty = height/2+(plno-7)*(LINES-height)/2+(LINES-height)/4;
            break;
        case 3: case 4: case 5: case 6:
            startx = (plno-3)*(COLS-layout.irc_width)/4+(COLS-layout.irc_width)/8-width/2;
            starty = 0;
            break;    
    } 
   
    int card_height = height-6, card_width = width/2;

    *w = malloc(sizeof(struct WIN2)); 

    (*w)->t = newwin(height, width, starty, startx);
    (*w)->d[0] = derwin((*w)->t, card_height, card_width, 2, 0);
    (*w)->d[1] = derwin((*w)->t, card_height, card_width, 2, width-width/2);
}

void drawplayer(struct WIN2 *w, int plno)
{
    int height = layout.player_height, 
        width  = layout.player_width;

    int card_height = height-6, card_width = width/2;

    if (w == NULL) return;
    curs_set(0);

    WINDOW *t = w->t;
    WINDOW *d[2];
    d[0] = w->d[0]; d[1] = w->d[1];

    int id = seat[plno];

    /* draw basic status */
    mvwprintw(t, 0, 0, empty);
    if (id == -1) {
        wrefresh(t);
        return;
    }

    int color = win[id] ? 1 : 0;
    _WIN_COLOR(t, 
            mvwprintw(t, 0, 0, playerinfo[id].name);
            mvwprintw(t, 1, 0, "$%d", playerinfo[id].money);
            mvwprintw(t, height-4, 0, "$%d", playerinfo[id].bet);
            , color);

    /* draw ready status */
    char t_str[MAX_LEN+1];
    switch (playerinfo[id].ready) {
        case 0:
            strncpy(t_str, "ready", MAX_LEN+1);
            color = 0;
            break;
        case 1:
            strncpy(t_str, "ready", MAX_LEN+1);
            color = 1;
            break;
        case 2:
            if (playerinfo[id].money > 0) {
                strncpy(t_str, "     ", MAX_LEN+1);
                color = 0;
                break;
            } else {
                strncpy(t_str, "allin", MAX_LEN+1);
                color = 1;
                break;
            }
        case 3:
            strncpy(t_str, "watch", MAX_LEN+1);
            color = 1;
            break;
    }
    _WIN_COLOR(t,
            mvwprintw(t, 1, width-5, t_str);
            , color);

    /* darw timer */
    mvwprintw(t, height-3, 0, "%c", '[');
    int i;
    for (i = 1; i <= 10 ; ++i) {
        if (id == turn && i <= rtime) {
            color = (rtime <= 5) ? 1 : 0;
            _WIN_COLOR(t,
                    mvwprintw(t, height-3, i, "%c", '#');
                    , color);
        } else {
            mvwprintw(t, height-3, i, "%c", ' ');
        }
    }
    mvwprintw(t, height-3, i, "%c", ']');
    
    /* draw cards */
    int status = (nwin > 0) ? 1 : 0;
    if (id == myid) status = 1;

    if (playerinfo[id].fold == 1
            && playerinfo[id].money > 0
            && !win[id])
        status = 2;

    if (nwin < 0 && id != myid)
        status = 2;

    if (playerinfo[id].c1.c == 4) 
        status = 2;

    drawcard(d[0], card_height, card_width, 
            playerinfo[id].c1, status);
    drawcard(d[1], card_height, card_width, 
            playerinfo[id].c2, status);

    /* draw suit pattern  */
    if (nwin > 0 && status != 2) {
        char k_w[MAX_LEN+1], n_w[MAX_LEN+1];
        CARD x[NUM_CARDS];

        int i;
        for (i = 0; i < NUM_PUBCARDS; ++i) {
            x[i] = pubcard[i]; 
        }

        x[NUM_PUBCARDS]   = playerinfo[id].c1;
        x[NUM_PUBCARDS+1] = playerinfo[id].c2;

        get_w_str(k_w, n_w, x);
        mvwprintw(t, height-2, 0, k_w);
        mvwprintw(t, height-1, 0, n_w);
    } else if (nwin < 0 && playerinfo[id].ready != 3) {
        mvwprintw(t, height-1, 0, "all fold");
    } else {
        mvwprintw(t, height-2, 0, empty);
    }

    wrefresh(t);
}

static void initchip(WINDOW **w)
{
    int startx, starty;
    int height = layout.chip_height, 
        width  = layout.chip_width;

    startx = (COLS-layout.irc_width)/2+1;
    starty = LINES-height-3;

    (*w) = newwin(height, width, starty, startx);
}

void drawchip(WINDOW *w)
{
    int height = layout.chip_height, 
        width  = layout.chip_width;

    if (w == NULL) return;
    curs_set(1);

    mvwprintw(w, 1, 1, empty);
    mvwprintw(w, 1, 1, "%d", chipin);
    int color = (mode == 0) ? 1 : 0;
    _WIN_COLOR(w,
            box(w, VLINE, HLINE);
            , color);

    wrefresh(w);
}

static void initirc(struct WIN2 **w)
{
    int startx = COLS-layout.irc_width;
    int starty = 0;
    int height = LINES,
        width  = layout.irc_width;

    *w = malloc(sizeof(struct WIN2)); 
    (*w)->t = newwin(height, width, starty, startx);
    (*w)->d[0] = derwin((*w)->t, height-3, width, 0, 0);
    (*w)->d[1] = derwin((*w)->t, 3, width, LINES-3, 0);

    pirc = newpad(height-5, width-2);
    scrollok(pirc, TRUE);
}

void drawirc(struct WIN2 *w)
{
    int startx = COLS-layout.irc_width;
    int starty = 0;
    int height = LINES,
        width  = layout.irc_width;

    curs_set(1);

    if (w == NULL || pirc == NULL) return;

    WINDOW *d[2];
    d[0] = w->d[0]; d[1] = w->d[1];

    int index = strlen(ircmsg)-width+2+1;
    if (index < 0) index = 0;
    mvwprintw(d[1], 1, 1, empty);
    mvwprintw(d[1], 1, 1, ircmsg+index);

    box(d[0], VLINE, HLINE);
    int color = (mode == 0) ? 0 : 1;
    _WIN_COLOR(d[1],
            box(d[1], VLINE, HLINE);
            , color);

    wrefresh(d[0]);
    prefresh(pirc, 0, 0, starty+1, startx+1, starty+height-5, startx+width-2);
    wrefresh(d[1]);
}

static void get_min_size(struct layout layout, int *min_height, int *min_width)
{
    int min_h1 = 3*layout.player_height,
        min_h2 = 2*layout.player_height+layout.pubcard_height+2,
        min_h3 = layout.player_height+layout.pubcard_height+layout.chip_width+MAX_PLAYER+2;
    _max_three(*min_height, min_h1, min_h2, min_h3);

    int min_w1 = layout.irc_width+5*layout.pubcard_width+2*layout.player_width,
        min_w2 = layout.irc_width+4*layout.player_width,
        min_w3 = layout.irc_width+3*layout.player_width+layout.chip_width;
    _max_three(*min_width, min_w1, min_w2, min_w3);
}

void init_gui()
{
    initscr();
    cbreak();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    noecho();
    curs_set(0);
    refresh();

    if (has_colors()) {
        start_color();

        init_pair(1, COLOR_RED,     COLOR_BLACK);
        init_pair(2, COLOR_GREEN,   COLOR_BLACK);
        init_pair(3, COLOR_YELLOW,  COLOR_BLACK);
        init_pair(4, COLOR_BLUE,    COLOR_BLACK);
        init_pair(5, COLOR_CYAN,    COLOR_BLACK);
        init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(7, COLOR_WHITE,   COLOR_BLACK);
    }

    int min_height, min_width;
    get_min_size(layout, &min_height, &min_width);

    if (LINES < min_height || COLS < min_width) {
        layout = min_layout;
    }

    get_min_size(layout, &min_height, &min_width);

    if (LINES < min_height || COLS < min_width) {
        endwin();
        fprintf(stderr, "screen is too small\n");
        fprintf(stderr, "min size: %d %d\n", min_height, min_width);
        fprintf(stderr, "now size: %d %d\n", LINES, COLS);
        exit(-1);
    }

    int i;
    for (i = 0; i < MAX_PLAYER; ++i) {
        initplayer(&wplayer[i], i);
    }

    for (i = 0; i < NUM_PUBCARDS; ++i) {
        initpubcard(&wcard[i], i);
    }

    initchip(&wchip);
    initirc(&wirc);
}

void clean_gui()
{
    delwin(wirc->d[0]); delwin(wirc->d[1]);
    delwin(wirc->t); free(wirc);

    delwin(wchip);

    int i;
    for (i = 0; i < NUM_PUBCARDS; ++i) {
        delwin(wcard[i]);
    }
    
    for (i = 0; i < MAX_PLAYER; ++i) {
        delwin(wplayer[i]->d[0]); 
        delwin(wplayer[i]->d[1]);
        delwin(wplayer[i]->t); 
        free(wplayer[i]);
    } 
}

void update_gui()
{
    int i;

    drawinfo();

    for (i = 0; i < MAX_PLAYER; ++i) {
        drawplayer(wplayer[i], i);
    }

    for (i = 0; i < NUM_PUBCARDS; ++i) {
        drawpubcard(wcard[i], i);
    }

    if (mode == 0) { 
        drawirc(wirc);
        drawchip(wchip);
    } else {
        drawchip(wchip);
        drawirc(wirc);
    }
}

