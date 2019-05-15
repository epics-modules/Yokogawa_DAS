#ifndef drvGM10_h
#define drvGM10_h


#define MAX_SIGNAL   (999)
#define MAX_MATH     (200)
#define MAX_COMM     (500)
#define MAX_CONST    (200)
#define MAX_VARCONST (200)

//      0001-0999   A001-A200  C001-C500  K001-K100     W001-W100
enum { ADDR_SIGNAL, ADDR_MATH, ADDR_COMM, ADDR_CONST, ADDR_VARCONST };

enum { TRIG_INFO, TRIG_CHANNELS, TRIG_MISC, TRIG_STATUS };

enum { MODE_SETTINGS, MODE_COMPUTE, MODE_COMPUTE_CMD, MODE_RECORDING };

enum { MODULE_PRESENCE, MODULE_STRING };

typedef struct devqueue devqueue;


devqueue *gm10_connect( char *device);
int gm10_test_module( struct devqueue *dq, int module);
int gm10_test_signal( devqueue *dq, int channel);
int gm10_test_analog_signal( struct devqueue *dq, int channel);
int gm10_test_binary_signal( struct devqueue *dq, int channel);
int gm10_test_integer_signal( struct devqueue *dq, int channel);
int gm10_test_output_analog_signal( devqueue *dq, int channel);
int gm10_test_output_binary_signal( devqueue *dq, int channel);

IOSCANPVT gm10_address_io_handler( devqueue *dq, int type);
IOSCANPVT gm10_info_io_handler( devqueue *dq);
IOSCANPVT gm10_status_io_handler( devqueue *dq);
IOSCANPVT gm10_error_io_handler( devqueue *dq);
IOSCANPVT gm10_channel_io_handler( devqueue *dq);

int gm10_system_info( struct devqueue *dq, int which, char *info);
int gm10_module_info( struct devqueue *dq, int type, int module,
                       int *val, char *str);

int gm10_get_mode( devqueue *dq, int type, int *value);
int gm10_mode_set( devqueue *dq, dbCommon *precord, int type, int pval);
int gm10_trigger( struct devqueue *dq, dbCommon *precord, int type);

int gm10_channel_start(devqueue *dq, dbCommon *precord, int type,
                        int channel);
int gm10_analog_get( devqueue *dq, int type, int channel, double *value);
int gm10_analog_set( devqueue *dq, dbCommon *precord, int type, 
                       int channel, double value);
int gm10_integer_get( devqueue *dq, int channel, int *value);
int gm10_binary_get( devqueue *dq, int channel, int *value);
int gm10_binary_set( devqueue *dq, dbCommon *precord, int channel, int value);

int gm10_channel_get_egu( devqueue *dq, int type, int channel, char *egu);
int gm10_channel_get_expr( devqueue *dq, int channel, char *expr);
int gm10_get_channel_status( devqueue *dq, int type, int channel, int *value);
int gm10_get_data_status( devqueue *dq, int type, int channel, int *value);
int gm10_get_channel_mode( devqueue *dq, int channel, int *value);
int gm10_get_alarm_flag( devqueue *dq, int *value);
int gm10_get_alarm( devqueue *dq, int type, int channel, int sub_channel, 
                     int *value);
int gm10_get_status_byte( devqueue *dq, int channel, int *value);

int gm10_get_error( devqueue *dq, int channel, char *error);
int gm10_get_error_flag( devqueue *dq, int *flag);
int gm10_clear_error( devqueue *dq, dbCommon *precord);
int gm10_acknowledge_alarms( devqueue *dq, dbCommon *precord);


#endif
