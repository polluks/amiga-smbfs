/*
 * :ts=4
 *
 * SMB file system wrapper for AmigaOS, using the AmiTCP V3 API
 *
 * Copyright (C) 2000-2019 by Olaf 'Olsen' Barthel <obarthel -at- gmx -dot- net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*****************************************************************************/

#define __USE_INLINE__
#define __NOGLOBALIFACE__
#define __NOLIBBASE__

/****************************************************************************/

#include <dos/dos.h>

#include <proto/exec.h>
#include <proto/dos.h>

#if defined(__amigaos4__)
#include <dos/obsolete.h>
#else
#define VARARGS68K
#endif /* __amigaos4__ */

#include <string.h>

/****************************************************************************/

#if defined(__SASC)
extern struct Library * AbsExecBase;
extern struct Library * DOSBase;

#define SysBase AbsExecBase

extern void kprintf(const char *,...);
extern void __stdargs kputc(char c);

#elif defined (__GNUC__) && defined(__amigaos4__)

extern struct Library * SysBase;
extern struct Library * DOSBase;

extern struct ExecIFace *	IExec;
extern struct DOSIFace *	IDOS;
extern struct SocketIFace *	ISocket;

extern void kprintf(const char *,...);
extern void kputc(char c);

#endif /* __GNUC__ */

/****************************************************************************/

#include <stdarg.h>

/****************************************************************************/

#define DEBUGLEVEL_OnlyAsserts	0
#define DEBUGLEVEL_Reports	1
#define DEBUGLEVEL_CallTracing	2

/****************************************************************************/

static BPTR debug_file = (BPTR)NULL;
static int indent_level = 0;
int __debug_level = DEBUGLEVEL_CallTracing;

static char program_name[40];
static int program_name_len = 0;

/****************************************************************************/

void
_SETDEBUGFILE(BPTR file)
{
	debug_file = file;
}

/****************************************************************************/

void
_SETPROGRAMNAME(char *name)
{
	if(name != NULL && name[0] != '\0')
	{
		program_name_len = strlen(name);
		if(program_name_len >= sizeof(program_name))
			program_name_len = sizeof(program_name)-1;

		strncpy(program_name,name,program_name_len);
		program_name[program_name_len] = '\0';
	}
	else
	{
		program_name_len = 0;
	}
}

/****************************************************************************/

int
_SETDEBUGLEVEL(int level)
{
	int old_level = __debug_level;

	__debug_level = level;

	return(old_level);
}

/****************************************************************************/

int
_GETDEBUGLEVEL(void)
{
	return(__debug_level);
}

/****************************************************************************/

static int previous_debug_level = -1;

void
_PUSHDEBUGLEVEL(int level)
{
	previous_debug_level = _SETDEBUGLEVEL(level);
}

void
_POPDEBUGLEVEL(void)
{
	if(previous_debug_level != -1)
	{
		_SETDEBUGLEVEL(previous_debug_level);

		previous_debug_level = -1;
	}
}

/****************************************************************************/

void
_INDENT(void)
{
	if(program_name_len > 0)
	{
		if(debug_file == (BPTR)NULL)
			kprintf("(%s) ",program_name);
		else
			FPrintf(debug_file,"(%s) ",program_name);
	}

	if(__debug_level >= DEBUGLEVEL_CallTracing)
	{
		int i;

		if(debug_file == (BPTR)NULL)
		{
			for(i = 0 ; i < indent_level ; i++)
				kprintf("   ");
		}
		else
		{
			for(i = 0 ; i < indent_level ; i++)
				FPrintf(debug_file,"   ");
		}
	}
}

/****************************************************************************/

void
_SHOWVALUE(
	unsigned long value,
	int size,
	const char *name,
	const char *file,
	int line)
{
	if(__debug_level >= DEBUGLEVEL_Reports)
	{
		char *fmt;

		switch(size)
		{
			case 1:

				fmt = "%s:%ld:%s = %ld, 0x%02lx";
				break;

			case 2:

				fmt = "%s:%ld:%s = %ld, 0x%04lx";
				break;

			default:

				fmt = "%s:%ld:%s = %ld, 0x%08lx";
				break;
		}

		_INDENT();

		if(debug_file == (BPTR)NULL)
			kprintf(fmt,file,line,name,value,value);
		else
			FPrintf(debug_file,fmt,file,line,name,value,value);

		if(size == 1 && value < 256)
		{
			if(debug_file == (BPTR)NULL)
			{
				if(value < ' ' || (value >= 127 && value < 160))
					kprintf(", '\\x%02lx'",value);
				else
					kprintf(", '%lc'",value);
			}
			else
			{
				if(value < ' ' || (value >= 127 && value < 160))
					FPrintf(debug_file,", '\\x%02lx'",value);
				else
					FPrintf(debug_file,", '%lc'",value);
			}
		}

		if(debug_file == (BPTR)NULL)
		{
			kprintf("\n");
		}
		else
		{
			FPrintf(debug_file,"\n");
			Flush(debug_file);
		}
	}
}

/****************************************************************************/

void
_SHOWPOINTER(
	void *pointer,
	const char *name,
	const char *file,
	int line)
{
	if(__debug_level >= DEBUGLEVEL_Reports)
	{
		char *fmt;

		_INDENT();

		if(pointer != NULL)
			fmt = "%s:%ld:%s = 0x%08lx\n";
		else
			fmt = "%s:%ld:%s = NULL\n";

		if(debug_file == (BPTR)NULL)
		{
			kprintf(fmt,file,line,name,pointer);
		}
		else
		{
			FPrintf(debug_file,fmt,file,line,name,pointer);
			Flush(debug_file);
		}
	}
}

/****************************************************************************/

void
_SHOWSTRING(
	const char *string,
	const char *name,
	const char *file,
	int line)
{
	if(__debug_level >= DEBUGLEVEL_Reports)
	{
		_INDENT();

		if(debug_file == (BPTR)NULL)
		{
			if(string != NULL)
				kprintf("%s:%ld:%s = 0x%08lx \"%s\"\n",file,line,name,string,string);
			else
				kprintf("%s:%ld:%s = NULL \"\"\n",file,line,name);
		}
		else
		{
			if(string != NULL)
				FPrintf(debug_file,"%s:%ld:%s = 0x%08lx \"%s\"\n",file,line,name,string,string);
			else
				FPrintf(debug_file,"%s:%ld:%s = NULL \"\"\n",file,line,name);

			Flush(debug_file);
		}
	}
}

/****************************************************************************/

void
_SHOWMSG(
	const char *string,
	const char *file,
	int line)
{
	if(__debug_level >= DEBUGLEVEL_Reports)
	{
		_INDENT();

		if(debug_file == (BPTR)NULL)
		{
			kprintf("%s:%ld:%s\n",file,line,string);
		}
		else
		{
			FPrintf(debug_file,"%s:%ld:%s\n",file,line,string);
			Flush(debug_file);
		}
	}
}

/****************************************************************************/

void
_DPRINTF_HEADER(
	const char *file,
	int line)
{
	if(__debug_level >= DEBUGLEVEL_Reports)
	{
		_INDENT();

		if(debug_file == (BPTR)NULL)
			kprintf("%s:%ld:",file,line);
		else
			FPrintf(debug_file,"%s:%ld:",file,line);
	}
}

#if defined(__SASC)

static void __asm putch(register __d0 UBYTE c)
{
	if(c != '\0')
		kputc(c);
}

#elif defined(__GNUC__) && defined(__amigaos4__)

static void putch(UBYTE c, APTR unused)
{
	if(c != '\0')
		kputc(c);
}

#elif defined(__GNUC__)

static void putch(UBYTE c __asm("d0"))
{
	if(c != '\0')
		kputc(c);
}

#endif /* __SASC */

void VARARGS68K
_DPRINTF(const char *fmt,...)
{
	if(__debug_level >= DEBUGLEVEL_Reports)
	{
		va_list args;

		#if defined(__amigaos4__)
		{
			va_startlinear(args,fmt);

			if(debug_file == (BPTR)NULL)
				RawDoFmt((char *)fmt,va_getlinearva(args, APTR),(VOID (*)())putch,NULL);
			else
				VFPrintf(debug_file,fmt,va_getlinearva(args, APTR));

			va_end(args);
		}
		#else
		{
			va_start(args,fmt);

			if(debug_file == (BPTR)NULL)
				RawDoFmt((char *)fmt,args,(VOID (*)())putch,NULL);
			else
				VFPrintf(debug_file,fmt,args);

			va_end(args);
		}
		#endif /* __amigaos4__ */

		if(debug_file == (BPTR)NULL)
		{
			kprintf("\n");
		}
		else
		{
			FPrintf(debug_file,"\n");
			Flush(debug_file);
		}
	}
}

void VARARGS68K
_DLOG(const char *fmt,...)
{
	if(__debug_level >= DEBUGLEVEL_Reports)
	{
		va_list args;

		#if defined(__amigaos4__)
		{
			va_startlinear(args,fmt);

			if(debug_file == (BPTR)NULL)
			{
				RawDoFmt((char *)fmt,va_getlinearva(args, APTR),(VOID (*)())putch,NULL);
			}
			else
			{
				VFPrintf(debug_file,fmt,va_getlinearva(args, APTR));
				Flush(debug_file);
			}

			va_end(args);
		}
		#else
		{
			va_start(args,fmt);

			if(debug_file == (BPTR)NULL)
			{
				RawDoFmt((char *)fmt,args,(VOID (*)())putch,NULL);
			}
			else
			{
				VFPrintf(debug_file,fmt,args);
				Flush(debug_file);
			}

			va_end(args);
		}
		#endif /* __amigaos4__ */
	}
}

/****************************************************************************/

void
_ENTER(
	const char *file,
	int line,
	const char *function)
{
	if(__debug_level >= DEBUGLEVEL_CallTracing)
	{
		_INDENT();

		if(debug_file == (BPTR)NULL)
		{
			kprintf("%s:%ld:Entering %s\n",file,line,function);
		}
		else
		{
			FPrintf(debug_file,"%s:%ld:Entering %s\n",file,line,function);
			Flush(debug_file);
		}
	}

	indent_level++;
}

void
_LEAVE(
	const char *file,
	int line,
	const char *function)
{
	indent_level--;

	if(__debug_level >= DEBUGLEVEL_CallTracing)
	{
		_INDENT();

		if(debug_file == (BPTR)NULL)
		{
			kprintf("%s:%ld: Leaving %s\n",file,line,function);
		}
		else
		{
			FPrintf(debug_file,"%s:%ld: Leaving %s\n",file,line,function);
			Flush(debug_file);
		}
	}
}

void
_RETURN(
	const char *file,
	int line,
	const char *function,
	unsigned long result)
{
	indent_level--;

	if(__debug_level >= DEBUGLEVEL_CallTracing)
	{
		_INDENT();

		if(debug_file == (BPTR)NULL)
		{
			kprintf("%s:%ld: Leaving %s (result 0x%08lx, %ld)\n",file,line,function,result,result);
		}
		else
		{
			FPrintf(debug_file,"%s:%ld: Leaving %s (result 0x%08lx, %ld)\n",file,line,function,result,result);
			Flush(debug_file);
		}
	}
}

/****************************************************************************/

void
_ASSERT(
	int x,
	const char *xs,
	const char *file,
	int line,
	const char *function)
{
	#ifdef CONFIRM
	{
		STATIC BOOL ScrollMode	= FALSE;
		STATIC BOOL BatchMode	= FALSE;

		if(BatchMode == FALSE && debug_file == (BPTR)NULL)
		{
			if(x == 0)
			{
				kprintf("%s:%ld:Expression '%s' failed assertion in %s().\n",
				        file,
				        line,
				        xs,
				        function);

				if(ScrollMode == FALSE)
				{
					ULONG Signals;

					SetSignal(0,SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D | SIGBREAKF_CTRL_E);

					kprintf(" ^C to continue, ^D to enter scroll mode, ^E to enter batch mode\r");

					Signals = Wait(SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D | SIGBREAKF_CTRL_E);

					if(Signals & SIGBREAKF_CTRL_D)
					{
						ScrollMode = TRUE;

						kprintf("Ok, entering scroll mode\033[K\n");
					}
					else if (Signals & SIGBREAKF_CTRL_E)
					{
						BatchMode = TRUE;

						kprintf("Ok, entering batch mode\033[K\n");
					}
					else
					{
						/* Continue */

						kprintf("\033[K\r");
					}
				}
			}
		}
	}
	#else
	{
		if(x == 0)
		{
			_INDENT();

			if(debug_file == (BPTR)NULL)
			{
				kprintf("%s:%ld:Expression '%s' failed assertion in %s().\n",
				        file,
				        line,
				        xs,
				        function);
			}
			else
			{
				FPrintf(debug_file,"%s:%ld:Expression '%s' failed assertion in %s().\n",
				        file,
				        line,
				        xs,
				        function);

				Flush(debug_file);
			}
		}
	}
	#endif	/* CONFIRM */
}
