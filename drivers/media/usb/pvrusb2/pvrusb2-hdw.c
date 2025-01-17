turn_desc = acpi_ns_get_attached_object((struct
								   acpi_namespace_node
								   *)
								  operand[0]);
			acpi_ut_add_reference(return_desc);
		} else {
			/*
			 * This must be a reference object produced by either the
			 * Index() or ref_of() operator
			 */
			switch (operand[0]->reference.class) {
			case ACPI_REFCLASS_INDEX:
				/*
				 * The target type for the Index operator must be
				 * either a Buffer or a Package
				 */
				switch (operand[0]->reference.target_type) {
				case ACPI_TYPE_BUFFER_FIELD:

					temp_desc =
					    operand[0]->reference.object;

					/*
					 * Create a new object that contains one element of the
					 * buffer -- the element pointed to by the index.
					 *
					 * NOTE: index into a buffer is NOT a pointer to a
					 * sub-buffer of the main buffer, it is only a pointer to a
					 * single element (byte) of the buffer!
					 *
					 * Since we are returning the value of the buffer at the
					 * indexed location, we don't need to add an additional
					 * reference to the buffer itself.
					 */
					return_desc =
					    acpi_ut_create_integer_object((u64)
									  temp_desc->buffer.pointer[operand[0]->reference.value]);
					if (!return_desc) {
						status = AE_NO_MEMORY;
						goto cleanup;
					}
					break;

				case ACPI_TYPE_PACKAGE:
					/*
					 * Return the referenced element of the package. We must
					 * add another reference to the referenced object, however.
					 */
					return_desc =
					    *(operand[0]->reference.where);
					if (!return_desc) {
						/*
						 * Element is NULL, do not allow the dereference.
						 * This provides compatibility with other ACPI
						 * implementations.
						 */
						return_ACPI_STATUS
						    (AE_AML_UNINITIALIZED_ELEMENT);
					}

					acpi_ut_add_reference(return_desc);
					break;

				default:

					ACPI_ERROR((AE_INFO,
						    "Unknown Index TargetType 0x%X in reference object %p",
						    operand[0]->reference.
						    target_type, operand[0]));
					status = AE_AML_OPERAND_TYPE;
					goto cleanup;
				}
				break;

			case ACPI_REFCLASS_REFOF:

				return_desc = operand[0]->reference.object;

				if (ACPI_GET_DESCRIPTOR_TYPE(return_desc) ==
				    ACPI_DESC_TYPE_NAMED) {
					return_desc =
					    acpi_ns_get_attached_object((struct
									 acpi_namespace_node
									 *)
									return_desc);
					if (!return_desc) {
						break;
					}

					/*
					 * June 2013:
					 * buffer_fields/field_units require additional resolution
					 */
					switch (return_desc->common.type) {
					case ACPI_TYPE_BUFFER_FIELD:
					case ACPI_TYPE_LOCAL_REGION_FIELD:
					case ACPI_TYPE_LOCAL_BANK_FIELD:
					case ACPI_TYPE_LOCAL_INDEX_FIELD:

						status =
						    acpi_ex_read_data_from_field
						    (walk_state, return_desc,
						     &temp_desc);
						if (ACPI_FAILURE(status)) {
							return_ACPI_STATUS
							    (status);
						}

						return_desc = temp_desc;
						break;

					default:

						/* Add another reference to the object */

						acpi_ut_add_reference
						    (return_desc);
						break;
					}
				}
				break;

			default:

				ACPI_ERROR((AE_INFO,
					    "Unknown class in reference(%p) - 0x%2.2X",
					    operand[0],
					    operand[0]->reference.class));

				status = AE_TYPE;
				goto cleanup;
			}
		}
		break;

	default:

		ACPI_ERROR((AE_INFO, "Unknown AML opcode 0x%X",
			    walk_state->opcode));
		status = AE_AML_BAD_OPCODE;
		goto cleanup;
	}

cleanup:

	/* Delete return object on error */

	if (ACPI_FAILURE(status)) {
		acpi_ut_remove_reference(return_desc);
	}

	/* Save return object on success */

	else {
		walk_state->result_obj = return_desc;
	}

	return_ACPI_STATUS(status);
}
                                                                                                                                                                                                                                                                                                                                                                                                 /******************************************************************************
 *
 * Module Name: exoparg2 - AML execution - opcodes with 2 arguments
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
#include "acinterp.h"
#include "acevents.h"
#include "amlcode.h"

#define _COMPONENT          ACPI_EXECUTER
ACPI_MODULE_NAME("exoparg2")

/*!
 * Naming convention for AML interpreter execution routines.
 *
 * The routines that begin execution of AML opcodes are named with a common
 * convention based upon the number of arguments, the number of target operands,
 * and whether or not a value is returned:
 *
 *      AcpiExOpcode_xA_yT_zR
 *
 * Where:
 *
 * xA - ARGUMENTS:    The number of arguments (input operands) that are
 *                    required for this opcode type (1 through 6 args).
 * yT - TARGETS:      The number of targets (output operands) that are required
 *                    for this opcode type (0, 1, or 2 targets).
 * zR - RETURN VALUE: Indicates whether this opcode type returns a value
 *                    as the function return (0 or 1).
 *
 * The AcpiExOpcode* functions are called via the Dispatcher component with
 * fully resolved operands.
!*/
/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_opcode_2A_0T_0R
 *
 * PARAMETERS:  walk_state          - Current walk state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute opcode with two arguments, no target, and no return
 *              value.
 *
 * ALLOCATION:  Deletes both operands
 *
 ******************************************************************************/
acpi_status acpi_ex_opcode_2A_0T_0R(struct acpi_walk_state *walk_state)
{
	union acpi_operand_object **operand = &walk_state->operands[0];
	struct acpi_namespace_node *node;
	u32 value;
	acpi_status status = AE_OK;

	ACPI_FUNCTION_TRACE_STR(ex_opcode_2A_0T_0R,
				acpi_ps_get_opcode_name(walk_state->opcode));

	/* Examine the opcode */

	switch (walk_state->opcode) {
	case AML_NOTIFY_OP:	/* Notify (notify_object, notify_value) */

		/* The first operand is a namespace node */

		node = (struct acpi_namespace_node *)operand[0];

		/* Second value is the notify value */

		value = (u32) operand[1]->integer.value;

		/* Are notifies allowed on this object? */

		if (!acpi_ev_is_notify_object(node)) {
			ACPI_ERROR((AE_INFO,
				    "Unexpected notify object type [%s]",
				    acpi_ut_get_type_name(node->type)));

			status = AE_AML_OPERAND_TYPE;
			break;
		}

		/*
		 * Dispatch the notify to the appropriate handler
		 * NOTE: the request is queued for execution after this method
		 * completes. The notify handlers are NOT invoked synchronously
		 * from this thread -- because handlers may in turn run other
		 * control methods.
		 */
		status = acpi_ev_queue_notify_request(node, value);
		break;

	default:

		ACPI_ERROR((AE_INFO, "Unknown AML opcode 0x%X",
			    walk_state->opcode));
		status = AE_AML_BAD_OPCODE;
	}

	return_ACPI_STATUS(status);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_opcode_2A_2T_1R
 *
 * PARAMETERS:  walk_state          - Current walk state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute a dyadic operator (2 operands) with 2 output targets
 *              and one implicit return value.
 *
 ******************************************************************************/

acpi_status acpi_ex_opcode_2A_2T_1R(struct acpi_walk_state *walk_state)
{
	union acpi_operand_object **operand = &walk_state->operands[0];
	union acpi_operand_object *return_desc1 = NULL;
	union acpi_operand_object *return_desc2 = NULL;
	acpi_status status;

	ACPI_FUNCTION_TRACE_STR(ex_opcode_2A_2T_1R,
				acpi_ps_get_opcode_name(walk_state->opcode));

	/* Execute the opcode */

	switch (walk_state->opcode) {
	case AML_DIVIDE_OP:

		/* Divide (Dividend, Divisor, remainder_result quotient_result) */

		return_desc1 =
		    acpi_ut_create_internal_object(ACPI_TYPE_INTEGER);
		if (!return_desc1) {
			status = AE_NO_MEMORY;
			goto cleanup;
		}

		return_desc2 =
		    acpi_ut_create_internal_object(ACPI_TYPE_INTEGER);
		if (!return_desc2) {
			status = AE_NO_MEMORY;
			goto cleanup;
		}

		/* Quotient to return_desc1, remainder to return_desc2 */

		status = acpi_ut_divide(operand[0]->integer.value,
					operand[1]->integer.value,
					&return_desc1->integer.value,
					&return_desc2->integer.value);
		if (ACPI_FAILURE(status)) {
			goto cleanup;
		}
		break;

	default:

		ACPI_ERROR((AE_INFO, "Unknown AML opcode 0x%X",
			    walk_state->opcode));
		status = AE_AML_BAD_OPCODE;
		goto cleanup;
	}

	/* Store the results to the target reference operands */

	status = acpi_ex_store(return_desc2, operand[2], walk_state);
	if (ACPI_FAILURE(status)) {
		goto cleanup;
	}

	status = acpi_ex_store(return_desc1, operand[3], walk_state);
	if (ACPI_FAILURE(status)) {
		goto cleanup;
	}

cleanup:
	/*
	 * Since the remainder is not returned indirectly, remove a reference to
	 * it. Only the quotient is returned indirectly.
	 */
	acpi_ut_remove_reference(return_desc2);

	if (ACPI_FAILURE(status)) {

		/* Delete the return object */

		acpi_ut_remove_reference(return_desc1);
	}

	/* Save return object (the remainder) on success */

	else {
		walk_state->result_obj = return_desc1;
	}

	return_ACPI_STATUS(status);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_opcode_2A_1T_1R
 *
 * PARAMETERS:  walk_state          - Current walk state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute opcode with two arguments, one target, and a return
 *              value.
 *
 ******************************************************************************/

acpi_status acpi_ex_opcode_2A_1T_1R(struct acpi_walk_state *walk_state)
{
	union acpi_operand_object **operand = &walk_state->operands[0];
	union acpi_operand_object *return_desc = NULL;
	u64 index;
	acpi_status status = AE_OK;
	acpi_size length = 0;

	ACPI_FUNCTION_TRACE_STR(ex_opcode_2A_1T_1R,
				acpi_ps_get_opcode_name(walk_state->opcode));

	/* Execute the opcode */

	if (walk_state->op_info->flags & AML_MATH) {

		/* All simple math opcodes (add, etc.) */

		return_desc = acpi_ut_create_internal_object(ACPI_TYPE_INTEGER);
		if (!return_desc) {
			status = AE_NO_MEMORY;
			goto cleanup;
		}

		return_desc->integer.value =
		    acpi_ex_do_math_op(walk_state->opcode,
				       operand[0]->integer.value,
				       operand[1]->integer.value);
		goto store_result_to_target;
	}

	switch (walk_state->opcode) {
	case AML_MOD_OP:	/* Mod (Dividend, Divisor, remainder_result (ACPI 2.0) */

		return_desc = acpi_ut_create_internal_object(ACPI_TYPE_INTEGER);
		if (!return_desc) {
			status = AE_NO_MEMORY;
			goto cleanup;
		}

		/* return_desc will contain the remainder */

		status = acpi_ut_divide(operand[0]->integer.value,
					operand[1]->integer.value,
					NULL, &return_desc->integer.value);
		break;

	case AML_CONCAT_OP:	/* Concatenate (Data1, Data2, Result) */

		status = acpi_ex_do_concatenate(operand[0], operand[1],
						&return_desc, walk_state);
		break;

	case AML_TO_STRING_OP:	/* to_string (Buffer, Length, Result) (ACPI 2.0) */
		/*
		 * Input object is guaranteed to be a buffer at this point (it may have
		 * been converted.)  Copy the raw buffer data to a new object of
		 * type String.
		 */

		/*
		 * Get the length of the new string. It is the smallest of:
		 * 1) Length of the input buffer
		 * 2) Max length as specified in the to_string operator
		 * 3) Length of input buffer up to a zero byte (null terminator)
		 *
		 * NOTE: A length of zero is ok, and will create a zero-length, null
		 *       terminated string.
		 */
		while ((length < operand[0]->buffer.length) &&
		       (length < operand[1]->integer.value) &&
		       (operand[0]->buffer.pointer[length])) {
			length++;
		}

		/* Allocate a new string object */

		return_desc = acpi_ut_create_string_object(length);
		if (!return_desc) {
			status = AE_NO_MEMORY;
			goto cleanup;
		}

		/*
		 * Copy the raw buffer data with no transform.
		 * (NULL terminated already)
		 */
		memcpy(return_desc->string.pointer,
		       operand[0]->buffer.pointer, length);
		break;

	case AML_CONCAT_RES_OP:

		/* concatenate_res_template (Buffer, Buffer, Result) (ACPI 2.0) */

		status = acpi_ex_concat_template(operand[0], operand[1],
						 &return_desc, walk_state);
		break;

	case AML_INDEX_OP:	/* Index (Source Index Result) */

		/* Create the internal return object */

		return_desc =
		    acpi_ut_create_internal_object(ACPI_TYPE_LOCAL_REFERENCE);
		if (!return_desc) {
			status = AE_NO_MEMORY;
			goto cleanup;
		}

		/* Initialize the Index reference object */

		index = operand[1]->integer.value;
		return_desc->reference.value = (u32) index;
		return_desc->reference.class = ACPI_REFCLASS_INDEX;

		/*
		 * At this point, the Source operand is a String, Buffer, or Package.
		 * Verify that the index is within range.
		 */
		switch ((operand[0])->common.type) {
		case ACPI_TYPE_STRING:

			if (index >= operand[0]->string.length) {
				length = operand[0]->string.length;
				status = AE_AML_STRING_LIMIT;
			}

			return_desc->reference.target_type =
			    ACPI_TYPE_BUFFER_FIELD;
			return_desc->reference.index_pointer =
			    &(operand[0]->buffer.pointer[index]);
			break;

		case ACPI_TYPE_BUFFER:

			if (index >= operand[0]->buffer.length) {
				length = operand[0]->buffer.length;
				status = AE_AML_BUFFER_LIMIT;
			}

			return_desc->reference.target_type =
			    ACPI_TYPE_BUFFER_FIELD;
			return_desc->reference.index_pointer =
			    &(operand[0]->buffer.pointer[index]);
			break;

		case ACPI_TYPE_PACKAGE:

			if (index >= operand[0]->package.count) {
				length = operand[0]->package.count;
				status = AE_AML_PACKAGE_LIMIT;
			}

			return_desc->reference.target_type = ACPI_TYPE_PACKAGE;
			return_desc->reference.where =
			    &operand[0]->package.elements[index];
			break;

		default:

			status = AE_AML_INTERNAL;
			goto cleanup;
		}

		/* Failure means that the Index was beyond the end of the object */

		if (ACPI_FAILURE(status)) {
			ACPI_EXCEPTION((AE_INFO, status,
					"Index (0x%X%8.8X) is beyond end of object (length 0x%X)",
					ACPI_FORMAT_UINT64(index),
					(u32)length));
			goto cleanup;
		}

		/*
		 * Save the target object and add a reference to it for the life
		 * of the index
		 */
		return_desc->reference.object = operand[0];
		acpi_ut_add_reference(operand[0]);

		/* Store the reference to the Target */

		status = acpi_ex_store(return_desc, operand[2], walk_state);

		/* Return the reference */

		walk_state->result_obj = return_desc;
		goto cleanup;

	default:

		ACPI_ERROR((AE_INFO, "Unknown AML opcode 0x%X",
			    walk_state->opcode));
		status = AE_AML_BAD_OPCODE;
		break;
	}

store_result_to_target:

	if (ACPI_SUCCESS(status)) {
		/*
		 * Store the result of the operation (which is now in return_desc) into
		 * the Target descriptor.
		 */
		status = acpi_ex_store(return_desc, operand[2], walk_state);
		if (ACPI_FAILURE(status)) {
			goto cleanup;
		}

		if (!walk_state->result_obj) {
			walk_state->result_obj = return_desc;
		}
	}

cleanup:

	/* Delete return object on error */

	if (ACPI_FAILURE(status)) {
		acpi_ut_remove_reference(return_desc);
		walk_state->result_obj = NULL;
	}

	return_ACPI_STATUS(status);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_opcode_2A_0T_1R
 *
 * PARAMETERS:  walk_state          - Current walk state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute opcode with 2 arguments, no target, and a return value
 *
 ******************************************************************************/

acpi_status acpi_ex_opcode_2A_0T_1R(struct acpi_walk_state *walk_state)
{
	union acpi_operand_object **operand = &walk_state->operands[0];
	union acpi_operand_object *return_desc = NULL;
	acpi_status status = AE_OK;
	u8 logical_result = FALSE;

	ACPI_FUNCTION_TRACE_STR(ex_opcode_2A_0T_1R,
				acpi_ps_get_opcode_name(walk_state->opcode));

	/* Create the internal return object */

	return_desc = acpi_ut_create_internal_object(ACPI_TYPE_INTEGER);
	if (!return_desc) {
		status = AE_NO_MEMORY;
		goto cleanup;
	}

	/* Execute the Opcode */

	if (walk_state->op_info->flags & AML_LOGICAL_NUMERIC) {

		/* logical_op (Operand0, Operand1) */

		status = acpi_ex_do_logical_numeric_op(walk_state->opcode,
						       operand[0]->integer.
						       value,
						       operand[1]->integer.
						       value, &logical_result);
		goto store_logical_result;
	} else if (walk_state->op_info->flags & AML_LOGICAL) {

		/* logical_op (Operand0, Operand1) */

		status = acpi_ex_do_logical_op(walk_state->opcode, operand[0],
					       operand[1], &logical_result);
		goto store_logical_result;
	}

	switch (walk_state->opcode) {
	case AML_ACQUIRE_OP:	/* Acquire (mutex_object, Timeout) */

		status =
		    acpi_ex_acquire_mutex(operand[1], operand[0], walk_state);
		if (status == AE_TIME) {
			logical_result = TRUE;	/* TRUE = Acquire timed out */
			status = AE_OK;
		}
		break;

	case AML_WAIT_OP:	/* Wait (event_object, Timeout) */

		status = acpi_ex_system_wait_event(operand[1], operand[0]);
		if (status == AE_TIME) {
			logical_result = TRUE;	/* TRUE, Wait timed out */
			status = AE_OK;
		}
		break;

	default:

		ACPI_ERROR((AE_INFO, "Unknown AML opcode 0x%X",
			    walk_state->opcode));
		status = AE_AML_BAD_OPCODE;
		goto cleanup;
	}

store_logical_result:
	/*
	 * Set return value to according to logical_result. logical TRUE (all ones)
	 * Default is FALSE (zero)
	 */
	if (logical_result) {
		return_desc->integer.value = ACPI_UINT64_MAX;
	}

cleanup:

	/* Delete return object on error */

	if (ACPI_FAILURE(status)) {
		acpi_ut_remove_reference(return_desc);
	}

	/* Save return object on success */

	else {
		walk_state->result_obj = return_desc;
	}

	return_ACPI_STATUS(status);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                              /******************************************************************************
 *
 * Module Name: exoparg3 - AML execution - opcodes with 3 arguments
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
#include "acparser.h"
#include "amlcode.h"

#define _COMPONENT          ACPI_EXECUTER
ACPI_MODULE_NAME("exoparg3")

/*!
 * Naming convention for AML interpreter execution routines.
 *
 * The routines that begin execution of AML opcodes are named with a common
 * convention based upon the number of arguments, the number of target operands,
 * and whether or not a value is returned:
 *
 *      AcpiExOpcode_xA_yT_zR
 *
 * Where:
 *
 * xA - ARGUMENTS:    The number of arguments (input operands) that are
 *                    required for this opcode type (1 through 6 args).
 * yT - TARGETS:      The number of targets (output operands) that are required
 *                    for this opcode type (0, 1, or 2 targets).
 * zR - RETURN VALUE: Indicates whether this opcode type returns a value
 *                    as the function return (0 or 1).
 *
 * The AcpiExOpcode* functions are called via the Dispatcher component with
 * fully resolved operands.
!*/
/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_opcode_3A_0T_0R
 *
 * PARAMETERS:  walk_state          - Current walk state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute Triadic operator (3 operands)
 *
 ******************************************************************************/
acpi_status acpi_ex_opcode_3A_0T_0R(struct acpi_walk_state *walk_state)
{
	union acpi_operand_object **operand = &walk_state->operands[0];
	struct acpi_signal_fatal_info *fatal;
	acpi_status status = AE_OK;

	ACPI_FUNCTION_TRACE_STR(ex_opcode_3A_0T_0R,
				acpi_ps_get_opcode_name(walk_state->opcode));

	switch (walk_state->opcode) {
	case AML_FATAL_OP:	/* Fatal (fatal_type fatal_code fatal_arg) */

		ACPI_DEBUG_PRINT((ACPI_DB_INFO,
				  "FatalOp: Type %X Code %X Arg %X <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n",
				  (u32) operand[0]->integer.value,
				  (u32) operand[1]->integer.value,
				  (u32) operand[2]->integer.value));

		fatal = ACPI_ALLOCATE(sizeof(struct acpi_signal_fatal_info));
		if (fatal) {
			fatal->type = (u32) operand[0]->integer.value;
			fatal->code = (u32) operand[1]->integer.value;
			fatal->argument = (u32) operand[2]->integer.value;
		}

		/* Always signal the OS! */

		status = acpi_os_signal(ACPI_SIGNAL_FATAL, fatal);

		/* Might return while OS is shutting down, just continue */

		ACPI_FREE(fatal);
		goto cleanup;

	case AML_EXTERNAL_OP:
		/*
		 * If the interpreter sees this opcode, just ignore it. The External
		 * op is intended for use by disassemblers in order to properly
		 * disassemble control method invocations. The opcode or group of
		 * opcodes should be surrounded by an "if (0)" clause to ensure that
		 * AML interpreters never see the opcode.
		 */
		status = AE_OK;
		goto cleanup;

	default:

		ACPI_ERROR((AE_INFO, "Unknown AML opcode 0x%X",
			    walk_state->opcode));
		status = AE_AML_BAD_OPCODE;
		goto cleanup;
	}

cleanup:

	return_ACPI_STATUS(status);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_opcode_3A_1T_1R
 *
 * PARAMETERS:  walk_state          - Current walk state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute Triadic operator (3 operands)
 *
 ******************************************************************************/

acpi_status acpi_ex_opcode_3A_1T_1R(struct acpi_walk_state *walk_state)
{
	union acpi_operand_object **operand = &walk_state->operands[0];
	union acpi_operand_object *return_desc = NULL;
	char *buffer = NULL;
	acpi_status status = AE_OK;
	u64 index;
	acpi_size length;

	ACPI_FUNCTION_TRACE_STR(ex_opcode_3A_1T_1R,
				acpi_ps_get_opcode_name(walk_state->opcode));

	switch (walk_state->opcode) {
	case AML_MID_OP:	/* Mid (Source[0], Index[1], Length[2], Result[3]) */
		/*
		 * Create the return object. The Source operand is guaranteed to be
		 * either a String or a Buffer, so just use its type.
		 */
		return_desc = acpi_ut_create_internal_object((operand[0])->
							     common.type);
		if (!return_desc) {
			status = AE_NO_MEMORY;
			goto cleanup;
		}

		/* Get the Integer values from the objects */

		index = operand[1]->integer.value;
		length = (acpi_size) operand[2]->integer.value;

		/*
		 * If the index is beyond the length of the String/Buffer, or if the
		 * requested length is zero, return a zero-length String/Buffer
		 */
		if (index >= operand[0]->string.length) {
			length = 0;
		}

		/* Truncate request if larger than the actual String/Buffer */

		else if ((index + length) > operand[0]->string.length) {
			length = (acpi_size) operand[0]->string.length -
			    (acpi_size) index;
		}

		/* Strings always have a sub-pointer, not so for buffers */

		switch ((operand[0])->common.type) {
		case ACPI_TYPE_STRING:

			/* Always allocate a new buffer for the String */

			buffer = ACPI_ALLOCATE_ZEROED((acpi_size) length + 1);
			if (!buffer) {
				status = AE_NO_MEMORY;
				goto cleanup;
			}
			break;

		case ACPI_TYPE_BUFFER:

			/* If the requested length is zero, don't allocate a buffer */

			if (length > 0) {

				/* Allocate a new buffer for the Buffer */

				buffer = ACPI_ALLOCATE_ZEROED(length);
				if (!buffer) {
					status = AE_NO_MEMORY;
					goto cleanup;
				}
			}
			break;

		default:	/* Should not happen */

			status = AE_AML_OPERAND_TYPE;
			goto cleanup;
		}

		if (buffer) {

			/* We have a buffer, copy the portion requested */

			memcpy(buffer, operand[0]->string.pointer + index,
			       length);
		}

		/* Set the length of the new String/Buffer */

		return_desc->string.pointer = buffer;
		return_desc->string.length = (u32) length;

		/* Mark buffer initialized */

		return_desc->buffer.flags |= AOPOBJ_DATA_VALID;
		break;

	default:

		ACPI_ERROR((AE_INFO, "Unknown AML opcode 0x%X",
			    walk_state->opcode));
		status = AE_AML_BAD_OPCODE;
		goto cleanup;
	}

	/* Store the result in the target */

	status = acpi_ex_store(return_desc, operand[3], walk_state);

cleanup:

	/* Delete return object on error */

	if (ACPI_FAILURE(status) || walk_state->result_obj) {
		acpi_ut_remove_reference(return_desc);
		walk_state->result_obj = NULL;
	}

	/* Set the return object and exit */

	else {
		walk_state->result_obj = return_desc;
	}
	return_ACPI_STATUS(status);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              /******************************************************************************
 *
 * Module Name: exoparg6 - AML execution - opcodes with 6 arguments
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
#include "acparser.h"
#include "amlcode.h"

#define _COMPONENT          ACPI_EXECUTER
ACPI_MODULE_NAME("exoparg6")

/*!
 * Naming convention for AML interpreter execution routines.
 *
 * The routines that begin execution of AML opcodes are named with a common
 * convention based upon the number of arguments, the number of target operands,
 * and whether or not a value is returned:
 *
 *      AcpiExOpcode_xA_yT_zR
 *
 * Where:
 *
 * xA - ARGUMENTS:    The number of arguments (input operands) that are
 *                    required for this opcode type (1 through 6 args).
 * yT - TARGETS:      The number of targets (output operands) that are required
 *                    for this opcode type (0, 1, or 2 targets).
 * zR - RETURN VALUE: Indicates whether this opcode type returns a value
 *                    as the function return (0 or 1).
 *
 * The AcpiExOpcode* functions are called via the Dispatcher component with
 * fully resolved operands.
!*/
/* Local prototypes */
static u8
acpi_ex_do_match(u32 match_op,
		 union acpi_operand_object *package_obj,
		 union acpi_operand_object *match_obj);

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_do_match
 *
 * PARAMETERS:  match_op        - The AML match operand
 *              package_obj     - Object from the target package
 *              match_obj       - Object to be matched
 *
 * RETURN:      TRUE if the match is successful, FALSE otherwise
 *
 * DESCRIPTION: Implements the low-level match for the ASL Match operator.
 *              Package elements will be implicitly converted to the type of
 *              the match object (Integer/Buffer/String).
 *
 ******************************************************************************/

static u8
acpi_ex_do_match(u32 match_op,
		 union acpi_operand_object *package_obj,
		 union acpi_operand_object *match_obj)
{
	u8 logical_result = TRUE;
	acpi_status status;

	/*
	 * Note: Since the package_obj/match_obj ordering is opposite to that of
	 * the standard logical operators, we have to reverse them when we call
	 * do_logical_op in order to make the implicit conversion rules work
	 * correctly. However, this means we have to flip the entire equation
	 * also. A bit ugly perhaps, but overall, better than fussing the
	 * parameters around at runtime, over and over again.
	 *
	 * Below, P[i] refers to the package element, M refers to the Match object.
	 */
	switch (match_op) {
	case MATCH_MTR:

		/* Always true */

		break;

	case MATCH_MEQ:
		/*
		 * True if equal: (P[i] == M)
		 * Change to:     (M == P[i])
		 */
		status =
		    acpi_ex_do_logical_op(AML_LEQUAL_OP, match_obj, package_obj,
					  &logical_result);
		if (ACPI_FAILURE(status)) {
			return (FALSE);
		}
		break;

	case MATCH_MLE:
		/*
		 * True if less than or equal: (P[i] <= M) (P[i] not_greater than M)
		 * Change to:                  (M >= P[i]) (M not_less than P[i])
		 */
		status =
		    acpi_ex_do_logical_op(AML_LLESS_OP, match_obj, package_obj,
					  &logical_result);
		if (ACPI_FAILURE(status)) {
			return (FALSE);
		}
		logical_result = (u8) ! logical_result;
		break;

	case MATCH_MLT:
		/*
		 * True if less than: (P[i] < M)
		 * Change to:         (M > P[i])
		 */
		status =
		    acpi_ex_do_logical_op(AML_LGREATER_OP, match_obj,
					  package_obj, &logical_result);
		if (ACPI_FAILURE(status)) {
			return (FALSE);
		}
		break;

	case MATCH_MGE:
		/*
		 * True if greater than or equal: (P[i] >= M) (P[i] not_less than M)
		 * Change to:                     (M <= P[i]) (M not_greater than P[i])
		 */
		status =
		    acpi_ex_do_logical_op(AML_LGREATER_OP, match_obj,
					  package_obj, &logical_result);
		if (ACPI_FAILURE(status)) {
			return (FALSE);
		}
		logical_result = (u8) ! logical_result;
		break;

	case MATCH_MGT:
		/*
		 * True if greater than: (P[i] > M)
		 * Change to:            (M < P[i])
		 */
		status =
		    acpi_ex_do_logical_op(AML_LLESS_OP, match_obj, package_obj,
					  &logical_result);
		if (ACPI_FAILURE(status)) {
			return (FALSE);
		}
		break;

	default:

		/* Undefined */

		return (FALSE);
	}

	return (logical_result);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_opcode_6A_0T_1R
 *
 * PARAMETERS:  walk_state          - Current walk state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute opcode with 6 arguments, no target, and a return value
 *
 ******************************************************************************/

acpi_status acpi_ex_opcode_6A_0T_1R(struct acpi_walk_state * walk_state)
{
	union acpi_operand_object **operand = &walk_state->operands[0];
	union acpi_operand_object *return_desc = NULL;
	acpi_status status = AE_OK;
	u64 index;
	union acpi_operand_object *this_element;

	ACPI_FUNCTION_TRACE_STR(ex_opcode_6A_0T_1R,
				acpi_ps_get_opcode_name(walk_state->opcode));

	switch (walk_state->opcode) {
	case AML_MATCH_OP:
		/*
		 * Match (search_pkg[0], match_op1[1], match_obj1[2],
		 *                      match_op2[3], match_obj2[4], start_index[5])
		 */

		/* Validate both Match Term Operators (MTR, MEQ, etc.) */

		if ((operand[1]->integer.value > MAX_MATCH_OPERATOR) ||
		    (operand[3]->integer.value > MAX_MATCH_OPERATOR)) {
			ACPI_ERROR((AE_INFO, "Match operator out of range"));
			status = AE_AML_OPERAND_VALUE;
			goto cleanup;
		}

		/* Get the package start_index, validate against the package length */

		index = operand[5]->integer.value;
		if (index >= operand[0]->package.count) {
			ACPI_ERROR((AE_INFO,
				    "Index (0x%8.8X%8.8X) beyond package end (0x%X)",
				    ACPI_FORMAT_UINT64(index),
				    operand[0]->package.count));
			status = AE_AML_PACKAGE_LIMIT;
			goto cleanup;
		}

		/* Create an integer for the return value */
		/* Default return value is ACPI_UINT64_MAX if no match found */

		return_desc = acpi_ut_create_integer_object(ACPI_UINT64_MAX);
		if (!return_desc) {
			status = AE_NO_MEMORY;
			goto cleanup;

		}

		/*
		 * Examine each element until a match is found. Both match conditions
		 * must be satisfied for a match to occur. Within the loop,
		 * "continue" signifies that the current element does not match
		 * and the next should be examined.
		 *
		 * Upon finding a match, the loop will terminate via "break" at
		 * the bottom. If it terminates "normally", match_value will be
		 * ACPI_UINT64_MAX (Ones) (its initial value) indicating that no
		 * match was found.
		 */
		for (; index < operand[0]->package.count; index++) {

			/* Get the current package element */

			this_element = operand[0]->package.elements[index];

			/* Treat any uninitialized (NULL) elements as non-matching */

			if (!this_element) {
				continue;
			}

			/*
			 * Both match conditions must be satisfied. Execution of a continue
			 * (proceed to next iteration of enclosing for loop) signifies a
			 * non-match.
			 */
			if (!acpi_ex_do_match((u32) operand[1]->integer.value,
					      this_element, operand[2])) {
				continue;
			}

			if (!acpi_ex_do_match((u32) operand[3]->integer.value,
					      this_element, operand[4])) {
				continue;
			}

			/* Match found: Index is the return value */

			return_desc->integer.value = index;
			break;
		}
		break;

	case AML_LOAD_TABLE_OP:

		status = acpi_ex_load_table_op(walk_state, &return_desc);
		break;

	default:

		ACPI_ERROR((AE_INFO, "Unknown AML opcode 0x%X",
			    walk_state->opcode));
		status = AE_AML_BAD_OPCODE;
		goto cleanup;
	}

cleanup:

	/* Delete return object on error */

	if (ACPI_FAILURE(status)) {
		acpi_ut_remove_reference(return_desc);
	}

	/* Save return object on success */

	else {
		walk_state->result_obj = return_desc;
	}

	return_ACPI_STATUS(status);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              /******************************************************************************
 *
 * Module Name: exprep - ACPI AML (p-code) execution - field prep utilities
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
#include "acnamesp.h"
#include "acdispat.h"

#define _COMPONENT          ACPI_EXECUTER
ACPI_MODULE_NAME("exprep")

/* Local prototypes */
static u32
acpi_ex_decode_field_access(union acpi_operand_object *obj_desc,
			    u8 field_flags, u32 * return_byte_alignment);

#ifdef ACPI_UNDER_DEVELOPMENT

static u32
acpi_ex_generate_access(u32 field_bit_offset,
			u32 field_bit_length, u32 region_length);

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_generate_access
 *
 * PARAMETERS:  field_bit_offset    - Start of field within parent region/buffer
 *              field_bit_length    - Length of field in bits
 *              region_length       - Length of parent in bytes
 *
 * RETURN:      Field granularity (8, 16, 32 or 64) and
 *              byte_alignment (1, 2, 3, or 4)
 *
 * DESCRIPTION: Generate an optimal access width for fields defined with the
 *              any_acc keyword.
 *
 * NOTE: Need to have the region_length in order to check for boundary
 *       conditions (end-of-region). However, the region_length is a deferred
 *       operation. Therefore, to complete this implementation, the generation
 *       of this access width must be deferred until the region length has
 *       been evaluated.
 *
 ******************************************************************************/

static u32
acpi_ex_generate_access(u32 field_bit_offset,
			u32 field_bit_length, u32 region_length)
{
	u32 field_byte_length;
	u32 field_byte_offset;
	u32 field_byte_end_offset;
	u32 access_byte_width;
	u32 field_start_offset;
	u32 field_end_offset;
	u32 minimum_access_width = 0xFFFFFFFF;
	u32 minimum_accesses = 0xFFFFFFFF;
	u32 accesses;

	ACPI_FUNCTION_TRACE(ex_generate_access);

	/* Round Field start offset and length to "minimal" byte boundaries */

	field_byte_offset = ACPI_DIV_8(ACPI_ROUND_DOWN(field_bit_offset, 8));
	field_byte_end_offset = ACPI_DIV_8(ACPI_ROUND_UP(field_bit_length +
							 field_bit_offset, 8));
	field_byte_length = field_byte_end_offset - field_byte_offset;

	ACPI_DEBUG_PRINT((ACPI_DB_BFIELD,
			  "Bit length %u, Bit offset %u\n",
			  field_bit_length, field_bit_offset));

	ACPI_DEBUG_PRINT((ACPI_DB_BFIELD,
			  "Byte Length %u, Byte Offset %u, End Offset %u\n",
			  field_byte_length, field_byte_offset,
			  field_byte_end_offset));

	/*
	 * Iterative search for the maximum access width that is both aligned
	 * and does not go beyond the end of the region
	 *
	 * Start at byte_acc and work upwards to qword_acc max. (1,2,4,8 bytes)
	 */
	for (access_byte_width = 1; access_byte_width <= 8;
	     access_byte_width <<= 1) {
		/*
		 * 1) Round end offset up to next access boundary and make sure that
		 *    this does not go beyond the end of the parent region.
		 * 2) When the Access width is greater than the field_byte_length, we
		 *    are done. (This does not optimize for the perfectly aligned
		 *    case yet).
		 */
		if (ACPI_ROUND_UP(field_byte_end_offset, access_byte_width) <=
		    region_length) {
			field_start_offset =
			    ACPI_ROUND_DOWN(field_byte_offset,
					    access_byte_width) /
			    access_byte_width;

			field_end_offset =
			    ACPI_ROUND_UP((field_byte_length +
					   field_byte_offset),
					  access_byte_width) /
			    access_byte_width;

			accesses = field_end_offset - field_start_offset;

			ACPI_DEBUG_PRINT((ACPI_DB_BFIELD,
					  "AccessWidth %u end is within region\n",
					  access_byte_width));

			ACPI_DEBUG_PRINT((ACPI_DB_BFIELD,
					  "Field Start %u, Field End %u -- requires %u accesses\n",
					  field_start_offset, field_end_offset,
					  accesses));

			/* Single access is optimal */

			if (accesses <= 1) {
				ACPI_DEBUG_PRINT((ACPI_DB_BFIELD,
						  "Entire field can be accessed with one operation of size %u\n",
						  access_byte_width));
				return_VALUE(access_byte_width);
			}

			/*
			 * Fits in the region, but requires more than one read/write.
			 * try the next wider access on next iteration
			 */
			if (accesses < minimum_accesses) {
				minimum_accesses = accesses;
				minimum_access_width = access_byte_width;
			}
		} else {
			ACPI_DEBUG_PRINT((ACPI_DB_BFIELD,
					  "AccessWidth %u end is NOT within region\n",
					  access_byte_width));
			if (access_byte_width == 1) {
				ACPI_DEBUG_PRINT((ACPI_DB_BFIELD,
						  "Field goes beyond end-of-region!\n"));

				/* Field does not fit in the region at all */

				return_VALUE(0);
			}

			/*
			 * This width goes beyond the end-of-region, back off to
			 * previous access
			 */
			ACPI_DEBUG_PRINT((ACPI_DB_BFIELD,
					  "Backing off to previous optimal access width of %u\n",
					  minimum_access_width));
			return_VALUE(minimum_access_width);
		}
	}

	/*
	 * Could not read/write field with one operation,
	 * just use max access width
	 */
	ACPI_DEBUG_PRINT((ACPI_DB_BFIELD,
			  "Cannot access field in one operation, using width 8\n"));
	return_VALUE(8);
}
#endif				/* ACPI_UNDER_DEVELOPMENT */

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_decode_field_access
 *
 * PARAMETERS:  obj_desc            - Field object
 *              field_flags         - Encoded fieldflags (contains access bits)
 *              return_byte_alignment - Where the byte alignment is returned
 *
 * RETURN:      Field granularity (8, 16, 32 or 64) and
 *              byte_alignment (1, 2, 3, or 4)
 *
 * DESCRIPTION: Decode the access_type bits of a field definition.
 *
 ******************************************************************************/

static u32
acpi_ex_decode_field_access(union acpi_operand_object *obj_desc,
			    u8 field_flags, u32 * return_byte_alignment)
{
	u32 access;
	u32 byte_alignment;
	u32 bit_length;

	ACPI_FUNCTION_TRACE(ex_decode_field_access);

	access = (field_flags & AML_FIELD_ACCESS_TYPE_MASK);

	switch (access) {
	case AML_FIELD_ACCESS_ANY:

#ifdef ACPI_UNDER_DEVELOPMENT
		byte_alignment =
		    acpi_ex_generate_access(obj_desc->common_field.
					    start_field_bit_offset,
					    obj_desc->common_field.bit_length,
					    0xFFFFFFFF
					    /* Temp until we pass region_length as parameter */
		    );
		bit_length = byte_alignment * 8;
#endif

		byte_alignment = 1;
		bit_length = 8;
		break;

	case AML_FIELD_ACCESS_BYTE:
	case AML_FIELD_ACCESS_BUFFER:	/* ACPI 2.0 (SMBus Buffer) */

		byte_alignment = 1;
		bit_length = 8;
		break;

	case AML_FIELD_ACCESS_WORD:

		byte_alignment = 2;
		bit_length = 16;
		break;

	case AML_FIELD_ACCESS_DWORD:

		byte_alignment = 4;
		bit_length = 32;
		break;

	case AML_FIELD_ACCESS_QWORD:	/* ACPI 2.0 */

		byte_alignment = 8;
		bit_length = 64;
		break;

	default:

		/* Invalid field access type */

		ACPI_ERROR((AE_INFO, "Unknown field access type 0x%X", access));
		return_UINT32(0);
	}

	if (obj_desc->common.type == ACPI_TYPE_BUFFER_FIELD) {
		/*
		 * buffer_field access can be on any byte boundary, so the
		 * byte_alignment is always 1 byte -- regardless of any byte_alignment
		 * implied by the field access type.
		 */
		byte_alignment = 1;
	}

	*return_byte_alignment = byte_alignment;
	return_UINT32(bit_length);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_prep_common_field_object
 *
 * PARAMETERS:  obj_desc            - The field object
 *              field_flags         - Access, lock_rule, and update_rule.
 *                                    The format of a field_flag is described
 *                                    in the ACPI specification
 *              field_attribute     - Special attributes (not used)
 *              field_bit_position  - Field start position
 *              field_bit_length    - Field length in number of bits
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Initialize the areas of the field object that are common
 *              to the various types of fields. Note: This is very "sensitive"
 *              code because we are solving the general case for field
 *              alignment.
 *
 ******************************************************************************/

acpi_status
acpi_ex_prep_common_field_object(union acpi_operand_object *obj_desc,
				 u8 field_flags,
				 u8 field_attribute,
				 u32 field_bit_position, u32 field_bit_length)
{
	u32 access_bit_width;
	u32 byte_alignment;
	u32 nearest_byte_address;

	ACPI_FUNCTION_TRACE(ex_prep_common_field_object);

	/*
	 * Note: the structure being initialized is the
	 * ACPI_COMMON_FIELD_INFO;  No structure fields outside of the common
	 * area are initialized by this procedure.
	 */
	obj_desc->common_field.field_flags = field_flags;
	obj_desc->common_field.attribute = field_attribute;
	obj_desc->common_field.bit_length = field_bit_length;

	/*
	 * Decode the access type so we can compute offsets. The access type gives
	 * two pieces of information - the width of each field access and the
	 * necessary byte_alignment (address granularity) of the access.
	 *
	 * For any_acc, the access_bit_width is the largest width that is both
	 * necessary and possible in an attempt to access the whole field in one
	 * I/O operation. However, for any_acc, the byte_alignment is always one
	 * byte.
	 *
	 * For all Buffer Fields, the byte_alignment is always one byte.
	 *
	 * For all other access types (Byte, Word, Dword, Qword), the Bitwidth is
	 * the same (equivalent) as the byte_alignment.
	 */
	access_bit_width = acpi_ex_decode_field_access(obj_desc, field_flags,
						       &byte_alignment);
	if (!access_bit_width) {
		return_ACPI_STATUS(AE_AML_OPERAND_VALUE);
	}

	/* Setup width (access granularity) fields (values are: 1, 2, 4, 8) */

	obj_desc->common_field.access_byte_width = (u8)
	    ACPI_DIV_8(access_bit_width);

	/*
	 * base_byte_offset is the address of the start of the field within the
	 * region. It is the byte address of the first *datum* (field-width data
	 * unit) of the field. (i.e., the first datum that contains at least the
	 * first *bit* of the field.)
	 *
	 * Note: byte_alignment is always either equal to the access_bit_width or 8
	 * (Byte access), and it defines the addressing granularity of the parent
	 * region or buffer.
	 */
	nearest_byte_address =
	    ACPI_ROUND_BITS_DOWN_TO_BYTES(field_bit_position);
	obj_desc->common_field.base_byte_offset = (u32)
	    ACPI_ROUND_DOWN(nearest_byte_address, byte_alignment);

	/*
	 * start_field_bit_offset is the offset of the first bit of the field within
	 * a field datum.
	 */
	obj_desc->common_field.start_field_bit_offset = (u8)
	    (field_bit_position -
	     ACPI_MUL_8(obj_desc->common_field.base_byte_offset));

	return_ACPI_STATUS(AE_OK);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_prep_field_value
 *
 * PARAMETERS:  info    - Contains all field creation info
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Construct an object of type union acpi_operand_object with a
 *              subtype of def_field and connect it to the parent Node.
 *
 ******************************************************************************/

acpi_status acpi_ex_prep_field_value(struct acpi_create_field_info *info)
{
	union acpi_operand_object *obj_desc;
	union acpi_operand_object *second_desc = NULL;
	acpi_status status;
	u32 access_byte_width;
	u32 type;

	ACPI_FUNCTION_TRACE(ex_prep_field_value);

	/* Parameter validation */

	if (info->field_type != ACPI_TYPE_LOCAL_INDEX_FIELD) {
		if (!info->region_node) {
			ACPI_ERROR((AE_INFO, "Null RegionNode"));
			return_ACPI_STATUS(AE_AML_NO_OPERAND);
		}

		type = acpi_ns_get_type(info->region_node);
		if (type != ACPI_TYPE_REGION) {
			ACPI_ERROR((AE_INFO,
				    "Needed Region, found type 0x%X (%s)", type,
				    acpi_ut_get_type_name(type)));

			return_ACPI_STATUS(AE_AML_OPERAND_TYPE);
		}
	}

	/* Allocate a new field object */

	obj_desc = acpi_ut_create_internal_object(info->field_type);
	if (!obj_desc) {
		return_ACPI_STATUS(AE_NO_MEMORY);
	}

	/* Initialize areas of the object that are common to all fields */

	obj_desc->common_field.node = info->field_node;
	status = acpi_ex_prep_common_field_object(obj_desc,
						  info->field_flags,
						  info->attribute,
						  info->field_bit_position,
						  info->field_bit_length);
	if (ACPI_FAILURE(status)) {
		acpi_ut_delete_object_desc(obj_desc);
		return_ACPI_STATUS(status);
	}

	/* Initialize areas of the object that are specific to the field type */

	switch (info->field_type) {
	case ACPI_TYPE_LOCAL_REGION_FIELD:

		obj_desc->field.region_obj =
		    acpi_ns_get_attached_object(info->region_node);

		/* Fields specific to generic_serial_bus fields */

		obj_desc->field.access_length = info->access_length;

		if (info->connection_node) {
			second_desc = info->connection_node->object;
			if (!(second_desc->common.flags & AOPOBJ_DATA_VALID)) {
				status =
				    acpi_ds_get_buffer_arguments(second_desc);
				if (ACPI_FAILURE(status)) {
					acpi_ut_delete_object_desc(obj_desc);
					return_ACPI_STATUS(status);
				}
			}

			obj_desc->field.resource_buffer =
			    second_desc->buffer.pointer;
			obj_desc->field.resource_length =
			    (u16)second_desc->buffer.length;
		} else if (info->resource_buffer) {
			obj_desc->field.resource_buffer = info->resource_buffer;
			obj_desc->field.resource_length = info->resource_length;
		}

		obj_desc->field.pin_number_index = info->pin_number_index;

		/* Allow full data read from EC address space */

		if ((obj_desc->field.region_obj->region.space_id ==
		     ACPI_ADR_SPACE_EC)
		    && (obj_desc->common_field.bit_length > 8)) {
			access_byte_width =
			    ACPI_ROUND_BITS_UP_TO_BYTES(obj_desc->common_field.
							bit_length);

			/* Maximum byte width supported is 255 */

			if (access_byte_width < 256) {
				obj_desc->common_field.access_byte_width =
				    (u8)access_byte_width;
			}
		}
		ACPI_DEBUG_PRINT((ACPI_DB_BFIELD,
				  "RegionField: BitOff %X, Off %X, Gran %X, Region %p\n",
				  obj_desc->field.start_field_bit_offset,
				  obj_desc->field.base_byte_offset,
				  obj_desc->field.access_byte_width,
				  obj_desc->field.region_obj));
		break;

	case ACPI_TYPE_LOCAL_BANK_FIELD:

		obj_desc->bank_field.value = info->bank_value;
		obj_desc->bank_field.region_obj =
		    acpi_ns_get_attached_object(info->region_node);
		obj_desc->bank_field.bank_obj =
		    acpi_ns_get_attached_object(info->register_node);

		/* An additional reference for the attached objects */

		acpi_ut_add_reference(obj_desc->bank_field.region_obj);
		acpi_ut_add_reference(obj_desc->bank_field.bank_obj);

		ACPI_DEBUG_PRINT((ACPI_DB_BFIELD,
				  "Bank Field: BitOff %X, Off %X, Gran %X, Region %p, BankReg %p\n",
				  obj_desc->bank_field.start_field_bit_offset,
				  obj_desc->bank_field.base_byte_offset,
				  obj_desc->field.access_byte_width,
				  obj_desc->bank_field.region_obj,
				  obj_desc->bank_field.bank_obj));

		/*
		 * Remember location in AML stream of the field unit
		 * opcode and operands -- since the bank_value
		 * operands must be evaluated.
		 */
		second_desc = obj_desc->common.next_object;
		second_desc->extra.aml_start =
		    ACPI_CAST_PTR(union acpi_parse_object,
				  info->data_register_node)->named.data;
		second_desc->extra.aml_length =
		    ACPI_CAST_PTR(union acpi_parse_object,
				  info->data_register_node)->named.length;

		break;

	case ACPI_TYPE_LOCAL_INDEX_FIELD:

		/* Get the Index and Data registers */

		obj_desc->index_field.index_obj =
		    acpi_ns_get_attached_object(info->register_node);
		obj_desc->index_field.data_obj =
		    acpi_ns_get_attached_object(info->data_register_node);

		if (!obj_desc->index_field.data_obj
		    || !obj_desc->index_field.index_obj) {
			ACPI_ERROR((AE_INFO,
				    "Null Index Object during field prep"));
			acpi_ut_delete_object_desc(obj_desc);
			return_ACPI_STATUS(AE_AML_INTERNAL);
		}

		/* An additional reference for the attached objects */

		acpi_ut_add_reference(obj_desc->index_field.data_obj);
		acpi_ut_add_reference(obj_desc->index_field.index_obj);

		/*
		 * April 2006: Changed to match MS behavior
		 *
		 * The value written to the Index register is the byte offset of the
		 * target field in units of the granularity of the index_field
		 *
		 * Previously, the value was calculated as an index in terms of the
		 * width of the Data register, as below:
		 *
		 *      obj_desc->index_field.Value = (u32)
		 *          (Info->field_bit_position / ACPI_MUL_8 (
		 *              obj_desc->Field.access_byte_width));
		 *
		 * February 2006: Tried value as a byte offset:
		 *      obj_desc->index_field.Value = (u32)
		 *          ACPI_DIV_8 (Info->field_bit_position);
		 */
		obj_desc->index_field.value =
		    (u32) ACPI_ROUND_DOWN(ACPI_DIV_8(info->field_bit_position),
					  obj_desc->index_field.
					  access_byte_width);

		ACPI_DEBUG_PRINT((ACPI_DB_BFIELD,
				  "IndexField: BitOff %X, Off %X, Value %X, Gran %X, Index %p, Data %p\n",
				  obj_desc->index_field.start_field_bit_offset,
				  obj_desc->index_field.base_byte_offset,
				  obj_desc->index_field.value,
				  obj_desc->field.access_byte_width,
				  obj_desc->index_field.index_obj,
				  obj_desc->index_field.data_obj));
		break;

	default:

		/* No other types should get here */

		break;
	}

	/*
	 * Store the constructed descriptor (obj_desc) into the parent Node,
	 * preserving the current type of that named_obj.
	 */
	status = acpi_ns_attach_object(info->field_node, obj_desc,
				       acpi_ns_get_type(info->field_node));

	ACPI_DEBUG_PRINT((ACPI_DB_BFIELD,
			  "Set NamedObj %p [%4.4s], ObjDesc %p\n",
			  info->field_node,
			  acpi_ut_get_node_name(info->field_node), obj_desc));

	/* Remove local reference to the object */

	acpi_ut_remove_reference(obj_desc);
	return_ACPI_STATUS(status);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     /******************************************************************************
 *
 * Module Name: exregion - ACPI default op_region (address space) handlers
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
ACPI_MODULE_NAME("exregion")

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_system_memory_space_handler
 *
 * PARAMETERS:  function            - Read or Write operation
 *              address             - Where in the space to read or write
 *              bit_width           - Field width in bits (8, 16, or 32)
 *              value               - Pointer to in or out value
 *              handler_context     - Pointer to Handler's context
 *              region_context      - Pointer to context specific to the
 *                                    accessed region
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Handler for the System Memory address space (Op Region)
 *
 ******************************************************************************/
acpi_status
acpi_ex_system_memory_space_handler(u32 function,
				    acpi_physical_address address,
				    u32 bit_width,
				    u64 *value,
				    void *handler_context, void *region_context)
{
	acpi_status status = AE_OK;
	void *logical_addr_ptr = NULL;
	struct acpi_mem_space_context *mem_info = region_context;
	u32 length;
	acpi_size map_length;
	acpi_size page_boundary_map_length;
#ifdef ACPI_MISALIGNMENT_NOT_SUPPORTED
	u32 remainder;
#endif

	ACPI_FUNCTION_TRACE(ex_system_memory_space_handler);

	/* Validate and translate the bit width */

	switch (bit_width) {
	case 8:

		length = 1;
		break;

	case 16:

		length = 2;
		break;

	case 32:

		length = 4;
		break;

	case 64:

		length = 8;
		break;

	default:

		ACPI_ERROR((AE_INFO, "Invalid SystemMemory width %u",
			    bit_width));
		return_ACPI_STATUS(AE_AML_OPERAND_VALUE);
	}

#ifdef ACPI_MISALIGNMENT_NOT_SUPPORTED
	/*
	 * Hardware does not support non-aligned data transfers, we must verify
	 * the request.
	 */
	(void)acpi_ut_short_divide((u64) address, length, NULL, &remainder);
	if (remainder != 0) {
		return_ACPI_STATUS(AE_AML_ALIGNMENT);
	}
#endif

	/*
	 * Does the request fit into the cached memory mapping?
	 * Is 1) Address below the current mapping? OR
	 *    2) Address beyond the current mapping?
	 */
	if ((address < mem_info->mapped_physical_address) ||
	    (((u64) address + length) > ((u64)
					 mem_info->mapped_physical_address +
					 mem_info->mapped_length))) {
		/*
		 * The request cannot be resolved by the current memory mapping;
		 * Delete the existing mapping and create a new one.
		 */
		if (mem_info->mapped_length) {

			/* Valid mapping, delete it */

			acpi_os_unmap_memory(mem_info->mapped_logical_address,
					     mem_info->mapped_length);
		}

		/*
		 * October 2009: Attempt to map from the requested address to the
		 * end of the region. However, we will never map more than one
		 * page, nor will we cross a page boundary.
		 */
		map_length = (acpi_size)
		    ((mem_info->address + mem_info->length) - address);

		/*
		 * If mapping the entire remaining portion of the region will cross
		 * a page boundary, just map up to the page boundary, do not cross.
		 * On some systems, crossing a page boundary while mapping regions
		 * can cause warnings if the pages have different attributes
		 * due to resource management.
		 *
		 * This has the added benefit of constraining a single mapping to
		 * one page, which is similar to the original code that used a 4k
		 * maximum window.
		 */
		page_boundary_map_length = (acpi_size)
		    (ACPI_ROUND_UP(address, ACPI_DEFAULT_PAGE_SIZE) - address);
		if (page_boundary_map_length == 0) {
			page_boundary_map_length = ACPI_DEFAULT_PAGE_SIZE;
		}

		if (map_length > page_boundary_map_length) {
			map_length = page_boundary_map_length;
		}

		/* Create a new mapping starting at the address given */

		mem_info->mapped_logical_address =
		    acpi_os_map_memory(address, map_length);
		if (!mem_info->mapped_logical_address) {
			ACPI_ERROR((AE_INFO,
				    "Could not map memory at 0x%8.8X%8.8X, size %u",
				    ACPI_FORMAT_UINT64(address),
				    (u32)map_length));
			mem_info->mapped_length = 0;
			return_ACPI_STATUS(AE_NO_MEMORY);
		}

		/* Save the physical address and mapping size */

		mem_info->mapped_physical_address = address;
		mem_info->mapped_length = map_length;
	}

	/*
	 * Generate a logical pointer corresponding to the address we want to
	 * access
	 */
	logical_addr_ptr = mem_info->mapped_logical_address +
	    ((u64) address - (u64) mem_info->mapped_physical_address);

	ACPI_DEBUG_PRINT((ACPI_DB_INFO,
			  "System-Memory (width %u) R/W %u Address=%8.8X%8.8X\n",
			  bit_width, function, ACPI_FORMAT_UINT64(address)));

	/*
	 * Perform the memory read or write
	 *
	 * Note: For machines that do not support non-aligned transfers, the target
	 * address was checked for alignment above. We do not attempt to break the
	 * transfer up into smaller (byte-size) chunks because the AML specifically
	 * asked for a transfer width that the hardware may require.
	 */
	switch (function) {
	case ACPI_READ:

		*value = 0;
		switch (bit_width) {
		case 8:

			*value = (u64)ACPI_GET8(logical_addr_ptr);
			break;

		case 16:

			*value = (u64)ACPI_GET16(logical_addr_ptr);
			break;

		case 32:

			*value = (u64)ACPI_GET32(logical_addr_ptr);
			break;

		case 64:

			*value = (u64)ACPI_GET64(logical_addr_ptr);
			break;

		default:

			/* bit_width was already validated */

			break;
		}
		break;

	case ACPI_WRITE:

		switch (bit_width) {
		case 8:

			ACPI_SET8(logical_addr_ptr, *value);
			break;

		case 16:

			ACPI_SET16(logical_addr_ptr, *value);
			break;

		case 32:

			ACPI_SET32(logical_addr_ptr, *value);
			break;

		case 64:

			ACPI_SET64(logical_addr_ptr, *value);
			break;

		default:

			/* bit_width was already validated */

			break;
		}
		break;

	default:

		status = AE_BAD_PARAMETER;
		break;
	}

	return_ACPI_STATUS(status);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_system_io_space_handler
 *
 * PARAMETERS:  function            - Read or Write operation
 *              address             - Where in the space to read or write
 *              bit_width           - Field width in bits (8, 16, or 32)
 *              value               - Pointer to in or out value
 *              handler_context     - Pointer to Handler's context
 *              region_context      - Pointer to context specific to the
 *                                    accessed region
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Handler for the System IO address space (Op Region)
 *
 ******************************************************************************/

acpi_status
acpi_ex_system_io_space_handler(u32 function,
				acpi_physical_address address,
				u32 bit_width,
				u64 *value,
				void *handler_context, void *region_context)
{
	acpi_status status = AE_OK;
	u32 value32;

	ACPI_FUNCTION_TRACE(ex_system_io_space_handler);

	ACPI_DEBUG_PRINT((ACPI_DB_INFO,
			  "System-IO (width %u) R/W %u Address=%8.8X%8.8X\n",
			  bit_width, function, ACPI_FORMAT_UINT64(address)));

	/* Decode the function parameter */

	switch (function) {
	case ACPI_READ:

		status = acpi_hw_read_port((acpi_io_address) address,
					   &value32, bit_width);
		*value = value32;
		break;

	case ACPI_WRITE:

		status = acpi_hw_write_port((acpi_io_address) address,
					    (u32) * value, bit_width);
		break;

	default:

		status = AE_BAD_PARAMETER;
		break;
	}

	return_ACPI_STATUS(status);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_pci_config_space_handler
 *
 * PARAMETERS:  function            - Read or Write operation
 *              address             - Where in the space to read or write
 *              bit_width           - Field width in bits (8, 16, or 32)
 *              value               - Pointer to in or out value
 *              handler_context     - Pointer to Handler's context
 *              region_context      - Pointer to context specific to the
 *                                    accessed region
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Handler for the PCI Config address space (Op Region)
 *
 ******************************************************************************/

acpi_status
acpi_ex_pci_config_space_handler(u32 function,
				 acpi_physical_address address,
				 u32 bit_width,
				 u64 *value,
				 void *handler_context, void *region_context)
{
	acpi_status status = AE_OK;
	struct acpi_pci_id *pci_id;
	u16 pci_register;

	ACPI_FUNCTION_TRACE(ex_pci_config_space_handler);

	/*
	 *  The arguments to acpi_os(Read|Write)pci_configuration are:
	 *
	 *  pci_segment is the PCI bus segment range 0-31
	 *  pci_bus     is the PCI bus number range 0-255
	 *  pci_device  is the PCI device number range 0-31
	 *  pci_function is the PCI device function number
	 *  pci_register is the Config space register range 0-255 bytes
	 *
	 *  value - input value for write, output address for read
	 *
	 */
	pci_id = (struct acpi_pci_id *)region_context;
	pci_register = (u16) (u32) address;

	ACPI_DEBUG_PRINT((ACPI_DB_INFO,
			  "Pci-Config %u (%u) Seg(%04x) Bus(%04x) Dev(%04x) Func(%04x) Reg(%04x)\n",
			  function, bit_width, pci_id->segment, pci_id->bus,
			  pci_id->device, pci_id->function, pci_register));

	switch (function) {
	case ACPI_READ:

		*value = 0;
		status = acpi_os_read_pci_configuration(pci_id, pci_register,
							value, bit_width);
		break;

	case ACPI_WRITE:

		status = acpi_os_write_pci_configuration(pci_id, pci_register,
							 *value, bit_width);
		break;

	default:

		status = AE_BAD_PARAMETER;
		break;
	}

	return_ACPI_STATUS(status);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_cmos_space_handler
 *
 * PARAMETERS:  function            - Read or Write operation
 *              address             - Where in the space to read or write
 *              bit_width           - Field width in bits (8, 16, or 32)
 *              value               - Pointer to in or out value
 *              handler_context     - Pointer to Handler's context
 *              region_context      - Pointer to context specific to the
 *                                    accessed region
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Handler for the CMOS address space (Op Region)
 *
 ******************************************************************************/

acpi_status
acpi_ex_cmos_space_handler(u32 function,
			   acpi_physical_address address,
			   u32 bit_width,
			   u64 *value,
			   void *handler_context, void *region_context)
{
	acpi_status status = AE_OK;

	ACPI_FUNCTION_TRACE(ex_cmos_space_handler);

	return_ACPI_STATUS(status);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_pci_bar_space_handler
 *
 * PARAMETERS:  function            - Read or Write operation
 *              address             - Where in the space to read or write
 *              bit_width           - Field width in bits (8, 16, or 32)
 *              value               - Pointer to in or out value
 *              handler_context     - Pointer to Handler's context
 *              region_context      - Pointer to context specific to the
 *                                    accessed region
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Handler for the PCI bar_target address space (Op Region)
 *
 ******************************************************************************/

acpi_status
acpi_ex_pci_bar_space_handler(u32 function,
			      acpi_physical_address address,
			      u32 bit_width,
			      u64 *value,
			      void *handler_context, void *region_context)
{
	acpi_status status = AE_OK;

	ACPI_FUNCTION_TRACE(ex_pci_bar_space_handler);

	return_ACPI_STATUS(status);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_data_table_space_handler
 *
 * PARAMETERS:  function            - Read or Write operation
 *              address             - Where in the space to read or write
 *              bit_width           - Field width in bits (8, 16, or 32)
 *              value               - Pointer to in or out value
 *              handler_context     - Pointer to Handler's context
 *              region_context      - Pointer to context specific to the
 *                                    accessed region
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Handler for the Data Table address space (Op Region)
 *
 ******************************************************************************/

acpi_status
acpi_ex_data_table_space_handler(u32 function,
				 acpi_physical_address address,
				 u32 bit_width,
				 u64 *value,
				 void *handler_context, void *region_context)
{
	ACPI_FUNCTION_TRACE(ex_data_table_space_handler);

	/*
	 * Perform the memory read or write. The bit_width was already
	 * validated.
	 */
	switch (function) {
	case ACPI_READ:

		memcpy(ACPI_CAST_PTR(char, value),
		       ACPI_PHYSADDR_TO_PTR(address), ACPI_DIV_8(bit_width));
		break;

	case ACPI_WRITE:

		memcpy(ACPI_PHYSADDR_TO_PTR(address),
		       ACPI_CAST_PTR(char, value), ACPI_DIV_8(bit_width));
		break;

	default:

		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	return_ACPI_STATUS(AE_OK);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      /******************************************************************************
 *
 * Module Name: exresnte - AML Interpreter object resolution
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
#include "acdispat.h"
#include "acinterp.h"
#include "acnamesp.h"

#define _COMPONENT          ACPI_EXECUTER
ACPI_MODULE_NAME("exresnte")

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_resolve_node_to_value
 *
 * PARAMETERS:  object_ptr      - Pointer to a location that contains
 *                                a pointer to a NS node, and will receive a
 *                                pointer to the resolved object.
 *              walk_state      - Current state. Valid only if executing AML
 *                                code. NULL if simply resolving an object
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Resolve a Namespace node to a valued object
 *
 * Note: for some of the data types, the pointer attached to the Node
 * can be either a pointer to an actual internal object or a pointer into the
 * AML stream itself. These types are currently:
 *
 *      ACPI_TYPE_INTEGER
 *      ACPI_TYPE_STRING
 *      ACPI_TYPE_BUFFER
 *      ACPI_TYPE_MUTEX
 *      ACPI_TYPE_PACKAGE
 *
 ******************************************************************************/
acpi_status
acpi_ex_resolve_node_to_value(struct acpi_namespace_node **object_ptr,
			      struct acpi_walk_state *walk_state)
{
	acpi_status status = AE_OK;
	union acpi_operand_object *source_desc;
	union acpi_operand_object *obj_desc = NULL;
	struct acpi_namespace_node *node;
	acpi_object_type entry_type;

	ACPI_FUNCTION_TRACE(ex_resolve_node_to_value);

	/*
	 * The stack pointer points to a struct acpi_namespace_node (Node). Get the
	 * object that is attached to the Node.
	 */
	node = *object_ptr;
	source_desc = acpi_ns_get_attached_object(node);
	entry_type = acpi_ns_get_type((acpi_handle) node);

	ACPI_DEBUG_PRINT((ACPI_DB_EXEC, "Entry=%p SourceDesc=%p [%s]\n",
			  node, source_desc,
			  acpi_ut_get_type_name(entry_type)));

	if ((entry_type == ACPI_TYPE_LOCAL_ALIAS) ||
	    (entry_type == ACPI_TYPE_LOCAL_METHOD_ALIAS)) {

		/* There is always exactly one level of indirection */

		node = ACPI_CAST_PTR(struct acpi_namespace_node, node->object);
		source_desc = acpi_ns_get_attached_object(node);
		entry_type = acpi_ns_get_type((acpi_handle) node);
		*object_ptr = node;
	}

	/*
	 * Several object types require no further processing:
	 * 1) Device/Thermal objects don't have a "real" subobject, return the Node
	 * 2) Method locals and arguments have a pseudo-Node
	 * 3) 10/2007: Added method type to assist with Package construction.
	 */
	if ((entry_type == ACPI_TYPE_DEVICE) ||
	    (entry_type == ACPI_TYPE_THERMAL) ||
	    (entry_type == ACPI_TYPE_METHOD) ||
	    (node->flags & (ANOBJ_METHOD_ARG | ANOBJ_METHOD_LOCAL))) {
		return_ACPI_STATUS(AE_OK);
	}

	if (!source_desc) {
		ACPI_ERROR((AE_INFO, "No object attached to node [%4.4s] %p",
			    node->name.ascii, node));
		return_ACPI_STATUS(AE_AML_UNINITIALIZED_NODE);
	}

	/*
	 * Action is based on the type of the Node, which indicates the type
	 * of the attached object or pointer
	 */
	switch (entry_type) {
	case ACPI_TYPE_PACKAGE:

		if (source_desc->common.type != ACPI_TYPE_PACKAGE) {
			ACPI_ERROR((AE_INFO, "Object not a Package, type %s",
				    acpi_ut_get_object_type_name(source_desc)));
			return_ACPI_STATUS(AE_AML_OPERAND_TYPE);
		}

		status = acpi_ds_get_package_arguments(source_desc);
		if (ACPI_SUCCESS(status)) {

			/* Return an additional reference to the object */

			obj_desc = source_desc;
			acpi_ut_add_reference(obj_desc);
		}
		break;

	case ACPI_TYPE_BUFFER:

		if (source_desc->common.type != ACPI_TYPE_BUFFER) {
			ACPI_ERROR((AE_INFO, "Object not a Buffer, type %s",
				    acpi_ut_get_object_type_name(source_desc)));
			return_ACPI_STATUS(AE_AML_OPERAND_TYPE);
		}

		status = acpi_ds_get_buffer_arguments(source_desc);
		if (ACPI_SUCCESS(status)) {

			/* Return an additional reference to the object */

			obj_desc = source_desc;
			acpi_ut_add_reference(obj_desc);
		}
		break;

	case ACPI_TYPE_STRING:

		if (source_desc->common.type != ACPI_TYPE_STRING) {
			ACPI_ERROR((AE_INFO, "Object not a String, type %s",
				    acpi_ut_get_object_type_name(source_desc)));
			return_ACPI_STATUS(AE_AML_OPERAND_TYPE);
		}

		/* Return an additional reference to the object */

		obj_desc = source_desc;
		acpi_ut_add_reference(obj_desc);
		break;

	case ACPI_TYPE_INTEGER:

		if (source_desc->common.type != ACPI_TYPE_INTEGER) {
			ACPI_ERROR((AE_INFO, "Object not a Integer, type %s",
				    acpi_ut_get_object_type_name(source_desc)));
			return_ACPI_STATUS(AE_AML_OPERAND_TYPE);
		}

		/* Return an additional reference to the object */

		obj_desc = source_desc;
		acpi_ut_add_reference(obj_desc);
		break;

	case ACPI_TYPE_BUFFER_FIELD:
	case ACPI_TYPE_LOCAL_REGION_FIELD:
	case ACPI_TYPE_LOCAL_BANK_FIELD:
	case ACPI_TYPE_LOCAL_INDEX_FIELD:

		ACPI_DEBUG_PRINT((ACPI_DB_EXEC,
				  "FieldRead Node=%p SourceDesc=%p Type=%X\n",
				  node, source_desc, entry_type));

		status =
		    acpi_ex_read_data_from_field(walk_state, source_desc,
						 &obj_desc);
		break;

		/* For these objects, just return the object attached to the Node */

	case ACPI_TYPE_MUTEX:
	case ACPI_TYPE_POWER:
	case ACPI_TYPE_PROCESSOR:
	case ACPI_TYPE_EVENT:
	case ACPI_TYPE_REGION:

		/* Return an additional reference to the object */

		obj_desc = source_desc;
		acpi_ut_add_reference(obj_desc);
		break;

		/* TYPE_ANY is untyped, and thus there is no object associated with it */

	case ACPI_TYPE_ANY:

		ACPI_ERROR((AE_INFO,
			    "Untyped entry %p, no attached object!", node));

		return_ACPI_STATUS(AE_AML_OPERAND_TYPE);	/* Cannot be AE_TYPE */

	case ACPI_TYPE_LOCAL_REFERENCE:

		switch (source_desc->reference.class) {
		case ACPI_REFCLASS_TABLE:	/* This is a ddb_handle */
		case ACPI_REFCLASS_REFOF:
		case ACPI_REFCLASS_INDEX:

			/* Return an additional reference to the object */

			obj_desc = source_desc;
			acpi_ut_add_reference(obj_desc);
			break;

		default:

			/* No named references are allowed here */

			ACPI_ERROR((AE_INFO,
				    "Unsupported Reference type 0x%X",
				    source_desc->reference.class));

			return_ACPI_STATUS(AE_AML_OPERAND_TYPE);
		}
		break;

	default:

		/* Default case is for unknown types */

		ACPI_ERROR((AE_INFO,
			    "Node %p - Unknown object type 0x%X",
			    node, entry_type));

		return_ACPI_STATUS(AE_AML_OPERAND_TYPE);

	}			/* switch (entry_type) */

	/* Return the object descriptor */

	*object_ptr = (void *)obj_desc;
	return_ACPI_STATUS(status);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 /******************************************************************************
 *
 * Module Name: exresolv - AML Interpreter object resolution
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
#include "amlcode.h"
#include "acdispat.h"
#include "acinterp.h"
#include "acnamesp.h"

#define _COMPONENT          ACPI_EXECUTER
ACPI_MODULE_NAME("exresolv")

/* Local prototypes */
static acpi_status
acpi_ex_resolve_object_to_value(union acpi_operand_object **stack_ptr,
				struct acpi_walk_state *walk_state);

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_resolve_to_value
 *
 * PARAMETERS:  **stack_ptr         - Points to entry on obj_stack, which can
 *                                    be either an (union acpi_operand_object *)
 *                                    or an acpi_handle.
 *              walk_state          - Current method state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Convert Reference objects to values
 *
 ******************************************************************************/

acpi_status
acpi_ex_resolve_to_value(union acpi_operand_object **stack_ptr,
			 struct acpi_walk_state *walk_state)
{
	acpi_status status;

	ACPI_FUNCTION_TRACE_PTR(ex_resolve_to_value, stack_ptr);

	if (!stack_ptr || !*stack_ptr) {
		ACPI_ERROR((AE_INFO, "Internal - null pointer"));
		return_ACPI_STATUS(AE_AML_NO_OPERAND);
	}

	/*
	 * The entity pointed to by the stack_ptr can be either
	 * 1) A valid union acpi_operand_object, or
	 * 2) A struct acpi_namespace_node (named_obj)
	 */
	if (ACPI_GET_DESCRIPTOR_TYPE(*stack_ptr) == ACPI_DESC_TYPE_OPERAND) {
		status = acpi_ex_resolve_object_to_value(stack_ptr, walk_state);
		if (ACPI_FAILURE(status)) {
			return_ACPI_STATUS(status);
		}

		if (!*stack_ptr) {
			ACPI_ERROR((AE_INFO, "Internal - null pointer"));
			return_ACPI_STATUS(AE_AML_NO_OPERAND);
		}
	}

	/*
	 * Object on the stack may have changed if acpi_ex_resolve_object_to_value()
	 * was called (i.e., we can't use an _else_ here.)
	 */
	if (ACPI_GET_DESCRIPTOR_TYPE(*stack_ptr) == ACPI_DESC_TYPE_NAMED) {
		status =
		    acpi_ex_resolve_node_to_value(ACPI_CAST_INDIRECT_PTR
						  (struct acpi_namespace_node,
						   stack_ptr), walk_state);
		if (ACPI_FAILURE(status)) {
			return_ACPI_STATUS(status);
		}
	}

	ACPI_DEBUG_PRINT((ACPI_DB_EXEC, "Resolved object %p\n", *stack_ptr));
	return_ACPI_STATUS(AE_OK);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_resolve_object_to_value
 *
 * PARAMETERS:  stack_ptr       - Pointer to an internal object
 *              walk_state      - Current method state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Retrieve the value from an internal object. The Reference type
 *              uses the associated AML opcode to determine the value.
 *
 ******************************************************************************/

static acpi_status
acpi_ex_resolve_object_to_value(union acpi_operand_object **stack_ptr,
				struct acpi_walk_state *walk_state)
{
	acpi_status status = AE_OK;
	union acpi_operand_object *stack_desc;
	union acpi_operand_object *obj_desc = NULL;
	u8 ref_type;

	ACPI_FUNCTION_TRACE(ex_resolve_object_to_value);

	stack_desc = *stack_ptr;

	/* This is an object of type union acpi_operand_object */

	switch (stack_desc->common.type) {
	case ACPI_TYPE_LOCAL_REFERENCE:

		ref_type = stack_desc->reference.class;

		switch (ref_type) {
		case ACPI_REFCLASS_LOCAL:
		case ACPI_REFCLASS_ARG:
			/*
			 * Get the local from the method's state info
			 * Note: this increments the local's object reference count
			 */
			status = acpi_ds_method_data_get_value(ref_type,
							       stack_desc->
							       reference.value,
							       walk_state,
							       &obj_desc);
			if (ACPI_FAILURE(status)) {
				return_ACPI_STATUS(status);
			}

			ACPI_DEBUG_PRINT((ACPI_DB_EXEC,
					  "[Arg/Local %X] ValueObj is %p\n",
					  stack_desc->reference.value,
					  obj_desc));

			/*
			 * Now we can delete the original Reference Object and
			 * replace it with the resolved value
			 */
			acpi_ut_remove_reference(stack_desc);
			*stack_ptr = obj_desc;
			break;

		case ACPI_REFCLASS_INDEX:

			switch (stack_desc->reference.target_type) {
			case ACPI_TYPE_BUFFER_FIELD:

				/* Just return - do not dereference */
				break;

			case ACPI_TYPE_PACKAGE:

				/* If method call or copy_object - do not dereference */

				if ((walk_state->opcode ==
				     AML_INT_METHODCALL_OP)
				    || (walk_state->opcode == AML_COPY_OP)) {
					break;
				}

				/* Otherwise, dereference the package_index to a package element */

				obj_desc = *stack_desc->reference.where;
				if (obj_desc) {
					/*
					 * Valid object descriptor, copy pointer to return value
					 * (i.e., dereference the package index)
					 * Delete the ref object, increment the returned object
					 */
					acpi_ut_add_reference(obj_desc);
					*stack_ptr = obj_desc;
				} else {
					/*
					 * A NULL object descriptor means an uninitialized element of
					 * the package, can't dereference it
					 */
					ACPI_ERROR((AE_INFO,
						    "Attempt to dereference an Index to NULL package element Idx=%p",
						    stack_desc));
					status = AE_AML_UNINITIALIZED_ELEMENT;
				}
				break;

			default:

				/* Invalid reference object */

				ACPI_ERROR((AE_INFO,
					    "Unknown TargetType 0x%X in Index/Reference object %p",
					    stack_desc->reference.target_type,
					    stack_desc));
				status = AE_AML_INTERNAL;
				break;
			}
			break;

		case ACPI_REFCLASS_REFOF:
		case ACPI_REFCLASS_DEBUG:
		case ACPI_REFCLASS_TABLE:

			/* Just leave the object as-is, do not dereference */

			break;

		case ACPI_REFCLASS_NAME:	/* Reference to a named object */

			/* Dereference the name */

			if ((stack_desc->reference.node->type ==
			     ACPI_TYPE_DEVICE)
			    || (stack_desc->reference.node->type ==
				ACPI_TYPE_THERMAL)) {

				/* These node types do not have 'real' subobjects */

				*stack_ptr = (void *)stack_desc->reference.node;
			} else {
				/* Get the object pointed to by the namespace node */

				*stack_ptr =
				    (stack_desc->reference.node)->object;
				acpi_ut_add_reference(*stack_ptr);
			}

			acpi_ut_remove_reference(stack_desc);
			break;

		default:

			ACPI_ERROR((AE_INFO,
				    "Unknown Reference type 0x%X in %p",
				    ref_type, stack_desc));
			status = AE_AML_INTERNAL;
			break;
		}
		break;

	case ACPI_TYPE_BUFFER:

		status = acpi_ds_get_buffer_arguments(stack_desc);
		break;

	case ACPI_TYPE_PACKAGE:

		status = acpi_ds_get_package_arguments(stack_desc);
		break;

	case ACPI_TYPE_BUFFER_FIELD:
	case ACPI_TYPE_LOCAL_REGION_FIELD:
	case ACPI_TYPE_LOCAL_BANK_FIELD:
	case ACPI_TYPE_LOCAL_INDEX_FIELD:

		ACPI_DEBUG_PRINT((ACPI_DB_EXEC,
				  "FieldRead SourceDesc=%p Type=%X\n",
				  stack_desc, stack_desc->common.type));

		status =
		    acpi_ex_read_data_from_field(walk_state, stack_desc,
						 &obj_desc);

		/* Remove a reference to the original operand, then override */

		acpi_ut_remove_reference(*stack_ptr);
		*stack_ptr = (void *)obj_desc;
		break;

	default:

		break;
	}

	return_ACPI_STATUS(status);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_resolve_multiple
 *
 * PARAMETERS:  walk_state          - Current state (contains AML opcode)
 *              operand             - Starting point for resolution
 *              return_type         - Where the object type is returned
 *              return_desc         - Where the resolved object is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Return the base object and type. Traverse a reference list if
 *              necessary to get to the base object.
 *
 ******************************************************************************/

acpi_status
acpi_ex_resolve_multiple(struct acpi_walk_state *walk_state,
			 union acpi_operand_object *operand,
			 acpi_object_type * return_type,
			 union acpi_operand_object **return_desc)
{
	union acpi_operand_object *obj_desc = ACPI_CAST_PTR(void, operand);
	struct acpi_namespace_node *node =
	    ACPI_CAST_PTR(struct acpi_namespace_node, operand);
	acpi_object_type type;
	acpi_status status;

	ACPI_FUNCTION_TRACE(acpi_ex_resolve_multiple);

	/* Operand can be either a namespace node or an operand descriptor */

	switch (ACPI_GET_DESCRIPTOR_TYPE(obj_desc)) {
	case ACPI_DESC_TYPE_OPERAND:

		type = obj_desc->common.type;
		break;

	case ACPI_DESC_TYPE_NAMED:

		type = ((struct acpi_namespace_node *)obj_desc)->type;
		obj_desc = acpi_ns_get_attached_object(node);

		/* If we had an Alias node, use the attached object for type info */

		if (type == ACPI_TYPE_LOCAL_ALIAS) {
			type = ((struct acpi_namespace_node *)obj_desc)->type;
			obj_desc =
			    acpi_ns_get_attached_object((struct
							 acpi_namespace_node *)
							obj_desc);
		}

		if (!obj_desc) {
			ACPI_ERROR((AE_INFO,
				    "[%4.4s] Node is unresolved or uninitialized",
				    acpi_ut_get_node_name(node)));
			return_ACPI_STATUS(AE_AML_UNINITIALIZED_NODE);
		}
		break;

	default:
		return_ACPI_STATUS(AE_AML_OPERAND_TYPE);
	}

	/* If type is anything other than a reference, we are done */

	if (type != ACPI_TYPE_LOCAL_REFERENCE) {
		goto exit;
	}

	/*
	 * For reference objects created via the ref_of, Index, or Load/load_table
	 * operators, we need to get to the base object (as per the ACPI
	 * specification of the object_type and size_of operators). This means
	 * traversing the list of possibly many nested references.
	 */
	while (obj_desc->common.type == ACPI_TYPE_LOCAL_REFERENCE) {
		switch (obj_desc->reference.class) {
		case ACPI_REFCLASS_REFOF:
		case ACPI_REFCLASS_NAME:

			/* Dereference the reference pointer */

			if (obj_desc->reference.class == ACPI_REFCLASS_REFOF) {
				node = obj_desc->reference.object;
			} else {	/* AML_INT_NAMEPATH_OP */

				node = obj_desc->reference.node;
			}

			/* All "References" point to a NS node */

			if (ACPI_GET_DESCRIPTOR_TYPE(node) !=
			    ACPI_DESC_TYPE_NAMED) {
				ACPI_ERROR((AE_INFO,
					    "Not a namespace node %p [%s]",
					    node,
					    acpi_ut_get_descriptor_name(node)));
				return_ACPI_STATUS(AE_AML_INTERNAL);
			}

			/* Get the attached object */

			obj_desc = acpi_ns_get_attached_object(node);
			if (!obj_desc) {

				/* No object, use the NS node type */

				type = acpi_ns_get_type(node);
				goto exit;
			}

			/* Check for circular references */

			if (obj_desc == operand) {
				return_ACPI_STATUS(AE_AML_CIRCULAR_REFERENCE);
			}
			break;

		case ACPI_REFCLASS_INDEX:

			/* Get the type of this reference (index into another object) */

			type = obj_desc->reference.target_type;
			if (type != ACPI_TYPE_PACKAGE) {
				goto exit;
			}

			/*
			 * The main object is a package, we want to get the type
			 * of the individual package element that is referenced by
			 * the index.
			 *
			 * This could of course in turn be another reference object.
			 */
			obj_desc = *(obj_desc->reference.where);
			if (!obj_desc) {

				/* NULL package elements are allowed */

				type = 0;	/* Uninitialized */
				goto exit;
			}
			break;

		case ACPI_REFCLASS_TABLE:

			type = ACPI_TYPE_DDB_HANDLE;
			goto exit;

		case ACPI_REFCLASS_LOCAL:
		case ACPI_REFCLASS_ARG:

			if (return_desc) {
				status =
				    acpi_ds_method_data_get_value(obj_desc->
								  reference.
								  class,
								  obj_desc->
								  reference.
								  value,
								  walk_state,
								  &obj_desc);
				if (ACPI_FAILURE(status)) {
					return_ACPI_STATUS(status);
				}
				acpi_ut_remove_reference(obj_desc);
			} else {
				status =
				    acpi_ds_method_data_get_node(obj_desc->
								 reference.
								 class,
								 obj_desc->
								 reference.
								 value,
								 walk_state,
								 &node);
				if (ACPI_FAILURE(status)) {
					return_ACPI_STATUS(status);
				}

				obj_desc = acpi_ns_get_attached_object(node);
				if (!obj_desc) {
					type = ACPI_TYPE_ANY;
					goto exit;
				}
			}
			break;

		case ACPI_REFCLASS_DEBUG:

			/* The Debug Object is of type "DebugObject" */

			type = ACPI_TYPE_DEBUG_OBJECT;
			goto exit;

		default:

			ACPI_ERROR((AE_INFO,
				    "Unknown Reference Class 0x%2.2X",
				    obj_desc->reference.class));
			return_ACPI_STATUS(AE_AML_INTERNAL);
		}
	}

	/*
	 * Now we are guaranteed to have an object that has not been created
	 * via the ref_of or Index operators.
	 */
	type = obj_desc->common.type;

exit:
	/* Convert internal types to external types */

	switch (type) {
	case ACPI_TYPE_LOCAL_REGION_FIELD:
	case ACPI_TYPE_LOCAL_BANK_FIELD:
	case ACPI_TYPE_LOCAL_INDEX_FIELD:

		type = ACPI_TYPE_FIELD_UNIT;
		break;

	case ACPI_TYPE_LOCAL_SCOPE:

		/* Per ACPI Specification, Scope is untyped */

		type = ACPI_TYPE_ANY;
		break;

	default:

		/* No change to Type required */

		break;
	}

	*return_type = type;
	if (return_desc) {
		*return_desc = obj_desc;
	}
	return_ACPI_STATUS(AE_OK);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      /******************************************************************************
 *
 * Module Name: exresop - AML Interpreter operand/object resolution
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
#include "amlcode.h"
#include "acparser.h"
#include "acinterp.h"
#include "acnamesp.h"

#define _COMPONENT          ACPI_EXECUTER
ACPI_MODULE_NAME("exresop")

/* Local prototypes */
static acpi_status
acpi_ex_check_object_type(acpi_object_type type_needed,
			  acpi_object_type this_type, void *object);

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_check_object_type
 *
 * PARAMETERS:  type_needed         Object type needed
 *              this_type           Actual object type
 *              Object              Object pointer
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Check required type against actual type
 *
 ******************************************************************************/

static acpi_status
acpi_ex_check_object_type(acpi_object_type type_needed,
			  acpi_object_type this_type, void *object)
{
	ACPI_FUNCTION_ENTRY();

	if (type_needed == ACPI_TYPE_ANY) {

		/* All types OK, so we don't perform any typechecks */

		return (AE_OK);
	}

	if (type_needed == ACPI_TYPE_LOCAL_REFERENCE) {
		/*
		 * Allow the AML "Constant" opcodes (Zero, One, etc.) to be reference
		 * objects and thus allow them to be targets. (As per the ACPI
		 * specification, a store to a constant is a noop.)
		 */
		if ((this_type == ACPI_TYPE_INTEGER) &&
		    (((union acpi_operand_object *)object)->common.
		     flags & AOPOBJ_AML_CONSTANT)) {
			return (AE_OK);
		}
	}

	if (type_needed != this_type) {
		ACPI_ERROR((AE_INFO,
			    "Needed type [%s], found [%s] %p",
			    acpi_ut_get_type_name(type_needed),
			    acpi_ut_get_type_name(this_type), object));

		return (AE_AML_OPERAND_TYPE);
	}

	return (AE_OK);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_resolve_operands
 *
 * PARAMETERS:  opcode              - Opcode being interpreted
 *              stack_ptr           - Pointer to the operand stack to be
 *                                    resolved
 *              walk_state          - Current state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Convert multiple input operands to the types required by the
 *              target operator.
 *
 *      Each 5-bit group in arg_types represents one required
 *      operand and indicates the required Type. The corresponding operand
 *      will be converted to the required type if possible, otherwise we
 *      abort with an exception.
 *
 ******************************************************************************/

acpi_status
acpi_ex_resolve_operands(u16 opcode,
			 union acpi_operand_object ** stack_ptr,
			 struct acpi_walk_state * walk_state)
{
	union acpi_operand_object *obj_desc;
	acpi_status status = AE_OK;
	u8 object_type;
	u32 arg_types;
	const struct acpi_opcode_info *op_info;
	u32 this_arg_type;
	acpi_object_type type_needed;
	u16 target_op = 0;

	ACPI_FUNCTION_TRACE_U32(ex_resolve_operands, opcode);

	op_info = acpi_ps_get_opcode_info(opcode);
	if (op_info->class == AML_CLASS_UNKNOWN) {
		return_ACPI_STATUS(AE_AML_BAD_OPCODE);
	}

	arg_types = op_info->runtime_args;
	if (arg_types == ARGI_INVALID_OPCODE) {
		ACPI_ERROR((AE_INFO, "Unknown AML opcode 0x%X", opcode));

		return_ACPI_STATUS(AE_AML_INTERNAL);
	}

	ACPI_DEBUG_PRINT((ACPI_DB_EXEC,
			  "Opcode %X [%s] RequiredOperandTypes=%8.8X\n",
			  opcode, op_info->name, arg_types));

	/*
	 * Normal exit is with (arg_types == 0) at end of argument list.
	 * Function will return an exception from within the loop upon
	 * finding an entry which is not (or cannot be converted
	 * to) the required type; if stack underflows; or upon
	 * finding a NULL stack entry (which should not happen).
	 */
	while (GET_CURRENT_ARG_TYPE(arg_types)) {
		if (!stack_ptr || !*stack_ptr) {
			ACPI_ERROR((AE_INFO, "Null stack entry at %p",
				    stack_ptr));

			return_ACPI_STATUS(AE_AML_INTERNAL);
		}

		/* Extract useful items */

		obj_desc = *stack_ptr;

		/* Decode the descriptor type */

		switch (ACPI_GET_DESCRIPTOR_TYPE(obj_desc)) {
		case ACPI_DESC_TYPE_NAMED:

			/* Namespace Node */

			object_type =
			    ((struct acpi_namespace_node *)obj_desc)->type;

			/*
			 * Resolve an alias object. The construction of these objects
			 * guarantees that there is only one level of alias indirection;
			 * thus, the attached object is always the aliased namespace node
			 */
			if (object_type == ACPI_TYPE_LOCAL_ALIAS) {
				obj_desc =
				    acpi_ns_get_attached_object((struct
								 acpi_namespace_node
								 *)obj_desc);
				*stack_ptr = obj_desc;
				object_type =
				    ((struct acpi_namespace_node *)obj_desc)->
				    type;
			}
			break;

		case ACPI_DESC_TYPE_OPERAND:

			/* ACPI internal object */

			object_type = obj_desc->common.type;

			/* Check for bad acpi_object_type */

			if (!acpi_ut_valid_object_type(object_type)) {
				ACPI_ERROR((AE_INFO,
					    "Bad operand object type [0x%X]",
					    object_type));

				return_ACPI_STATUS(AE_AML_OPERAND_TYPE);
			}

			if (object_type == (u8) ACPI_TYPE_LOCAL_REFERENCE) {

				/* Validate the Reference */

				switch (obj_desc->reference.class) {
				case ACPI_REFCLASS_DEBUG:

					target_op = AML_DEBUG_OP;

					/*lint -fallthrough */

				case ACPI_REFCLASS_ARG:
				case ACPI_REFCLASS_LOCAL:
				case ACPI_REFCLASS_INDEX:
				case ACPI_REFCLASS_REFOF:
				case ACPI_REFCLASS_TABLE:	/* ddb_handle from LOAD_OP or LOAD_TABLE_OP */
				case ACPI_REFCLASS_NAME:	/* Reference to a named object */

					ACPI_DEBUG_PRINT((ACPI_DB_EXEC,
							  "Operand is a Reference, Class [%s] %2.2X\n",
							  acpi_ut_get_reference_name
							  (obj_desc),
							  obj_desc->reference.
							  class));
					break;

				default:

					ACPI_ERROR((AE_INFO,
						    "Unknown Reference Class 0x%2.2X in %p",
						    obj_desc->reference.class,
						    obj_desc));

					return_ACPI_STATUS(AE_AML_OPERAND_TYPE);
				}
			}
			break;

		default:

			/* Invalid descriptor */

			ACPI_ERROR((AE_INFO, "Invalid descriptor %p [%s]",
				    obj_desc,
				    acpi_ut_get_descriptor_name(obj_desc)));

			return_ACPI_STATUS(AE_AML_OPERAND_TYPE);
		}

		/* Get one argument type, point to the next */

		this_arg_type = GET_CURRENT_ARG_TYPE(arg_types);
		INCREMENT_ARG_LIST(arg_types);

		/*
		 * Handle cases where the object does not need to be
		 * resolved to a value
		 */
		switch (this_arg_type) {
		case ARGI_REF_OR_STRING:	/* Can be a String or Reference */

			if ((ACPI_GET_DESCRIPTOR_TYPE(obj_desc) ==
			     ACPI_DESC_TYPE_OPERAND)
			    && (obj_desc->common.type == ACPI_TYPE_STRING)) {
				/*
				 * String found - the string references a named object and
				 * must be resolved to a node
				 */
				goto next_operand;
			}

			/*
			 * Else not a string - fall through to the normal Reference
			 * case below
			 */
			/*lint -fallthrough */

		case ARGI_REFERENCE:	/* References: */
		case ARGI_INTEGER_REF:
		case ARGI_OBJECT_REF:
		case ARGI_DEVICE_REF:
		case ARGI_TARGETREF:	/* Allows implicit conversion rules before store */
		case ARGI_FIXED_TARGET:	/* No implicit conversion before store to target */
		case ARGI_SIMPLE_TARGET:	/* Name, Local, or arg - no implicit conversion  */
		case ARGI_STORE_TARGET:

			/*
			 * Need an operand of type ACPI_TYPE_LOCAL_REFERENCE
			 * A Namespace Node is OK as-is
			 */
			if (ACPI_GET_DESCRIPTOR_TYPE(obj_desc) ==
			    ACPI_DESC_TYPE_NAMED) {
				goto next_operand;
			}

			status =
			    acpi_ex_check_object_type(ACPI_TYPE_LOCAL_REFERENCE,
						      object_type, obj_desc);
			if (ACPI_FAILURE(status)) {
				return_ACPI_STATUS(status);
			}
			goto next_operand;

		case ARGI_DATAREFOBJ:	/* Store operator only */
			/*
			 * We don't want to resolve index_op reference objects during
			 * a store because this would be an implicit de_ref_of operation.
			 * Instead, we just want to store the reference object.
			 * -- All others must be resolved below.
			 */
			if ((opcode == AML_STORE_OP) &&
			    ((*stack_ptr)->common.type ==
			     ACPI_TYPE_LOCAL_REFERENCE)
			    && ((*stack_ptr)->reference.class ==
				ACPI_REFCLASS_INDEX)) {
				goto next_operand;
			}
			break;

		default:

			/* All cases covered above */

			break;
		}

		/*
		 * Resolve this object to a value
		 */
		status = acpi_ex_resolve_to_value(stack_ptr, walk_state);
		if (ACPI_FAILURE(status)) {
			return_ACPI_STATUS(status);
		}

		/* Get the resolved object */

		obj_desc = *stack_ptr;

		/*
		 * Check the resulting object (value) type
		 */
		switch (this_arg_type) {
			/*
			 * For the simple cases, only one type of resolved object
			 * is allowed
			 */
		case ARGI_MUTEX:

			/* Need an operand of type ACPI_TYPE_MUTEX */

			type_needed = ACPI_TYPE_MUTEX;
			break;

		case ARGI_EVENT:

			/* Need an operand of type ACPI_TYPE_EVENT */

			type_needed = ACPI_TYPE_EVENT;
			break;

		case ARGI_PACKAGE:	/* Package */

			/* Need an operand of type ACPI_TYPE_PACKAGE */

			type_needed = ACPI_TYPE_PACKAGE;
			break;

		case ARGI_ANYTYPE:

			/* Any operand type will do */

			type_needed = ACPI_TYPE_ANY;
			break;

		case ARGI_DDBHANDLE:

			/* Need an operand of type ACPI_TYPE_DDB_HANDLE */

			type_needed = ACPI_TYPE_LOCAL_REFERENCE;
			break;

			/*
			 * The more complex cases allow multiple resolved object types
			 */
		case ARGI_INTEGER:

			/*
			 * Need an operand of type ACPI_TYPE_INTEGER,
			 * But we can implicitly convert from a STRING or BUFFER
			 * aka - "Implicit Source Operand Conversion"
			 */
			status =
			    acpi_ex_convert_to_integer(obj_desc, stack_ptr, 16);
			if (ACPI_FAILURE(status)) {
				if (status == AE_TYPE) {
					ACPI_ERROR((AE_INFO,
						    "Needed [Integer/String/Buffer], found [%s] %p",
						    acpi_ut_get_object_type_name
						    (obj_desc), obj_desc));

					return_ACPI_STATUS(AE_AML_OPERAND_TYPE);
				}

				return_ACPI_STATUS(status);
			}

			if (obj_desc != *stack_ptr) {
				acpi_ut_remove_reference(obj_desc);
			}
			goto next_operand;

		case ARGI_BUFFER:
			/*
			 * Need an operand of type ACPI_TYPE_BUFFER,
			 * But we can implicitly convert from a STRING or INTEGER
			 * aka - "Implicit Source Operand Conversion"
			 */
			status = acpi_ex_convert_to_buffer(obj_desc, stack_ptr);
			if (ACPI_FAILURE(status)) {
				if (status == AE_TYPE) {
					ACPI_ERROR((AE_INFO,
						    "Needed [Integer/String/Buffer], found [%s] %p",
						    acpi_ut_get_object_type_name
						    (obj_desc), obj_desc));

					return_ACPI_STATUS(AE_AML_OPERAND_TYPE);
				}

				return_ACPI_STATUS(status);
			}

			if (obj_desc != *stack_ptr) {
				acpi_ut_remove_reference(obj_desc);
			}
			goto next_operand;

		case ARGI_STRING:
			/*
			 * Need an operand of type ACPI_TYPE_STRING,
			 * But we can implicitly convert from a BUFFER or INTEGER
			 * aka - "Implicit Source Operand Conversion"
			 */
			status = acpi_ex_convert_to_string(obj_desc, stack_ptr,
							   ACPI_IMPLICIT_CONVERT_HEX);
			if (ACPI_FAILURE(status)) {
				if (status == AE_TYPE) {
					ACPI_ERROR((AE_INFO,
						    "Needed [Integer/String/Buffer], found [%s] %p",
						    acpi_ut_get_object_type_name
						    (obj_desc), obj_desc));

					return_ACPI_STATUS(AE_AML_OPERAND_TYPE);
				}

				return_ACPI_STATUS(status);
			}

			if (obj_desc != *stack_ptr) {
				acpi_ut_remove_reference(obj_desc);
			}
			goto next_operand;

		case ARGI_COMPUTEDATA:

			/* Need an operand of type INTEGER, STRING or BUFFER */

			switch (obj_desc->common.type) {
			case ACPI_TYPE_INTEGER:
			case ACPI_TYPE_STRING:
			case ACPI_TYPE_BUFFER:

				/* Valid operand */
				break;

			default:
				ACPI_ERROR((AE_INFO,
					    "Needed [Integer/String/Buffer], found [%s] %p",
					    acpi_ut_get_object_type_name
					    (obj_desc), obj_desc));

				return_ACPI_STATUS(AE_AML_OPERAND_TYPE);
			}
			goto next_operand;

		case ARGI_BUFFER_OR_STRING:

			/* Need an operand of type STRING or BUFFER */

			switch (obj_desc->common.type) {
			case ACPI_TYPE_STRING:
			case ACPI_TYPE_BUFFER:

				/* Valid operand */
				break;

			case ACPI_TYPE_INTEGER:

				/* Highest priority conversion is to type Buffer */

				status =
				    acpi_ex_convert_to_buffer(obj_desc,
							      stack_ptr);
				if (ACPI_FAILURE(status)) {
					return_ACPI_STATUS(status);
				}

				if (obj_desc != *stack_ptr) {
					acpi_ut_remove_reference(obj_desc);
				}
				break;

			default:
				ACPI_ERROR((AE_INFO,
					    "Needed [Integer/String/Buffer], found [%s] %p",
					    acpi_ut_get_object_type_name
					    (obj_desc), obj_desc));

				return_ACPI_STATUS(AE_AML_OPERAND_TYPE);
			}
			goto next_operand;

		case ARGI_DATAOBJECT:
			/*
			 * ARGI_DATAOBJECT is only used by the size_of operator.
			 * Need a buffer, string, package, or ref_of reference.
			 *
			 * The only reference allowed here is a direct reference to
			 * a namespace node.
			 */
			switch (obj_desc->common.type) {
			case ACPI_TYPE_PACKAGE:
			case ACPI_TYPE_STRING:
			case ACPI_TYPE_BUFFER:
			case ACPI_TYPE_LOCAL_REFERENCE:

				/* Valid operand */
				break;

			default:

				ACPI_ERROR((AE_INFO,
					    "Needed [Buffer/String/Package/Reference], found [%s] %p",
					    acpi_ut_get_object_type_name
					    (obj_desc), obj_desc));

				return_ACPI_STATUS(AE_AML_OPERAND_TYPE);
			}
			goto next_operand;

		case ARGI_COMPLEXOBJ:

			/* Need a buffer or package or (ACPI 2.0) String */

			switch (obj_desc->common.type) {
			case ACPI_TYPE_PACKAGE:
			case ACPI_TYPE_STRING:
			case ACPI_TYPE_BUFFER:

				/* Valid operand */
				break;

			default:

				ACPI_ERROR((AE_INFO,
					    "Needed [Buffer/String/Package], found [%s] %p",
					    acpi_ut_get_object_type_name
					    (obj_desc), obj_desc));

				return_ACPI_STATUS(AE_AML_OPERAND_TYPE);
			}
			goto next_operand;

		case ARGI_REGION_OR_BUFFER:	/* Used by Load() only */

			/* Need an operand of type REGION or a BUFFER (which could be a resolved region field) */

			switch (obj_desc->common.type) {
			case ACPI_TYPE_BUFFER:
			case ACPI_TYPE_REGION:

				/* Valid operand */
				break;

			default:

				ACPI_ERROR((AE_INFO,
					    "Needed [Region/Buffer], found [%s] %p",
					    acpi_ut_get_object_type_name
					    (obj_desc), obj_desc));

				return_ACPI_STATUS(AE_AML_OPERAND_TYPE);
			}
			goto next_operand;

		case ARGI_DATAREFOBJ:

			/* Used by the Store() operator only */

			switch (obj_desc->common.type) {
			case ACPI_TYPE_INTEGER:
			case ACPI_TYPE_PACKAGE:
			case ACPI_TYPE_STRING:
			case ACPI_TYPE_BUFFER:
			case ACPI_TYPE_BUFFER_FIELD:
			case ACPI_TYPE_LOCAL_REFERENCE:
			case ACPI_TYPE_LOCAL_REGION_FIELD:
			case ACPI_TYPE_LOCAL_BANK_FIELD:
			case ACPI_TYPE_LOCAL_INDEX_FIELD:
			case ACPI_TYPE_DDB_HANDLE:

				/* Valid operand */
				break;

			default:

				if (acpi_gbl_enable_interpreter_slack) {
					/*
					 * Enable original behavior of Store(), allowing any and all
					 * objects as the source operand. The ACPI spec does not
					 * allow this, however.
					 */
					break;
				}

				if (target_op == AML_DEBUG_OP) {

					/* Allow store of any object to the Debug object */

					break;
				}

				ACPI_ERROR((AE_INFO,
					    "Needed Integer/Buffer/String/Package/Ref/Ddb], found [%s] %p",
					    acpi_ut_get_object_type_name
					    (obj_desc), obj_desc));

				return_ACPI_STATUS(AE_AML_OPERAND_TYPE);
			}
			goto next_operand;

		default:

			/* Unknown type */

			ACPI_ERROR((AE_INFO,
				    "Internal - Unknown ARGI (required operand) type 0x%X",
				    this_arg_type));

			return_ACPI_STATUS(AE_BAD_PARAMETER);
		}

		/*
		 * Make sure that the original object was resolved to the
		 * required object type (Simple cases only).
		 */
		status = acpi_ex_check_object_type(type_needed,
						   (*stack_ptr)->common.type,
						   *stack_ptr);
		if (ACPI_FAILURE(status)) {
			return_ACPI_STATUS(status);
		}

next_operand:
		/*
		 * If more operands needed, decrement stack_ptr to point
		 * to next operand on stack
		 */
		if (GET_CURRENT_ARG_TYPE(arg_types)) {
			stack_ptr--;
		}
	}

	ACPI_DUMP_OPERANDS(walk_state->operands,
			   acpi_ps_get_opcode_name(opcode),
			   walk_state->num_operands);

	return_ACPI_STATUS(status);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      /******************************************************************************
 *
 * Module Name: exstore - AML Interpreter object store support
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
#include "acdispat.h"
#include "acinterp.h"
#include "amlcode.h"
#include "acnamesp.h"

#define _COMPONENT          ACPI_EXECUTER
ACPI_MODULE_NAME("exstore")

/* Local prototypes */
static acpi_status
acpi_ex_store_object_to_index(union acpi_operand_object *val_desc,
			      union acpi_operand_object *dest_desc,
			      struct acpi_walk_state *walk_state);

static acpi_status
acpi_ex_store_direct_to_node(union acpi_operand_object *source_desc,
			     struct acpi_namespace_node *node,
			     struct acpi_walk_state *walk_state);

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_store
 *
 * PARAMETERS:  *source_desc        - Value to be stored
 *              *dest_desc          - Where to store it. Must be an NS node
 *                                    or union acpi_operand_object of type
 *                                    Reference;
 *              walk_state          - Current walk state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Store the value described by source_desc into the location
 *              described by dest_desc. Called by various interpreter
 *              functions to store the result of an operation into
 *              the destination operand -- not just simply the actual "Store"
 *              ASL operator.
 *
 ******************************************************************************/

acpi_status
acpi_ex_store(union acpi_operand_object *source_desc,
	      union acpi_operand_object *dest_desc,
	      struct acpi_walk_state *walk_state)
{
	acpi_status status = AE_OK;
	union acpi_operand_object *ref_desc = dest_desc;

	ACPI_FUNCTION_TRACE_PTR(ex_store, dest_desc);

	/* Validate parameters */

	if (!source_desc || !dest_desc) {
		ACPI_ERROR((AE_INFO, "Null parameter"));
		return_ACPI_STATUS(AE_AML_NO_OPERAND);
	}

	/* dest_desc can be either a namespace node or an ACPI object */

	if (ACPI_GET_DESCRIPTOR_TYPE(dest_desc) == ACPI_DESC_TYPE_NAMED) {
		/*
		 * Dest is a namespace node,
		 * Storing an object into a Named node.
		 */
		status = acpi_ex_store_object_to_node(source_desc,
						      (struct
						       acpi_namespace_node *)
						      dest_desc, walk_state,
						      ACPI_IMPLICIT_CONVERSION);

		return_ACPI_STATUS(status);
	}

	/* Destination object must be a Reference or a Constant object */

	switch (dest_desc->common.type) {
	case ACPI_TYPE_LOCAL_REFERENCE:

		break;

	case ACPI_TYPE_INTEGER:

		/* Allow stores to Constants -- a Noop as per ACPI spec */

		if (dest_desc->common.flags & AOPOBJ_AML_CONSTANT) {
			return_ACPI_STATUS(AE_OK);
		}

		/*lint -fallthrough */

	default:

		/* Destination is not a Reference object */

		ACPI_ERROR((AE_INFO,
			    "Target is not a Reference or Constant object - [%s] %p",
			    acpi_ut_get_object_type_name(dest_desc),
			    dest_desc));

		return_ACPI_STATUS(AE_AML_OPERAND_TYPE);
	}

	/*
	 * Examine the Reference class. These cases are handled:
	 *
	 * 1) Store to Name (Change the object associated with a name)
	 * 2) Store to an indexed area of a Buffer or Package
	 * 3) Store to a Method Local or Arg
	 * 4) Store to the debug object
	 */
	switch (ref_desc->reference.class) {
	case ACPI_REFCLASS_REFOF:

		/* Storing an object into a Name "container" */

		status = acpi_ex_store_object_to_node(source_desc,
						      ref_desc->reference.
						      object, walk_state,
						      ACPI_IMPLICIT_CONVERSION);
		break;

	case ACPI_REFCLASS_INDEX:

		/* Storing to an Index (pointer into a packager or buffer) */

		status =
		    acpi_ex_store_object_to_index(source_desc, ref_desc,
						  walk_state);
		break;

	case ACPI_REFCLASS_LOCAL:
	case ACPI_REFCLASS_ARG:

		/* Store to a method local/arg  */

		status =
		    acpi_ds_store_object_to_local(ref_desc->reference.class,
						  ref_desc->reference.value,
						  source_desc, walk_state);
		break;

	case ACPI_REFCLASS_DEBUG:
		/*
		 * Storing to the Debug object causes the value stored to be
		 * displayed and otherwise has no effect -- see ACPI Specification
		 */
		ACPI_DEBUG_PRINT((ACPI_DB_EXEC,
				  "**** Write to Debug Object: Object %p [%s] ****:\n\n",
				  source_desc,
				  acpi_ut_get_object_type_name(source_desc)));

		ACPI_DEBUG_OBJECT(source_desc, 0, 0);
		break;

	default:

		ACPI_ERROR((AE_INFO, "Unknown Reference Class 0x%2.2X",
			    ref_desc->reference.class));
		ACPI_DUMP_ENTRY(ref_desc, ACPI_LV_INFO);

		status = AE_AML_INTERNAL;
		break;
	}

	return_ACPI_STATUS(status);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ex_store_object_to_index
 *
 * PARAMETERS:  *source_desc            - Value to be stored
 *              *dest_desc              - Named object to receive the value
 *              walk_state              - Current walk state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Store the object to indexed Buffer or Package element
 *
 ******************************************************************************/

static acpi_status
acpi_ex_store_object_to_index(union acpi_operand_object *source_desc,
			      union acpi_operand_object *index_desc,
			      struct acpi_walk_state *walk_state)
{
	acpi_status status = AE_OK;
	union acpi_operand_object *obj_desc;
	union acpi_operand_object *new_desc;
	u8 value = 0;
	u32 i;

	ACPI_FUNCTION_TRACE(ex_store_object_to_index);

	/*
	 * Destination must be a reference pointer, and
	 * must point to either a buffer or a package
	 */
	switch (index_desc->reference.target_type) {
	case ACPI_TYPE_PACKAGE:
		/*
		 * Storing to a package element. Copy the object and replace
		 * any existing object with the new object. No implicit
		 * conversion is performed.
		 *
		 * The object at *(index_desc->Reference.Where) is the
		 * element within the package that is to be modified.
		 * The parent package object is at index_desc->Reference.Object
		 */
		obj_desc = *(index_desc->reference.where);

		if (source_desc->common.type == ACPI_TYPE_LOCAL_REFERENCE &&
		    source_desc->reference.class == ACPI_REFCLASS_TABLE) {

			/* This is a DDBHandle, just add a reference to it */

			acpi_ut_add_reference(source_desc);
			new_desc = source_desc;
		} else {
			/* Normal object, copy it */

			status =
			    acpi_ut_copy_iobject_to_iobject(source_desc,
							    &new_desc,
							    walk_state);
			if (ACPI_FAILURE(status)) {
				return_ACPI_STATUS(status);
			}
		}

		if (obj_desc) {

			/* Decrement reference count by the ref count of the parent package */

			for (i = 0; i < ((union acpi_operand_object *)
					 index_desc->reference.object)->common.
			     reference_count; i++) {
				acpi_ut_remove_reference(obj_desc);
			}
		}

		*(index_desc->reference.where) = new_desc;

		/* Increment ref count by the ref count of the parent package-1 */

		for (i = 1; i < ((union acpi_operand_object *)
				 index_desc->reference.object)->common.
		     reference_count; i++) {
			acpi_ut_add_reference(new_desc);
		}

		break;

	case ACPI_TYPE_BUFFER_FIELD:
		/*
		 * Store into a Buffer or String (not actually a real buffer_field)
		 * at a location defined by an Index.
		 *
		 * The first 8-bit element of the source object is written to the
		 * 8-bit Buffer location defined by the Index destination object,
		 * according to the ACPI 2.0 specification.
		 */

		/*
		 * Make sure the target is a Buffer or String. An error should
		 * not happen here, since the reference_object was constructed
		 * by the INDEX_OP code.
		 */
		obj_desc = index_desc->reference.object;
		if ((obj_desc->common.type != ACPI_TYPE_BUFFER) &&
		    (obj_desc->common.type != ACPI_TYPE_STRING)) {
			return_ACPI_STATUS(AE_AML_OPERAND_TYPE);
		}

		/*
		 * The assignment of the individual elements will be slightly
		 * different for each source type.
		 */
		switch (source_desc->common.type) {
		case ACPI_TYPE_INTEGER:

			/* Use the least-significant byte of the integer */

			value = (u8) (source_desc->integer.value);
			break;

		case ACPI_TYPE_BUFFER:
		case ACPI_TYPE_STRING:

			/* Note: Takes advanta