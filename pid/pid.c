/**
 * @file pid.c
 * @author liuhai
 * @brief PID控制算法实现
 * @version 0.1
 * @date 2023-12-16
 */



#include "pid.h"


void pid_struct_init(pid_t *pid, double Kp, double Ki, double Kd)
{
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;
    pid->I = 0;
    pid->err1 = 0;
    pid->err2 = 0;
}



/**
 * @brief 位置式PID模拟计算
 * @param pid 控制句柄
 * @param errval PID环节的输入值，该值来自预期输出与实际输出之间的差值
 * @param dt 距离上一次计算的间隔时间，即误差变化为errval所用的时间
 * @return double PID环节的输出值，该值作为输出控制值，实际系统输出为f(pid_out)
 * @author liuhai
 * @date 2023-12-16
 */
double pid_simulation(pid_t *pid, double errval, double dt)
{    
    double  P, I, D, out;
    double  Kp, Ki, Kd;
    double  lastI, lasterr;
    
    Kp = pid->Kp;
    Ki = pid->Ki;
    Kd = pid->Kd;
    lastI = pid->I;
    lasterr = pid->err1;
    
    P = Kp * errval;
    I = lastI + Ki * ((errval-lasterr) * dt);
    D = Kd * ((errval - lasterr) / dt);
    out = P + I + D;

    pid->I = I;
    pid->err2 = lasterr;
    pid->err1 = errval;
    return out;
}



/**
 * @brief 位置式PID离散计算
 * @param pid 控制句柄
 * @param errval PID环节的输入值，该值来自预期输出与实际输出之间的差值
 * @return double PID环节的输出值，该值作为输出控制值，实际系统输出为f(pid_out)
 * @author liuhai
 * @date 2023-12-16
 */
double pid_discrete(pid_t *pid, double errval)
{
    double      P, I, D, out;
    double      Kp, Ki, Kd;
    double      lastI, lasterr;
    
    Kp = pid->Kp;
    Ki = pid->Ki;
    Kd = pid->Kd;
    lastI = pid->I;
    lasterr = pid->err1;
    
    P = Kp * errval;
    I = lastI + Ki * errval;
    D = Kd * (errval - lasterr);
    out = P + I + D;

    pid->I = I;
    pid->err2 = lasterr;
    pid->err1 = errval;
    return out;
}



/**
 * @brief 增量式PID计算
 * @param pid 控制句柄
 * @param errval PID环节的输入值，该值来自预期输出与实际输出之间的差值
 * @return double PID环节的输出增量，PID输出值 = PID输出增量 + 上一次PID输出值
 * @author liuhai
 * @date 2023-12-28
 */
double pid_incremental(pid_t *pid, double errval)
{
    double  dP, dI, dD, dout;
    double  Kp, Ki, Kd;
    double  lastI, lasterr, lastlasterr;
    
    Kp = pid->Kp;
    Ki = pid->Ki;
    Kd = pid->Kd;
    lasterr = pid->err1;
    lastlasterr = pid->err2;
    
    dP = Kp * (errval - lasterr);
    dI = Ki * errval;
    dD = Kd * (errval - 2*lasterr + lastlasterr);
    dout = dP + dI + dD;
    
    pid->I += dI;
    pid->err2 = lasterr;
    pid->err1 = errval;
    return dout;
}
