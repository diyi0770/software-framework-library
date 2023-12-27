#include <stdio.h>
#include "pid.h"


int main()
{
    pid_t   pid;
    // double  err;
    double  in = 100, out = 0;
    
    pid_struct_init(&pid, 0.1, 0.01, 0.01);
    
    do {
        out = pid_discrete(&pid, in - out);
        printf("%.8lf\n", out);
    } while (! (-0.000001 < (in - out) && (in - out) < 0.000001));
    
    return 0;
}

