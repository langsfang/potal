/*
 * Created: 05/20/2010 10:26:11 PM
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
#include <unistd.h>
#include <math.h>
#include <signal.h>
#include <string.h>

#include "constant.h"
#include "typedef.h"

#include "client.h"
#include "gui.h"
#include "parse.h"

static void unpack_playerinfo(char *msg, int unpack_id)
{
    char *token = msg;
    int count = 0;
    int id, relative_id;
    int c, i;

    while ((msg = strpbrk(token, ":")) != NULL)  {
        *msg++ = '\0';
        switch (count) {
            case 0:
                id = atoi(token);
                if (unpack_id != -1 && unpack_id != id) return;
                relative_id = (MAX_PLAYER+id-myid)%MAX_PLAYER;
                seat[relative_id] = id;
                playerinfo[id].id = id;
                break;
            case 1:
                strncpy(playerinfo[id].name, token, MAX_NAME+1);
                break;
            case 2:
                playerinfo[id].money = atoi(token);
                break;
            case 3: 
                playerinfo[id].bet = atoi(token);
                if (playerinfo[id].bet < 0)
                    playerinfo[id].bet = 0;
                break;
            case 4:
                playerinfo[id].ready = atoi(token);
                break;
            case 5:
                playerinfo[id].fold  = atoi(token);
                break;
            case 6:
                playerinfo[id].c1.c = token[0]-'0';
                playerinfo[id].c1.i = atoi(token+1);
                break;
        }

        token = msg;
        count += 1;
    }

    playerinfo[id].c2.c = token[0]-'0';
    playerinfo[id].c2.i = atoi(token+1);
}

static void parse_g(char *msg)
{
    int count = 0, nplayer = 0;
    char *token = msg;
    PLE_BASE *t;
    int i;

    while (count < 8 && (msg = strpbrk(token, ",")) != NULL)  {
        *msg++ = '\0';
        switch (count) {
            case 0:
                nplayer = atoi(token);
                break;
            case 1:
                pot = atoi(token);
                break;
            case 2:
                limit = atoi(token);
                if (limit < 0) limit = 0;
                break;
            case 3: case 4: case 5: case 6: case 7: 
                pubcard[count-3].c = token[0]-'0';
                pubcard[count-3].i = atoi(token+1);
                break;
        }

        token = msg;
        count += 1;
    }

    if (count < 8) {
        endwin();
        exit(1);
    }

    for (i = 0; i < MAX_PLAYER; ++i) {
        seat[i] = -1;
    }

    for (i = 0; i < nplayer; ++i) {
        if ((msg = strpbrk(token, ",")) == NULL) {
            endwin();
            fprintf(stderr, "received illegal message\n");
            exit(1);
        }
        
       *msg++ = '\0'; 
       unpack_playerinfo(token, -1); 
       token = msg;
    }

    chipin = limit-playerinfo[myid].bet;
    if (chipin < 0) chipin = 0;

    need_update = 1;
}

static void parse_n(char *msg)
{
    int count = 0, nplayer = 0, id;
    char *token = msg;
    PLE_BASE *t;
    int i;

    while (count < 8 && (msg = strpbrk(token, ",")) != NULL)  {
        *msg++ = '\0';
        switch (count) {
            case 0:
                id = atoi(token);
                break;
            case 1:
                nplayer = atoi(token);
                break;
            case 2:
                if (id == myid) {
                    pot = atoi(token);
                }
                break;
            case 3: case 4: case 5: case 6: case 7: 
                if (id == myid) {
                    pubcard[count-3].c = -1;
                    pubcard[count-3].i = -1;
                }
                break;
        }

        token = msg;
        count += 1;
    }

    if (count < 8) {
        endwin();
        exit(1);
    }

    if (id == myid) {
        for (i = 0; i < MAX_PLAYER; ++i) {
            seat[i] = -1;
        }

        for (i = 0; i < nplayer; ++i) {
            if ((msg = strpbrk(token, ",")) == NULL) {
                endwin();
                fprintf(stderr, "received illegal message\n");
                exit(1);
            }

            *msg++ = '\0'; 
            unpack_playerinfo(token, -1); 
            token = msg;
        }

        chipin = limit-playerinfo[myid].bet;
        if (chipin < 0) chipin = 0;
    } else {
        for (i = 0; i < nplayer; ++i) {
            if ((msg = strpbrk(token, ",")) == NULL) {
                endwin();
                fprintf(stderr, "received illegal message\n");
                exit(1);
            }

            *msg++ = '\0'; 
            unpack_playerinfo(token, id); 
            token = msg;
        }
    }

    need_update = 1;
}

static void parse_t(char *msg)
{
    turn = atoi(msg);
    rtime = 10;
    alarm(TIMER);

    need_update = 1;
}

static void parse_m(char *msg)
{
    char *token;

    token = strtok(msg, ":");
    _WIN_COLOR(pirc, 
            wprintw(pirc, "%s:\n", token); 
            , 3);

    token = strtok(NULL, "\0");
    wprintw(pirc, "%s", token); 

    if (mode == 0) {
        drawirc(wirc);
        drawchip(wchip);
    } else {
        drawchip(wchip);
        drawirc(wirc);
    }

    need_update = 0;
}

static void parse_s(char *msg)
{
    _WIN_COLOR(pirc, 
        wprintw(pirc, "%s", msg); 
        , 5);

    if (mode == 0) {
        drawirc(wirc);
        drawchip(wchip);
    } else {
        drawchip(wchip);
        drawirc(wirc);
    }

    need_update = 0;
}

static void parse_b(char *msg)
{
    int arg = atoi(msg);

    if (arg < 0) {
        playerinfo[turn].fold = 1;
        playerinfo[turn].bet = 0;
    } else {
        playerinfo[turn].bet += arg;
        playerinfo[turn].money -= arg;

        if (playerinfo[turn].bet > limit) {
            limit = playerinfo[turn].bet;
        }

        chipin = limit-playerinfo[myid].bet;
        if (chipin < 0) chipin = 0;
    }

    need_update = 1; 
}

static void parse_w(char *msg)
{
    int count = 0;
    int id, prz, index = 0;
    char *token = msg;
    char t_msg[MAX_LEN+1];

    while ((msg = strpbrk(token, ",")) != NULL) {
        *msg++ = '\0';

        switch (count) {
            case 0:
                nwin = atoi(token);
                break;
            case 1:
                pot = atoi(token);
                break;
            default:
                if (count%2 == 0) {
                    id = atoi(token);
                    win[id] = 1;
                } else {
                    prz = atoi(token);
                    playerinfo[id].money += prz;

                    snprintf(t_msg, MAX_LEN+1, "%s win %d\n",
                            playerinfo[id].name, prz);
                    parse_s(t_msg);

                    _WIN_COLOR(pirc,
                            wprintw(pirc, "winner is %s\n", playerinfo[id].name);
                            , 1);
                }
                break;
        }

        token = msg;
        count += 1;
    }

    int i;
    for (i = 0;i < MAX_PLAYER; ++i) {
        playerinfo[i].ready = (playerinfo[i].money < 10) ? 3 : 0;
        playerinfo[i].bet = 0;
    } 

    chipin = 0;
    turn = -1;

    allow_send_ready = 1;
    need_update = 1;
}

static void parse_r(char *msg)
{
    int id = atoi(msg);
    int i;

    if (id == -1) {
        init_vars();
    } else {
        playerinfo[id].ready = 1;
        _WIN_COLOR(pirc, 
                wprintw(pirc, "%s is ready\n", playerinfo[id].name); 
                , 6);
    }

    need_update = 1;
}

static void parse_e(char *msg)
{
    int id = atoi(msg);
    int relative_id = (MAX_PLAYER+id-myid)%MAX_PLAYER;
    seat[relative_id] = -1;

    need_update = 1;
}

void parse(char *msg)
{
    switch (msg[0]) {
        case 'g': parse_g(msg+2); break;
        case 't': parse_t(msg+2); break;
        case 'm': parse_m(msg+2); break;
        case 'b': parse_b(msg+2); break;
        case 'w': parse_w(msg+2); break;
        case 'r': parse_r(msg+2); break;
        case 's': parse_s(msg+2); break;
        case 'e': parse_e(msg+2); break;
        case 'n': parse_n(msg+2); break;
    }
}

