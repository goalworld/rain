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
	struct rainModule *module;
	int caps;
	int cut;

}rain_module_mgr_t;
static rain_module_mgr_t* MGR = NULL;
struct rainModule
{
	void * _module;//模块指针
	char name[255];
	rainNewFn _init;
	rainDeleteFn _destroy;//模块导出的删除函数;
};
static int _open(const char * mod_name,struct rainModule *pmod);
static void* _tryopen(const char *mod_name);
int
rain_module_init(const char * mod_path)
{
	MGR = malloc(sizeof(rain_module_mgr_t));
	MGR->module = malloc(sizeof(struct rainModule)*MGR_SET);
	MGR->caps = MGR_SET;
	MGR->cut = 0;
	MGR->path = strdup(mod_path);
	return RAIN_OK;
}
const char*
rain_module_name(struct rainModule *mod)
{
	return mod->name;
}
struct rainModule *
rain_module_query(const char * mod_name)
{
	rain_module_mgr_t * mgr = MGR;
	int i;
	for(i=0; i<mgr->cut; i++){
		if(strcmp(mgr->module[i].name,mod_name) ==0){
			return &mgr->module[i];
		}
	}
	struct rainModule mod;
	int ret = _open(mod_name,&mod);
	if(ret == RAIN_ERROR){
		return NULL;
	}
	if(mgr->cut == mgr->caps){
		int new_caps = 2*mgr->caps;
		mgr->module = realloc(mgr->module,sizeof(struct rainModule)*new_caps);
	}
	struct rainModule *pmod = &mgr->module[mgr->cut++];
	*pmod = mod;
	return pmod;
}
void *
rain_module_inst_init(struct rainModule *mod,struct rainContext *ctx,
		const char *args)
{
	return mod->_init(ctx,args);
}
void
rain_module_inst_destroy(struct rainModule *mod,void *env,int code)
{
	mod->_destroy(env,code);
}
static int
_open(const char * mod_name,struct rainModule *pmod)
{
	void* dl = _tryopen(mod_name);
	if(!dl){
		return RAIN_ERROR;
	}
	int sz1= strlen(mod_name),sz2;

	sz2 = strlen("New");
	char init_name[sz1+sz2+1];
	memcpy(init_name ,mod_name,sz1);
	memcpy(init_name+sz1,"New",sz2);
	init_name[sz1+sz2] = 0x00;

	sz2 = strlen("Delete");
	char destroy_name[sz1+sz2+1];
	memcpy(destroy_name,mod_name,sz1);
	memcpy(destroy_name+sz1,"Delete",sz2);
	destroy_name[sz1+sz2] = 0x00;

	rainNewFn mainfn = (rainNewFn )( dlsym(dl,init_name) );
	rainDeleteFn delfn = ( rainDeleteFn )( dlsym(dl,destroy_name) );
	if( !delfn || !mainfn ){
		RAIN_LOG(0,"ERROR:dlsym New:%p,Delete:%p",mainfn,delfn);
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
