/*
 * Created: 05/18/2010 10:17:10 AM
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
#include <time.h>
#include <string.h>

#include "constant.h"
#include "typedef.h"

#include "card.h"

static CARD card[TOTAL_NUM];
static int current_card = 0;

static const char *num[13] = {
    "2", "3", "4", "5", "6", "7", "8", 
    "9", "10", "J", "Q", "K", "A" };

/* handle cards */
void init_card()
{
    int i, j, k;

    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 13; ++j) {
            k = i*13+j;
            card[k].c = i;
            card[k].i = j;
        }
    }
    return;
}

void riffle_cards()
{
    int i, j;
    CARD t;

    srand(time(NULL));
    for (i = 0; i < TOTAL_NUM-1; ++i) {
        j = i+rand()%(TOTAL_NUM-i);
        t = card[i];
        card[i] = card[j];
        card[j] = t;
    }
    current_card = 0;
    return;
}

CARD get_card()
{
    return card[current_card++];
}

static int comp_CARD(const void *a, const void *b)
{
	return (*(CARD*)b).i - (*(CARD*)a).i;
}

static void isNumkind(int *r, CARD *x)
{
    int flush = -1;
    const int nbyte = (NUM_PUBCARDS+2)*sizeof(int);

	int i = 0, j = 0, k = 0, m = 0, n = 0;

    int r4[NUM_PUBCARDS+2]; r4[0] = 0; r4[6] = -1;

	int card_stat[NUM_CARDS+1][2];
    memset(card_stat, 0, 2*(NUM_CARDS+1)*sizeof(int));

	int num_stat[4][NUM_CARDS+1];
    memset(num_stat, 0, 4*(NUM_CARDS+1)*sizeof(int));

    int kind_stat[4][NUM_CARDS+2];
    memset(kind_stat, 0, 4*(NUM_CARDS+2)*sizeof(int));
	
	card_stat[0][0] = x[0].i;
	card_stat[0][1] = 1;
    n = ++kind_stat[x[0].c][0];
    kind_stat[x[0].c][n] = x[0].i;
	for	(i = 1; i < NUM_CARDS; i++) {
        if ((n = ++kind_stat[x[i].c][0]) > 4) {
            flush = x[i].c;
        }
        kind_stat[x[i].c][n] = x[i].i;

	  	if (x[i].i == x[i-1].i)	{
		  	++card_stat[j][1];
		} else {
		    k = 4-card_stat[j][1];
			m = ++num_stat[k][0];
			num_stat[k][m] = card_stat[j][0];
		  	card_stat[++j][0] = x[i].i;
            card_stat[j][1] = 1;
		}
	}

    k = 4-card_stat[j][1];
	m = ++num_stat[k][0];
	num_stat[k][m] = card_stat[j][0];

    if (flush >= 0) {
        if (kind_stat[flush][1] == 12) {
            n = ++kind_stat[flush][0];
            kind_stat[flush][n] = -1;
        }
        n = kind_stat[flush][0];
        for (i = 1; i+4 <= n; ++i) {
            if (kind_stat[flush][i]-kind_stat[flush][i+4] == 4) {
                r[0] = 1; r[1] = kind_stat[flush][i];
                r[2] = 0; return;            
            }
        } 
        r4[0] = 4; r4[1] = kind_stat[flush][1];
        r4[2] = kind_stat[flush][2];
        r4[3] = kind_stat[flush][3];
        r4[4] = kind_stat[flush][4];
        r4[5] = kind_stat[flush][5];
    }

	if (num_stat[0][0] > 0) {
	  	r[0] = 2; r[1] = num_stat[0][1];
		r[2] = num_stat[3][1];
		if (r[2] < num_stat[2][1]) r[2] = num_stat[2][1];
		if (r[2] < num_stat[1][1]) r[2] = num_stat[1][1];
		r[3] = 0; return;
	}

	if (num_stat[1][0] > 1 || 
	   (num_stat[1][0] > 0 && num_stat[2][0] > 0)) {
	  	r[0] = 3; r[1] = num_stat[1][1];
		r[2] = num_stat[2][1];
		if (r[2] < num_stat[1][2]) r[2] = num_stat[1][2];
		r[3] = 0; return;
	}

    if (r4[0] == 4) {
        memcpy(r, r4, nbyte);
        return;
    }

    if (card_stat[0][0] == 12) {
        card_stat[++j][0] = -1;
    }
    for (i = 0; i+4 <= j; ++i) {
        if (card_stat[i][0]-card_stat[i+4][0] == 4) {
            r[0] = 5; r[1] = card_stat[i][0];
            r[2] = 0; return;
        } 
    }

	if (num_stat[1][0] > 0) {
	  	r[0] = 6; r[1] = num_stat[1][1];
		r[2] = num_stat[3][1];
		r[3] = num_stat[3][2];
		r[4] = 0; return;
	}

	if (num_stat[2][0] > 1) {
	  	r[0] = 7; r[1] = num_stat[2][1];
		r[2] = num_stat[2][2];
		r[3] = num_stat[3][1];
		if (r[3] < num_stat[2][3]) r[3] = num_stat[2][3];
		r[4] = 0; return;
	}

	if (num_stat[2][0] > 0) {
	  	r[0] = 8; r[1] = num_stat[2][1];
		r[2] = num_stat[3][1];
		r[3] = num_stat[3][2];
		r[4] = num_stat[3][3];
		r[5] = 0; return;
	}

	r[0] = 9; r[1] = num_stat[3][1];
	r[2] = num_stat[3][2];
	r[3] = num_stat[3][3];
	r[4] = num_stat[3][4];
	r[5] = num_stat[3][5];
	return;
}

static void whatisit(int *r, CARD *x)
{
    CARD xx[NUM_CARDS];

    memcpy(xx, x, NUM_CARDS*sizeof(CARD));
	qsort(xx, NUM_CARDS, sizeof(CARD), comp_CARD);
 	isNumkind(r, xx);

	return;
}

int compare(CARD *x, CARD *y)
{
	int r1[NUM_PUBCARDS+2];
	int r2[NUM_PUBCARDS+2];

    int i; r1[0] = r2[0] = 0;
    for (i = 1; i < NUM_PUBCARDS+2; ++i) {
        r1[i] = r2[i] = -1;
    }

	whatisit(r1, x);
	whatisit(r2, y);

	if (r1[0] < r2[0])
		return 0;
	else if (r1[0] > r2[0])
		return 1;
	else{
		int i;
		for (i = 1; i < NUM_PUBCARDS+2; i++) {
			if ((r1[i] == -1 && r2[i] != -1)|| (r1[i] != -1 && r2[i] == -1)) {
				printf("how can this happen\n");
				exit(1);
			}
			if (r1[i] > r2[i])
				return 0;
			else if (r1[i] < r2[i])
				return 1;
			else if( r1[i] == -1 )
                return 2;
		}
	}

    return -1;
}

void get_w_str(char *k_w, char *n_w, CARD *x)
{
    int i;

    n_w[0] = '\0';
    char ct[MAX_LEN+1];
    memset(ct, '\0', MAX_LEN+1);

    int r[NUM_PUBCARDS+2]; 
    whatisit(r, x);

    switch (r[0]) {
        case 1:
            snprintf(k_w, MAX_LEN+1, "Straight Flush");

            int j = r[1];
            for (i = 0; i < 5; ++i,--j) {
                if (j == -1) {
                    strncat(n_w, "A ", MAX_LEN+1);
                } else {
                    snprintf(ct, MAX_LEN+1, "%s ", num[j]);
                    strncat(n_w, ct, MAX_LEN+1);
                }
            }
            break;
        case 2:
            snprintf(k_w, MAX_LEN+1, "Four of a Kind");

            for (i = 0; i < 4; ++i) {
                snprintf(ct, MAX_LEN+1, "%s ", num[r[1]]);
                strncat(n_w, ct, MAX_LEN+1);
            }
            snprintf(ct, MAX_LEN+1, "%s ", num[r[2]]);
            strncat(n_w, ct, MAX_LEN+1);
            break;
        case 3:
            snprintf(k_w, MAX_LEN+1, "Full House");

            for (i = 0; i < 3; ++i) {
                snprintf(ct, MAX_LEN+1, "%s ", num[r[1]]);
                strncat(n_w, ct, MAX_LEN+1);
            }

            for (i = 0; i < 2; ++i) {
                snprintf(ct, MAX_LEN+1, "%s ", num[r[2]]);
                strncat(n_w, ct, MAX_LEN+1);
            }
            break;
        case 4:
            snprintf(k_w, MAX_LEN+1, "Flush");

            for (i = 0; i < 5; ++i) {
                snprintf(ct, MAX_LEN+1, "%s ", num[r[i+1]]);
                strncat(n_w, ct, MAX_LEN+1);
            }
            break;
        case 5:
            snprintf(k_w, MAX_LEN+1, "Straight");

            j = r[1];
            for (i = 0; i < 5; ++i,--j) {
                if (j == -1) {
                    strncat(n_w, "A ", MAX_LEN+1);
                } else {
                    snprintf(ct, MAX_LEN+1, "%s ", num[j]);
                    strncat(n_w, ct, MAX_LEN+1);
                }
            }
            break;
        case 6:
            snprintf(k_w, MAX_LEN+1, "Three of a Kind");

            for (i = 0; i < 3; ++i) {
                snprintf(ct, MAX_LEN+1, "%s ", num[r[1]]);
                strncat(n_w, ct, MAX_LEN+1);
            }
            snprintf(ct, MAX_LEN+1, "%s ", num[r[2]]);
            strncat(n_w, ct, MAX_LEN+1);
            snprintf(ct, MAX_LEN+1, "%s ", num[r[3]]);
            strncat(n_w, ct, MAX_LEN+1);
            break;
        case 7:
            snprintf(k_w, MAX_LEN+1, "Two pairs");

            for (i = 0; i < 2; ++i) {
                snprintf(ct, MAX_LEN+1, "%s ", num[r[1]]);
                strncat(n_w, ct, MAX_LEN+1);
            }
            for (i = 0; i < 2; ++i) {
                snprintf(ct, MAX_LEN+1, "%s ", num[r[2]]);
                strncat(n_w, ct, MAX_LEN+1);
            }
            snprintf(ct, MAX_LEN+1, "%s ", num[r[3]]);
            strncat(n_w, ct, MAX_LEN+1);
            break;
        case 8:
            snprintf(k_w, MAX_LEN+1, "Pair");

            for (i = 0; i < 2; ++i) {
                snprintf(ct, MAX_LEN+1, "%s ", num[r[1]]);
                strncat(n_w, ct, MAX_LEN+1);
            }
            snprintf(ct, MAX_LEN+1, "%s ", num[r[2]]);
            strncat(n_w, ct, MAX_LEN+1);
            snprintf(ct, MAX_LEN+1, "%s ", num[r[3]]);
            strncat(n_w, ct, MAX_LEN+1);
            snprintf(ct, MAX_LEN+1, "%s ", num[r[4]]);
            strncat(n_w, ct, MAX_LEN+1);
            break;
        case 9:
            snprintf(k_w, MAX_LEN+1, "High Card");

            for (i = 0; i < 5; ++i) {
                snprintf(ct, MAX_LEN+1, "%s ", num[r[i+1]]);
                strncat(n_w, ct, MAX_LEN+1);
            }
            break;
        default:
            snprintf(k_w, MAX_LEN+1, "Unknown");
            snprintf(n_w, MAX_LEN+1, "00 00 00 00 00");
            break;
    }
}

