#include "gdbserver.h"

#include <stdio.h>
#include <string.h>
#include <linux/kvm.h>

void putDebugChar(char);	/* write a single character      */
char getDebugChar();	/* read and return a single char */
void exceptionHandler();	/* assign an exception handler   */
extern void kvm_debug_set_step(bool enable_step);

extern unsigned char *guest_physical_memory;

/************************************************************************/
/* BUFMAX defines the maximum number of characters in inbound/outbound buffers*/
/* at least NUMREGBYTES*2 are needed for register packets */
#define BUFMAX 400

int remote_debug;
/*  debug >  0 prints ill-formed commands in valid packets & checksum errors */

static const char hexchars[]="0123456789abcdef";

/* Number of registers.  */
#define NUMREGS	24

/* Number of bytes of registers.  */
#define NUMREGBYTES (NUMREGS * 8)

/*
 * these should not be static cuz they can be used outside this module
 */
long registers[NUMREGS];

#define STACKSIZE 10000
int remcomStack[STACKSIZE/sizeof(int)];
static int* stackPtr = &remcomStack[STACKSIZE/sizeof(int) - 1];

int hex(char ch)
{
	if ((ch >= 'a') && (ch <= 'f'))
		return (ch - 'a' + 10);
	if ((ch >= '0') && (ch <= '9'))
		return (ch - '0');
	if ((ch >= 'A') && (ch <= 'F'))
		return (ch - 'A' + 10);
	return (-1);
}

static char remcomInBuffer[BUFMAX];
static char remcomOutBuffer[BUFMAX];

/* scan for the sequence $<data>#<checksum>     */
char *getpacket()
{
	char *buffer = &remcomInBuffer[0];
	char checksum;
	char xmitcsum;
	int count;
	char ch;

	while(1) {
		/* wait around for the start character, ignore all other characters */
		while((ch = getDebugChar()) != '$');

		retry:
			checksum = 0;
			xmitcsum = -1;
			count = 0;

			/* now, read until a # or end of buffer is found */
			while(count < BUFMAX - 1)
			{
				ch = getDebugChar();
				if(ch == '$')
					goto retry;
				if(ch == '#')
					break;
				checksum = checksum + ch;
				buffer[count] = ch;
				count = count + 1;
			}
			buffer[count] = '\0';

			if(ch == '#')
			{
				ch = getDebugChar();
				xmitcsum = hex(ch) << 4;
				ch = getDebugChar();
				xmitcsum += hex(ch);

				if(checksum != xmitcsum)
				{
					if(remote_debug)
					{
						fprintf(stderr,
							 "bad checksum.  My count = 0x%x, sent=0x%x. buf=%s\n",
							 checksum, xmitcsum, buffer);
					}
					putDebugChar('-');	/* failed checksum */
				}
				else
				{
					putDebugChar('+');	/* successful transfer */

					/* if a sequence char is present, reply the sequence ID */
					if(buffer[2] == ':')
					{
						putDebugChar(buffer[0]);
						putDebugChar(buffer[1]);

						return &buffer[3];
					}

					return &buffer[0];
				}
			}
		}
}

/* send the packet in buffer.  */
void putpacket(char *buffer)
{
	unsigned char checksum;
	int count;
	char ch;

	/*  $<packet info>#<checksum>.  */
	do
	{
		putDebugChar('$');
		checksum = 0;
		count = 0;

		while(ch = buffer[count])
		{
			putDebugChar(ch);
			checksum += ch;
			count++;
		}

		putDebugChar('#');
		putDebugChar(hexchars[checksum >> 4]);
		putDebugChar(hexchars[checksum % 16]);

	}
	while(getDebugChar() != '+');
}

/* Address of a routine to RTE to if we get a memory fault.  */
static void(*volatile mem_fault_routine)() = NULL;

/* Indicate to caller of mem2hex or hex2mem that there has been an
	 error.  */
static volatile int mem_err = 0;

void set_mem_err(void)
{
	mem_err = 1;
}

/* convert the memory pointed to by mem into hex, placing result in buf */
/* return a pointer to the last char put in buf(null) */
/* If MAY_FAULT is non-zero, then we should set mem_err in response to
	 a fault; if zero treat a fault like any other fault in the stub.  */
char *mem2hex(char *mem, char *buf, int count)
{
	for(int i = 0; i < count; i++)
	{
		unsigned char ch = *mem++;
		*buf++ = hexchars[ch >> 4];
		*buf++ = hexchars[ch % 16];
	}
	*buf = '\0';

	return(buf);
}

/* convert the hex array pointed to by buf into binary to be placed in mem */
/* return a pointer to the character AFTER the last byte written */
char *hex2mem(char *buf, char *mem, int count)
{
	int i;
	char ch;

	for(i = 0; i < count; i++)
	{
		ch = hex(*buf++) << 4;
		ch = ch + hex(*buf++);
		*mem++ = ch;
	}
	return(mem);
}

/* this function takes the 386 exception vector and attempts to
	 translate this number into a unix compatible signal value */
int computeSignal(int exceptionVector)
{
	int sigval;
	switch(exceptionVector)
		{
		case 0:
			sigval = 8;
			break;			/* divide by zero */
		case 1:
			sigval = 5;
			break;			/* debug exception */
		case 3:
			sigval = 5;
			break;			/* breakpoint */
		case 4:
			sigval = 16;
			break;			/* into instruction (overflow) */
		case 5:
			sigval = 16;
			break;			/* bound instruction */
		case 6:
			sigval = 4;
			break;			/* Invalid opcode */
		case 7:
			sigval = 8;
			break;			/* coprocessor not available */
		case 8:
			sigval = 7;
			break;			/* double fault */
		case 9:
			sigval = 11;
			break;			/* coprocessor segment overrun */
		case 10:
			sigval = 11;
			break;			/* Invalid TSS */
		case 11:
			sigval = 11;
			break;			/* Segment not present */
		case 12:
			sigval = 11;
			break;			/* stack exception */
		case 13:
			sigval = 11;
			break;			/* general protection */
		case 14:
			sigval = 11;
			break;			/* page fault */
		case 16:
			sigval = 7;
			break;			/* coprocessor error */
		default:
			sigval = 7;		/* "software generated" */
		}
	return(sigval);
}

/**********************************************/
/* WHILE WE FIND NICE HEX CHARS, BUILD AN INT */
/* RETURN NUMBER OF CHARS PROCESSED           */
/**********************************************/
int hexToInt(char **ptr, int *intValue)
{
	int numChars = 0;
	int hexValue;

	*intValue = 0;

	while(**ptr)
	{
		hexValue = hex(**ptr);
		if(hexValue >= 0)
		{
			*intValue = (*intValue << 4) | hexValue;
			numChars++;
		}
		else
			break;

		(*ptr)++;
	}

	return(numChars);
}

/*
 * This function does all command procesing for interfacing to gdb.
 */
void handle_exception(int exceptionVector)
{
	int sigval, stepping;
	int addr, length;
	char *ptr;
	int newPC;

	/* reply to host that an exception has occurred */
	sigval = computeSignal(exceptionVector);

	ptr = remcomOutBuffer;

	*ptr++ = 'T';			/* notify gdb with signo, PC, FP and SP */
	*ptr++ = hexchars[sigval >> 4];
	*ptr++ = hexchars[sigval & 0xf];

	*ptr++ = hexchars[AMD64_RSP_REGNUM];
	*ptr++ = ':';
	ptr = mem2hex((char *)&registers[AMD64_RSP_REGNUM], ptr, 8);	/* SP */
	*ptr++ = ';';

	*ptr++ = hexchars[AMD64_RBP_REGNUM];
	*ptr++ = ':';
	ptr = mem2hex((char *)&registers[AMD64_RBP_REGNUM], ptr, 8); 	/* FP */
	*ptr++ = ';';

	*ptr++ = hexchars[AMD64_RIP_REGNUM >> 4];
	*ptr++ = hexchars[AMD64_RIP_REGNUM & 0xf];
	*ptr++ = ':';
	ptr = mem2hex((char *)&registers[AMD64_RIP_REGNUM], ptr, 8); 	/* PC */
	*ptr++ = ';';

	*ptr = '\0';

	putpacket(remcomOutBuffer);

	while(true)
	{
		remcomOutBuffer[0] = '\0';
		ptr = getpacket();

		switch(*ptr++)
		{
			case '?':
				remcomOutBuffer[0] = 'S';
				remcomOutBuffer[1] = hexchars[sigval >> 4];
				remcomOutBuffer[2] = hexchars[sigval % 16];
				remcomOutBuffer[3] = '\0';
				break;
			case 'd':
				remote_debug = !(remote_debug);	/* toggle debug flag */
				break;
			case 'g':		/* return the value of the CPU registers */
				mem2hex((char *) registers, remcomOutBuffer, NUMREGBYTES);
				break;
			case 'G':		/* set the value of the CPU registers - return OK */
				hex2mem(ptr,(char *) registers, NUMREGBYTES);
				strcpy(remcomOutBuffer, "OK");
				break;
			case 'p':		/* get the value of a single CPU register */
				{
					int regno;

					if(hexToInt(&ptr, &regno))
						if(regno >= 0 && regno < NUMREGS)
						{
							//hex2mem(ptr,(char *) &registers[regno], 8);
							mem2hex((char *)&registers[regno], remcomOutBuffer, 8);
							break;
						}

					strcpy(remcomOutBuffer, "E01");
					break;
				}
			case 'P':		/* set the value of a single CPU register - return OK */
				{
					int regno;

					if(hexToInt(&ptr, &regno) && *ptr++ == '=')
						if(regno >= 0 && regno < NUMREGS)
						{
							hex2mem(ptr,(char *) &registers[regno], 8);
							strcpy(remcomOutBuffer, "OK");
							break;
						}

					strcpy(remcomOutBuffer, "E01");
					break;
				}

				/* mAA..AA,LLLL  Read LLLL bytes at address AA..AA */
			case 'm':
				/* TRY TO READ %x,%x.  IF SUCCEED, SET PTR = 0 */
				//strcpy(remcomOutBuffer, "E01"); break;
				if(hexToInt(&ptr, &addr))
					if(*(ptr++) == ',')
						if(hexToInt(&ptr, &length))
						{
							ptr = 0;
							mem2hex(reinterpret_cast<char*>(guest_physical_memory + addr), remcomOutBuffer, length);
						}

				if(ptr)
					strcpy(remcomOutBuffer, "E01");

				break;

				/* MAA..AA,LLLL: Write LLLL bytes at address AA.AA return OK */
			case 'M':
				/* TRY TO READ '%x,%x:'.  IF SUCCEED, SET PTR = 0 */
				if(hexToInt(&ptr, &addr))
					if(*(ptr++) == ',')
						if(hexToInt(&ptr, &length))
							if(*(ptr++) == ':')
							{
								mem_err = 0;
								hex2mem(ptr, reinterpret_cast<char*>(guest_physical_memory + addr), length);
								strcpy(remcomOutBuffer, "OK");
								ptr = '\0';
							}
				if(ptr)
					strcpy(remcomOutBuffer, "E02");

				break;

				/* cAA..AA    Continue at address AA..AA(optional) */
				/* sAA..AA   Step one instruction from AA..AA(optional) */
			case 's':
				kvm_debug_set_step(true);
				return;
			case 'c':
				kvm_debug_set_step(false);
				return;	// return to kvm monitor
				/* try to read optional parameter, pc unchanged if no parm */
				/*if(hexToInt(&ptr, &addr))
					registers[AMD64_RIP_REGNUM] = addr;*/

				//newPC = registers[AMD64_RIP_REGNUM];

				/* clear the trace bit */
				//registers[AMD64_RSP_REGNUM] &= 0xfffffeff;

				/* set the trace bit if we're stepping
				if(stepping)
					registers[PS] |= 0x100;*/

				break;

				/* kill the program */
			case 'k':		/* do nothing */
		#if 0
				/* Huh? This doesn't look like "nothing".
					 m68k-stub.c and sparc-stub.c don't have it.  */
				BREAKPOINT();
		#endif
				break;
		}			/* switch */

		/* reply to the request */
		putpacket(remcomOutBuffer);
		//printf("\n");
	}
}

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>

int connected_client_fd;

int initialize_server_socket(const char * bind_addr, int port)
{
	int ret_sock = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in my_addr;
	memset(&my_addr, 0, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	inet_pton(AF_INET, bind_addr, &my_addr.sin_addr);

	if(bind(ret_sock, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1)
	{
		perror("Bind fallita");
		exit(1);
	}
	if(listen(ret_sock, 10) == -1)
	{
		perror("Listen fallita");
		exit(1);
	}

	return ret_sock;
}

void putDebugChar(char c)
{
	//printf("%c", c);
	fflush(stdout);

	if(write(connected_client_fd, &c, 1) < 0)
	{
		perror("Errore su putDebugChar: ");
		exit(1);
	}
}

char getDebugChar()
{
	char value;
	if(read(connected_client_fd, &value, 1) < 0)
	{
		perror("Errore su getDebugChar: ");
		exit(1);
	}

	//printf("\033[4m%c\033[0m", value);
	fflush(stdout);

	return value;
}

void flush_i_cache() {}

void gdbserver_set_register(amd64_regnum name, unsigned long value)
{
	registers[name] = value;
}

bool gdbserver_start(const char* ip_addr, unsigned short port)
{
	int server_socket = initialize_server_socket(ip_addr, port);
	if(server_socket < 0)
	{
		printf("gdbserver_start: creazione server_socket fallita\n");
		exit(1);
	}

	printf("gdbserver_start: server in ascolto su %s:%d\n", ip_addr, port);

	// dobbiamo aspettare che il client si connetta
	struct sockaddr_in cl_addr;
	int my_len = sizeof(cl_addr);
	connected_client_fd = accept(server_socket, (struct sockaddr*)&cl_addr, (socklen_t*)&my_len);
	printf("gdbserver_start: client connesso!\n");

	// va lanciato un breakpoint per notificare gdb
	printf("gdbserver_start: lancio eccezione iniziale\n");
	handle_exception(3);

	printf("gdbserver_start: termino\n");
}