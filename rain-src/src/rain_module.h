/*
 * rain_module.h
 *
 *  Created on: 2012-11-9
 *      Author: goalworld
 */

#ifndef RAIN_MODULE_H_
#define RAIN_MODULE_H_
struct rain_ctx_s;
typedef struct rain_module_s rain_module_t;
int rain_module_init(const char * mod_path);
rain_module_t * rain_module_query(const char * mod_name);
void * rain_module_inst_init(rain_module_t *mod,struct rain_ctx_s *ctx,const char *args);
void rain_module_inst_destroy(rain_module_t *mod,void *env,int code);
const char* rain_module_name(rain_module_t *mod);
#endif /* RAIN_MODULE_H_ */
