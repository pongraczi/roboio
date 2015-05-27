#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define  USE_COMMON
#include "common.h"

#if defined(WIN32) || defined(WINCE)
	#include <windows.h>
#elif defined(linux)
    #include <unistd.h>
    #include <sys/times.h>
#elif defined(BC30)
	#include <dos.h>
#elif defined(DJGPP)
    //#include <bios.h>
    #include <time.h>
#endif



/******************  Error Message Functions  ******************/
static int   ERR_Type;
static char  ERR_MsgBuf[512] = {'\0'};

RBAPI(int) roboio_GetErrCode(void) {
    return ERR_Type;
}

RBAPI(char*) roboio_GetErrMsg(void) {
    return &(ERR_MsgBuf[0]);
}

static FILE* ERR_outputDevice = stderr;

RBAPI(bool) err_SetLogFile(char* logfile) {
    err_CloseLogFile();

	if (logfile == NULL)
	{
		ERR_outputDevice = NULL;
		return true;
	}

	if ((ERR_outputDevice = fopen(logfile, "w")) != NULL) return true;

	ERR_outputDevice = stderr;
    return false;
}

RBAPI(void) err_CloseLogFile(void) {
    if ((ERR_outputDevice != stderr) && (ERR_outputDevice != NULL))
        fclose(ERR_outputDevice);

	ERR_outputDevice = stderr;
}

#ifdef _MANAGED
	#pragma managed(push, off)
#endif
_RBAPI_C(void) errmsg(char* fmt, ...) {
    va_list args;

    if (ERR_outputDevice == NULL) return;

	va_start(args, fmt);
	vfprintf(ERR_outputDevice, fmt, args);
	va_end(args);

    fflush(ERR_outputDevice);
}

_RBAPI_C(void) errmsg_exit(char* fmt, ...) {
    va_list args;

    if (ERR_outputDevice != NULL)
    {
	    va_start(args, fmt);
	    vfprintf(ERR_outputDevice, fmt, args);
	    va_end(args);

        fflush(ERR_outputDevice);
    }

	exit(2);
}

_RBAPI_C(void) err_SetMsg(int errtype, char* fmt, ...) {
     va_list args;

	 va_start(args, fmt);
	 vsprintf(ERR_MsgBuf, fmt, args);
	 va_end(args);

	 ERR_Type = errtype;
}
#ifdef _MANAGED
	#pragma managed(pop)
#endif
/*-------------- end of Error Message Functions ---------------*/



/******************  Common Timer Functions  *******************/
RBAPI(unsigned long) timer_nowtime(void) { //in ms
#if defined(WIN32) || defined(WINCE)
	return GetTickCount();
#elif defined(linux)
    static bool usetimer = false;
    static unsigned long long inittime;
    struct tms cputime;

    if (usetimer == false)
    {
        inittime  = (unsigned long long)times(&cputime);
        usetimer = true;
    }

    return (unsigned long)((times(&cputime) - inittime)*1000UL/sysconf(_SC_CLK_TCK));
#elif defined(BC30)
    static bool usetimer = false;
    static unsigned long far* timeraddr;
    static unsigned long inittime;
    
    if (usetimer == false)
    {
        timeraddr = (unsigned long far*)MK_FP(0x40,0x6c);
        inittime  = *timeraddr;
        usetimer = true;
    }
    
    return ((*timeraddr) - inittime) * 55UL;
#elif defined(DJGPP)
    static bool usetimer = false;
    static uclock_t inittime;
    
    if (usetimer == false)
    {
        //inittime  = biostime(0, 0);
        inittime = uclock();
        usetimer = true;
    }
    
    //return (biostime(0, 0) - inittime) * 55UL;
    return (unsigned long)((uclock() - inittime)*1000UL/UCLOCKS_PER_SEC);
#else
	errmsg_exit("ERROR: timer isn't supported due to unrecognized OS!\n");
	return 0L; //just for preventing compiler error:p
#endif
}


RBAPI(void) delay_ms(unsigned long delaytime) { //delay in ms
#ifdef BC30
    while (delaytime > 0xffffUL)
    {
        delay(0xffff);
        delaytime = delaytime - 0xffffUL;
    }

    if (delaytime > 0UL) delay((unsigned)delaytime);
#else
	delaytime = delaytime + timer_nowtime();
	while (timer_nowtime() < delaytime);
#endif
}


static unsigned long getclocks(void) {
    unsigned long nowclocks;
 
#if defined(WIN32) || defined(WINCE)
    _asm {
        rdtsc
        mov DWORD PTR nowclocks, eax
        //mov DWORD PTR nowclocks_msb, edx
    }
#elif defined(BC30)
    __emit__(0x0f, 0x31);
    __emit__(0x66); _asm mov WORD PTR nowclocks, ax;
    //__emit__(0x66); _asm mov WORD PTR nowclocks_msb, dx;
#elif defined(DJGPP) || defined(linux)
    __asm__ __volatile__ (
        ".byte 0x0f; .byte 0x31"
        : "=a" (nowclocks) //, "=d"(nowclocks_msb)
        : //no input registers
        : "edx");
#else
	errmsg_exit("ERROR: %s() isn't supported due to unrecognized OS!\n", __FUNCTION__);
#endif

    return nowclocks;
}

RBAPI(void) delay_us(unsigned long delaytime) { //delay in us
    unsigned long nowclocks;

    #define CLOCKS_PER_MICROSEC (999UL) //only for RoBoard (RB-100)
    nowclocks = getclocks();
    while ((getclocks() - nowclocks)/CLOCKS_PER_MICROSEC < delaytime);
}
/*------------------ end of Timer Functions -------------------*/



/******************  Common Memory Functions  ******************/
RBAPI(void*) mem_alloc(size_t size) {
	void* p;

	if ((p = malloc(size)) == NULL)
	   errmsg_exit("ERROR: no enough memory to allocate %u bytes!", size);

	return p;
}

RBAPI(void*) mem_realloc(void* pointer, size_t size) {
	void* p;

	if ((p = realloc(pointer, size)) == NULL)
	   errmsg_exit("ERROR: no enough memory to reallocate %u bytes!", size);

	return p;
}
/*------------------- end of Memory Functions -----------------*/

