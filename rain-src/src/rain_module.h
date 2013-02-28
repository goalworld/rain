/*
 * rain_module.h
 *
 *  Created on: 2012-11-9
 *      Author: goalworld
 */

#ifndef RAIN_MODULE_H_
#define RAIN_MODULE_H_
struct rain_ctx;
struct rain_moudle;
int rain_moudle_init(const char * mod_path);
struct rain_moudle * rain_module_query(const char * mod_name);
void * rain_module_instance_init(struct rain_moudle *mod,struct rain_ctx *ctx,const char *args);
void rain_module_init_destroy(struct rain_moudle *mod,void *env,int code);
const char* rain_module_get_name(struct rain_moudle *mod);
#endif /* RAIN_MODULE_H_ */
