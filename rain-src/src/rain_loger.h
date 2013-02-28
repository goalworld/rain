/*
 * rain_loger.h
 *
 *  Created on: 2012-11-17
 *      Author: goalworld
 */

#ifndef RAIN_LOGER_H_
#define RAIN_LOGER_H_
#define LOG_LEVEL 3
#define RAIN_LOG(level,fmt,...)\
		do{\
			if(level <= LOG_LEVEL){\
				rain_log(__FILE__,__LINE__,fmt,__VA_ARGS__);	\
			}						      \
		}while(0)
int rain_log_init();
void rain_log(const char *filename,int line,const char *fmt,...);

#endif /* RAIN_LOGER_H_ */
