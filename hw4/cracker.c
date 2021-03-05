#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>

#define VALID 1
#define INVALID -1

#define HAS_ANSWER 1
#define NO_ANSWER -1

#define ASCII_LBOUND 32
#define ASCII_UBOUND 126

#define MAX_STRLEN 500
#define N_THREAD 80

char *md5(char *initial_msg, size_t initial_len);
pthread_mutex_t lock;
char *goal;
int n, m, n_find = 0;

int verify(char *str, int cur_i)
{
    if (cur_i > strlen(goal))
        return INVALID;

    int valid = VALID;
    char *hashed_str = md5(str, strlen(str));
    
    for (int i = 0; i < cur_i; i++)
    {
        if (hashed_str[i] != goal[i])
        {
            valid = INVALID;
            break;
        }
    }
    
    return valid;
}

//Append a char to the string
void addChar(char *str, char c)
{
    char c_str[5] = {c, '\0'};
    strcat(str, c_str);
}

//Delete the last char of the string
void deleteChar(char *str)
{
    str[strlen(str) - 1] = '\0';
}

int findTreasure(char *original_str, int cur_i, char *ans[])
{
    //If the length of the string is too long, return 
    if (strlen(original_str) > MAX_STRLEN)
        return NO_ANSWER;

    //Found n treasure!!
    if (cur_i > n)
        return HAS_ANSWER;

    char *str = (char *)malloc(sizeof(char) * (MAX_STRLEN + 500));
    strncpy(str, original_str, strlen(original_str));

    //Add character and verify
    for (int c = ASCII_LBOUND; c <= ASCII_UBOUND; c++)
    {
        //Already get m treasures
        if (n_find > m)
            return NO_ANSWER;

        addChar(str, (char)c);

        //printf("%d: %d %d %s\n", pthread_self() , cur_i, verify(str, cur_i), str);
        if (verify(str, cur_i) == 1)
        {
            //Copy the found treasure into answer table
            strcpy(ans[cur_i - 1], str);
            
            int return_val = findTreasure(str, cur_i + 1, ans);

            free(str);
            return return_val;
        }

        deleteChar(str);
    }

    //If not found, append a to current str
    addChar(str, 'a');
    int return_val = findTreasure(str, cur_i, ans);

    free(str);
    return return_val;
}

void *thread_entry(void *str)
{
    str = (char *)str;

    //Construct answer table
    char *ans[n];
    for (int i = 0; i < n; i++)
        ans[i] = (char *)malloc(sizeof(char) * (MAX_STRLEN + 5));
    
    if (findTreasure(str, 1, ans) == HAS_ANSWER)
    {
        pthread_mutex_lock(&lock);
        n_find++;
        if(n_find <= m){
            for (int i = 0; i < n; i++)
                printf("%s\n", ans[i]);
            printf("===\n");
        }
        pthread_mutex_unlock(&lock);
    }
    
    for (int i = 0; i < n; i++)
        free(ans[i]);

    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    if(argc < 6){
        printf("usage: ./cracker $prefix $goal $n $m $outfile\n");
        return 0;
    }

    // Parse arguments
    char prefix[130];
    strcpy(prefix, argv[1]);

    goal = argv[2];
    n = atoi(argv[3]), m = atoi(argv[4]);

    char outfile[MAX_STRLEN];
    strcpy(outfile, argv[5]);
    freopen(outfile, "w", stdout); 

    //Init lock
    pthread_mutex_init(&lock, NULL);

    //Create threads
    pthread_t thread[N_THREAD];
    char *str[N_THREAD];
    
    char *ans[n];
    for (int i = 0; i < n; i++)
        ans[i] = (char *)malloc(sizeof(char) * (MAX_STRLEN + 5));

    for (int i = 0; i < N_THREAD; i++)
    {
        //Give each thread different starting prefix
        str[i] = (char *)malloc(sizeof(char) * (MAX_STRLEN + 5));
        strcpy(str[i], prefix);
        addChar(str[i], (char)ASCII_LBOUND + i + 1);
        pthread_create(&thread[i], NULL, &thread_entry, (void *)str[i]);
    }

    //Join threads
    for (int i = 0; i < N_THREAD; i++)
    {
        pthread_join(thread[i], NULL);
        free(str[i]);
    }

    //Free lock
    pthread_mutex_destroy(&lock);
}