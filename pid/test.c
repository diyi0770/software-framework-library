#include <stdio.h>
#include "pid.h"



int main()
{
    pid2_t   pid;
    long    i = 0;
    double  in = 100, out = 0;
    
    pid_init2(&pid, 0.7, 3, 0.0001);
    // pid_init(&pid, 0.7, 0, 0.2);
    
    do {
        out = pid_diff_incremental(&pid, in - out, 1);
        // out /= 2;
        printf("%.8lf,%ld\n", out, ++i);
    } while (! (-0.000001 < (in - out) && (in - out) < 0.000001));
    
    return 0;
}
