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
int rainModuleInit(const char * mod_path);
struct rainModule * rainModuleQuery(const char * mod_name);
void * rainModuleInstNew(struct rainModule *mod,struct rainContext *ctx,const char *args);
void rainModuleInstDel(struct rainModule *mod,void *env,int code);
const char* rainModuleName(struct rainModule *mod);
#endif /* RAIN_MODULE_H_ */
