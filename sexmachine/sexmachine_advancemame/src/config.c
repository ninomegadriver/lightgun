/***************************************************************************

    config.c

    Configuration file I/O.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "driver.h"
#include "config.h"
#include "xmlfile.h"


#define DEBUG_CONFIG		0



/*************************************
 *
 *  Constants
 *
 *************************************/

#define CONFIG_VERSION			10



/*************************************
 *
 *  Type definitions
 *
 *************************************/

typedef struct _config_type config_type;
struct _config_type
{
	struct _config_type *	next;				/* next in line */
	const char *			name;				/* node name */
	config_callback			load;				/* load callback */
	config_callback			save;				/* save callback */
};



/*************************************
 *
 *  Global variables
 *
 *************************************/

static config_type *typelist;



/*************************************
 *
 *  Prototypes
 *
 *************************************/

static int config_load_xml(mame_file *file, int type);
static int config_save_xml(mame_file *file, int type);



/*************************************
 *
 *  Reset the configuration callbacks
 *
 *************************************/

void config_init(void)
{
	typelist = NULL;

#ifdef MESS
	mess_config_init();
#endif
}



/*************************************
 *
 *  Register to be involved in config
 *  save/load
 *
 *************************************/

void config_register(const char *nodename, config_callback load, config_callback save)
{
	config_type *newtype;
	config_type **ptype;

	/* allocate a new type */
	newtype = auto_malloc(sizeof(*newtype));
	newtype->next = NULL;
	newtype->name = nodename;
	newtype->load = load;
	newtype->save = save;

	/* add us to the end */
	for (ptype = &typelist; *ptype; ptype = &(*ptype)->next) ;
	*ptype = newtype;
}



/*************************************
 *
 *  Settings save/load frontend
 *
 *************************************/

int input_config_load_settings();
void input_config_save_settings(void);

int config_load_settings(void)
{
	config_type *type;
	mame_file *file;
	int loaded = 0;

	/* loop over all registrants and call their init function */
	for (type = typelist; type; type = type->next)
		(*type->load)(CONFIG_TYPE_INIT, NULL);

/* AdvanceMAME: Specific configuration for port */
#if 1
	loaded = input_config_load_settings();
#else
	/* now load the controller file */
	if (options.controller != NULL)
	{
		/* open the config file */
		file = mame_fopen(NULL, options.controller, FILETYPE_CTRLR, 0);
		if (!file)
			fatalerror("Could not load controller file %s.cfg", options.controller);

		/* load the XML */
		if (!config_load_xml(file, CONFIG_TYPE_CONTROLLER))
			fatalerror("Could not load controller file %s.cfg", options.controller);
		mame_fclose(file);
	}

	/* next load the defaults file */
	file = mame_fopen("default", 0, FILETYPE_CONFIG, 0);
	if (file)
	{
		config_load_xml(file, CONFIG_TYPE_DEFAULT);
		mame_fclose(file);
	}

	/* finally, load the game-specific file */
	file = mame_fopen(Machine->gamedrv->name, 0, FILETYPE_CONFIG, 0);
	if (file)
	{
		loaded = config_load_xml(file, CONFIG_TYPE_GAME);
		mame_fclose(file);
	}
#endif

	/* loop over all registrants and call their final function */
	for (type = typelist; type; type = type->next)
		(*type->load)(CONFIG_TYPE_FINAL, NULL);

	/* if we didn't find a saved config, return 0 so the main core knows that it */
	/* is the first time the game is run and it should diplay the disclaimer. */
	return loaded;
}


void config_save_settings(void)
{
	config_type *type;
	mame_file *file;

	/* loop over all registrants and call their init function */
	for (type = typelist; type; type = type->next)
		(*type->save)(CONFIG_TYPE_INIT, NULL);

/* AdvanceMAME: Specific configuration for port */
#if 1
	input_config_save_settings();
#else
	/* save the defaults file */
	file = mame_fopen("default", 0, FILETYPE_CONFIG, 1);
	if (file)
	{
		config_save_xml(file, CONFIG_TYPE_DEFAULT);
		mame_fclose(file);
	}

	/* finally, save the game-specific file */
	file = mame_fopen(Machine->gamedrv->name, 0, FILETYPE_CONFIG, 1);
	if (file)
	{
		config_save_xml(file, CONFIG_TYPE_GAME);
		mame_fclose(file);
	}
#endif

	/* loop over all registrants and call their final function */
	for (type = typelist; type; type = type->next)
		(*type->save)(CONFIG_TYPE_FINAL, NULL);
}



/*************************************
 *
 *  XML file load
 *
 *************************************/

static int config_load_xml(mame_file *file, int which_type)
{
	xml_data_node *root, *confignode, *systemnode;
	config_type *type;
	const char *srcfile;
	int version, count;

	/* read the file */
	root = xml_file_read(file, NULL);
	if (!root)
		goto error;

	/* find the config node */
	confignode = xml_get_sibling(root->child, "mameconfig");
	if (!confignode)
		goto error;

	/* validate the config data version */
	version = xml_get_attribute_int(confignode, "version", 0);
	if (version != CONFIG_VERSION)
		goto error;

	/* strip off all the path crap from the source filename */
	srcfile = strrchr(Machine->gamedrv->source_file, '/');
	if (!srcfile)
		srcfile = strrchr(Machine->gamedrv->source_file, '\\');
	if (!srcfile)
		srcfile = strrchr(Machine->gamedrv->source_file, ':');
	if (!srcfile)
		srcfile = Machine->gamedrv->source_file;
	else
		srcfile++;

	/* loop over all system nodes in the file */
	count = 0;
	for (systemnode = xml_get_sibling(confignode->child, "system"); systemnode; systemnode = xml_get_sibling(systemnode->next, "system"))
	{
		/* look up the name of the system here; skip if none */
		const char *name = xml_get_attribute_string(systemnode, "name", "");

		/* based on the file type, determine whether we have a match */
		switch (which_type)
		{
			case CONFIG_TYPE_GAME:
				/* only match on the specific game name */
				if (strcmp(name, Machine->gamedrv->name) != 0)
					continue;
				break;

			case CONFIG_TYPE_DEFAULT:
				/* only match on default */
				if (strcmp(name, "default") != 0)
					continue;
				break;

			case CONFIG_TYPE_CONTROLLER:
			{
				const game_driver *clone_of;
				/* match on: default, game name, source file name, parent name, grandparent name */
				if (strcmp(name, "default") != 0 &&
					strcmp(name, Machine->gamedrv->name) != 0 &&
					strcmp(name, srcfile) != 0 &&
					((clone_of = driver_get_clone(Machine->gamedrv)) == NULL || strcmp(name, clone_of->name) != 0) &&
					(clone_of == NULL || ((clone_of = driver_get_clone(clone_of)) == NULL) || strcmp(name, clone_of->name) != 0))
					continue;
				break;
			}
		}

		/* log that we are processing this entry */
		if (DEBUG_CONFIG)
			printf("Entry: %s -- processing\n", name);

		/* loop over all registrants and call their load function */
		for (type = typelist; type; type = type->next)
			(*type->load)(which_type, xml_get_sibling(systemnode->child, type->name));
		count++;
	}

	/* error if this isn't a valid game match */
	if (count == 0)
		goto error;

	/* free the parser */
	xml_file_free(root);
	return 1;

error:
	if (root)
		xml_file_free(root);
	return 0;
}



/*************************************
 *
 *  XML file save
 *
 *************************************/

static int config_save_xml(mame_file *file, int which_type)
{
	xml_data_node *root = xml_file_create();
	xml_data_node *confignode, *systemnode;
	config_type *type;

	/* if we don't have a root, bail */
	if (!root)
		return 0;

	/* create a config node */
	confignode = xml_add_child(root, "mameconfig", NULL);
	if (!confignode)
		goto error;
	xml_set_attribute_int(confignode, "version", CONFIG_VERSION);

	/* create a system node */
	systemnode = xml_add_child(confignode, "system", NULL);
	if (!systemnode)
		goto error;
	xml_set_attribute(systemnode, "name", (which_type == CONFIG_TYPE_DEFAULT) ? "default" : Machine->gamedrv->name);

	/* create the input node and write it out */
	/* loop over all registrants and call their save function */
	for (type = typelist; type; type = type->next)
	{
		xml_data_node *curnode = xml_add_child(systemnode, type->name, NULL);
		if (!curnode)
			goto error;
		(*type->save)(which_type, curnode);

		/* if nothing was added, just nuke the node */
		if (!curnode->value && !curnode->child)
			xml_delete_node(curnode);
	}

	/* flush the file */
	xml_file_write(root, file);

	/* free and get out of here */
	xml_file_free(root);
	return 1;

error:
	xml_file_free(root);
	return 0;
}
