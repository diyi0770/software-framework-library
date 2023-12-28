/**
 * @file pid.c
 * @author diyi12
 * @brief PID控制算法实现
 * @version 0.1
 * @date 2023-12-16
 */



#include "pid.h"



void pid_init(pid_t *pid, double Kp, double Ki, double Kd)
{
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;
    pid->I = 0;
    pid->err1 = 0;
    pid->err2 = 0;
    pid->out = 0;
}



void pid_init2(pid2_t *pid2, double Kp, double Ti, double Td)
{
    pid2->Kp = Kp;
    pid2->Ti = Ti;
    pid2->Td = Td;
    pid2->err1 = 0;
    pid2->err2 = 0;
    pid2->out = 0;
}



/**
 * @brief 位置式PID标准公式: u(t) = Kp*e(t) + Ki*e(t)*dt+I(t-dt) + Kd*{[e(t)-e(t-dt)]/dt}
 * @param pid 控制句柄
 * @param errval PID环节的输入值，该值来自预期输出与实际输出之间的差值
 * @param dt 距离上一次计算的间隔时间
 * @return double PID环节的输出值，该值作为输出控制值，实际系统输出为f(pid_out)
 * @author diyi12
 * @date 2023-12-16
 */
double pid_standard_position(pid_t *pid, double errval, double dt)
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
    I = Ki * (errval * dt) + lastI;
    D = Kd * ((errval - lasterr) / dt);
    out = P + I + D;
    
    pid->out = out;
    pid->I = I;
    pid->err2 = lasterr;
    pid->err1 = errval;
    return out;
}



/**
 * @brief 增量式PID标准公式：u(t)-u(t-dt) = Kp*[e(t)-e(t-dt)] + Ki*e(t)*dt + Kd*{[e(t)-2e(t-dt)+e(t-2dt)]/dt}
 * @param pid 控制句柄
 * @param errval
 * @return double
 * @author diyi12
 * @date 2023-12-28
 */
double pid_standard_incremental(pid_t *pid, double errval, double dt)
{
    double  dP, dI, dD, dout;
    double  Kp, Ki, Kd;
    double  lasterr, lastlasterr;
    
    Kp = pid->Kp;
    Ki = pid->Ki;
    Kd = pid->Kd;
    lasterr = pid->err1;
    lastlasterr = pid->err2;
    
    dP = Kp * (errval - lasterr);
    dI = Ki * errval * dt;
    dD = Kd * ((errval - 2*lasterr + lastlasterr) / dt);
    dout = dP + dI + dD;
    
    pid->out += dout;
    pid->I += dI;
    pid->err2 = lasterr;
    pid->err1 = errval;
    return pid->out;
}



/**
 * @brief 增量式PID差分形式：u(t)-u(t-dt) = a0*e(t) + a1*e(t-dt) + a2*e(t-2dt)
 * a0 = Kp*(1+dt/Ti+Td/dt)
 * a1 = -Kp*(1+2Td/dt)
 * a2 = Kp*Td/dt
 * Ti = Kp/Ki
 * Td = Kd/Kp
 * 
 * @param pid 
 * @param errval 
 * @return double 
 * @author diyi12
 * @date 2023-12-28
 */
double pid_diff_incremental(pid2_t *pid2, double errval, double dt)
{
    double  a0, a1, a2, dout;
    double  Kp, Ti, Td;
    double  lasterr, lastlasterr;
    
    Kp = pid2->Kp;
    Ti = pid2->Ti;
    Td = pid2->Td;
    lasterr = pid2->err1;
    lastlasterr = pid2->err2;
    
    a0 = Kp * (1 + dt/Ti + Td/dt);
    a1 = -Kp * (1 + 2*Td/dt);
    a2 = Kp * Td/dt;
    dout = a0*errval + a1*lasterr + a2*lastlasterr;
    
    pid2->out += dout;
    pid2->err2 = lasterr;
    pid2->err1 = errval;
    return pid2->out;
}
