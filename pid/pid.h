#ifndef __PID_H
#define __PID_H


#define     Ki2Ti(kp, ki)       ((kp)*1.0/(ki))
#define     Kd2Td(kp, kd)       ((kd)*1.0/(kp))
#define     Ti2Ki(kp, Ti)       ((kp)*1.0/(Ti))
#define     Td2Kd(kp, Td)       ((Td)*(kp))



typedef struct {
    double Kp;
    double Ki;
    double Kd;
    double I;           /* @brief 积分环节 */
    double err1;        /* @brief 上次的误差 */
    double err2;        /* @brief 上上次的误差 */
    double out;
} pid_t;



typedef struct {
    double Kp;
    double Ti;
    double Td;
    double err1;        /* @brief 上次的误差 */
    double err2;        /* @brief 上上次的误差 */
    double out;
} pid2_t;



void pid_init(pid_t *pid, double Kp, double Ki, double Kd);
double pid_standard_position(pid_t *pid, double errval, double dt);
double pid_standard_incremental(pid_t *pid, double errval, double dt);

void pid_init2(pid2_t *pid2, double Kp, double Ti, double Td);
double pid_diff_incremental(pid2_t *pid2, double errval, double dt);

#endif
