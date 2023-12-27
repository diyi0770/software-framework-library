#ifndef __PID_H
#define __PID_H


#define     Ki2Ti(kp, ki)       ((ki)*1.0/(kp))
#define     Kd2Td(kp, kd)       ((kd)*1.0/(kp))
#define     Ti2Ki(kp, Ti)       ((Ti)*(kp))
#define     Td2Kd(kp, Td)       ((Td)*(kp))



typedef struct {
    double Kp;
    double Ki;
    double Kd;
    double I;           /* @brief 积分环节 */
    double err1;        /* @brief 上次的误差 */
    double err2;        /* @brief 上上次的误差 */
} pid_t;



void pid_struct_init(pid_t *pid, double Kp, double Ki, double Kd);
double pid_simulation(pid_t *pid, double in, double dt);
double pid_discrete(pid_t *pid, double errval);
double pid_incremental(pid_t *pid, double errval);

#endif
