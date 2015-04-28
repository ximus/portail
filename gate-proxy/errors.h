#ifndef __ERRORS_H_
#define __ERRORS_H_

// 255 - x is a vague attempt to avoid conflicts with standard errno.h

#define E_CALIB_INVAL         (1)
#define E_LASER_NOT_RDY       (2)

void print_error(int);

#endif