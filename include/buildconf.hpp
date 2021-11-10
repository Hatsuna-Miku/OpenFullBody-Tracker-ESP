//build flags
//#define CYCTIMETEST

//programs
#define GETBMI
#define OUTPUT

//rules
#ifdef CYCTIMETEST
#undef OUTPUT
#endif
