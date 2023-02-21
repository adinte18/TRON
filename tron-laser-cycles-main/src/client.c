#include "../include/common.h"

#define BLUE_ON_BLACK       0
#define RED_ON_BLACK        2
#define YELLOW_ON_BLACK     1
#define MAGENTA_ON_BLACK    3
#define CYAN_ON_BLACK       4

#define BLUE_ON_BLUE        50
#define RED_ON_RED          52
#define YELLOW_ON_YELLOW    51
#define MAGENTA_ON_MAGENTA  53
#define CYAN_ON_CYAN        54

void tune_terminal()
{
    struct termios term;
    tcgetattr(0, &term);
    term.c_iflag &= ~ICANON;
    term.c_lflag &= ~ICANON;
    term.c_cc[VMIN] = 0;
    term.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &term);
}

int connect_to_server(SAI addr, char * ip)
{
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&addr, '\0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERV_PORT);
    addr.sin_addr.s_addr = inet_addr(ip);
    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0 )
    {
        perror("connect error");
        exit(EXIT_FAILURE);
    }
    
    printw("Connected to server on port %d\n", SERV_PORT);

    return sockfd;
}

/// @brief Function to check and send input to server
/// @param client_socket Socket to send to
/// @param c_in Input structure
/// @param id Player ID
/// @param nb_players Number of players
void send_input(int client_socket, struct client_input c_in, int id, int nb_players)
{
    char c;

    read(STDIN_FILENO, &c, 1);
    if (nb_players == 1)
    {
        switch(c)
        {
            case 'i':
                c_in.input = UP;
                c_in.id = id;
                break;
            case 'j':
                c_in.input = LEFT;
                c_in.id = id;
                break;
            case 'k':
                c_in.input = DOWN;
                c_in.id = id;
                break;
            case 'l':
                c_in.input = RIGHT;
                c_in.id = id;
                break;
            case 'm':
                c_in.input = TRAIL_UP;
                c_in.id = id;
            default: 
                break;
        }
    }
    if (nb_players == 2) 
    {
        switch(c)
        {
            case 'i':
                c_in.input = UP;
                c_in.id = 0;
                break;
            case 'j':
                c_in.input = LEFT;
                c_in.id = 0;
                break;
            case 'k':
                c_in.input = DOWN;
                c_in.id = 0;
                break;
            case 'l':
                c_in.input = RIGHT;
                c_in.id = 0;
                break;
            case 'm':
                c_in.input = TRAIL_UP;
                c_in.id = 0;
                break;
            case 'z':
                c_in.input = UP;
                c_in.id = 1;
                break;
            case 'q':
                c_in.input = LEFT;
                c_in.id = 1;
                break;
            case 's':
                c_in.input = DOWN;
                c_in.id = 1;
                break;
            case 'd':
                c_in.input = RIGHT;
                c_in.id = 1;
                break;
            case 32:
                c_in.input = TRAIL_UP;
                c_in.id = 1;
                break;
            default: 
                break;
        }
    }


    send(client_socket, &c_in, sizeof(struct client_input), 0);
}

void init_graphics()
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(100);
    start_color();
    init_pair(BLUE_ON_BLACK, COLOR_BLUE, COLOR_BLACK);
    init_pair(RED_ON_BLACK, COLOR_RED, COLOR_BLACK);
    init_pair(YELLOW_ON_BLACK, COLOR_YELLOW, COLOR_BLACK);
    init_pair(MAGENTA_ON_BLACK, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(CYAN_ON_BLACK, COLOR_CYAN, COLOR_BLACK);

    init_pair(BLUE_ON_BLUE, COLOR_BLUE, COLOR_BLUE);
    init_pair(RED_ON_RED, COLOR_RED, COLOR_RED);
    init_pair(YELLOW_ON_YELLOW, COLOR_YELLOW, COLOR_YELLOW);
    init_pair(MAGENTA_ON_MAGENTA, COLOR_MAGENTA, COLOR_MAGENTA);
    init_pair(CYAN_ON_CYAN, COLOR_CYAN, COLOR_CYAN);

    init_pair(WALL, COLOR_WHITE, COLOR_WHITE);
}


void display_character(int color, int y, int x, char character) {
    attron(COLOR_PAIR(color));
    mvaddch(y, x, character);
    attroff(COLOR_PAIR(color));
}

/// @brief Function responsible for drawing the board
/// @param di Display info structure
void draw_window(display_info di)
{
    for (int i = 0; i < YMAX; i++) {
        for (int j = 0; j < XMAX; j++) {

            if (di.board[j][i] == WALL) 
            {
                display_character(CYAN_ON_CYAN, i, j, WALL);
            }

            //p1
            if (di.board[j][i] == 0)
            {
                display_character(BLUE_ON_BLACK, i, j, 'X');
            }

            //p2
            if (di.board[j][i] == 1)
            {
                display_character(RED_ON_BLACK, i, j, 'X');
            }

            //trail
            if (di.board[j][i] == 51)
            {
                display_character(RED_ON_RED, i, j, '#');
            }

            //trail
            if(di.board[j][i] == 50)
            {
                display_character(BLUE_ON_BLUE, i, j, '|');
            }
        }
    }
}

/// @brief Function to check winner
/// @param tab Display info structure
/// @return 
int check_winner(display_info tab)
{
    if (tab.winner == 0) return 0;
    
    if (tab.winner == 1) return 1;

    if (tab.winner == TIE) return TIE;

    if (tab.winner == 3) return 3;

    return -1;
}

int main(int argc, char **argv)
{
    tune_terminal();
    init_graphics();

    struct client_input c_to_serv;
    display_info di_cl;

    if (argc < 4)
    {
        printf("usage : ./client ip port n_joueurs");
        exit(EXIT_FAILURE);
    }

    int n_players = atoi(argv[3]);

    if(n_players > 2) 
    {
        printf("Maximum 2\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;
    struct client_init_infos cinf;
    char * ip = argv[1];

    //Bool variable to check if we have a winner, initialised at false (because we dont have a winner)
    bool has_winner = false;

    int client_socket = connect_to_server(addr, ip);

    //Store number of players in client init infos structure
    cinf.nb_players = n_players;

    //Send number of players
    send(client_socket, &cinf, sizeof(struct client_init_infos), 0);

    //Recieve ID from server
    recv(client_socket, &c_to_serv, sizeof(struct client_input), 0);

    //Store Id 
    int id = c_to_serv.id;

    //Select stuff
    struct timeval tv;
    int retval;
    fd_set readfds;
    int max_socket;

    while(!has_winner)
    {
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        clear();
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(client_socket, &readfds); 
                
        max_socket = max(STDIN_FILENO, client_socket);

        retval = select(max_socket + 1, &readfds, NULL, NULL, &tv);

        CHECK(retval != -1);

        if (retval)
        {
            //process input and send
            if (FD_ISSET(STDIN_FILENO, &readfds))
            {
                send_input(client_socket, c_to_serv, id, n_players);
            }

            //recieve coordinates to move to
            if (FD_ISSET(client_socket, &readfds))
            {
                //recieve stuff to draw
                recv(client_socket, &di_cl, sizeof(display_info), 0);
                
                //once recieved -> draw
                draw_window(di_cl);
            }
        }
        else printf("No data within 1 second\n");

        if (check_winner(di_cl) == 0)
        {
            mvaddstr(YMAX/2, XMAX/2, "Player 1 a gagne\n");
            refresh();
            break;
        }
        
        if (check_winner(di_cl) == 1)
        {
            mvaddstr(YMAX/2, XMAX/2, "Player 2 a gagne\n");
            refresh();
            break;
        }

        if (check_winner(di_cl) == TIE)
        {
            mvaddstr(YMAX/2, XMAX/2, "Draw\n");
            refresh();
            break;
        }

        if (check_winner(di_cl) == 3)
        {
            mvaddstr(YMAX/2, XMAX/2, "Server closed\n");
            refresh();
            break;
        }
        refresh();
    }
    close(client_socket);

    return EXIT_SUCCESS;
}
