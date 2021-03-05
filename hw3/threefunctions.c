#include "threadutils.h"

int calBlackhole(int n)
{
    int a = n / 100, b = n / 10 % 10, c = n % 10, x, y, z;
    if (a <= b && b <= c)
        x = c, y = b, z = a;
    if (a <= c && b >= c)
        z = a, y = c, x = b;
    if (b <= a && a <= c)
        z = b, y = a, x = c;
    if (b <= c && c <= a)
        z = b, y = c, x = a;
    if (c <= a && a <= b)
        z = c, y = a, x = b;
    if (c <= b && b <= a)
        z = c, y = b, x = a;

    return (x * 100 + y * 10 + z) - (z * 100 + y * 10 + x);
}

void BinarySearch(int thread_id, int init, int maxiter)
{

    ThreadInit(thread_id, init, maxiter);

    static int l = 0, r = 100;

    for (Current->i = 0; Current->i < Current->N; ++Current->i)
    {
        sleep(1);

        int m = (l + r) / 2;
        printf("BinarySearch: %d\n", m);
        if (m == Current->x)
        {
            ThreadExit();
        }
        else if (Current->x > m)
            l = m + 1;
        else
            r = m - 1;

        ThreadYield();
    }
    ThreadExit()
}

void BlackholeNumber(int thread_id, int init, int maxiter)
{
    ThreadInit(thread_id, init, maxiter);

    for (Current->i = 0; Current->i < Current->N; ++Current->i)
    {
        sleep(1);

        Current->x = calBlackhole(Current->x);
        printf("BlackholeNumber: %d\n", Current->x);
        if (Current->x == 495)
        {
            ThreadExit();
        }

        ThreadYield();
    }
    ThreadExit()
}

void FibonacciSequence(int thread_id, int init, int maxiter)
{
    ThreadInit(thread_id, init, maxiter);

    static int last = 1, now = 1;

    for (Current->i = 0; Current->i < Current->N; ++Current->i)
    {
        sleep(1);

        printf("FibonacciSequence: %d\n", now);
        int temp = now;
        now += last;
        last = temp;

        ThreadYield();
    }
    ThreadExit()
}
