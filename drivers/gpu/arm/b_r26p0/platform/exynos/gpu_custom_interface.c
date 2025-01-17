n acpi_operand_object *new_desc;
	acpi_object_type target_type;

	ACPI_FUNCTION_TRACE_PTR(ex_store_object_to_node, source_desc);

	/* Get current type of the node, and object attached to Node */

	target_type = acpi_ns_get_type(node);
	target_desc = acpi_ns_get_attached_object(node);

	ACPI_DEBUG_PRINT((ACPI_DB_EXEC, "Storing %p [%s] to node %p [%s]\n",
			  source_desc,
			  acpi_ut_get_object_type_name(source_desc), node,
			  acpi_ut_get_type_name(target_type)));

	/* Only limited target types possible for everything except copy_object */

	if (walk_state->opcode != AML_COPY_OP) {
		/*
		 * Only copy_object allows all object types to be overwritten. For
		 * target_ref(s), there are restrictions on the object types that
		 * are allowed.
		 *
		 * Allowable operations/typing for Store:
		 *
		 * 1) Simple Store
		 *      Integer     --> Integer (Named/Local/Arg)
		 *      String      --> String  (Named/Local/Arg)
		 *      Buffer      --> Buffer  (Named/Local/Arg)
		 *      Package     --> Package (Named/Local/Arg)
		 *
		 * 2) Store with implicit conversion
		 *      Integer     --> String or Buffer  (Named)
		 *      String      --> Integer or Buffer (Named)
		 *      Buffer      --> Integer or String (Named)
		 */
		switch (target_type) {
		case ACPI_TYPE_PACKAGE:
			/*
			 * Here, can only store a package to an existing package.
			 * Storing a package to a Local/Arg is OK, and handled
			 * elsewhere.
			 */
			if (walk_state->opcode == AML_STORE_OP) {
				if (source_desc->common.type !=
				    ACPI_TYPE_PACKAGE) {
					ACPI_ERROR((AE_INFO,
						    "Cannot assign type [%s] to [Package] "
						    "(source must be type Pkg)",
						    acpi_ut_get_object_type_name
						    (source_desc)));

					return_ACPI_STATUS(AE_AML_TARGET_TYPE);
				}
				break;
			}

			/* Fallthrough */

		case ACPI_TYPE_DEVICE:
		case ACPI_TYPE_EVENT:
		case ACPI_TYPE_MUTEX:
		case ACPI_TYPE_REGION:
		case ACPI_TYPE_POWER:
		case ACPI_TYPE_PROCESSOR:
		case ACPI_TYPE_THERMAL:

			ACPI_ERROR((AE_INFO,
				    "Target must be [Buffer/Integer/String/Reference], found [%s] (%4.4s)",
				    acpi_ut_get_type_name(node->type),
				    node->name.ascii));

			return_ACPI_STATUS(AE_AML_TARGET_TYPE);

		default:
			break;
		}
	}

	/*
	 * Resolve the source object to an actual value
	 * (If it is a reference object)
	 */
	status = acpi_ex_resolve_object(&source_desc, target_type, walk_state);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/* Do the actual store operation */

	switch (target_type) {
		/*
		 * The simple data types all support implicit source operand
		 * conversion before the store.
		 */
	case ACPI_TYPE_INTEGER:
	case ACPI_TYPE_STRING:
	case ACPI_TYPE_BUFFER:

		if ((walk_state->opcode == AML_COPY_OP) || !implicit_conversion) {
			/*
			 * However, copy_object and Stores to arg_x do not perform
			 * an implicit conversion, as per the ACPI specification.
			 * A direct store is performed instead.
			 */
			status = acpi_ex_store_direct_to_node(source_desc, node,
							      walk_state);
			break;
		}

		/* Store with implicit source operand conversion support */

		status =
		    acpi_ex_store_object_to_object(source_desc, target_desc,
						   &new_desc, walk_state);
		if (ACPI_FAILURE(status)) {
			return_ACPI_STATUS(status);
		}

		if (new_desc != target_desc) {
			/*
			 * Store the new new_desc as the new value of the Name, and set
			 * the Name's type to that of the value being stored in it.
			 * source_desc reference count is incremented by attach_object.
			 *
			 * Note: This may change the type of the node if an explicit
			 * store has been performed such that the node/object type
			 * has been changed.
			 */
			status = acpi_ns_attach_object(node, new_desc,
						       new_desc->common.type);

			ACPI_DEBUG_PRINT((ACPI_DB_EXEC,
					  "Store type [%s] into [%s] via Convert/Attach\n",
					  acpi_ut_get_object_type_name
					  (source_desc),
					  acpi_ut_get_object_type_name
					  (new_desc)));
		}
		break;

	case ACPI_TYPE_BUFFER_FIELD:
	case ACPI_TYPE_LOCAL_REGION_FIELD:
	case ACPI_TYPE_LOCAL_BANK_FIELD:
	case ACPI_TYPE_LOCAL_INDEX_FIELD:
		/*
		 * For all fields, always write the source data to the target
		 * field. Any required implicit source operand conversion is
		 * performed in the function below as necessary. Note, field
		 * objects must retain their original type permanently.
		 */
		status = acpi_ex_write_data_to_field(source_desc, target_desc,
						     &walk_state->result_obj);
		break;

	default:
		/*
		 * copy_object operator: No conversions for all other types.
		 * Instead, directly store a copy of the source object.
		 *
		 * This is the ACPI spec-defined behavior for the copy_object
		 * operator. (Note, for this default case, all normal
		 * Store/Target operations exited above with an error).
		 */
		status = acpi_ex_store_direct_to_node(source_desc, node,
						      walk_state);
		break;
	}

	return_ACPI_STATUS(status);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_store_direct_to_node
 *
 * PARAMETERS:  source_desc             - Value to be stored
 *              node                    - Named object to receive the value
 *              walk_state              - Current walk state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: "Store" an object directly to a node. This involves a copy
 *              and an attach.
 *
 ******************************************************************************/

static acpi_status
acpi_ex_store_direct_to_node(union acpi_operand_object *source_desc,
			     struct acpi_namespace_node *node,
			     struct acpi_walk_state *walk_state)
{
	acpi_status status;
	union acpi_operand_object *new_desc;

	ACPI_FUNCTION_TRACE(ex_store_direct_to_node);

	ACPI_DEBUG_PRINT((ACPI_DB_EXEC,
			  "Storing [%s] (%p) directly into node [%s] (%p)"
			  " with no implicit conversion\n",
			  acpi_ut_get_object_type_name(source_desc),
			  source_desc, acpi_ut_get_type_name(node->type),
			  node));

	/* Copy the source object to a new object */

	status =
	    acpi_ut_copy_iobject_to_iobject(source_desc, &new_desc, walk_state);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/* Attach the new object to the node */

	status = acpi_ns_attach_object(node, new_desc, new_desc->common.type);
	acpi_ut_remove_reference(new_desc);
	return_ACPI_STATUS(status);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        /******************************************************************************
 *
 * Module Name: exstoren - AML Interpreter object store support,
 *                        Store to Node (namespace object)
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
#include "acinterp.h"
#include "amlcode.h"

#define _COMPONENT          ACPI_EXECUTER
ACPI_MODULE_NAME("exstoren")

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_resolve_object
 *
 * PARAMETERS:  source_desc_ptr     - Pointer to the source object
 *              target_type         - Current type of the target
 *              walk_state          - Current walk state
 *
 * RETURN:      Status, resolved object in source_desc_ptr.
 *
 * DESCRIPTION: Resolve an object. If the object is a reference, dereference
 *              it and return the actual object in the source_desc_ptr.
 *
 ******************************************************************************/
acpi_status
acpi_ex_resolve_object(union acpi_operand_object **source_desc_ptr,
		       acpi_object_type target_type,
		       struct acpi_walk_state *walk_state)
{
	union acpi_operand_object *source_desc = *source_desc_ptr;
	acpi_status status = AE_OK;

	ACPI_FUNCTION_TRACE(ex_resolve_object);

	/* Ensure we have a Target that can be stored to */

	switch (target_type) {
	case ACPI_TYPE_BUFFER_FIELD:
	case ACPI_TYPE_LOCAL_REGION_FIELD:
	case ACPI_TYPE_LOCAL_BANK_FIELD:
	case ACPI_TYPE_LOCAL_INDEX_FIELD:
		/*
		 * These cases all require only Integers or values that
		 * can be converted to Integers (Strings or Buffers)
		 */
	case ACPI_TYPE_INTEGER:
	case ACPI_TYPE_STRING:
	case ACPI_TYPE_BUFFER:
		/*
		 * Stores into a Field/Region or into a Integer/Buffer/String
		 * are all essentially the same. This case handles the
		 * "interchangeable" types Integer, String, and Buffer.
		 */
		if (source_desc->common.type == ACPI_TYPE_LOCAL_REFERENCE) {

			/* Resolve a reference object first */

			status =
			    acpi_ex_resolve_to_value(source_desc_ptr,
						     walk_state);
			if (ACPI_FAILURE(status)) {
				break;
			}
		}

		/* For copy_object, no further validation necessary */

		if (walk_state->opcode == AML_COPY_OP) {
			break;
		}

		/* Must have a Integer, Buffer, or String */

		if ((source_desc->common.type != ACPI_TYPE_INTEGER) &&
		    (source_desc->common.type != ACPI_TYPE_BUFFER) &&
		    (source_desc->common.type != ACPI_TYPE_STRING) &&
		    !((source_desc->common.type == ACPI_TYPE_LOCAL_REFERENCE) &&
		      (source_desc->reference.class == ACPI_REFCLASS_TABLE))) {

			/* Conversion successful but still not a valid type */

			ACPI_ERROR((AE_INFO,
				    "Cannot assign type [%s] to [%s] (must be type Int/Str/Buf)",
				    acpi_ut_get_object_type_name(source_desc),
				    acpi_ut_get_type_name(target_type)));

			status = AE_AML_OPERAND_TYPE;
		}
		break;

	case ACPI_TYPE_LOCAL_ALIAS:
	case ACPI_TYPE_LOCAL_METHOD_ALIAS:
		/*
		 * All aliases should have been resolved earlier, during the
		 * operand resolution phase.
		 */
		ACPI_ERROR((AE_INFO, "Store into an unresolved Alias object"));
		status = AE_AML_INTERNAL;
		break;

	case ACPI_TYPE_PACKAGE:
	default:
		/*
		 * All other types than Alias and the various Fields come here,
		 * including the untyped case - ACPI_TYPE_ANY.
		 */
		break;
	}

	return_ACPI_STATUS(status);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_store_object_to_object
 *
 * PARAMETERS:  source_desc         - Object to store
 *              dest_desc           - Object to receive a copy of the source
 *              new_desc            - New object if dest_desc is obsoleted
 *              walk_state          - Current walk state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: "Store" an object to another object. This may include
 *              converting the source type to the target type (implicit
 *              conversion), and a copy of the value of the source to
 *              the target.
 *
 *              The Assignment of an object to another (not named) object
 *              is handled here.
 *              The Source passed in will replace the current value (if any)
 *              with the input value.
 *
 *              When storing into an object the data is converted to the
 *              target object type then stored in the object. This means
 *              that the target object type (for an initialized target) will
 *              not be changed by a store operation.
 *
 *              This module allows destination types of Number, String,
 *              Buffer, and Package.
 *
 *              Assumes parameters are already validated. NOTE: source_desc
 *              resolution (from a reference object) must be performed by
 *              the caller if necessary.
 *
 ******************************************************************************/

acpi_status
acpi_ex_store_object_to_object(union acpi_operand_object *source_desc,
			       union acpi_operand_object *dest_desc,
			       union acpi_operand_object **new_desc,
			       struct acpi_walk_state *walk_state)
{
	union acpi_operand_object *actual_src_desc;
	acpi_status status = AE_OK;

	ACPI_FUNCTION_TRACE_PTR(ex_store_object_to_object, source_desc);

	actual_src_desc = source_desc;
	if (!dest_desc) {
		/*
		 * There is no destination object (An uninitialized node or
		 * package element), so we can simply copy the source object
		 * creating a new destination object
		 */
		status =
		    acpi_ut_copy_iobject_to_iobject(actual_src_desc, new_desc,
						    walk_state);
		return_ACPI_STATUS(status);
	}

	if (source_desc->common.type != dest_desc->common.type) {
		/*
		 * The source type does not match the type of the destination.
		 * Perform the "implicit conversion" of the source to the current type
		 * of the target as per the ACPI specification.
		 *
		 * If no conversion performed, actual_src_desc = source_desc.
		 * Otherwise, actual_src_desc is a temporary object to hold the
		 * converted object.
		 */
		status = acpi_ex_convert_to_target_type(dest_desc->common.type,
							source_desc,
							&actual_src_desc,
							walk_state);
		if (ACPI_FAILURE(status)) {
			return_ACPI_STATUS(status);
		}

		if (source_desc == actual_src_desc) {
			/*
			 * No conversion was performed. Return the source_desc as the
			 * new object.
			 */
			*new_desc = source_desc;
			return_ACPI_STATUS(AE_OK);
		}
	}

	/*
	 * We now have two objects of identical types, and we can perform a
	 * copy of the *value* of the source object.
	 */
	switch (dest_desc->common.type) {
	case ACPI_TYPE_INTEGER:

		dest_desc->integer.value = actual_src_desc->integer.value;

		/* Truncate value if we are executing from a 32-bit ACPI table */

		(void)acpi_ex_truncate_for32bit_table(dest_desc);
		break;

	case ACPI_TYPE_STRING:

		status =
		    acpi_ex_store_string_to_string(actual_src_desc, dest_desc);
		break;

	case ACPI_TYPE_BUFFER:

		status =
		    acpi_ex_store_buffer_to_buffer(actual_src_desc, dest_desc);
		break;

	case ACPI_TYPE_PACKAGE:

		status =
		    acpi_ut_copy_iobject_to_iobject(actual_src_desc, &dest_desc,
						    walk_state);
		break;

	default:
		/*
		 * All other types come here.
		 */
		ACPI_WARNING((AE_INFO, "Store into type [%s] not implemented",
			      acpi_ut_get_object_type_name(dest_desc)));

		status = AE_NOT_IMPLEMENTED;
		break;
	}

	if (actual_src_desc != source_desc) {

		/* Delete the intermediate (temporary) source object */

		acpi_ut_remove_reference(actual_src_desc);
	}

	*new_desc = dest_desc;
	return_ACPI_STATUS(status);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      /******************************************************************************
 *
 * Module Name: exstorob - AML Interpreter object store support, store to object
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
#include "acinterp.h"

#define _COMPONENT          ACPI_EXECUTER
ACPI_MODULE_NAME("exstorob")

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_store_buffer_to_buffer
 *
 * PARAMETERS:  source_desc         - Source object to copy
 *              target_desc         - Destination object of the copy
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Copy a buffer object to another buffer object.
 *
 ******************************************************************************/
acpi_status
acpi_ex_store_buffer_to_buffer(union acpi_operand_object *source_desc,
			       union acpi_operand_object *target_desc)
{
	u32 length;
	u8 *buffer;

	ACPI_FUNCTION_TRACE_PTR(ex_store_buffer_to_buffer, source_desc);

	/* If Source and Target are the same, just return */

	if (source_desc == target_desc) {
		return_ACPI_STATUS(AE_OK);
	}

	/* We know that source_desc is a buffer by now */

	buffer = ACPI_CAST_PTR(u8, source_desc->buffer.pointer);
	length = source_desc->buffer.length;

	/*
	 * If target is a buffer of length zero or is a static buffer,
	 * allocate a new buffer of the proper length
	 */
	if ((target_desc->buffer.length == 0) ||
	    (target_desc->common.flags & AOPOBJ_STATIC_POINTER)) {
		target_desc->buffer.pointer = ACPI_ALLOCATE(length);
		if (!target_desc->buffer.pointer) {
			return_ACPI_STATUS(AE_NO_MEMORY);
		}

		target_desc->buffer.length = length;
	}

	/* Copy source buffer to target buffer */

	if (length <= target_desc->buffer.length) {

		/* Clear existing buffer and copy in the new one */

		memset(target_desc->buffer.pointer, 0,
		       target_desc->buffer.length);
		memcpy(target_desc->buffer.pointer, buffer, length);

#ifdef ACPI_OBSOLETE_BEHAVIOR
		/*
		 * NOTE: ACPI versions up to 3.0 specified that the buffer must be
		 * truncated if the string is smaller than the buffer. However, "other"
		 * implementations of ACPI never did this and thus became the defacto
		 * standard. ACPI 3.0A changes this behavior such that the buffer
		 * is no longer truncated.
		 */

		/*
		 * OBSOLETE BEHAVIOR:
		 * If the original source was a string, we must truncate the buffer,
		 * according to the ACPI spec. Integer-to-Buffer and Buffer-to-Buffer
		 * copy must not truncate the original buffer.
		 */
		if (original_src_type == ACPI_TYPE_STRING) {

			/* Set the new length of the target */

			target_desc->buffer.length = length;
		}
#endif
	} else {
		/* Truncate the source, copy only what will fit */

		memcpy(target_desc->buffer.pointer, buffer,
		       target_desc->buffer.length);

		ACPI_DEBUG_PRINT((ACPI_DB_INFO,
				  "Truncating source buffer from %X to %X\n",
				  length, target_desc->buffer.length));
	}

	/* Copy flags */

	target_desc->buffer.flags = source_desc->buffer.flags;
	target_desc->common.flags &= ~AOPOBJ_STATIC_POINTER;
	return_ACPI_STATUS(AE_OK);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_store_string_to_string
 *
 * PARAMETERS:  source_desc         - Source object to copy
 *              target_desc         - Destination object of the copy
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Copy a String object to another String object
 *
 ******************************************************************************/

acpi_status
acpi_ex_store_string_to_string(union acpi_operand_object *source_desc,
			       union acpi_operand_object *target_desc)
{
	u32 length;
	u8 *buffer;

	ACPI_FUNCTION_TRACE_PTR(ex_store_string_to_string, source_desc);

	/* If Source and Target are the same, just return */

	if (source_desc == target_desc) {
		return_ACPI_STATUS(AE_OK);
	}

	/* We know that source_desc is a string by now */

	buffer = ACPI_CAST_PTR(u8, source_desc->string.pointer);
	length = source_desc->string.length;

	/*
	 * Replace existing string value if it will fit and the string
	 * pointer is not a static pointer (part of an ACPI table)
	 */
	if ((length < target_desc->string.length) &&
	    (!(target_desc->common.flags & AOPOBJ_STATIC_POINTER))) {
		/*
		 * String will fit in existing non-static buffer.
		 * Clear old string and copy in the new one
		 */
		memset(target_desc->string.pointer, 0,
		       (acpi_size) target_desc->string.length + 1);
		memcpy(target_desc->string.pointer, buffer, length);
	} else {
		/*
		 * Free the current buffer, then allocate a new buffer
		 * large enough to hold the value
		 */
		if (target_desc->string.pointer &&
		    (!(target_desc->common.flags & AOPOBJ_STATIC_POINTER))) {

			/* Only free if not a pointer into the DSDT */

			ACPI_FREE(target_desc->string.pointer);
		}

		target_desc->string.pointer = ACPI_ALLOCATE_ZEROED((acpi_size)
								   length + 1);
		if (!target_desc->string.pointer) {
			return_ACPI_STATUS(AE_NO_MEMORY);
		}

		target_desc->common.flags &= ~AOPOBJ_STATIC_POINTER;
		memcpy(target_desc->string.pointer, buffer, length);
	}

	/* Set the new target length */

	target_desc->string.length = length;
	return_ACPI_STATUS(AE_OK);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       /******************************************************************************
 *
 * Module Name: exsystem - Interface to OS services
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
#include "acinterp.h"

#define _COMPONENT          ACPI_EXECUTER
ACPI_MODULE_NAME("exsystem")

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_system_wait_semaphore
 *
 * PARAMETERS:  semaphore       - Semaphore to wait on
 *              timeout         - Max time to wait
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Implements a semaphore wait with a check to see if the
 *              semaphore is available immediately. If it is not, the
 *              interpreter is released before waiting.
 *
 ******************************************************************************/
acpi_status acpi_ex_system_wait_semaphore(acpi_semaphore semaphore, u16 timeout)
{
	acpi_status status;

	ACPI_FUNCTION_TRACE(ex_system_wait_semaphore);

	status = acpi_os_wait_semaphore(semaphore, 1, ACPI_DO_NOT_WAIT);
	if (ACPI_SUCCESS(status)) {
		return_ACPI_STATUS(status);
	}

	if (status == AE_TIME) {

		/* We must wait, so unlock the interpreter */

		acpi_ex_exit_interpreter();

		status = acpi_os_wait_semaphore(semaphore, 1, timeout);

		ACPI_DEBUG_PRINT((ACPI_DB_EXEC,
				  "*** Thread awake after blocking, %s\n",
				  acpi_format_exception(status)));

		/* Reacquire the interpreter */

		acpi_ex_enter_interpreter();
	}

	return_ACPI_STATUS(status);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_system_wait_mutex
 *
 * PARAMETERS:  mutex           - Mutex to wait on
 *              timeout         - Max time to wait
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Implements a mutex wait with a check to see if the
 *              mutex is available immediately. If it is not, the
 *              interpreter is released before waiting.
 *
 ******************************************************************************/

acpi_status acpi_ex_system_wait_mutex(acpi_mutex mutex, u16 timeout)
{
	acpi_status status;

	ACPI_FUNCTION_TRACE(ex_system_wait_mutex);

	status = acpi_os_acquire_mutex(mutex, ACPI_DO_NOT_WAIT);
	if (ACPI_SUCCESS(status)) {
		return_ACPI_STATUS(status);
	}

	if (status == AE_TIME) {

		/* We must wait, so unlock the interpreter */

		acpi_ex_exit_interpreter();

		status = acpi_os_acquire_mutex(mutex, timeout);

		ACPI_DEBUG_PRINT((ACPI_DB_EXEC,
				  "*** Thread awake after blocking, %s\n",
				  acpi_format_exception(status)));

		/* Reacquire the interpreter */

		acpi_ex_enter_interpreter();
	}

	return_ACPI_STATUS(status);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_system_do_stall
 *
 * PARAMETERS:  how_long        - The amount of time to stall,
 *                                in microseconds
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Suspend running thread for specified amount of time.
 *              Note: ACPI specification requires that Stall() does not
 *              relinquish the processor, and delays longer than 100 usec
 *              should use Sleep() instead. We allow stalls up to 255 usec
 *              for compatibility with other interpreters and existing BIOSs.
 *
 ******************************************************************************/

acpi_status acpi_ex_system_do_stall(u32 how_long)
{
	acpi_status status = AE_OK;

	ACPI_FUNCTION_ENTRY();

	if (how_long > 255) {	/* 255 microseconds */
		/*
		 * Longer than 255 usec, this is an error
		 *
		 * (ACPI specifies 100 usec as max, but this gives some slack in
		 * order to support existing BIOSs)
		 */
		ACPI_ERROR((AE_INFO, "Time parameter is too large (%u)",
			    how_long));
		status = AE_AML_OPERAND_VALUE;
	} else {
		acpi_os_stall(how_long);
	}

	return (status);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_system_do_sleep
 *
 * PARAMETERS:  how_long        - The amount of time to sleep,
 *                                in milliseconds
 *
 * RETURN:      None
 *
 * DESCRIPTION: Sleep the running thread for specified amount of time.
 *
 ******************************************************************************/

acpi_status acpi_ex_system_do_sleep(u64 how_long)
{
	ACPI_FUNCTION_ENTRY();

	/* Since this thread will sleep, we must release the interpreter */

	acpi_ex_exit_interpreter();

	/*
	 * For compatibility with other ACPI implementations and to prevent
	 * accidental deep sleeps, limit the sleep time to something reasonable.
	 */
	if (how_long > ACPI_MAX_SLEEP) {
		how_long = ACPI_MAX_SLEEP;
	}

	acpi_os_sleep(how_long);

	/* And now we must get the interpreter again */

	acpi_ex_enter_interpreter();
	return (AE_OK);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_system_signal_event
 *
 * PARAMETERS:  obj_desc        - The object descriptor for this op
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Provides an access point to perform synchronization operations
 *              within the AML.
 *
 ******************************************************************************/

acpi_status acpi_ex_system_signal_event(union acpi_operand_object * obj_desc)
{
	acpi_status status = AE_OK;

	ACPI_FUNCTION_TRACE(ex_system_signal_event);

	if (obj_desc) {
		status =
		    acpi_os_signal_semaphore(obj_desc->event.os_semaphore, 1);
	}

	return_ACPI_STATUS(status);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_system_wait_event
 *
 * PARAMETERS:  time_desc       - The 'time to delay' object descriptor
 *              obj_desc        - The object descriptor for this op
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Provides an access point to perform synchronization operations
 *              within the AML. This operation is a request to wait for an
 *              event.
 *
 ******************************************************************************/

acpi_status
acpi_ex_system_wait_event(union acpi_operand_object *time_desc,
			  union acpi_operand_object *obj_desc)
{
	acpi_status status = AE_OK;

	ACPI_FUNCTION_TRACE(ex_system_wait_event);

	if (obj_desc) {
		status =
		    acpi_ex_system_wait_semaphore(obj_desc->event.os_semaphore,
						  (u16) time_desc->integer.
						  value);
	}

	return_ACPI_STATUS(status);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_system_reset_event
 *
 * PARAMETERS:  obj_desc        - The object descriptor for this op
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Reset an event to a known state.
 *
 ******************************************************************************/

acpi_status acpi_ex_system_reset_event(union acpi_operand_object *obj_desc)
{
	acpi_status status = AE_OK;
	acpi_semaphore temp_semaphore;

	ACPI_FUNCTION_ENTRY();

	/*
	 * We are going to simply delete the existing semaphore and
	 * create a new one!
	 */
	status =
	    acpi_os_create_semaphore(ACPI_NO_UNIT_LIMIT, 0, &temp_semaphore);
	if (ACPI_SUCCESS(status)) {
		(void)acpi_os_delete_semaphore(obj_desc->event.os_semaphore);
		obj_desc->event.os_semaphore = temp_semaphore;
	}

	return (status);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         /******************************************************************************
 *
 * Module Name: exutils - interpreter/scanner utilities
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

/*
 * DEFINE_AML_GLOBALS is tested in amlcode.h
 * to determine whether certain global names should be "defined" or only
 * "declared" in the current compilation. This enhances maintainability
 * by enabling a single header file to embody all knowledge of the names
 * in question.
 *
 * Exactly one module of any executable should #define DEFINE_GLOBALS
 * before #including the header files which use this convention. The
 * names in question will be defined and initialized in that module,
 * and declared as extern in all other modules which #include those
 * header files.
 */

#define DEFINE_AML_GLOBALS

#include <acpi/acpi.h>
#include "accommon.h"
#include "acinterp.h"
#include "amlcode.h"

#define _COMPONENT          ACPI_EXECUTER
ACPI_MODULE_NAME("exutils")

/* Local prototypes */
static u32 acpi_ex_digits_needed(u64 value, u32 base);

#ifndef ACPI_NO_METHOD_EXECUTION
/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_enter_interpreter
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 * DESCRIPTION: Enter the interpreter execution region. Failure to enter
 *              the interpreter region is a fatal system error. Used in
 *              conjunction with exit_interpreter.
 *
 ******************************************************************************/

void acpi_ex_enter_interpreter(void)
{
	acpi_status status;

	ACPI_FUNCTION_TRACE(ex_enter_interpreter);

	status = acpi_ut_acquire_mutex(ACPI_MTX_INTERPRETER);
	if (ACPI_FAILURE(status)) {
		ACPI_ERROR((AE_INFO,
			    "Could not acquire AML Interpreter mutex"));
	}

	return_VOID;
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_exit_interpreter
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 * DESCRIPTION: Exit the interpreter execution region. This is the top level
 *              routine used to exit the interpreter when all processing has
 *              been completed, or when the method blocks.
 *
 * Cases where the interpreter is unlocked internally:
 *      1) Method will be blocked on a Sleep() AML opcode
 *      2) Method will be blocked on an Acquire() AML opcode
 *      3) Method will be blocked on a Wait() AML opcode
 *      4) Method will be blocked to acquire the global lock
 *      5) Method will be blocked waiting to execute a serialized control
 *          method that is currently executing
 *      6) About to invoke a user-installed opregion handler
 *
 ******************************************************************************/

void acpi_ex_exit_interpreter(void)
{
	acpi_status status;

	ACPI_FUNCTION_TRACE(ex_exit_interpreter);

	status = acpi_ut_release_mutex(ACPI_MTX_INTERPRETER);
	if (ACPI_FAILURE(status)) {
		ACPI_ERROR((AE_INFO,
			    "Could not release AML Interpreter mutex"));
	}

	return_VOID;
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_truncate_for32bit_table
 *
 * PARAMETERS:  obj_desc        - Object to be truncated
 *
 * RETURN:      TRUE if a truncation was performed, FALSE otherwise.
 *
 * DESCRIPTION: Truncate an ACPI Integer to 32 bits if the execution mode is
 *              32-bit, as determined by the revision of the DSDT.
 *
 ******************************************************************************/

u8 acpi_ex_truncate_for32bit_table(union acpi_operand_object *obj_desc)
{

	ACPI_FUNCTION_ENTRY();

	/*
	 * Object must be a valid number and we must be executing
	 * a control method. Object could be NS node for AML_INT_NAMEPATH_OP.
	 */
	if ((!obj_desc) ||
	    (ACPI_GET_DESCRIPTOR_TYPE(obj_desc) != ACPI_DESC_TYPE_OPERAND) ||
	    (obj_desc->common.type != ACPI_TYPE_INTEGER)) {
		return (FALSE);
	}

	if ((acpi_gbl_integer_byte_width == 4) &&
	    (obj_desc->integer.value > (u64)ACPI_UINT32_MAX)) {
		/*
		 * We are executing in a 32-bit ACPI table.
		 * Truncate the value to 32 bits by zeroing out the upper 32-bit field
		 */
		obj_desc->integer.value &= (u64)ACPI_UINT32_MAX;
		return (TRUE);
	}

	return (FALSE);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_acquire_global_lock
 *
 * PARAMETERS:  field_flags           - Flags with Lock rule:
 *                                      always_lock or never_lock
 *
 * RETURN:      None
 *
 * DESCRIPTION: Obtain the ACPI hardware Global Lock, only if the field
 *              flags specifiy that it is to be obtained before field access.
 *
 ******************************************************************************/

void acpi_ex_acquire_global_lock(u32 field_flags)
{
	acpi_status status;

	ACPI_FUNCTION_TRACE(ex_acquire_global_lock);

	/* Only use the lock if the always_lock bit is set */

	if (!(field_flags & AML_FIELD_LOCK_RULE_MASK)) {
		return_VOID;
	}

	/* Attempt to get the global lock, wait forever */

	status = acpi_ex_acquire_mutex_object(ACPI_WAIT_FOREVER,
					      acpi_gbl_global_lock_mutex,
					      acpi_os_get_thread_id());

	if (ACPI_FAILURE(status)) {
		ACPI_EXCEPTION((AE_INFO, status,
				"Could not acquire Global Lock"));
	}

	return_VOID;
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_release_global_lock
 *
 * PARAMETERS:  field_flags           - Flags with Lock rule:
 *                                      always_lock or never_lock
 *
 * RETURN:      None
 *
 * DESCRIPTION: Release the ACPI hardware Global Lock
 *
 ******************************************************************************/

void acpi_ex_release_global_lock(u32 field_flags)
{
	acpi_status status;

	ACPI_FUNCTION_TRACE(ex_release_global_lock);

	/* Only use the lock if the always_lock bit is set */

	if (!(field_flags & AML_FIELD_LOCK_RULE_MASK)) {
		return_VOID;
	}

	/* Release the global lock */

	status = acpi_ex_release_mutex_object(acpi_gbl_global_lock_mutex);
	if (ACPI_FAILURE(status)) {

		/* Report the error, but there isn't much else we can do */

		ACPI_EXCEPTION((AE_INFO, status,
				"Could not release Global Lock"));
	}

	return_VOID;
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_digits_needed
 *
 * PARAMETERS:  value           - Value to be represented
 *              base            - Base of representation
 *
 * RETURN:      The number of digits.
 *
 * DESCRIPTION: Calculate the number of digits needed to represent the Value
 *              in the given Base (Radix)
 *
 ******************************************************************************/

static u32 acpi_ex_digits_needed(u64 value, u32 base)
{
	u32 num_digits;
	u64 current_value;

	ACPI_FUNCTION_TRACE(ex_digits_needed);

	/* u64 is unsigned, so we don't worry about a '-' prefix */

	if (value == 0) {
		return_UINT32(1);
	}

	current_value = value;
	num_digits = 0;

	/* Count the digits in the requested base */

	while (current_value) {
		(void)acpi_ut_short_divide(current_value, base, &current_value,
					   NULL);
		num_digits++;
	}

	return_UINT32(num_digits);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_eisa_id_to_string
 *
 * PARAMETERS:  compressed_id   - EISAID to be converted
 *              out_string      - Where to put the converted string (8 bytes)
 *
 * RETURN:      None
 *
 * DESCRIPTION: Convert a numeric EISAID to string representation. Return
 *              buffer must be large enough to hold the string. The string
 *              returned is always exactly of length ACPI_EISAID_STRING_SIZE
 *              (includes null terminator). The EISAID is always 32 bits.
 *
 ******************************************************************************/

void acpi_ex_eisa_id_to_string(char *out_string, u64 compressed_id)
{
	u32 swapped_id;

	ACPI_FUNCTION_ENTRY();

	/* The EISAID should be a 32-bit integer */

	if (compressed_id > ACPI_UINT32_MAX) {
		ACPI_WARNING((AE_INFO,
			      "Expected EISAID is larger than 32 bits: 0x%8.8X%8.8X, truncating",
			      ACPI_FORMAT_UINT64(compressed_id)));
	}

	/* Swap ID to big-endian to get contiguous bits */

	swapped_id = acpi_ut_dword_byte_swap((u32)compressed_id);

	/* First 3 bytes are uppercase letters. Next 4 bytes are hexadecimal */

	out_string[0] =
	    (char)(0x40 + (((unsigned long)swapped_id >> 26) & 0x1F));
	out_string[1] = (char)(0x40 + ((swapped_id >> 21) & 0x1F));
	out_string[2] = (char)(0x40 + ((swapped_id >> 16) & 0x1F));
	out_string[3] = acpi_ut_hex_to_ascii_char((u64) swapped_id, 12);
	out_string[4] = acpi_ut_hex_to_ascii_char((u64) swapped_id, 8);
	out_string[5] = acpi_ut_hex_to_ascii_char((u64) swapped_id, 4);
	out_string[6] = acpi_ut_hex_to_ascii_char((u64) swapped_id, 0);
	out_string[7] = 0;
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_integer_to_string
 *
 * PARAMETERS:  out_string      - Where to put the converted string. At least
 *                                21 bytes are needed to hold the largest
 *                                possible 64-bit integer.
 *              value           - Value to be converted
 *
 * RETURN:      None, string
 *
 * DESCRIPTION: Convert a 64-bit integer to decimal string representation.
 *              Assumes string buffer is large enough to hold the string. The
 *              largest string is (ACPI_MAX64_DECIMAL_DIGITS + 1).
 *
 ******************************************************************************/

void acpi_ex_integer_to_string(char *out_string, u64 value)
{
	u32 count;
	u32 digits_needed;
	u32 remainder;

	ACPI_FUNCTION_ENTRY();

	digits_needed = acpi_ex_digits_needed(value, 10);
	out_string[digits_needed] = 0;

	for (count = digits_needed; count > 0; count--) {
		(void)acpi_ut_short_divide(value, 10, &value, &remainder);
		out_string[count - 1] = (char)('0' + remainder);
	}
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_pci_cls_to_string
 *
 * PARAMETERS:  out_string      - Where to put the converted string (7 bytes)
 * PARAMETERS:  class_code      - PCI class code to be converted (3 bytes)
 *
 * RETURN:      None
 *
 * DESCRIPTION: Convert 3-bytes PCI class code to string representation.
 *              Return buffer must be large enough to hold the string. The
 *              string returned is always exactly of length
 *              ACPI_PCICLS_STRING_SIZE (includes null terminator).
 *
 ******************************************************************************/

void acpi_ex_pci_cls_to_string(char *out_string, u8 class_code[3])
{

	ACPI_FUNCTION_ENTRY();

	/* All 3 bytes are hexadecimal */

	out_string[0] = acpi_ut_hex_to_ascii_char((u64)class_code[0], 4);
	out_string[1] = acpi_ut_hex_to_ascii_char((u64)class_code[0], 0);
	out_string[2] = acpi_ut_hex_to_ascii_char((u64)class_code[1], 4);
	out_string[3] = acpi_ut_hex_to_ascii_char((u64)class_code[1], 0);
	out_string[4] = acpi_ut_hex_to_ascii_char((u64)class_code[2], 4);
	out_string[5] = acpi_ut_hex_to_ascii_char((u64)class_code[2], 0);
	out_string[6] = 0;
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_is_valid_space_id
 *
 * PARAMETERS:  space_id            - ID to be validated
 *
 * RETURN:      TRUE if valid/supported ID.
 *
 * DESCRIPTION: Validate an operation region space_ID.
 *
 ******************************************************************************/

u8 acpi_is_valid_space_id(u8 space_id)
{

	if ((space_id >= ACPI_NUM_PREDEFINED_REGIONS) &&
	    (space_id < ACPI_USER_REGION_BEGIN) &&
	    (space_id != ACPI_ADR_SPACE_DATA_TABLE) &&
	    (space_id != ACPI_ADR_SPACE_FIXED_HARDWARE)) {
		return (FALSE);
	}

	return (TRUE);
}

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           /******************************************************************************
 *
 * Module Name: hwacpi - ACPI Hardware Initialization/Mode Interface
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

#define _COMPONENT          ACPI_HARDWARE
ACPI_MODULE_NAME("hwacpi")

#if (!ACPI_REDUCED_HARDWARE)	/* Entire module */
/******************************************************************************
 *
 * FUNCTION:    acpi_hw_set_mode
 *
 * PARAMETERS:  mode            - SYS_MODE_ACPI or SYS_MODE_LEGACY
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Transitions the system into the requested mode.
 *
 ******************************************************************************/
acpi_status acpi_hw_set_mode(u32 mode)
{

	acpi_status status;

	ACPI_FUNCTION_TRACE(hw_set_mode);

	/* If the Hardware Reduced flag is set, machine is always in acpi mode */

	if (acpi_gbl_reduced_hardware) {
		return_ACPI_STATUS(AE_OK);
	}

	/*
	 * ACPI 2.0 clarified that if SMI_CMD in FADT is zero,
	 * system does not support mode transition.
	 */
	if (!acpi_gbl_FADT.smi_command) {
		ACPI_ERROR((AE_INFO,
			    "No SMI_CMD in FADT, mode transition failed"));
		return_ACPI_STATUS(AE_NO_HARDWARE_RESPONSE);
	}

	/*
	 * ACPI 2.0 clarified the meaning of ACPI_ENABLE and ACPI_DISABLE
	 * in FADT: If it is zero, enabling or disabling is not supported.
	 * As old systems may have used zero for mode transition,
	 * we make sure both the numbers are zero to determine these
	 * transitions are not supported.
	 */
	if (!acpi_gbl_FADT.acpi_enable && !acpi_gbl_FADT.acpi_disable) {
		ACPI_ERROR((AE_INFO,
			    "No ACPI mode transition supported in this system "
			    "(enable/disable both zero)"));
		return_ACPI_STATUS(AE_OK);
	}

	switch (mode) {
	case ACPI_SYS_MODE_ACPI:

		/* BIOS should have disabled ALL fixed and GP events */

		status = acpi_hw_write_port(acpi_gbl_FADT.smi_command,
					    (u32) acpi_gbl_FADT.acpi_enable, 8);
		ACPI_DEBUG_PRINT((ACPI_DB_INFO,
				  "Attempting to enable ACPI mode\n"));
		break;

	case ACPI_SYS_MODE_LEGACY:
		/*
		 * BIOS should clear all fixed status bits and restore fixed event
		 * enable bits to default
		 */
		status = acpi_hw_write_port(acpi_gbl_FADT.smi_command,
					    (u32)acpi_gbl_FADT.acpi_disable, 8);
		ACPI_DEBUG_PRINT((ACPI_DB_INFO,
				  "Attempting to enable Legacy (non-ACPI) mode\n"));
		break;

	default:

		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	if (ACPI_FAILURE(status)) {
		ACPI_EXCEPTION((AE_INFO, status,
				"Could not write ACPI mode change"));
		return_ACPI_STATUS(status);
	}

	return_ACPI_STATUS(AE_OK);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_hw_get_mode
 *
 * PARAMETERS:  none
 *
 * RETURN:      SYS_MODE_ACPI or SYS_MODE_LEGACY
 *
 * DESCRIPTION: Return current operating state of system. Determined by
 *              querying the SCI_EN bit.
 *
 ******************************************************************************/

u32 acpi_hw_get_mode(void)
{
	acpi_status status;
	u32 value;

	ACPI_FUNCTION_TRACE(hw_get_mode);

	/* If the Hardware Reduced flag is set, machine is always in acpi mode */

	if (acpi_gbl_reduced_hardware) {
		return_UINT32(ACPI_SYS_MODE_ACPI);
	}

	/*
	 * ACPI 2.0 clarified that if SMI_CMD in FADT is zero,
	 * system does not support mode transition.
	 */
	if (!acpi_gbl_FADT.smi_command) {
		return_UINT32(ACPI_SYS_MODE_ACPI);
	}

	status = acpi_read_bit_register(ACPI_BITREG_SCI_ENABLE, &value);
	if (ACPI_FAILURE(status)) {
		return_UINT32(ACPI_SYS_MODE_LEGACY);
	}

	if (value) {
		return_UINT32(ACPI_SYS_MODE_ACPI);
	} else {
		return_UINT32(ACPI_SYS_MODE_LEGACY);
	}
}

#endif				/* !ACPI_REDUCED_HARDWARE */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          /******************************************************************************
 *
 * Name: hwesleep.c - ACPI Hardware Sleep/Wake Support functions for the
 *                    extended FADT-V5 sleep registers.
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
#include <linux/acpi.h>
#include "accommon.h"

#define _COMPONENT          ACPI_HARDWARE
ACPI_MODULE_NAME("hwesleep")

/*******************************************************************************
 *
 * FUNCTION:    acpi_hw_execute_sleep_method
 *
 * PARAMETERS:  method_pathname     - Pathname of method to execute
 *              integer_argument    - Argument to pass to the method
 *
 * RETURN:      None
 *
 * DESCRIPTION: Execute a sleep/wake related method with one integer argument
 *              and no return value.
 *
 ******************************************************************************/
void acpi_hw_execute_sleep_method(char *method_pathname, u32 integer_argument)
{
	struct acpi_object_list arg_list;
	union acpi_object arg;
	acpi_status status;

	ACPI_FUNCTION_TRACE(hw_execute_sleep_method);

	/* One argument, integer_argument; No return value expected */

	arg_list.count = 1;
	arg_list.pointer = &arg;
	arg.type = ACPI_TYPE_INTEGER;
	arg.integer.value = (u64)integer_argument;

	status = acpi_evaluate_object(NULL, method_pathname, &arg_list, NULL);
	if (ACPI_FAILURE(status) && status != AE_NOT_FOUND) {
		ACPI_EXCEPTION((AE_INFO, status, "While executing method %s",
				method_pathname));
	}

	return_VOID;
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_hw_extended_sleep
 *
 * PARAMETERS:  sleep_state         - Which sleep state to enter
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Enter a system sleep state via the extended FADT sleep
 *              registers (V5 FADT).
 *              THIS FUNCTION MUST BE CALLED WITH INTERRUPTS DISABLED
 *
 ******************************************************************************/

acpi_status acpi_hw_extended_sleep(u8 sleep_state)
{
	acpi_status status;
	u8 sleep_type_value;
	u64 sleep_status;

	ACPI_FUNCTION_TRACE(hw_extended_sleep);

	/* Extended sleep registers must be valid */

	if (!acpi_gbl_FADT.sleep_control.address ||
	    !acpi_gbl_FADT.sleep_status.address) {
		return_ACPI_STATUS(AE_NOT_EXIST);
	}

	/* Clear wake status (WAK_STS) */

	status =
	    acpi_write((u64)ACPI_X_WAKE_STATUS, &acpi_gbl_FADT.sleep_status);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	acpi_gbl_system_awake_and_running = FALSE;

	/* Flush caches, as per ACPI specification */

	ACPI_FLUSH_CPU_CACHE();

	status = acpi_os_prepare_extended_sleep(sleep_state,
						acpi_gbl_sleep_type_a,
						acpi_gbl_sleep_type_b);
	if (ACPI_SKIP(status))
		return_ACPI_STATUS(AE_OK);
	if (ACPI_FAILURE(status))
		return_ACPI_STATUS(status);

	/*
	 * Set the SLP_TYP and SLP_EN bits.
	 *
	 * Note: We only use the first value returned by the \_Sx method
	 * (acpi_gbl_sleep_type_a) - As per ACPI specification.
	 */
	ACPI_DEBUG_PRINT((ACPI_DB_INIT,
			  "Entering sleep state [S%u]\n", sleep_state));

	sleep_type_value =
	    ((acpi_gbl_sleep_type_a << ACPI_X_SLEEP_TYPE_POSITION) &
	     ACPI_X_SLEEP_TYPE_MASK);

	status = acpi_write((u64)(sleep_type_value | ACPI_X_SLEEP_ENABLE),
			    &acpi_gbl_FADT.sleep_control);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/* Wait for transition back to Working State */

	do {
		status = acpi_read(&sleep_status, &acpi_gbl_FADT.sleep_status);
		if (ACPI_FAILURE(status)) {
			return_ACPI_STATUS(status);
		}

	} while (!(((u8)sleep_status) & ACPI_X_WAKE_STATUS));

	return_ACPI_STATUS(AE_OK);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_hw_extended_wake_prep
 *
 * PARAMETERS:  sleep_state         - Which sleep state we just exited
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Perform first part of OS-independent ACPI cleanup after
 *              a sleep. Called with interrupts ENABLED.
 *
 ******************************************************************************/

acpi_status acpi_hw_extended_wake_prep(u8 sleep_state)
{
	u8 sleep_type_value;

	ACPI_FUNCTION_TRACE(hw_extended_wake_prep);

	if (acpi_gbl_sleep_type_a_s0 != ACPI_SLEEP_TYPE_INVALID) {
		sleep_type_value =
		    ((acpi_gbl_sleep_type_a_s0 << ACPI_X_SLEEP_TYPE_POSITION) &
		     ACPI_X_SLEEP_TYPE_MASK);

		(void)acpi_write((u64)(sleep_type_value | ACPI_X_SLEEP_ENABLE),
				 &acpi_gbl_FADT.sleep_control);
	}

	return_ACPI_STATUS(AE_OK);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_hw_extended_wake
 *
 * PARAMETERS:  sleep_state         - Which sleep state we just exited
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Perform OS-independent ACPI cleanup after a sleep
 *              Called with interrupts ENABLED.
 *
 ******************************************************************************/

acpi_status acpi_hw_extended_wake(u8 sleep_state)
{
	ACPI_FUNCTION_TRACE(hw_extended_wake);

	/* Ensure enter_sleep_state_prep -> enter_sleep_state ordering */

	acpi_gbl_sleep_type_a = ACPI_SLEEP_TYPE_INVALID;

	/* Execute the wake methods */

	acpi_hw_execute_sleep_method(METHOD_PATHNAME__SST, ACPI_SST_WAKING);
	acpi_hw_execute_sleep_method(METHOD_PATHNAME__WAK, sleep_state);

	/*
	 * Some BIOS code assumes that WAK_STS will be cleared on resume
	 * and use it to determine whether the system is rebooting or
	 * resuming. Clear WAK_STS for compatibility.
	 */
	(void)acpi_write((u64)ACPI_X_WAKE_STATUS, &acpi_gbl_FADT.sleep_status);
	acpi_gbl_system_awake_and_running = TRUE;

	acpi_hw_execute_sleep_method(METHOD_PATHNAME__SST, ACPI_SST_WORKING);
	return_ACPI_STATUS(AE_OK);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             /******************************************************************************
 *
 * Module Name: hwgpe - Low level GPE enable/disable/clear functions
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
#include "acevents.h"

#define _COMPONENT          ACPI_HARDWARE
ACPI_MODULE_NAME("hwgpe")
#if (