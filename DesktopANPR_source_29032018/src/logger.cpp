/*
 * logger.cpp
 *
 *  Created on: Mar 12, 2018
 *      Author: amis
 */

#include "logger.h"

static const char *LOGLEVEL_NAME[] = { "CRITICAL", "ERROR", "WARNING", "NOTICE", "INFO", "DEBUG" };

void stdout_log(LogLevel level, const char * file, int line, const char * format, ...)
{
	va_list ap;
	va_start(ap, format);
	char message[LOG_LINE_MAXLEN + 1];
	vsnprintf(message, sizeof(message), format, ap);
	va_end(ap);

	for(int ptr = strlen(file) - 1, cnt = 0; ptr > 0; --ptr)
	{
		if(file[ptr] == '/')
		{
			cnt++;
			if(cnt >= 2)
			{
				file += ptr + 1;
				break;
			}
		}
	}

    bool run = true;
    for(char *cur = message, *begin = cur; run; ++cur)
    {
    	if((*cur == '\n') || (*cur == 0))
    	{
    		if(*cur == 0) { run = false; }
    		else { *cur = 0; }
    		if((!run) || (begin == message) || (*begin != 0))
    		{
    			printf("<%s:%d> [%s] %s\n", file, line, LOGLEVEL_NAME[(int)level], begin);
    		}
    		begin = cur + 1;
    	}
    }
}
