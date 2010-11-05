/*
 * Created: 05/19/2010 09:55:49 AM
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

#ifndef _TYPEDEF_H_
#define _TYPEDEF_H_

#include "constant.h"

typedef struct CARD {
    int c, i;
} CARD;

typedef struct PLE_BASE {
    int     id;
    char    name[MAX_NAME+1];
    int     money;
    int     bet;
    int     ready;
    int     fold;
    CARD    c1, c2;
} PLE_BASE;

typedef struct PLAYER {
    PLE_BASE    player;
    int         sock;
} PLAYER;

#endif

