/*
 * rain_module.c
 *
 *  Created on: 2012-11-9
 *      Author: goalworld
 */

#include "rain_module.h"
#include "error.h"
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "rain.h"
#include "rain_loger.h"
#define MGR_SET 64
typedef struct rain_module_mgr_s
{
	char * path;
	struct rain_moudle *module;
	int caps;
	int cut;

}rain_module_mgr_t;
static rain_module_mgr_t* MGR = NULL;
struct rain_moudle
{
	void * _module;//模块指针
	char name[255];
	rain_new_fn _init;
	rain_del_fn _destroy;//模块导出的删除函数;
};
static int _open(const char * mod_name,struct rain_moudle *pmod);
static void* _tryopen(const char *mod_name);
int
rain_moudle_init(const char * mod_path)
{
	MGR = malloc(sizeof(rain_module_mgr_t));
	MGR->module = malloc(sizeof(struct rain_moudle)*MGR_SET);
	MGR->caps = MGR_SET;
	MGR->cut = 0;
	MGR->path = strdup(mod_path);
	return RAIN_OK;
}
const char*
rain_module_get_name(struct rain_moudle *mod)
{
	return mod->name;
}
struct rain_moudle *
rain_module_query(const char * mod_name)
{
	rain_module_mgr_t * mgr = MGR;
	int i;
	for(i=0; i<mgr->cut; i++){
		if(strcmp(mgr->module[i].name,mod_name) ==0){
			return &mgr->module[i];
		}
	}
	struct rain_moudle mod;
	int ret = _open(mod_name,&mod);
	if(ret == RAIN_ERROR){
		return NULL;
	}
	if(mgr->cut == mgr->caps){
		int new_caps = 2*mgr->caps;
		mgr->module = realloc(mgr->module,sizeof(struct rain_moudle)*new_caps);
	}
	struct rain_moudle *pmod = &mgr->module[mgr->cut++];
	*pmod = mod;
	return pmod;
}
void *
rain_module_instance_init(struct rain_moudle *mod,struct rain_ctx *ctx,
		const char *args)
{
	return mod->_init(ctx,args);
}
void
rain_module_init_destroy(struct rain_moudle *mod,void *env,int code)
{
	mod->_destroy(env,code);
}
static int
_open(const char * mod_name,struct rain_moudle *pmod)
{
	void* dl = _tryopen(mod_name);
	if(!dl){
		return RAIN_ERROR;
	}
	int sz1= strlen(mod_name),sz2;

	sz2 = strlen("_new");
	char init_name[sz1+sz2+1];
	memcpy(init_name ,mod_name,sz1);
	memcpy(init_name+sz1,"_new",sz2);
	init_name[sz1+sz2] = 0x00;

	sz2 = strlen("_delete");
	char destroy_name[sz1+sz2+1];
	memcpy(destroy_name,mod_name,sz1);
	memcpy(destroy_name+sz1,"_delete",sz2);
	destroy_name[sz1+sz2] = 0x00;

	rain_new_fn mainfn = (rain_new_fn )( dlsym(dl,init_name) );
	rain_del_fn delfn = ( rain_del_fn )( dlsym(dl,destroy_name) );
	if( !delfn || !mainfn ){
		RAIN_LOG(0,"ERROR: %s dlsym _new:%p,_delete:%p",mod_name,mainfn,delfn);
		dlclose(dl);
		return RAIN_ERROR;
	}
	pmod->_destroy = delfn;
	pmod->_init = mainfn;
	pmod->_module = dl;
	strcpy(pmod->name,mod_name);
	return RAIN_OK;
}
static void*
_tryopen(const char *name)
{
	int sz1,sz2,sz3,sz4;
	sz1 = strlen(MGR->path);
	char * lib = "lib";
	sz4 = strlen(lib);
	sz2 = strlen(name);
	char * so = ".so";
	sz3 = strlen(so);
	int len = sz1+sz4+sz2+sz3+1;
	char path[len];
	memcpy(path,MGR->path,sz1);
	memcpy(path+sz1,lib,sz4);
	memcpy(path+sz1+sz4,name,sz2);
	memcpy(path+sz1+sz2+sz4,so,sz3);
	path[len-1] = 0x00;
	void * dl = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
	if(!dl){
		RAIN_LOG(0,"dlopen-error:%s  %s\n",path,dlerror());
	}
	return dl;
}
