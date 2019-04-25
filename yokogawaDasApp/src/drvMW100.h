#ifndef drvMW100_h
#define drvMW100_h

#include "drvMW100_comm.h"

//      001-060     A001-A300  C001-C300  K01-K60 
enum { ADDR_SIGNAL, ADDR_MATH, ADDR_COMM, ADDR_CONST };

enum { TRIG_INFO, TRIG_INPUT, TRIG_OUTPUT, TRIG_STATUS };

enum { MODE_SETTINGS, MODE_MEASUREMENT, MODE_COMPUTE, MODE_OPERATING,
       MODE_COMPUTE_CMD };

enum { MODULE_PRESENCE, MODULE_STRING, MODULE_MODEL, MODULE_CODE, MODULE_SPEED,
       MODULE_NUMBER };

typedef struct devqueue devqueue;


devqueue *mw100_connect( char *device);
int mw100_test_module( struct devqueue *dq, int module);
int mw100_test_signal( devqueue *dq, int channel);
int mw100_test_analog_signal( struct devqueue *dq, int channel);
int mw100_test_binary_signal( struct devqueue *dq, int channel);
int mw100_test_integer_signal( struct devqueue *dq, int channel);
int mw100_test_output_analog_signal( devqueue *dq, int channel);
int mw100_test_output_binary_signal( devqueue *dq, int channel);

IOSCANPVT mw100_channel_io_handler( devqueue *dq, int type, int channel);
IOSCANPVT mw100_info_io_handler( devqueue *dq);
IOSCANPVT mw100_status_io_handler( devqueue *dq);
IOSCANPVT mw100_error_io_handler( devqueue *dq);
IOSCANPVT mw100_input_io_handler( devqueue *dq);

int mw100_system_info( struct devqueue *dq, int which, char *info);
int mw100_module_info( struct devqueue *dq, int type, int module,
                       int *val, char *str);

int mw100_get_mode( devqueue *dq, int type, int *value);
int mw100_mode_set( devqueue *dq, dbCommon *precord, int type, int pval);
int mw100_trigger( struct devqueue *dq, dbCommon *precord, int type);

int mw100_channel_start(devqueue *dq, dbCommon *precord, int type,
                        int channel);
int mw100_analog_get( devqueue *dq, int type, int channel, double *value);
int mw100_analog_set( devqueue *dq, dbCommon *precord, int type, 
                       int channel, double value);
int mw100_integer_get( devqueue *dq, int channel, int *value);
int mw100_binary_get( devqueue *dq, int channel, int *value);
int mw100_binary_set( devqueue *dq, dbCommon *precord, int channel, int value);

int mw100_channel_get_egu( devqueue *dq, int type, int channel, char *egu);
int mw100_channel_get_expr( devqueue *dq, int channel, char *expr);
int mw100_get_channel_status( devqueue *dq, int type, int channel, int *value);
int mw100_get_data_status( devqueue *dq, int type, int channel, int *value);
int mw100_get_channel_mode( devqueue *dq, int channel, int *value);
int mw100_get_alarm_flag( devqueue *dq, int *value);
int mw100_get_alarm( devqueue *dq, int type, int channel, int sub_channel, 
                     int *value);
int mw100_get_status_byte( devqueue *dq, int channel, int *value);

int mw100_get_error( devqueue *dq, int channel, char *error);
int mw100_get_error_flag( devqueue *dq, int *flag);
int mw100_clear_error( devqueue *dq, dbCommon *precord);
int mw100_acknowledge_alarms( devqueue *dq, dbCommon *precord);


#endif
