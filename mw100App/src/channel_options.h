


static int volt_entry_number = 10;
struct volt_entry
{
  char tag[6];
  char unit[3];
  double range[2]; // difference range is the same
  int precision;
};
static struct volt_entry volt_info[10] = 
  { 
    {  "20mV", "mV", {-20.000, 20.000}, 3},
    {  "60mV", "mV", { -60.00,  60.00}, 2},
    { "200mV", "mV", {-200.00, 200.00}, 2},
    {    "2V",  "V", {-2.0000, 2.0000}, 4},
    {    "6V",  "V", { -6.000,  6.000}, 3},
    {   "20V",  "V", {-20.000, 20.000}, 3},
    {  "100V",  "V", {-100.00, 100.00}, 2},
    { "60mVH", "mV", {  0.000, 60.000}, 3},
    {    "1V",  "V", {-1.0000, 1.0000}, 4},
    {   "6VH",  "V", { 0.0000, 6.0000}, 4}
  };
    
static int di_entry_number = 2;
struct di_entry
{ 
  char tag[8];
};
static struct di_entry di_info[2] = 
  {
    { "LEVEL"   },
    { "CONTACT" }
  };


static int tc_entry_number = 19;
struct tc_entry
{ 
  char tag[16];
  char unit[2];
  double range[2];
  double diff_range;  // +/- this
  // precision is always 1
};
static struct tc_entry tc_info[19] = 
  {
    {         "R", "C", {   0.0, 1760.0}, 1760.0 },
    {         "S", "C", {   0.0, 1760.0}, 1760.0 },
    {         "B", "C", {   0.0, 1820.0}, 1820.0 },
    {         "K", "C", {-200.0, 1370.0}, 1570.0 },
    {         "E", "C", {-200.0,  800.0}, 1100.0 },
    {         "J", "C", {-200.0, 1100.0}, 1300.0 },
    {         "T", "C", {-200.0,  400.0},  600.0 },
    {         "N", "C", {   0.0, 1300.0}, 1300.0 },
    {         "W", "C", {   0.0, 2315.0}, 2315.0 },
    {         "L", "C", {-200.0,  900.0}, 1100.0 },
    {         "U", "C", {-200.0,  400.0},  600.0 },
    { "KPvsAu7Fe", "K", {   0.0,  300.0},  300.0 },
    {  "PLATINEL", "C", {   0.0, 1400.0}, 1400.0 },
    {   "PR40-20", "C", {   0.0, 1900.0}, 1900.0 },
    {    "NiNiMo", "C", {   0.0, 1310.0}, 1310.0 },
    {   "WRe3-25", "C", {   0.0, 2400.0}, 2400.0 },
    {    "WWRe26", "C", {   0.0, 2400.0}, 2400.0 },
    {       "N14", "C", {   0.0, 1300.0}, 1300.0 },
    {        "XK", "C", {-200.0,  600.0},  800.0 }
  };

static int rtd_entry_number = 37;
struct rtd_entry
{ 
  char tag[16];
  char unit[2];
  double range[2];
  double diff_range;  // +/- this
  int precision;
};
static struct rtd_entry rtd_info[37] = 
  {
    {     "Pt100-1", "C", { -200.0,  600.0},  800.0, 1 },
    {     "Pt100-2", "C", { -200.0,  250.0},  450.0, 1 },
    {    "JPt100-1", "C", { -200.0,  550.0},  750.0, 1 },
    {    "JPt100-2", "C", { -200.0,  250.0},  450.0, 1 },
    {    "Pt100-1H", "C", {-140.00, 150.00}, 290.00, 2 },
    {    "Pt100-2H", "C", {-140.00, 150.00}, 290.00, 2 },
    {   "JPt100-1H", "C", {-140.00, 150.00}, 290.00, 2 },
    {   "JPt100-2H", "C", {-140.00, 150.00}, 290.00, 2 },
    {   "Ni100SAMA", "C", { -200.0,  250.0},  450.0, 1 },
    {    "Ni100DIN", "C", {  -60.0,  180.0},  240.0, 1 },
    {       "Ni120", "C", {  -70.0,  200.0},  270.0, 1 },
    {        "Pt50", "C", { -200.0,  550.0},  750.0, 1 },
    {      "Cu10GE", "C", { -200.0,  300.0},  500.0, 1 },
    {      "Cu10LN", "C", { -200.0,  300.0},  500.0, 1 },
    {    "Cu10WEED", "C", { -200.0,  300.0},  500.0, 1 },
    {  "Cu10BAILEY", "C", { -200.0,  300.0},  500.0, 1 },
    {       "J263B", "K", {    0.0,  300.0},  300.0, 1 },
    {    "Cu10a392", "C", { -200.0,  300.0},  500.0, 1 },
    {    "Cu10a393", "C", { -200.0,  300.0},  500.0, 1 },
    {        "Cu25", "C", { -200.0,  300.0},  500.0, 1 },
    {        "Cu53", "C", {  -50.0,  150.0},  200.0, 1 },
    {       "Cu100", "C", {  -50.0,  150.0},  200.0, 1 },
    {        "Pt25", "C", { -200.0,  550.0},  750.0, 1 },
    {     "Cu10GEH", "C", { -200.0,  300.0},  500.0, 1 },
    {     "Cu10LNH", "C", { -200.0,  300.0},  500.0, 1 },
    {   "Cu10WEEDH", "C", { -200.0,  300.0},  500.0, 1 },
    { "Cu10BAILEYH", "C", { -200.0,  300.0},  500.0, 1 },
    {    "Pt100-1R", "C", { -200.0,  600.0},  800.0, 1 },
    {    "Pt100-2R", "C", { -200.0,  250.0},  450.0, 1 },
    {   "JPt100-1R", "C", { -200.0,  550.0},  750.0, 1 },
    {   "JPt100-2R", "C", { -200.0,  250.0},  450.0, 1 },
    {      "Pt100G", "C", { -200.0,  600.0},  800.0, 1 },
    {      "Cu100G", "C", { -200.0,  200.0},  400.0, 1 },
    {       "Cu50G", "C", { -200.0,  200.0},  400.0, 1 },
    {       "Cu10G", "C", { -200.0,  200.0},  400.0, 1 },
    {       "Pt500", "C", { -200.0,  600.0},  800.0, 1 },
    {      "Pt1000", "C", { -200.0,  600.0},  800.0, 1 }
  };


static int ohm_entry_number = 3;
struct ohm_entry
{ 
  char tag[10];
  // unit is always ohm
  double range; // always starts at 0.0
  // diff_range is +/- the range
  int precision;
};
static struct ohm_entry ohm_info[3] = 
  {
    {   "20ohm", 20.000, 3 },
    {  "200ohm", 200.00, 2 },
    { "2000ohm", 2000.0, 1 }
  };


static int str_entry_number = 3;
struct str_entry
{ 
  char tag[12];
  // unit is always uSTR
  double range; // +/- this value
  double diff_range; // +/- this value
  int precision;
};
static struct str_entry str_info[3] = 
  {
    {   "2000uSTR",  2000.0,   2000.0,  1 },
    {  "20000uSTR", 20000.0,  20000.0,  0 },
    { "200000uSTR", 20000.0, 200000.0, -1 }  // that middle value is right
  };


static int pulse_entry_number = 2;
struct pulse_entry
{ 
  char tag[8];
  // range is 0 to 30000
  // diff_range is +/- 30000
};
static struct pulse_entry pulse_info[2] = 
  {
    { "LEVEL"   },
    { "CONTACT" }
  };



enum { IN_TYPE_VOLT, IN_TYPE_DI, IN_TYPE_TC, IN_TYPE_RTD, IN_TYPE_OHM, 
       IN_TYPE_STR, IN_TYPE_PULSE, IN_TYPE_NUMBER };
static char *in_type_str[] = { "VOLT", "DI", "TC", "RTD", "OHM", "STR", 
                               "PULSE" };
static int in_type_len[] = 
  { volt_entry_number, di_entry_number, tc_entry_number, rtd_entry_number, 
    ohm_entry_number, str_entry_number, pulse_entry_number };


struct input_options
{
  int type;
  void *entry;
};
    

//enum { OUT_TYPE_AO, OUT_TYPE_PWM };
