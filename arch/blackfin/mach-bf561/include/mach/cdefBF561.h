/******************************************************************************
 *
 * Module Name: nsdump - table dumping routines for debug
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2015, Intel Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 */

#include <acpi/acpi.h>
#include "accommon.h"
#include "acnamesp.h"
#include <acpi/acoutput.h>

#define _COMPONENT          ACPI_NAMESPACE
ACPI_MODULE_NAME("nsdump")

/* Local prototypes */
#ifdef ACPI_OBSOLETE_FUNCTIONS
void acpi_ns_dump_root_devices(void);

static acpi_status
acpi_ns_dump_one_device(acpi_handle obj_handle,
			u32 level, void *context, void **return_value);
#endif

#if defined(ACPI_DEBUG_OUTPUT) || defined(ACPI_DEBUGGER)

static acpi_status
acpi_ns_dump_one_object_path(acpi_handle obj_handle,
			     u32 level, void *context, void **return_value);

static acpi_status
acpi_ns_get_max_depth(acpi_handle obj_handle,
		      u32 level, void *context, void **return_value);

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_print_pathname
 *
 * PARAMETERS:  num_segments        - Number of ACPI name segments
 *              pathname            - The compressed (internal) path
 *
 * RETURN:      None
 *
 * DESCRIPTION: Print an object's full namespace pathname
 *
 ******************************************************************************/

void acpi_ns_print_pathname(u32 num_segments, char *pathname)
{
	u32 i;

	ACPI_FUNCTION_NAME(ns_print_pathname);

	/* Check if debug output enabled */

	if (!ACPI_IS_DEBUG_ENABLED(ACPI_LV_NAMES, ACPI_NAMESPACE)) {
		return;
	}

	/* Print the entire name */

	ACPI_DEBUG_PRINT((ACPI_DB_NAMES, "["));

	while (num_segments) {
		for (i = 0; i < 4; i++) {
			isprint((int)pathname[i]) ?
			    acpi_os_printf("%c", pathname[i]) :
			    acpi_os_printf("?");
		}

		pathname += ACPI_NAME_SIZE;
		num_segments--;
		if (num_segments) {
			acpi_os_printf(".");
		}
	}

	acpi_os_printf("]\n");
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_dump_pathname
 *
 * PARAMETERS:  handle              - Object
 *              msg                 - Prefix message
 *              level               - Desired debug level
 *              component           - Caller's component ID
 *
 * RETURN:      None
 *
 * DESCRIPTION: Print an object's full namespace pathname
 *              Manages allocation/freeing of a pathname buffer
 *
 ******************************************************************************/

void
acpi_ns_dump_pathname(acpi_handle handle, char *msg, u32 level, u32 component)
{

	ACPI_FUNCTION_TRACE(ns_dump_pathname);

	/* Do this only if the requested debug level and component are enabled */

	if (!ACPI_IS_DEBUG_ENABLED(level, component)) {
		return_VOID;
	}

	/* Convert handle to a full pathname and print it (with supplied message) */

	acpi_ns_print_node_pathname(handle, msg);
	acpi_os_printf("\n");
	return_VOID;
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_dump_one_object
 *
 * PARAMETERS:  obj_handle          - Node to be dumped
 *              level               - Nesting level of the handle
 *              context             - Passed into walk_namespace
 *              return_value        - Not used
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Dump a single Node
 *              This procedure is a user_function called by acpi_ns_walk_namespace.
 *
 ******************************************************************************/

acpi_status
acpi_ns_dump_one_object(acpi_handle obj_handle,
			u32 level, void *context, void **return_value)
{
	struct acpi_walk_info *info = (struct acpi_walk_info *)context;
	struct acpi_namespace_node *this_node;
	union acpi_operand_object *obj_desc = NULL;
	acpi_object_type obj_type;
	acpi_object_type type;
	u32 bytes_to_dump;
	u32 dbg_level;
	u32 i;

	ACPI_FUNCTION_NAME(ns_dump_one_object);

	/* Is output enabled? */

	if (!(acpi_dbg_level & info->debug_level)) {
		return (AE_OK);
	}

	if (!obj_handle) {
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Null object handle\n"));
		return (AE_OK);
	}

	this_node = acpi_ns_validate_handle(obj_handle);
	if (!this_node) {
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Invalid object handle %p\n",
				  obj_handle));
		return (AE_OK);
	}

	type = this_node->type;

	/* Check if the owner matches */

	if ((info->owner_id != ACPI_OWNER_ID_MAX) &&
	    (info->owner_id != this_node->owner_id)) {
		return (AE_OK);
	}

	if (!(info->display_type & ACPI_DISPLAY_SHORT)) {

		/* Indent the object according to the level */

		acpi_os_printf("%2d%*s", (u32) level - 1, (int)level * 2, " ");

		/* Check the node type and name */

		if (type > ACPI_TYPE_LOCAL_MAX) {
			ACPI_WARNING((AE_INFO,
				      "Invalid ACPI Object Type 0x%08X", type));
		}

		acpi_os_printf("%4.4s", acpi_ut_get_node_name(this_node));
	}

	/* Now we can print out the pertinent information */

	acpi_os_printf(" %-12s %p %2.2X ",
		       acpi_ut_get_type_name(type), this_node,
		       this_node->owner_id);

	dbg_level = acpi_dbg_level;
	acpi_dbg_level = 0;
	obj_desc = acpi_ns_get_attached_object(this_node);
	acpi_dbg_level = dbg_level;

	/* Temp nodes are those nodes created by a control method */

	if (this_node->flags & ANOBJ_TEMPORARY) {
		acpi_os_printf("(T) ");
	}

	switch (info->display_type & ACPI_DISPLAY_MASK) {
	case ACPI_DISPLAY_SUMMARY:

		if (!obj_desc) {

			/* No attached object. Some types should always have an object */

			switch (type) {
			case ACPI_TYPE_INTEGER:
			case ACPI_TYPE_PACKAGE:
			case ACPI_TYPE_BUFFER:
			case ACPI_TYPE_STRING:
			case ACPI_TYPE_METHOD:

				acpi_os_printf("<No attached object>");
				break;

			default:

				break;
			}

			acpi_os_printf("\n");
			return (AE_OK);
		}

		switch (type) {
		case ACPI_TYPE_PROCESSOR:

			acpi_os_printf("ID %02X Len %02X Addr %8.8X%8.8X\n",
				       obj_desc->processor.proc_id,
				       obj_desc->processor.length,
				       ACPI_FORMAT_UINT64(obj_desc->processor.
							  address));
			break;

		case ACPI_TYPE_DEVICE:

			acpi_os_printf("Notify Object: %p\n", obj_desc);
			break;

		case ACPI_TYPE_METHOD:

			acpi_os_printf("Args %X Len %.4X Aml %p\n",
				       (u32) obj_desc->method.param_count,
				       obj_desc->method.aml_length,
				       obj_desc->method.aml_start);
			break;

		case ACPI_TYPE_INTEGER:

			acpi_os_printf("= %8.8X%8.8X\n",
				       ACPI_FORMAT_UINT64(obj_desc->integer.
							  value));
			break;

		case ACPI_TYPE_PACKAGE:

			if (obj_desc->common.flags & AOPOBJ_DATA_VALID) {
				acpi_os_printf("Elements %.2X\n",
					       obj_desc->package.count);
			} else {
				acpi_os_printf("[Length not yet evaluated]\n");
			}
			break;

		case ACPI_TYPE_BUFFER:

			if (obj_desc->common.flags & AOPOBJ_DATA_VALID) {
				acpi_os_printf("Len %.2X",
					       obj_desc->buffer.length);

				/* Dump some of the buffer */

				if (obj_desc->buffer.length > 0) {
					acpi_os_printf(" =");
					for (i = 0;
					     (i < obj_desc->buffer.length
					      && i < 12); i++) {
						acpi_os_printf(" %.2hX",
							       obj_desc->buffer.
							       pointer[i]);
					}
				}
				acpi_os_printf("\n");
			} else {
				acpi_os_printf("[Length not yet evaluated]\n");
			}
			break;

		case ACPI_TYPE_STRING:

			acpi_os_printf("Len %.2X ", obj_desc->string.length);
			acpi_ut_print_string(obj_desc->string.pointer, 32);
			acpi_os_printf("\n");
			break;

		case ACPI_TYPE_REGION:

			acpi_os_printf("[%s]",
				       acpi_ut_get_region_name(obj_desc->region.
							       space_id));
			if (obj_desc->region.flags & AOPOBJ_DATA_VALID) {
				acpi_os_printf(" Addr %8.8X%8.8X Len %.4X\n",
					       ACPI_FORMAT_UINT64(obj_desc->
								  region.
								  address),
					       obj_desc->region.length);
			} else {
				acpi_os_printf
				    (" [Address/Length not yet evaluated]\n");
			}
			break;

		case ACPI_TYPE_LOCAL_REFERENCE:

			acpi_os_printf("[%s]\n",
				       acpi_ut_get_reference_name(obj_desc));
			break;

		case ACPI_TYPE_BUFFER_FIELD:

			if (obj_desc->buffer_field.buffer_obj &&
			    obj_desc->buffer_field.buffer_obj->buffer.node) {
				acpi_os_printf("Buf [%4.4s]",
					       acpi_ut_get_node_name(obj_desc->
								     buffer_field.
								     buffer_obj->
								     buffer.
								     node));
			}
			break;

		case ACPI_TYPE_LOCAL_REGION_FIELD:

			acpi_os_printf("Rgn [%4.4s]",
				       acpi_ut_get_node_name(obj_desc->
							     common_field.
							     region_obj->region.
							     node));
			break;

		case ACPI_TYPE_LOCAL_BANK_FIELD:

			acpi_os_printf("Rgn [%4.4s] Bnk [%4.4s]",
				       acpi_ut_get_node_name(obj_desc->
							     common_field.
							     region_obj->region.
							     node),
				       acpi_ut_get_node_name(obj_desc->
							     bank_field.
							     bank_obj->
							     common_field.
							     node));
			break;

		case ACPI_TYPE_LOCAL_INDEX_FIELD:

			acpi_os_printf("Idx [%4.4s] Dat [%4.4s]",
				       acpi_ut_get_node_name(obj_desc->
							     index_field.
							     index_obj->
							     common_field.node),
				       acpi_ut_get_node_name(obj_desc->
							     index_field.
							     data_obj->
							     common_field.
							     node));
			break;

		case ACPI_TYPE_LOCAL_ALIAS:
		case ACPI_TYPE_LOCAL_METHOD_ALIAS:

			acpi_os_printf("Target %4.4s (%p)\n",
				       acpi_ut_get_node_name(obj_desc),
				       obj_desc);
			break;

		default:

			acpi_os_printf("Object %p\n", obj_desc);
			break;
		}

		/* Common field handling */

		switch (type) {
		case ACPI_TYPE_BUFFER_FIELD:
		case ACPI_TYPE_LOCAL_REGION_FIELD:
		case ACPI_TYPE_LOCAL_BANK_FIELD:
		case ACPI_TYPE_LOCAL_INDEX_FIELD:

			acpi_os_printf(" Off %.3X Len %.2X Acc %.2hd\n",
				       (obj_desc->common_field.
					base_byte_offset * 8)
				       +
				       obj_desc->common_field.
				       start_field_bit_offset,
				       obj_desc->common_field.bit_length,
				       obj_desc->common_field.
				       access_byte_width);
			break;

		default:

			break;
		}
		break;

	case ACPI_DISPLAY_OBJECTS:

		acpi_os_printf("O:%p", obj_desc);
		if (!obj_desc) {

			/* No attached object, we are done */

			acpi_os_printf("\n");
			return (AE_OK);
		}

		acpi_os_printf("(R%u)", obj_desc->common.reference_count);

		switch (type) {
		case ACPI_TYPE_METHOD:

			/* Name is a Method and its AML offset/length are set */

			acpi_os_printf(" M:%p-%X\n", obj_desc->method.aml_start,
				       obj_desc->method.aml_length);
			break;

		case ACPI_TYPE_INTEGER:

			acpi_os_printf(" I:%8.8X8.8%X\n",
				       ACPI_FORMAT_UINT64(obj_desc->integer.
							  value));
			break;

		case ACPI_TYPE_STRING:

			acpi_os_printf(" S:%p-%X\n", obj_desc->string.pointer,
				       obj_desc->string.length);
			break;

		case ACPI_TYPE_BUFFER:

			acpi_os_printf(" B:%p-%X\n", obj_desc->buffer.pointer,
				       obj_desc->buffer.length);
			break;

		default:

			acpi_os_printf("\n");
			break;
		}
		break;

	default:
		acpi_os_printf("\n");
		break;
	}

	/* If debug turned off, done */

	if (!(acpi_dbg_level & ACPI_LV_VALUES)) {
		return (AE_OK);
	}

	/* If there is an attached object, display it */

	dbg_level = acpi_dbg_level;
	acpi_dbg_level = 0;
	obj_desc = acpi_ns_get_attached_object(this_node);
	acpi_dbg_level = dbg_level;

	/* Dump attached objects */

	while (obj_desc) {
		obj_type = ACPI_TYPE_INVALID;
		acpi_os_printf("Attached Object %p: ", obj_desc);

		/* Decode the type of attached object and dump the contents */

		switch (ACPI_GET_DESCRIPTOR_TYPE(obj_desc)) {
		case ACPI_DESC_TYPE_NAMED:

			acpi_os_printf("(Ptr to Node)\n");
			bytes_to_dump = sizeof(struct acpi_namespace_node);
			ACPI_DUMP_BUFFER(obj_desc, bytes_to_dump);
			break;

		case ACPI_DESC_TYPE_OPERAND:

			obj_type = obj_desc->common.type;

			if (obj_type > ACPI_TYPE_LOCAL_MAX) {
				acpi_os_printf
				    ("(Pointer to ACPI Object type %.2X [UNKNOWN])\n",
				     obj_type);
				bytes_to_dump = 32;
			} else {
				acpi_os_printf
				    ("(Pointer to ACPI Object type %.2X [%s])\n",
				     obj_type, acpi_ut_get_type_name(obj_type));
				bytes_to_dump =
				    sizeof(union acpi_operand_object);
			}

			ACPI_DUMP_BUFFER(obj_desc, bytes_to_dump);
			break;

		default:

			break;
		}

		/* If value is NOT an internal object, we are done */

		if (ACPI_GET_DESCRIPTOR_TYPE(obj_desc) !=
		    ACPI_DESC_TYPE_OPERAND) {
			goto cleanup;
		}

		/* Valid object, get the pointer to next level, if any */

		switch (obj_type) {
		case ACPI_TYPE_BUFFER:
		case ACPI_TYPE_STRING:
			/*
			 * NOTE: takes advantage of common fields between string/buffer
			 */
			bytes_to_dump = obj_desc->string.length;
			obj_desc = (void *)obj_desc->string.pointer;
			acpi_os_printf("(Buffer/String pointer %p length %X)\n",
				       obj_desc, bytes_to_dump);
			ACPI_DUMP_BUFFER(obj_desc, bytes_to_dump);
			goto cleanup;

		case ACPI_TYPE_BUFFER_FIELD:

			obj_desc =
			    (union acpi_operand_object *)obj_desc->buffer_field.
			    buffer_obj;
			break;

		case ACPI_TYPE_PACKAGE:

			obj_desc = (void *)obj_desc->package.elements;
			break;

		case ACPI_TYPE_METHOD:

			obj_desc = (void *)obj_desc->method.aml_start;
			break;

		case ACPI_TYPE_LOCAL_REGION_FIELD:

			obj_desc = (void *)obj_desc->field.region_obj;
			break;

		case ACPI_TYPE_LOCAL_BANK_FIELD:

			obj_desc = (void *)obj_desc->bank_field.region_obj;
			break;

		case ACPI_TYPE_LOCAL_INDEX_FIELD:

			obj_desc = (void *)obj_desc->index_field.index_obj;
			break;

		default:

			goto cleanup;
		}

		obj_type = ACPI_TYPE_INVALID;	/* Terminate loop after next pass */
	}

cleanup:
	acpi_os_printf("\n");
	return (AE_OK);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_dump_objects
 *
 * PARAMETERS:  type                - Object type to be dumped
 *              display_type        - 0 or ACPI_DISPLAY_SUMMARY
 *              max_depth           - Maximum depth of dump. Use ACPI_UINT32_MAX
 *                                    for an effectively unlimited depth.
 *              owner_id            - Dump only objects owned by this ID. Use
 *                                    ACPI_UINT32_MAX to match all owners.
 *              start_handle        - Where in namespace to start/end search
 *
 * RETURN:      None
 *
 * DESCRIPTION: Dump typed objects within the loaded namespace. Uses
 *              acpi_ns_walk_namespace in conjunction with acpi_ns_dump_one_object.
 *
 ******************************************************************************/

void
acpi_ns_dump_objects(acpi_object_type type,
		     u8 display_type,
		     u32 max_depth,
		     acpi_owner_id owner_id, acpi_handle start_handle)
{
	struct acpi_walk_info info;
	acpi_status status;

	ACPI_FUNCTION_ENTRY();

	/*
	 * Just lock the entire namespace for the duration of the dump.
	 * We don't want any changes to the namespace during this time,
	 * especially the temporary nodes since we are going to display
	 * them also.
	 */
	status = acpi_ut_acquire_mutex(ACPI_MTX_NAMESPACE);
	if (ACPI_FAILURE(status)) {
		acpi_os_printf("Could not acquire namespace mutex\n");
		return;
	}

	info.debug_level = ACPI_LV_TABLES;
	info.owner_id = owner_id;
	info.display_type = display_type;

	(void)acpi_ns_walk_namespace(type, start_handle, max_depth,
				     ACPI_NS_WALK_NO_UNLOCK |
				     ACPI_NS_WALK_TEMP_NODES,
				     acpi_ns_dump_one_object, NULL,
				     (void *)&info, NULL);

	(void)acpi_ut_release_mutex(ACPI_MTX_NAMESPACE);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_dump_one_object_path, acpi_ns_get_max_depth
 *
 * PARAMETERS:  obj_handle          - Node to be dumped
 *              level               - Nesting level of the handle
 *              context             - Passed into walk_namespace
 *              return_value        - Not used
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Dump the full pathname to a namespace object. acp_ns_get_max_depth
 *              computes the maximum nesting depth in the namespace tree, in
 *              order to simplify formatting in acpi_ns_dump_one_object_path.
 *              These procedures are user_functions called by acpi_ns_walk_namespace.
 *
 ******************************************************************************/

static acpi_status
acpi_ns_dump_one_object_path(acpi_handle obj_handle,
			     u32 level, void *context, void **return_value)
{
	u32 max_level = *((u32 *)context);
	char *pathname;
	struct acpi_namespace_node *node;
	int path_indent;

	if (!obj_handle) {
		return (AE_OK);
	}

	node = acpi_ns_validate_handle(obj_handle);
	if (!node) {

		/* Ignore bad node during namespace walk */

		return (AE_OK);
	}

	pathname = acpi_ns_get_external_pathname(node);

	path_indent = 1;
	if (level <= max_level) {
		path_indent = max_level - level + 1;
	}

	acpi_os_printf("%2d%*s%-12s%*s",
		       level, level, " ", acpi_ut_get_type_name(node->type),
		       path_indent, " ");

	acpi_os_printf("%s\n", &pathname[1]);
	ACPI_FREE(pathname);
	return (AE_OK);
}

static acpi_status
acpi_ns_get_max_depth(acpi_handle obj_handle,
		      u32 level, void *context, void **return_value)
{
	u32 *max_level = (u32 *)context;

	if (level > *max_level) {
		*max_level = level;
	}
	return (AE_OK);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_dump_object_paths
 *
 * PARAMETERS:  type                - Object type to be dumped
 *              display_type        - 0 or ACPI_DISPLAY_SUMMARY
 *              max_depth           - Maximum depth of dump. Use ACPI_UINT32_MAX
 *                                    for an effectively unlimited depth.
 *              owner_id            - Dump only objects owned by this ID. Use
 *                                    ACPI_UINT32_MAX to match all owners.
 *              start_handle        - Where in namespace to start/end search
 *
 * RETURN:      None
 *
 * DESCRIPTION: Dump full object pathnames within the loaded namespace. Uses
 *              acpi_ns_walk_namespace in conjunction with acpi_ns_dump_one_object_path.
 *
 ******************************************************************************/

void
acpi_ns_dump_object_paths(acpi_object_type type,
			  u8 display_type,
			  u32 max_depth,
			  acpi_owner_id owner_id, acpi_handle start_handle)
{
	acpi_status status;
	u32 max_level = 0;

	ACPI_FUNCTION_ENTRY();

	/*
	 * Just lock the entire namespace for the duration of the dump.
	 * We don't want any changes to the namespace during this time,
	 * especially the temporary nodes since we are going to display
	 * them also.
	 */
	status = acpi_ut_acquire_mutex(ACPI_MTX_NAMESPACE);
	if (ACPI_FAILURE(status)) {
		acpi_os_printf("Could not acquire namespace mutex\n");
		return;
	}

	/* Get the max depth of the namespace tree, for formatting later */

	(void)acpi_ns_walk_namespace(type, start_handle, max_depth,
				     ACPI_NS_WALK_NO_UNLOCK |
				     ACPI_NS_WALK_TEMP_NODES,
				     acpi_ns_get_max_depth, NULL,
				     (void *)&max_level, NULL);

	/* Now dump the entire namespace */

	(void)acpi_ns_walk_namespace(type, start_handle, max_depth,
				     ACPI_NS_WALK_NO_UNLOCK |
				     ACPI_NS_WALK_TEMP_NODES,
				     acpi_ns_dump_one_object_path, NULL,
				     (void *)&max_level, NULL);

	(void)acpi_ut_release_mutex(ACPI_MTX_NAMESPACE);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_dump_entry
 *
 * PARAMETERS:  handle              - Node to be dumped
 *              debug_level         - Output level
 *
 * RETURN:      None
 *
 * DESCRIPTION: Dump a single Node
 *
 ******************************************************************************/

void acpi_ns_dump_entry(acpi_handle handle, u32 debug_level)
{
	struct acpi_walk_info info;

	ACPI_FUNCTION_ENTRY();

	info.debug_level = debug_level;
	info.owner_id = ACPI_OWNER_ID_MAX;
	info.display_type = ACPI_DISPLAY_SUMMARY;

	(void)acpi_ns_dump_one_object(handle, 1, &info, NULL);
}

#ifdef ACPI_ASL_COMPILER
/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_dump_tables
 *
 * PARAMETERS:  search_base         - Root of subtree to be dumped, or
 *                                    NS_ALL to dump the entire namespace
 *              max_depth           - Maximum depth of dump. Use INT_MAX
 *                                    for an effectively unlimited depth.
 *
 * RETURN:      None
 *
 * DESCRIPTION: Dump the name space, or a portion of it.
 *
 ******************************************************************************/

void acpi_ns_dump_tables(acpi_handle search_base, u32 max_depth)
{
	acpi_handle search_handle = search_base;

	ACPI_FUNCTION_TRACE(ns_dump_tables);

	if (!acpi_gbl_root_node) {
		/*
		 * If the name space has not been initialized,
		 * there is nothing to dump.
		 */
		ACPI_DEBUG_PRINT((ACPI_DB_TABLES,
				  "namespace not initialized!\n"));
		return_VOID;
	}

	if (ACPI_NS_ALL == search_base) {

		/* Entire namespace */

		search_handle = acpi_gbl_root_node;
		ACPI_DEBUG_PRINT((ACPI_DB_TABLES, "\\\n"));
	}

	acpi_ns_dump_objects(ACPI_TYPE_ANY, ACPI_DISPLAY_OBJECTS, max_depth,
			     ACPI_OWNER_ID_MAX, search_handle);
	return_VOID;
}
#endif
#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    /*******************************************************************************
 *
 * Module Name: nseval - Object evaluation, includes control method execution
 *
 ******************************************************************************/

/*
 * Copyright (C) 2000 - 2015, Intel Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 */

#include <acpi/acpi.h>
#include "accommon.h"
#include "acparser.h"
#include "acinterp.h"
#include "acnamesp.h"

#define _COMPONENT          ACPI_NAMESPACE
ACPI_MODULE_NAME("nseval")

/* Local prototypes */
static void
acpi_ns_exec_module_code(union acpi_operand_object *method_obj,
			 struct acpi_evaluate_info *info);

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_evaluate
 *
 * PARAMETERS:  info            - Evaluation info block, contains these fields
 *                                and more:
 *                  prefix_node     - Prefix or Method/Object Node to execute
 *                  relative_path   - Name of method to execute, If NULL, the
 *                                    Node is the object to execute
 *                  parameters      - List of parameters to pass to the method,
 *                                    terminated by NULL. Params itself may be
 *                                    NULL if no parameters are being passed.
 *                  parameter_type  - Type of Parameter list
 *                  return_object   - Where to put method's return value (if
 *                                    any). If NULL, no value is returned.
 *                  flags           - ACPI_IGNORE_RETURN_VALUE to delete return
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute a control method or return the current value of an
 *              ACPI namespace object.
 *
 * MUTEX:       Locks interpreter
 *
 ******************************************************************************/

acpi_status acpi_ns_evaluate(struct acpi_evaluate_info *info)
{
	acpi_status status;

	ACPI_FUNCTION_TRACE(ns_evaluate);

	if (!info) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	if (!info->node) {
		/*
		 * Get the actual namespace node for the target object if we
		 * need to. Handles these cases:
		 *
		 * 1) Null node, valid pathname from root (absolute path)
		 * 2) Node and valid pathname (path relative to Node)
		 * 3) Node, Null pathname
		 */
		status =
		    acpi_ns_get_node(info->prefix_node, info->relative_pathname,
				     ACPI_NS_NO_UPSEARCH, &info->node);
		if (ACPI_FAILURE(status)) {
			return_ACPI_STATUS(status);
		}
	}

	/*
	 * For a method alias, we must grab the actual method node so that
	 * proper scoping context will be established before execution.
	 */
	if (acpi_ns_get_type(info->node) == ACPI_TYPE_LOCAL_METHOD_ALIAS) {
		info->node =
		    ACPI_CAST_PTR(struct acpi_namespace_node,
				  info->node->object);
	}

	/* Complete the info block initialization */

	info->return_object = NULL;
	info->node_flags = info->node->flags;
	info->obj_desc = acpi_ns_get_attached_object(info->node);

	ACPI_DEBUG_PRINT((ACPI_DB_NAMES, "%s [%p] Value %p\n",
			  info->relative_pathname, info->node,
			  acpi_ns_get_attached_object(info->node)));

	/* Get info if we have a predefined name (_HID, etc.) */

	info->predefined =
	    acpi_ut_match_predefined_method(info->node->name.ascii);

	/* Get the full pathname to the object, for use in warning messages */

	info->full_pathname = acpi_ns_get_external_pathname(info->node);
	if (!info->full_pathname) {
		return_ACPI_STATUS(AE_NO_MEMORY);
	}

	/* Count the number of arguments being passed in */

	info->param_count = 0;
	if (info->parameters) {
		while (info->parameters[info->param_count]) {
			info->param_count++;
		}

		/* Warn on impossible argument count */

		if (info->param_count > ACPI_METHOD_NUM_ARGS) {
			ACPI_WARN_PREDEFINED((AE_INFO, info->full_pathname,
					      ACPI_WARN_ALWAYS,
					      "Excess arguments (%u) - using only %u",
					      info->param_count,
					      ACPI_METHOD_NUM_ARGS));

			info->param_count = ACPI_METHOD_NUM_ARGS;
		}
	}

	/*
	 * For predefined names: Check that the declared argument count
	 * matches the ACPI spec -- otherwise this is a BIOS error.
	 */
	acpi_ns_check_acpi_compliance(info->full_pathname, info->node,
				      info->predefined);

	/*
	 * For all names: Check that the incoming argument count for
	 * this method/object matches the actual ASL/AML definition.
	 */
	acpi_ns_check_argument_count(info->full_pathname, info->node,
				     info->param_count, info->predefined);

	/* For predefined names: Typecheck all incoming arguments */

	acpi_ns_check_argument_types(info);

	/*
	 * Three major evaluation cases:
	 *
	 * 1) Object types that cannot be evaluated by definition
	 * 2) The object is a control method -- execute it
	 * 3) The object is not a method -- just return it's current value
	 */
	switch (acpi_ns_get_type(info->node)) {
	case ACPI_TYPE_DEVICE:
	case ACPI_TYPE_EVENT:
	case ACPI_TYPE_MUTEX:
	case ACPI_TYPE_REGION:
	case ACPI_TYPE_THERMAL:
	case ACPI_TYPE_LOCAL_SCOPE:
		/*
		 * 1) Disallow evaluation of certain object types. For these,
		 *    object evaluation is undefined and not supported.
		 */
		ACPI_ERROR((AE_INFO,
			    "%s: Evaluation of object type [%s] is not supported",
			    info->full_pathname,
			    acpi_ut_get_type_name(info->node->type)));

		status = AE_TYPE;
		goto cleanup;

	case ACPI_TYPE_METHOD:
		/*
		 * 2) Object is a control method - execute it
		 */

		/* Verify that there is a method object associated with this node */

		if (!info->obj_desc) {
			ACPI_ERROR((AE_INFO,
				    "%s: Method has no attached sub-object",
				    info->full_pathname));
			status = AE_NULL_OBJECT;
			goto cleanup;
		}

		ACPI_DEBUG_PRINT((ACPI_DB_EXEC,
				  "**** Execute method [%s] at AML address %p length %X\n",
				  info->full_pathname,
				  info->obj_desc->method.aml_start + 1,
				  info->obj_desc->method.aml_length - 1));

		/*
		 * Any namespace deletion must acquire both the namespace and
		 * interpreter locks to ensure that no thread is using the portion of
		 * the namespace that is being deleted.
		 *
		 * Execute the method via the interpreter. The interpreter is locked
		 * here before calling into the AML parser
		 */
		acpi_ex_enter_interpreter();
		status = acpi_ps_execute_method(info);
		acpi_ex_exit_interpreter();
		break;

	default:
		/*
		 * 3) All other non-method objects -- get the current object value
		 */

		/*
		 * Some objects require additional resolution steps (e.g., the Node
		 * may be a field that must be read, etc.) -- we can't just grab
		 * the object out of the node.
		 *
		 * Use resolve_node_to_value() to get the associated value.
		 *
		 * NOTE: we can get away with passing in NULL for a walk state because
		 * the Node is guaranteed to not be a reference to either a method
		 * local or a method argument (because this interface is never called
		 * from a running method.)
		 *
		 * Even though we do not directly invoke the interpreter for object
		 * resolution, we must lock it because we could access an op_region.
		 * The op_region access code assumes that the interpreter is locked.
		 */
		acpi_ex_enter_interpreter();

		/* TBD: resolve_node_to_value has a strange interface, fix */

		info->return_object =
		    ACPI_CAST_PTR(union acpi_operand_object, info->node);

		status =
		    acpi_ex_resolve_node_to_value(ACPI_CAST_INDIRECT_PTR
						  (struct acpi_namespace_node,
						   &info->return_object), NULL);
		acpi_ex_exit_interpreter();

		if (ACPI_FAILURE(status)) {
			info->return_object = NULL;
			goto cleanup;
		}

		ACPI_DEBUG_PRINT((ACPI_DB_NAMES, "Returned object %p [%s]\n",
				  info->return_object,
				  acpi_ut_get_object_type_name(info->
							       return_object)));

		status = AE_CTRL_RETURN_VALUE;	/* Always has a "return value" */
		break;
	}

	/*
	 * For predefined names, check the return value against the ACPI
	 * specification. Some incorrect return value types are repaired.
	 */
	(void)acpi_ns_check_return_value(info->node, info, info->param_count,
					 status, &info->return_object);

	/* Check if there is a return value that must be dealt with */

	if (status == AE_CTRL_RETURN_VALUE) {

		/* If caller does not want the return value, delete it */

		if (info->flags & ACPI_IGNORE_RETURN_VALUE) {
			acpi_ut_remove_reference(info->return_object);
			info->return_object = NULL;
		}

		/* Map AE_CTRL_RETURN_VALUE to AE_OK, we are done with it */

		status = AE_OK;
	} else if (ACPI_FAILURE(status)) {

		/* If return_object exists, delete it */

		if (info->return_object) {
			acpi_ut_remove_reference(info->return_object);
			info->return_object = NULL;
		}
	}

	ACPI_DEBUG_PRINT((ACPI_DB_NAMES,
			  "*** Completed evaluation of object %s ***\n",
			  info->relative_pathname));

cleanup:
	/*
	 * Namespace was unlocked by the handling acpi_ns* function, so we
	 * just free the pathname and return
	 */
	ACPI_FREE(info->full_pathname);
	info->full_pathname = NULL;
	return_ACPI_STATUS(status);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_exec_module_code_list
 *
 * PARAMETERS:  None
 *
 * RETURN:      None. Exceptions during method execution are ignored, since
 *              we cannot abort a table load.
 *
 * DESCRIPTION: Execute all elements of the global module-level code list.
 *              Each element is executed as a single control method.
 *
 ******************************************************************************/

void acpi_ns_exec_module_code_list(void)
{
	union acpi_operand_object *prev;
	union acpi_operand_object *next;
	struct acpi_evaluate_info *info;
	u32 method_count = 0;

	ACPI_FUNCTION_TRACE(ns_exec_module_code_list);

	/* Exit now if the list is empty */

	next = acpi_gbl_module_code_list;
	if (!next) {
		return_VOID;
	}

	/* Allocate the evaluation information block */

	info = ACPI_ALLOCATE(sizeof(struct acpi_evaluate_info));
	if (!info) {
		return_VOID;
	}

	/* Walk the list, executing each "method" */

	while (next) {
		prev = next;
		next = next->method.mutex;

		/* Clear the link field and execute the method */

		prev->method.mutex = NULL;
		acpi_ns_exec_module_code(prev, info);
		method_count++;

		/* Delete the (temporary) method object */

		acpi_ut_remove_reference(prev);
	}

	ACPI_INFO((AE_INFO,
		   "Executed %u blocks of module-level executable AML code",
		   method_count));

	ACPI_FREE(info);
	acpi_gbl_module_code_list = NULL;
	return_VOID;
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_exec_module_code
 *
 * PARAMETERS:  method_obj          - Object container for the module-level code
 *              info                - Info block for method evaluation
 *
 * RETURN:      None. Exceptions during method execution are ignored, since
 *              we cannot abort a table load.
 *
 * DESCRIPTION: Execute a control method containing a block of module-level
 *              executable AML code. The control method is temporarily
 *              installed to the root node, then evaluated.
 *
 ******************************************************************************/

static void
acpi_ns_exec_module_code(union acpi_operand_object *method_obj,
			 struct acpi_evaluate_info *info)
{
	union acpi_operand_object *parent_obj;
	struct acpi_namespace_node *parent_node;
	acpi_object_type type;
	acpi_status status;

	ACPI_FUNCTION_TRACE(ns_exec_module_code);

	/*
	 * Get the parent node. We cheat by using the next_object field
	 * of the method object descriptor.
	 */
	parent_node = ACPI_CAST_PTR(struct acpi_namespace_node,
				    method_obj->method.next_object);
	type = acpi_ns_get_type(parent_node);

	/*
	 * Get the region handler and save it in the method object. We may need
	 * this if an operation region declaration causes a _REG method to be run.
	 *
	 * We can't do this in acpi_ps_link_module_code because
	 * acpi_gbl_root_node->Object is NULL at PASS1.
	 */
	if ((type == ACPI_TYPE_DEVICE) && parent_node->object) {
		method_obj->method.dispatch.handler =
		    parent_node->object->device.handler;
	}

	/* Must clear next_object (acpi_ns_attach_object needs the field) */

	method_obj->method.next_object = NULL;

	/* Initialize the evaluation information block */

	memset(info, 0, sizeof(struct acpi_evaluate_info));
	info->prefix_node = parent_node;

	/*
	 * Get the currently attached parent object. Add a reference, because the
	 * ref count will be decreased when the method object is installed to
	 * the parent node.
	 */
	parent_obj = acpi_ns_get_attached_object(parent_node);
	if (parent_obj) {
		acpi_ut_add_reference(parent_obj);
	}

	/* Install the method (module-level code) in the parent node */

	status = acpi_ns_attach_object(parent_node, method_obj,
				       ACPI_TYPE_METHOD);
	if (ACPI_FAILURE(status)) {
		goto exit;
	}

	/* Execute the parent node as a control method */

	status = acpi_ns_evaluate(info);

	ACPI_DEBUG_PRINT((ACPI_DB_INIT_NAMES,
			  "Executed module-level code at %p\n",
			  method_obj->method.aml_start));

	/* Delete a possible implicit return value (in slack mode) */

	if (info->return_object) {
		acpi_ut_remove_reference(info->return_object);
	}

	/* Detach the temporary method object */

	acpi_ns_detach_object(parent_node);

	/* Restore the original parent object */

	if (parent_obj) {
		status = acpi_ns_attach_object(parent_node, parent_obj, type);
	} else {
		parent_node->type = (u8)type;
	}

exit:
	if (parent_obj) {
		acpi_ut_remove_reference(parent_obj);
	}
	return_VOID;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   /******************************************************************************
 *
 * Module Name: nsinit - namespace initialization
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2015, Intel Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 */

#include <acpi/acpi.h>
#include "accommon.h"
#include "acnamesp.h"
#include "acdispat.h"
#include "acinterp.h"

#define _COMPONENT          ACPI_NAMESPACE
ACPI_MODULE_NAME("nsinit")

/* Local prototypes */
static acpi_status
acpi_ns_init_one_object(acpi_handle obj_handle,
			u32 level, void *context, void **return_value);

static acpi_status
acpi_ns_init_one_device(acpi_handle obj_handle,
			u32 nesting_level, void *context, void **return_value);

static acpi_status
acpi_ns_find_ini_methods(acpi_handle obj_handle,
			 u32 nesting_level, void *context, void **return_value);

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_initialize_objects
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Walk the entire namespace and perform any necessary
 *              initialization on the objects found therein
 *
 ******************************************************************************/

acpi_status acpi_ns_initialize_objects(void)
{
	acpi_status status;
	struct acpi_init_walk_info info;

	ACPI_FUNCTION_TRACE(ns_initialize_objects);

	ACPI_DEBUG_PRINT((ACPI_DB_DISPATCH,
			  "**** Starting initialization of namespace objects ****\n"));
	ACPI_DEBUG_PRINT_RAW((ACPI_DB_INIT,
			      "Completing Region/Field/Buffer/Package initialization:\n"));

	/* Set all init info to zero */

	memset(&info, 0, sizeof(struct acpi_init_walk_info));

	/* Walk entire namespace from the supplied root */

	status = acpi_walk_namespace(ACPI_TYPE_ANY, ACPI_ROOT_OBJECT,
				     ACPI_UINT32_MAX, acpi_ns_init_one_object,
				     NULL, &info, NULL);
	if (ACPI_FAILURE(status)) {
		ACPI_EXCEPTION((AE_INFO, status, "During WalkNamespace"));
	}

	ACPI_DEBUG_PRINT_RAW((ACPI_DB_INIT,
			      "    Initialized %u/%u Regions %u/%u Fields %u/%u "
			      "Buffers %u/%u Packages (%u nodes)\n",
			      info.op_region_init, info.op_region_count,
			      info.field_init, info.field_count,
			      info.buffer_init, info.buffer_count,
			      info.package_init, info.package_count,
			      info.object_count));

	ACPI_DEBUG_PRINT((ACPI_DB_DISPATCH,
			  "%u Control Methods found\n%u Op Regions found\n",
			  info.method_count, info.op_region_count));

	return_ACPI_STATUS(AE_OK);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_initialize_devices
 *
 * PARAMETERS:  None
 *
 * RETURN:      acpi_status
 *
 * DESCRIPTION: Walk the entire namespace and initialize all ACPI devices.
 *              This means running _INI on all present devices.
 *
 *              Note: We install PCI config space handler on region access,
 *              not here.
 *
 ******************************************************************************/

acpi_status acpi_ns_initialize_devices(void)
{
	acpi_status status;
	struct acpi_device_walk_info info;

	ACPI_FUNCTION_TRACE(ns_initialize_devices);

	/* Init counters */

	info.device_count = 0;
	info.num_STA = 0;
	info.num_INI = 0;

	ACPI_DEBUG_PRINT_RAW((ACPI_DB_INIT,
			      "Initializing Device/Processor/Thermal objects "
			      "and executing _INI/_STA methods:\n"));

	/* Tree analysis: find all subtrees that contain _INI methods */

	status = acpi_ns_walk_namespace(ACPI_TYPE_ANY, ACPI_ROOT_OBJECT,
					ACPI_UINT32_MAX, FALSE,
					acpi_ns_find_ini_methods, NULL, &info,
					NULL);
	if (ACPI_FAILURE(status)) {
		goto error_exit;
	}

	/* Allocate the evaluation information block */

	info.evaluate_info =
	    ACPI_ALLOCATE_ZEROED(sizeof(struct acpi_evaluate_info));
	if (!info.evaluate_info) {
		status = AE_NO_MEMORY;
		goto error_exit;
	}

	/*
	 * Execute the "global" _INI method that may appear at the root. This
	 * support is provided for Windows compatibility (Vista+) and is not
	 * part of the ACPI specification.
	 */
	info.evaluate_info->prefix_node = acpi_gbl_root_node;
	info.evaluate_info->relative_pathname = METHOD_NAME__INI;
	info.evaluate_info->parameters = NULL;
	info.evaluate_info->flags = ACPI_IGNORE_RETURN_VALUE;

	status = acpi_ns_evaluate(info.evaluate_info);
	if (ACPI_SUCCESS(status)) {
		info.num_INI++;
	}

	/* Walk namespace to execute all _INIs on present devices */

	status = acpi_ns_walk_namespace(ACPI_TYPE_ANY, ACPI_ROOT_OBJECT,
					ACPI_UINT32_MAX, FALSE,
					acpi_ns_init_one_device, NULL, &info,
					NULL);

	/*
	 * Any _OSI requests should be completed by now. If the BIOS has
	 * requested any Windows OSI strings, we will always truncate
	 * I/O addresses to 16 bits -- for Windows compatibility.
	 */
	if (acpi_gbl_osi_data >= ACPI_OSI_WIN_2000) {
		acpi_gbl_truncate_io_addresses = TRUE;
	}

	ACPI_FREE(info.evaluate_info);
	if (ACPI_FAILURE(status)) {
		goto error_exit;
	}

	ACPI_DEBUG_PRINT_RAW((ACPI_DB_INIT,
			      "    Executed %u _INI methods requiring %u _STA executions "
			      "(examined %u objects)\n",
			      info.num_INI, info.num_STA, info.device_count));

	return_ACPI_STATUS(status);

error_exit:
	ACPI_EXCEPTION((AE_INFO, status, "During device initialization"));
	return_ACPI_STATUS(status);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_init_one_object
 *
 * PARAMETERS:  obj_handle      - Node
 *              level           - Current nesting level
 *              context         - Points to a init info struct
 *              return_value    - Not used
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Callback from acpi_walk_namespace. Invoked for every object
 *              within the  namespace.
 *
 *              Currently, the only objects that require initialization are:
 *              1) Methods
 *              2) Op Regions
 *
 ******************************************************************************/

static acpi_status
acpi_ns_init_one_object(acpi_handle obj_handle,
			u32 level, void *context, void **return_value)
{
	acpi_object_type type;
	acpi_status status = AE_OK;
	struct acpi_init_walk_info *info =
	    (struct acpi_init_walk_info *)context;
	struct acpi_namespace_node *node =
	    (struct acpi_namespace_node *)obj_handle;
	union acpi_operand_object *obj_desc;

	ACPI_FUNCTION_NAME(ns_init_one_object);

	info->object_count++;

	/* And even then, we are only interested in a few object types */

	type = acpi_ns_get_type(obj_handle);
	obj_desc = acpi_ns_get_attached_object(node);
	if (!obj_desc) {
		return (AE_OK);
	}

	/* Increment counters for object types we are looking for */

	switch (type) {
	case ACPI_TYPE_REGION:

		info->op_region_count++;
		break;

	case ACPI_TYPE_BUFFER_FIELD:

		info->field_count++;
		break;

	case ACPI_TYPE_LOCAL_BANK_FIELD:

		info->field_count++;
		break;

	case ACPI_TYPE_BUFFER:

		info->buffer_count++;
		break;

	case ACPI_TYPE_PACKAGE:

		info->package_count++;
		break;

	default:

		/* No init required, just exit now */

		return (AE_OK);
	}

	/* If the object is already initialized, nothing else to do */

	if (obj_desc->common.flags & AOPOBJ_DATA_VALID) {
		return (AE_OK);
	}

	/* Must lock the interpreter before executing AML code */

	acpi_ex_enter_interpreter();

	/*
	 * Each of these types can contain executable AML code within the
	 * declaration.
	 */
	switch (type) {
	case ACPI_TYPE_REGION:

		info->op_region_init++;
		status = acpi_ds_get_region_arguments(obj_desc);
		break;

	case ACPI_TYPE_BUFFER_FIELD:

		info->field_init++;
		status = acpi_ds_get_buffer_field_arguments(obj_desc);
		break;

	case ACPI_TYPE_LOCAL_BANK_FIELD:

		info->field_init++;
		status = acpi_ds_get_bank_field_arguments(obj_desc);
		break;

	case ACPI_TYPE_BUFFER:

		info->buffer_init++;
		status = acpi_ds_get_buffer_arguments(obj_desc);
		break;

	case ACPI_TYPE_PACKAGE:

		info->package_init++;
		status = acpi_ds_get_package_arguments(obj_desc);
		break;

	default:

		/* No other types can get here */

		break;
	}

	if (ACPI_FAILURE(status)) {
		ACPI_EXCEPTION((AE_INFO, status,
				"Could not execute arguments for [%4.4s] (%s)",
				acpi_ut_get_node_name(node),
				acpi_ut_get_type_name(type)));
	}

	/*
	 * We ignore errors from above, and always return OK, since we don't want
	 * to abort the walk on any single error.
	 */
	acpi_ex_exit_interpreter();
	return (AE_OK);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_find_ini_methods
 *
 * PARAMETERS:  acpi_walk_callback
 *
 * RETURN:      acpi_status
 *
 * DESCRIPTION: Called during namespace walk. Finds objects named _INI under
 *              device/processor/thermal objects, and marks the entire subtree
 *              with a SUBTREE_HAS_INI flag. This flag is used during the
 *              subsequent device initialization walk to avoid entire subtrees
 *              that do not contain an _INI.
 *
 ******************************************************************************/

static acpi_status
acpi_ns_find_ini_methods(acpi_handle obj_handle,
			 u32 nesting_level, void *context, void **return_value)
{
	struct acpi_device_walk_info *info =
	    ACPI_CAST_PTR(struct acpi_device_walk_info, context);
	struct acpi_namespace_node *node;
	struct acpi_namespace_node *parent_node;

	/* Keep count of device/processor/thermal objects */

	node = ACPI_CAST_PTR(struct acpi_namespace_node, obj_handle);
	if ((node->type == ACPI_TYPE_DEVICE) ||
	    (node->type == ACPI_TYPE_PROCESSOR) ||
	    (node->type == ACPI_TYPE_THERMAL)) {
		info->device_count++;
		return (AE_OK);
	}

	/* We are only looking for methods named _INI */

	if (!ACPI_COMPARE_NAME(node->name.ascii, METHOD_NAME__INI)) {
		return (AE_OK);
	}

	/*
	 * The only _INI methods that we care about are those that are
	 * present under Device, Processor, and Thermal objects.
	 */
	parent_node = node->parent;
	switch (parent_node->type) {
	case ACPI_TYPE_DEVICE:
	case ACPI_TYPE_PROCESSOR:
	case ACPI_TYPE_THERMAL:

		/* Mark parent and bubble up the INI present flag to the root */

		while (parent_node) {
			parent_node->flags |= ANOBJ_SUBTREE_HAS_INI;
			parent_node = parent_node->parent;
		}
		break;

	default:

		break;
	}

	return (AE_OK);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_init_one_device
 *
 * PARAMETERS:  acpi_walk_callback
 *
 * RETURN:      acpi_status
 *
 * DESCRIPTION: This is called once per device soon after ACPI is enabled
 *              to initialize each device. It determines if the device is
 *              present, and if so, calls _INI.
 *
 ******************************************************************************/

static acpi_status
acpi_ns_init_one_device(acpi_handle obj_handle,
			u32 nesting_level, void *context, void **return_value)
{
	struct acpi_device_walk_info *walk_info =
	    ACPI_CAST_PTR(struct acpi_device_walk_info, context);
	struct acpi_evaluate_info *info = walk_info->evaluate_info;
	u32 flags;
	acpi_status status;
	struct acpi_namespace_node *device_node;

	ACPI_FUNCTION_TRACE(ns_init_one_device);

	/* We are interested in Devices, Processors and thermal_zones only */

	device_node = ACPI_CAST_PTR(struct acpi_namespace_node, obj_handle);
	if ((device_node->type != ACPI_TYPE_DEVICE) &&
	    (device_node->type != ACPI_TYPE_PROCESSOR) &&
	    (device_node->type != ACPI_TYPE_THERMAL)) {
		return_ACPI_STATUS(AE_OK);
	}

	/*
	 * Because of an earlier namespace analysis, all subtrees that contain an
	 * _INI method are tagged.
	 *
	 * If this device subtree does not contain any _INI methods, we
	 * can exit now and stop traversing this entire subtree.
	 */
	if (!(device_node->flags & ANOBJ_SUBTREE_HAS_INI)) {
		return_ACPI_STATUS(AE_CTRL_DEPTH);
	}

	/*
	 * Run _STA to determine if this device is present and functioning. We
	 * must know this information for two important reasons (from ACPI spec):
	 *
	 * 1) We can only run _INI if the device is present.
	 * 2) We must abort the device tree walk on this subtree if the device is
	 *    not present and is not functional (we will not examine the children)
	 *
	 * The _STA method is not required to be present under the device, we
	 * assume the device is present if _STA does not exist.
	 */
	ACPI_DEBUG_EXEC(acpi_ut_display_init_pathname
			(ACPI_TYPE_METHOD, device_node, METHOD_NAME__STA));

	status = acpi_ut_execute_STA(device_node, &flags);
	if (ACPI_FAILURE(status)) {

		/* Ignore error and move on to next device */

		return_ACPI_STATUS(AE_OK);
	}

	/*
	 * Flags == -1 means that _STA was not found. In this case, we assume that
	 * the device is both present and functional.
	 *
	 * From the ACPI spec, description of _STA:
	 *
	 * "If a device object (including the processor object) does not have an
	 * _STA object, then OSPM assumes that all of the above bits are set (in
	 * other words, the device is present, ..., and functioning)"
	 */
	if (flags != ACPI_UINT32_MAX) {
		walk_info->num_STA++;
	}

	/*
	 * Examine the PRESENT and FUNCTIONING status bits
	 *
	 * Note: ACPI spec does not seem to specify behavior for the present but
	 * not functioning case, so we assume functioning if present.
	 */
	if (!(flags & ACPI_STA_DEVICE_PRESENT)) {

		/* Device is not present, we must examine the Functioning bit */

		if (flags & ACPI_STA_DEVICE_FUNCTIONING) {
			/*
			 * Device is not present but is "functioning". In this case,
			 * we will not run _INI, but we continue to examine the children
			 * of this device.
			 *
			 * From the ACPI spec, description of _STA: (note - no mention
			 * of whether to run _INI or not on the device in question)
			 *
			 * "_STA may return bit 0 clear (not present) with bit 3 set
			 * (device is functional). This case is used to indicate a valid
			 * device for which no device driver should be loaded (for example,
			 * a bridge device.) Children of this device may be present and
			 * valid. OSPM should continue enumeration below a device whose
			 * _STA returns this bit combination"
			 */
			return_ACPI_STATUS(AE_OK);
		} else {
			/*
			 * Device is not present and is not functioning. We must abort the
			 * walk of this subtree immediately -- don't look at the children
			 * of such a device.
			 *
			 * From the ACPI spec, description of _INI:
			 *
			 * "If the _STA method indicates that the device is not present,
			 * OSPM will not run the _INI and will not examine the children
			 * of the device for _INI methods"
			 */
			return_ACPI_STATUS(AE_CTRL_DEPTH);
		}
	}

	/*
	 * The device is present or is assumed present if no _STA exists.
	 * Run the _INI if it exists (not required to exist)
	 *
	 * Note: We know there is an _INI within this subtree, but it may not be
	 * under this particular device, it may be lower in the branch.
	 */
	ACPI_DEBUG_EXEC(acpi_ut_display_init_pathname
			(ACPI_TYPE_METHOD, device_node, METHOD_NAME__INI));

	memset(info, 0, sizeof(struct acpi_evaluate_info));
	info->prefix_node = device_node;
	info->relative_pathname = METHOD_NAME__INI;
	info->parameters = NULL;
	info->flags = ACPI_IGNORE_RETURN_VALUE;

	status = acpi_ns_evaluate(info);

	if (ACPI_SUCCESS(status)) {
		walk_info->num_INI++;
	}
#ifdef ACPI_DEBUG_OUTPUT
	else if (status != AE_NOT_FOUND) {

		/* Ignore error and move on to next device */

		char *scope_name = acpi_ns_get_external_pathname(info->node);

		ACPI_EXCEPTION((AE_INFO, status, "during %s._INI execution",
				scope_name));
		ACPI_FREE(scope_name);
	}
#endif

	/* Ignore errors from above */

	status = AE_OK;

	/*
	 * The _INI method has been run if present; call the Global Initialization
	 * Handler for this device.
	 */
	if (acpi_gbl_init_handler) {
		status =
		    acpi_gbl_init_handler(device_node, ACPI_INIT_DEVICE_INI);
	}

	return_ACPI_STATUS(status);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 /*******************************************************************************
 *
 * Module Name: nsnames - Name manipulation and search
 *
 ******************************************************************************/

/*
 * Copyright (C) 2000 - 2015, Intel Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 */

#include <acpi/acpi.h>
#include "accommon.h"
#include "amlcode.h"
#include "acnamesp.h"

#define _COMPONENT          ACPI_NAMESPACE
ACPI_MODULE_NAME("nsnames")

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_get_external_pathname
 *
 * PARAMETERS:  node            - Namespace node whose pathname is needed
 *
 * RETURN:      Pointer to storage containing the fully qualified name of
 *              the node, In external format (name segments separated by path
 *              separators.)
 *
 * DESCRIPTION: Used to obtain the full pathname to a namespace node, usually
 *              for error and debug statements.
 *
 ******************************************************************************/
char *acpi_ns_get_external_pathname(struct acpi_namespace_node *node)
{
	char *name_buffer;

	ACPI_FUNCTION_TRACE_PTR(ns_get_external_pathname, node);

	name_buffer = acpi_ns_get_normalized_pathname(node, FALSE);

	return_PTR(name_buffer);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_get_pathname_length
 *
 * PARAMETERS:  node        - Namespace node
 *
 * RETURN:      Length of path, including prefix
 *
 * DESCRIPTION: Get the length of the pathname string for this node
 *
 ******************************************************************************/

acpi_size acpi_ns_get_pathname_length(struct acpi_namespace_node *node)
{
	acpi_size size;

	ACPI_FUNCTION_ENTRY();

	size = acpi_ns_build_normalized_path(node, NULL, 0, FALSE);

	return (size);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_handle_to_pathname
 *
 * PARAMETERS:  target_handle           - Handle of named object whose name is
 *                                        to be found
 *              buffer                  - Where the pathname is returned
 *              no_trailing             - Remove trailing '_' for each name
 *                                        segment
 *
 * RETURN:      Status, Buffer is filled with pathname if status is AE_OK
 *
 * DESCRIPTION: Build and return a full namespace pathname
 *
 ******************************************************************************/

acpi_status
acpi_ns_handle_to_pathname(acpi_handle target_handle,
			   struct acpi_buffer * buffer, u8 no_trailing)
{
	acpi_status status;
	struct acpi_namespace_node *node;
	acpi_size required_size;

	ACPI_FUNCTION_TRACE_PTR(ns_handle_to_pathname, target_handle);

	node = acpi_ns_validate_handle(target_handle);
	if (!node) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	/* Determine size required for the caller buffer */

	required_size =
	    acpi_ns_build_normalized_path(node, NULL, 0, no_trailing);
	if (!required_size) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	/* Validate/Allocate/Clear caller buffer */

	status = acpi_ut_initialize_buffer(buffer, required_size);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/* Build the path in the caller buffer */

	(void)acpi_ns_build_normalized_path(node, buffer->pointer,
					    required_size, no_trailing);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	ACPI_DEBUG_PRINT((ACPI_DB_EXEC, "%s [%X]\n",
			  (char *)buffer->pointer, (u32) required_size));
	return_ACPI_STATUS(AE_OK);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_build_normalized_path
 *
 * PARAMETERS:  node        - Namespace node
 *              full_path   - Where the path name is returned
 *              path_size   - Size of returned path name buffer
 *              no_trailing - Remove trailing '_' from each name segment
 *
 * RETURN:      Return 1 if the AML path is empty, otherwise returning (length
 *              of pathname + 1) which means the 'FullPath' contains a trailing
 *              null.
 *
 * DESCRIPTION: Build and return a full namespace pathname.
 *              Note that if the size of 'FullPath' isn't large enough to
 *              contain the namespace node's path name, the actual required
 *              buffer length is returned, and it should be greater than
 *              'PathSize'. So callers are able to check the returning value
 *              to determine the buffer size of 'FullPath'.
 *
 ******************************************************************************/

u32
acpi_ns_build_normalized_path(struct acpi_namespace_node *node,
			      char *full_path, u32 path_size, u8 no_trailing)
{
	u32 length = 0, i;
	char name[ACPI_NAME_SIZE];
	u8 do_no_trailing;
	char c, *left, *right;
	struct acpi_namespace_node *next_node;

	ACPI_FUNCTION_TRACE_PTR(ns_build_normalized_path, node);

#define ACPI_PATH_PUT8(path, size, byte, length)    \
	do {                                            \
		if ((length) < (size))                      \
		{                                           \
			(path)[(length)] = (byte);              \
		}                                           \
		(length)++;                                 \
	} while (0)

	/*
	 * Make sure the path_size is correct, so that we don't need to
	 * validate both full_path and path_size.
	 */
	if (!full_path) {
		path_size = 0;
	}

	if (!node) {
		goto build_trailing_null;
	}

	next_node = node;
	while (next_node && next_node != acpi_gbl_root_node) {
		if (next_node != node) {
			ACPI_PATH_PUT8(full_path, path_size,
				       AML_DUAL_NAME_PREFIX, length);
		}
		ACPI_MOVE_32_TO_32(name, &next_node->name);
		do_no_trailing = no_trailing;
		for (i = 0; i < 4; i++) {
			c = name[4 - i - 1];
			if (do_no_trailing && c != '_') {
				do_no_trailing = FALSE;
			}
			if (!do_no_trailing) {
				ACPI_PATH_PUT8(full_path, path_size, c, length);
			}
		}
		next_node = next_node->parent;
	}
	ACPI_PATH_PUT8(full_path, path_size, AML_ROOT_PREFIX, length);

	/* Reverse the path string */

	if (length <= path_size) {
		left = full_path;
		right = full_path + length - 1;
		while (left < right) {
			c = *left;
			*left++ = *right;
			*right-- = c;
		}
	}

	/* Append the trailing null */

build_trailing_null:
	ACPI_PATH_PUT8(full_path, path_size, '\0', length);

#undef ACPI_PATH_PUT8

	return_UINT32(length);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_get_normalized_pathname
 *
 * PARAMETERS:  node            - Namespace node whose pathname is needed
 *              no_trailing     - Remove trailing '_' from each name segment
 *
 * RETURN:      Pointer to storage containing the fully qualified name of
 *              the node, In external format (name segments separated by path
 *              separators.)
 *
 * DESCRIPTION: Used to obtain the full pathname to a namespace node, usually
 *              for error and debug statements. All trailing '_' will be
 *              removed from the full pathname if 'NoTrailing' is specified..
 *
 ******************************************************************************/

char *acpi_ns_get_normalized_pathname(struct acpi_namespace_node *node,
				      u8 no_trailing)
{
	char *name_buffer;
	acpi_size size;

	ACPI_FUNCTION_TRACE_PTR(ns_get_normalized_pathname, node);

	/* Calculate required buffer size based on depth below root */

	size = acpi_ns_build_normalized_path(node, NULL, 0, no_trailing);
	if (!size) {
		return_PTR(NULL);
	}

	/* Allocate a buffer to be returned to caller */

	name_buffer = ACPI_ALLOCATE_ZEROED(size);
	if (!name_buffer) {
		ACPI_ERROR((AE_INFO, "Could not allocate %u bytes", (u32)size));
		return_PTR(NULL);
	}

	/* Build the path in the allocated buffer */

	(void)acpi_ns_build_normalized_path(node, name_buffer, size,
					    no_trailing);

	return_PTR(name_buffer);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         /*******************************************************************************
 *
 * Module Name: nsobject - Utilities for objects attached to namespace
 *                         table entries
 *
 ******************************************************************************/

/*
 * Copyright (C) 2000 - 2015, Intel Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 */

#include <acpi/acpi.h>
#include "accommon.h"
#include "acnamesp.h"

#define _COMPONENT          ACPI_NAMESPACE
ACPI_MODULE_NAME("nsobject")

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_attach_object
 *
 * PARAMETERS:  node                - Parent Node
 *              object              - Object to be attached
 *              type                - Type of object, or ACPI_TYPE_ANY if not
 *                                    known
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Record the given object as the value associated with the
 *              name whose acpi_handle is passed. If Object is NULL
 *              and Type is ACPI_TYPE_ANY, set the name as having no value.
 *              Note: Future may require that the Node->Flags field be passed
 *              as a parameter.
 *
 * MUTEX:       Assumes namespace is locked
 *
 ******************************************************************************/
acpi_status
acpi_ns_attach_object(struct acpi_namespace_node *node,
		      union acpi_operand_object *object, acpi_object_type type)
{
	union acpi_operand_object *obj_desc;
	union acpi_operand_object *last_obj_desc;
	acpi_object_type object_type = ACPI_TYPE_ANY;

	ACPI_FUNCTION_TRACE(ns_attach_object);

	/*
	 * Parameter validation
	 */
	if (!node) {

		/* Invalid handle */

		ACPI_ERROR((AE_INFO, "Null NamedObj handle"));
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	if (!object && (ACPI_TYPE_ANY != type)) {

		/* Null object */

		ACPI_ERROR((AE_INFO,
			    "Null object, but type not ACPI_TYPE_ANY"));
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	if (ACPI_GET_DESCRIPTOR_TYPE(node) != ACPI_DESC_TYPE_NAMED) {

		/* Not a name handle */

		ACPI_ERROR((AE_INFO, "Invalid handle %p [%s]",
			    node, acpi_ut_get_descriptor_name(node)));
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	/* Check if this object is already attached */

	if (node->object == object) {
		ACPI_DEBUG_PRINT((ACPI_DB_EXEC,
				  "Obj %p already installed in NameObj %p\n",
				  object, node));

		return_ACPI_STATUS(AE_OK);
	}

	/* If null object, we will just install it */

	if (!object) {
		obj_desc = NULL;
		object_type = ACPI_TYPE_ANY;
	}

	/*
	 * If the source object is a namespace Node with an attached object,
	 * we will use that (attached) object
	 */
	else if ((ACPI_GET_DESCRIPTOR_TYPE(object) == ACPI_DESC_TYPE_NAMED) &&
		 ((struct acpi_namespace_node *)object)->object) {
		/*
		 * Value passed is a name handle and that name has a
		 * non-null value. Use that name's value and type.
		 */
		obj_desc = ((struct acpi_namespace_node *)object)->object;
		object_type = ((struct acpi_namespace_node *)object)->type;
	}

	/*
	 * Otherwise, we will use the parameter object, but we must type
	 * it first
	 */
	else {
		obj_desc = (union acpi_operand_object *)object;

		/* Use the given type */

		object_type = type;
	}

	ACPI_DEBUG_PRINT((ACPI_DB_EXEC, "Installing %p into Node %p [%4.4s]\n",
			  obj_desc, node, acpi_ut_get_node_name(node)));

	/* Detach an existing attached object if present */

	if (node->object) {
		acpi_ns_detach_object(node);
	}

	if (obj_desc) {
		/*
		 * Must increment the new value's reference count
		 * (if it is an internal object)
		 */
		acpi_ut_add_reference(obj_desc);

		/*
		 * Handle objects with multiple descriptors - walk
		 * to the end of the descriptor list
		 */
		last_obj_desc = obj_desc;
		while (last_obj_desc->common.next_object) {
			last_obj_desc = last_obj_desc->common.next_object;
		}

		/* Install the object at the front of the object list */

		last_obj_desc->common.next_object = node->object;
	}

	node->type = (u8) object_type;
	node->object = obj_desc;

	return_ACPI_STATUS(AE_OK);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_detach_object
 *
 * PARAMETERS:  node           - A Namespace node whose object will be detached
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Detach/delete an object associated with a namespace node.
 *              if the object is an allocated object, it is freed.
 *              Otherwise, the field is simply cleared.
 *
 ******************************************************************************/

void acpi_ns_detach_object(struct acpi_namespace_node *node)
{
	union acpi_operand_object *obj_desc;

	ACPI_FUNCTION_TRACE(ns_detach_object);

	obj_desc = node->object;

	if (!obj_desc || (obj_desc->common.type == ACPI_TYPE_LOCAL_DATA)) {
		return_VOID;
	}

	if (node->flags & ANOBJ_ALLOCATED_BUFFER) {

		/* Free the dynamic aml buffer */

		if (obj_desc->common.type == ACPI_TYPE_METHOD) {
			ACPI_FREE(obj_desc->method.aml_start);
		}
	}

	/* Clear the Node entry in all cases */

	node->object = NULL;
	if (ACPI_GET_DESCRIPTOR_TYPE(obj_desc) == ACPI_DESC_TYPE_OPERAND) {

		/* Unlink object from front of possible object list */

		node->object = obj_desc->common.next_object;

		/* Handle possible 2-descriptor object */

		if (node->object &&
		    (node->object->common.type != ACPI_TYPE_LOCAL_DATA)) {
			node->object = node->object->common.next_object;
		}

		/*
		 * Detach the object from any data objects (which are still held by
		 * the namespace node)
		 */
		if (obj_desc->common.next_object &&
		    ((obj_desc->common.next_object)->common.type ==
		     ACPI_TYPE_LOCAL_DATA)) {
			obj_desc->common.next_object = NULL;
		}
	}

	/* Reset the node type to untyped */

	node->type = ACPI_TYPE_ANY;

	ACPI_DEBUG_PRINT((ACPI_DB_NAMES, "Node %p [%4.4s] Object %p\n",
			  node, acpi_ut_get_node_name(node), obj_desc));

	/* Remove one reference on the object (and all subobjects) */

	acpi_ut_remove_reference(obj_desc);
	return_VOID;
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_get_attached_object
 *
 * PARAMETERS:  node             - Namespace node
 *
 * RETURN:      Current value of the object field from the Node whose
 *              handle is passed
 *
 * DESCRIPTION: Obtain the object attached to a namespace node.
 *
 ******************************************************************************/

union acpi_operand_object *acpi_ns_get_attached_object(struct
						       acpi_namespace_node
						       *node)
{
	ACPI_FUNCTION_TRACE_PTR(ns_get_attached_object, node);

	if (!node) {
		ACPI_WARNING((AE_INFO, "Null Node ptr"));
		return_PTR(NULL);
	}

	if (!node->object ||
	    ((ACPI_GET_DESCRIPTOR_TYPE(node->object) != ACPI_DESC_TYPE_OPERAND)
	     && (ACPI_GET_DESCRIPTOR_TYPE(node->object) !=
		 ACPI_DESC_TYPE_NAMED))
	    || ((node->object)->common.type == ACPI_TYPE_LOCAL_DATA)) {
		return_PTR(NULL);
	}

	return_PTR(node->object);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_get_secondary_object
 *
 * PARAMETERS:  node             - Namespace node
 *
 * RETURN:      Current value of the object field from the Node whose
 *              handle is passed.
 *
 * DESCRIPTION: Obtain a secondary object associated with a namespace node.
 *
 ******************************************************************************/

union acpi_operand_object *acpi_ns_get_secondary_object(union
							acpi_operand_object
							*obj_desc)
{
	ACPI_FUNCTION_TRACE_PTR(ns_get_secondary_object, obj_desc);

	if ((!obj_desc) ||
	    (obj_desc->common.type == ACPI_TYPE_LOCAL_DATA) ||
	    (!obj_desc->common.next_object) ||
	    ((obj_desc->common.next_object)->common.type ==
	     ACPI_TYPE_LOCAL_DATA)) {
		return_PTR(NULL);
	}

	return_PTR(obj_desc->common.next_object);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_attach_data
 *
 * PARAMETERS:  node            - Namespace node
 *              handler         - Handler to be associated with the data
 *              data            - Data to be attached
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Low-level attach data. Create and attach a Data object.
 *
 ******************************************************************************/

acpi_status
acpi_ns_attach_data(struct acpi_namespace_node *node,
		    acpi_object_handler handler, void *data)
{
	union acpi_operand_object *prev_obj_desc;
	union acpi_operand_object *obj_desc;
	union acpi_operand_object *data_desc;

	/* We only allow one attachment per handler */

	prev_obj_desc = NULL;
	obj_desc = node->object;
	while (obj_desc) {
		if ((obj_desc->common.type == ACPI_TYPE_LOCAL_DATA) &&
		    (obj_desc->data.handler == handler)) {
			return (AE_ALREADY_EXISTS);
		}

		prev_obj_desc = obj_desc;
		obj_desc = obj_desc->common.next_object;
	}

	/* Create an internal object for the data */

	data_desc = acpi_ut_create_internal_object(ACPI_TYPE_LOCAL_DATA);
	if (!data_desc) {
		return (AE_NO_MEMORY);
	}

	data_desc->data.handler = handler;
	data_desc->data.pointer = data;

	/* Install the data object */

	if (prev_obj_desc) {
		prev_obj_desc->common.next_object = data_desc;
	} else {
		node->object = data_desc;
	}

	return (AE_OK);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_detach_data
 *
 * PARAMETERS:  node            - Namespace node
 *              handler         - Handler associated with the data
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Low-level detach data. Delete the data node, but the caller
 *              is responsible for the actual data.
 *
 ******************************************************************************/

acpi_status
acpi_ns_detach_data(struct acpi_namespace_node * node,
		    acpi_object_handler handler)
{
	union acpi_operand_object *obj_desc;
	union acpi_operand_object *prev_obj_desc;

	prev_obj_desc = NULL;
	obj_desc = node->object;
	while (obj_desc) {
		if ((obj_desc->common.type == ACPI_TYPE_LOCAL_DATA) &&
		    (obj_desc->data.handler == handler)) {
			if (prev_obj_desc) {
				prev_obj_desc->common.next_object =
				    obj_desc->common.next_object;
			} else {
				node->object = obj_desc->common.next_object;
			}

			acpi_ut_remove_reference(obj_desc);
			return (AE_OK);
		}

		prev_obj_desc = obj_desc;
		obj_desc = obj_desc->common.next_object;
	}

	return (AE_NOT_FOUND);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_get_attached_data
 *
 * PARAMETERS:  node            - Namespace node
 *              handler         - Handler associated with the data
 *              data            - Where the data is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Low level interface to obtain data previously associated with
 *              a namespace node.
 *
 ******************************************************************************/

acpi_status
acpi_ns_get_attached_data(struct acpi_namespace_node * node,
			  acpi_object_handler handler, void **data)
{
	union acpi_operand_object *obj_desc;

	obj_desc = node->object;
	while (obj_desc) {
		if ((obj_desc->common.type == ACPI_TYPE_LOCAL_DATA) &&
		    (obj_desc->data.handler == handler)) {
			*data = obj_desc->data.pointer;
			return (AE_OK);
		}

		obj_desc = obj_desc->common.next_object;
	}

	return (AE_NOT_FOUND);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           /******************************************************************************
 *
 * Module Name: nsparse - namespace interface to AML parser
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2015, Intel Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 */

#include <acpi/acpi.h>
#include "accommon.h"
#include "acnamesp.h"
#include "acparser.h"
#include "acdispat.h"
#include "actables.h"

#define _COMPONENT          ACPI_NAMESPACE
ACPI_MODULE_NAME("nsparse")

/*******************************************************************************
 *
 * FUNCTION:    ns_one_complete_parse
 *
 * PARAMETERS:  pass_number             - 1 or 2
 *              table_desc              - The table to be parsed.
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Perform one complete parse of an ACPI/AML table.
 *
 ******************************************************************************/
acpi_status
acpi_ns_one_complete_parse(u32 pass_number,
			   u32 table_index,
			   struct acpi_namespace_node *start_node)
{
	union acpi_parse_object *parse_root;
	acpi_status status;
	u32 aml_length;
	u8 *aml_start;
	struct acpi_walk_state *walk_state;
	struct acpi_table_header *table;
	acpi_owner_id owner_id;

	ACPI_FUNCTION_TRACE(ns_one_complete_parse);

	status = acpi_get_table_by_index(table_index, &table);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/* Table must consist of at least a complete header */

	if (table->length < sizeof(struct acpi_table_header)) {
		return_ACPI_STATUS(AE_BAD_HEADER);
	}

	aml_start = (u8 *)table + sizeof(struct acpi_table_header);
	aml_length = table->length - sizeof(struct acpi_table_header);

	status = acpi_tb_get_owner_id(table_index, &owner_id);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/* Create and init a Root Node */

	parse_root = acpi_ps_create_scope_op(aml_start);
	if (!parse_root) {
		return_ACPI_STATUS(AE_NO_MEMORY);
	}

	/* Create and initialize a new walk state */

	walk_state = acpi_ds_create_walk_state(owner_id, NULL, NULL, NULL);
	if (!walk_state) {
		acpi_ps_free_op(parse_root);
		return_ACPI_STATUS(AE_NO_MEMORY);
	}

	status = acpi_ds_init_aml_walk(walk_state, parse_root, NULL,
				       aml_start, aml_length, NULL,
				       (u8)pass_number);
	if (ACPI_FAILURE(status)) {
		acpi_ds_delete_walk_state(walk_state);
		goto cleanup;
	}

	/* Found OSDT table, enable the namespace override feature */

	if (ACPI_COMPARE_NAME(table->signature, ACPI_SIG_OSDT) &&
	    pass_number == ACPI_IMODE_LOAD_PASS1) {
		walk_state->namespace_override = TRUE;
	}

	/* start_node is the default location to load the table */

	if (start_node && start_node != acpi_gbl_root_node) {
		status =
		    acpi_ds_scope_stack_push(start_node, ACPI_TYPE_METHOD,
					     walk_state);
		if (ACPI_FAILURE(status)) {
			acpi_ds_delete_walk_state(walk_state);
			goto cleanup;
		}
	}

	/* Parse the AML */

	ACPI_DEBUG_PRINT((ACPI_DB_PARSE, "*PARSE* pass %u parse\n",
			  pass_number));
	status = acpi_ps_parse_aml(walk_state);

cleanup:
	acpi_ps_delete_parse_tree(parse_root);
	return_ACPI_STATUS(status);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_parse_table
 *
 * PARAMETERS:  table_desc      - An ACPI table descriptor for table to parse
 *              start_node      - Where to enter the table into the namespace
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Parse AML within an ACPI table and return a tree of ops
 *
 ******************************************************************************/

acpi_status
acpi_ns_parse_table(u32 table_index, struct acpi_namespace_node *start_node)
{
	acpi_status status;

	ACPI_FUNCTION_TRACE(ns_parse_table);

	/*
	 * AML Parse, pass 1
	 *
	 * In this pass, we load most of the namespace. Control methods
	 * are not parsed until later. A parse tree is not created. Instead,
	 * each Parser Op subtree is deleted when it is finished. This saves
	 * a great deal of memory, and allows a small cache of parse objects
	 * to service the entire parse. The second pass of the parse then
	 * performs another complete parse of the AML.
	 */
	ACPI_DEBUG_PRINT((ACPI_DB_PARSE, "**** Start pass 1\n"));
	status = acpi_ns_one_complete_parse(ACPI_IMODE_LOAD_PASS1,
					    table_index, start_node);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/*
	 * AML Parse, pass 2
	 *
	 * In this pass, we resolve forward references and other things
	 * that could not be completed during the first pass.
	 * Another complete parse of the AML is performed, but the
	 * overhead of this is compensated for by the fact that the
	 * parse objects are all cached.
	 */
	ACPI_DEBUG_PRINT((ACPI_DB_PARSE, "**** Start pass 2\n"));
	status = acpi_ns_one_complete_parse(ACPI_IMODE_LOAD_PASS2,
					    table_index, start_node);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	return_ACPI_STATUS(status);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  /******************************************************************************
 *
 * Module Name: nspredef - Validation of ACPI predefined methods and objects
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2015, Intel Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 */

#define ACPI_CREATE_PREDEFINED_TABLE

#include <acpi/acpi.h>
#include "accommon.h"
#include "acnamesp.h"
#include "acpredef.h"

#define _COMPONENT          ACPI_NAMESPACE
ACPI_MODULE_NAME("nspredef")

/*******************************************************************************
 *
 * This module validates predefined ACPI objects that appear in the namespace,
 * at the time they are evaluated (via acpi_evaluate_object). The purpose of this
 * validation is to detect problems with BIOS-exposed predefined ACPI objects
 * before the results are returned to the ACPI-related drivers.
 *
 * There are several areas that are validated:
 *
 *  1) The number of input arguments as defined by the method/object in the
 *     ASL is validated against the ACPI specification.
 *  2) The type of the return object (if any) is validated against the ACPI
 *     specification.
 *  3) For returned package objects, the count of package elements is
 *     validated, as well as the type of each package element. Nested
 *     packages are supported.
 *
 * For any problems found, a warning message is issued.
 *
 ******************************************************************************/
/* Local prototypes */
static acpi_status
acpi_ns_check_reference(struct acpi_evaluate_info *info,
			union acpi_operand_object *return_object);

static u32 acpi_ns_get_bitmapped_type(union acpi_operand_object *return_object);

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_check_return_value
 *
 * PARAMETERS:  node            - Namespace node for the method/object
 *              info            - Method execution information block
 *              user_param_count - Number of parameters actually passed
 *              return_status   - Status from the object evaluation
 *              return_object_ptr - Pointer to the object returned from the
 *                                evaluation of a method or object
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Check the value returned from a predefined name.
 *
 ******************************************************************************/

acpi_status
acpi_ns_check_return_value(struct acpi_namespace_node *node,
			   struct acpi_evaluate_info *info,
			   u32 user_param_count,
			   acpi_status return_status,
			   union acpi_operand_object **return_object_ptr)
{
	acpi_status status;
	const union acpi_predefined_info *predefined;

	/* If not a predefined name, we cannot validate the return object */

	predefined = info->predefined;
	if (!predefined) {
		return (AE_OK);
	}

	/*
	 * If the method failed or did not actually return an object, we cannot
	 * validate the return object
	 */
	if ((return_status != AE_OK) && (return_status != AE_CTRL_RETURN_VALUE)) {
		return (AE_OK);
	}

	/*
	 * Return value validation and possible repair.
	 *
	 * 1) Don't perform return value validation/repair if this feature
	 * has been disabled via a global option.
	 *
	 * 2) We have a return value, but if one wasn't expected, just exit,
	 * this is not a problem. For example, if the "Implicit Return"
	 * feature is enabled, methods will always return a value.
	 *
	 * 3) If the return value can be of any type, then we cannot perform
	 * any validation, just exit.
	 */
	if (acpi_gbl_disable_auto_repair ||
	    (!predefined->info.expected_btypes) ||
	    (predefined->info.expected_btypes == ACPI_RTYPE_ALL)) {
		return (AE_OK);
	}

	/*
	 * Check that the type of the main return object is what is expected
	 * for this predefined name
	 */
	status = acpi_ns_check_object_type(info, return_object_ptr,
					   predefined->info.expected_btypes,
					   ACPI_NOT_PACKAGE_ELEMENT);
	if (ACPI_FAILURE(status)) {
		goto exit;
	}

	/*
	 *
	 * 4) If there is no return value and it is optional, just return
	 * AE_OK (_WAK).
	 */
	if (!(*return_object_ptr)) {
		goto exit;
	}

	/*
	 * For returned Package objects, check the type of all sub-objects.
	 * Note: Package may have been newly created by call above.
	 */
	if ((*return_object_ptr)->common.type == ACPI_TYPE_PACKAGE) {
		info->parent_package = *return_object_ptr;
		status = acpi_ns_check_package(info, return_object_ptr);
		if (ACPI_FAILURE(status)) {

			/* We might be able to fix some errors */

			if ((status != AE_AML_OPERAND_TYPE) &&
			    (status != AE_AML_OPERAND_VALUE)) {
				goto exit;
			}
		}
	}

	/*
	 * The return object was OK, or it was successfully repaired above.
	 * Now make some additional checks such as verifying that package
	 * objects are sorted correctly (if required) or buffer objects have
	 * the correct data width (bytes vs. dwords). These repairs are
	 * performed on a per-name basis, i.e., the code is specific to
	 * particular predefined names.
	 */
	status = acpi_ns_complex_repairs(info, node, status, return_object_ptr);

exit:
	/*
	 * If the object validation failed or if we successfully repaired one
	 * or more objects, mark the parent node to suppress further warning
	 * messages during the next evaluation of the same method/object.
	 */
	if (ACPI_FAILURE(status) || (info->return_flags & ACPI_OBJECT_REPAIRED)) {
		node->flags |= ANOBJ_EVALUATED;
	}

	return (status);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_check_object_type
 *
 * PARAMETERS:  info            - Method execution information block
 *              return_object_ptr - Pointer to the object returned from the
 *                                evaluation of a method or object
 *              expected_btypes - Bitmap of expected return type(s)
 *              package_index   - Index of object within parent package (if
 *                                applicable - ACPI_NOT_PACKAGE_ELEMENT
 *                                otherwise)
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Check the type of the return object against the expected object
 *              type(s). Use of Btype allows multiple expected object types.
 *
 ******************************************************************************/

acpi_status
acpi_ns_check_object_type(struct acpi_evaluate_info *info,
			  union acpi_operand_object **return_object_ptr,
			  u32 expected_btypes, u32 package_index)
{
	union acpi_operand_object *return_object = *return_object_ptr;
	acpi_status status = AE_OK;
	char type_buffer[96];	/* Room for 10 types */

	/* A Namespace node should not get here, but make sure */

	if (return_object &&
	    ACPI_GET_DESCRIPTOR_TYPE(return_object) == ACPI_DESC_TYPE_NAMED) {
		ACPI_WARN_PREDEFINED((AE_INFO, info->full_pathname,
				      info->node_flags,
				      "Invalid return type - Found a Namespace node [%4.4s] type %s",
				      return_object->node.name.ascii,
				      acpi_ut_get_type_name(return_object->node.
							    type)));
		return (AE_AML_OPERAND_TYPE);
	}

	/*
	 * Convert the object type (ACPI_TYPE_xxx) to a bitmapped object type.
	 * The bitmapped type allows multiple possible return types.
	 *
	 * Note, the cases below must handle all of the possible types returned
	 * from all of the predefined names (including elements of returned
	 * packages)
	 */
	info->return_btype = acpi_ns_get_bitmapped_type(return_object);
	if (info->return_btype == ACPI_RTYPE_ANY) {

		/* Not one of the supported objects, must be incorrect */
		goto type_error_exit;
	}

	/* For reference objects, check that the reference type is correct */

	if ((info->return_btype & expected_btypes) == ACPI_RTYPE_REFERENCE) {
		status = acpi_ns_check_reference(info, return_object);
		return (status);
	}

	/* Attempt simple repair of the returned object if necessary */

	status = acpi_ns_simple_repair(info, expected_btypes,
				       package_index, return_object_ptr);
	if (ACPI_SUCCESS(status)) {
		return (AE_OK);	/* Successful repair */
	}

type_error_exit:

	/* Create a string with all expected types for this predefined object */

	acpi_ut_get_expected_return_types(type_buffer, expected_btypes);

	if (!return_object) {
		ACPI_WARN_PREDEFINED((AE_INFO, info->full_pathname,
				      info->node_flags,
				      "Expected return object of type %s",
				      type_buffer));
	} else if (package_index == ACPI_NOT_PACKAGE_ELEMENT) {
		ACPI_WARN_PREDEFINED((AE_INFO, info->full_pathname,
				      info->node_flags,
				      "Return type mismatch - found %s, expected %s",
				      acpi_ut_get_object_type_name
				      (return_object), type_buffer));
	} else {
		ACPI_WARN_PREDEFINED((AE_INFO, info->full_pathname,
				      info->node_flags,
				      "Return Package type mismatch at index %u - "
				      "found %s, expected %s", package_index,
				      acpi_ut_get_object_type_name
				      (return_object), type_buffer));
	}

	return (AE_AML_OPERAND_TYPE);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_check_reference
 *
 * PARAMETERS:  info            - Method execution information block
 *              return_object   - Object returned from the evaluation of a
 *                                method or object
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Check a returned reference object for the correct reference
 *              type. The only reference type that can be returned from a
 *              predefined method is a named reference. All others are invalid.
 *
 ******************************************************************************/

static acpi_status
acpi_ns_check_reference(struct acpi_evaluate_info *info,
			union acpi_operand_object *return_object)
{

	/*
	 * Check the reference object for the correct reference type (opcode).
	 * The only type of reference that can be converted to an union acpi_object is
	 * a reference to a named object (ref