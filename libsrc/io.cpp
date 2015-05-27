#define __IO_LIB

#define  USE_COMMON
#include "common.h"
#include "io.h"


#if defined     USE_WINIO
    #include <winio.h>
#elif defined   USE_PCIDEBUG
	#include <windows.h>
    #include <pcifunc.h>
#elif defined   linux
    #include <sys/io.h>
#elif defined   DJGPP
    #include <pc.h>
#elif defined   BC30
    #include <conio.h>

    //386 instructions
    #define EMIT_DWORD(x) \
        __emit__((unsigned char)x);__emit__((unsigned char)(x>>8));\
        __emit__((unsigned char)(x>>16));__emit__((unsigned char)(x>>24))

    #define MOV_EAX(x) __emit__(0x66, 0xb8); EMIT_DWORD(x)

    #define MOV_VAR_EAX(x) __emit__(0x66); _asm mov WORD PTR x, ax
    #define MOV_VAR_EDX(x) __emit__(0x66); _asm mov WORD PTR x, dx

    #define MOV_EAX_VAR(x) __emit__(0x66); _asm mov ax, WORD PTR x

    #define OUT_EDX_EAX __emit__(0x66); _asm out dx, ax
    #define IN_EAX_EDX __emit__(0x66); _asm in ax, dx

    //#define RDTSC(x,y) __emit__(0x0f, 0x31); MOV_VAR_EAX(x); MOV_VAR_EDX(y)
#endif


static bool IO_inUse = false;
RBAPI(bool) io_InUse(void) {
    return IO_inUse;
}

#define IO_MAXSECTIONS  (128)
static bool IO_sections[IO_MAXSECTIONS] = {false};

static int IO_edIrqCnt = 0;
RBAPI(void) io_EnableIRQ(void) {
    if (IO_edIrqCnt > 0) IO_edIrqCnt--;
    if (IO_edIrqCnt == 0)
    {
        #if defined(RB_BC_DOS) || defined(RB_DJGPP)
            enable();
        #elif defined(RB_WATCOM)
            _enable();
        #elif defined(RB_LINUX)
            asm("sti");  // only work on Vortex86SX/DX/...
        #endif
    }
}

RBAPI(void) io_DisableIRQ(void) {
    if (IO_edIrqCnt == 0)
    {
        #if defined(RB_BC_DOS) || defined(RB_DJGPP)
            disable();
        #elif defined(RB_WATCOM)
            _disable();
        #elif defined(RB_LINUX)
            asm("cli");  // only work on Vortex86SX/DX/...
        #endif
    }
    IO_edIrqCnt++;
}

RBAPI(int) io_Init(void) {
    int i;
    
    if (IO_inUse == true)
    {
        for (i=0; i<IO_MAXSECTIONS; i++)
            if (IO_sections[i] == false)
            {
                IO_sections[i] = true;
                return i;
            }
        err_SetMsg(ERROR_IOSECTIONFULL, "no free I/O section is available");
        return -1;
    }

	#if defined		USE_WINIO
        if (!InitializeWinIo())
        {
			err_SetMsg(ERROR_IOINITFAIL, "I/O library fails to initialize");
            return -1;
        }
    #elif defined	USE_PCIDEBUG
        if (getdllstatus() != DLLSTATUS_NOERROR)
        {
            err_SetMsg(ERROR_IOINITFAIL, "I/O library fails to initialize");
            return -1;
        }
    #elif defined	linux
        if (iopl(3) != 0)
		{
            err_SetMsg(ERROR_IOINITFAIL, "I/O library fails to initialize");
            return -1;
        }
    #endif

	IO_sections[0] = true;
	IO_inUse = true;

	//if ((io_CpuID() != CPU_VORTEX86DX_1) && (io_CpuID() != CPU_VORTEX86DX_2))
        if ((io_CpuID() != CPU_VORTEX86DX_1) && (io_CpuID() != CPU_VORTEX86DX_2) && (io_CpuID() != CPU_VORTEX86DX_3) && (io_CpuID() != CPU_VORTEX86MX) && (io_CpuID() != CPU_VORTEX86MX_PLUS) && (io_CpuID() != CPU_VORTEX86EX))

	{
		io_Close(0);
		err_SetMsg(ERROR_CPUUNSUPPORTED, "unrecognized CPU");
        return -1;
	}

	return 0;
}

RBAPI(void) io_Close(int section) {
    int i;
    
    if ((IO_inUse == false) || (section < 0) || (section >= IO_MAXSECTIONS)) return;

    IO_sections[section] = false;
    for (i=0; i<IO_MAXSECTIONS; i++) if (IO_sections[i] == true) return;

	#ifdef USE_WINIO
        ShutdownWinIo();
    #endif

	IO_inUse = false;
}


//the following functions are for backward compatibility to RoBoIO library 1.0
static int IO_ioSection = -1;
RBAPI(bool) io_init(void) {
    if (IO_ioSection != -1) return true;

    if ((IO_ioSection = io_Init()) != -1) return true;

    err_SetMsg(ERROR_IOINITFAIL, "I/O library fails to initialize");
    return false;
}

RBAPI(void) io_close(void) {
    if (IO_ioSection == -1) return;

    io_Close(IO_ioSection);
    IO_ioSection = -1;
}


//outport: double word
RBAPI(void) io_outpdw(unsigned addr, unsigned long val) {
    #if   defined   USE_WINIO
	    SetPortVal(addr, val, 4);
    #elif defined	USE_PCIDEBUG
		_IoWriteLong((ULONG)addr, val);
    #elif defined	WINCE
        _asm {
            mov dx, WORD PTR addr
            mov eax, val
            out dx, eax
        }
    #elif defined	linux
        outl(val, addr);
    #elif defined   DJGPP
        outportl(addr, val);
    #elif defined   BC30
        _asm mov dx, WORD PTR addr
        MOV_EAX_VAR(val);
        OUT_EDX_EAX;
    #endif
}

//inport: double word
RBAPI(unsigned long) io_inpdw(unsigned addr) {
    #if   defined   USE_WINIO
        unsigned long val;

	    GetPortVal(addr, &val, 4);
	    return (val);
    #elif defined	USE_PCIDEBUG
		return _IoReadLong((ULONG)addr);
    #elif defined	WINCE
        unsigned long val = 0L;

        _asm {
            mov dx, WORD PTR addr
            in eax, dx
            mov val, eax
        }
        return val;
    #elif defined	linux
        return inl(addr);
	#elif defined   DJGPP
        return inportl(addr);
    #elif defined   BC30
        unsigned long retval;

        _asm mov dx, WORD PTR addr
        IN_EAX_EDX;
        MOV_VAR_EAX(retval);

        return retval;
    #endif
}


//outport: word
RBAPI(void) io_outpw(unsigned addr, unsigned val) {
    #if   defined   USE_WINIO
	    SetPortVal(addr, val, 2);
    #elif defined	USE_PCIDEBUG
		_IoWriteShort((ULONG)addr, val);
    #elif defined	WINCE
        _asm {
            mov dx, WORD PTR addr
            mov ax, WORD PTR val
            out dx, ax
        }
    #elif defined	linux
        outw(val, addr);
    #elif defined   DJGPP
        outportw(addr, val);
    #elif defined   BC30
        outpw(addr, val);
    #endif
}

//inport: word
RBAPI(unsigned) io_inpw(unsigned addr) {
    #if   defined   USE_WINIO
        unsigned long val;

	    GetPortVal(addr, &val, 2);
	    return ((unsigned)val);
    #elif defined	USE_PCIDEBUG
		return _IoReadShort((ULONG)addr);
    #elif defined	WINCE
        unsigned val = 0;

        _asm {
            mov dx, WORD PTR addr
            in ax, dx
            mov WORD PTR val, ax
        }
        return val;
    #elif defined	linux
        return inw(addr);
    #elif defined   DJGPP
        return inportw(addr);
    #elif defined   BC30
        return inpw(addr);
    #endif
}


//outport: byte
RBAPI(void) io_outpb(unsigned addr, unsigned char val) {
    #if   defined   USE_WINIO
	    SetPortVal(addr, val, 1);
    #elif defined	USE_PCIDEBUG
		_IoWriteChar((ULONG)addr, val);
    #elif defined	WINCE
        _asm {
            mov dx, WORD PTR addr
            mov al, val
            out dx, al
        }
    #elif defined	linux
        outb(val, addr);
    #elif defined   DJGPP
        outportb(addr, val);
    #elif defined   BC30
        outp(addr, val);
    #endif
}

//inport: byte
RBAPI(unsigned char) io_inpb(unsigned addr) {
    #if   defined   USE_WINIO
        unsigned long val;

	    GetPortVal(addr, &val, 1);
	    return ((unsigned char)val);
    #elif defined	USE_PCIDEBUG
		return _IoReadChar((ULONG)addr);
    #elif defined	WINCE
        unsigned char val = 0;

        _asm {
            mov dx, WORD PTR addr
            in al, dx
            mov val, al
        }
        return val;
    #elif defined	linux
        return inb(addr);
    #elif defined   DJGPP
        return inportb(addr);
    #elif defined   BC30
        return inp(addr);
    #endif
}

RBAPI(unsigned long) io_inpcidw(unsigned long addr) {
    #if   defined   USE_PCIDEBUG
        return _pciConfigReadLong((addr & 0x0fffffffL) >> 8, addr & 0xfcL);
    #elif defined   USE_PHYMEM
        unsigned long tmp;

        ReadPCI((addr>>16) & 0xffL,  // bus no.
                (addr>>11) & 0x1fL,  // dev no.
                (addr>>8)  & 0x07L,  // fun no.
                addr & 0xfcL,        // reg offset
                4, &tmp);

        return tmp;
    #else
        unsigned long tmp;

        io_DisableIRQ();
            io_outpdw(0x0cf8, addr & 0xfffffffcL);
        tmp = io_inpdw(0x0cfc);
        io_EnableIRQ();

        return tmp;
    #endif
}


_RB_INLINE unsigned long read_ide_reg(unsigned char idx) {
    return io_inpcidw(0x80006000L+(unsigned long)idx);
}

RBAPI(int) io_CpuID(void) {
        unsigned long id   = read_nb_reg(0x90);
        unsigned char nbrv, sbrv;
        unsigned long ide;

        switch (id)
        {
                case 0x31504D44L: //"DMP1"
                        return CPU_VORTEX86SX;
                case 0x32504D44L: //"DMP2"
                nbrv = read_nb_regb(0x08);
                sbrv = read_sb_regb(0x08);
                ide  = read_ide_reg(0x00);

                    if ((nbrv == 1) && (sbrv == 1) && (ide == 0x101017f3L)) return CPU_VORTEX86DX_1;  // Vortex86DX ver. A
                    if ((nbrv == 1) && (sbrv == 2) && (ide == 0x101117f3L)) return CPU_VORTEX86DX_2;  // Vortex86DX ver. C (PBA/PBB)
                    if ((nbrv == 1) && (sbrv == 2) && (ide == 0x24db8086L)) return CPU_VORTEX86DX_2;  // Vortex86DX ver. C (PBA/PBB) with standard IDE enabled
                    if ((nbrv == 2) && (sbrv == 2) && (ide == 0x101117f3L)) return CPU_VORTEX86DX_3;  // Vortex86DX ver. D
                    if ((nbrv == 2) && (sbrv == 2) && (ide == 0x24db8086L)) return CPU_VORTEX86DX_3;  // Vortex86DX ver. D with standard IDE enabled

            	    return CPU_VORTEX86DX_UNKNOWN;
                case 0x33504D44L: //"DMP3"
                        return CPU_VORTEX86MX;
                case 0x35504D44L: //"DMP5"
                        return CPU_VORTEX86MX_PLUS;
		case 0x37504D44L: // Vortex86EX
                        return CPU_VORTEX86EX;

        }

        return CPU_UNSUPPORTED;

//	unsigned long id = read_nb_reg(0x90);
//	unsigned char rv = read_nb_regb(0x08);

//	switch (id)
//	{
//		case 0x31504D44L: //"DMP1"
//			return CPU_VORTEX86SX;
//
//			//break;
//		case 0x32504D44L: //"DMP2"
//			if (rv == 1) return CPU_VORTEX86DX_1;
//			if (rv == 2) return CPU_VORTEX86DX_2;
//			break;
//	}
//
//	return CPU_UNSUPPORTED;
}

