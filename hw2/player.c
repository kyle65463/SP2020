#include <stdio.h>
#include <stdlib.h>

#define N_ROUND 10

int bid_list[21] = {
    20, 18, 5, 21, 8, 7, 2, 19, 14, 13,
    9, 1, 6, 10, 16, 11, 4, 12, 15, 17, 3};

int getBid(int player_id, int round){
    return  bid_list[player_id + round - 2] * 100;
}

int main(int argc, char *argv[])
{
    // Error handling
    if (argc != 2)
    {
        fprintf(stderr, "usage: ./player [player_id]\n");
        return 0;
    }
    // Parse arguments
    int player_id = atoi(argv[1]);

    for (int i = 1; i <= N_ROUND; i++)
    {
        // Send the winner to leaf host
        int bid = getBid(player_id, i);
        printf("%d %d\n", player_id, bid);
    }
}