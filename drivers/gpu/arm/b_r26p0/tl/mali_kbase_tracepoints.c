l_name_length || !internal_name || !converted_name) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	/* Check for a prefix (one '\' | one or more '^') */

	switch (internal_name[0]) {
	case AML_ROOT_PREFIX:

		prefix_length = 1;
		break;

	case AML_PARENT_PREFIX:

		for (i = 0; i < internal_name_length; i++) {
			if (ACPI_IS_PARENT_PREFIX(internal_name[i])) {
				prefix_length = i + 1;
			} else {
				break;
			}
		}

		if (i == internal_name_length) {
			prefix_length = i;
		}

		break;

	default:

		break;
	}

	/*
	 * Check for object names. Note that there could be 0-255 of these
	 * 4-byte elements.
	 */
	if (prefix_length < internal_name_length) {
		switch (internal_name[prefix_length]) {
		case AML_MULTI_NAME_PREFIX_OP:

			/* <count> 4-byte names */

			names_index = prefix_length + 2;
			num_segments = (u8)
			    internal_name[(acpi_size) prefix_length + 1];
			break;

		case AML_DUAL_NAME_PREFIX:

			/* Two 4-byte names */

			names_index = prefix_length + 1;
			num_segments = 2;
			break;

		case 0:

			/* null_name */

			names_index = 0;
			num_segments = 0;
			break;

		default:

			/* one 4-byte name */

			names_index = prefix_length;
			num_segments = 1;
			break;
		}
	}

	/*
	 * Calculate the length of converted_name, which equals the length
	 * of the prefix, length of all object names, length of any required
	 * punctuation ('.') between object names, plus the NULL terminator.
	 */
	required_length = prefix_length + (4 * num_segments) +
	    ((num_segments > 0) ? (num_segments - 1) : 0) + 1;

	/*
	 * Check to see if we're still in bounds. If not, there's a problem
	 * with internal_name (invalid format).
	 */
	if (required_length > internal_name_length) {
		ACPI_ERROR((AE_INFO, "Invalid internal name"));
		return_ACPI_STATUS(AE_BAD_PATHNAME);
	}

	/* Build the converted_name */

	*converted_name = ACPI_ALLOCATE_ZEROED(required_length);
	if (!(*converted_name)) {
		return_ACPI_STATUS(AE_NO_MEMORY);
	}

	j = 0;

	for (i = 0; i < prefix_length; i++) {
		(*converted_name)[j++] = internal_name[i];
	}

	if (num_segments > 0) {
		for (i = 0; i < num_segments; i++) {
			if (i > 0) {
				(*converted_name)[j++] = '.';
			}

			/* Copy and validate the 4-char name segment */

			ACPI_MOVE_NAME(&(*converted_name)[j],
				       &internal_name[names_index]);
			acpi_ut_repair_name(&(*converted_name)[j]);

			j += ACPI_NAME_SIZE;
			names_index += ACPI_NAME_SIZE;
		}
	}

	if (converted_name_length) {
		*converted_name_length = (u32) required_length;
	}

	return_ACPI_STATUS(AE_OK);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_validate_handle
 *
 * PARAMETERS:  handle          - Handle to be validated and typecast to a
 *                                namespace node.
 *
 * RETURN:      A pointer to a namespace node
 *
 * DESCRIPTION: Convert a namespace handle to a namespace node. Handles special
 *              cases for the root node.
 *
 * NOTE: Real integer handles would allow for more verification
 *       and keep all pointers within this subsystem - however this introduces
 *       more overhead and has not been necessary to this point. Drivers
 *       holding handles are typically notified before a node becomes invalid
 *       due to a table unload.
 *
 ******************************************************************************/

struct acpi_namespace_node *acpi_ns_validate_handle(acpi_handle handle)
{

	ACPI_FUNCTION_ENTRY();

	/* Parameter validation */

	if ((!handle) || (handle == ACPI_ROOT_OBJECT)) {
		return (acpi_gbl_root_node);
	}

	/* We can at least attempt to verify the handle */

	if (ACPI_GET_DESCRIPTOR_TYPE(handle) != ACPI_DESC_TYPE_NAMED) {
		return (NULL);
	}

	return (ACPI_CAST_PTR(struct acpi_namespace_node, handle));
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_terminate
 *
 * PARAMETERS:  none
 *
 * RETURN:      none
 *
 * DESCRIPTION: free memory allocated for namespace and ACPI table storage.
 *
 ******************************************************************************/

void acpi_ns_terminate(void)
{
	acpi_status status;
	union acpi_operand_object *prev;
	union acpi_operand_object *next;

	ACPI_FUNCTION_TRACE(ns_terminate);

	/* Delete any module-level code blocks */

	next = acpi_gbl_module_code_list;
	while (next) {
		prev = next;
		next = next->method.mutex;
		prev->method.mutex = NULL;	/* Clear the Mutex (cheated) field */
		acpi_ut_remove_reference(prev);
	}

	/*
	 * Free the entire namespace -- all nodes and all objects
	 * attached to the nodes
	 */
	acpi_ns_delete_namespace_subtree(acpi_gbl_root_node);

	/* Delete any objects attached to the root node */

	status = acpi_ut_acquire_mutex(ACPI_MTX_NAMESPACE);
	if (ACPI_FAILURE(status)) {
		return_VOID;
	}

	acpi_ns_delete_node(acpi_gbl_root_node);
	(void)acpi_ut_release_mutex(ACPI_MTX_NAMESPACE);

	ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Namespace freed\n"));
	return_VOID;
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_opens_scope
 *
 * PARAMETERS:  type        - A valid namespace type
 *
 * RETURN:      NEWSCOPE if the passed type "opens a name scope" according
 *              to the ACPI specification, else 0
 *
 ******************************************************************************/

u32 acpi_ns_opens_scope(acpi_object_type type)
{
	ACPI_FUNCTION_ENTRY();

	if (type > ACPI_TYPE_LOCAL_MAX) {

		/* type code out of range  */

		ACPI_WARNING((AE_INFO, "Invalid Object Type 0x%X", type));
		return (ACPI_NS_NORMAL);
	}

	return (((u32)acpi_gbl_ns_properties[type]) & ACPI_NS_NEWSCOPE);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_get_node
 *
 * PARAMETERS:  *pathname   - Name to be found, in external (ASL) format. The
 *                            \ (backslash) and ^ (carat) prefixes, and the
 *                            . (period) to separate segments are supported.
 *              prefix_node  - Root of subtree to be searched, or NS_ALL for the
 *                            root of the name space. If Name is fully
 *                            qualified (first s8 is '\'), the passed value
 *                            of Scope will not be accessed.
 *              flags       - Used to indicate whether to perform upsearch or
 *                            not.
 *              return_node - Where the Node is returned
 *
 * DESCRIPTION: Look up a name relative to a given scope and return the
 *              corresponding Node. NOTE: Scope can be null.
 *
 * MUTEX:       Locks namespace
 *
 ******************************************************************************/

acpi_status
acpi_ns_get_node(struct acpi_namespace_node *prefix_node,
		 const char *pathname,
		 u32 flags, struct acpi_namespace_node **return_node)
{
	union acpi_generic_state scope_info;
	acpi_status status;
	char *internal_path;

	ACPI_FUNCTION_TRACE_PTR(ns_get_node, ACPI_CAST_PTR(char, pathname));

	/* Simplest case is a null pathname */

	if (!pathname) {
		*return_node = prefix_node;
		if (!prefix_node) {
			*return_node = acpi_gbl_root_node;
		}
		return_ACPI_STATUS(AE_OK);
	}

	/* Quick check for a reference to the root */

	if (ACPI_IS_ROOT_PREFIX(pathname[0]) && (!pathname[1])) {
		*return_node = acpi_gbl_root_node;
		return_ACPI_STATUS(AE_OK);
	}

	/* Convert path to internal representation */

	status = acpi_ns_internalize_name(pathname, &internal_path);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/* Must lock namespace during lookup */

	status = acpi_ut_acquire_mutex(ACPI_MTX_NAMESPACE);
	if (ACPI_FAILURE(status)) {
		goto cleanup;
	}

	/* Setup lookup scope (search starting point) */

	scope_info.scope.node = prefix_node;

	/* Lookup the name in the namespace */

	status = acpi_ns_lookup(&scope_info, internal_path, ACPI_TYPE_ANY,
				ACPI_IMODE_EXECUTE,
				(flags | ACPI_NS_DONT_OPEN_SCOPE), NULL,
				return_node);
	if (ACPI_FAILURE(status)) {
		ACPI_DEBUG_PRINT((ACPI_DB_EXEC, "%s, %s\n",
				  pathname, acpi_format_exception(status)));
	}

	(void)acpi_ut_release_mutex(ACPI_MTX_NAMESPACE);

cleanup:
	ACPI_FREE(internal_path);
	return_ACPI_STATUS(status);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    /******************************************************************************
 *
 * Module Name: nswalk - Functions for walking the ACPI namespace
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

#define _COMPONENT          ACPI_NAMESPACE
ACPI_MODULE_NAME("nswalk")

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_get_next_node
 *
 * PARAMETERS:  parent_node         - Parent node whose children we are
 *                                    getting
 *              child_node          - Previous child that was found.
 *                                    The NEXT child will be returned
 *
 * RETURN:      struct acpi_namespace_node - Pointer to the NEXT child or NULL if
 *                                    none is found.
 *
 * DESCRIPTION: Return the next peer node within the namespace. If Handle
 *              is valid, Scope is ignored. Otherwise, the first node
 *              within Scope is returned.
 *
 ******************************************************************************/
struct acpi_namespace_node *acpi_ns_get_next_node(struct acpi_namespace_node
						  *parent_node,
						  struct acpi_namespace_node
						  *child_node)
{
	ACPI_FUNCTION_ENTRY();

	if (!child_node) {

		/* It's really the parent's _scope_ that we want */

		return (parent_node->child);
	}

	/* Otherwise just return the next peer */

	return (child_node->peer);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_get_next_node_typed
 *
 * PARAMETERS:  type                - Type of node to be searched for
 *              parent_node         - Parent node whose children we are
 *                                    getting
 *              child_node          - Previous child that was found.
 *                                    The NEXT child will be returned
 *
 * RETURN:      struct acpi_namespace_node - Pointer to the NEXT child or NULL if
 *                                    none is found.
 *
 * DESCRIPTION: Return the next peer node within the namespace. If Handle
 *              is valid, Scope is ignored. Otherwise, the first node
 *              within Scope is returned.
 *
 ******************************************************************************/

struct acpi_namespace_node *acpi_ns_get_next_node_typed(acpi_object_type type,
							struct
							acpi_namespace_node
							*parent_node,
							struct
							acpi_namespace_node
							*child_node)
{
	struct acpi_namespace_node *next_node = NULL;

	ACPI_FUNCTION_ENTRY();

	next_node = acpi_ns_get_next_node(parent_node, child_node);


	/* If any type is OK, we are done */

	if (type == ACPI_TYPE_ANY) {

		/* next_node is NULL if we are at the end-of-list */

		return (next_node);
	}

	/* Must search for the node -- but within this scope only */

	while (next_node) {

		/* If type matches, we are done */

		if (next_node->type == type) {
			return (next_node);
		}

		/* Otherwise, move on to the next peer node */

		next_node = next_node->peer;
	}

	/* Not found */

	return (NULL);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_walk_namespace
 *
 * PARAMETERS:  type                - acpi_object_type to search for
 *              start_node          - Handle in namespace where search begins
 *              max_depth           - Depth to which search is to reach
 *              flags               - Whether to unlock the NS before invoking
 *                                    the callback routine
 *              descending_callback - Called during tree descent
 *                                    when an object of "Type" is found
 *              ascending_callback  - Called during tree ascent
 *                                    when an object of "Type" is found
 *              context             - Passed to user function(s) above
 *              return_value        - from the user_function if terminated
 *                                    early. Otherwise, returns NULL.
 * RETURNS:     Status
 *
 * DESCRIPTION: Performs a modified depth-first walk of the namespace tree,
 *              starting (and ending) at the node specified by start_handle.
 *              The callback function is called whenever a node that matches
 *              the type parameter is found. If the callback function returns
 *              a non-zero value, the search is terminated immediately and
 *              this value is returned to the caller.
 *
 *              The point of this procedure is to provide a generic namespace
 *              walk routine that can be called from multiple places to
 *              provide multiple services; the callback function(s) can be
 *              tailored to each task, whether it is a print function,
 *              a compare function, etc.
 *
 ******************************************************************************/

acpi_status
acpi_ns_walk_namespace(acpi_object_type type,
		       acpi_handle start_node,
		       u32 max_depth,
		       u32 flags,
		       acpi_walk_callback descending_callback,
		       acpi_walk_callback ascending_callback,
		       void *context, void **return_value)
{
	acpi_status status;
	acpi_status mutex_status;
	struct acpi_namespace_node *child_node;
	struct acpi_namespace_node *parent_node;
	acpi_object_type child_type;
	u32 level;
	u8 node_previously_visited = FALSE;

	ACPI_FUNCTION_TRACE(ns_walk_namespace);

	/* Special case for the namespace Root Node */

	if (start_node == ACPI_ROOT_OBJECT) {
		start_node = acpi_gbl_root_node;
		if (!start_node) {
			return_ACPI_STATUS(AE_NO_NAMESPACE);
		}
	}

	/* Null child means "get first node" */

	parent_node = start_node;
	child_node = acpi_ns_get_next_node(parent_node, NULL);
	child_type = ACPI_TYPE_ANY;
	level = 1;

	/*
	 * Traverse the tree of nodes until we bubble back up to where we
	 * started. When Level is zero, the loop is done because we have
	 * bubbled up to (and passed) the original parent handle (start_entry)
	 */
	while (level > 0 && child_node) {
		status = AE_OK;

		/* Found next child, get the type if we are not searching for ANY */

		if (type != ACPI_TYPE_ANY) {
			child_type = child_node->type;
		}

		/*
		 * Ignore all temporary namespace nodes (created during control
		 * method execution) unless told otherwise. These temporary nodes
		 * can cause a race condition because they can be deleted during
		 * the execution of the user function (if the namespace is
		 * unlocked before invocation of the user function.) Only the
		 * debugger namespace dump will examine the temporary nodes.
		 */
		if ((child_node->flags & ANOBJ_TEMPORARY) &&
		    !(flags & ACPI_NS_WALK_TEMP_NODES)) {
			status = AE_CTRL_DEPTH;
		}

		/* Type must match requested type */

		else if (child_type == type) {
			/*
			 * Found a matching node, invoke the user callback function.
			 * Unlock the namespace if flag is set.
			 */
			if (flags & ACPI_NS_WALK_UNLOCK) {
				mutex_status =
				    acpi_ut_release_mutex(ACPI_MTX_NAMESPACE);
				if (ACPI_FAILURE(mutex_status)) {
					return_ACPI_STATUS(mutex_status);
				}
			}

			/*
			 * Invoke the user function, either descending, ascending,
			 * or both.
			 */
			if (!node_previously_visited) {
				if (descending_callback) {
					status =
					    descending_callback(child_node,
								level, context,
								return_value);
				}
			} else {
				if (ascending_callback) {
					status =
					    ascending_callback(child_node,
							       level, context,
							       return_value);
				}
			}

			if (flags & ACPI_NS_WALK_UNLOCK) {
				mutex_status =
				    acpi_ut_acquire_mutex(ACPI_MTX_NAMESPACE);
				if (ACPI_FAILURE(mutex_status)) {
					return_ACPI_STATUS(mutex_status);
				}
			}

			switch (status) {
			case AE_OK:
			case AE_CTRL_DEPTH:

				/* Just keep going */
				break;

			case AE_CTRL_TERMINATE:

				/* Exit now, with OK status */

				return_ACPI_STATUS(AE_OK);

			default:

				/* All others are valid exceptions */

				return_ACPI_STATUS(status);
			}
		}

		/*
		 * Depth first search: Attempt to go down another level in the
		 * namespace if we are allowed to. Don't go any further if we have
		 * reached the caller specified maximum depth or if the user
		 * function has specified that the maximum depth has been reached.
		 */
		if (!node_previously_visited &&
		    (level < max_depth) && (status != AE_CTRL_DEPTH)) {
			if (child_node->child) {

				/* There is at least one child of this node, visit it */

				level++;
				parent_node = child_node;
				child_node =
				    acpi_ns_get_next_node(parent_node, NULL);
				continue;
			}
		}

		/* No more children, re-visit this node */

		if (!node_previously_visited) {
			node_previously_visited = TRUE;
			continue;
		}

		/* No more children, visit peers */

		child_node = acpi_ns_get_next_node(parent_node, child_node);
		if (child_node) {
			node_previously_visited = FALSE;
		}

		/* No peers, re-visit parent */

		else {
			/*
			 * No more children of this node (acpi_ns_get_next_node failed), go
			 * back upwards in the namespace tree to the node's parent.
			 */
			level--;
			child_node = parent_node;
			parent_node = parent_node->parent;

			node_previously_visited = TRUE;
		}
	}

	/* Complete walk, not terminated by user function */

	return_ACPI_STATUS(AE_OK);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            /*******************************************************************************
 *
 * Module Name: nsxfeval - Public interfaces to the ACPI subsystem
 *                         ACPI Object evaluation interfaces
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

#define EXPORT_ACPI_INTERFACES

#include <acpi/acpi.h>
#include "accommon.h"
#include "acnamesp.h"
#include "acinterp.h"

#define _COMPONENT          ACPI_NAMESPACE
ACPI_MODULE_NAME("nsxfeval")

/* Local prototypes */
static void acpi_ns_resolve_references(struct acpi_evaluate_info *info);

/*******************************************************************************
 *
 * FUNCTION:    acpi_evaluate_object_typed
 *
 * PARAMETERS:  handle              - Object handle (optional)
 *              pathname            - Object pathname (optional)
 *              external_params     - List of parameters to pass to method,
 *                                    terminated by NULL. May be NULL
 *                                    if no parameters are being passed.
 *              return_buffer       - Where to put method's return value (if
 *                                    any). If NULL, no value is returned.
 *              return_type         - Expected type of return object
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Find and evaluate the given object, passing the given
 *              parameters if necessary. One of "Handle" or "Pathname" must
 *              be valid (non-null)
 *
 ******************************************************************************/

acpi_status
acpi_evaluate_object_typed(acpi_handle handle,
			   acpi_string pathname,
			   struct acpi_object_list *external_params,
			   struct acpi_buffer *return_buffer,
			   acpi_object_type return_type)
{
	acpi_status status;
	u8 free_buffer_on_error = FALSE;

	ACPI_FUNCTION_TRACE(acpi_evaluate_object_typed);

	/* Return buffer must be valid */

	if (!return_buffer) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	if (return_buffer->length == ACPI_ALLOCATE_BUFFER) {
		free_buffer_on_error = TRUE;
	}

	/* Evaluate the object */

	status = acpi_evaluate_object(handle, pathname,
				      external_params, return_buffer);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/* Type ANY means "don't care" */

	if (return_type == ACPI_TYPE_ANY) {
		return_ACPI_STATUS(AE_OK);
	}

	if (return_buffer->length == 0) {

		/* Error because caller specifically asked for a return value */

		ACPI_ERROR((AE_INFO, "No return value"));
		return_ACPI_STATUS(AE_NULL_OBJECT);
	}

	/* Examine the object type returned from evaluate_object */

	if (((union acpi_object *)return_buffer->pointer)->type == return_type) {
		return_ACPI_STATUS(AE_OK);
	}

	/* Return object type does not match requested type */

	ACPI_ERROR((AE_INFO,
		    "Incorrect return type [%s] requested [%s]",
		    acpi_ut_get_type_name(((union acpi_object *)return_buffer->
					   pointer)->type),
		    acpi_ut_get_type_name(return_type)));

	if (free_buffer_on_error) {
		/*
		 * Free a buffer created via ACPI_ALLOCATE_BUFFER.
		 * Note: We use acpi_os_free here because acpi_os_allocate was used
		 * to allocate the buffer. This purposefully bypasses the
		 * (optionally enabled) allocation tracking mechanism since we
		 * only want to track internal allocations.
		 */
		acpi_os_free(return_buffer->pointer);
		return_buffer->pointer = NULL;
	}

	return_buffer->length = 0;
	return_ACPI_STATUS(AE_TYPE);
}

ACPI_EXPORT_SYMBOL(acpi_evaluate_object_typed)

/*******************************************************************************
 *
 * FUNCTION:    acpi_evaluate_object
 *
 * PARAMETERS:  handle              - Object handle (optional)
 *              pathname            - Object pathname (optional)
 *              external_params     - List of parameters to pass to method,
 *                                    terminated by NULL. May be NULL
 *                                    if no parameters are being passed.
 *              return_buffer       - Where to put method's return value (if
 *                                    any). If NULL, no value is returned.
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Find and evaluate the given object, passing the given
 *              parameters if necessary. One of "Handle" or "Pathname" must
 *              be valid (non-null)
 *
 ******************************************************************************/
acpi_status
acpi_evaluate_object(acpi_handle handle,
		     acpi_string pathname,
		     struct acpi_object_list *external_params,
		     struct acpi_buffer *return_buffer)
{
	acpi_status status;
	struct acpi_evaluate_info *info;
	acpi_size buffer_space_needed;
	u32 i;

	ACPI_FUNCTION_TRACE(acpi_evaluate_object);

	/* Allocate and initialize the evaluation information block */

	info = ACPI_ALLOCATE_ZEROED(sizeof(struct acpi_evaluate_info));
	if (!info) {
		return_ACPI_STATUS(AE_NO_MEMORY);
	}

	/* Convert and validate the device handle */

	info->prefix_node = acpi_ns_validate_handle(handle);
	if (!info->prefix_node) {
		status = AE_BAD_PARAMETER;
		goto cleanup;
	}

	/*
	 * Get the actual namespace node for the target object.
	 * Handles these cases:
	 *
	 * 1) Null node, valid pathname from root (absolute path)
	 * 2) Node and valid pathname (path relative to Node)
	 * 3) Node, Null pathname
	 */
	if ((pathname) && (ACPI_IS_ROOT_PREFIX(pathname[0]))) {

		/* The path is fully qualified, just evaluate by name */

		info->prefix_node = NULL;
	} else if (!handle) {
		/*
		 * A handle is optional iff a fully qualified pathname is specified.
		 * Since we've already handled fully qualified names above, this is
		 * an error.
		 */
		if (!pathname) {
			ACPI_DEBUG_PRINT((ACPI_DB_INFO,
					  "Both Handle and Pathname are NULL"));
		} else {
			ACPI_DEBUG_PRINT((ACPI_DB_INFO,
					  "Null Handle with relative pathname [%s]",
					  pathname));
		}

		status = AE_BAD_PARAMETER;
		goto cleanup;
	}

	info->relative_pathname = pathname;

	/*
	 * Convert all external objects passed as arguments to the
	 * internal version(s).
	 */
	if (external_params && external_params->count) {
		info->param_count = (u16)external_params->count;

		/* Warn on impossible argument count */

		if (info->param_count > ACPI_METHOD_NUM_ARGS) {
			ACPI_WARN_PREDEFINED((AE_INFO, pathname,
					      ACPI_WARN_ALWAYS,
					      "Excess arguments (%u) - using only %u",
					      info->param_count,
					      ACPI_METHOD_NUM_ARGS));

			info->param_count = ACPI_METHOD_NUM_ARGS;
		}

		/*
		 * Allocate a new parameter block for the internal objects
		 * Add 1 to count to allow for null terminated internal list
		 */
		info->parameters = ACPI_ALLOCATE_ZEROED(((acpi_size) info->
							 param_count +
							 1) * sizeof(void *));
		if (!info->parameters) {
			status = AE_NO_MEMORY;
			goto cleanup;
		}

		/* Convert each external object in the list to an internal object */

		for (i = 0; i < info->param_count; i++) {
			status =
			    acpi_ut_copy_eobject_to_iobject(&external_params->
							    pointer[i],
							    &info->
							    parameters[i]);
			if (ACPI_FAILURE(status)) {
				goto cleanup;
			}
		}

		info->parameters[info->param_count] = NULL;
	}

#if 0

	/*
	 * Begin incoming argument count analysis. Check for too few args
	 * and too many args.
	 */

	switch (acpi_ns_get_type(info->node)) {
	case ACPI_TYPE_METHOD:

		/* Check incoming argument count against the method definition */

		if (info->obj_desc->method.param_count > info->param_count) {
			ACPI_ERROR((AE_INFO,
				    "Insufficient arguments (%u) - %u are required",
				    info->param_count,
				    info->obj_desc->method.param_count));

			status = AE_MISSING_ARGUMENTS;
			goto cleanup;
		}

		else if (info->obj_desc->method.param_count < info->param_count) {
			ACPI_WARNING((AE_INFO,
				      "Excess arguments (%u) - only %u are required",
				      info->param_count,
				      info->obj_desc->method.param_count));

			/* Just pass the required number of arguments */

			info->param_count = info->obj_desc->method.param_count;
		}

		/*
		 * Any incoming external objects to be passed as arguments to the
		 * method must be converted to internal objects
		 */
		if (info->param_count) {
			/*
			 * Allocate a new parameter block for the internal objects
			 * Add 1 to count to allow for null terminated internal list
			 */
			info->parameters = ACPI_ALLOCATE_ZEROED(((acpi_size)
								 info->
								 param_count +
								 1) *
								sizeof(void *));
			if (!info->parameters) {
				status = AE_NO_MEMORY;
				goto cleanup;
			}

			/* Convert each external object in the list to an internal object */

			for (i = 0; i < info->param_count; i++) {
				status =
				    acpi_ut_copy_eobject_to_iobject
				    (&external_params->pointer[i],
				     &info->parameters[i]);
				if (ACPI_FAILURE(status)) {
					goto cleanup;
				}
			}

			info->parameters[info->param_count] = NULL;
		}
		break;

	default:

		/* Warn if arguments passed to an object that is not a method */

		if (info->param_count) {
			ACPI_WARNING((AE_INFO,
				      "%u arguments were passed to a non-method ACPI object",
				      info->param_count));
		}
		break;
	}

#endif

	/* Now we can evaluate the object */

	status = acpi_ns_evaluate(info);

	/*
	 * If we are expecting a return value, and all went well above,
	 * copy the return value to an external object.
	 */
	if (return_buffer) {
		if (!info->return_object) {
			return_buffer->length = 0;
		} else {
			if (ACPI_GET_DESCRIPTOR_TYPE(info->return_object) ==
			    ACPI_DESC_TYPE_NAMED) {
				/*
				 * If we received a NS Node as a return object, this means that
				 * the object we are evaluating has nothing interesting to
				 * return (such as a mutex, etc.)  We return an error because
				 * these types are essentially unsupported by this interface.
				 * We don't check up front because this makes it easier to add
				 * support for various types at a later date if necessary.
				 */
				status = AE_TYPE;
				info->return_object = NULL;	/* No need to delete a NS Node */
				return_buffer->length = 0;
			}

			if (ACPI_SUCCESS(status)) {

				/* Dereference Index and ref_of references */

				acpi_ns_resolve_references(info);

				/* Get the size of the returned object */

				status =
				    acpi_ut_get_object_size(info->return_object,
							    &buffer_space_needed);
				if (ACPI_SUCCESS(status)) {

					/* Validate/Allocate/Clear caller buffer */

					status =
					    acpi_ut_initialize_buffer
					    (return_buffer,
					     buffer_space_needed);
					if (ACPI_FAILURE(status)) {
						/*
						 * Caller's buffer is too small or a new one can't
						 * be allocated
						 */
						ACPI_DEBUG_PRINT((ACPI_DB_INFO,
								  "Needed buffer size %X, %s\n",
								  (u32)
								  buffer_space_needed,
								  acpi_format_exception
								  (status)));
					} else {
						/* We have enough space for the object, build it */

						status =
						    acpi_ut_copy_iobject_to_eobject
						    (info->return_object,
						     return_buffer);
					}
				}
			}
		}
	}

	if (info->return_object) {
		/*
		 * Delete the internal return object. NOTE: Interpreter must be
		 * locked to avoid race condition.
		 */
		acpi_ex_enter_interpreter();

		/* Remove one reference on the return object (should delete it) */

		acpi_ut_remove_reference(info->return_object);
		acpi_ex_exit_interpreter();
	}

cleanup:

	/* Free the input parameter list (if we created one) */

	if (info->parameters) {

		/* Free the allocated parameter block */

		acpi_ut_delete_internal_object_list(info->parameters);
	}

	ACPI_FREE(info);
	return_ACPI_STATUS(status);
}

ACPI_EXPORT_SYMBOL(acpi_evaluate_object)

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_resolve_references
 *
 * PARAMETERS:  info                    - Evaluation info block
 *
 * RETURN:      Info->return_object is replaced with the dereferenced object
 *
 * DESCRIPTION: Dereference certain reference objects. Called before an
 *              internal return object is converted to an external union acpi_object.
 *
 * Performs an automatic dereference of Index and ref_of reference objects.
 * These reference objects are not supported by the union acpi_object, so this is a
 * last resort effort to return something useful. Also, provides compatibility
 * with other ACPI implementations.
 *
 * NOTE: does not handle references within returned package objects or nested
 * references, but this support could be added later if found to be necessary.
 *
 ******************************************************************************/
static void acpi_ns_resolve_references(struct acpi_evaluate_info *info)
{
	union acpi_operand_object *obj_desc = NULL;
	struct acpi_namespace_node *node;

	/* We are interested in reference objects only */

	if ((info->return_object)->common.type != ACPI_TYPE_LOCAL_REFERENCE) {
		return;
	}

	/*
	 * Two types of references are supported - those created by Index and
	 * ref_of operators. A name reference (AML_NAMEPATH_OP) can be converted
	 * to an union acpi_object, so it is not dereferenced here. A ddb_handle
	 * (AML_LOAD_OP) cannot be dereferenced, nor can it be converted to
	 * an union acpi_object.
	 */
	switch (info->return_object->reference.class) {
	case ACPI_REFCLASS_INDEX:

		obj_desc = *(info->return_object->reference.where);
		break;

	case ACPI_REFCLASS_REFOF:

		node = info->return_object->reference.object;
		if (node) {
			obj_desc = node->object;
		}
		break;

	default:

		return;
	}

	/* Replace the existing reference object */

	if (obj_desc) {
		acpi_ut_add_reference(obj_desc);
		acpi_ut_remove_reference(info->return_object);
		info->return_object = obj_desc;
	}

	return;
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_walk_namespace
 *
 * PARAMETERS:  type                - acpi_object_type to search for
 *              start_object        - Handle in namespace where search begins
 *              max_depth           - Depth to which search is to reach
 *              descending_callback - Called during tree descent
 *                                    when an object of "Type" is found
 *              ascending_callback  - Called during tree ascent
 *                                    when an object of "Type" is found
 *              context             - Passed to user function(s) above
 *              return_value        - Location where return value of
 *                                    user_function is put if terminated early
 *
 * RETURNS      Return value from the user_function if terminated early.
 *              Otherwise, returns NULL.
 *
 * DESCRIPTION: Performs a modified depth-first walk of the namespace tree,
 *              starting (and ending) at the object specified by start_handle.
 *              The callback function is called whenever an object that matches
 *              the type parameter is found. If the callback function returns
 *              a non-zero value, the search is terminated immediately and this
 *              value is returned to the caller.
 *
 *              The point of this procedure is to provide a generic namespace
 *              walk routine that can be called from multiple places to
 *              provide multiple services; the callback function(s) can be
 *              tailored to each task, whether it is a print function,
 *              a compare function, etc.
 *
 ******************************************************************************/

acpi_status
acpi_walk_namespace(acpi_object_type type,
		    acpi_handle start_object,
		    u32 max_depth,
		    acpi_walk_callback descending_callback,
		    acpi_walk_callback ascending_callback,
		    void *context, void **return_value)
{
	acpi_status status;

	ACPI_FUNCTION_TRACE(acpi_walk_namespace);

	/* Parameter validation */

	if ((type > ACPI_TYPE_LOCAL_MAX) ||
	    (!max_depth) || (!descending_callback && !ascending_callback)) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	/*
	 * Need to acquire the namespace reader lock to prevent interference
	 * with any concurrent table unloads (which causes the deletion of
	 * namespace objects). We cannot allow the deletion of a namespace node
	 * while the user function is using it. The exception to this are the
	 * nodes created and deleted during control method execution -- these
	 * nodes are marked as temporary nodes and are ignored by the namespace
	 * walk. Thus, control methods can be executed while holding the
	 * namespace deletion lock (and the user function can execute control
	 * methods.)
	 */
	status = acpi_ut_acquire_read_lock(&acpi_gbl_namespace_rw_lock);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/*
	 * Lock the namespace around the walk. The namespace will be
	 * unlocked/locked around each call to the user function - since the user
	 * function must be allowed to make ACPICA calls itself (for example, it
	 * will typically execute control methods during device enumeration.)
	 */
	status = acpi_ut_acquire_mutex(ACPI_MTX_NAMESPACE);
	if (ACPI_FAILURE(status)) {
		goto unlock_and_exit;
	}

	/* Now we can validate the starting node */

	if (!acpi_ns_validate_handle(start_object)) {
		status = AE_BAD_PARAMETER;
		goto unlock_and_exit2;
	}

	status = acpi_ns_walk_namespace(type, start_object, max_depth,
					ACPI_NS_WALK_UNLOCK,
					descending_callback, ascending_callback,
					context, return_value);

unlock_and_exit2:
	(void)acpi_ut_release_mutex(ACPI_MTX_NAMESPACE);

unlock_and_exit:
	(void)acpi_ut_release_read_lock(&acpi_gbl_namespace_rw_lock);
	return_ACPI_STATUS(status);
}

ACPI_EXPORT_SYMBOL(acpi_walk_namespace)

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_get_device_callback
 *
 * PARAMETERS:  Callback from acpi_get_device
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Takes callbacks from walk_namespace and filters out all non-
 *              present devices, or if they specified a HID, it filters based
 *              on that.
 *
 ******************************************************************************/
static acpi_status
acpi_ns_get_device_callback(acpi_handle obj_handle,
			    u32 nesting_level,
			    void *context, void **return_value)
{
	struct acpi_get_devices_info *info = context;
	acpi_status status;
	struct acpi_namespace_node *node;
	u32 flags;
	struct acpi_pnp_device_id *hid;
	struct acpi_pnp_device_id_list *cid;
	u32 i;
	u8 found;
	int no_match;

	status = acpi_ut_acquire_mutex(ACPI_MTX_NAMESPACE);
	if (ACPI_FAILURE(status)) {
		return (status);
	}

	node = acpi_ns_validate_handle(obj_handle);
	status = acpi_ut_release_mutex(ACPI_MTX_NAMESPACE);
	if (ACPI_FAILURE(status)) {
		return (status);
	}

	if (!node) {
		return (AE_BAD_PARAMETER);
	}

	/*
	 * First, filter based on the device HID and CID.
	 *
	 * 01/2010: For this case where a specific HID is requested, we don't
	 * want to run _STA until we have an actual HID match. Thus, we will
	 * not unnecessarily execute _STA on devices for which the caller
	 * doesn't care about. Previously, _STA was executed unconditionally
	 * on all devices found here.
	 *
	 * A side-effect of this change is that now we will continue to search
	 * for a matching HID even under device trees where the parent device
	 * would have returned a _STA that indicates it is not present or
	 * not functioning (thus aborting the search on that branch).
	 */
	if (info->hid != NULL) {
		status = acpi_ut_execute_HID(node, &hid);
		if (status == AE_NOT_FOUND) {
			return (AE_OK);
		} else if (ACPI_FAILURE(status)) {
			return (AE_CTRL_DEPTH);
		}

		no_match = strcmp(hid->string, info->hid);
		ACPI_FREE(hid);

		if (no_match) {
			/*
			 * HID does not match, attempt match within the
			 * list of Compatible IDs (CIDs)
			 */
			status = acpi_ut_execute_CID(node, &cid);
			if (status == AE_NOT_FOUND) {
				return (AE_OK);
			} else if (ACPI_FAILURE(status)) {
				return (AE_CTRL_DEPTH);
			}

			/* Walk the CID list */

			found = FALSE;
			for (i = 0; i < cid->count; i++) {
				if (strcmp(cid->ids[i].string, info->hid) == 0) {

					/* Found a matching CID */

					found = TRUE;
					break;
				}
			}

			ACPI_FREE(cid);
			if (!found) {
				return (AE_OK);
			}
		}
	}

	/* Run _STA to determine if device is present */

	status = acpi_ut_execute_STA(node, &flags);
	if (ACPI_FAILURE(status)) {
		return (AE_CTRL_DEPTH);
	}

	if (!(flags & ACPI_STA_DEVICE_PRESENT) &&
	    !(flags & ACPI_STA_DEVICE_FUNCTIONING)) {
		/*
		 * Don't examine the children of the device only when the
		 * device is neither present nor functional. See ACPI spec,
		 * description of _STA for more information.
		 */
		return (AE_CTRL_DEPTH);
	}

	/* We have a valid device, invoke the user function */

	status = info->user_function(obj_handle, nesting_level, info->context,
				     return_value);
	return (status);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_get_devices
 *
 * PARAMETERS:  HID                 - HID to search for. Can be NULL.
 *              user_function       - Called when a matching object is found
 *              context             - Passed to user function
 *              return_value        - Location where return value of
 *                                    user_function is put if terminated early
 *
 * RETURNS      Return value from the user_function if terminated early.
 *              Otherwise, returns NULL.
 *
 * DESCRIPTION: Performs a modified depth-first walk of the namespace tree,
 *              starting (and ending) at the object specified by start_handle.
 *              The user_function is called whenever an object of type
 *              Device is found. If the user function returns
 *              a non-zero value, the search is terminated immediately and this
 *              value is returned to the caller.
 *
 *              This is a wrapper for walk_namespace, but the callback performs
 *              additional filtering. Please see acpi_ns_get_device_callback.
 *
 ******************************************************************************/

acpi_status
acpi_get_devices(const char *HID,
		 acpi_walk_callback user_function,
		 void *context, void **return_value)
{
	acpi_status status;
	struct acpi_get_devices_info info;

	ACPI_FUNCTION_TRACE(acpi_get_devices);

	/* Parameter validation */

	if (!user_function) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	/*
	 * We're going to call their callback from OUR callback, so we need
	 * to know what it is, and their context parameter.
	 */
	info.hid = HID;
	info.context = context;
	info.user_function = user_function;

	/*
	 * Lock the namespace around the walk.
	 * The namespace will be unlocked/locked around each call
	 * to the user function - since this function
	 * must be allowed to make Acpi calls itself.
	 */
	status = acpi_ut_acquire_mutex(ACPI_MTX_NAMESPACE);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	status = acpi_ns_walk_namespace(ACPI_TYPE_DEVICE, ACPI_ROOT_OBJECT,
					ACPI_UINT32_MAX, ACPI_NS_WALK_UNLOCK,
					acpi_ns_get_device_callback, NULL,
					&info, return_value);

	(void)acpi_ut_release_mutex(ACPI_MTX_NAMESPACE);
	return_ACPI_STATUS(status);
}

ACPI_EXPORT_SYMBOL(acpi_get_devices)

/*******************************************************************************
 *
 * FUNCTION:    acpi_attach_data
 *
 * PARAMETERS:  obj_handle          - Namespace node
 *              handler             - Handler for this attachment
 *              data                - Pointer to data to be attached
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Attach arbitrary data and handler to a namespace node.
 *
 ******************************************************************************/
acpi_status
acpi_attach_data(acpi_handle obj_handle,
		 acpi_object_handler handler, void *data)
{
	struct acpi_namespace_node *node;
	acpi_status status;

	/* Parameter validation */

	if (!obj_handle || !handler || !data) {
		return (AE_BAD_PARAMETER);
	}

	status = acpi_ut_acquire_mutex(ACPI_MTX_NAMESPACE);
	if (ACPI_FAILURE(status)) {
		return (status);
	}

	/* Convert and validate the handle */

	node = acpi_ns_validate_handle(obj_handle);
	if (!node) {
		status = AE_BAD_PARAMETER;
		goto unlock_and_exit;
	}

	status = acpi_ns_attach_data(node, handler, data);

unlock_and_exit:
	(void)acpi_ut_release_mutex(ACPI_MTX_NAMESPACE);
	return (status);
}

ACPI_EXPORT_SYMBOL(acpi_attach_data)

/*******************************************************************************
 *
 * FUNCTION:    acpi_detach_data
 *
 * PARAMETERS:  obj_handle          - Namespace node handle
 *              handler             - Handler used in call to acpi_attach_data
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Remove data that was previously attached to a node.
 *
 ******************************************************************************/
acpi_status
acpi_detach_data(acpi_handle obj_handle, acpi_object_handler handler)
{
	struct acpi_namespace_node *node;
	acpi_status status;

	/* Parameter validation */

	if (!obj_handle || !handler) {
		return (AE_BAD_PARAMETER);
	}

	status = acpi_ut_acquire_mutex(ACPI_MTX_NAMESPACE);
	if (ACPI_FAILURE(status)) {
		return (status);
	}

	/* Convert and validate the handle */

	node = acpi_ns_validate_handle(obj_handle);
	if (!node) {
		status = AE_BAD_PARAMETER;
		goto unlock_and_exit;
	}

	status = acpi_ns_detach_data(node, handler);

unlock_and_exit:
	(void)acpi_ut_release_mutex(ACPI_MTX_NAMESPACE);
	return (status);
}

ACPI_EXPORT_SYMBOL(acpi_detach_data)

/*******************************************************************************
 *
 * FUNCTION:    acpi_get_data_full
 *
 * PARAMETERS:  obj_handle          - Namespace node
 *              handler             - Handler used in call to attach_data
 *              data                - Where the data is returned
 *              callback            - function to execute before returning
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Retrieve data that was previously attached to a namespace node
 *              and execute a callback before returning.
 *
 ******************************************************************************/
acpi_status
acpi_get_data_full(acpi_handle obj_handle, acpi_object_handler handler,
		   void **data, void (*callback)(void *))
{
	struct acpi_namespace_node *node;
	acpi_status status;

	/* Parameter validation */

	if (!obj_handle || !handler || !data) {
		return (AE_BAD_PARAMETER);
	}

	status = acpi_ut_acquire_mutex(ACPI_MTX_NAMESPACE);
	if (ACPI_FAILURE(status)) {
		return (status);
	}

	/* Convert and validate the handle */

	node = acpi_ns_validate_handle(obj_handle);
	if (!node) {
		status = AE_BAD_PARAMETER;
		goto unlock_and_exit;
	}

	status = acpi_ns_get_attached_data(node, handler, data);
	if (ACPI_SUCCESS(status) && callback) {
		callback(*data);
	}

unlock_and_exit:
	(void)acpi_ut_release_mutex(ACPI_MTX_NAMESPACE);
	return (status);
}

ACPI_EXPORT_SYMBOL(acpi_get_data_full)

/*******************************************************************************
 *
 * FUNCTION:    acpi_get_data
 *
 * PARAMETERS:  obj_handle          - Namespace node
 *              handler             - Handler used in call to attach_data
 *              data                - Where the data is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Retrieve data that was previously attached to a namespace node.
 *
 ******************************************************************************/
acpi_status
acpi_get_data(acpi_handle obj_handle, acpi_object_handler handler, void **data)
{
	return acpi_get_data_full(obj_handle, handler, data, NULL);
}

ACPI_EXPORT_SYMBOL(acpi_get_data)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      /******************************************************************************
 *
 * Module Name: nsxfname - Public interfaces to the ACPI subsystem
 *                         ACPI Namespace oriented interfaces
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

#define EXPORT_ACPI_INTERFACES

#include <acpi/acpi.h>
#include "accommon.h"
#include "acnamesp.h"
#include "acparser.h"
#include "amlcode.h"

#define _COMPONENT          ACPI_NAMESPACE
ACPI_MODULE_NAME("nsxfname")

/* Local prototypes */
static char *acpi_ns_copy_device_id(struct acpi_pnp_device_id *dest,
				    struct acpi_pnp_device_id *source,
				    char *string_area);

/******************************************************************************
 *
 * FUNCTION:    acpi_get_handle
 *
 * PARAMETERS:  parent          - Object to search under (search scope).
 *              pathname        - Pointer to an asciiz string containing the
 *                                name
 *              ret_handle      - Where the return handle is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: This routine will search for a caller specified name in the
 *              name space. The caller can restrict the search region by
 *              specifying a non NULL parent. The parent value is itself a
 *              namespace handle.
 *
 ******************************************************************************/

acpi_status
acpi_get_handle(acpi_handle parent,
		acpi_string pathname, acpi_handle * ret_handle)
{
	acpi_status status;
	struct acpi_namespace_node *node = NULL;
	struct acpi_namespace_node *prefix_node = NULL;

	ACPI_FUNCTION_ENTRY();

	/* Parameter Validation */

	if (!ret_handle || !pathname) {
		return (AE_BAD_PARAMETER);
	}

	/* Convert a parent handle to a prefix node */

	if (parent) {
		prefix_node = acpi_ns_validate_handle(parent);
		if (!prefix_node) {
			return (AE_BAD_PARAMETER);
		}
	}

	/*
	 * Valid cases are:
	 * 1) Fully qualified pathname
	 * 2) Parent + Relative pathname
	 *
	 * Error for <null Parent + relative path>
	 */
	if (ACPI_IS_ROOT_PREFIX(pathname[0])) {

		/* Pathname is fully qualified (starts with '\') */

		/* Special case for root-only, since we can't search for it */

		if (!strcmp(pathname, ACPI_NS_ROOT_PATH)) {
			*ret_handle =
			    ACPI_CAST_PTR(acpi_handle, acpi_gbl_root_node);
			return (AE_OK);
		}
	} else if (!prefix_node) {

		/* Relative path with null prefix is disallowed */

		return (AE_BAD_PARAMETER);
	}

	/* Find the Node and convert to a handle */

	status =
	    acpi_ns_get_node(prefix_node, pathname, ACPI_NS_NO_UPSEARCH, &node);
	if (ACPI_SUCCESS(status)) {
		*ret_handle = ACPI_CAST_PTR(acpi_handle, node);
	}

	return (status);
}

ACPI_EXPORT_SYMBOL(acpi_get_handle)

/******************************************************************************
 *
 * FUNCTION:    acpi_get_name
 *
 * PARAMETERS:  handle          - Handle to be converted to a pathname
 *              name_type       - Full pathname or single segment
 *              buffer          - Buffer for returned path
 *
 * RETURN:      Pointer to a string containing the fully qualified Name.
 *
 * DESCRIPTION: This routine returns the fully qualified name associated with
 *              the Handle parameter. This and the acpi_pathname_to_handle are
 *              complementary functions.
 *
 ******************************************************************************/
acpi_status
acpi_get_name(acpi_handle handle, u32 name_type, struct acpi_buffer * buffer)
{
	acpi_status status;
	struct acpi_namespace_node *node;
	char *node_name;

	/* Parameter validation */

	if (name_type > ACPI_NAME_TYPE_MAX) {
		return (AE_BAD_PARAMETER);
	}

	status = acpi_ut_validate_buffer(buffer);
	if (ACPI_FAILURE(status)) {
		return (status);
	}

	if (name_type == ACPI_FULL_PATHNAME ||
	    name_type == ACPI_FULL_PATHNAME_NO_TRAILING) {

		/* Get the full pathname (From the namespace root) */

		status = acpi_ns_handle_to_pathname(handle, buffer,
						    name_type ==
						    ACPI_FULL_PATHNAME ? FALSE :
						    TRUE);
		return (status);
	}

	/*
	 * Wants the single segment ACPI name.
	 * Validate handle and convert to a namespace Node
	 */
	status = acpi_ut_acquire_mutex(ACPI_MTX_NAMESPACE);
	if (ACPI_FAILURE(status)) {
		return (status);
	}

	node = acpi_ns_validate_handle(handle);
	if (!node) {
		status = AE_BAD_PARAMETER;
		goto unlock_and_exit;
	}

	/* Validate/Allocate/Clear caller buffer */

	status = acpi_ut_initialize_buffer(buffer, ACPI_PATH_SEGMENT_LENGTH);
	if (ACPI_FAILURE(status)) {
		goto unlock_and_exit;
	}

	/* Just copy the ACPI name from the Node and zero terminate it */

	node_name = acpi_ut_get_node_name(node);
	ACPI_MOVE_NAME(buffer->pointer, node_name);
	((char *)buffer->pointer)[ACPI_NAME_SIZE] = 0;
	status = AE_OK;

unlock_and_exit:

	(void)acpi_ut_release_mutex(ACPI_MTX_NAMESPACE);
	return (status);
}

ACPI_EXPORT_SYMBOL(acpi_get_name)

/******************************************************************************
 *
 * FUNCTION:    acpi_ns_copy_device_id
 *
 * PARAMETERS:  dest                - Pointer to the destination PNP_DEVICE_ID
 *              source              - Pointer to the source PNP_DEVICE_ID
 *              string_area         - Pointer to where to copy the dest string
 *
 * RETURN:      Pointer to the next string area
 *
 * DESCRIPTION: Copy a single PNP_DEVICE_ID, including the string data.
 *
 ******************************************************************************/
static char *acpi_ns_copy_device_id(struct acpi_pnp_device_id *dest,
				    struct acpi_pnp_device_id *source,
				    char *string_area)
{

	/* Create the destination PNP_DEVICE_ID */

	dest->string = string_area;
	dest->length = source->length;

	/* Copy actual string and return a pointer to the next string area */

	memcpy(string_area, source->string, source->length);
	return (string_area + source->length);
}

/******************************************************************************
 *
 * FUNCTION:    acpi_get_object_info
 *
 * PARAMETERS:  handle              - Object Handle
 *              return_buffer       - Where the info is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Returns information about an object as gleaned from the
 *              namespace node and possibly by running several standard
 *              control methods (Such as in the case of a device.)
 *
 * For Device and Processor objects, run the Device _HID, _UID, _CID, _SUB,
 * _CLS, _STA, _ADR, _sx_w, and _sx_d methods.
 *
 * Note: Allocates the return buffer, must be freed by the caller.
 *
 ******************************************************************************/

acpi_status
acpi_get_object_info(acpi_handle handle,
		     struct acpi_device_info **return_buffer)
{
	struct acpi_namespace_node *node;
	struct acpi_device_info *info;
	struct acpi_pnp_device_id_list *cid_list = NULL;
	struct acpi_pnp_device_id *hid = NULL;
	struct acpi_pnp_device_id *uid = NULL;
	struct acpi_pnp_device_id *sub = NULL;
	struct acpi_pnp_device_id *cls = NULL;
	char *next_id_string;
	acpi_object_type type;
	acpi_name name;
	u8 param_count = 0;
	u16 valid = 0;
	u32 info_size;
	u32 i;
	acpi_status status;

	/* Parameter validation */

	if (!handle || !return_buffer) {
		return (AE_BAD_PARAMETER);
	}

	status = acpi_ut_acquire_mutex(ACPI_MTX_NAMESPACE);
	if (ACPI_FAILURE(status)) {
		return (status);
	}

	node = acpi_ns_validate_handle(handle);
	if (!node) {
		(void)acpi_ut_release_mutex(ACPI_MTX_NAMESPACE);
		return (AE_BAD_PARAMETER);
	}

	/* Get the namespace node data while the namespace is locked */

	info_size = sizeof(struct acpi_device_info);
	type = node->type;
	name = node->name.integer;

	if (node->type == ACPI_TYPE_METHOD) {
		param_count = node->object->method.param_count;
	}

	status = acpi_ut_release_mutex(ACPI_MTX_NAMESPACE);
	if (ACPI_FAILURE(status)) {
		return (status);
	}

	if ((type == ACPI_TYPE_DEVICE) || (type == ACPI_TYPE_PROCESSOR)) {
		/*
		 * Get extra info for ACPI Device/Processor objects only:
		 * Run the Device _HID, _UID, _SUB, _CID, and _CLS methods.
		 *
		 * Note: none of these methods are required, so they may or may
		 * not be present for this device. The Info->Valid bitfield is used
		 * to indicate which methods were found and run successfully.
		 */

		/* Execute the Device._HID method */

		status = acpi_ut_execute_HID(node, &hid);
		if (ACPI_SUCCESS(status)) {
			info_size += hid->length;
			valid |= ACPI_VALID_HID;
		}

		/* Execute the Device._UID method */

		status = acpi_ut_execute_UID(node, &uid);
		if (ACPI_SUCCESS(status)) {
			info_size += uid->length;
			valid |= ACPI_VALID_UID;
		}

		/* Execute the Device._SUB method */

		status = acpi_ut_execute_SUB(node, &sub);
		if (ACPI_SUCCESS(status)) {
			info_size += sub->length;
			valid |= ACPI_VALID_SUB;
		}

		/* Execute the Device._CID method */

		status = acpi_ut_execute_CID(node, &cid_list);
		if (ACPI_SUCCESS(status)) {

			/* Add size of CID strings and CID pointer array */

			info_size +=
			    (cid_list->list_size -
			     sizeof(struct acpi_pnp_device_id_list));
			valid |= ACPI_VALID_CID;
		}

		/* Execute the Device._CLS method */

		status = acpi_ut_execute_CLS(node, &cls);
		if (ACPI_SUCCESS(status)) {
			info_size += cls->length;
			valid |= ACPI_VALID_CLS;
		}
	}

	/*
	 * Now that we have the variable-length data, we can allocate the
	 * return buffer
	 */
	info = ACPI_ALLOCATE_ZEROED(info_size);
	if (!info) {
		status = AE_NO_MEMORY;
		goto cleanup;
	}

	/* Get the fixed-length data */

	if ((type == ACPI_TYPE_DEVICE) || (type == ACPI_TYPE_PROCESSOR)) {
		/*
		 * Get extra info for ACPI Device/Processor objects only:
		 * Run the _STA, _ADR and, sx_w, and _sx_d methods.
		 *
		 * Notes: none of these methods are required, so they may or may
		 * not be present for this device. The Info->Valid bitfield is used
		 * to indicate which methods were found and run successfully.
		 *
		 * For _STA, if the method does not exist, then (as per the ACPI
		 * specification), the returned current_status flags will indicate
		 * that the device is present/functional/enabled. Otherwise, the
		 * current_status flags reflect the value returned from _STA.
		 */

		/* Execute the Device._STA method */

		status = acpi_ut_execute_STA(node, &info->current_status);
		if (ACPI_SUCCESS(status)) {
			valid |= ACPI_VALID_STA;
		}

		/* Execute the Device._ADR method */

		status = acpi_ut_evaluate_numeric_object(METHOD_NAME__ADR, node,
							 &info->address);
		if (ACPI_SUCCESS(status)) {
			valid |= ACPI_VALID_ADR;
		}

		/* Execute the Device._sx_w methods */

		status = acpi_ut_execute_power_methods(node,
						       acpi_gbl_lowest_dstate_names,
						       ACPI_NUM_sx_w_METHODS,
						       info->lowest_dstates);
		if (ACPI_SUCCESS(status)) {
			valid |= ACPI_VALID_SXWS;
		}

		/* Execute the Device._sx_d methods */

		status = acpi_ut_execute_power_methods(node,
						       acpi_gbl_highest_dstate_names,
						       ACPI_NUM_sx_d_METHODS,
						       info->highest_dstates);
		if (ACPI_SUCCESS(status)) {
			valid |= ACPI_VALID_SXDS;
		}
	}

	/*
	 * Create a pointer to the string area of the return buffer.
	 * Point to the end of the base struct acpi_device_info structure.
	 */
	next_id_string = ACPI_CAST_PTR(char, info->compatible_id_list.ids);
	if (cid_list) {

		/* Point past the CID PNP_DEVICE_ID array */

		next_id_string +=
		    ((acpi_size) cid_list->count *
		     sizeof(struct acpi_pnp_device_id));
	}

	/*
	 * Copy the HID, UID, SUB, and CIDs to the return buffer.
	 * The variable-length strings are copied to the reserved area
	 * at the end of the buffer.
	 *
	 * For HID and CID, check if the ID is a PCI Root Bridge.
	 */
	if (hid) {
		next_id_string = acpi_ns_copy_device_id(&info->hardware_id,
							hid, next_id_string);

		if (acpi_ut_is_pci_root_bridge(hid->string)) {
			info->flags |= ACPI_PCI_ROOT_BRIDGE;
		}
	}

	if (uid) {
		next_id_string = acpi_ns_copy_device_id(&info->unique_id,
							uid, next_id_string);
	}

	if (sub) {
		next_id_string = acpi_ns_copy_device_id(&info->subsystem_id,
							sub, next_id_string);
	}

	if (cid_list) {
		info->compatible_id_list.count = cid_list->count;
		info->compatible_id_list.list_size = cid_list->list_size;

		/* Copy each CID */

		for (i = 0; i < cid_list->count; i++) {
			next_id_string =
			    acpi_ns_copy_device_id(&info->compatible_id_list.
						   ids[i], &cid_list->ids[i],
						   next_id_string);

			if (acpi_ut_is_pci_root_bridge(cid_list->ids[i].string)) {
				info->flags |= ACPI_PCI_ROOT_BRIDGE;
			}
		}
	}

	if (cls) {
		next_id_string = acpi_ns_copy_device_id(&info->class_code,
							cls, next_id_string);
	}

	/* Copy the fixed-length data */

	info->info_size = info_size;
	info->type = type;
	info->name = name;
	info->param_count = param_count;
	info->valid = valid;

	*return_buffer = info;
	status = AE_OK;

cleanup:
	if (hid) {
		ACPI_FREE(hid);
	}
	if (uid) {
		ACPI_FREE(uid);
	}
	if (sub) {
		ACPI_FREE(sub);
	}
	if (cid_list) {
		ACPI_FREE(cid_list);
	}
	if (cls) {
		ACPI_FREE(cls);
	}
	return (status);
}

ACPI_EXPORT_SYMBOL(acpi_get_object_info)

/******************************************************************************
 *
 * FUNCTION:    acpi_install_method
 *
 * PARAMETERS:  buffer         - An ACPI table containing one control method
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Install a control method into the namespace. If the method
 *              name already exists in the namespace, it is overwritten. The
 *              input buffer must contain a valid DSDT or SSDT containing a
 *              single control method.
 *
 ******************************************************************************/
acpi_status acpi_install_method(u8 *buffer)
{
	struct acpi_table_header *table =
	    ACPI_CAST_PTR(struct acpi_table_header, buffer);
	u8 *aml_buffer;
	u8 *aml_start;
	char *path;
	struct acpi_namespace_node *node;
	union acpi_operand_object *method_obj;
	struct acpi_parse_state parser_state;
	u32 aml_length;
	u16 opcode;
	u8 method_flags;
	acpi_status status;

	/* Parameter validation */

	if (!buffer) {
		return (AE_BAD_PARAMETER);
	}

	/* Table must be a DSDT or SSDT */

	if (!ACPI_COMPARE_NAME(table->signature, ACPI_SIG_DSDT) &&
	    !ACPI_COMPARE_NAME(table->signature, ACPI_SIG_SSDT)) {
		return (AE_BAD_HEADER);
	}

	/* First AML opcode in the table must be a control method */

	parser_state.aml = buffer + sizeof(struct acpi_table_header);
	opcode = acpi_ps_peek_opcode(&parser_state);
	if (opcode != AML_METHOD_OP) {
		return (AE_BAD_PARAMETER);
	}

	/* Extract method information from the raw AML */

	parser_state.aml += acpi_ps_get_opcode_size(opcode);
	parser_state.pkg_end = acpi_ps_get_next_package_end(&parser_state);
	path = acpi_ps_get_next_namestring(&parser_state);
	method_flags = *parser_state.aml++;
	aml_start = parser_state.aml;
	aml_length = ACPI_PTR_DIFF(parser_state.pkg_end, aml_start);

	/*
	 * Allocate resources up-front. We don't want to have to delete a new
	 * node from the namespace if we cannot allocate memory.
	 */
	aml_buffer = ACPI_ALLOCATE(aml_length);
	if (!aml_buffer) {
		return (AE_NO_MEMORY);
	}

	method_obj = acpi_ut_create_internal_object(ACPI_TYPE_METHOD);
	if (!method_obj) {
		ACPI_FREE(aml_buffer);
		return (AE_NO_MEMORY);
	}

	/* Lock namespace for acpi_ns_lookup, we may be creating a new node */

	status = acpi_ut_acquire_mutex(ACPI_MTX_NAMESPACE);
	if (ACPI_FAILURE(status)) {
		goto error_exit;
	}

	/* The lookup either returns an existing node or creates a new one */

	status =
	    acpi_ns_lookup(NULL, path, ACPI_TYPE_METHOD, ACPI_IMODE_LOAD_PASS1,
			   ACPI_NS_DONT_OPEN_SCOPE | ACPI_NS_ERROR_IF_FOUND,
			   NULL, &node);

	(void)acpi_ut_release_mutex(ACPI_MTX_NAMESPACE);

	if (ACPI_FAILURE(status)) {	/* ns_lookup */
		if (status != AE_ALREADY_EXISTS) {
			goto error_exit;
		}

		/* Node existed previously, make sure it is a method node */

		if (node->type != ACPI_TYPE_METHOD) {
			status = AE_TYPE;
			goto error_exit;
		}
	}

	/* Copy the method AML to the local buffer */

	memcpy(aml_buffer, aml_start, aml_length);

	/* Initialize the method object with the new method's information */

	method_obj->method.aml_start = aml_buffer;
	method_obj->method.aml_length = aml_length;

	method_obj->method.param_count = (u8)
	    (method_flags & AML_METHOD_ARG_COUNT);

	if (method_flags & AML_METHOD_SERIALIZED) {
		method_obj->method.info_flags = ACPI_METHOD_SERIALIZED;

		method_obj->method.sync_level = (u8)
		    ((method_flags & AML_METHOD_SYNC_LEVEL) >> 4);
	}

	/*
	 * Now that it is complete, we can attach the new method object to
	 * the method Node (detaches/deletes any existing object)
	 */
	status = acpi_ns_attach_object(node, method_obj, ACPI_TYPE_METHOD);

	/*
	 * Flag indicates AML buffer is dynamic, must be deleted later.
	 * Must be set only after attach above.
	 */
	node->flags |= ANOBJ_ALLOCATED_BUFFER;

	/* Remove local reference to the method object */

	acpi_ut_remove_reference(method_obj);
	return (status);

error_exit:

	ACPI_FREE(aml_buffer);
	ACPI_FREE(method_obj);
	return (status);
}
ACPI_EXPORT_SYMBOL(acpi_install_method)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          /*******************************************************************************
 *
 * Module Name: nsxfobj - Public interfaces to the ACPI subsystem
 *                         ACPI Object oriented interfaces
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

#define EXPORT_ACPI_INTERFACES

#include <acpi/acpi.h>
#include "accommon.h"
#include "acnamesp.h"

#define _COMPONENT          ACPI_NAMESPACE
ACPI_MODULE_NAME("nsxfobj")

/*******************************************************************************
 *
 * FUNCTION:    acpi_get_type
 *
 * PARAMETERS:  handle          - Handle of object whose type is desired
 *              ret_type        - Where the type will be placed
 *
 * RETURN:      Status
 *
 * DESCRIPTION: This routine returns the type associatd with a particular handle
 *
 ******************************************************************************/
acpi_status acpi_get_type(acpi_handle handle, acpi_object_type * ret_type)
{
	struct acpi_namespace_node *node;
	acpi_status status;

	/* Parameter Validation */

	if (!ret_type) {
		return (AE_BAD_PARAMETER);
	}

	/*
	 * Special case for the predefined Root Node
	 * (return type ANY)
	 */
	if (handle == ACPI_ROOT_OBJECT) {
		*ret_type = ACPI_TYPE_ANY;
		return (AE_OK);
	}

	status = acpi_ut_acquire_mutex(ACPI_MTX_NAMESPACE);
	if (ACPI_FAILURE(status)) {
		return (status);
	}

	/* Convert and validate the handle */

	node = acpi_ns_validate_handle(handle);
	if (!node) {
		(void)acpi_ut_release_mutex(ACPI_MTX_NAMESPACE);
		return (AE_BAD_PARAMETER);
	}

	*ret_type = node->type;

	status = acpi_ut_release_mutex(ACPI_MTX_NAMESPACE);
	return (status);
}

ACPI_EXPORT_SYMBOL(acpi_get_type)

/*******************************************************************************
 *
 * FUNCTION:    acpi_get_parent
 *
 * PARAMETERS:  handle          - Handle of object whose parent is desired
 *              ret_handle      - Where the parent handle will be placed
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Returns a handle to the parent of the object represented by
 *              Handle.
 *
 ******************************************************************************/
acpi_status acpi_get_parent(acpi_handle handle, acpi_handle * ret_handle)
{
	struct acpi_namespace_node *node;
	struct acpi_namespace_node *parent_node;
	acpi_status status;

	if (!ret_handle) {
		return (AE_BAD_PARAMETER);
	}

	/* Special case for the predefined Root Node (no parent) */

	if (handle == ACPI_ROOT_OBJECT) {
		return (AE_NULL_ENTRY);
	}

	status = acpi_ut_acquire_mutex(ACPI_MTX_NAMESPACE);
	if (ACPI_FAILURE(status)) {
		return (status);
	}

	/* Convert and validate the handle */

	node = acpi_ns_validate_handle(handle);
	if (!node) {
		status = AE_BAD_PARAMETER;
		goto unlock_and_exit;
	}

	/* Get the parent entry */

	parent_node = node->parent;
	*ret_handle = ACPI_CAST_PTR(acpi_handle, parent_node);

	/* Return exception if parent is null */

	if (!parent_node) {
		status = AE_NULL_ENTRY;
	}

unlock_and_exit:

	(void)acpi_ut_release_mutex(ACPI_MTX_NAMESPACE);
	return (status);
}

ACPI_EXPORT_SYMBOL(acpi_get_parent)

/*******************************************************************************
 *
 * FUNCTION:    acpi_get_next_object
 *
 * PARAMETERS:  type            - Type of object to be searched for
 *              parent          - Parent object whose children we are getting
 *              last_child      - Previous child that was found.
 *                                The NEXT child will be returned
 *              ret_handle      - Where handle to the next object is placed
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Return the next peer object within the namespace. If Handle is
 *              valid, Scope is ignored. Otherwise, the first object within
 *              Scope is returned.
 *
 ******************************************************************************/
acpi_status
acpi_get_next_object(acpi_object_type type,
		     acpi_handle parent,
		     acpi_handle child, acpi_handle * ret_handle)
{
	acpi_status status;
	struct acpi_namespace_node *node;
	struct acpi_namespace_node *parent_node = NULL;
	struct acpi_namespace_node *child_node = NULL;

	/* Parameter validation */

	if (type > ACPI_TYPE_EXTERNAL_MAX) {
		return (AE_BAD_PARAMETER);
	}

	status = acpi_ut_acquire_mutex(ACPI_MTX_NAMESPACE);
	if (ACPI_FAILURE(status)) {
		return (status);
	}

	/* If null handle, use the parent */

	if (!child) {

		/* Start search at the beginning of the specified scope */

		parent_node = acpi_ns_validate_handle(parent);
		if (!parent_node) {
			status = AE_BAD_PARAMETER;
			goto unlock_and_exit;
		}
	} else {
		/* Non-null handle, ignore the parent */
		/* Convert and validate the handle */

		child_node = acpi_ns_validate_handle(child);
		if (!child_node) {
			status = AE_BAD_PARAMETER;
			goto unlock_and_exit;
		}
	}

	/* Internal function does the real work */

	node = acpi_ns_get_next_node_typed(type, parent_node, child_node);
	if (!node) {
		status = AE_NOT_FOUND;
		goto unlock_and_exit;
	}

	if (ret_handle) {
		*ret_handle = ACPI_CAST_PTR(acpi_handle, node);
	}

unlock_and_exit:

	(void)acpi_ut_release_mutex(ACPI_MTX_NAMESPACE);
	return (status);
}

ACPI_EXPORT_SYMBOL(acpi_get_next_object)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    /******************************************************************************
 *
 * Module Name: psargs - Parse AML opcode arguments
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
#include "acparser.h"
#include "amlcode.h"
#include "acnamesp.h"
#include "acdispat.h"

#define _COMPONENT          ACPI_PARSER
ACPI_MODULE_NAME("psargs")

/* Local prototypes */
static u32
acpi_ps_get_next_package_length(struct acpi_parse_state *parser_state);

static union acpi_parse_object *acpi_ps_get_next_field(struct acpi_parse_state
						       *parser_state);

/*******************************************************************************
 *
 * FUNCTION:    acpi_ps_get_next_package_length
 *
 * PARAMETERS:  parser_state        - Current parser state object
 *
 * RETURN:      Decoded package length. On completion, the AML pointer points
 *              past the length byte or bytes.
 *
 * DESCRIPTION: Decode and return a package length field.
 *              Note: Largest package length is 28 bits, from ACPI specification
 *
 ******************************************************************************/

static u32
acpi_ps_get_next_package_length(struct acpi_parse_state *parser_state)
{
	u8 *aml = parser_state->aml;
	u32 package_length = 0;
	u32 byte_count;
	u8 byte_zero_mask = 0x3F;	/* Default [0:5] */

	ACPI_FUNCTION_TRACE(ps_get_next_package_length);

	/*
	 * Byte 0 bits [6:7] contain the number of additional bytes
	 * used to encode the package length, either 0,1,2, or 3
	 */
	byte_count = (aml[0] >> 6);
	parser_state->aml += ((acpi_size) byte_count + 1);

	/* Get bytes 3, 2, 1 as needed */

	while (byte_count) {
		/*
		 * Final bit positions for the package length bytes:
		 *      Byte3->[20:27]
		 *      Byte2->[12:19]
		 *      Byte1->[04:11]
		 *      Byte0->[00:03]
		 */
		package_length |= (aml[byte_count] << ((byte_count << 3) - 4));

		byte_zero_mask = 0x0F;	/* Use bits [0:3] of byte 0 */
		byte_count--;
	}

	/* Byte 0 is a special case, either bits [0:3] or [0:5] are used */

	package_length |= (aml[0] & byte_zero_mask);
	return_UINT32(package_length);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ps_get_next_package_end
 *
 * PARAMETERS:  parser_state        - Current parser state object
 *
 * RETURN:      Pointer to end-of-package +1
 *
 * DESCRIPTION: Get next package length and return a pointer past the end of
 *              the package. Consumes the package length field
 *
 ******************************************************************************/

u8 *acpi_ps_get_next_package_end(struct acpi_parse_state *parser_state)
{
	u8 *start = parser_state->aml;
	u32 package_length;

	ACPI_FUNCTION_TRACE(ps_get_next_package_end);

	/* Function below updates parser_state->Aml */

	package_length = acpi_ps_get_next_package_length(parser_state);

	return_PTR(start + package_length);	/* end of package */
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ps_get_next_namestring
 *
 * PARAMETERS:  parser_state        - Current parser state object
 *
 * RETURN:      Pointer to the start of the name string (pointer points into
 *              the AML.
 *
 * DESCRIPTION: Get next raw namestring within the AML stream. Handles all name
 *              prefix characters. Set parser state to point past the string.
 *              (Name is consumed from the AML.)
 *
 ******************************************************************************/

char *acpi_ps_get_next_namestring(struct acpi_parse_state *parser_state)
{
	u8 *start = parser_state->aml;
	u8 *end = parser_state->aml;

	ACPI_FUNCTION_TRACE(ps_get_next_namestring);

	/* Point past any namestring prefix characters (backslash or carat) */

	while (ACPI_IS_ROOT_PREFIX(*end) || ACPI_IS_PARENT_PREFIX(*end)) {
		end++;
	}

	/* Decode the path prefix character */

	switch (*end) {
	case 0:

		/* null_name */

		if (end == start) {
			start = NULL;
		}
		end++;
		break;

	case AML_DUAL_NAME_PREFIX:

		/* Two name segments */

		end += 1 + (2 * ACPI_NAME_SIZE);
		break;

	case AML_MULTI_NAME_PREFIX_OP:

		/* Multiple name segments, 4 chars each, count in next byte */

		end += 2 + (*(end + 1) * ACPI_NAME_SIZE);
		break;

	default:

		/* Single name segment */

		end += ACPI_NAME_SIZE;
		break;
	}

	parser_state->aml = end;
	return_PTR((char *)start);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ps_get_next_namepath
 *
 * PARAMETERS:  parser_state        - Current parser state object
 *              arg                 - Where the namepath will be stored
 *              arg_count           - If the namepath points to a control method
 *                                    the method's argument is returned here.
 *              possible_method_call - Whether the namepath can possibly be the
 *                                    start of a method call
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Get next name (if method call, return # of required args).
 *              Names are looked up in the internal namespace to determine
 *              if the name represents a control method. If a method
 *              is found, the number of arguments to the method is returned.
 *              This information is critical for parsing to continue correctly.
 *
 ******************************************************************************/

acpi_status
acpi_ps_get_next_namepath(struct acpi_walk_state *walk_state,
			  struct acpi_parse_state *parser_state,
			  union acpi_parse_object *arg, u8 possible_method_call)
{
	acpi_status status;
	char *path;
	union acpi_parse_object *name_op;
	union acpi_operand_object *method_desc;
	struct acpi_namespace_node *node;
	u8 *start = parser_state->aml;

	ACPI_FUNCTION_TRACE(ps_get_next_namepath);

	path = acpi_ps_get_next_namestring(parser_state);
	acpi_ps_init_op(arg, AML_INT_NAMEPATH_OP);

	/* Null path case is allowed, just exit */

	if (!path) {
		arg->common.value.name = path;
		return_ACPI_STATUS(AE_OK);
	}

	/*
	 * Lookup the name in the internal namespace, starting with the current
	 * scope. We don't want to add anything new to the namespace here,
	 * however, so we use MODE_EXECUTE.
	 * Allow searching of the parent tree, but don't open a new scope -
	 * we just want to lookup the object (must be mode EXECUTE to perform
	 * the upsearch)
	 */
	status = acpi_ns_lookup(walk_state->scope_info, path,
				ACPI_TYPE_ANY, ACPI_IMODE_EXECUTE,
				ACPI_NS_SEARCH_PARENT | ACPI_NS_DONT_OPEN_SCOPE,
				NULL, &node);

	/*
	 * If this name is a control method invocation, we must
	 * setup the method call
	 */
	if (ACPI_SUCCESS(status) &&
	    possible_method_call && (node->type == ACPI_TYPE_METHOD)) {
		if (walk_state->opcode == AML_UNLOAD_OP) {
			/*
			 * acpi_ps_get_next_namestring has increased the AML pointer,
			 * so we need to restore the saved AML pointer for method call.
			 */
			walk_state->parser_state.aml = start;
			walk_state->arg_count = 1;
			acpi_ps_init_op(arg, AML_INT_METHODCALL_OP);
			return_ACPI_STATUS(AE_OK);
		}

		/* This name is actually a control method invocation */

		method_desc = acpi_ns_get_attached_object(node);
		ACPI_DEBUG_PRINT((ACPI_DB_PARSE,
				  "Control Method - %p Desc %p Path=%p\n", node,
				  method_desc, path));

		name_op = acpi_ps_alloc_op(AML_INT_NAMEPATH_OP, start);
		if (!name_op) {
			return_ACPI_STATUS(AE_NO_MEMORY);
		}