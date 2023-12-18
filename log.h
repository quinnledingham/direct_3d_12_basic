#ifndef LOG_H
#define LOG_H

// stupid log just to have my own functions

void output(const char* msg, ...);
void error(int line_num, const char* msg, ...);
void error(const char* msg, ...);
void warning(int line_num, const char* msg, ...);

#endif //LOG_H