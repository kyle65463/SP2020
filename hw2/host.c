#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define N_ROUND 10
#define MAX_PLAYER 13
#define HOST 0
#define PLAYER 1
#define NONE -1
#define MAX_STRLEN 128

#define READ 0
#define WRITE 1

char *id;
char *key;

void playerToString(int player_id[], int n_player, char msg1[], char msg2[])
{
    if (n_player == 8)
    {
        sprintf(msg1, "%d %d %d %d\n", player_id[0], player_id[1], player_id[2], player_id[3]);
        sprintf(msg2, "%d %d %d %d\n", player_id[4], player_id[5], player_id[6], player_id[7]);
    }
    else if (n_player == 4)
    {
        sprintf(msg1, "%d %d\n", player_id[0], player_id[1]);
        sprintf(msg2, "%d %d\n", player_id[2], player_id[3]);
    }
}

void forkChild(char *depth, char *player, int to_child[], int from_child[], int type)
{
    // Build pipes
    pipe(to_child);
    pipe(from_child);

    // Fork
    if (fork() == 0)
    {
        // Redirect stdin/out to fds
        dup2(to_child[READ], STDIN_FILENO);
        close(to_child[READ]);
        dup2(from_child[WRITE], STDOUT_FILENO);
        close(from_child[WRITE]);

        // Exec
        if (type == HOST)
            execl("./host", "host", id, key, depth, NULL);
        else if (type == PLAYER)
            execl("./player", "player", player, NULL);
    }
}

void forkChildren(int new_depth, int player_id[], FILE *read_from_child[], FILE *write_to_child[], int type)
{
    // Parse arguments
    char depth[MAX_STRLEN] = "";
    char player[2][MAX_STRLEN] = {"", ""};
    if (type == HOST)
        sprintf(depth, "%d", new_depth);
    else if (type == PLAYER)
        for (int i = 0; i < 2; i++)
            sprintf(player[i], "%d", player_id[i]);

    // Fork children
    int to_child[2][2], from_child[2][2];
    forkChild(depth, player[0], to_child[0], from_child[0], type);
    forkChild(depth, player[1], to_child[1], from_child[1], type);

    // Convert fd to FILE*
    for (int i = 0; i < 2; i++)
    {
        read_from_child[i] = fdopen(from_child[i][READ], "r");
        write_to_child[i] = fdopen(to_child[i][WRITE], "w");
        setbuf(write_to_child[i], NULL);
    }
}

FILE *fifo_id;
FILE *fifo_0;

void openFifos()
{
    char fifo_name[MAX_STRLEN];
    sprintf(fifo_name, "fifo_%s.tmp", id);
    fifo_id = fopen(fifo_name, "r");
    fifo_0 = fopen("fifo_0.tmp", "w");
}

typedef struct result
{
    int id, score;
} Player;

void initResult(Player result[], int player_id[])
{
    for (int i = 0; i < MAX_PLAYER; i++)
    {
        result[i].id = i;
        result[i].score = NONE;
    }
    for (int i = 0; i < 8; i++)
        result[player_id[i]].score = 0;
}

void getWinner(FILE *read_from_child[], int *winner, int *winner_bid)
{
    // Get two player and their bid
    int player[2], bid[2];
    for (int i = 0; i < 2; i++)
        fscanf(read_from_child[i], "%d%d", &player[i], &bid[i]);

    *winner = player[0], *winner_bid = bid[0];
    if (bid[1] > bid[0])
    {
        *winner = player[1];
        *winner_bid = bid[1];
    }
}

int cmp(const void *a, const void *b)
{
    return ((Player *)b)->score - ((Player *)a)->score;
}

void printResult(Player result[])
{
    // Sort result by score
    qsort(result, MAX_PLAYER, sizeof(Player), cmp);
    int rank = 1, real_rank = 1, last_score = NONE;

    // Print ranks
    fprintf(fifo_0, "%s\n", key);
    for (int i = 0; i < MAX_PLAYER; i++)
    {
        if (result[i].score != NONE)
        {
            if (result[i].score != last_score && last_score != NONE)
                real_rank = rank;
            fprintf(fifo_0, "%d %d\n", result[i].id, real_rank);
            last_score = result[i].score;
            rank++;
        }
    }
    fflush(fifo_0);
}

//================================================================

int main(int argc, char *argv[])
{
    // Error handling
    if (argc != 4)
    {
        fprintf(stderr, "usage: ./host [host_id] [key] [depth]\n");
        return 0;
    }

    // Parse arguments
    id = argv[1];
    key = argv[2];
    int depth = atoi(argv[3]);

    if (depth == 0)
    {
        // Root host
        openFifos();
        FILE *read_from_child[2], *write_to_child[2];
        forkChildren(1, NULL, read_from_child, write_to_child, HOST);

        while (1)
        {
            // Get player ids
            int player_id[8] = {};
            for (int i = 0; i < 8; i++)
                fscanf(fifo_id, "%d", &player_id[i]);

            if (player_id[0] == -1)
            {
                fprintf(write_to_child[0], "-1 -1 -1 -1\n");
                fprintf(write_to_child[1], "-1 -1 -1 -1\n");
                break;
            }

            // Pass down player ids
            char msg1[MAX_STRLEN] = "", msg2[MAX_STRLEN] = "";
            playerToString(player_id, 8, msg1, msg2);
            fprintf(write_to_child[0], "%s", msg1);
            fprintf(write_to_child[1], "%s", msg2);

            // Init result
            Player result[MAX_PLAYER];
            initResult(result, player_id);

            // Get result
            for (int i = 1; i <= N_ROUND; i++)
            {
                // Get the winner
                int winner, winner_bid;
                getWinner(read_from_child, &winner, &winner_bid);

                // Add the score of the winner
                result[winner].score++;
            }

            //Print result
            printResult(result);
        }
    }
    else if (depth == 1)
    {
        // Inter host
        FILE *read_from_child[2], *write_to_child[2];
        forkChildren(2, NULL, read_from_child, write_to_child, HOST);

        while (1)
        {
            // Get player ids
            int player_id[4];
            for (int i = 0; i < 4; i++)
                scanf("%d", &player_id[i]);

            if (player_id[0] == -1)
            {
                fprintf(write_to_child[0], "-1 -1\n");
                fprintf(write_to_child[1], "-1 -1\n");
                break;
            }

            // Pass down player ids
            char msg1[MAX_STRLEN] = "", msg2[MAX_STRLEN] = "";
            playerToString(player_id, 4, msg1, msg2);
            fprintf(write_to_child[0], "%s", msg1);
            fprintf(write_to_child[1], "%s", msg2);

            for (int i = 1; i <= N_ROUND; i++)
            {
                // Get the winner
                int winner, winner_bid;
                getWinner(read_from_child, &winner, &winner_bid);

                // Pass the winner to root host
                printf("%d %d\n", winner, winner_bid);
            }
            fflush(stdout);
        }
    }
    else
    {
        // Leaf host
        while (1)
        {
            // Get player ids
            int player_id[2];
            for (int i = 0; i < 2; i++)
                scanf("%d", &player_id[i]);

            if (player_id[0] == -1)
                break;

            // Fork players
            FILE *read_from_child[2], *write_to_child[2];
            forkChildren(NONE, player_id, read_from_child, write_to_child, PLAYER);

            for (int i = 1; i <= N_ROUND; i++)
            {
                // Get the winner
                int winner, winner_bid;
                getWinner(read_from_child, &winner, &winner_bid);

                // Pass the winner to inter host
                printf("%d %d\n", winner, winner_bid);
            }
            fflush(stdout);

            pid_t child;
            while (wait(&child) > 0) {}
        }
    }

    pid_t child;
    while (wait(&child) > 0) {}
    return 0;
}