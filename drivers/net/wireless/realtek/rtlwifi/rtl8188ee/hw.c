orm the I/O */

	status = acpi_hw_validate_io_request(address, width);
	if (ACPI_SUCCESS(status)) {
		status = acpi_os_read_port(address, value, width);
		return (status);
	}

	if (status != AE_AML_ILLEGAL_ADDRESS) {
		return (status);
	}

	/*
	 * There has been a protection violation within the request. Fall
	 * back to byte granularity port I/O and ignore the failing bytes.
	 * This provides Windows compatibility.
	 */
	for (i = 0, *value = 0; i < width; i += 8) {

		/* Validate and read one byte */

		if (acpi_hw_validate_io_request(address, 8) == AE_OK) {
			status = acpi_os_read_port(address, &one_byte, 8);
			if (ACPI_FAILURE(status)) {
				return (status);
			}

			*value |= (one_byte << i);
		}

		address++;
	}

	return (AE_OK);
}

/******************************************************************************
 *
 * FUNCTION:    acpi_hw_write_port
 *
 * PARAMETERS:  Address             Address of I/O port/register to write
 *              Value               Value to write
 *              Width               Number of bits
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Write data to an I/O port or register. This is a front-end
 *              to acpi_os_write_port that performs validation on both the port
 *              address and the length.
 *
 *****************************************************************************/

acpi_status acpi_hw_write_port(acpi_io_address address, u32 value, u32 width)
{
	acpi_status status;
	u32 i;

	/* Truncate address to 16 bits if requested */

	if (acpi_gbl_truncate_io_addresses) {
		address &= ACPI_UINT16_MAX;
	}

	/* Validate the entire request and perform the I/O */

	status = acpi_hw_validate_io_request(address, width);
	if (ACPI_SUCCESS(status)) {
		status = acpi_os_write_port(address, value, width);
		return (status);
	}

	if (status != AE_AML_ILLEGAL_ADDRESS) {
		return (status);
	}

	/*
	 * There has been a protection violation within the request. Fall
	 * back to byte granularity port I/O and ignore the failing bytes.
	 * This provides Windows compatibility.
	 */
	for (i = 0; i < width; i += 8) {

		/* Validate and write one byte */

		if (acpi_hw_validate_io_request(address, 8) == AE_OK) {
			status =
			    acpi_os_write_port(address, (value >> i) & 0xFF, 8);
			if (ACPI_FAILURE(status)) {
				return (status);
			}
		}

		address++;
	}

	return (AE_OK);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               /******************************************************************************
 *
 * Module Name: hwxface - Public ACPICA hardware interfaces
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

#define _COMPONENT          ACPI_HARDWARE
ACPI_MODULE_NAME("hwxface")

/******************************************************************************
 *
 * FUNCTION:    acpi_reset
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Set reset register in memory or IO space. Note: Does not
 *              support reset register in PCI config space, this must be
 *              handled separately.
 *
 ******************************************************************************/
acpi_status acpi_reset(void)
{
	struct acpi_generic_address *reset_reg;
	acpi_status status;

	ACPI_FUNCTION_TRACE(acpi_reset);

	reset_reg = &acpi_gbl_FADT.reset_register;

	/* Check if the reset register is supported */

	if (!(acpi_gbl_FADT.flags & ACPI_FADT_RESET_REGISTER) ||
	    !reset_reg->address) {
		return_ACPI_STATUS(AE_NOT_EXIST);
	}

	if (reset_reg->space_id == ACPI_ADR_SPACE_SYSTEM_IO) {
		/*
		 * For I/O space, write directly to the OSL. This bypasses the port
		 * validation mechanism, which may block a valid write to the reset
		 * register.
		 *
		 * NOTE:
		 * The ACPI spec requires the reset register width to be 8, so we
		 * hardcode it here and ignore the FADT value. This maintains
		 * compatibility with other ACPI implementations that have allowed
		 * BIOS code with bad register width values to go unnoticed.
		 */
		status =
		    acpi_os_write_port((acpi_io_address) reset_reg->address,
				       acpi_gbl_FADT.reset_value,
				       ACPI_RESET_REGISTER_WIDTH);
	} else {
		/* Write the reset value to the reset register */

		status = acpi_hw_write(acpi_gbl_FADT.reset_value, reset_reg);
	}

	return_ACPI_STATUS(status);
}

ACPI_EXPORT_SYMBOL(acpi_reset)

/******************************************************************************
 *
 * FUNCTION:    acpi_read
 *
 * PARAMETERS:  value               - Where the value is returned
 *              reg                 - GAS register structure
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Read from either memory or IO space.
 *
 * LIMITATIONS: <These limitations also apply to acpi_write>
 *      bit_width must be exactly 8, 16, 32, or 64.
 *      space_ID must be system_memory or system_IO.
 *      bit_offset and access_width are currently ignored, as there has
 *          not been a need to implement these.
 *
 ******************************************************************************/
acpi_status acpi_read(u64 *return_value, struct acpi_generic_address *reg)
{
	u32 value_lo;
	u32 value_hi;
	u32 width;
	u64 address;
	acpi_status status;

	ACPI_FUNCTION_NAME(acpi_read);

	if (!return_value) {
		return (AE_BAD_PARAMETER);
	}

	/* Validate contents of the GAS register. Allow 64-bit transfers */

	status = acpi_hw_validate_register(reg, 64, &address);
	if (ACPI_FAILURE(status)) {
		return (status);
	}

	/*
	 * Two address spaces supported: Memory or I/O. PCI_Config is
	 * not supported here because the GAS structure is insufficient
	 */
	if (reg->space_id == ACPI_ADR_SPACE_SYSTEM_MEMORY) {
		status = acpi_os_read_memory((acpi_physical_address)
					     address, return_value,
					     reg->bit_width);
		if (ACPI_FAILURE(status)) {
			return (status);
		}
	} else {		/* ACPI_ADR_SPACE_SYSTEM_IO, validated earlier */

		value_lo = 0;
		value_hi = 0;

		width = reg->bit_width;
		if (width == 64) {
			width = 32;	/* Break into two 32-bit transfers */
		}

		status = acpi_hw_read_port((acpi_io_address)
					   address, &value_lo, width);
		if (ACPI_FAILURE(status)) {
			return (status);
		}

		if (reg->bit_width == 64) {

			/* Read the top 32 bits */

			status = acpi_hw_read_port((acpi_io_address)
						   (address + 4), &value_hi,
						   32);
			if (ACPI_FAILURE(status)) {
				return (status);
			}
		}

		/* Set the return value only if status is AE_OK */

		*return_value = (value_lo | ((u64)value_hi << 32));
	}

	ACPI_DEBUG_PRINT((ACPI_DB_IO,
			  "Read:  %8.8X%8.8X width %2d from %8.8X%8.8X (%s)\n",
			  ACPI_FORMAT_UINT64(*return_value), reg->bit_width,
			  ACPI_FORMAT_UINT64(address),
			  acpi_ut_get_region_name(reg->space_id)));

	return (AE_OK);
}

ACPI_EXPORT_SYMBOL(acpi_read)

/******************************************************************************
 *
 * FUNCTION:    acpi_write
 *
 * PARAMETERS:  value               - Value to be written
 *              reg                 - GAS register structure
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Write to either memory or IO space.
 *
 ******************************************************************************/
acpi_status acpi_write(u64 value, struct acpi_generic_address *reg)
{
	u32 width;
	u64 address;
	acpi_status status;

	ACPI_FUNCTION_NAME(acpi_write);

	/* Validate contents of the GAS register. Allow 64-bit transfers */

	status = acpi_hw_validate_register(reg, 64, &address);
	if (ACPI_FAILURE(status)) {
		return (status);
	}

	/*
	 * Two address spaces supported: Memory or IO. PCI_Config is
	 * not supported here because the GAS structure is insufficient
	 */
	if (reg->space_id == ACPI_ADR_SPACE_SYSTEM_MEMORY) {
		status = acpi_os_write_memory((acpi_physical_address)
					      address, value, reg->bit_width);
		if (ACPI_FAILURE(status)) {
			return (status);
		}
	} else {		/* ACPI_ADR_SPACE_SYSTEM_IO, validated earlier */

		width = reg->bit_width;
		if (width == 64) {
			width = 32;	/* Break into two 32-bit transfers */
		}

		status = acpi_hw_write_port((acpi_io_address)
					    address, ACPI_LODWORD(value),
					    width);
		if (ACPI_FAILURE(status)) {
			return (status);
		}

		if (reg->bit_width == 64) {
			status = acpi_hw_write_port((acpi_io_address)
						    (address + 4),
						    ACPI_HIDWORD(value), 32);
			if (ACPI_FAILURE(status)) {
				return (status);
			}
		}
	}

	ACPI_DEBUG_PRINT((ACPI_DB_IO,
			  "Wrote: %8.8X%8.8X width %2d   to %8.8X%8.8X (%s)\n",
			  ACPI_FORMAT_UINT64(value), reg->bit_width,
			  ACPI_FORMAT_UINT64(address),
			  acpi_ut_get_region_name(reg->space_id)));

	return (status);
}

ACPI_EXPORT_SYMBOL(acpi_write)

#if (!ACPI_REDUCED_HARDWARE)
/*******************************************************************************
 *
 * FUNCTION:    acpi_read_bit_register
 *
 * PARAMETERS:  register_id     - ID of ACPI Bit Register to access
 *              return_value    - Value that was read from the register,
 *                                normalized to bit position zero.
 *
 * RETURN:      Status and the value read from the specified Register. Value
 *              returned is normalized to bit0 (is shifted all the way right)
 *
 * DESCRIPTION: ACPI bit_register read function. Does not acquire the HW lock.
 *
 * SUPPORTS:    Bit fields in PM1 Status, PM1 Enable, PM1 Control, and
 *              PM2 Control.
 *
 * Note: The hardware lock is not required when reading the ACPI bit registers
 *       since almost all of them are single bit and it does not matter that
 *       the parent hardware register can be split across two physical
 *       registers. The only multi-bit field is SLP_TYP in the PM1 control
 *       register, but this field does not cross an 8-bit boundary (nor does
 *       it make much sense to actually read this field.)
 *
 ******************************************************************************/
acpi_status acpi_read_bit_register(u32 register_id, u32 *return_value)
{
	struct acpi_bit_register_info *bit_reg_info;
	u32 register_value;
	u32 value;
	acpi_status status;

	ACPI_FUNCTION_TRACE_U32(acpi_read_bit_register, register_id);

	/* Get the info structure corresponding to the requested ACPI Register */

	bit_reg_info = acpi_hw_get_bit_register_info(register_id);
	if (!bit_reg_info) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	/* Read the entire parent register */

	status = acpi_hw_register_read(bit_reg_info->parent_register,
				       &register_value);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/* Normalize the value that was read, mask off other bits */

	value = ((register_value & bit_reg_info->access_bit_mask)
		 >> bit_reg_info->bit_position);

	ACPI_DEBUG_PRINT((ACPI_DB_IO,
			  "BitReg %X, ParentReg %X, Actual %8.8X, ReturnValue %8.8X\n",
			  register_id, bit_reg_info->parent_register,
			  register_value, value));

	*return_value = value;
	return_ACPI_STATUS(AE_OK);
}

ACPI_EXPORT_SYMBOL(acpi_read_bit_register)

/*******************************************************************************
 *
 * FUNCTION:    acpi_write_bit_register
 *
 * PARAMETERS:  register_id     - ID of ACPI Bit Register to access
 *              value           - Value to write to the register, in bit
 *                                position zero. The bit is automatically
 *                                shifted to the correct position.
 *
 * RETURN:      Status
 *
 * DESCRIPTION: ACPI Bit Register write function. Acquires the hardware lock
 *              since most operations require a read/modify/write sequence.
 *
 * SUPPORTS:    Bit fields in PM1 Status, PM1 Enable, PM1 Control, and
 *              PM2 Control.
 *
 * Note that at this level, the fact that there may be actually two
 * hardware registers (A and B - and B may not exist) is abstracted.
 *
 ******************************************************************************/
acpi_status acpi_write_bit_register(u32 register_id, u32 value)
{
	struct acpi_bit_register_info *bit_reg_info;
	acpi_cpu_flags lock_flags;
	u32 register_value;
	acpi_status status = AE_OK;

	ACPI_FUNCTION_TRACE_U32(acpi_write_bit_register, register_id);

	/* Get the info structure corresponding to the requested ACPI Register */

	bit_reg_info = acpi_hw_get_bit_register_info(register_id);
	if (!bit_reg_info) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	raw_spin_lock_irqsave(acpi_gbl_hardware_lock, lock_flags);

	/*
	 * At this point, we know that the parent register is one of the
	 * following: PM1 Status, PM1 Enable, PM1 Control, or PM2 Control
	 */
	if (bit_reg_info->parent_register != ACPI_REGISTER_PM1_STATUS) {
		/*
		 * 1) Case for PM1 Enable, PM1 Control, and PM2 Control
		 *
		 * Perform a register read to preserve the bits that we are not
		 * interested in
		 */
		status = acpi_hw_register_read(bit_reg_info->parent_register,
					       &register_value);
		if (ACPI_FAILURE(status)) {
			goto unlock_and_exit;
		}

		/*
		 * Insert the input bit into the value that was just read
		 * and write the register
		 */
		ACPI_REGISTER_INSERT_VALUE(register_value,
					   bit_reg_info->bit_position,
					   bit_reg_info->access_bit_mask,
					   value);

		status = acpi_hw_register_write(bit_reg_info->parent_register,
						register_value);
	} else {
		/*
		 * 2) Case for PM1 Status
		 *
		 * The Status register is different from the rest. Clear an event
		 * by writing 1, writing 0 has no effect. So, the only relevant
		 * information is the single bit we're interested in, all others
		 * should be written as 0 so they will be left unchanged.
		 */
		register_value = ACPI_REGISTER_PREPARE_BITS(value,
							    bit_reg_info->
							    bit_position,
							    bit_reg_info->
							    access_bit_mask);

		/* No need to write the register if value is all zeros */

		if (register_value) {
			status =
			    acpi_hw_register_write(ACPI_REGISTER_PM1_STATUS,
						   register_value);
		}
	}

	ACPI_DEBUG_PRINT((ACPI_DB_IO,
			  "BitReg %X, ParentReg %X, Value %8.8X, Actual %8.8X\n",
			  register_id, bit_reg_info->parent_register, value,
			  register_value));

unlock_and_exit:

	raw_spin_unlock_irqrestore(acpi_gbl_hardware_lock, lock_flags);
	return_ACPI_STATUS(status);
}

ACPI_EXPORT_SYMBOL(acpi_write_bit_register)
#endif				/* !ACPI_REDUCED_HARDWARE */
/*******************************************************************************
 *
 * FUNCTION:    acpi_get_sleep_type_data
 *
 * PARAMETERS:  sleep_state         - Numeric sleep state
 *              *sleep_type_a        - Where SLP_TYPa is returned
 *              *sleep_type_b        - Where SLP_TYPb is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Obtain the SLP_TYPa and SLP_TYPb values for the requested
 *              sleep state via the appropriate \_Sx object.
 *
 *  The sleep state package returned from the corresponding \_Sx_ object
 *  must contain at least one integer.
 *
 *  March 2005:
 *  Added support for a package that contains two integers. This
 *  goes against the ACPI specification which defines this object as a
 *  package with one encoded DWORD integer. However, existing practice
 *  by many BIOS vendors is to return a package with 2 or more integer
 *  elements, at least one per sleep type (A/B).
 *
 *  January 2013:
 *  Therefore, we must be prepared to accept a package with either a
 *  single integer or multiple integers.
 *
 *  The single integer DWORD format is as follows:
 *      BYTE 0 - Value for the PM1A SLP_TYP register
 *      BYTE 1 - Value for the PM1B SLP_TYP register
 *      BYTE 2-3 - Reserved
 *
 *  The dual integer format is as follows:
 *      Integer 0 - Value for the PM1A SLP_TYP register
 *      Integer 1 - Value for the PM1A SLP_TYP register
 *
 ******************************************************************************/
acpi_status
acpi_get_sleep_type_data(u8 sleep_state, u8 *sleep_type_a, u8 *sleep_type_b)
{
	acpi_status status;
	struct acpi_evaluate_info *info;
	union acpi_operand_object **elements;

	ACPI_FUNCTION_TRACE(acpi_get_sleep_type_data);

	/* Validate parameters */

	if ((sleep_state > ACPI_S_STATES_MAX) || !sleep_type_a || !sleep_type_b) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	/* Allocate the evaluation information block */

	info = ACPI_ALLOCATE_ZEROED(sizeof(struct acpi_evaluate_info));
	if (!info) {
		return_ACPI_STATUS(AE_NO_MEMORY);
	}

	/*
	 * Evaluate the \_Sx namespace object containing the register values
	 * for this state
	 */
	info->relative_pathname = ACPI_CAST_PTR(char,
						acpi_gbl_sleep_state_names
						[sleep_state]);

	status = acpi_ns_evaluate(info);
	if (ACPI_FAILURE(status)) {
		if (status == AE_NOT_FOUND) {

			/* The _Sx states are optional, ignore NOT_FOUND */

			goto final_cleanup;
		}

		goto warning_cleanup;
	}

	/* Must have a return object */

	if (!info->return_object) {
		ACPI_ERROR((AE_INFO, "No Sleep State object returned from [%s]",
			    info->relative_pathname));
		status = AE_AML_NO_RETURN_VALUE;
		goto warning_cleanup;
	}

	/* Return object must be of type Package */

	if (info->return_object->common.type != ACPI_TYPE_PACKAGE) {
		ACPI_ERROR((AE_INFO,
			    "Sleep State return object is not a Package"));
		status = AE_AML_OPERAND_TYPE;
		goto return_value_cleanup;
	}

	/*
	 * Any warnings about the package length or the object types have
	 * already been issued by the predefined name module -- there is no
	 * need to repeat them here.
	 */
	elements = info->return_object->package.elements;
	switch (info->return_object->package.count) {
	case 0:

		status = AE_AML_PACKAGE_LIMIT;
		break;

	case 1:

		if (elements[0]->common.type != ACPI_TYPE_INTEGER) {
			status = AE_AML_OPERAND_TYPE;
			break;
		}

		/* A valid _Sx_ package with one integer */

		*sleep_type_a = (u8)elements[0]->integer.value;
		*sleep_type_b = (u8)(elements[0]->integer.value >> 8);
		break;

	case 2:
	default:

		if ((elements[0]->common.type != ACPI_TYPE_INTEGER) ||
		    (elements[1]->common.type != ACPI_TYPE_INTEGER)) {
			status = AE_AML_OPERAND_TYPE;
			break;
		}

		/* A valid _Sx_ package with two integers */

		*sleep_type_a = (u8)elements[0]->integer.value;
		*sleep_type_b = (u8)elements[1]->integer.value;
		break;
	}

return_value_cleanup:
	acpi_ut_remove_reference(info->return_object);

warning_cleanup:
	if (ACPI_FAILURE(status)) {
		ACPI_EXCEPTION((AE_INFO, status,
				"While evaluating Sleep State [%s]",
				info->relative_pathname));
	}

final_cleanup:
	ACPI_FREE(info);
	return_ACPI_STATUS(status);
}

ACPI_EXPORT_SYMBOL(acpi_get_sleep_type_data)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   /******************************************************************************
 *
 * Name: hwxfsleep.c - ACPI Hardware Sleep/Wake External Interfaces
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

#define _COMPONENT          ACPI_HARDWARE
ACPI_MODULE_NAME("hwxfsleep")

/* Local prototypes */
#if (!ACPI_REDUCED_HARDWARE)
static acpi_status
acpi_hw_set_firmware_waking_vectors(struct acpi_table_facs *facs,
				    acpi_physical_address physical_address,
				    acpi_physical_address physical_address64);
#endif

static acpi_status acpi_hw_sleep_dispatch(u8 sleep_state, u32 function_id);

/*
 * Dispatch table used to efficiently branch to the various sleep
 * functions.
 */
#define ACPI_SLEEP_FUNCTION_ID         0
#define ACPI_WAKE_PREP_FUNCTION_ID     1
#define ACPI_WAKE_FUNCTION_ID          2

/* Legacy functions are optional, based upon ACPI_REDUCED_HARDWARE */

static struct acpi_sleep_functions acpi_sleep_dispatch[] = {
	{ACPI_HW_OPTIONAL_FUNCTION(acpi_hw_legacy_sleep),
	 acpi_hw_extended_sleep},
	{ACPI_HW_OPTIONAL_FUNCTION(acpi_hw_legacy_wake_prep),
	 acpi_hw_extended_wake_prep},
	{ACPI_HW_OPTIONAL_FUNCTION(acpi_hw_legacy_wake), acpi_hw_extended_wake}
};

/*
 * These functions are removed for the ACPI_REDUCED_HARDWARE case:
 *      acpi_set_firmware_waking_vectors
 *      acpi_set_firmware_waking_vector
 *      acpi_set_firmware_waking_vector64
 *      acpi_enter_sleep_state_s4bios
 */

#if (!ACPI_REDUCED_HARDWARE)
/*******************************************************************************
 *
 * FUNCTION:    acpi_hw_set_firmware_waking_vectors
 *
 * PARAMETERS:  facs                - Pointer to FACS table
 *              physical_address    - 32-bit physical address of ACPI real mode
 *                                    entry point.
 *              physical_address64  - 64-bit physical address of ACPI protected
 *                                    mode entry point.
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Sets the firmware_waking_vector fields of the FACS
 *
 ******************************************************************************/

static acpi_status
acpi_hw_set_firmware_waking_vectors(struct acpi_table_facs *facs,
				    acpi_physical_address physical_address,
				    acpi_physical_address physical_address64)
{
	ACPI_FUNCTION_TRACE(acpi_hw_set_firmware_waking_vectors);


	/*
	 * According to the ACPI specification 2.0c and later, the 64-bit
	 * waking vector should be cleared and the 32-bit waking vector should
	 * be used, unless we want the wake-up code to be called by the BIOS in
	 * Protected Mode.  Some systems (for example HP dv5-1004nr) are known
	 * to fail to resume if the 64-bit vector is used.
	 */

	/* Set the 32-bit vector */

	facs->firmware_waking_vector = (u32)physical_address;

	if (facs->length > 32) {
		if (facs->version >= 1) {

			/* Set the 64-bit vector */

			facs->xfirmware_waking_vector = physical_address64;
		} else {
			/* Clear the 64-bit vector if it exists */

			facs->xfirmware_waking_vector = 0;
		}
	}

	return_ACPI_STATUS(AE_OK);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_set_firmware_waking_vectors
 *
 * PARAMETERS:  physical_address    - 32-bit physical address of ACPI real mode
 *                                    entry point.
 *              physical_address64  - 64-bit physical address of ACPI protected
 *                                    mode entry point.
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Sets the firmware_waking_vector fields of the FACS
 *
 ******************************************************************************/

acpi_status
acpi_set_firmware_waking_vectors(acpi_physical_address physical_address,
				 acpi_physical_address physical_address64)
{

	ACPI_FUNCTION_TRACE(acpi_set_firmware_waking_vectors);

	if (acpi_gbl_FACS) {
		(void)acpi_hw_set_firmware_waking_vectors(acpi_gbl_FACS,
							  physical_address,
							  physical_address64);
	}

	return_ACPI_STATUS(AE_OK);
}

ACPI_EXPORT_SYMBOL(acpi_set_firmware_waking_vectors)

/*******************************************************************************
 *
 * FUNCTION:    acpi_set_firmware_waking_vector
 *
 * PARAMETERS:  physical_address    - 32-bit physical address of ACPI real mode
 *                                    entry point.
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Sets the 32-bit firmware_waking_vector field of the FACS
 *
 ******************************************************************************/
acpi_status acpi_set_firmware_waking_vector(u32 physical_address)
{
	acpi_status status;

	ACPI_FUNCTION_TRACE(acpi_set_firmware_waking_vector);

	status = acpi_set_firmware_waking_vectors((acpi_physical_address)
						  physical_address, 0);

	return_ACPI_STATUS(status);
}

ACPI_EXPORT_SYMBOL(acpi_set_firmware_waking_vector)

#if ACPI_MACHINE_WIDTH == 64
/*******************************************************************************
 *
 * FUNCTION:    acpi_set_firmware_waking_vector64
 *
 * PARAMETERS:  physical_address    - 64-bit physical address of ACPI protected
 *                                    mode entry point.
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Sets the 64-bit X_firmware_waking_vector field of the FACS, if
 *              it exists in the table. This function is intended for use with
 *              64-bit host operating systems.
 *
 ******************************************************************************/
acpi_status acpi_set_firmware_waking_vector64(u64 physical_address)
{
	acpi_status status;

	ACPI_FUNCTION_TRACE(acpi_set_firmware_waking_vector64);

	status = acpi_set_firmware_waking_vectors(0,
						  (acpi_physical_address)
						  physical_address);

	return_ACPI_STATUS(status);
}

ACPI_EXPORT_SYMBOL(acpi_set_firmware_waking_vector64)
#endif
/*******************************************************************************
 *
 * FUNCTION:    acpi_enter_sleep_state_s4bios
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Perform a S4 bios request.
 *              THIS FUNCTION MUST BE CALLED WITH INTERRUPTS DISABLED
 *
 ******************************************************************************/
acpi_status acpi_enter_sleep_state_s4bios(void)
{
	u32 in_value;
	acpi_status status;

	ACPI_FUNCTION_TRACE(acpi_enter_sleep_state_s4bios);

	/* Clear the wake status bit (PM1) */

	status =
	    acpi_write_bit_register(ACPI_BITREG_WAKE_STATUS, ACPI_CLEAR_STATUS);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	status = acpi_hw_clear_acpi_status();
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/*
	 * 1) Disable/Clear all GPEs
	 * 2) Enable all wakeup GPEs
	 */
	status = acpi_hw_disable_all_gpes();
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}
	acpi_gbl_system_awake_and_running = FALSE;

	status = acpi_hw_enable_all_wakeup_gpes();
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	ACPI_FLUSH_CPU_CACHE();

	status = acpi_hw_write_port(acpi_gbl_FADT.smi_command,
				    (u32)acpi_gbl_FADT.s4_bios_request, 8);

	do {
		acpi_os_stall(ACPI_USEC_PER_MSEC);
		status =
		    acpi_read_bit_register(ACPI_BITREG_WAKE_STATUS, &in_value);
		if (ACPI_FAILURE(status)) {
			return_ACPI_STATUS(status);
		}
	} while (!in_value);

	return_ACPI_STATUS(AE_OK);
}

ACPI_EXPORT_SYMBOL(acpi_enter_sleep_state_s4bios)
#endif				/* !ACPI_REDUCED_HARDWARE */
/*******************************************************************************
 *
 * FUNCTION:    acpi_hw_sleep_dispatch
 *
 * PARAMETERS:  sleep_state         - Which sleep state to enter/exit
 *              function_id         - Sleep, wake_prep, or Wake
 *
 * RETURN:      Status from the invoked sleep handling function.
 *
 * DESCRIPTION: Dispatch a sleep/wake request to the appropriate handling
 *              function.
 *
 ******************************************************************************/
static acpi_status acpi_hw_sleep_dispatch(u8 sleep_state, u32 function_id)
{
	acpi_status status;
	struct acpi_sleep_functions *sleep_functions =
	    &acpi_sleep_dispatch[function_id];

#if (!ACPI_REDUCED_HARDWARE)
	/*
	 * If the Hardware Reduced flag is set (from the FADT), we must
	 * use the extended sleep registers (FADT). Note: As per the ACPI
	 * specification, these extended registers are to be used for HW-reduced
	 * platforms only. They are not general-purpose replacements for the
	 * legacy PM register sleep support.
	 */
	if (acpi_gbl_reduced_hardware) {
		status = sleep_functions->extended_function(sleep_state);
	} else {
		/* Legacy sleep */

		status = sleep_functions->legacy_function(sleep_state);
	}

	return (status);

#else
	/*
	 * For the case where reduced-hardware-only code is being generated,
	 * we know that only the extended sleep registers are available
	 */
	status = sleep_functions->extended_function(sleep_state);
	return (status);

#endif				/* !ACPI_REDUCED_HARDWARE */
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_enter_sleep_state_prep
 *
 * PARAMETERS:  sleep_state         - Which sleep state to enter
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Prepare to enter a system sleep state.
 *              This function must execute with interrupts enabled.
 *              We break sleeping into 2 stages so that OSPM can handle
 *              various OS-specific tasks between the two steps.
 *
 ******************************************************************************/

acpi_status acpi_enter_sleep_state_prep(u8 sleep_state)
{
	acpi_status status;
	struct acpi_object_list arg_list;
	union acpi_object arg;
	u32 sst_value;

	ACPI_FUNCTION_TRACE(acpi_enter_sleep_state_prep);

	status = acpi_get_sleep_type_data(sleep_state,
					  &acpi_gbl_sleep_type_a,
					  &acpi_gbl_sleep_type_b);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	status = acpi_get_sleep_type_data(ACPI_STATE_S0,
					  &acpi_gbl_sleep_type_a_s0,
					  &acpi_gbl_sleep_type_b_s0);
	if (ACPI_FAILURE(status)) {
		acpi_gbl_sleep_type_a_s0 = ACPI_SLEEP_TYPE_INVALID;
	}

	/* Execute the _PTS method (Prepare To Sleep) */

	arg_list.count = 1;
	arg_list.pointer = &arg;
	arg.type = ACPI_TYPE_INTEGER;
	arg.integer.value = sleep_state;

	status =
	    acpi_evaluate_object(NULL, METHOD_PATHNAME__PTS, &arg_list, NULL);
	if (ACPI_FAILURE(status) && status != AE_NOT_FOUND) {
		return_ACPI_STATUS(status);
	}

	/* Setup the argument to the _SST method (System STatus) */

	switch (sleep_state) {
	case ACPI_STATE_S0:

		sst_value = ACPI_SST_WORKING;
		break;

	case ACPI_STATE_S1:
	case ACPI_STATE_S2:
	case ACPI_STATE_S3:

		sst_value = ACPI_SST_SLEEPING;
		break;

	case ACPI_STATE_S4:

		sst_value = ACPI_SST_SLEEP_CONTEXT;
		break;

	default:

		sst_value = ACPI_SST_INDICATOR_OFF;	/* Default is off */
		break;
	}

	/*
	 * Set the system indicators to show the desired sleep state.
	 * _SST is an optional method (return no error if not found)
	 */
	acpi_hw_execute_sleep_method(METHOD_PATHNAME__SST, sst_value);
	return_ACPI_STATUS(AE_OK);
}

ACPI_EXPORT_SYMBOL(acpi_enter_sleep_state_prep)

/*******************************************************************************
 *
 * FUNCTION:    acpi_enter_sleep_state
 *
 * PARAMETERS:  sleep_state         - Which sleep state to enter
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Enter a system sleep state
 *              THIS FUNCTION MUST BE CALLED WITH INTERRUPTS DISABLED
 *
 ******************************************************************************/
acpi_status acpi_enter_sleep_state(u8 sleep_state)
{
	acpi_status status;

	ACPI_FUNCTION_TRACE(acpi_enter_sleep_state);

	if ((acpi_gbl_sleep_type_a > ACPI_SLEEP_TYPE_MAX) ||
	    (acpi_gbl_sleep_type_b > ACPI_SLEEP_TYPE_MAX)) {
		ACPI_ERROR((AE_INFO, "Sleep values out of range: A=0x%X B=0x%X",
			    acpi_gbl_sleep_type_a, acpi_gbl_sleep_type_b));
		return_ACPI_STATUS(AE_AML_OPERAND_VALUE);
	}

	status = acpi_hw_sleep_dispatch(sleep_state, ACPI_SLEEP_FUNCTION_ID);
	return_ACPI_STATUS(status);
}

ACPI_EXPORT_SYMBOL(acpi_enter_sleep_state)

/*******************************************************************************
 *
 * FUNCTION:    acpi_leave_sleep_state_prep
 *
 * PARAMETERS:  sleep_state         - Which sleep state we are exiting
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Perform the first state of OS-independent ACPI cleanup after a
 *              sleep. Called with interrupts DISABLED.
 *              We break wake/resume into 2 stages so that OSPM can handle
 *              various OS-specific tasks between the two steps.
 *
 ******************************************************************************/
acpi_status acpi_leave_sleep_state_prep(u8 sleep_state)
{
	acpi_status status;

	ACPI_FUNCTION_TRACE(acpi_leave_sleep_state_prep);

	status =
	    acpi_hw_sleep_dispatch(sleep_state, ACPI_WAKE_PREP_FUNCTION_ID);
	return_ACPI_STATUS(status);
}

ACPI_EXPORT_SYMBOL(acpi_leave_sleep_state_prep)

/*******************************************************************************
 *
 * FUNCTION:    acpi_leave_sleep_state
 *
 * PARAMETERS:  sleep_state         - Which sleep state we are exiting
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Perform OS-independent ACPI cleanup after a sleep
 *              Called with interrupts ENABLED.
 *
 ******************************************************************************/
acpi_status acpi_leave_sleep_state(u8 sleep_state)
{
	acpi_status status;

	ACPI_FUNCTION_TRACE(acpi_leave_sleep_state);

	status = acpi_hw_sleep_dispatch(sleep_state, ACPI_WAKE_FUNCTION_ID);
	return_ACPI_STATUS(status);
}

ACPI_EXPORT_SYMBOL(acpi_leave_sleep_state)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  /*******************************************************************************
 *
 * Module Name: nsaccess - Top-level functions for accessing ACPI namespace
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
#include "acdispat.h"

#define _COMPONENT          ACPI_NAMESPACE
ACPI_MODULE_NAME("nsaccess")

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_root_initialize
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Allocate and initialize the default root named objects
 *
 * MUTEX:       Locks namespace for entire execution
 *
 ******************************************************************************/
acpi_status acpi_ns_root_initialize(void)
{
	acpi_status status;
	const struct acpi_predefined_names *init_val = NULL;
	struct acpi_namespace_node *new_node;
	union acpi_operand_object *obj_desc;
	acpi_string val = NULL;

	ACPI_FUNCTION_TRACE(ns_root_initialize);

	status = acpi_ut_acquire_mutex(ACPI_MTX_NAMESPACE);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/*
	 * The global root ptr is initially NULL, so a non-NULL value indicates
	 * that acpi_ns_root_initialize() has already been called; just return.
	 */
	if (acpi_gbl_root_node) {
		status = AE_OK;
		goto unlock_and_exit;
	}

	/*
	 * Tell the rest of the subsystem that the root is initialized
	 * (This is OK because the namespace is locked)
	 */
	acpi_gbl_root_node = &acpi_gbl_root_node_struct;

	/* Enter the pre-defined names in the name table */

	ACPI_DEBUG_PRINT((ACPI_DB_INFO,
			  "Entering predefined entries into namespace\n"));

	for (init_val = acpi_gbl_pre_defined_names; init_val->name; init_val++) {

		/* _OSI is optional for now, will be permanent later */

		if (!strcmp(init_val->name, "_OSI")
		    && !acpi_gbl_create_osi_method) {
			continue;
		}

		status = acpi_ns_lookup(NULL, init_val->name, init_val->type,
					ACPI_IMODE_LOAD_PASS2,
					ACPI_NS_NO_UPSEARCH, NULL, &new_node);
		if (ACPI_FAILURE(status)) {
			ACPI_EXCEPTION((AE_INFO, status,
					"Could not create predefined name %s",
					init_val->name));
			continue;
		}

		/*
		 * Name entered successfully. If entry in pre_defined_names[] specifies
		 * an initial value, create the initial value.
		 */
		if (init_val->val) {
			status = acpi_os_predefined_override(init_val, &val);
			if (ACPI_FAILURE(status)) {
				ACPI_ERROR((AE_INFO,
					    "Could not override predefined %s",
					    init_val->name));
			}

			if (!val) {
				val = init_val->val;
			}

			/*
			 * Entry requests an initial value, allocate a
			 * descriptor for it.
			 */
			obj_desc =
			    acpi_ut_create_internal_object(init_val->type);
			if (!obj_desc) {
				status = AE_NO_MEMORY;
				goto unlock_and_exit;
			}

			/*
			 * Convert value string from table entry to
			 * internal representation. Only types actually
			 * used for initial values are implemented here.
			 */
			switch (init_val->type) {
			case ACPI_TYPE_METHOD:

				obj_desc->method.param_count =
				    (u8) ACPI_TO_INTEGER(val);
				obj_desc->common.flags |= AOPOBJ_DATA_VALID;

#if defined (ACPI_ASL_COMPILER)

				/* Save the parameter count for the iASL compiler */

				new_node->value = obj_desc->method.param_count;
#else
				/* Mark this as a very SPECIAL method */

				obj_desc->method.info_flags =
				    ACPI_METHOD_INTERNAL_ONLY;
				obj_desc->method.dispatch.implementation =
				    acpi_ut_osi_implementation;
#endif
				break;

			case ACPI_TYPE_INTEGER:

				obj_desc->integer.value = ACPI_TO_INTEGER(val);
				break;

			case ACPI_TYPE_STRING:

				/* Build an object around the static string */

				obj_desc->string.length = (u32)strlen(val);
				obj_desc->string.pointer = val;
				obj_desc->common.flags |= AOPOBJ_STATIC_POINTER;
				break;

			case ACPI_TYPE_MUTEX:

				obj_desc->mutex.node = new_node;
				obj_desc->mutex.sync_level =
				    (u8) (ACPI_TO_INTEGER(val) - 1);

				/* Create a mutex */

				status =
				    acpi_os_create_mutex(&obj_desc->mutex.
							 os_mutex);
				if (ACPI_FAILURE(status)) {
					acpi_ut_remove_reference(obj_desc);
					goto unlock_and_exit;
				}

				/* Special case for ACPI Global Lock */

				if (strcmp(init_val->name, "_GL_") == 0) {
					acpi_gbl_global_lock_mutex = obj_desc;

					/* Create additional counting semaphore for global lock */

					status =
					    acpi_os_create_semaphore(1, 0,
								     &acpi_gbl_global_lock_semaphore);
					if (ACPI_FAILURE(status)) {
						acpi_ut_remove_reference
						    (obj_desc);
						goto unlock_and_exit;
					}
				}
				break;

			default:

				ACPI_ERROR((AE_INFO,
					    "Unsupported initial type value 0x%X",
					    init_val->type));
				acpi_ut_remove_reference(obj_desc);
				obj_desc = NULL;
				continue;
			}

			/* Store pointer to value descriptor in the Node */

			status = acpi_ns_attach_object(new_node, obj_desc,
						       obj_desc->common.type);

			/* Remove local reference to the object */

			acpi_ut_remove_reference(obj_desc);
		}
	}

unlock_and_exit:
	(void)acpi_ut_release_mutex(ACPI_MTX_NAMESPACE);

	/* Save a handle to "_GPE", it is always present */

	if (ACPI_SUCCESS(status)) {
		status = acpi_ns_get_node(NULL, "\\_GPE", ACPI_NS_NO_UPSEARCH,
					  &acpi_gbl_fadt_gpe_device);
	}

	return_ACPI_STATUS(status);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_lookup
 *
 * PARAMETERS:  scope_info      - Current scope info block
 *              pathname        - Search pathname, in internal format
 *                                (as represented in the AML stream)
 *              type            - Type associated with name
 *              interpreter_mode - IMODE_LOAD_PASS2 => add name if not found
 *              flags           - Flags describing the search restrictions
 *              walk_state      - Current state of the walk
 *              return_node     - Where the Node is placed (if found
 *                                or created successfully)
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Find or enter the passed name in the name space.
 *              Log an error if name not found in Exec mode.
 *
 * MUTEX:       Assumes namespace is locked.
 *
 ******************************************************************************/

acpi_status
acpi_ns_lookup(union acpi_generic_state *scope_info,
	       char *pathname,
	       acpi_object_type type,
	       acpi_interpreter_mode interpreter_mode,
	       u32 flags,
	       struct acpi_walk_state *walk_state,
	       struct acpi_namespace_node **return_node)
{
	acpi_status status;
	char *path = pathname;
	struct acpi_namespace_node *prefix_node;
	struct acpi_namespace_node *current_node = NULL;
	struct acpi_namespace_node *this_node = NULL;
	u32 num_segments;
	u32 num_carats;
	acpi_name simple_name;
	acpi_object_type type_to_check_for;
	acpi_object_type this_search_type;
	u32 search_parent_flag = ACPI_NS_SEARCH_PARENT;
	u32 local_flags;

	ACPI_FUNCTION_TRACE(ns_lookup);

	if (!return_node) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	local_flags = flags &
	    ~(ACPI_NS_ERROR_IF_FOUND | ACPI_NS_OVERRIDE_IF_FOUND |
	      ACPI_NS_SEARCH_PARENT);
	*return_node = ACPI_ENTRY_NOT_FOUND;
	acpi_gbl_ns_lookup_count++;

	if (!acpi_gbl_root_node) {
		return_ACPI_STATUS(AE_NO_NAMESPACE);
	}

	/* Get the prefix scope. A null scope means use the root scope */

	if ((!scope_info) || (!scope_info->scope.node)) {
		ACPI_DEBUG_PRINT((ACPI_DB_NAMES,
				  "Null scope prefix, using root node (%p)\n",
				  acpi_gbl_root_node));

		prefix_node = acpi_gbl_root_node;
	} else {
		prefix_node = scope_info->scope.node;
		if (ACPI_GET_DESCRIPTOR_TYPE(prefix_node) !=
		    ACPI_DESC_TYPE_NAMED) {
			ACPI_ERROR((AE_INFO, "%p is not a namespace node [%s]",
				    prefix_node,
				    acpi_ut_get_descriptor_name(prefix_node)));
			return_ACPI_STATUS(AE_AML_INTERNAL);
		}

		if (!(flags & ACPI_NS_PREFIX_IS_SCOPE)) {
			/*
			 * This node might not be a actual "scope" node (such as a
			 * Device/Method, etc.)  It could be a Package or other object
			 * node. Backup up the tree to find the containing scope node.
			 */
			while (!acpi_ns_opens_scope(prefix_node->type) &&
			       prefix_node->type != ACPI_TYPE_ANY) {
				prefix_node = prefix_node->parent;
			}
		}
	}

	/* Save type. TBD: may be no longer necessary */

	type_to_check_for = type;

	/*
	 * Begin examination of the actual pathname
	 */
	if (!pathname) {

		/* A Null name_path is allowed and refers to the root */

		num_segments = 0;
		this_node = acpi_gbl_root_node;
		path = "";

		ACPI_DEBUG_PRINT((ACPI_DB_NAMES,
				  "Null Pathname (Zero segments), Flags=%X\n",
				  flags));
	} else {
		/*
		 * Name pointer is valid (and must be in internal name format)
		 *
		 * Check for scope prefixes:
		 *
		 * As represented in the AML stream, a namepath consists of an
		 * optional scope prefix followed by a name segment part.
		 *
		 * If present, the scope prefix is either a Root Prefix (in
		 * which case the name is fully qualified), or one or more
		 * Parent Prefixes (in which case the name's scope is relative
		 * to the current scope).
		 */
		if (*path == (u8) AML_ROOT_PREFIX) {

			/* Pathname is fully qualified, start from the root */

			this_node = acpi_gbl_root_node;
			search_parent_flag = ACPI_NS_NO_UPSEARCH;

			/* Point to name segment part */

			path++;

			ACPI_DEBUG_PRINT((ACPI_DB_NAMES,
					  "Path is absolute from root [%p]\n",
					  this_node));
		} else {
			/* Pathname is relative to current scope, start there */

			ACPI_DEBUG_PRINT((ACPI_DB_NAMES,
					  "Searching relative to prefix scope [%4.4s] (%p)\n",
					  acpi_ut_get_node_name(prefix_node),
					  prefix_node));

			/*
			 * Handle multiple Parent Prefixes (carat) by just getting
			 * the parent node for each prefix instance.
			 */
			this_node = prefix_node;
			num_carats = 0;
			while (*path == (u8) AML_PARENT_PREFIX) {

				/* Name is fully qualified, no search rules apply */

				search_parent_flag = ACPI_NS_NO_UPSEARCH;

				/*
				 * Point past this prefix to the name segment
				 * part or the next Parent Prefix
				 */
				path++;

				/* Backup to the parent node */

				num_carats++;
				this_node = this_node->parent;
				if (!this_node) {

					/* Current scope has no parent scope */

					ACPI_ERROR((AE_INFO,
						    "%s: Path has too many parent prefixes (^) "
						    "- reached beyond root node",
						    pathname));
					return_ACPI_STATUS(AE_NOT_FOUND);
				}
			}

			if (search_parent_flag == ACPI_NS_NO_UPSEARCH) {
				ACPI_DEBUG_PRINT((ACPI_DB_NAMES,
						  "Search scope is [%4.4s], path has %u carat(s)\n",
						  acpi_ut_get_node_name
						  (this_node), num_carats));
			}
		}

		/*
		 * Determine the number of ACPI name segments in this pathname.
		 *
		 * The segment part consists of either:
		 *  - A Null name segment (0)
		 *  - A dual_name_prefix followed by two 4-byte name segments
		 *  - A multi_name_prefix followed by a byte indicating the
		 *      number of segments and the segments themselves.
		 *  - A single 4-byte name segment
		 *
		 * Examine the name prefix opcode, if any, to determine the number of
		 * segments.
		 */
		switch (*path) {
		case 0:
			/*
			 * Null name after a root or parent prefixes. We already
			 * have the correct target node and there are no name segments.
			 */
			num_segments = 0;
			type = this_node->type;

			ACPI_DEBUG_PRINT((ACPI_DB_NAMES,
					  "Prefix-only Pathname (Zero name segments), Flags=%X\n",
					  flags));
			break;

		case AML_DUAL_NAME_PREFIX:

			/* More than one name_seg, search rules do not apply */

			search_parent_flag = ACPI_NS_NO_UPSEARCH;

			/* Two segments, point to first name segment */

			num_segments = 2;
			path++;

			ACPI_DEBUG_PRINT((ACPI_DB_NAMES,
					  "Dual Pathname (2 segments, Flags=%X)\n",
					  flags));
			break;

		case AML_MULTI_NAME_PREFIX_OP:

			/* More than one name_seg, search rules do not apply */

			search_parent_flag = ACPI_NS_NO_UPSEARCH;

			/* Extract segment count, point to first name segment */

			path++;
			num_segments = (u32) (u8) * path;
			path++;

			ACPI_DEBUG_PRINT((ACPI_DB_NAMES,
					  "Multi Pathname (%u Segments, Flags=%X)\n",
					  num_segments, flags));
			break;

		default:
			/*
			 * Not a Null name, no Dual or Multi prefix, hence there is
			 * only one name segment and Pathname is already pointing to it.
			 */
			num_segments = 1;

			ACPI_DEBUG_PRINT((ACPI_DB_NAMES,
					  "Simple Pathname (1 segment, Flags=%X)\n",
					  flags));
			break;
		}

		ACPI_DEBUG_EXEC(acpi_ns_print_pathname(num_segments, path));
	}

	/*
	 * Search namespace for each segment of the name. Loop through and
	 * verify (or add to the namespace) each name segment.
	 *
	 * The object type is significant only at the last name
	 * segment. (We don't care about the types along the path, only
	 * the type of the final target object.)
	 */
	this_search_type = ACPI_TYPE_ANY;
	current_node = this_node;
	while (num_segments && current_node) {
		num_segments--;
		if (!num_segments) {

			/* This is the last segment, enable typechecking */

			this_search_type = type;

			/*
			 * Only allow automatic parent search (search rules) if the caller
			 * requested it AND we have a single, non-fully-qualified name_seg
			 */
			if ((search_parent_flag != ACPI_NS_NO_UPSEARCH) &&
			    (flags & ACPI_NS_SEARCH_PARENT)) {
				local_flags |= ACPI_NS_SEARCH_PARENT;
			}

			/* Set error flag according to caller */

			if (flags & ACPI_NS_ERROR_IF_FOUND) {
				local_flags |= ACPI_NS_ERROR_IF_FOUND;
			}

			/* Set override flag according to caller */

			if (flags & ACPI_NS_OVERRIDE_IF_FOUND) {
				local_flags |= ACPI_NS_OVERRIDE_IF_FOUND;
			}
		}

		/* Extract one ACPI name from the front of the pathname */

		ACPI_MOVE_32_TO_32(&simple_name, path);

		/* Try to find the single (4 character) ACPI name */

		status =
		    acpi_ns_search_and_enter(simple_name, walk_state,
					     current_node, interpreter_mode,
					     this_search_type, local_flags,
					     &this_node);
		if (ACPI_FAILURE(status)) {
			if (status == AE_NOT_FOUND) {

				/* Name not found in ACPI namespace */

				ACPI_DEBUG_PRINT((ACPI_DB_NAMES,
						  "Name [%4.4s] not found in scope [%4.4s] %p\n",
						  (char *)&simple_name,
						  (char *)&current_node->name,
						  current_node));
			}

			*return_node = this_node;
			return_ACPI_STATUS(status);
		}

		/* More segments to follow? */

		if (num_segments > 0) {
			/*
			 * If we have an alias to an object that opens a scope (such as a
			 * device or processor), we need to dereference the alias here so
			 * that we can access any children of the original node (via the
			 * remaining segments).
			 */
			if (this_node->type == ACPI_TYPE_LOCAL_ALIAS) {
				if (!this_node->object) {
					return_ACPI_STATUS(AE_NOT_EXIST);
				}

				if (acpi_ns_opens_scope
				    (((struct acpi_namespace_node *)
				      this_node->object)->type)) {
					this_node =
					    (struct acpi_namespace_node *)
					    this_node->object;
				}
			}
		}

		/* Special handling for the last segment (num_segments == 0) */

		else {
			/*
			 * Sanity typecheck of the target object:
			 *
			 * If 1) This is the last segment (num_segments == 0)
			 *    2) And we are looking for a specific type
			 *       (Not checking for TYPE_ANY)
			 *    3) Which is not an alias
			 *    4) Which is not a local type (TYPE_SCOPE)
			 *    5) And the type of target object is known (not TYPE_ANY)
			 *    6) And target object does not match what we are looking for
			 *
			 * Then we have a type mismatch. Just warn and ignore it.
			 */
			if ((type_to_check_for != ACPI_TYPE_ANY) &&
			    (type_to_check_for != ACPI_TYPE_LOCAL_ALIAS) &&
			    (type_to_check_for != ACPI_TYPE_LOCAL_METHOD_ALIAS)
			    && (type_to_check_for != ACPI_TYPE_LOCAL_SCOPE)
			    && (this_node->type != ACPI_TYPE_ANY)
			    && (this_node->type != type_to_check_for)) {

				/* Complain about a type mismatch */

				ACPI_WARNING((AE_INFO,
					      "NsLookup: Type mismatch on %4.4s (%s), searching for (%s)",
					      ACPI_CAST_PTR(char, &simple_name),
					      acpi_ut_get_type_name(this_node->
								    type),
					      acpi_ut_get_type_name
					      (type_to_check_for)));
			}

			/*
			 * If this is the last name segment and we are not looking for a
			 * specific type, but the type of found object is known, use that
			 * type to (later) see if it opens a scope.
			 */
			if (type == ACPI_TYPE_ANY) {
				type = this_node->type;
			}
		}

		/* Point to next name segment and make this node current */

		path += ACPI_NAME_SIZE;
		current_node = this_node;
	}

	/* Always check if we need to open a new scope */

	if (!(flags & ACPI_NS_DONT_OPEN_SCOPE) && (walk_state)) {
		/*
		 * If entry is a type which opens a scope, push the new scope on the
		 * scope stack.
		 */
		if (acpi_ns_opens_scope(type)) {
			status =
			    acpi_ds_scope_stack_push(this_node, type,
						     walk_state);
			if (ACPI_FAILURE(status)) {
				return_ACPI_STATUS(status);
			}
		}
	}

	*return_node = this_node;
	return_ACPI_STATUS(AE_OK);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               /*******************************************************************************
 *
 * Module Name: nsalloc - Namespace allocation and deletion utilities
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
ACPI_MODULE_NAME("nsalloc")

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_create_node
 *
 * PARAMETERS:  name            - Name of the new node (4 char ACPI name)
 *
 * RETURN:      New namespace node (Null on failure)
 *
 * DESCRIPTION: Create a namespace node
 *
 ******************************************************************************/
struct acpi_namespace_node *acpi_ns_create_node(u32 name)
{
	struct acpi_namespace_node *node;
#ifdef ACPI_DBG_TRACK_ALLOCATIONS
	u32 temp;
#endif

	ACPI_FUNCTION_TRACE(ns_create_node);

	node = acpi_os_acquire_object(acpi_gbl_namespace_cache);
	if (!node) {
		return_PTR(NULL);
	}

	ACPI_MEM_TRACKING(acpi_gbl_ns_node_list->total_allocated++);

#ifdef ACPI_DBG_TRACK_ALLOCATIONS
	temp = acpi_gbl_ns_node_list->total_allocated -
	    acpi_gbl_ns_node_list->total_freed;
	if (temp > acpi_gbl_ns_node_list->max_occupied) {
		acpi_gbl_ns_node_list->max_occupied = temp;
	}
#endif

	node->name.integer = name;
	ACPI_SET_DESCRIPTOR_TYPE(node, ACPI_DESC_TYPE_NAMED);
	return_PTR(node);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_delete_node
 *
 * PARAMETERS:  node            - Node to be deleted
 *
 * RETURN:      None
 *
 * DESCRIPTION: Delete a namespace node. All node deletions must come through
 *              here. Detaches any attached objects, including any attached
 *              data. If a handler is associated with attached data, it is
 *              invoked before the node is deleted.
 *
 ******************************************************************************/

void acpi_ns_delete_node(struct acpi_namespace_node *node)
{
	union acpi_operand_object *obj_desc;
	union acpi_operand_object *next_desc;

	ACPI_FUNCTION_NAME(ns_delete_node);

	/* Detach an object if there is one */

	acpi_ns_detach_object(node);

	/*
	 * Delete an attached data object list if present (objects that were
	 * attached via acpi_attach_data). Note: After any normal object is
	 * detached above, the only possible remaining object(s) are data
	 * objects, in a linked list.
	 */
	obj_desc = node->object;
	while (obj_desc && (obj_desc->common.type == ACPI_TYPE_LOCAL_DATA)) {

		/* Invoke the attached data deletion handler if present */

		if (obj_desc->data.handler) {
			obj_desc->data.handler(node, obj_desc->data.pointer);
		}

		next_desc = obj_desc->common.next_object;
		acpi_ut_remove_reference(obj_desc);
		obj_desc = next_desc;
	}

	/* Special case for the statically allocated root node */

	if (node == acpi_gbl_root_node) {
		return;
	}

	/* Now we can delete the node */

	(void)acpi_os_release_object(acpi_gbl_namespace_cache, node);

	ACPI_MEM_TRACKING(acpi_gbl_ns_node_list->total_freed++);
	ACPI_DEBUG_PRINT((ACPI_DB_ALLOCATIONS, "Node %p, Remaining %X\n",
			  node, acpi_gbl_current_node_count));
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_remove_node
 *
 * PARAMETERS:  node            - Node to be removed/deleted
 *
 * RETURN:      None
 *
 * DESCRIPTION: Remove (unlink) and delete a namespace node
 *
 ******************************************************************************/

void acpi_ns_remove_node(struct acpi_namespace_node *node)
{
	struct acpi_namespace_node *parent_node;
	struct acpi_namespace_node *prev_node;
	struct acpi_namespace_node *next_node;

	ACPI_FUNCTION_TRACE_PTR(ns_remove_node, node);

	parent_node = node->parent;

	prev_node = NULL;
	next_node = parent_node->child;

	/* Find the node that is the previous peer in the parent's child list */

	while (next_node != node) {
		prev_node = next_node;
		next_node = next_node->peer;
	}

	if (prev_node) {

		/* Node is not first child, unlink it */

		prev_node->peer = node->peer;
	} else {
		/*
		 * Node is first child (has no previous peer).
		 * Link peer list to parent
		 */
		parent_node->child = node->peer;
	}

	/* Delete the node and any attached objects */

	acpi_ns_delete_node(node);
	return_VOID;
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_install_node
 *
 * PARAMETERS:  walk_state      - Current state of the walk
 *              parent_node     - The parent of the new Node
 *              node            - The new Node to install
 *              type            - ACPI object type of the new Node
 *
 * RETURN:      None
 *
 * DESCRIPTION: Initialize a new namespace node and install it amongst
 *              its peers.
 *
 *              Note: Current namespace lookup is linear search. This appears
 *              to be sufficient as namespace searches consume only a small
 *              fraction of the execution time of the ACPI subsystem.
 *
 ******************************************************************************/

void acpi_ns_install_node(struct acpi_walk_state *walk_state, struct acpi_namespace_node *parent_node,	/* Parent */
			  struct acpi_namespace_node *node,	/* New Child */
			  acpi_object_type type)
{
	acpi_owner_id owner_id = 0;
	struct acpi_namespace_node *child_node;

	ACPI_FUNCTION_TRACE(ns_install_node);

	if (walk_state) {
		/*
		 * Get the owner ID from the Walk state. The owner ID is used to
		 * track table deletion and deletion of objects created by methods.
		 */
		owner_id = walk_state->owner_id;

		if ((walk_state->method_desc) &&
		    (parent_node != walk_state->method_node)) {
			/*
			 * A method is creating a new node that is not a child of the
			 * method (it is non-local). Mark the executing method as having
			 * modified the namespace. This is used for cleanup when the
			 * method exits.
			 */
			walk_state->method_desc->method.info_flags |=
			    ACPI_METHOD_MODIFIED_NAMESPACE;
		}
	}

	/* Link the new entry into the parent and existing children */

	node->peer = NULL;
	node->parent = parent_node;
	child_node = parent_node->child;

	if (!child_node) {
		parent_node->child = node;
	} else {
		/* Add node to the end of the peer list */

		while (child_node->peer) {
			child_node = child_node->peer;
		}

		child_node->peer = node;
	}

	/* Init the new entry */

	node->owner_id = owner_id;
	node->type = (u8) type;

	ACPI_DEBUG_PRINT((ACPI_DB_NAMES,
			  "%4.4s (%s) [Node %p Owner %X] added to %4.4s (%s) [Node %p]\n",
			  acpi_ut_get_node_name(node),
			  acpi_ut_get_type_name(node->type), node, owner_id,
			  acpi_ut_get_node_name(parent_node),
			  acpi_ut_get_type_name(parent_node->type),
			  parent_node));

	return_VOID;
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_delete_children
 *
 * PARAMETERS:  parent_node     - Delete this objects children
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Delete all children of the parent object. In other words,
 *              deletes a "scope".
 *
 ******************************************************************************/

void acpi_ns_delete_children(struct acpi_namespace_node *parent_node)
{
	struct acpi_namespace_node *next_node;
	struct acpi_namespace_node *node_to_delete;

	ACPI_FUNCTION_TRACE_PTR(ns_delete_children, parent_node);

	if (!parent_node) {
		return_VOID;
	}

	/* Deallocate all children at this level */

	next_node = parent_node->child;
	while (next_node) {

		/* Grandchildren should have all been deleted already */

		if (next_node->child) {
			ACPI_ERROR((AE_INFO, "Found a grandchild! P=%p C=%p",
				    parent_node, next_node));
		}

		/*
		 * Delete this child node and move on to the next child in the list.
		 * No need to unlink the node since we are deleting the entire branch.
		 */
		node_to_delete = next_node;
		next_node = next_node->peer;
		acpi_ns_delete_node(node_to_delete);
	};

	/* Clear the parent's child pointer */

	parent_node->child = NULL;
	return_VOID;
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_delete_namespace_subtree
 *
 * PARAMETERS:  parent_node     - Root of the subtree to be deleted
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Delete a subtree of the namespace. This includes all objects
 *              stored within the subtree.
 *
 ******************************************************************************/

void acpi_ns_delete_namespace_subtree(struct acpi_namespace_node *parent_node)
{
	struct acpi_namespace_node *child_node = NULL;
	u32 level = 1;
	acpi_status status;

	ACPI_FUNCTION_TRACE(ns_delete_namespace_subtree);

	if (!parent_node) {
		return_VOID;
	}

	/* Lock namespace for possible update */

	status = acpi_ut_acquire_mutex(ACPI_MTX_NAMESPACE);
	if (ACPI_FAILURE(status)) {
		return_VOID;
	}

	/*
	 * Traverse the tree of objects until we bubble back up
	 * to where we started.
	 */
	while (level > 0) {

		/* Get the next node in this scope (NULL if none) */

		child_node = acpi_ns_get_next_node(parent_node, child_node);
		if (child_node) {

			/* Found a child node - detach any attached object */

			acpi_ns_detach_object(child_node);

			/* Check if this node has any children */

			if (child_node->child) {
				/*
				 * There is at least one child of this node,
				 * visit the node
				 */
				level++;
				parent_node = child_node;
				child_node = NULL;
			}
		} else {
			/*
			 * No more children of this parent node.
			 * Move up to the grandparent.
			 */
			level--;

			/*
			 * Now delete all of the children of this parent
			 * all at the same time.
			 */
			acpi_ns_delete_children(parent_node);

			/* New "last child" is this parent node */

			child_node = parent_node;

			/* Move up the tree to the grandparent */

			parent_node = parent_node->parent;
		}
	}

	(void)acpi_ut_release_mutex(ACPI_MTX_NAMESPACE);
	return_VOID;
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_ns_delete_namespace_by_owner
 *
 * PARAMETERS:  owner_id    - All nodes with this owner will be deleted
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Delete entries within the namespace that are owned by a
 *              specific ID. Used to delete entire ACPI tables. All
 *              reference counts are updated.
 *
 * MUTEX:       Locks namespace during deletion walk.
 *
 ******************************************************************************/

void acpi_ns_delete_namespace_by_owner(acpi_owner_id owner_id)
{
	struct acpi_namespace_node *child_node;
	struct acpi_namespace_node *deletion_node;
	struct acpi_namespace_node *parent_node;
	u32 level;
	acpi_status status;

	ACPI_FUNCTION_TRACE_U32(ns_delete_namespace_by_owner, owner_id);

	if (owner_id == 0) {
		return_VOID;
	}

	/* Lock namespace for possi