***************
 *
 * FUNCTION:    acpi_ns_get_bitmapped_type
 *
 * PARAMETERS:  return_object   - Object returned from method/obj evaluation
 *
 * RETURN:      Object return type. ACPI_RTYPE_ANY indicates that the object
 *              type is not supported. ACPI_RTYPE_NONE indicates that no
 *              object was returned (return_object is NULL).
 *
 * DESCRIPTION: Convert object type into a bitmapped object return type.
 *
 ******************************************************************************/

static u32 acpi_ns_get_bitmapped_type(union acpi_operand_object *return_object)
{
	u32 return_btype;

	if (!return_object) {
		return (ACPI_RTYPE_NONE);
	}

	/* Map acpi_object_type to internal bitmapped type */

	switch (return_object->common.type) {
	case ACPI_TYPE_INTEGER:

		return_btype = ACPI_RTYPE_INTEGER;
		break;

	case ACPI_TYPE_BUFFER:

		return_btype = ACPI_RTYPE_BUFFER;
		break;

	case ACPI_TYPE_STRING:

		return_btype = ACPI_RTYPE_STRING;
		break;

	case ACPI_TYPE_PACKAGE:

		return_btype = ACPI_RTYPE_PACKAGE;
		break;

	case ACPI_TYPE_LOCAL_REFERENCE:

		return_btype = ACPI_RTYPE_REFERENCE;
		break;

	default:

		/* Not one of the supported objects, must be incorrect */

		return_btype = ACPI_RTYPE_ANY;
		break;
	}

	return (return_btype);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            /******************************************************************************
 *
 * Module Name: nsprepkg - Validation of package objects for predefined names
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
#include "acpredef.h"

#define _COMPONENT          ACPI_NAMESPACE
ACPI_MODULE_NAME("nsprepkg")

/* Local prototypes */
static acpi_status
acpi_ns_check_package_list(struct acpi_evaluate_info *info,
			   const union acpi_predefined_info *package,
			   union acpi_operand_object **elements, u32 count);

static acpi_status
acpi_ns_check_package_elements(struct acpi_evaluate_info *info,
			       union acpi_operand_object **elements,
			       u8 type1,
			       u32 count1,
			       u8 type2, u32 count2, u32 start_index);

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_check_package
 *
 * PARAMETERS:  info                - Method execution information block
 *              return_object_ptr   - Pointer to the object returned from the
 *                                    evaluation of a method or object
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Check a returned package object for the correct count and
 *              correct type of all sub-objects.
 *
 ******************************************************************************/

acpi_status
acpi_ns_check_package(struct acpi_evaluate_info *info,
		      union acpi_operand_object **return_object_ptr)
{
	union acpi_operand_object *return_object = *return_object_ptr;
	const union acpi_predefined_info *package;
	union acpi_operand_object **elements;
	acpi_status status = AE_OK;
	u32 expected_count;
	u32 count;
	u32 i;

	ACPI_FUNCTION_NAME(ns_check_package);

	/* The package info for this name is in the next table entry */

	package = info->predefined + 1;

	ACPI_DEBUG_PRINT((ACPI_DB_NAMES,
			  "%s Validating return Package of Type %X, Count %X\n",
			  info->full_pathname, package->ret_info.type,
			  return_object->package.count));

	/*
	 * For variable-length Packages, we can safely remove all embedded
	 * and trailing NULL package elements
	 */
	acpi_ns_remove_null_elements(info, package->ret_info.type,
				     return_object);

	/* Extract package count and elements array */

	elements = return_object->package.elements;
	count = return_object->package.count;

	/*
	 * Most packages must have at least one element. The only exception
	 * is the variable-length package (ACPI_PTYPE1_VAR).
	 */
	if (!count) {
		if (package->ret_info.type == ACPI_PTYPE1_VAR) {
			return (AE_OK);
		}

		ACPI_WARN_PREDEFINED((AE_INFO, info->full_pathname,
				      info->node_flags,
				      "Return Package has no elements (empty)"));

		return (AE_AML_OPERAND_VALUE);
	}

	/*
	 * Decode the type of the expected package contents
	 *
	 * PTYPE1 packages contain no subpackages
	 * PTYPE2 packages contain subpackages
	 */
	switch (package->ret_info.type) {
	case ACPI_PTYPE1_FIXED:
		/*
		 * The package count is fixed and there are no subpackages
		 *
		 * If package is too small, exit.
		 * If package is larger than expected, issue warning but continue
		 */
		expected_count =
		    package->ret_info.count1 + package->ret_info.count2;
		if (count < expected_count) {
			goto package_too_small;
		} else if (count > expected_count) {
			ACPI_DEBUG_PRINT((ACPI_DB_REPAIR,
					  "%s: Return Package is larger than needed - "
					  "found %u, expected %u\n",
					  info->full_pathname, count,
					  expected_count));
		}

		/* Validate all elements of the returned package */

		status = acpi_ns_check_package_elements(info, elements,
							package->ret_info.
							object_type1,
							package->ret_info.
							count1,
							package->ret_info.
							object_type2,
							package->ret_info.
							count2, 0);
		break;

	case ACPI_PTYPE1_VAR:
		/*
		 * The package count is variable, there are no subpackages, and all
		 * elements must be of the same type
		 */
		for (i = 0; i < count; i++) {
			status = acpi_ns_check_object_type(info, elements,
							   package->ret_info.
							   object_type1, i);
			if (ACPI_FAILURE(status)) {
				return (status);
			}
			elements++;
		}
		break;

	case ACPI_PTYPE1_OPTION:
		/*
		 * The package count is variable, there are no subpackages. There are
		 * a fixed number of required elements, and a variable number of
		 * optional elements.
		 *
		 * Check if package is at least as large as the minimum required
		 */
		expected_count = package->ret_info3.count;
		if (count < expected_count) {
			goto package_too_small;
		}

		/* Variable number of sub-objects */

		for (i = 0; i < count; i++) {
			if (i < package->ret_info3.count) {

				/* These are the required package elements (0, 1, or 2) */

				status =
				    acpi_ns_check_object_type(info, elements,
							      package->
							      ret_info3.
							      object_type[i],
							      i);
				if (ACPI_FAILURE(status)) {
					return (status);
				}
			} else {
				/* These are the optional package elements */

				status =
				    acpi_ns_check_object_type(info, elements,
							      package->
							      ret_info3.
							      tail_object_type,
							      i);
				if (ACPI_FAILURE(status)) {
					return (status);
				}
			}
			elements++;
		}
		break;

	case ACPI_PTYPE2_REV_FIXED:

		/* First element is the (Integer) revision */

		status = acpi_ns_check_object_type(info, elements,
						   ACPI_RTYPE_INTEGER, 0);
		if (ACPI_FAILURE(status)) {
			return (status);
		}

		elements++;
		count--;

		/* Examine the subpackages */

		status =
		    acpi_ns_check_package_list(info, package, elements, count);
		break;

	case ACPI_PTYPE2_PKG_COUNT:

		/* First element is the (Integer) count of subpackages to follow */

		status = acpi_ns_check_object_type(info, elements,
						   ACPI_RTYPE_INTEGER, 0);
		if (ACPI_FAILURE(status)) {
			return (status);
		}

		/*
		 * Count cannot be larger than the parent package length, but allow it
		 * to be smaller. The >= accounts for the Integer above.
		 */
		expected_count = (u32)(*elements)->integer.value;
		if (expected_count >= count) {
			goto package_too_small;
		}

		count = expected_count;
		elements++;

		/* Examine the subpackages */

		status =
		    acpi_ns_check_package_list(info, package, elements, count);
		break;

	case ACPI_PTYPE2:
	case ACPI_PTYPE2_FIXED:
	case ACPI_PTYPE2_MIN:
	case ACPI_PTYPE2_COUNT:
	case ACPI_PTYPE2_FIX_VAR:
		/*
		 * These types all return a single Package that consists of a
		 * variable number of subpackages.
		 *
		 * First, ensure that the first element is a subpackage. If not,
		 * the BIOS may have incorrectly returned the object as a single
		 * package instead of a Package of Packages (a common error if
		 * there is only one entry). We may be able to repair this by
		 * wrapping the returned Package with a new outer Package.
		 */
		if (*elements
		    && ((*elements)->common.type != ACPI_TYPE_PACKAGE)) {

			/* Create the new outer package and populate it */

			status =
			    acpi_ns_wrap_with_package(info, return_object,
						      return_object_ptr);
			if (ACPI_FAILURE(status)) {
				return (status);
			}

			/* Update locals to point to the new package (of 1 element) */

			return_object = *return_object_ptr;
			elements = return_object->package.elements;
			count = 1;
		}

		/* Examine the subpackages */

		status =
		    acpi_ns_check_package_list(info, package, elements, count);
		break;

	case ACPI_PTYPE2_VAR_VAR:
		/*
		 * Returns a variable list of packages, each with a variable list
		 * of objects.
		 */
		break;

	case ACPI_PTYPE2_UUID_PAIR:

		/* The package must contain pairs of (UUID + type) */

		if (count & 1) {
			expected_count = count + 1;
			goto package_too_small;
		}

		while (count > 0) {
			status = acpi_ns_check_object_type(info, elements,
							   package->ret_info.
							   object_type1, 0);
			if (ACPI_FAILURE(status)) {
				return (status);
			}

			/* Validate length of the UUID buffer */

			if ((*elements)->buffer.length != 16) {
				ACPI_WARN_PREDEFINED((AE_INFO,
						      info->full_pathname,
						      info->node_flags,
						      "Invalid length for UUID Buffer"));
				return (AE_AML_OPERAND_VALUE);
			}

			status = acpi_ns_check_object_type(info, elements + 1,
							   package->ret_info.
							   object_type2, 0);
			if (ACPI_FAILURE(status)) {
				return (status);
			}

			elements += 2;
			count -= 2;
		}
		break;

	default:

		/* Should not get here if predefined info table is correct */

		ACPI_WARN_PREDEFINED((AE_INFO, info->full_pathname,
				      info->node_flags,
				      "Invalid internal return type in table entry: %X",
				      package->ret_info.type));

		return (AE_AML_INTERNAL);
	}

	return (status);

package_too_small:

	/* Error exit for the case with an incorrect package count */

	ACPI_WARN_PREDEFINED((AE_INFO, info->full_pathname, info->node_flags,
			      "Return Package is too small - found %u elements, expected %u",
			      count, expected_count));

	return (AE_AML_OPERAND_VALUE);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_check_package_list
 *
 * PARAMETERS:  info            - Method execution information block
 *              package         - Pointer to package-specific info for method
 *              elements        - Element list of parent package. All elements
 *                                of this list should be of type Package.
 *              count           - Count of subpackages
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Examine a list of subpackages
 *
 ******************************************************************************/

static acpi_status
acpi_ns_check_package_list(struct acpi_evaluate_info *info,
			   const union acpi_predefined_info *package,
			   union acpi_operand_object **elements, u32 count)
{
	union acpi_operand_object *sub_package;
	union acpi_operand_object **sub_elements;
	acpi_status status;
	u32 expected_count;
	u32 i;
	u32 j;

	/*
	 * Validate each subpackage in the parent Package
	 *
	 * NOTE: assumes list of subpackages contains no NULL elements.
	 * Any NULL elements should have been removed by earlier call
	 * to acpi_ns_remove_null_elements.
	 */
	for (i = 0; i < count; i++) {
		sub_package = *elements;
		sub_elements = sub_package->package.elements;
		info->parent_package = sub_package;

		/* Each sub-object must be of type Package */

		status = acpi_ns_check_object_type(info, &sub_package,
						   ACPI_RTYPE_PACKAGE, i);
		if (ACPI_FAILURE(status)) {
			return (status);
		}

		/* Examine the different types of expected subpackages */

		info->parent_package = sub_package;
		switch (package->ret_info.type) {
		case ACPI_PTYPE2:
		case ACPI_PTYPE2_PKG_COUNT:
		case ACPI_PTYPE2_REV_FIXED:

			/* Each subpackage has a fixed number of elements */

			expected_count =
			    package->ret_info.count1 + package->ret_info.count2;
			if (sub_package->package.count < expected_count) {
				goto package_too_small;
			}

			status =
			    acpi_ns_check_package_elements(info, sub_elements,
							   package->ret_info.
							   object_type1,
							   package->ret_info.
							   count1,
							   package->ret_info.
							   object_type2,
							   package->ret_info.
							   count2, 0);
			if (ACPI_FAILURE(status)) {
				return (status);
			}
			break;

		case ACPI_PTYPE2_FIX_VAR:
			/*
			 * Each subpackage has a fixed number of elements and an
			 * optional element
			 */
			expected_count =
			    package->ret_info.count1 + package->ret_info.count2;
			if (sub_package->package.count < expected_count) {
				goto package_too_small;
			}

			status =
			    acpi_ns_check_package_elements(info, sub_elements,
							   package->ret_info.
							   object_type1,
							   package->ret_info.
							   count1,
							   package->ret_info.
							   object_type2,
							   sub_package->package.
							   count -
							   package->ret_info.
							   count1, 0);
			if (ACPI_FAILURE(status)) {
				return (status);
			}
			break;

		case ACPI_PTYPE2_VAR_VAR:
			/*
			 * Each subpackage has a fixed or variable number of elements
			 */
			break;

		case ACPI_PTYPE2_FIXED:

			/* Each subpackage has a fixed length */

			expected_count = package->ret_info2.count;
			if (sub_package->package.count < expected_count) {
				goto package_too_small;
			}

			/* Check the type of each subpackage element */

			for (j = 0; j < expected_count; j++) {
				status =
				    acpi_ns_check_object_type(info,
							      &sub_elements[j],
							      package->
							      ret_info2.
							      object_type[j],
							      j);
				if (ACPI_FAILURE(status)) {
					return (status);
				}
			}
			break;

		case ACPI_PTYPE2_MIN:

			/* Each subpackage has a variable but minimum length */

			expected_count = package->ret_info.count1;
			if (sub_package->package.count < expected_count) {
				goto package_too_small;
			}

			/* Check the type of each subpackage element */

			status =
			    acpi_ns_check_package_elements(info, sub_elements,
							   package->ret_info.
							   object_type1,
							   sub_package->package.
							   count, 0, 0, 0);
			if (ACPI_FAILURE(status)) {
				return (status);
			}
			break;

		case ACPI_PTYPE2_COUNT:
			/*
			 * First element is the (Integer) count of elements, including
			 * the count field (the ACPI name is num_elements)
			 */
			status = acpi_ns_check_object_type(info, sub_elements,
							   ACPI_RTYPE_INTEGER,
							   0);
			if (ACPI_FAILURE(status)) {
				return (status);
			}

			/*
			 * Make sure package is large enough for the Count and is
			 * is as large as the minimum size
			 */
			expected_count = (u32)(*sub_elements)->integer.value;
			if (sub_package->package.count < expected_count) {
				goto package_too_small;
			}
			if (sub_package->package.count <
			    package->ret_info.count1) {
				expected_count = package->ret_info.count1;
				goto package_too_small;
			}
			if (expected_count == 0) {
				/*
				 * Either the num_entries element was originally zero or it was
				 * a NULL element and repaired to an Integer of value zero.
				 * In either case, repair it by setting num_entries to be the
				 * actual size of the subpackage.
				 */
				expected_count = sub_package->package.count;
				(*sub_elements)->integer.value = expected_count;
			}

			/* Check the type of each subpackage element */

			status =
			    acpi_ns_check_package_elements(info,
							   (sub_elements + 1),
							   package->ret_info.
							   object_type1,
							   (expected_count - 1),
							   0, 0, 1);
			if (ACPI_FAILURE(status)) {
				return (status);
			}
			break;

		default:	/* Should not get here, type was validated by caller */

			return (AE_AML_INTERNAL);
		}

		elements++;
	}

	return (AE_OK);

package_too_small:

	/* The subpackage count was smaller than required */

	ACPI_WARN_PREDEFINED((AE_INFO, info->full_pathname, info->node_flags,
			      "Return SubPackage[%u] is too small - found %u elements, expected %u",
			      i, sub_package->package.count, expected_count));

	return (AE_AML_OPERAND_VALUE);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_check_package_elements
 *
 * PARAMETERS:  info            - Method execution information block
 *              elements        - Pointer to the package elements array
 *              type1           - Object type for first group
 *              count1          - Count for first group
 *              type2           - Object type for second group
 *              count2          - Count for second group
 *              start_index     - Start of the first group of elements
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Check that all elements of a package are of the correct object
 *              type. Supports up to two groups of different object types.
 *
 ******************************************************************************/

static acpi_status
acpi_ns_check_package_elements(struct acpi_evaluate_info *info,
			       union acpi_operand_object **elements,
			       u8 type1,
			       u32 count1,
			       u8 type2, u32 count2, u32 start_index)
{
	union acpi_operand_object **this_element = elements;
	acpi_status status;
	u32 i;

	/*
	 * Up to two groups of package elements are supported by the data
	 * structure. All elements in each group must be of the same type.
	 * The second group can have a count of zero.
	 */
	for (i = 0; i < count1; i++) {
		status = acpi_ns_check_object_type(info, this_element,
						   type1, i + start_index);
		if (ACPI_FAILURE(status)) {
			return (status);
		}
		this_element++;
	}

	for (i = 0; i < count2; i++) {
		status = acpi_ns_check_object_type(info, this_element,
						   type2,
						   (i + count1 + start_index));
		if (ACPI_FAILURE(status)) {
			return (status);
		}
		this_element++;
	}

	return (AE_OK);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             /******************************************************************************
 *
 * Module Name: nsrepair - Repair for objects returned by predefined methods
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
#include "acinterp.h"
#include "acpredef.h"
#include "amlresrc.h"

#define _COMPONENT          ACPI_NAMESPACE
ACPI_MODULE_NAME("nsrepair")

/*******************************************************************************
 *
 * This module attempts to repair or convert objects returned by the
 * predefined methods to an object type that is expected, as per the ACPI
 * specification. The need for this code is dictated by the many machines that
 * return incorrect types for the standard predefined methods. Performing these
 * conversions here, in one place, eliminates the need for individual ACPI
 * device drivers to do the same. Note: Most of these conversions are different
 * than the internal object conversion routines used for implicit object
 * conversion.
 *
 * The following conversions can be performed as necessary:
 *
 * Integer -> String
 * Integer -> Buffer
 * String  -> Integer
 * String  -> Buffer
 * Buffer  -> Integer
 * Buffer  -> String
 * Buffer  -> Package of Integers
 * Package -> Package of one Package
 *
 * Additional conversions that are available:
 *  Convert a null return or zero return value to an end_tag descriptor
 *  Convert an ASCII string to a Unicode buffer
 *
 * An incorrect standalone object is wrapped with required outer package
 *
 * Additional possible repairs:
 * Required package elements that are NULL replaced by Integer/String/Buffer
 *
 ******************************************************************************/
/* Local prototypes */
static const struct acpi_simple_repair_info *acpi_ns_match_simple_repair(struct
									 acpi_namespace_node
									 *node,
									 u32
									 return_btype,
									 u32
									 package_index);

/*
 * Special but simple repairs for some names.
 *
 * 2nd argument: Unexpected types that can be repaired
 */
static const struct acpi_simple_repair_info acpi_object_repair_info[] = {
	/* Resource descriptor conversions */

	{"_CRS",
	 ACPI_RTYPE_INTEGER | ACPI_RTYPE_STRING | ACPI_RTYPE_BUFFER |
	 ACPI_RTYPE_NONE,
	 ACPI_NOT_PACKAGE_ELEMENT,
	 acpi_ns_convert_to_resource},
	{"_DMA",
	 ACPI_RTYPE_INTEGER | ACPI_RTYPE_STRING | ACPI_RTYPE_BUFFER |
	 ACPI_RTYPE_NONE,
	 ACPI_NOT_PACKAGE_ELEMENT,
	 acpi_ns_convert_to_resource},
	{"_PRS",
	 ACPI_RTYPE_INTEGER | ACPI_RTYPE_STRING | ACPI_RTYPE_BUFFER |
	 ACPI_RTYPE_NONE,
	 ACPI_NOT_PACKAGE_ELEMENT,
	 acpi_ns_convert_to_resource},

	/* Unicode conversions */

	{"_MLS", ACPI_RTYPE_STRING, 1,
	 acpi_ns_convert_to_unicode},
	{"_STR", ACPI_RTYPE_STRING | ACPI_RTYPE_BUFFER,
	 ACPI_NOT_PACKAGE_ELEMENT,
	 acpi_ns_convert_to_unicode},
	{{0, 0, 0, 0}, 0, 0, NULL}	/* Table terminator */
};

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_simple_repair
 *
 * PARAMETERS:  info                - Method execution information block
 *              expected_btypes     - Object types expected
 *              package_index       - Index of object within parent package (if
 *                                    applicable - ACPI_NOT_PACKAGE_ELEMENT
 *                                    otherwise)
 *              return_object_ptr   - Pointer to the object returned from the
 *                                    evaluation of a method or object
 *
 * RETURN:      Status. AE_OK if repair was successful.
 *
 * DESCRIPTION: Attempt to repair/convert a return object of a type that was
 *              not expected.
 *
 ******************************************************************************/

acpi_status
acpi_ns_simple_repair(struct acpi_evaluate_info *info,
		      u32 expected_btypes,
		      u32 package_index,
		      union acpi_operand_object **return_object_ptr)
{
	union acpi_operand_object *return_object = *return_object_ptr;
	union acpi_operand_object *new_object = NULL;
	acpi_status status;
	const struct acpi_simple_repair_info *predefined;

	ACPI_FUNCTION_NAME(ns_simple_repair);

	/*
	 * Special repairs for certain names that are in the repair table.
	 * Check if this name is in the list of repairable names.
	 */
	predefined = acpi_ns_match_simple_repair(info->node,
						 info->return_btype,
						 package_index);
	if (predefined) {
		if (!return_object) {
			ACPI_WARN_PREDEFINED((AE_INFO, info->full_pathname,
					      ACPI_WARN_ALWAYS,
					      "Missing expected return value"));
		}

		status =
		    predefined->object_converter(return_object, &new_object);
		if (ACPI_FAILURE(status)) {

			/* A fatal error occurred during a conversion */

			ACPI_EXCEPTION((AE_INFO, status,
					"During return object analysis"));
			return (status);
		}
		if (new_object) {
			goto object_repaired;
		}
	}

	/*
	 * Do not perform simple object repair unless the return type is not
	 * expected.
	 */
	if (info->return_btype & expected_btypes) {
		return (AE_OK);
	}

	/*
	 * At this point, we know that the type of the returned object was not
	 * one of the expected types for this predefined name. Attempt to
	 * repair the object by converting it to one of the expected object
	 * types for this predefined name.
	 */

	/*
	 * If there is no return value, check if we require a return value for
	 * this predefined name. Either one return value is expected, or none,
	 * for both methods and other objects.
	 *
	 * Try to fix if there was no return object. Warning if failed to fix.
	 */
	if (!return_object) {
		if (expected_btypes) {
			if (!(expected_btypes & ACPI_RTYPE_NONE) &&
			    package_index != ACPI_NOT_PACKAGE_ELEMENT) {
				ACPI_WARN_PREDEFINED((AE_INFO,
						      info->full_pathname,
						      ACPI_WARN_ALWAYS,
						      "Found unexpected NULL package element"));

				status =
				    acpi_ns_repair_null_element(info,
								expected_btypes,
								package_index,
								return_object_ptr);
				if (ACPI_SUCCESS(status)) {
					return (AE_OK);	/* Repair was successful */
				}
			}

			if (expected_btypes != ACPI_RTYPE_NONE) {
				ACPI_WARN_PREDEFINED((AE_INFO,
						      info->full_pathname,
						      ACPI_WARN_ALWAYS,
						      "Missing expected return value"));
				return (AE_AML_NO_RETURN_VALUE);
			}
		}
	}

	if (expected_btypes & ACPI_RTYPE_INTEGER) {
		status = acpi_ns_convert_to_integer(return_object, &new_object);
		if (ACPI_SUCCESS(status)) {
			goto object_repaired;
		}
	}
	if (expected_btypes & ACPI_RTYPE_STRING) {
		status = acpi_ns_convert_to_string(return_object, &new_object);
		if (ACPI_SUCCESS(status)) {
			goto object_repaired;
		}
	}
	if (expected_btypes & ACPI_RTYPE_BUFFER) {
		status = acpi_ns_convert_to_buffer(return_object, &new_object);
		if (ACPI_SUCCESS(status)) {
			goto object_repaired;
		}
	}
	if (expected_btypes & ACPI_RTYPE_PACKAGE) {
		/*
		 * A package is expected. We will wrap the existing object with a
		 * new package object. It is often the case that if a variable-length
		 * package is required, but there is only a single object needed, the
		 * BIOS will return that object instead of wrapping it with a Package
		 * object. Note: after the wrapping, the package will be validated
		 * for correct contents (expected object type or types).
		 */
		status =
		    acpi_ns_wrap_with_package(info, return_object, &new_object);
		if (ACPI_SUCCESS(status)) {
			/*
			 * The original object just had its reference count
			 * incremented for being inserted into the new package.
			 */
			*return_object_ptr = new_object;	/* New Package object */
			info->return_flags |= ACPI_OBJECT_REPAIRED;
			return (AE_OK);
		}
	}

	/* We cannot repair this object */

	return (AE_AML_OPERAND_TYPE);

object_repaired:

	/* Object was successfully repaired */

	if (package_index != ACPI_NOT_PACKAGE_ELEMENT) {
		/*
		 * The original object is a package element. We need to
		 * decrement the reference count of the original object,
		 * for removing it from the package.
		 *
		 * However, if the original object was just wrapped with a
		 * package object as part of the repair, we don't need to
		 * change the reference count.
		 */
		if (!(info->return_flags & ACPI_OBJECT_WRAPPED)) {
			new_object->common.reference_count =
			    return_object->common.reference_count;

			if (return_object->common.reference_count > 1) {
				return_object->common.reference_count--;
			}
		}

		ACPI_DEBUG_PRINT((ACPI_DB_REPAIR,
				  "%s: Converted %s to expected %s at Package index %u\n",
				  info->full_pathname,
				  acpi_ut_get_object_type_name(return_object),
				  acpi_ut_get_object_type_name(new_object),
				  package_index));
	} else {
		ACPI_DEBUG_PRINT((ACPI_DB_REPAIR,
				  "%s: Converted %s to expected %s\n",
				  info->full_pathname,
				  acpi_ut_get_object_type_name(return_object),
				  acpi_ut_get_object_type_name(new_object)));
	}

	/* Delete old object, install the new return object */

	acpi_ut_remove_reference(return_object);
	*return_object_ptr = new_object;
	info->return_flags |= ACPI_OBJECT_REPAIRED;
	return (AE_OK);
}

/******************************************************************************
 *
 * FUNCTION:    acpi_ns_match_simple_repair
 *
 * PARAMETERS:  node                - Namespace node for the method/object
 *              return_btype        - Object type that was returned
 *              package_index       - Index of object within parent package (if
 *                                    applicable - ACPI_NOT_PACKAGE_ELEMENT
 *                                    otherwise)
 *
 * RETURN:      Pointer to entry in repair table. NULL indicates not found.
 *
 * DESCRIPTION: Check an object name against the repairable object list.
 *
 *****************************************************************************/

static const struct acpi_simple_repair_info *acpi_ns_match_simple_repair(struct
									 acpi_namespace_node
									 *node,
									 u32
									 return_btype,
									 u32
									 package_index)
{
	const struct acpi_simple_repair_info *this_name;

	/* Search info table for a repairable predefined method/object name */

	this_name = acpi_object_repair_info;
	while (this_name->object_converter) {
		if (ACPI_COMPARE_NAME(node->name.ascii, this_name->name)) {

			/* Check if we can actually repair this name/type combination */

			if ((return_btype & this_name->unexpected_btypes) &&
			    (package_index == this_name->package_index)) {
				return (this_name);
			}

			return (NULL);
		}
		this_name++;
	}

	return (NULL);		/* Name was not found in the repair table */
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_repair_null_element
 *
 * PARAMETERS:  info                - Method execution information block
 *              expected_btypes     - Object types expected
 *              package_index       - Index of object within parent package (if
 *                                    applicable - ACPI_NOT_PACKAGE_ELEMENT
 *                                    otherwise)
 *              return_object_ptr   - Pointer to the object returned from the
 *                                    evaluation of a method or object
 *
 * RETURN:      Status. AE_OK if repair was successful.
 *
 * DESCRIPTION: Attempt to repair a NULL element of a returned Package object.
 *
 ******************************************************************************/

acpi_status
acpi_ns_repair_null_element(struct acpi_evaluate_info * info,
			    u32 expected_btypes,
			    u32 package_index,
			    union acpi_operand_object **return_object_ptr)
{
	union acpi_operand_object *return_object = *return_object_ptr;
	union acpi_operand_object *new_object;

	ACPI_FUNCTION_NAME(ns_repair_null_element);

	/* No repair needed if return object is non-NULL */

	if (return_object) {
		return (AE_OK);
	}

	/*
	 * Attempt to repair a NULL element of a Package object. This applies to
	 * predefined names that return a fixed-length package and each element
	 * is required. It does not apply to variable-length packages where NULL
	 * elements are allowed, especially at the end of the package.
	 */
	if (expected_btypes & ACPI_RTYPE_INTEGER) {

		/* Need an integer - create a zero-value integer */

		new_object = acpi_ut_create_integer_object((u64)0);
	} else if (expected_btypes & ACPI_RTYPE_STRING) {

		/* Need a string - create a NULL string */

		new_object = acpi_ut_create_string_object(0);
	} else if (expected_btypes & ACPI_RTYPE_BUFFER) {

		/* Need a buffer - create a zero-length buffer */

		new_object = acpi_ut_create_buffer_object(0);
	} else {
		/* Error for all other expected types */

		return (AE_AML_OPERAND_TYPE);
	}

	if (!new_object) {
		return (AE_NO_MEMORY);
	}

	/* Set the reference count according to the parent Package object */

	new_object->common.reference_count =
	    info->parent_package->common.reference_count;

	ACPI_DEBUG_PRINT((ACPI_DB_REPAIR,
			  "%s: Converted NULL package element to expected %s at index %u\n",
			  info->full_pathname,
			  acpi_ut_get_object_type_name(new_object),
			  package_index));

	*return_object_ptr = new_object;
	info->return_flags |= ACPI_OBJECT_REPAIRED;
	return (AE_OK);
}

/******************************************************************************
 *
 * FUNCTION:    acpi_ns_remove_null_elements
 *
 * PARAMETERS:  info                - Method execution information block
 *              package_type        - An acpi_return_package_types value
 *              obj_desc            - A Package object
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Remove all NULL package elements from packages that contain
 *              a variable number of subpackages. For these types of
 *              packages, NULL elements can be safely removed.
 *
 *****************************************************************************/

void
acpi_ns_remove_null_elements(struct acpi_evaluate_info *info,
			     u8 package_type,
			     union acpi_operand_object *obj_desc)
{
	union acpi_operand_object **source;
	union acpi_operand_object **dest;
	u32 count;
	u32 new_count;
	u32 i;

	ACPI_FUNCTION_NAME(ns_remove_null_elements);

	/*
	 * We can safely remove all NULL elements from these package types:
	 * PTYPE1_VAR packages contain a variable number of simple data types.
	 * PTYPE2 packages contain a variable number of subpackages.
	 */
	switch (package_type) {
	case ACPI_PTYPE1_VAR:
	case ACPI_PTYPE2:
	case ACPI_PTYPE2_COUNT:
	case ACPI_PTYPE2_PKG_COUNT:
	case ACPI_PTYPE2_FIXED:
	case ACPI_PTYPE2_MIN:
	case ACPI_PTYPE2_REV_FIXED:
	case ACPI_PTYPE2_FIX_VAR:
		break;

	default:
	case ACPI_PTYPE2_VAR_VAR:
	case ACPI_PTYPE1_FIXED:
	case ACPI_PTYPE1_OPTION:
		return;
	}

	count = obj_desc->package.count;
	new_count = count;

	source = obj_desc->package.elements;
	dest = source;

	/* Examine all elements of the package object, remove nulls */

	for (i = 0; i < count; i++) {
		if (!*source) {
			new_count--;
		} else {
			*dest = *source;
			dest++;
		}
		source++;
	}

	/* Update parent package if any null elements were removed */

	if (new_count < count) {
		ACPI_DEBUG_PRINT((ACPI_DB_REPAIR,
				  "%s: Found and removed %u NULL elements\n",
				  info->full_pathname, (count - new_count)));

		/* NULL terminate list and update the package count */

		*dest = NULL;
		obj_desc->package.count = new_count;
	}
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_wrap_with_package
 *
 * PARAMETERS:  info                - Method execution information block
 *              original_object     - Pointer to the object to repair.
 *              obj_desc_ptr        - The new package object is returned here
 *
 * RETURN:      Status, new object in *obj_desc_ptr
 *
 * DESCRIPTION: Repair a common problem with objects that are defined to
 *              return a variable-length Package of sub-objects. If there is
 *              only one sub-object, some BIOS code mistakenly simply declares
 *              the single object instead of a Package with one sub-object.
 *              This function attempts to repair this error by wrapping a
 *              Package object around the original object, creating the
 *              correct and expected Package with one sub-object.
 *
 *              Names that can be repaired in this manner include:
 *              _ALR, _CSD, _HPX, _MLS, _PLD, _PRT, _PSS, _TRT, _TSS,
 *              _BCL, _DOD, _FIX, _Sx
 *
 ******************************************************************************/

acpi_status
acpi_ns_wrap_with_package(struct acpi_evaluate_info *info,
			  union acpi_operand_object *original_object,
			  union acpi_operand_object **obj_desc_ptr)
{
	union acpi_operand_object *pkg_obj_desc;

	ACPI_FUNCTION_NAME(ns_wrap_with_package);

	/*
	 * Create the new outer package and populate it. The new package will
	 * have a single element, the lone sub-object.
	 */
	pkg_obj_desc = acpi_ut_create_package_object(1);
	if (!pkg_obj_desc) {
		return (AE_NO_MEMORY);
	}

	pkg_obj_desc->package.elements[0] = original_object;

	ACPI_DEBUG_PRINT((ACPI_DB_REPAIR,
			  "%s: Wrapped %s with expected Package object\n",
			  info->full_pathname,
			  acpi_ut_get_object_type_name(original_object)));

	/* Return the new object in the object pointer */

	*obj_desc_ptr = pkg_obj_desc;
	info->return_flags |= ACPI_OBJECT_REPAIRED | ACPI_OBJECT_WRAPPED;
	return (AE_OK);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              /******************************************************************************
 *
 * Module Name: nsrepair2 - Repair for objects returned by specific
 *                          predefined methods
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
ACPI_MODULE_NAME("nsrepair2")

/*
 * Information structure and handler for ACPI predefined names that can
 * be repaired on a per-name basis.
 */
typedef
acpi_status(*acpi_repair_function) (struct acpi_evaluate_info * info,
				    union acpi_operand_object
				    **return_object_ptr);

typedef struct acpi_repair_info {
	char name[ACPI_NAME_SIZE];
	acpi_repair_function repair_function;

} acpi_repair_info;

/* Local prototypes */

static const struct acpi_repair_info *acpi_ns_match_complex_repair(struct
								   acpi_namespace_node
								   *node);

static acpi_status
acpi_ns_repair_ALR(struct acpi_evaluate_info *info,
		   union acpi_operand_object **return_object_ptr);

static acpi_status
acpi_ns_repair_CID(struct acpi_evaluate_info *info,
		   union acpi_operand_object **return_object_ptr);

static acpi_status
acpi_ns_repair_CST(struct acpi_evaluate_info *info,
		   union acpi_operand_object **return_object_ptr);

static acpi_status
acpi_ns_repair_FDE(struct acpi_evaluate_info *info,
		   union acpi_operand_object **return_object_ptr);

static acpi_status
acpi_ns_repair_HID(struct acpi_evaluate_info *info,
		   union acpi_operand_object **return_object_ptr);

static acpi_status
acpi_ns_repair_PRT(struct acpi_evaluate_info *info,
		   union acpi_operand_object **return_object_ptr);

static acpi_status
acpi_ns_repair_PSS(struct acpi_evaluate_info *info,
		   union acpi_operand_object **return_object_ptr);

static acpi_status
acpi_ns_repair_TSS(struct acpi_evaluate_info *info,
		   union acpi_operand_object **return_object_ptr);

static acpi_status
acpi_ns_check_sorted_list(struct acpi_evaluate_info *info,
			  union acpi_operand_object *return_object,
			  u32 start_index,
			  u32 expected_count,
			  u32 sort_index,
			  u8 sort_direction, char *sort_key_name);

/* Values for sort_direction above */

#define ACPI_SORT_ASCENDING     0
#define ACPI_SORT_DESCENDING    1

static void
acpi_ns_remove_element(union acpi_operand_object *obj_desc, u32 index);

static void
acpi_ns_sort_list(union acpi_operand_object **elements,
		  u32 count, u32 index, u8 sort_direction);

/*
 * This table contains the names of the predefined methods for which we can
 * perform more complex repairs.
 *
 * As necessary:
 *
 * _ALR: Sort the list ascending by ambient_illuminance
 * _CID: Strings: uppercase all, remove any leading asterisk
 * _CST: Sort the list ascending by C state type
 * _FDE: Convert Buffer of BYTEs to a Buffer of DWORDs
 * _GTM: Convert Buffer of BYTEs to a Buffer of DWORDs
 * _HID: Strings: uppercase all, remove any leading asterisk
 * _PRT: Fix reversed source_name and source_index
 * _PSS: Sort the list descending by Power
 * _TSS: Sort the list descending by Power
 *
 * Names that must be packages, but cannot be sorted:
 *
 * _BCL: Values are tied to the Package index where they appear, and cannot
 * be moved or sorted. These index values are used for _BQC and _BCM.
 * However, we can fix the case where a buffer is returned, by converting
 * it to a Package of integers.
 */
static const struct acpi_repair_info acpi_ns_repairable_names[] = {
	{"_ALR", acpi_ns_repair_ALR},
	{"_CID", acpi_ns_repair_CID},
	{"_CST", acpi_ns_repair_CST},
	{"_FDE", acpi_ns_repair_FDE},
	{"_GTM", acpi_ns_repair_FDE},	/* _GTM has same repair as _FDE */
	{"_HID", acpi_ns_repair_HID},
	{"_PRT", acpi_ns_repair_PRT},
	{"_PSS", acpi_ns_repair_PSS},
	{"_TSS", acpi_ns_repair_TSS},
	{{0, 0, 0, 0}, NULL}	/* Table terminator */
};

#define ACPI_FDE_FIELD_COUNT        5
#define ACPI_FDE_BYTE_BUFFER_SIZE   5
#define ACPI_FDE_DWORD_BUFFER_SIZE  (ACPI_FDE_FIELD_COUNT * sizeof (u32))

/******************************************************************************
 *
 * FUNCTION:    acpi_ns_complex_repairs
 *
 * PARAMETERS:  info                - Method execution information block
 *              node                - Namespace node for the method/object
 *              validate_status     - Original status of earlier validation
 *              return_object_ptr   - Pointer to the object returned from the
 *                                    evaluation of a method or object
 *
 * RETURN:      Status. AE_OK if repair was successful. If name is not
 *              matched, validate_status is returned.
 *
 * DESCRIPTION: Attempt to repair/convert a return object of a type that was
 *              not expected.
 *
 *****************************************************************************/

acpi_status
acpi_ns_complex_repairs(struct acpi_evaluate_info *info,
			struct acpi_namespace_node *node,
			acpi_status validate_status,
			union acpi_operand_object **return_object_ptr)
{
	const struct acpi_repair_info *predefined;
	acpi_status status;

	/* Check if this name is in the list of repairable names */

	predefined = acpi_ns_match_complex_repair(node);
	if (!predefined) {
		return (validate_status);
	}

	status = predefined->repair_function(info, return_object_ptr);
	return (status);
}

/******************************************************************************
 *
 * FUNCTION:    acpi_ns_match_complex_repair
 *
 * PARAMETERS:  node                - Namespace node for the method/object
 *
 * RETURN:      Pointer to entry in repair table. NULL indicates not found.
 *
 * DESCRIPTION: Check an object name against the repairable object list.
 *
 *****************************************************************************/

static const struct acpi_repair_info *acpi_ns_match_complex_repair(struct
								   acpi_namespace_node
								   *node)
{
	const struct acpi_repair_info *this_name;

	/* Search info table for a repairable predefined method/object name */

	this_name = acpi_ns_repairable_names;
	while (this_name->repair_function) {
		if (ACPI_COMPARE_NAME(node->name.ascii, this_name->name)) {
			return (this_name);
		}
		this_name++;
	}

	return (NULL);		/* Not found */
}

/******************************************************************************
 *
 * FUNCTION:    acpi_ns_repair_ALR
 *
 * PARAMETERS:  info                - Method execution information block
 *              return_object_ptr   - Pointer to the object returned from the
 *                                    evaluation of a method or object
 *
 * RETURN:      Status. AE_OK if object is OK or was repaired successfully
 *
 * DESCRIPTION: Repair for the _ALR object. If necessary, sort the object list
 *              ascending by the ambient illuminance values.
 *
 *****************************************************************************/

static acpi_status
acpi_ns_repair_ALR(struct acpi_evaluate_info *info,
		   union acpi_operand_object **return_object_ptr)
{
	union acpi_operand_object *return_object = *return_object_ptr;
	acpi_status status;

	status = acpi_ns_check_sorted_list(info, return_object, 0, 2, 1,
					   ACPI_SORT_ASCENDING,
					   "AmbientIlluminance");

	return (status);
}

/******************************************************************************
 *
 * FUNCTION:    acpi_ns_repair_FDE
 *
 * PARAMETERS:  info                - Method execution information block
 *              return_object_ptr   - Pointer to the object returned from the
 *                                    evaluation of a method or object
 *
 * RETURN:      Status. AE_OK if object is OK or was repaired successfully
 *
 * DESCRIPTION: Repair for the _FDE and _GTM objects. The expected return
 *              value is a Buffer of 5 DWORDs. This function repairs a common
 *              problem where the return value is a Buffer of BYTEs, not
 *              DWORDs.
 *
 *****************************************************************************/

static acpi_status
acpi_ns_repair_FDE(struct acpi_evaluate_info *info,
		   union acpi_operand_object **return_object_ptr)
{
	union acpi_operand_object *return_object = *return_object_ptr;
	union acpi_operand_object *buffer_object;
	u8 *byte_buffer;
	u32 *dword_buffer;
	u32 i;

	ACPI_FUNCTION_NAME(ns_repair_FDE);

	switch (return_object->common.type) {
	case ACPI_TYPE_BUFFER:

		/* This is the expected type. Length should be (at least) 5 DWORDs */

		if (return_object->buffer.length >= ACPI_FDE_DWORD_BUFFER_SIZE) {
			return (AE_OK);
		}

		/* We can only repair if we have exactly 5 BYTEs */

		if (return_object->buffer.length != ACPI_FDE_BYTE_BUFFER_SIZE) {
			ACPI_WARN_PREDEFINED((AE_INFO, info->full_pathname,
					      info->node_flags,
					      "Incorrect return buffer length %u, expected %u",
					      return_object->buffer.length,
					      ACPI_FDE_DWORD_BUFFER_SIZE));

			return (AE_AML_OPERAND_TYPE);
		}

		/* Create the new (larger) buffer object */

		buffer_object =
		    acpi_ut_create_buffer_object(ACPI_FDE_DWORD_BUFFER_SIZE);
		if (!buffer_object) {
			return (AE_NO_MEMORY);
		}

		/* Expand each byte to a DWORD */

		byte_buffer = return_object->buffer.pointer;
		dword_buffer =
		    ACPI_CAST_PTR(u32, buffer_object->buffer.pointer);

		for (i = 0; i < ACPI_FDE_FIELD_COUNT; i++) {
			*dword_buffer = (u32) *byte_buffer;
			dword_buffer++;
			byte_buffer++;
		}

		ACPI_DEBUG_PRINT((ACPI_DB_REPAIR,
				  "%s Expanded Byte Buffer to expected DWord Buffer\n",
				  info->full_pathname));
		break;

	default:

		return (AE_AML_OPERAND_TYPE);
	}

	/* Delete the original return object, return the new buffer object */

	acpi_ut_remove_reference(return_object);
	*return_object_ptr = buffer_object;

	info->return_flags |= ACPI_OBJECT_REPAIRED;
	return (AE_OK);
}

/******************************************************************************
 *
 * FUNCTION:    acpi_ns_repair_CID
 *
 * PARAMETERS:  info                - Method execution information block
 *              return_object_ptr   - Pointer to the object returned from the
 *                                    evaluation of a method or object
 *
 * RETURN:      Status. AE_OK if object is OK or was repaired successfully
 *
 * DESCRIPTION: Repair for the _CID object. If a string, ensure that all
 *              letters are uppercase and that there is no leading asterisk.
 *              If a Package, ensure same for all string elements.
 *
 *****************************************************************************/

static acpi_status
acpi_ns_repair_CID(struct acpi_evaluate_info *info,
		   union acpi_operand_object **return_object_ptr)
{
	acpi_status status;
	union acpi_operand_object *return_object = *return_object_ptr;
	union acpi_operand_object **element_ptr;
	union acpi_operand_object *original_element;
	u16 original_ref_count;
	u32 i;

	/* Check for _CID as a simple string */

	if (return_object->common.type == ACPI_TYPE_STRING) {
		status = acpi_ns_repair_HID(info, return_object_ptr);
		return (status);
	}

	/* Exit if not a Package */

	if (return_object->common.type != ACPI_TYPE_PACKAGE) {
		return (AE_OK);
	}

	/* Examine each element of the _CID package */

	element_ptr = return_object->package.elements;
	for (i = 0; i < return_object->package.count; i++) {
		original_element = *element_ptr;
		original_ref_count = original_element->common.reference_count;

		status = acpi_ns_repair_HID(info, element_ptr);
		if (ACPI_FAILURE(status)) {
			return (status);
		}

		/* Take care with reference counts */

		if (original_element != *element_ptr) {

			/* Element was replaced */

			(*element_ptr)->common.reference_count =
			    original_ref_count;

			acpi_ut_remove_reference(original_element);
		}

		element_ptr++;
	}

	return (AE_OK);
}

/******************************************************************************
 *
 * FUNCTION:    acpi_ns_repair_CST
 *
 * PARAMETERS:  info                - Method execution information block
 *              return_object_ptr   - Pointer to the object returned from the
 *                                    evaluation of a method or object
 *
 * RETURN:      Status. AE_OK if object is OK or was repaired successfully
 *
 * DESCRIPTION: Repair for the _CST object:
 *              1. Sort the list ascending by C state type
 *              2. Ensure type cannot be zero
 *              3. A subpackage count of zero means _CST is meaningless
 *              4. Count must match the number of C state subpackages
 *
 *****************************************************************************/

static acpi_status
acpi_ns_repair_CST(struct acpi_evaluate_info *info,
		   union acpi_operand_object **return_object_ptr)
{
	union acpi_operand_object *return_object = *return_object_ptr;
	union acpi_operand_object **outer_elements;
	u32 outer_element_count;
	union acpi_operand_object *obj_desc;
	acpi_status status;
	u8 removing;
	u32 i;

	ACPI_FUNCTION_NAME(ns_repair_CST);

	/*
	 * Check if the C-state type values are proportional.
	 */
	outer_element_count = return_object->package.count - 1;
	i = 0;
	while (i < outer_element_count) {
		outer_elements = &return_object->package.elements[i + 1];
		removing = FALSE;

		if ((*outer_elements)->package.count == 0) {
			ACPI_WARN_PREDEFINED((AE_INFO, info->full_pathname,
					      info->node_flags,
					      "SubPackage[%u] - removing entry due to zero count",
					      i));
			removing = TRUE;
			goto remove_element;
		}

		obj_desc = (*outer_elements)->package.elements[1];	/* Index1 = Type */
		if ((u32)obj_desc->integer.value == 0) {
			ACPI_WARN_PREDEFINED((AE_INFO, info->full_pathname,
					      info->node_flags,
					      "SubPackage[%u] - removing entry due to invalid Type(0)",
					      i));
			removing = TRUE;
		}

remove_element:
		if (removing) {
			acpi_ns_remove_element(return_object, i + 1);
			outer_element_count--;
		} else {
			i++;
		}
	}

	/* Update top-level package count, Type "Integer" checked elsewhere */

	obj_desc = return_object->package.elements[0];
	obj_desc->integer.value = outer_element_count;

	/*
	 * Entries (subpackages) in the _CST Package must be sorted by the
	 * C-state type, in ascending order.
	 */
	status = acpi_ns_check_sorted_list(info, return_object, 1, 4, 1,
					   ACPI_SORT_ASCENDING, "C-State Type");
	if (ACPI_FAILURE(status)) {
		return (status);
	}

	return (AE_OK);
}

/******************************************************************************
 *
 * FUNCTION:    acpi_ns_repair_HID
 *
 * PARAMETERS:  info                - Method execution information block
 *              return_object_ptr   - Pointer to the object returned from the
 *                                    evaluation of a method or object
 *
 * RETURN:      Status. AE_OK if object is OK or was repaired successfully
 *
 * DESCRIPTION: Repair for the _HID object. If a string, ensure that all
 *              letters are uppercase and that there is no leading asterisk.
 *
 *****************************************************************************/

static acpi_status
acpi_ns_repair_HID(struct acpi_evaluate_info *info,
		   union acpi_operand_object **return_object_ptr)
{
	union acpi_operand_object *return_object = *return_object_ptr;
	union acpi_operand_object *new_string;
	char *source;
	char *dest;

	ACPI_FUNCTION_NAME(ns_repair_HID);

	/* We only care about string _HID objects (not integers) */

	if (return_object->common.type != ACPI_TYPE_STRING) {
		return (AE_OK);
	}

	if (return_object->string.length == 0) {
		ACPI_WARN_PREDEFINED((AE_INFO, info->full_pathname,
				      info->node_flags,
				      "Invalid zero-length _HID or _CID string"));

		/* Return AE_OK anyway, let driver handle it */

		info->return_flags |= ACPI_OBJECT_REPAIRED;
		return (AE_OK);
	}

	/* It is simplest to always create a new string object */

	new_string = acpi_ut_create_string_object(return_object->string.length);
	if (!new_string) {
		return (AE_NO_MEMORY);
	}

	/*
	 * Remove a leading asterisk if present. For some unknown reason, there
	 * are many machines in the field that contains IDs like this.
	 *
	 * Examples: "*PNP0C03", "*ACPI0003"
	 */
	source = return_object->string.pointer;
	if (*source == '*') {
		source++;
		new_string->string.length--;

		ACPI_DEBUG_PRINT((ACPI_DB_REPAIR,
				  "%s: Removed invalid leading asterisk\n",
				  info->full_pathname));
	}

	/*
	 * Copy and uppercase the string. From the ACPI 5.0 specification:
	 *
	 * A valid PNP ID must be of the form "AAA####" where A is an uppercase
	 * letter and # is a hex digit. A valid ACPI ID must be of the form
	 * "NNNN####" where N is an uppercase letter or decimal digit, and
	 * # is a hex digit.
	 */
	for (dest = new_string->string.pointer; *source; dest++, source++) {
		*dest = (char)toupper((int)*source);
	}

	acpi_ut_remove_reference(return_object);
	*return_object_ptr = new_string;
	return (AE_OK);
}

/******************************************************************************
 *
 * FUNCTION:    acpi_ns_repair_PRT
 *
 * PARAMETERS:  info                - Method execution information block
 *              return_object_ptr   - Pointer to the object returned from the
 *                                    evaluation of a method or object
 *
 * RETURN:      Status. AE_OK if object is OK or was repaired successfully
 *
 * DESCRIPTION: Repair for the _PRT object. If necessary, fix reversed
 *              source_name and source_index field, a common BIOS bug.
 *
 *****************************************************************************/

static acpi_status
acpi_ns_repair_PRT(struct acpi_evaluate_info *info,
		   union acpi_operand_object **return_object_ptr)
{
	union acpi_operand_object *package_object = *return_object_ptr;
	union acpi_operand_object **top_object_list;
	union acpi_operand_object **sub_object_list;
	union acpi_operand_object *obj_desc;
	union acpi_operand_object *sub_package;
	u32 element_count;
	u32 index;

	/* Each element in the _PRT package is a subpackage */

	top_object_list = package_object->package.elements;
	element_count = package_object->package.count;

	/* Examine each subpackage */

	for (index = 0; index < element_count; index++, top_object_list++) {
		sub_package = *top_object_list;
		sub_object_list = sub_package->package.elements;

		/* Check for minimum required element count */

		if (sub_package->package.count < 4) {
			continue;
		}

		/*
		 * If the BIOS has erroneously reversed the _PRT source_name (index 2)
		 * and the source_index (index 3), fix it. _PRT is important enough to
		 * workaround this BIOS error. This also provides compatibility with
		 * other ACPI implementations.
		 */
		obj_desc = sub_object_list[3];
		if (!obj_desc || (obj_desc->common.type != ACPI_TYPE_INTEGER)) {
			sub_object_list[3] = sub_object_list[2];
			sub_object_list[2] = obj_desc;
			info->return_flags |= ACPI_OBJECT_REPAIRED;

			ACPI_WARN_PREDEFINED((AE_INFO,
					      info->full_pathname,
					      info->node_flags,
					      "PRT[%X]: Fixed reversed SourceName and SourceIndex",
					      index));
		}
	}

	return (AE_OK);
}

/******************************************************************************
 *
 * FUNCTION:    acpi_ns_repair_PSS
 *
 * PARAMETERS:  info                - Method execution information block
 *              return_object_ptr   - Pointer to the object returned from the
 *                                    evaluation of a method or object
 *
 * RETURN:      Status. AE_OK if object is OK or was repaired successfully
 *
 * DESCRIPTION: Repair for the _PSS object. If necessary, sort the object list
 *              by the CPU frequencies. Check that the power dissipation values
 *              are all proportional to CPU frequency (i.e., sorting by
 *              frequency should be the same as sorting by power.)
 *
 *****************************************************************************/

static acpi_status
acpi_ns_repair_PSS(struct acpi_evaluate_info *info,
		   union acpi_operand_object **return_object_ptr)
{
	union acpi_operand_object *return_object = *return_object_ptr;
	union acpi_operand_object **outer_elements;
	u32 outer_element_count;
	union acpi_operand_object **elements;
	union acpi_operand_object *obj_desc;
	u32 previous_value;
	acpi_status status;
	u32 i;

	/*
	 * Entries (subpackages) in the _PSS Package must be sorted by power
	 * dissipation, in descending order. If it appears that the list is
	 * incorrectly sorted, sort it. We sort by cpu_frequency, since this
	 * should be proportional to the power.
	 */
	status = acpi_ns_check_sorted_list(info, return_object, 0, 6, 0,
					   ACPI_SORT_DESCENDING,
					   "CpuFrequency");
	if (ACPI_FAILURE(status)) {
		return (status);
	}

	/*
	 * We now know the list is correctly sorted by CPU frequency. Check if
	 * the power dissipation values are proportional.
	 */
	previous_value = ACPI_UINT32_MAX;
	outer_elements = return_object->package.elements;
	outer_element_count = return_object->package.count;

	for (i = 0; i < outer_element_count; i++) {
		elements = (*outer_elements)->package.elements;
		obj_desc = elements[1];	/* Index1 = power_dissipation */

		if ((u32) obj_desc->integer.value > previous_value) {
			ACPI_WARN_PREDEFINED((AE_INFO, info->full_pathname,
					      info->node_flags,
					      "SubPackage[%u,%u] - suspicious power dissipation values",
					      i - 1, i));
		}

		previous_value = (u32) obj_desc->integer.value;
		outer_elements++;
	}

	return (AE_OK);
}

/******************************************************************************
 *
 * FUNCTION:    acpi_ns_repair_TSS
 *
 * PARAMETERS:  info                - Method execution information block
 *              return_object_ptr   - Pointer to the object returned from the
 *                                    evaluation of a method or object
 *
 * RETURN:      Status. AE_OK if object is OK or was repaired successfully
 *
 * DESCRIPTION: Repair for the _TSS object. If necessary, sort the object list
 *              descending by the power dissipation values.
 *
 *****************************************************************************/

static acpi_status
acpi_ns_repair_TSS(struct acpi_evaluate_info *info,
		   union acpi_operand_object **return_object_ptr)
{
	union acpi_operand_object *return_object = *return_object_ptr;
	acpi_status status;
	struct acpi_namespace_node *node;

	/*
	 * We can only sort the _TSS return package if there is no _PSS in the
	 * same scope. This is because if _PSS is present, the ACPI specification
	 * dictates that the _TSS Power Dissipation field is to be ignored, and
	 * therefore some BIOSs leave garbage values in the _TSS Power field(s).
	 * In this case, it is best to just return the _TSS package as-is.
	 * (May, 2011)
	 */
	status = acpi_ns_get_node(info->node, "^_PSS",
				  ACPI_NS_NO_UPSEARCH, &node);
	if (ACPI_SUCCESS(status)) {
		return (AE_OK);
	}

	status = acpi_ns_check_sorted_list(info, return_object, 0, 5, 1,
					   ACPI_SORT_DESCENDING,
					   "PowerDissipation");

	return (status);
}

/******************************************************************************
 *
 * FUNCTION:    acpi_ns_check_sorted_list
 *
 * PARAMETERS:  info                - Method execution information block
 *              return_object       - Pointer to the top-level returned object
 *              start_index         - Index of the first subpackage
 *              expected_count      - Minimum length of each subpackage
 *              sort_index          - Subpackage entry to sort on
 *              sort_direction      - Ascending or descending
 *              sort_key_name       - Name of the sort_index field
 *
 * RETURN:      Status. AE_OK if the list is valid and is sorted correctly or
 *              has been repaired by sorting the list.
 *
 * DESCRIPTION: Check if the package list is valid and sorted correctly by the
 *              sort_index. If not, then sort the list.
 *
 *****************************************************************************/

static acpi_status
acpi_ns_check_sorted_list(struct acpi_evaluate_info *info,
			  union acpi_operand_object *return_object,
			  u32 start_index,
			  u32 expected_count,
			  u32 sort_index,
			  u8 sort_direction, char *sort_key_name)
{
	u32 outer_element_count;
	union acpi_operand_object **outer_elements;
	union acpi_operand_object **elements;
	union acpi_operand_object *obj_desc;
	u32 i;
	u32 previous_value;

	ACPI_FUNCTION_NAME(ns_check_sorted_list);

	/* The top-level object must be a package */

	if (return_object->common.type != ACPI_TYPE_PACKAGE) {
		return (AE_AML_OPERAND_TYPE);
	}

	/*
	 * NOTE: assumes list of subpackages contains no NULL elements.
	 * Any NULL elements should have been removed by earlier call
	 * to acpi_ns_remove_null_elements.
	 */
	outer_element_count = return_object->package.count;
	if (!outer_element_count || start_index >= outer_element_count) {
		return (AE_AML_PACKAGE_LIMIT);
	}

	outer_elements = &return_object->package.elements[start_index];
	outer_element_count -= start_index;

	previous_value = 0;
	if (sort_direction == ACPI_SORT_DESCENDING) {
		previous_value = ACPI_UINT32_MAX;
	}

	/* Examine each subpackage */

	for (i = 0; i < outer_element_count; i++) {

		/* Each element of the top-level package must also be a package */

		if ((*outer_elements)->common.type != ACPI_TYPE_PACKAGE) {
			return (AE_AML_OPERAND_TYPE);
		}

		/* Each subpackage must have the minimum length */

		if ((*outer_elements)->package.count < expected_count) {
			return (AE_AML_PACKAGE_LIMIT);
		}

		elements = (*outer_elements)->package.elements;
		obj_desc = elements[sort_index];

		if (obj_desc->common.type != ACPI_TYPE_INTEGER) {
			return (AE_AML_OPERAND_TYPE);
		}

		/*
		 * The list must be sorted in the specified order. If we detect a
		 * discrepancy, sort the entire list.
		 */
		if (((sort_direction == ACPI_SORT_ASCENDING) &&
		     (obj_desc->integer.value < previous_value)) ||
		    ((sort_direction == ACPI_SORT_DESCENDING) &&
		     (obj_desc->integer.value > previous_value))) {
			acpi_ns_sort_list(&return_object->package.
					  elements[start_index],
					  outer_element_count, sort_index,
					  sort_direction);

			info->return_flags |= ACPI_OBJECT_REPAIRED;

			ACPI_DEBUG_PRINT((ACPI_DB_REPAIR,
					  "%s: Repaired unsorted list - now sorted by %s\n",
					  info->full_pathname, sort_key_name));
			return (AE_OK);
		}

		previous_value = (u32) obj_desc->integer.value;
		outer_elements++;
	}

	return (AE_OK);
}

/******************************************************************************
 *
 * FUNCTION:    acpi_ns_sort_list
 *
 * PARAMETERS:  elements            - Package object element list
 *              count               - Element count for above
 *              index               - Sort by which package element
 *              sort_direction      - Ascending or Descending sort
 *
 * RETURN:      None
 *
 * DESCRIPTION: Sort the objects that are in a package element list.
 *
 * NOTE: Assumes that all NULL elements have been removed from the package,
 *       and that all elements have been verified to be of type Integer.
 *
 *****************************************************************************/

static void
acpi_ns_sort_list(union acpi_operand_object **elements,
		  u32 count, u32 index, u8 sort_direction)
{
	union acpi_operand_object *obj_desc1;
	union acpi_operand_object *obj_desc2;
	union acpi_operand_object *temp_obj;
	u32 i;
	u32 j;

	/* Simple bubble sort */

	for (i = 1; i < count; i++) {
		for (j = (count - 1); j >= i; j--) {
			obj_desc1 = elements[j - 1]->package.elements[index];
			obj_desc2 = elements[j]->package.elements[index];

			if (((sort_direction == ACPI_SORT_ASCENDING) &&
			     (obj_desc1->integer.value >
			      obj_desc2->integer.value))
			    || ((sort_direction == ACPI_SORT_DESCENDING)
				&& (obj_desc1->integer.value <
				    obj_desc2->integer.value))) {
				temp_obj = elements[j - 1];
				elements[j - 1] = elements[j];
				elements[j] = temp_obj;
			}
		}
	}
}

/******************************************************************************
 *
 * FUNCTION:    acpi_ns_remove_element
 *
 * PARAMETERS:  obj_desc            - Package object element list
 *              index               - Index of element to remove
 *
 * RETURN:      None
 *
 * DESCRIPTION: Remove the requested element of a package and delete it.
 *
 *****************************************************************************/

static void
acpi_ns_remove_element(union acpi_operand_object *obj_desc, u32 index)
{
	union acpi_operand_object **source;
	union acpi_operand_object **dest;
	u32 count;
	u32 new_count;
	u32 i;

	ACPI_FUNCTION_NAME(ns_remove_element);

	count = obj_desc->package.count;
	new_count = count - 1;

	source = obj_desc->package.elements;
	dest = source;

	/* Examine all elements of the package object, remove matched index */

	for (i = 0; i < count; i++) {
		if (i == index) {
			acpi_ut_remove_reference(*source);	/* Remove one ref for being in pkg */
			acpi_ut_remove_reference(*source);
		} else {
			*dest = *source;
			dest++;
		}
		source++;
	}

	/* NULL terminate list and update the package count */

	*dest = NULL;
	obj_desc->package.count = new_count;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         /*******************************************************************************
 *
 * Module Name: nssearch - Namespace search
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

#ifdef ACPI_ASL_COMPILER
#include "amlcode.h"
#endif

#define _COMPONENT          ACPI_NAMESPACE
ACPI_MODULE_NAME("nssearch")

/* Local prototypes */
static acpi_status
acpi_ns_search_parent_tree(u32 target_name,
			   struct acpi_namespace_node *node,
			   acpi_object_type type,
			   struct acpi_namespace_node **return_node);

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_search_one_scope
 *
 * PARAMETERS:  target_name     - Ascii ACPI name to search for
 *              parent_node     - Starting node where search will begin
 *              type            - Object type to match
 *              return_node     - Where the matched Named obj is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Search a single level of the namespace. Performs a
 *              simple search of the specified level, and does not add
 *              entries or search parents.
 *
 *
 *      Named object lists are built (and subsequently dumped) in the
 *      order in which the names are encountered during the namespace load;
 *
 *      All namespace searching is linear in this implementation, but
 *      could be easily modified to support any improved search
 *      algorithm. However, the linear search was chosen for simplicity
 *      and because the trees are small and the other interpreter
 *      execution overhead is relatively high.
 *
 *      Note: CPU execution analysis has shown that the AML interpreter spends
 *      a very small percentage of its time searching the namespace. Therefore,
 *      the linear search seems to be sufficient, as there would seem to be
 *      little value in improving the search.
 *
 ******************************************************************************/

acpi_status
acpi_ns_search_one_scope(u32 target_name,
			 struct acpi_namespace_node *parent_node,
			 acpi_object_type type,
			 struct acpi_namespace_node **return_node)
{
	struct acpi_namespace_node *node;

	ACPI_FUNCTION_TRACE(ns_search_one_scope);

#ifdef ACPI_DEBUG_OUTPUT
	if (ACPI_LV_NAMES & acpi_dbg_level) {
		char *scope_name;

		scope_name = acpi_ns_get_external_pathname(parent_node);
		if (scope_name) {
			ACPI_DEBUG_PRINT((ACPI_DB_NAMES,
					  "Searching %s (%p) For [%4.4s] (%s)\n",
					  scope_name, parent_node,
					  ACPI_CAST_PTR(char, &target_name),
					  acpi_ut_get_type_name(type)));

			ACPI_FREE(scope_name);
		}
	}
#endif

	/*
	 * Search for name at this namespace level, which is to say that we
	 * must search for the name among the children of this object
	 */
	node = parent_node->child;
	while (node) {

		/* Check for match against the name */

		if (node->name.integer == target_name) {

			/* Resolve a control method alias if any */

			if (acpi_ns_get_type(node) ==
			    ACPI_TYPE_LOCAL_METHOD_ALIAS) {
				node =
				    ACPI_CAST_PTR(struct acpi_namespace_node,
						  node->object);
			}

			/* Found matching entry */

			ACPI_DEBUG_PRINT((ACPI_DB_NAMES,
					  "Name [%4.4s] (%s) %p found in scope [%4.4s] %p\n",
					  ACPI_CAST_PTR(char, &target_name),
					  acpi_ut_get_type_name(node->type),
					  node,
					  acpi_ut_get_node_name(parent_node),
					  parent_node));

			*return_node = node;
			return_ACPI_STATUS(AE_OK);
		}

		/* Didn't match name, move on to the next peer object */

		node = node->peer;
	}

	/* Searched entire namespace level, not found */

	ACPI_DEBUG_PRINT((ACPI_DB_NAMES,
			  "Name [%4.4s] (%s) not found in search in scope [%4.4s] "
			  "%p first child %p\n",
			  ACPI_CAST_PTR(char, &target_name),
			  acpi_ut_get_type_name(type),
			  acpi_ut_get_node_name(parent_node), parent_node,
			  parent_node->child));

	return_ACPI_STATUS(AE_NOT_FOUND);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_search_parent_tree
 *
 * PARAMETERS:  target_name     - Ascii ACPI name to search for
 *              node            - Starting node where search will begin
 *              type            - Object type to match
 *              return_node     - Where the matched Node is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Called when a name has not been found in the current namespace
 *              level. Before adding it or giving up, ACPI scope rules require
 *              searching enclosing scopes in cases identified by acpi_ns_local().
 *
 *              "A name is located by finding the matching name in the current
 *              name space, and then in the parent name space. If the parent
 *              name space does not contain the name, the search continues
 *              recursively until either the name is found or the name space
 *              does not have a parent (the root of the name space). This
 *              indicates that the name is not found" (From ACPI Specification,
 *              section 5.3)
 *
 ******************************************************************************/

static acpi_status
acpi_ns_search_parent_tree(u32 target_name,
			   struct acpi_namespace_node *node,
			   acpi_object_type type,
			   struct acpi_namespace_node **return_node)
{
	acpi_status status;
	struct acpi_namespace_node *parent_node;

	ACPI_FUNCTION_TRACE(ns_search_parent_tree);

	parent_node = node->parent;

	/*
	 * If there is no parent (i.e., we are at the root) or type is "local",
	 * we won't be searching the parent tree.
	 */
	if (!parent_node) {
		ACPI_DEBUG_PRINT((ACPI_DB_NAMES, "[%4.4s] has no parent\n",
				  ACPI_CAST_PTR(char, &target_name)));
		return_ACPI_STATUS(AE_NOT_FOUND);
	}

	if (acpi_ns_local(type)) {
		ACPI_DEBUG_PRINT((ACPI_DB_NAMES,
				  "[%4.4s] type [%s] must be local to this scope (no parent search)\n",
				  ACPI_CAST_PTR(char, &target_name),
				  acpi_ut_get_type_name(type)));
		return_ACPI_STATUS(AE_NOT_FOUND);
	}

	/* Search the parent tree */

	ACPI_DEBUG_PRINT((ACPI_DB_NAMES,
			  "Searching parent [%4.4s] for [%4.4s]\n",
			  acpi_ut_get_node_name(parent_node),
			  ACPI_CAST_PTR(char, &target_name)));

	/* Search parents until target is found or we have backed up to the root */

	while (parent_node) {
		/*
		 * Search parent scope. Use TYPE_ANY because we don't care about the
		 * object type at this point, we only care about the existence of
		 * the actual name we are searching for. Typechecking comes later.
		 */
		status =
		    acpi_ns_search_one_scope(target_name, parent_node,
					     ACPI_TYPE_ANY, return_node);
		if (ACPI_SUCCESS(status)) {
			return_ACPI_STATUS(status);
		}

		/* Not found here, go up another level (until we reach the root) */

		parent_node = parent_node->parent;
	}

	/* Not found in parent tree */

	return_ACPI_STATUS(AE_NOT_FOUND);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_search_and_enter
 *
 * PARAMETERS:  target_name         - Ascii ACPI name to search for (4 chars)
 *              walk_state          - Current state of the walk
 *              node                - Starting node where search will begin
 *              interpreter_mode    - Add names only in ACPI_MODE_LOAD_PASS_x.
 *                                    Otherwise,search only.
 *              type                - Object type to match
 *              flags               - Flags describing the search restrictions
 *              return_node         - Where the Node is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Search for a name segment in a single namespace level,
 *              optionally adding it if it is not found. If the passed
 *              Type is not Any and the type previously stored in the
 *              entry was Any (i.e. unknown), update the stored type.
 *
 *              In ACPI_IMODE_EXECUTE, search only.
 *              In other modes, search and add if not found.
 *
 ******************************************************************************/

acpi_status
acpi_ns_search_and_enter(u32 target_name,
			 struct acpi_walk_state *walk_state,
			 struct acpi_namespace_node *node,
			 acpi_interpreter_mode interpreter_mode,
			 acpi_object_type type,
			 u32 flags, struct acpi_namespace_node **return_node)
{
	acpi_status status;
	struct acpi_namespace_node *new_node;

	ACPI_FUNCTION_TRACE(ns_search_and_enter);

	/* Parameter validation */

	if (!node || !target_name || !return_node) {
		ACPI_ERROR((AE_INFO,
			    "Null parameter: Node %p Name 0x%X ReturnNode %p",
			    node, target_name, return_node));
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	/*
	 * Name must consist of valid ACPI characters. We will repair the name if
	 * necessary because we don't want to abort because of this, but we want
	 * all namespace names to be printable. A warning message is appropriate.
	 *
	 * This issue came up because there are in fact machines that exhibit
	 * this problem, and we want to be able to enable ACPI support for them,
	 * even though there are a few bad names.
	 */
	acpi_ut_repair_name(ACPI_CAST_PTR(char, &target_name));

	/* Try to find the name in the namespace level specified by the caller */

	*return_node = ACPI_ENTRY_NOT_FOUND;
	status = acpi_ns_search_one_scope(target_name, node, type, return_node);
	if (status != AE_NOT_FOUND) {
		/*
		 * If we found it AND the request specifies that a find is an error,
		 * return the error
		 */
		if (status == AE_OK) {

			/* The node was found in the namespace */

			/*
			 * If the namespace override feature is enabled for this node,
			 * delete any existing attached sub-object and make the node
			 * look like a new node that is owned by the override table.
			 */
			if (flags & ACPI_NS_OVERRIDE_IF_FOUND) {
				ACPI_DEBUG_PRINT((ACPI_DB_NAMES,
						  "Namespace override: %4.4s pass %u type %X Owner %X\n",
						  ACPI_CAST_PTR(char,
								&target_name),
						  interpreter_mode,
						  (*return_node)->type,
						  walk_state->owner_id));

				acpi_ns_delete_children(*return_node);
				if (acpi_gbl_runtime_namespace_override) {
					acpi_ut_remove_reference((*return_node)->object);
					(*return_node)->object = NULL;
					(*return_node)->owner_id =
					    walk_state->owner_id;
				} else {
					acpi_ns_remove_node(*return_node);
					*return_node = ACPI_ENTRY_NOT_FOUND;
				}
			}

			/* Return an error if we don't expect to find the object */

			else if (flags & ACPI_NS_ERROR_IF_FOUND) {
				status = AE_ALREADY_EXISTS;
			}
		}
#ifdef ACPI_ASL_COMPILER
		if (*return_node && (*return_node)->type == ACPI_TYPE_ANY) {
			(*return_node)->flags |= ANOBJ_IS_EXTERNAL;
		}
#endif

		/* Either found it or there was an error: finished either way */

		return_ACPI_STATUS(status);
	}

	/*
	 * The name was not found. If we are NOT performing the first pass
	 * (name entry) of loading the namespace, search the parent tree (all the
	 * way to the root if necessary.) We don't want to perform the parent
	 * search when the namespace is actually being loaded. We want to perform
	 * the search when namespace references are being resolved (load pass 2)
	 * and during the execution phase.
	 */
	if ((interpreter_mode != ACPI_IMODE_LOAD_PASS1) &&
	    (flags & ACPI_NS_SEARCH_PARENT)) {
		/*
		 * Not found at this level - search parent tree according to the
		 * ACPI specification
		 */
		status =
		    acpi_ns_search_parent_tree(target_name, node, type,
					       return_node);
		if (ACPI_SUCCESS(status)) {
			return_ACPI_STATUS(status);
		}
	}

	/* In execute mode, just search, never add names. Exit now */

	if (interpreter_mode == ACPI_IMODE_EXECUTE) {
		ACPI_DEBUG_PRINT((ACPI_DB_NAMES,
				  "%4.4s Not found in %p [Not adding]\n",
				  ACPI_CAST_PTR(char, &target_name), node));

		return_ACPI_STATUS(AE_NOT_FOUND);
	}

	/* Create the new named object */

	new_node = acpi_ns_create_node(target_name);
	if (!new_node) {
		return_ACPI_STATUS(AE_NO_MEMORY);
	}
#ifdef ACPI_ASL_COMPILER

	/* Node is an object defined by an External() statement */

	if (flags & ACPI_NS_EXTERNAL ||
	    (walk_state && walk_state->opcode == AML_SCOPE_OP)) {
		new_node->flags |= ANOBJ_IS_EXTERNAL;
	}
#endif

	if (flags & ACPI_NS_TEMPORARY) {
		new_node->flags |= ANOBJ_TEMPORARY;
	}

	/* Install the new object into the parent's list of children */

	acpi_ns_install_node(walk_state, node, new_node, type);
	*return_node = new_node;
	return_ACPI_STATUS(AE_OK);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  /******************************************************************************
 *
 * Module Name: nsutils - Utilities for accessing ACPI namespace, accessing
 *                        parents and siblings and Scope manipulation
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
#include "amlcode.h"

#define _COMPONENT          ACPI_NAMESPACE
ACPI_MODULE_NAME("nsutils")

/* Local prototypes */
#ifdef ACPI_OBSOLETE_FUNCTIONS
acpi_name acpi_ns_find_parent_name(struct acpi_namespace_node *node_to_search);
#endif

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_print_node_pathname
 *
 * PARAMETERS:  node            - Object
 *              message         - Prefix message
 *
 * DESCRIPTION: Print an object's full namespace pathname
 *              Manages allocation/freeing of a pathname buffer
 *
 ******************************************************************************/

void
acpi_ns_print_node_pathname(struct acpi_namespace_node *node,
			    const char *message)
{
	struct acpi_buffer buffer;
	acpi_status status;

	if (!node) {
		acpi_os_printf("[NULL NAME]");
		return;
	}

	/* Convert handle to full pathname and print it (with supplied message) */

	buffer.length = ACPI_ALLOCATE_LOCAL_BUFFER;

	status = acpi_ns_handle_to_pathname(node, &buffer, TRUE);
	if (ACPI_SUCCESS(status)) {
		if (message) {
			acpi_os_printf("%s ", message);
		}

		acpi_os_printf("[%s] (Node %p)", (char *)buffer.pointer, node);
		ACPI_FREE(buffer.pointer);
	}
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_get_type
 *
 * PARAMETERS:  node        - Parent Node to be examined
 *
 * RETURN:      Type field from Node whose handle is passed
 *
 * DESCRIPTION: Return the type of a Namespace node
 *
 ******************************************************************************/

acpi_object_type acpi_ns_get_type(struct acpi_namespace_node * node)
{
	ACPI_FUNCTION_TRACE(ns_get_type);

	if (!node) {
		ACPI_WARNING((AE_INFO, "Null Node parameter"));
		return_UINT8(ACPI_TYPE_ANY);
	}

	return_UINT8(node->type);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_local
 *
 * PARAMETERS:  type        - A namespace object type
 *
 * RETURN:      LOCAL if names must be found locally in objects of the
 *              passed type, 0 if enclosing scopes should be searched
 *
 * DESCRIPTION: Returns scope rule for the given object type.
 *
 ******************************************************************************/

u32 acpi_ns_local(acpi_object_type type)
{
	ACPI_FUNCTION_TRACE(ns_local);

	if (!acpi_ut_valid_object_type(type)) {

		/* Type code out of range  */

		ACPI_WARNING((AE_INFO, "Invalid Object Type 0x%X", type));
		return_UINT32(ACPI_NS_NORMAL);
	}

	return_UINT32(acpi_gbl_ns_properties[type] & ACPI_NS_LOCAL);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_get_internal_name_length
 *
 * PARAMETERS:  info            - Info struct initialized with the
 *                                external name pointer.
 *
 * RETURN:      None
 *
 * DESCRIPTION: Calculate the length of the internal (AML) namestring
 *              corresponding to the external (ASL) namestring.
 *
 ******************************************************************************/

void acpi_ns_get_internal_name_length(struct acpi_namestring_info *info)
{
	const char *next_external_char;
	u32 i;

	ACPI_FUNCTION_ENTRY();

	next_external_char = info->external_name;
	info->num_carats = 0;
	info->num_segments = 0;
	info->fully_qualified = FALSE;

	/*
	 * For the internal name, the required length is 4 bytes per segment, plus
	 * 1 each for root_prefix, multi_name_prefix_op, segment count, trailing null
	 * (which is not really needed, but no there's harm in putting it there)
	 *
	 * strlen() + 1 covers the first name_seg, which has no path separator
	 */
	if (ACPI_IS_ROOT_PREFIX(*next_external_char)) {
		info->fully_qualified = TRUE;
		next_external_char++;

		/* Skip redundant root_prefix, like \\_SB.PCI0.SBRG.EC0 */

		while (ACPI_IS_ROOT_PREFIX(*next_external_char)) {
			next_external_char++;
		}
	} else {
		/* Handle Carat prefixes */

		while (ACPI_IS_PARENT_PREFIX(*next_external_char)) {
			info->num_carats++;
			next_external_char++;
		}
	}

	/*
	 * Determine the number of ACPI name "segments" by counting the number of
	 * path separators within the string. Start with one segment since the
	 * segment count is [(# separators) + 1], and zero separators is ok.
	 */
	if (*next_external_char) {
		info->num_segments = 1;
		for (i = 0; next_external_char[i]; i++) {
			if (ACPI_IS_PATH_SEPARATOR(next_external_char[i])) {
				info->num_segments++;
			}
		}
	}

	info->length = (ACPI_NAME_SIZE * info->num_segments) +
	    4 + info->num_carats;

	info->next_external_char = next_external_char;
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_build_internal_name
 *
 * PARAMETERS:  info            - Info struct fully initialized
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Construct the internal (AML) namestring
 *              corresponding to the external (ASL) namestring.
 *
 ******************************************************************************/

acpi_status acpi_ns_build_internal_name(struct acpi_namestring_info *info)
{
	u32 num_segments = info->num_segments;
	char *internal_name = info->internal_name;
	const char *external_name = info->next_external_char;
	char *result = NULL;
	u32 i;

	ACPI_FUNCTION_TRACE(ns_build_internal_name);

	/* Setup the correct prefixes, counts, and pointers */

	if (info->fully_qualified) {
		internal_name[0] = AML_ROOT_PREFIX;

		if (num_segments <= 1) {
			result = &internal_name[1];
		} else if (num_segments == 2) {
			internal_name[1] = AML_DUAL_NAME_PREFIX;
			result = &internal_name[2];
		} else {
			internal_name[1] = AML_MULTI_NAME_PREFIX_OP;
			internal_name[2] = (char)num_segments;
			result = &internal_name[3];
		}
	} else {
		/*
		 * Not fully qualified.
		 * Handle Carats first, then append the name segments
		 */
		i = 0;
		if (info->num_carats) {
			for (i = 0; i < info->num_carats; i++) {
				internal_name[i] = AML_PARENT_PREFIX;
			}
		}

		if (num_segments <= 1) {
			result = &internal_name[i];
		} else if (num_segments == 2) {
			internal_name[i] = AML_DUAL_NAME_PREFIX;
			result = &internal_name[(acpi_size) i + 1];
		} else {
			internal_name[i] = AML_MULTI_NAME_PREFIX_OP;
			internal_name[(acpi_size) i + 1] = (char)num_segments;