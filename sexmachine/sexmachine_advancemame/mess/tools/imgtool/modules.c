#include "modules.h"

#ifndef MODULES_RECURSIVE
#define MODULES_RECURSIVE

/* step 1: declare all external references */
#define MODULE(name)	extern imgtoolerr_t name##_createmodule(imgtool_library *library);
#include "modules.c"
#undef MODULE

/* step 2: define the modules[] array */
#define MODULE(name)	name##_createmodule,
static imgtoolerr_t (*modules[])(imgtool_library *library) =
{
#include "modules.c"
};

/* step 3: declare imgtool_create_cannonical_library() */
imgtoolerr_t imgtool_create_cannonical_library(int omit_untested, imgtool_library **library)
{
	imgtoolerr_t err;
	size_t i;
	imgtool_library *lib;
	struct ImageModule *module;

	/* list of modules that we drop */
	static const char *irrelevant_modules[] =
	{
		"coco_os9_rsdos"
	};

	lib = imgtool_library_create();
	if (!lib)
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto error;
	}

	for (i = 0; i < sizeof(modules) / sizeof(modules[0]); i++)
	{
		err = modules[i](lib);
		if (err)
			goto error;
	}

	/* remove irrelevant modules */
	for (i = 0; i < sizeof(irrelevant_modules)
			/ sizeof(irrelevant_modules[0]); i++)
	{
		imgtool_library_unlink(lib, irrelevant_modules[i]);
	}

	/* if we are omitting untested, go through and block out the functionality in question */
	if (omit_untested)
	{
		module = NULL;
		while((module = imgtool_library_iterate(lib, module)) != NULL)
		{
			if (module->writing_untested)
			{
				module->write_file = NULL;
				module->delete_file = NULL;
				module->create_dir = NULL;
				module->delete_dir = NULL;
				module->writefile_optguide = NULL;
				module->writefile_optspec = NULL;
				module->write_sector = NULL;
			}
			if (module->creation_untested)
			{
				module->create = NULL;
				module->createimage_optguide = NULL;
				module->createimage_optspec = NULL;
			}
		}
	}

	*library = lib;
	return IMGTOOLERR_SUCCESS;

error:
	if (lib)
		imgtool_library_close(lib);
	return err;

}


#else /* MODULES_RECURSIVE */

MODULE(concept)
MODULE(mac_mfs)
MODULE(mac_hfs)
MODULE(mess_hd)
MODULE(rsdos)
MODULE(vzdos)
MODULE(os9)
MODULE(ti99)
MODULE(ti990)
MODULE(fat)
MODULE(pc_chd)
MODULE(prodos_525)
MODULE(prodos_35)

#endif /* MODULES_RECURSIVE */
