#pragma once

void debug(const char* file,int line,const char* message);
void fatal(const char* file,int line,const char* message);

#define DEBUG(message) debug(__FILE__, __LINE__, message)
#define FATAL(message) fatal(__FILE__, __LINE__, message)
#define FORCE(expression, message) ((expression) ? 0 : (FATAL(message), 0))