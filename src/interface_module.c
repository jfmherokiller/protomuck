//
// Created by peter on 7/9/20.
//
/* ProtoMUCK's main interface.c */
/*  int main() is in this file  */
#include "copyright.h"
#include "config.h"

#include "cgi.h"
#include "defaults.h"
#include "db.h"
#include "interface.h"
#include "params.h"
#include "tune.h"
#include "props.h"
#include "match.h"
#include "reg.h"
#ifdef MCP_SUPPORT
# include "mcp.h"
# include "mcpgui.h"
#endif
#include "mpi.h"
#include "reg.h"
#include "externs.h"
#include "mufevent.h"
#include "interp.h"
#include "newhttp.h"            /* hinoserm */
#include "netresolve.h"         /* hinoserm */
#ifdef UTF8_SUPPORT
# include <locale.h>
#endif

#ifdef MODULAR_SUPPORT
struct module *modules = NULL;
#endif
#ifdef MODULAR_SUPPORT

char module_error[BUFFER_LEN];

void
module_remember(struct module *m)
{
	if (modules)
		modules->prev = m;
	m->next = modules;
	modules = m;
}

char *
module_load(const char *filename, dbref who)
{
	struct module *m = (struct module *)malloc(sizeof(struct module));
	struct module *mo;
	void *(*func)();
	char *error;

	dlerror();
	m->handle = dlopen(filename, RTLD_NOW | RTLD_GLOBAL);

	if (!m->handle && (error = dlerror()) != NULL) {
		free((void *)m);
		log_status("MODULE_LOAD(%s): ERROR %s\r\n", filename, error);
		return error;
	}

	func = dlsym(m->handle, "__get_module_info");

	if ((error = dlerror()) != NULL)  {
		dlclose(m->handle);
		free((void *)m);
		log_status("MODULE_LOAD(%s): ERROR %s\r\n", filename, error);
		return error;
	}

	if (!(m->info = (*func)(m))) {
		dlclose(m->handle);
		free((void *)m);
		log_status("MODULE_LOAD(%s): ERROR %s\r\n", filename, module_error);
		return module_error;
	}

	mo = modules;
	while (mo) {
        if (mo->handle == m->handle) {
			strcpy(module_error, "module is already loaded");
			break;
		} else if (!string_compare(mo->info->name, m->info->name)) {
			strcpy(module_error, "module with same name is already loaded");
			dlclose(m->handle);
			break;
		}
		mo = mo->next;
	}

	if (mo) {
		log_status("MODULE_LOAD(%s): ERROR %s\r\n", m->info->name, module_error);
		free((void *)m);
		return module_error;
	}

	/* for (i = 0; mr->info->requires[i].name; i++) {

	} */


	m->prims = NULL;
	func = dlsym(m->handle, "__get_prim_list");

	if ((error = dlerror()) == NULL)  {
		m->prims = (*func)();

		if (m->prims) {
			int i;
			for (i = 0; m->prims[i].name; i++) {
				m->prims[i].mod = m;
				m->prims[i].func = dlsym(m->handle, m->prims[i].sym);

				if ((error = dlerror()) != NULL)  {
					sprintf(module_error, "Bad Primitive (%s): %s", m->prims[i].name,  error);
					log_status("MODULE_LOAD(%s): ERROR %s\r\n", m->info->name, module_error);

					dlclose(m->handle);
					free((void *)m);
					return module_error;
				}

				log_status("MODULE(%s) Has PRIM_%s\r\n", filename, m->prims[i].name);
			}
		}
	}

	strcpy(module_error, "");

	m->progs = NULL;
	m->prev = NULL;
	m->next = NULL;

	module_remember(m);

	return NULL;
}

void
module_free(struct module *m)
{

	if (m->progs) {
		struct mod_proglist *mpr = m->progs;

		while (mpr) {
			struct mod_proglist *ompr = mpr->next;
			/* TODO: this needs to safely deal with loaded muf programs --hinoserm */
			uncompile_program(mpr->prog);

			//free((void *)mpr);
			mpr = ompr;
		}
	}

	if (m->next)
	   m->next->prev = m->prev;
	if (m->prev)
		m->prev->next = m->next;
	if (m == modules)
		modules = m->next;

	log_status("MODULE: Free'd %s.\r\n", m->info->name);
	dlclose(m->handle);
	free((void *)m);
}


#endif
