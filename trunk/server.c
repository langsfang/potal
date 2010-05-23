/*
 * Created: 05/17/2010 05:14:56 PM
 * fangwei@email.ustc.edu.cn
 * zhaoshicao@gmail.com
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
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <poll.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "logger.h"
#include "constant.h"
#include "typedef.h"
#include "card.h"

#define init_player( a ) do { \
    (a)->player.id = -1;\
    (a)->player.money = 1000;\
    (a)->player.bet = -1; \
    (a)->player.ready = 0;\
    (a)->player.c1.c = (a)->player.c2.c = 4;\
    (a)->player.c1.i = (a)->player.c2.i = 0;\
    (a)->player.fold = 1;\
    (a)->sock = -1;\
} while(0) 

/* globle var */
static PLAYER *players;
static int pot = 0;
static int player_no;
static int playing;
static struct pollfd *pfd;
static CARD public_cards[NUM_PUBCARDS];

static int win[MAX_PLAYER];
static int prz[MAX_PLAYER];
static int allin[MAX_PLAYER];
static int nallin;
static int low = -1;
static int dealer = 0;
static int current_high = -1;
static char msg[MAX_LEN+1];

static void *server_logger = NULL;

/* broadcast messages */
static void sendtoall( char *c, int ex ){
    int i;
    for( i=0; i<MAX_PLAYER; i++ ){
        if( players[i].sock != -1 && i != ex){
            send(players[i].sock, c, MAX_LEN, 0);
        }
    }
/*     printf("send to all msg is: %s\n", c); */
    return;
}

int set_listen_sock( int port ){

    int listenfd;
    struct sockaddr_in servaddr;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    int on = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(port);

    if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        FATAL_LOGGER( server_logger, "%s", "bind error");
        exit(0);
    }
    listen(listenfd, 50);

    int option = 1;
    int len = sizeof(int);
    int ret = setsockopt (listenfd, IPPROTO_TCP, TCP_NODELAY, &option, len);
    if( ret < 0 ){
        FATAL_LOGGER( server_logger, "%s", "setsockopt failed");
    }

    return listenfd;
}

static void server_init( int port ){

    LOGGER_INIT(server_logger, "texas_server.log");

    srand(time(NULL));

    int i;
    players = malloc(sizeof(PLAYER)*MAX_PLAYER);
    for( i=0; i<MAX_PLAYER; i++ ){
        init_player( players + i );
    }

    pfd = (struct pollfd *)malloc((MAX_PLAYER+1)*sizeof(struct pollfd));

    pfd[0].fd      = set_listen_sock(port);
    pfd[0].events  = POLLIN; 

    for( i=1; i<MAX_PLAYER+1; i++ ){
        pfd[i].fd = -1;
        pfd[i].events  = POLLIN; 
    }

    init_card();
    for( i=0; i<NUM_PUBCARDS; i++ ){
        public_cards[i].c = 4;
        public_cards[i].i = 0;
    }

    dealer = -1;
   
    DEBUG_LOGGER( server_logger, "%s", "init server done");

    return;
}

static void fill_player_info(){

    int len = strlen(msg);

    char m[MAX_LEN+1];
    int tag = 0;
    int i;
    for( i=0; i<MAX_PLAYER; i++ ){
       if( players[i].sock != -1 ){
           snprintf(m, MAX_LEN+1, "%d:%s:%d:%d:%d:%d:%d%d:%d%d,",
                   players[i].player.id, players[i].player.name,
                   players[i].player.money, players[i].player.bet,
                   players[i].player.ready, players[i].player.fold,
                   players[i].player.c1.c, players[i].player.c1.i,
                   players[i].player.c2.c, players[i].player.c2.i);
           strncat(msg, m, MAX_LEN);
           tag++;
       } 
    }
    if( tag != player_no )
        FATAL_LOGGER( server_logger, "%s", "player_no is not right");

    return;
}

static void fill_public_cards(){

    int i;
    char m[MAX_LEN+1];
    for( i=0; i<NUM_PUBCARDS; i++ ){
        snprintf(m, MAX_LEN+1, "%d%d,", public_cards[i].c, public_cards[i].i);
        strncat( msg, m, MAX_LEN );
    }
    return;
}

static void send_new_info(int newid){

    snprintf(msg, MAX_LEN+1, "n:%d,%d,%d,", newid, player_no, pot);
    fill_public_cards();
    fill_player_info();
    sendtoall(msg, -1);
    return;
}

static void send_globle_info(void){

    snprintf(msg, MAX_LEN+1, "g:%d,%d,%d,", player_no, pot, low);
    fill_public_cards();
    fill_player_info();
    sendtoall(msg, -1);
    return;
}

static void create_player( int connfd, int ging )
{
    char c[MAX_NAME+1] = {'\0'};

    if( recv( connfd, c, MAX_NAME,  MSG_WAITALL ) <= 0 ){
        goto err_hd;
    }
    if( strcmp(c, "Hey, pot" )!= 0 ){
        /* not the poker guy */
        goto err_hd;
    }

    /* recv name */
    if( recv( connfd, c, MAX_NAME,  MSG_WAITALL ) <= 0 ){
        goto err_hd;
    }

    /*  no ':' and ',' in user name */
    char *p = c;
    while( p && *p ){
        if( *p == ':' || *p == ',' ){
            goto err_hd;
        }
        p++;
    }

    /* confirmed */
    player_no++;

    int i, j;
    j = rand()%MAX_PLAYER;
    i = (j+1)<MAX_PLAYER ? j+1 : 0;
    for( ; i<MAX_PLAYER; i=(i+1)%MAX_PLAYER ){
        if( players[i].sock == -1 )
            break;
        if( i == j ){
            FATAL_LOGGER( server_logger, "%s", "table is full but i'm creating new player now!");
            exit(0);
        }
    }

    init_player( &players[i] );
    players[i].player.id = i;
    players[i].sock      = connfd;
    strncpy(players[i].player.name, c, MAX_NAME);

    pfd[i+1].fd = connfd;

    int send_id = (ging >= 0) ? -i-1 : i;
    send(connfd, &send_id, sizeof(int), 0);
    send_new_info(i);

    DEBUG_LOGGER( server_logger, "%s is conneted, pl_no %d, id is %d", c, player_no, i);
    snprintf(msg, MAX_LEN+1, "s:%s joined\n", c);
    sendtoall( msg, -1 );

    return;

err_hd:
    close(connfd);
    return;
}

static int delete_player(int id, int a){

	if( players[id].player.fold == 0 ){
		playing--;
	}

	if( id == a ){
		snprintf(msg, MAX_LEN+1, "b:-1");
		sendtoall(msg, id);
	}
	snprintf(msg, MAX_LEN+1, "s:%s left\n", players[id].player.name);
	sendtoall(msg, id);
	snprintf(msg, MAX_LEN+1, "e:%d", id);
	sendtoall(msg, id);

	init_player( &players[id] );
	player_no--;

	if( player_no == 0 )
		return -2;
	if( player_no == 1 )
		return -1;
	return 0;
}

/* most of the time, we are in this function */
static int server_poll( int a ){

    int tag = (a<0)?0:1;
    char c[MAX_NAME+1];

    do {
        if( poll( pfd, MAX_PLAYER+1, -1 ) < 0 ){
            printf("poll error\n");
        }

        if( pfd[0].revents & POLLIN ){
            /* new connection */
            int connfd = accept(pfd[0].fd, (struct sockaddr *)NULL, NULL);
            if( connfd < 0 ){
                break;
            }
            if( player_no == MAX_PLAYER ){
                /* table full */
                /* FIXME: we should tell this guy someting */
                close(connfd);
                break;
            }

            /* create new player */
            create_player( connfd, a );

        }else{
            int i;
            for( i=1; i<MAX_PLAYER+1; i++ ){
                if( pfd[i].revents & POLLIN ){
                    int id = i-1;
		    DEBUG_LOGGER( server_logger, "player %d take an action", id);
                    int n = recv( pfd[i].fd, msg, MAX_LEN, MSG_WAITALL);
                    if( n <= 0 ){
                        /* client closed? */
                        pfd[i].fd = -1;
			if( id == a )
			    tag = 0;

			switch (delete_player(id, a)){
		            case -2:
			        return -2;
			    case -1:
			 	return -1;
			    default:
				continue;
			}
                    }
		    DEBUG_LOGGER( server_logger, "received %s\n", msg);

                    /* handle msg */
                    switch (msg[0]){
                        case 'm':
                            /* irc */
                            sendtoall(msg, -1);
                            break;
                        case 'b':
                            /* bet */
                            if( id == a )
                                tag = 0;
                            else{
                                printf("shoudn't be here\n");
                            }
                            break;
                        case 'r':
                            /* ready for playing */
                            players[id].player.ready = 1;
                            if( players[id].player.money < 10 ){
                                /* this guy cannot afford big blind, so the only 
                                 * thing he can do is reconnecting server */
                                snprintf(msg, MAX_LEN+1, "s:%s's money is not enough. now he is watcher.\n",
                                        players[id].player.name);
                                sendtoall(msg, -1);
                                players[id].player.fold = 1;
                                break;
                            }
                            players[id].player.fold = 0;
                            playing++;
                            snprintf(msg, MAX_LEN+1, "r:%d", id);
                            sendtoall( msg, -1 );
                            if( dealer == -1 )
                                dealer = id;
                            break;     
                        default:
		            DEBUG_LOGGER( server_logger, "received unkown message");
			    break;
                    }
                }
            }
        }

    } while( tag );

    return 0;
}

static int next_player( int p){
    int i = (p+1)<MAX_PLAYER?p+1:0;
    for (; ; i = (i+1)%MAX_PLAYER) {
        if (players[i].player.fold == 0){
            return i;
        }
    }
}

/* bet, call, raise, allin, etc..., from dealer */
static int betting( void ){

    if( playing <= 1 ) return 0; /* this only happen when nallin != 0 */

    int tag = 0;
    int i;

    int sbd_allin = 0;
    int ori_pot = pot;
    int allin_player[MAX_PLAYER];
    for( i=0; i<MAX_PLAYER; i++ )
        allin_player[i] = -1;

    if( current_high == -1 ){
        i = dealer;
        tag = 1;
    }else{
        i = next_player(current_high);
	ori_pot = 0;
    }

    for (; i != current_high; i = (i+1)%MAX_PLAYER) {

        if (players[i].player.fold == 1)
            continue;

        snprintf(msg, MAX_LEN+1, "t:%d", i);
        sendtoall(msg, -1);

        int ret = server_poll(i); /* recv from player i */
        if( ret == -1 ){ /* only one left */
            nallin = 0;
            return -1; 
        }else if ( ret == -2 ){ /* no player left */
            nallin = 0;
            return -2;
        }
        sendtoall(msg, -1); /* bcast bet info */

        int b = atoi(msg+2);
        if( b < 0 ){ 
            if (players[i].player.fold == 0){
                /* fold */
                players[i].player.fold = 1;
                playing--;
                snprintf(msg, MAX_LEN+1, "s:%s fold\n", players[i].player.name);
                sendtoall(msg, -1);
		if( playing == 1 )
			return -1;
            }
        } else {
            players[i].player.money -= b;
            int *bet = &players[i].player.bet;
            *bet = ( *bet < 0 )? b : b + *bet; 

            if (players[i].player.money < 0) {
                printf("money < 0, shouldnt happen\n");
            }
            pot += b;

            if( (tag == 0 && *bet == low) || *bet > low ){
                tag = 1;
                low = *bet;
                current_high = i;
            }

            if( *bet < low || players[i].player.money == 0 ){
                /* this guy is all in! */
                snprintf(msg, MAX_LEN+1, "s:%s all in\n", players[i].player.name);
                sendtoall(msg, -1); 

                allin_player[sbd_allin++] = i;

                players[i].player.fold = 1;
                playing--;
            }

            if( b == 0 ){
                snprintf(msg, MAX_LEN+1, "s:%s check\n", players[i].player.name);
                sendtoall(msg, -1); 
            }else if(players[i].player.fold != 1){ /* we already said this guy was all in */
                snprintf(msg, MAX_LEN+1, "s:%s bet $%d\n", players[i].player.name, b);
                sendtoall(msg, -1); /* irc */
            }
        }
    } 

    if( sbd_allin != 0 ){
        int k;
        for( i=0; i<sbd_allin; i++ ){
            allin[nallin] = allin_player[i];
            prz[nallin] = ori_pot;
            int t1 = players[allin_player[i]].player.bet;
            int t2;
            for( k=0; k<MAX_PLAYER; k++ ){
                if((t2 = players[k].player.bet)<=0) continue;
                prz[nallin] += (t1>t2)?t2:t1;
            }
            nallin++;
        }
        return 0;
    }

    snprintf(msg, MAX_LEN+1, "s:everyone bet $%d\n", low);
    sendtoall(msg, -1);

    return 0;
}

static void pot_round( int a ){

    int i;
    switch (a){
        case 1: /* blind chips */   
            players[current_high].player.bet = 5;	
            players[current_high].player.money -= 5;	
            pot += 5;
            current_high = next_player(current_high);	
            players[current_high].player.bet = 10;	
            players[current_high].player.money -= 10;	
            pot += 10;
            low = 10;
            /* pocket card */
            for( i=0; i<MAX_PLAYER; i++ ){
                if( players[i].player.fold != 1 ){
                    players[i].player.c1 = get_card();
                    players[i].player.c2 = get_card();
                }
            }
            send_globle_info();
            return;
        case 2: /* flops */    
            public_cards[0] = get_card();
            public_cards[1] = get_card();
            public_cards[2] = get_card();
            break;
        case 3: /* turn */    
            public_cards[3] = get_card();
            break;
        case 4: /* river */    
            public_cards[4] = get_card();
            break;
    }

    for( i=0; i<MAX_PLAYER; i++ )
        players[i].player.bet = -1;
    low = -1;
    current_high = -1;

    send_globle_info();

    return;
}

static void prizegving(int nwin)
{

    if (nwin == 0) return;

    int i;
    for (i = 0; i < nwin; ++i) {
        players[win[i]].player.money += prz[i];
    }

}

static void init( void )
{
    int i;
    playing = 0;

    pot = 0;
    low = -1;

    riffle_cards();

    /* init players info */
    for( i=0; i<MAX_PLAYER; i++ ){
        players[i].player.ready = (players[i].player.money < 10) ? 3 : 0;
        players[i].player.bet = -1;
        if( players[i].sock != -1 ){
            players[i].player.fold = 1;
            players[i].player.c1.c =4;
            players[i].player.c1.i =0;
            players[i].player.c2.c =4;
            players[i].player.c2.i =0;
        }
    }

    /* init public cards */
    for( i=0; i<NUM_PUBCARDS; i++ ){
        public_cards[i].c = 4;
        public_cards[i].i = 0;
    }

    /* init table info */
    for( i=0; i<MAX_PLAYER; i++ ){
        prz[i] = -1;
        win[i] = -1;
        allin[i] = -1;
    }
    nallin = 0;

    return;
}

static void whoiswin()
{

    /* allin player's info are in allin array and prz array */

    int nwin = 0, i;
    int tmp_win[MAX_PLAYER];
    int tmp_prz[MAX_PLAYER];

    CARD x[NUM_CARDS], y[NUM_CARDS];

    for (i = 0; i < MAX_PLAYER; ++i) {
        tmp_win[i] = -1;
        tmp_prz[i] = prz[i];
    }

    for (i = 0; i < NUM_PUBCARDS; ++i) {
        x[i] = public_cards[i];
        y[i] = public_cards[i];
    }

    i = 0;
    while (i < MAX_PLAYER && players[i].player.fold == 1) {
        i += 1;
    }

    if (i == MAX_PLAYER) {
        nwin = 0;
    } else {
        nwin = 1; tmp_win[0] = i;
    }

    if ((playing + nallin) == 1){
        /* win by all players fold */
        if( nwin != 1 ){
            win[0] = allin[0];
        }else{
            win[0] = tmp_win[0];
        }
        prz[0] = pot;
        snprintf(msg, MAX_LEN+1, "w:-1,%d,%d,%d,", pot, win[0], prz[0]);
        sendtoall(msg, -1);
        prizegving(1);
        return;
    } 

    if( playing >= 2 ) { /* at least two players survived */
        for (i = tmp_win[0]+1; i < MAX_PLAYER; ++i) {
            if (players[i].player.fold == 1)
                continue;

            x[5] = players[tmp_win[0]].player.c1;
            x[6] = players[tmp_win[0]].player.c2;
            y[5] = players[i].player.c1;
            y[6] = players[i].player.c2;

            switch (compare(x,y)) {
                case 0:
                    break;
                case 1:
                    nwin = 1; tmp_win[0] = i;
                    break;
                case 2:
                    nwin += 1; tmp_win[nwin-1] = i;
                    break;
            } 
        }
    }

    for( i=0; i<nwin; i++ ){
        allin[nallin+i] = tmp_win[i];
        tmp_prz[i+nallin] = pot;
    }
    nallin += nwin;
    tmp_prz[nallin] = -1;

    /* now all the remaining players are in the allin array */
    /* sort allin */
    int tmp, j, k;
    for( i=0; i<nallin; i++ ){
        for( j=i+1; j<nallin; j++ ){
            x[5] = players[allin[i]].player.c1;
            x[6] = players[allin[i]].player.c2;
            y[5] = players[allin[j]].player.c1;
            y[6] = players[allin[j]].player.c2;
            switch (compare(x,y)) {
                case 0: /* x>y */
                    break;
                case 2: /* x==y */
                    if( tmp_prz[i] > tmp_prz[j] ){ /* swap */
                        tmp = allin[i];
                        allin[i] = allin[j];
                        allin[j] = tmp;
                        tmp = tmp_prz[i];
                        tmp_prz[i] = tmp_prz[j];
                        tmp_prz[j] = tmp;
                    }
                    break;
                case 1: /* x<y, swap */
                    tmp = allin[i];
                    allin[i] = allin[j];
                    allin[j] = tmp;
                    tmp = tmp_prz[i];
                    tmp_prz[i] = tmp_prz[j];
                    tmp_prz[j] = tmp;
                    break;
            }
        }
    }

    /* 
     * now the situation like this:
     * allin: biggest hand, biggest hand, bigger hand, small hand
     * prz  : small pot   , big pot     , ...        , ...      
     *
     */
    int n = 1;
    int tag = 0;
    for( i=0; i<nallin-1 && tmp_prz[i+1] > 0 && allin[i+1]>0; i++ ){
        x[5] = players[allin[i]].player.c1;
        x[6] = players[allin[i]].player.c2;
        y[5] = players[allin[i+1]].player.c1;
        y[6] = players[allin[i+1]].player.c2;
        if( compare(x, y) != 2 ){ /* minus player i+1 ... player nallin-1's money  */
            for( j=i+1; j<nallin && tmp_prz[j] > 0; j++ ){
                tmp_prz[j] -= tmp_prz[i];
                if( tmp_prz[j] <= 0 ){ /* this guy win nothing */
                    for( k=j; k<MAX_PLAYER-1; k++ ){
                        allin[k] = allin[k+1];
                        tmp_prz[k] = tmp_prz[k+1];
                    }
                    tmp_prz[MAX_PLAYER-1] = -1;
                    j--;
                }
            }
            int prize, rem;
            int m = n;
            for( j=i-n+1; j<i+1; j++ ){
                prize = tmp_prz[j]/m;
                rem = tmp_prz[j] - m*prize;
                for( k=j; k<i+1; k++ )
                    tmp_prz[k] -= prize*(m-1);
                tmp_prz[j] += rem;
                win[tag] = allin[j];
                prz[tag] = tmp_prz[j];
                tag++;
                m--;
            }
            n = 1;
        }else{
            n++;
        }
    }

    if( tmp_prz[i] > 0 ){
        win[tag] = allin[i];
        prz[tag] = tmp_prz[i];
        tag++;
    }
    nwin = tag;

    char m[MAX_LEN+1];
    snprintf(msg, MAX_LEN+1, "w:%d,%d,", nwin, pot);
    for( i=0; i<nwin; i++ ){
        snprintf(m, MAX_LEN+1, "%d,%d,", win[i], prz[i]);
        strncat(msg, m, MAX_LEN);
    }

    sendtoall(msg, -1);
    prizegving(nwin);

    return;
}

static int all_player_ready( void ){

    int i;
    for( i=0; i<MAX_PLAYER; i++ ){
        if( players[i].sock != -1 ){
            if (players[i].player.ready == 0)
                return 0;
        }
    }
    return 1;
}

static void usage()
{
    fprintf(stderr, "usage: server [-p port]\n");
    exit(1);
}

static void waitting_for_start(void)
{
    while( player_no < 2 || playing < 2 || all_player_ready() == 0 ){
        server_poll(-1);
    }

    int i;
    for (i = 0; i < MAX_PLAYER; ++i) {
        if (players[i].sock != -1) {
            players[i].player.ready = 
                (players[i].player.money < 10) ? 3 : 2;
        }
    }

    sendtoall( "r:-1", -1 ); /* ok, let's start */
}

static int main_game( void ){

    int i;
    for( i=1; i<5; i++ ){
        pot_round(i); 
        int ret = betting();
        if ( ret == -2 ){
            return -1; /* goto init */
        }else if ( ret == -1 ){
            break; /* goto result */
        }
        if( (playing + nallin) == 1 )
            break;  /* goto result */
    }
    return 0;
}

static void result( void ){

    whoiswin();
}
   
int main(int argc, char *argv[])
{
    int port = DEFAULT_PORT;

    while (1) {
        opterr = 0;
        int c = getopt(argc, argv, "p:h");
        if (c == -1) break;
        switch (c) {
            case 'p':
                port = atoi(optarg);
                break;
            case 'h':
            default:
                usage();
                break;
        }
    }

    server_init( port );

    while(1) {

        init();
        waitting_for_start();

        dealer = next_player(dealer);
        current_high = dealer;

        if( main_game() < 0 ){
            continue;
        }

        result();
    }

    return EXIT_SUCCESS;
}
