/*
 * Created: 05/19/2010 10:16:14 AM
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

#ifndef _CARD_H_
#define _CARD_H_

#include "typedef.h"

void init_card();
void riffle_card();
CARD get_card();

int  compare(CARD *x, CARD *y);
void get_w_str(char *k_w, char *n_w, CARD *x);

#endif

