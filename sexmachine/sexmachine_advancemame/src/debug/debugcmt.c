/***************************************************************************

    debugcmt.c

    Debugger code-comment management functions.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

****************************************************************************

    Debugger comment file format:

    (from comment struct - assuming (MAX_COMMENT_LINE_LENGTH == 128))
    0           - valid byte
    1:4         - address
    5:133       - comment
    134:138     - color
    139:142     - instruction crc

***************************************************************************/

#include "driver.h"
#include "xmlfile.h"
#include "debugcmt.h"
#include "debugcpu.h"
#include "debugvw.h"
#include <zlib.h>



/***************************************************************************
    DEBUGGING
***************************************************************************/

//#define VERBOSE

#ifdef VERBOSE
#define TRACE(x) do {x;} while (0)
#else
#define TRACE(x)
#endif



/***************************************************************************
    CONSTANTS
***************************************************************************/

#define COMMENT_VERSION	(1)



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _debug_comment debug_comment;
struct _debug_comment
{
	UINT8 is_valid;
	UINT32 address;
	char text[DEBUG_COMMENT_MAX_LINE_LENGTH];
	rgb_t color;
	UINT32 crc;
};

typedef struct _comment_group comment_group;
struct _comment_group
{
	int comment_count;
	UINT32 change_count;
	debug_comment *comment_info[DEBUG_COMMENT_MAX_NUM];
};



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

static comment_group *debug_comments;



/***************************************************************************
    PROTOTYPES
***************************************************************************/

static void debug_comment_exit(void);
static void debug_comment_free(void);



/***************************************************************************

    Initialization

***************************************************************************/

/*-------------------------------------------------------------------------
    debug_comment_init - initializes the comment memory and
                         loads any existing comment file
-------------------------------------------------------------------------*/

int debug_comment_init(void)
{
	/* allocate enough comment groups for the total # of cpu's */
	debug_comments = (comment_group*) malloc(cpu_gettotalcpu() * sizeof(comment_group));

	/* automatically load em up */
	debug_comment_load();

	add_exit_callback(debug_comment_exit);

	return 1;
}


/*-------------------------------------------------------------------------
    debug_comment_add - adds a comment to the list at the given address.
                        use debug_comment_get_opcode_crc32(addr) to get
                        the proper crc32
-------------------------------------------------------------------------*/

int debug_comment_add(int cpu_num, offs_t addr, const char *comment, rgb_t color, UINT32 c_crc)
{
	int i = 0;
	int insert_point = debug_comments[cpu_num].comment_count;
	int match = 0;

	/* Create a new item to insert into the list */
	debug_comment *insert_me = (debug_comment*) malloc(sizeof(debug_comment));
	insert_me->color = color;
	insert_me->is_valid = 1;
	insert_me->address = addr;
	insert_me->crc = c_crc;
	strcpy(insert_me->text, comment);

	/* Find the insert point */
	for (i = 0; i < debug_comments[cpu_num].comment_count; i++)
	{
		if (insert_me->address < debug_comments[cpu_num].comment_info[i]->address)
		{
			insert_point = i;
			break;
		}
		else if (insert_me->address == debug_comments[cpu_num].comment_info[i]->address &&
				 insert_me->crc == debug_comments[cpu_num].comment_info[i]->crc)
		{
			insert_point = i;
			match = 1;
			break;
		}
	}

	/* Got an exact match?  Just replace */
	if (match == 1)
	{
		free(debug_comments[cpu_num].comment_info[insert_point]);
		debug_comments[cpu_num].comment_info[insert_point] = insert_me;
		debug_comments[cpu_num].change_count++;

		/* force an update of disassembly views */
		debug_view_update_type(DVT_DISASSEMBLY);
		return 1;
	}

	/* Otherwise insert */
	/* First, shift the list down */
	for (i = debug_comments[cpu_num].comment_count; i >= insert_point; i--)
		debug_comments[cpu_num].comment_info[i] = debug_comments[cpu_num].comment_info[i-1];

	/* do the insertion */
	debug_comments[cpu_num].comment_info[insert_point] = insert_me;
	debug_comments[cpu_num].comment_count++;
	debug_comments[cpu_num].change_count++;

	/* force an update of disassembly views */
	debug_view_update_type(DVT_DISASSEMBLY);

	return 1;
}


/*-------------------------------------------------------------------------
    debug_comment_remove - removes a comment at a given address
                        use debug_comment_get_opcode_crc32(addr) to get
                        the proper crc32
-------------------------------------------------------------------------*/

int debug_comment_remove(int cpu_num, offs_t addr, UINT32 c_crc)
{
	int i;
	int remove_index = -1;

	for (i = 0; i < debug_comments[cpu_num].comment_count; i++)
	{
		if (debug_comments[cpu_num].comment_info[i]->address == addr)	/* got an address match */
		{
			if (debug_comments[cpu_num].comment_info[i]->crc == c_crc)
			{
				remove_index = i;
			}
		}
	}

	/* The comment doesn't exist? */
	if (remove_index == -1)
		return 0;

	/* Okay, it's there, now remove it */
	free(debug_comments[cpu_num].comment_info[remove_index]);

	for (i = remove_index; i < debug_comments[cpu_num].comment_count-1; i++)
	{
		debug_comments[cpu_num].comment_info[i] = debug_comments[cpu_num].comment_info[i+1];
	}

	debug_comments[cpu_num].comment_count--;
	debug_comments[cpu_num].change_count++;

	/* force an update of disassembly views */
	debug_view_update_type(DVT_DISASSEMBLY);

	return 1;
}


/*-------------------------------------------------------------------------
    debug_comment_get_text - returns the comment for a given addresses
                        use debug_comment_get_opcode_crc32(addr) to get
                        the proper crc32
-------------------------------------------------------------------------*/

const char *debug_comment_get_text(int cpu_num, offs_t addr, UINT32 c_crc)
{
	int i;

	/* inefficient - should use bsearch - but will be a little tricky with multiple comments per addr */
	for (i = 0; i < debug_comments[cpu_num].comment_count; i++)
	{
		if (debug_comments[cpu_num].comment_info[i]->address == addr)	/* got an address match */
		{
			/* now check the bank information to be sure */
			if (debug_comments[cpu_num].comment_info[i]->crc == c_crc)
			{
				return debug_comments[cpu_num].comment_info[i]->text;
			}
		}
	}

	return 0x00;
}


/*-------------------------------------------------------------------------
    debug_comment_get_count - returns the number of comments
    for a given cpu number
-------------------------------------------------------------------------*/

int debug_comment_get_count(int cpu_num)
{
	return debug_comments[cpu_num].comment_count;
}


/*-------------------------------------------------------------------------
    debug_comment_get_change_count - returns the change counter
    for a given cpu number
-------------------------------------------------------------------------*/

UINT32 debug_comment_get_change_count(int cpu_num)
{
	return debug_comments[cpu_num].change_count;
}


/*-------------------------------------------------------------------------
    debug_comment_get_opcode_crc32 - magic function that takes all the
                        current state of the debugger and returns a crc32
                        for the opcode at the requested address.
-------------------------------------------------------------------------*/

UINT32 debug_comment_get_opcode_crc32(offs_t address)
{
	int i;
	UINT32 crc;
	UINT8 opbuf[64], argbuf[64];
	char buff[256];
	offs_t numbytes;
	int maxbytes = activecpu_max_instruction_bytes();
	UINT32 addrmask = (debug_get_cpu_info(cpu_getactivecpu()))->space[ADDRESS_SPACE_PROGRAM].logaddrmask;

	int use_new_dasm = (activecpu_get_info_fct(CPUINFO_PTR_DISASSEMBLE_NEW) != NULL);

	memset(opbuf, 0x00, sizeof(opbuf));
	memset(argbuf, 0x00, sizeof(argbuf));

	// fetch the bytes up to the maximum
	for (i = 0; i < maxbytes; i++)
	{
		opbuf[i] = debug_read_opcode(address + i, 1, FALSE);
		argbuf[i] = debug_read_opcode(address + i, 1, TRUE);
	}

	if (use_new_dasm)
	{
		numbytes = cpunum_dasm_new(cpu_getactivecpu(), buff, address & addrmask, opbuf, argbuf, maxbytes) & DASMFLAG_LENGTHMASK;
	}
	else
	{
		numbytes = cpunum_dasm(cpu_getactivecpu(), buff, address & addrmask) & DASMFLAG_LENGTHMASK;
	}

	crc = crc32(0, argbuf, numbytes);

	return crc;
}


/*-------------------------------------------------------------------------
    debug_comment_dump - debugging function to dump junk to the command line
-------------------------------------------------------------------------*/

void debug_comment_dump(int cpu_num, offs_t addr)
{
	int i;
	int ff = 0;

	if (addr == -1)
	{
		for (i = 0; i < debug_comments[cpu_num].comment_count; i++)
		{
			if (debug_comments[cpu_num].comment_info[i]->is_valid)
			{
				logerror("%d : %s (%d %d)\n", i, debug_comments[cpu_num].comment_info[i]->text,
												 debug_comments[cpu_num].comment_info[i]->address,
												 debug_comments[cpu_num].comment_info[i]->crc);
			}
		}
	}
	else
	{
		UINT32 c_crc = debug_comment_get_opcode_crc32(addr);

		for (i = 0; i < debug_comments[cpu_num].comment_count; i++)
		{
			if (debug_comments[cpu_num].comment_info[i]->address == addr)	/* got an address match */
			{
				/* now check the bank information to be sure */
				if (debug_comments[cpu_num].comment_info[i]->crc == c_crc)
				{
					logerror("%d : %s (%d %d)\n", addr,
												  debug_comments[cpu_num].comment_info[addr]->text,
												  debug_comments[cpu_num].comment_info[addr]->address,
												  debug_comments[cpu_num].comment_info[addr]->crc);
					ff = 1;
				}
			}
		}

		if (!ff) logerror("No comment exists for address : 0x%x\n", addr);
	}
}


/*-------------------------------------------------------------------------
    debug_comment_save - comment file saving
-------------------------------------------------------------------------*/

int debug_comment_save(void)
{
	int i, j;
	char crc_buf[20];
	xml_data_node *root = xml_file_create();
	xml_data_node *commentnode, *systemnode;
	int total_comments = 0;

	/* if we don't have a root, bail */
	if (!root)
		return 0;

	/* create a comment node */
	commentnode = xml_add_child(root, "mamecommentfile", NULL);
	if (!commentnode)
		goto error;
	xml_set_attribute_int(commentnode, "version", COMMENT_VERSION);

	/* create a system node */
	systemnode = xml_add_child(commentnode, "system", NULL);
	if (!systemnode)
		goto error;
	xml_set_attribute(systemnode, "name", Machine->gamedrv->name);

	/* for each cpu */
	for (i = 0; i < cpu_gettotalcpu(); i++)
	{
		xml_data_node *curnode = xml_add_child(systemnode, "cpu", NULL);
		if (!curnode)
			goto error;
		xml_set_attribute_int(curnode, "num", i);

		for (j = 0; j < debug_comments[i].comment_count; j++)
		{
			xml_data_node *datanode = xml_add_child(curnode, "comment", debug_comments[i].comment_info[j]->text);
			if (!datanode)
				goto error;
			xml_set_attribute_int(datanode, "address", debug_comments[i].comment_info[j]->address);
			xml_set_attribute_int(datanode, "color", debug_comments[i].comment_info[j]->color);
			sprintf(crc_buf, "%08X", debug_comments[i].comment_info[j]->crc);
			xml_set_attribute(datanode, "crc", crc_buf);
			total_comments++;
		}
	}

	/* flush the file */
	if (total_comments > 0)
	{
 		mame_file *fp = mame_fopen(Machine->gamedrv->name, 0, FILETYPE_COMMENT, 1);
 		xml_file_write(root, fp);
		mame_fclose(fp);
	}

	/* free and get out of here */
	xml_file_free(root);
	return 1;

error:
	xml_file_free(root);
	return 0;
}


/*-------------------------------------------------------------------------
    debug_comment_load(_xml) - comment file loading
-------------------------------------------------------------------------*/

int debug_comment_load(void)
{
	mame_file *fp = mame_fopen(Machine->gamedrv->name, 0, FILETYPE_COMMENT, 0);
	if (!fp) return 0;
	debug_comment_load_xml(fp);
	mame_fclose(fp);

	return 1;
}

static int debug_comment_load_xml(mame_file *fp)
{
	int i, j;
	xml_data_node *root, *commentnode, *systemnode, *cpunode, *datanode;
	const char *name;
	int version;

	/* read the file */
	root = xml_file_read(fp, NULL);
	if (!root)
		goto error;

	/* find the config node */
	commentnode = xml_get_sibling(root->child, "mamecommentfile");
	if (!commentnode)
		goto error;

	/* validate the config data version */
	version = xml_get_attribute_int(commentnode, "version", 0);
	if (version != COMMENT_VERSION)
		goto error;

	/* check to make sure the file is applicable */
	systemnode = xml_get_sibling(commentnode->child, "system");
	name = xml_get_attribute_string(systemnode, "name", "");
	if (strcmp(name, Machine->gamedrv->name) != 0)
		goto error;

	i = 0;

	for (cpunode = xml_get_sibling(systemnode->child, "cpu"); cpunode; cpunode = xml_get_sibling(cpunode->next, "cpu"))
	{
		j = 0;

		for (datanode = xml_get_sibling(cpunode->child, "comment"); datanode; datanode = xml_get_sibling(datanode->next, "comment"))
		{
			/* Malloc the comment */
			debug_comments[i].comment_info[j] = (debug_comment*) malloc(sizeof(debug_comment));

			debug_comments[i].comment_info[j]->address = xml_get_attribute_int(datanode, "address", 0);
			debug_comments[i].comment_info[j]->color = xml_get_attribute_int(datanode, "color", 0);
			sscanf(xml_get_attribute_string(datanode, "crc", 0), "%08X", &debug_comments[i].comment_info[j]->crc);
			strcpy(debug_comments[i].comment_info[j]->text, datanode->value);
			debug_comments[i].comment_info[j]->is_valid = 1;

			j++;
		}

		debug_comments[i].comment_count = j;

		i++;
	}

	/* free the parser */
	xml_file_free(root);

	return 1;

error:
	if (root)
		xml_file_free(root);
	return 0;
}


/*-------------------------------------------------------------------------
    debug_comment_exit - saves the comments and frees memory
-------------------------------------------------------------------------*/

static void debug_comment_exit(void)
{
	debug_comment_save();
	debug_comment_free();
}


/*-------------------------------------------------------------------------
    debug_comment_free - cleans up memory
-------------------------------------------------------------------------*/

static void debug_comment_free(void)
{
	int i, j;

	for (i = 0; i < cpu_gettotalcpu(); i++)
	{
		for (j = 0; j < debug_comments[i].comment_count; j++)
		{
			free(debug_comments[i].comment_info[j]);
		}

		debug_comments[i].comment_count = 0;
	}

	free(debug_comments);
}
