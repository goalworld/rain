/*
 * rain_module.h
 *
 *  Created on: 2012-11-9
 *      Author: goalworld
 */

#ifndef RAIN_MODULE_H_
#define RAIN_MODULE_H_
struct rainContext;
struct rainModule;
int rain_module_init(const char * mod_path);
struct rainModule * rain_module_query(const char * mod_name);
void * rain_module_inst_init(struct rainModule *mod,struct rainContext *ctx,const char *args);
void rain_module_inst_destroy(struct rainModule *mod,void *env,int code);
const char* rain_module_name(struct rainModule *mod);
#endif /* RAIN_MODULE_H_ */
