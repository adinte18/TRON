#include "../include/common.h"

/// @brief Create server on a given Port
/// @param port Port
/// @return 
int create_server(int port)
{
    int server_sock;
    SAI address;

    //check pour la fonction socket
    CHECK((server_sock = socket(AF_INET, SOCK_STREAM, 0)));

    address.sin_family = AF_INET;
    // port
    address.sin_port = htons(port);
    // ip
    address.sin_addr.s_addr = htonl(INADDR_ANY);    

    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    //bind
    if (bind(server_sock, (SA*)&address, sizeof(address)) < 0)
    {
        perror("Bind error\n");
        exit(EXIT_FAILURE);
    }

    //listen
    if (listen(server_sock, 2) < 0)
    {
        perror("Listen error\n");
        exit(EXIT_FAILURE);
    }

    printf("Server created! Listening on port %d...\n", port);

    // si tout se passe bien , on renvoie le descripteur
    return server_sock;
}

/// @brief Accept new connection
/// @param server_sock Server socket
/// @return 
int accept_new_connection(int server_sock)
{   
    int client_sock;
    SAI client_addr;
    socklen_t client_addr_size = sizeof(struct sockaddr_in);

    client_sock = accept(server_sock, 
                                (SA*)&client_addr, 
                                &client_addr_size);

    CHECK(client_sock);
    
    printf("Client sock : %d\n", client_sock);

    return client_sock;
}

/// @brief This function helps initialize the board that should be used by the client
/// @return 
display_info board_init()
{
    display_info tab;
    for (int y = 0; y < YMAX; y++)
    {
        for (int x = 0; x < XMAX; x++)
        {   
            if (y == 0 || y == YMAX-1)
            {
                tab.board[x][y] = WALL;
            }

            else if (x == 0 || x == XMAX-1) {
                tab.board[x][y] = WALL;
            }
            else tab.board[x][y] = -1;
        }
    }
    return tab;
}

/// @brief Move the player P to last known position
/// @param p Player p
/// @param last_position Last known position
void move_to_last_pos(Player * p, int last_position)
{
    if (last_position == UP)
    {
        p->y--;
    }
    if (last_position == DOWN)
    {
        p->y++;
    }
    if (last_position == LEFT)
    {
        p->x--;
    }
    if (last_position == RIGHT)
    {
        p->x++;
    }
}

/// @brief Setting the player P to random positions
/// @param p Player p
void set_player_pos(Player * p)
{
    p->x = rand() % (XMAX + 1); 
    p->y = rand() % (YMAX + 1); 

    printf("X : %d\n", p->x);
    printf("Y : %d\n", p->y);
}

/// @brief Recieving from client, a structure with the number of players
/// @param sockfd Client socket
/// @param cinf_in Client init info structure
/// @return 
int recieve_nb_joueurs(int sockfd, struct client_init_infos cinf_in)
{
    recv(sockfd, &cinf_in, sizeof(struct client_init_infos), 0);
    return cinf_in.nb_players;
}

/// @brief Sending given id to given socket with necessary structure
/// @param sockfd Client socket 
/// @param id Id of the client
/// @param c_in Structure coresponding to input of the player
void send_id_to(int sockfd, int id, struct client_input c_in)
{
    c_in.id = id;
    send(sockfd, &c_in, sizeof(struct client_input), 0);
}

/// @brief Helps initialize the players at random coordinates
/// @param player1 Player 1 structure
/// @param player2 Player 2 structure
void player_init(Player * player1, Player * player2)
{
    player1->player_id = 0;
    player2->player_id = 1;

    player1->laser_on = false;
    player2->laser_on = false;

    set_player_pos(player1);
    set_player_pos(player2);
}

/// @brief Testing if there is any collision
/// @param tab Game board/winner structure
/// @param p1 Player 1 structure
/// @param p2 Player 2 structure
/// @param sock_p1 Client 1 socket
/// @param sock_p2 Client 2 socket
/// @return 
bool collision(display_info tab, Player * p1, Player * p2, int sock_p1, int sock_p2)
{
    if (tab.board[p1->x][p1->y] == 111 || tab.board[p1->x][p1->y] == 51 || tab.board[p1->x][p1->y] == 50)
    {
        tab.winner = p2->player_id;
        printf("Player 2 wins\n");
        printf("Winner value: %d\n", tab.winner);
        send(sock_p1, &tab, sizeof(display_info), 0);
        if (sock_p2 != 0) send(sock_p2, &tab, sizeof(display_info), 0);
        return true;;
    }

    if (tab.board[p2->x][p2->y] == 111 || tab.board[p2->x][p2->y] == 50 || tab.board[p2->x][p2->y] == 51)
    {
        tab.winner = p1->player_id;
        printf("Player 1 wins\n");
        printf("Winner value: %d\n", tab.winner);
        send(sock_p1, &tab, sizeof(display_info), 0);
        if (sock_p2 != 0) send(sock_p2, &tab, sizeof(display_info), 0);
        return true;
    }

    if ((tab.board[p1->x][p1->y] == p2->player_id) && (tab.board[p2->x][p2->y] == p1->player_id))
    {
        tab.winner = TIE;
        send(sock_p1, &tab, sizeof(display_info), 0);
        if (sock_p2 != 0) send(sock_p2, &tab, sizeof(display_info), 0);
        return true;
    }
    return false;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Usage : ./server port refresh_rate");
        exit(EXIT_FAILURE);
    }

    /*===================SERVER CONFIG===================*/
    int port = atoi(argv[1]);
    int refresh_rate = atoi(argv[2]);
    int server_socket = create_server(port);
    /*===================================================*/

    /*Allocate memory for 2 players.*/
    Player * p1 = malloc(sizeof(struct Player));
    Player * p2 = malloc(sizeof(struct Player));

    /*Socket init*/
    int player1_socket = 0;
    int player2_socket = 0;
    char stdin_buff[8];

    /*Struct init*/
    struct client_init_infos cinf_in;
    struct client_input c_in;
    display_info tab;

    /*Select stuff*/
    int max_socket;
    fd_set readfds;
    struct timeval tv;
    int retval;
    int test = refresh_rate * 1000;

    /*Variables to stock last known position of a player*/
    int last_position_p1 = 0;
    int last_position_p2 = 0;

    /*===================PLAYER 1 CONFIG===================*/
    player1_socket = accept_new_connection(server_socket);
    int nb_joueurs_client1 = recieve_nb_joueurs(player1_socket, cinf_in);
    /*====================================================*/

    /*====================PLAYER INIT====================*/
    player_init(p1, p2);
    /*===================================================*/

    /*Send id to first player*/
    send_id_to(player1_socket, p1->player_id, c_in);

    if (nb_joueurs_client1 == 1)
    {
        /*===================PLAYER 2 CONFIG===================*/
        player2_socket = accept_new_connection(server_socket);
        int nb_joueurs_client2 = recieve_nb_joueurs(player2_socket, cinf_in);
        send_id_to(player2_socket, p2->player_id, c_in);

        //If the second client sends 2, exit (You can't have more than 3 players)
        if (nb_joueurs_client2 == 2) exit(EXIT_FAILURE);
        /*====================================================*/

        tab = board_init();
        tab.winner = -1;

        tab.board[p1->x][p1->y] = 0;
        tab.board[p2->x][p2->y] = 1;

        //send board
        send(player1_socket, &tab, sizeof(display_info), 0);
        send(player2_socket, &tab, sizeof(display_info), 0);

        while(true)
        {
            tv.tv_usec = refresh_rate*1000;

            FD_ZERO(&readfds);
            FD_SET(STDIN_FILENO, &readfds);
            FD_SET(player1_socket, &readfds); 
            FD_SET(player2_socket, &readfds);

            //common.h => max des deux descripteurs avec la macro max(a,b)
            max_socket = max(player1_socket, player2_socket);

            retval = select(max_socket + 1, &readfds, NULL, NULL, &tv);
            CHECK(retval != -1);

            if (retval)
            {
                if (FD_ISSET(STDIN_FILENO, &readfds))
                {
                    CHECK(read(STDIN_FILENO, &stdin_buff, sizeof(stdin_buff)));
                    
                    if (strcmp(stdin_buff, "quit\n") == 0)
                    {
                        tab.winner = 3;
                        send(player1_socket, &tab, sizeof(display_info), 0);
                        send(player2_socket, &tab, sizeof(display_info), 0);
                        break;
                    }

                    if (strcmp(stdin_buff, "restart\n") == 0)
                    {
                        tab.board[p1->x][p1->y] = -1;
                        tab.board[p2->x][p2->y] = -1;

                        player_init(p1, p2);

                        //clearing buffer
                        memset(stdin_buff, 0, sizeof(stdin_buff));
                    }
                }

                if (FD_ISSET(player1_socket, &readfds))
                {
                    recv(player1_socket, &c_in, sizeof(struct client_input), 0);
                    /*If the input recieved is for the laser, check the value of p1->laser_on*/
                    if (c_in.input == 4) 
                    {
                        if (p1->laser_on == true) p1->laser_on = false;
                        else p1->laser_on = true;
                    }
                }

                if (FD_ISSET(player2_socket, &readfds))
                {
                    recv(player2_socket, &c_in, sizeof(struct client_input), 0);
                    /*If the input recieved is for the laser, check the value of p1->laser_on*/
                    if (c_in.input == 4) 
                    {
                        if (p2->laser_on == true) p2->laser_on = false;
                        else p2->laser_on = true;
                    }
                }
            }
            else 
            {
                /*Here we are checking the input. We want it to be different to the value 4(TRAIL_UP)
                If the input is different from 4, we want to stock the last input, to be later used in movement.*/
                if (c_in.input != 4)
                {
                    if (c_in.id == p1->player_id)
                    {
                        last_position_p1 = c_in.input; 
                    } else last_position_p2 = c_in.input;
                }
            }

            /*If no data to read, move constantly to this direction*/

            /*Check for laser*/
            if (p1->laser_on == true)
            {
                tab.board[p1->x][p1->y] = 50;
            } else tab.board[p1->x][p1->y] = -1;

            /*Check for laser*/
            if (p2->laser_on == true)
            {
                tab.board[p2->x][p2->y] = 51;
            } else tab.board[p2->x][p2->y] = -1;

            /*Move to last known position*/
            move_to_last_pos(p1, last_position_p1);
            move_to_last_pos(p2, last_position_p2);

            /*If we detect a collision, send winner to clients and break from the loop*/
            if(collision(tab, p1, p2, player1_socket, player2_socket)) break;

            /*If no collision is detected, continue playing game*/
            tab.board[p1->x][p1->y] = p1->player_id;
            tab.board[p2->x][p2->y] = p2->player_id;

            /*Send new coordinates to clients*/
            send(player1_socket, &tab, sizeof(display_info), 0);
            send(player2_socket, &tab, sizeof(display_info), 0);

            usleep(test);
        }
        /*If we broken out of the loop, close the sockets, game is over*/
        close(player1_socket);
        close(player2_socket);
    }

    if (nb_joueurs_client1 == 2)
    {
        display_info tab;
        tab = board_init();
        tab.winner = -1;

        tab.board[p1->x][p1->y] = 0;
        tab.board[p2->x][p2->y] = 1;

        //send board
        send(player1_socket, &tab, sizeof(display_info), 0);

        while(true)
        {
            FD_ZERO(&readfds);
            FD_SET(STDIN_FILENO, &readfds);
            FD_SET(player1_socket, &readfds); 

            max_socket = max(player1_socket, STDIN_FILENO);

            retval = select(max_socket + 1, &readfds, NULL, NULL, &tv);
            CHECK(retval != -1);

            if (retval)
            {
                //printf("Data avaliable now. \n");
                if (FD_ISSET(STDIN_FILENO, &readfds))
                {
                    CHECK(read(STDIN_FILENO, &stdin_buff, sizeof(stdin_buff)));
                    
                    if (strcmp(stdin_buff, "quit\n") == 0)
                    {
                        tab.winner = 3;
                        send(player1_socket, &tab, sizeof(display_info), 0);
                        break;
                    }

                    if (strcmp(stdin_buff, "restart\n") == 0)
                    {
                        tab.board[p1->x][p1->y] = -1;
                        tab.board[p2->x][p2->y] = -1;

                        player_init(p1, p2);

                        //clearing buffer
                        memset(stdin_buff, 0, sizeof(stdin_buff));
                    }
                }

                if (FD_ISSET(player1_socket, &readfds))
                {
                    recv(player1_socket, &c_in, sizeof(struct client_input), 0);
                    if (c_in.input == 4) 
                    {
                        if (c_in.id == 0)
                        {
                            if (p1->laser_on == true) p1->laser_on = false;
                            else p1->laser_on = true;
                        }

                        if (c_in.id == 1)
                        {
                            if (p2->laser_on == true) p2->laser_on = false;
                            else p2->laser_on = true;
                        }
                    }
                }
            }
            else 
            {
                if (c_in.input != 4)
                {
                    if (c_in.id == p1->player_id)
                    {
                        last_position_p1 = c_in.input; 
                    } else last_position_p2 = c_in.input;
                }
            }
            //if no data to read, move constantly to this direction
            if (p1->laser_on == true)
            {
                tab.board[p1->x][p1->y] = 50;
            } else tab.board[p1->x][p1->y] = -1;

            if (p2->laser_on == true)
            {
                tab.board[p2->x][p2->y] = 51;
            } else tab.board[p2->x][p2->y] = -1;

            move_to_last_pos(p1, last_position_p1);
            move_to_last_pos(p2, last_position_p2);

            if (collision(tab, p1, p2, player1_socket, 0)) break;

            tab.board[p1->x][p1->y] = p1->player_id;
            tab.board[p2->x][p2->y] = p2->player_id;

            send(player1_socket, &tab, sizeof(display_info), 0);

            usleep(test);
        }
        close(player1_socket);
    }

    return EXIT_SUCCESS;
}
