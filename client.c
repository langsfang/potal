/*
 * Created: 05/17/2010 01:32:53 PM
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ncurses.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <poll.h>
#include <locale.h>

#include "constant.h"
#include "typedef.h"

#include "client.h"
#include "gui.h"
#include "parse.h"

#define ENTER        10
#define TAB          9
#define BACKSPACE    8

int mode, turn, limit, nwin;
int myid, pot, rtime;
int need_update = 0;
int allow_send_ready = 1;
int sock = -1;

struct WIN2 *wplayer[MAX_PLAYER];
PLE_BASE playerinfo[MAX_PLAYER];
int seat[MAX_PLAYER];
int win[MAX_PLAYER];
int timing[MAX_PLAYER];

struct WIN2 *wirc;
WINDOW *pirc;
char ircmsg[MAX_LEN+1];

WINDOW *wchip;
int chipin;
int gaming = 0;

WINDOW *wcard[NUM_PUBCARDS];
CARD pubcard[NUM_PUBCARDS];

static int connect_to_server(const char *ip, int port)
{
    int sockfd;
    struct sockaddr_in servaddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &servaddr.sin_addr) < 0)
        printf("inet_pton error\n");

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
        printf("connect %s:%d error\n", ip, port);
        return -1;
    }

    struct linger l;
    l.l_onoff = 1;
    l.l_linger = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_LINGER, (struct linger *)&l, sizeof(struct linger));

    return sockfd;
}

static void send_msg(char cmd)
{
    char send_msg[MAX_LEN+1];
    send_msg[0] = '\0';

    switch (cmd) {
        case 'r':
            snprintf(send_msg, MAX_LEN+1, "r:");
            break;
        case 'b':
            alarm(0);
            snprintf(send_msg, MAX_LEN+1, "b:%d", chipin);
            break;
        case 'm':
            snprintf(send_msg, MAX_LEN+1, "m:%s:%s\n", 
                    playerinfo[myid].name, ircmsg);
            ircmsg[0] = '\0';
            break;
    }

    send(sock, send_msg, MAX_LEN, 0);
}

static void sig_alrm(int signo)
{
    if (--rtime == 0) {
        if (timing[myid]) {
            if( gaming ){
                chipin = -1;
                send_msg('b');
                chipin = 0;
            }else{
                close(sock);
                clean_gui();
                endwin();
                exit(0);
            }
        }
        alarm(0);
    } else {
        alarm(TIMER);
        int i;
        for (i = 0; i < MAX_PLAYER; ++i) {
            drawplayer(wplayer[i], i);
        }
        drawchip(wchip);
    }
}

void init_vars()
{
    int i;

    mode = 0; turn = -1; nwin = 0;
    rtime = 0; chipin = 0; rtime = 10;

    for (i = 0; i < MAX_PLAYER; ++i) {
        seat[i] = -1; win[i] = 0; timing[i] = 0;
    }
    
    for (i = 0; i < NUM_PUBCARDS; ++i) {
        pubcard[i].c = 4;
        pubcard[i].i = 0;
    }

    memset(ircmsg, MAX_LEN+1, '\0');
}

static void init_network(char *host, int port, char *name)
{
    printf("connecting to %s...\n", host);

    if ((sock = connect_to_server(host, port)) <= 0) {
        exit(1);
    }

    if (send( sock, "Hey, pot", MAX_NAME, 0 )<0 || send(sock, name, MAX_NAME, 0) <0) {
        printf("auth failed\n");
        exit(1);
    }

    if (recv(sock, &myid, sizeof(int), MSG_WAITALL)<0) {
        printf("recv id failed\n");
        exit(1);
    }

    if (myid < 0) {
        myid = -myid-1;
        allow_send_ready = 0;
    } else {
        allow_send_ready = 1;
    }
}


static void sig_intr(int signo)
{
    if (signo == SIGINT) {
        clean_gui();
        endwin();
        exit(0);
    }
    return;
}

static void init_client()
{
    setlocale(LC_ALL, "");

    init_vars();
    init_gui();
   
    if (signal(SIGALRM, sig_alrm) == SIG_ERR)
        perror("signa error");

    if (signal(SIGINT, sig_intr) == SIG_ERR) {
        fprintf(stderr, "SIG_ERR\n");
    }

    char msg[MAX_LEN+1] = "m:Usage:-----\nTAB -\n   "
	    		  "Switch between    bet window and    chat-window\n"
                  "r/R -\n   "
                  "Ready for star-   ting game\n"
                  "f/F -\n   Fold\n"
                  "Ctrl+C -\n   "
                  "Leave the game\n-----\n";
    parse(msg);
}

static void keymod0()
{
    int c;

    if (c = getch()) {
        switch (c) {
            case 'f': case 'F':
                if (turn == myid) {
                    chipin = -1;
                    send_msg('b');
                    chipin = 0;
                }
                need_update = 0;
                break;
            case 'r': case 'R':
                if (allow_send_ready) {
                    send_msg('r');
                    allow_send_ready = 0;
                }
                break;
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                if (turn == myid) {
                    chipin = chipin*10+c-'0';
                    if( chipin > playerinfo[myid].money )
                        chipin /= 10;
                    drawchip(wchip);
                }

                need_update = 0;
                break;
            case BACKSPACE:
            case KEY_BACKSPACE:
                if (turn == myid) {
                   chipin /= 10;
                   drawchip(wchip);
                }

                need_update = 0;
                break;
            case ENTER:
                if (turn == myid) {
                    if (chipin < limit-playerinfo[myid].bet 
                            && chipin < playerinfo[myid].money)
                        break;
                    if (chipin > playerinfo[myid].money)
                        chipin = playerinfo[myid].money;
                    send_msg('b');
                }

                need_update = 0;
                break;
            case TAB:
                mode = 1-mode;
                drawchip(wchip);
                drawirc(wirc);
                need_update = 0;
                break;
        } 
    }
}

static void keymod1()
{
    int i, c;

    if (c = getch()) {
        if (c >= 20 && c <= 126) {
            i = strlen(ircmsg);
            if (i < MAX_LEN) {
                ircmsg[i] = c; 
                ircmsg[i+1] = '\0';
            }

            drawirc(wirc);
            need_update = 0;
            return;
        }
    }

    switch (c) {
        case BACKSPACE:
        case KEY_BACKSPACE:
            i = strlen(ircmsg);
            if (i != 0) ircmsg[i-1] = '\0';
            drawirc(wirc);
            need_update = 0;
            break;
        case ENTER:
            send_msg('m');
            need_update = 0;
            break;
        case TAB:
            mode = 1-mode;
            drawirc(wirc);
            drawchip(wchip);
            need_update = 0;
            break;
    } 
}

static void usage()
{
    fprintf(stderr, "usage: client -s server [-p port] -n username\n");
    fprintf(stderr, "If you configured with ncursesw, please make sure your locale is UTF-8\n");
    exit(1);
}

int main(int argc, char *argv[])
{
    char server[MAX_LEN+1];
    char name[MAX_NAME+1];    
    int port = DEFAULT_PORT;
   
    memset(name, 0, MAX_NAME+1);
    printf("\n== Welcome to Poker On Everwhere ==\n\n"
           "Yes, this is the first _POKER_ game that can be run on every platform of the world.\n"
           "So, what's your name, dude.\n");
    while( name[0] == 0 ){
        printf(">:");
        fgets(name, MAX_NAME, stdin);
        name[strlen(name)-1] = '\0';
    }

    while (1) {
        opterr = 0;
        int c = getopt(argc, argv, "s:n:p:");
        if (c == -1) break;
        switch (c) {
            case 's':
                strncpy(server, optarg, MAX_LEN+1);
                break;
            case 'p':
                port = atoi(optarg);
                break;
            default:
                usage();
                break;
        }
    }

    init_network(server, port, name);
    init_client();

    struct pollfd pfd[2];

    pfd[0].fd = STDIN_FILENO;
    pfd[0].events = POLLIN;
    pfd[1].fd = sock;
    pfd[1].events = POLLIN;

    char msg[MAX_LEN+1];
    while (1) {
        if (poll(pfd, 2, -1) < 0)  {
            if (errno == EINTR) continue;
            perror("poll error\n");
        }

        if (pfd[1].revents & POLLIN) {    
            if (recv(sock, msg, MAX_LEN, MSG_WAITALL) <= 2) {
                wprintw(pirc, "%s", "server maybe down\n");
                drawirc(wirc);
                alarm(0);
                pfd[1].fd = -1;
                continue;
            }
            parse(msg);
        }

        if (pfd[0].revents & POLLIN) {
            char *p = ttyname(0);
            if( strncmp(p, "/dev/pts/", 8) != 0 )
                _exit(0);
            switch (mode) {
                case 0:
                    keymod0();
                    break;
                case 1:
                    keymod1();
                    break;
            }
        }

        if (need_update) update_gui();
    }

    return EXIT_SUCCESS;
}

