#include "stdio.h"
#include "stdlib.h"
#include "errors.h"


void print_error(int err)
{
    err = abs(err);
    char *msg;

    switch (err) {
        case E_CALIB_INVAL:
            msg = "invalid calibration data";
            break;
        case E_LASER_NOT_RDY:
            msg = "laser not ready";
            break;
        default:
            msg = "";
    }
    printf("error: %s\n", msg);
}