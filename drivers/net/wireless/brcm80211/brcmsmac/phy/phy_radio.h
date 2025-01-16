= gpe_event_info->register_info;
	if (!gpe_register_info) {
		return (AE_NOT_EXIST);
	}

	/* Get current value of the enable register that contains this GPE */

	status = acpi_hw_read(&enable_mask, &gpe_register_info->enable_address);
	if (ACPI_FAILURE(status)) {
		return (status);
	}

	/* Set or clear just the bit that corresponds to this GPE */

	register_bit = acpi_hw_get_gpe_register_bit(gpe_event_info);
	switch (action) {
	case ACPI_GPE_CONDITIONAL_ENABLE:

		/* Only enable if the corresponding enable_mask bit is set */

		if (!(register_bit & gpe_register_info->enable_mask)) {
			return (AE_BAD_PARAMETER);
		}

		/*lint -fallthrough */

	case ACPI_GPE_ENABLE:

		ACPI_SET_BIT(enable_mask, register_bit);
		break;

	case ACPI_GPE_DISABLE:

		ACPI_CLEAR_BIT(enable_mask, register_bit);
		break;

	default:

		ACPI_ERROR((AE_INFO, "Invalid GPE Action, %u", action));
		return (AE_BAD_PARAMETER);
	}

	/* Write the updated enable mask */

	status = acpi_hw_write(enable_mask, &gpe_register_info->enable_address);
	return (status);
}

/******************************************************************************
 *
 * FUNCTION:    acpi_hw_clear_gpe
 *
 * PARAMETERS:  gpe_event_info      - Info block for the GPE to be cleared
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Clear the status bit for a single GPE.
 *
 ******************************************************************************/

acpi_status acpi_hw_clear_gpe(struct acpi_gpe_event_info * gpe_event_info)
{
	struct acpi_gpe_register_info *gpe_register_info;
	acpi_status status;
	u32 register_bit;

	ACPI_FUNCTION_ENTRY();

	/* Get the info block for the entire GPE register */

	gpe_register_info = gpe_event_info->register_info;
	if (!gpe_register_info) {
		return (AE_NOT_EXIST);
	}

	/*
	 * Write a one to the appropriate bit in the status register to
	 * clear this GPE.
	 */
	register_bit = acpi_hw_get_gpe_register_bit(gpe_event_info);

	status = acpi_hw_write(register_bit,
			       &gpe_register_info->status_address);

	return (status);
}

/******************************************************************************
 *
 * FUNCTION:    acpi_hw_get_gpe_status
 *
 * PARAMETERS:  gpe_event_info      - Info block for the GPE to queried
 *              event_status        - Where the GPE status is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Return the status of a single GPE.
 *
 ******************************************************************************/

acpi_status
acpi_hw_get_gpe_status(struct acpi_gpe_event_info * gpe_event_info,
		       acpi_event_status *event_status)
{
	u32 in_byte;
	u32 register_bit;
	struct acpi_gpe_register_info *gpe_register_info;
	acpi_event_status local_event_status = 0;
	acpi_status status;

	ACPI_FUNCTION_ENTRY();

	if (!event_status) {
		return (AE_BAD_PARAMETER);
	}

	/* GPE currently handled? */

	if (ACPI_GPE_DISPATCH_TYPE(gpe_event_info->flags) !=
	    ACPI_GPE_DISPATCH_NONE) {
		local_event_status |= ACPI_EVENT_FLAG_HAS_HANDLER;
	}

	/* Get the info block for the entire GPE register */

	gpe_register_info = gpe_event_info->register_info;

	/* Get the register bitmask for this GPE */

	register_bit = acpi_hw_get_gpe_register_bit(gpe_event_info);

	/* GPE currently enabled? (enabled for runtime?) */

	if (register_bit & gpe_register_info->enable_for_run) {
		local_event_status |= ACPI_EVENT_FLAG_ENABLED;
	}

	/* GPE enabled for wake? */

	if (register_bit & gpe_register_info->enable_for_wake) {
		local_event_status |= ACPI_EVENT_FLAG_WAKE_ENABLED;
	}

	/* GPE currently enabled (enable bit == 1)? */

	status = acpi_hw_read(&in_byte, &gpe_register_info->enable_address);
	if (ACPI_FAILURE(status)) {
		return (status);
	}

	if (register_bit & in_byte) {
		local_event_status |= ACPI_EVENT_FLAG_ENABLE_SET;
	}

	/* GPE currently active (status bit == 1)? */

	status = acpi_hw_read(&in_byte, &gpe_register_info->status_address);
	if (ACPI_FAILURE(status)) {
		return (status);
	}

	if (register_bit & in_byte) {
		local_event_status |= ACPI_EVENT_FLAG_STATUS_SET;
	}

	/* Set return value */

	(*event_status) = local_event_status;
	return (AE_OK);
}

/******************************************************************************
 *
 * FUNCTION:    acpi_hw_gpe_enable_write
 *
 * PARAMETERS:  enable_mask         - Bit mask to write to the GPE register
 *              gpe_register_info   - Gpe Register info
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Write the enable mask byte to the given GPE register.
 *
 ******************************************************************************/

static acpi_status
acpi_hw_gpe_enable_write(u8 enable_mask,
			 struct acpi_gpe_register_info *gpe_register_info)
{
	acpi_status status;

	gpe_register_info->enable_mask = enable_mask;
	status = acpi_hw_write(enable_mask, &gpe_register_info->enable_address);
	return (status);
}

/******************************************************************************
 *
 * FUNCTION:    acpi_hw_disable_gpe_block
 *
 * PARAMETERS:  gpe_xrupt_info      - GPE Interrupt info
 *              gpe_block           - Gpe Block info
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Disable all GPEs within a single GPE block
 *
 ******************************************************************************/

acpi_status
acpi_hw_disable_gpe_block(struct acpi_gpe_xrupt_info *gpe_xrupt_info,
			  struct acpi_gpe_block_info *gpe_block, void *context)
{
	u32 i;
	acpi_status status;

	/* Examine each GPE Register within the block */

	for (i = 0; i < gpe_block->register_count; i++) {

		/* Disable all GPEs in this register */

		status =
		    acpi_hw_gpe_enable_write(0x00,
					     &gpe_block->register_info[i]);
		if (ACPI_FAILURE(status)) {
			return (status);
		}
	}

	return (AE_OK);
}

/******************************************************************************
 *
 * FUNCTION:    acpi_hw_clear_gpe_block
 *
 * PARAMETERS:  gpe_xrupt_info      - GPE Interrupt info
 *              gpe_block           - Gpe Block info
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Clear status bits for all GPEs within a single GPE block
 *
 ******************************************************************************/

acpi_status
acpi_hw_clear_gpe_block(struct acpi_gpe_xrupt_info *gpe_xrupt_info,
			struct acpi_gpe_block_info *gpe_block, void *context)
{
	u32 i;
	acpi_status status;

	/* Examine each GPE Register within the block */

	for (i = 0; i < gpe_block->register_count; i++) {

		/* Clear status on all GPEs in this register */

		status =
		    acpi_hw_write(0xFF,
				  &gpe_block->register_info[i].status_address);
		if (ACPI_FAILURE(status)) {
			return (status);
		}
	}

	return (AE_OK);
}

/******************************************************************************
 *
 * FUNCTION:    acpi_hw_enable_runtime_gpe_block
 *
 * PARAMETERS:  gpe_xrupt_info      - GPE Interrupt info
 *              gpe_block           - Gpe Block info
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Enable all "runtime" GPEs within a single GPE block. Includes
 *              combination wake/run GPEs.
 *
 ******************************************************************************/

acpi_status
acpi_hw_enable_runtime_gpe_block(struct acpi_gpe_xrupt_info *gpe_xrupt_info,
				 struct acpi_gpe_block_info * gpe_block,
				 void *context)
{
	u32 i;
	acpi_status status;
	struct acpi_gpe_register_info *gpe_register_info;

	/* NOTE: assumes that all GPEs are currently disabled */

	/* Examine each GPE Register within the block */

	for (i = 0; i < gpe_block->register_count; i++) {
		gpe_register_info = &gpe_block->register_info[i];
		if (!gpe_register_info->enable_for_run) {
			continue;
		}

		/* Enable all "runtime" GPEs in this register */

		status =
		    acpi_hw_gpe_enable_write(gpe_register_info->enable_for_run,
					     gpe_register_info);
		if (ACPI_FAILURE(status)) {
			return (status);
		}
	}

	return (AE_OK);
}

/******************************************************************************
 *
 * FUNCTION:    acpi_hw_enable_wakeup_gpe_block
 *
 * PARAMETERS:  gpe_xrupt_info      - GPE Interrupt info
 *              gpe_block           - Gpe Block info
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Enable all "wake" GPEs within a single GPE block. Includes
 *              combination wake/run GPEs.
 *
 ******************************************************************************/

static acpi_status
acpi_hw_enable_wakeup_gpe_block(struct acpi_gpe_xrupt_info *gpe_xrupt_info,
				struct acpi_gpe_block_info *gpe_block,
				void *context)
{
	u32 i;
	acpi_status status;
	struct acpi_gpe_register_info *gpe_register_info;

	/* Examine each GPE Register within the block */

	for (i = 0; i < gpe_block->register_count; i++) {
		gpe_register_info = &gpe_block->register_info[i];

		/*
		 * Enable all "wake" GPEs in this register and disable the
		 * remaining ones.
		 */

		status =
		    acpi_hw_gpe_enable_write(gpe_register_info->enable_for_wake,
					     gpe_register_info);
		if (ACPI_FAILURE(status)) {
			return (status);
		}
	}

	return (AE_OK);
}

/******************************************************************************
 *
 * FUNCTION:    acpi_hw_disable_all_gpes
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Disable and clear all GPEs in all GPE blocks
 *
 ******************************************************************************/

acpi_status acpi_hw_disable_all_gpes(void)
{
	acpi_status status;

	ACPI_FUNCTION_TRACE(hw_disable_all_gpes);

	status = acpi_ev_walk_gpe_list(acpi_hw_disable_gpe_block, NULL);
	status = acpi_ev_walk_gpe_list(acpi_hw_clear_gpe_block, NULL);
	return_ACPI_STATUS(status);
}

/******************************************************************************
 *
 * FUNCTION:    acpi_hw_enable_all_runtime_gpes
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Enable all "runtime" GPEs, in all GPE blocks
 *
 ******************************************************************************/

acpi_status acpi_hw_enable_all_runtime_gpes(void)
{
	acpi_status status;

	ACPI_FUNCTION_TRACE(hw_enable_all_runtime_gpes);

	status = acpi_ev_walk_gpe_list(acpi_hw_enable_runtime_gpe_block, NULL);
	return_ACPI_STATUS(status);
}

/******************************************************************************
 *
 * FUNCTION:    acpi_hw_enable_all_wakeup_gpes
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Enable all "wakeup" GPEs, in all GPE blocks
 *
 ******************************************************************************/

acpi_status acpi_hw_enable_all_wakeup_gpes(void)
{
	acpi_status status;

	ACPI_FUNCTION_TRACE(hw_enable_all_wakeup_gpes);

	status = acpi_ev_walk_gpe_list(acpi_hw_enable_wakeup_gpe_block, NULL);
	return_ACPI_STATUS(status);
}

#endif				/* !ACPI_REDUCED_HARDWARE */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       /*******************************************************************************
 *
 * Module Name: hwpci - Obtain PCI bus, device, and function numbers
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

#define _COMPONENT          ACPI_NAMESPACE
ACPI_MODULE_NAME("hwpci")

/* PCI configuration space values */
#define PCI_CFG_HEADER_TYPE_REG             0x0E
#define PCI_CFG_PRIMARY_BUS_NUMBER_REG      0x18
#define PCI_CFG_SECONDARY_BUS_NUMBER_REG    0x19
/* PCI header values */
#define PCI_HEADER_TYPE_MASK                0x7F
#define PCI_TYPE_BRIDGE                     0x01
#define PCI_TYPE_CARDBUS_BRIDGE             0x02
typedef struct acpi_pci_device {
	acpi_handle device;
	struct acpi_pci_device *next;

} acpi_pci_device;

/* Local prototypes */

static acpi_status
acpi_hw_build_pci_list(acpi_handle root_pci_device,
		       acpi_handle pci_region,
		       struct acpi_pci_device **return_list_head);

static acpi_status
acpi_hw_process_pci_list(struct acpi_pci_id *pci_id,
			 struct acpi_pci_device *list_head);

static void acpi_hw_delete_pci_list(struct acpi_pci_device *list_head);

static acpi_status
acpi_hw_get_pci_device_info(struct acpi_pci_id *pci_id,
			    acpi_handle pci_device,
			    u16 *bus_number, u8 *is_bridge);

/*******************************************************************************
 *
 * FUNCTION:    acpi_hw_derive_pci_id
 *
 * PARAMETERS:  pci_id              - Initial values for the PCI ID. May be
 *                                    modified by this function.
 *              root_pci_device     - A handle to a PCI device object. This
 *                                    object must be a PCI Root Bridge having a
 *                                    _HID value of either PNP0A03 or PNP0A08
 *              pci_region          - A handle to a PCI configuration space
 *                                    Operation Region being initialized
 *
 * RETURN:      Status
 *
 * DESCRIPTION: This function derives a full PCI ID for a PCI device,
 *              consisting of a Segment number, Bus number, Device number,
 *              and function code.
 *
 *              The PCI hardware dynamically configures PCI bus numbers
 *              depending on the bus topology discovered during system
 *              initialization. This function is invoked during configuration
 *              of a PCI_Config Operation Region in order to (possibly) update
 *              the Bus/Device/Function numbers in the pci_id with the actual
 *              values as determined by the hardware and operating system
 *              configuration.
 *
 *              The pci_id parameter is initially populated during the Operation
 *              Region initialization. This function is then called, and is
 *              will make any necessary modifications to the Bus, Device, or
 *              Function number PCI ID subfields as appropriate for the
 *              current hardware and OS configuration.
 *
 * NOTE:        Created 08/2010. Replaces the previous OSL acpi_os_derive_pci_id
 *              interface since this feature is OS-independent. This module
 *              specifically avoids any use of recursion by building a local
 *              temporary device list.
 *
 ******************************************************************************/

acpi_status
acpi_hw_derive_pci_id(struct acpi_pci_id *pci_id,
		      acpi_handle root_pci_device, acpi_handle pci_region)
{
	acpi_status status;
	struct acpi_pci_device *list_head;

	ACPI_FUNCTION_TRACE(hw_derive_pci_id);

	if (!pci_id) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	/* Build a list of PCI devices, from pci_region up to root_pci_device */

	status =
	    acpi_hw_build_pci_list(root_pci_device, pci_region, &list_head);
	if (ACPI_SUCCESS(status)) {

		/* Walk the list, updating the PCI device/function/bus numbers */

		status = acpi_hw_process_pci_list(pci_id, list_head);

		/* Delete the list */

		acpi_hw_delete_pci_list(list_head);
	}

	return_ACPI_STATUS(status);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_hw_build_pci_list
 *
 * PARAMETERS:  root_pci_device     - A handle to a PCI device object. This
 *                                    object is guaranteed to be a PCI Root
 *                                    Bridge having a _HID value of either
 *                                    PNP0A03 or PNP0A08
 *              pci_region          - A handle to the PCI configuration space
 *                                    Operation Region
 *              return_list_head    - Where the PCI device list is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Builds a list of devices from the input PCI region up to the
 *              Root PCI device for this namespace subtree.
 *
 ******************************************************************************/

static acpi_status
acpi_hw_build_pci_list(acpi_handle root_pci_device,
		       acpi_handle pci_region,
		       struct acpi_pci_device **return_list_head)
{
	acpi_handle current_device;
	acpi_handle parent_device;
	acpi_status status;
	struct acpi_pci_device *list_element;

	/*
	 * Ascend namespace branch until the root_pci_device is reached, building
	 * a list of device nodes. Loop will exit when either the PCI device is
	 * found, or the root of the namespace is reached.
	 */
	*return_list_head = NULL;
	current_device = pci_region;
	while (1) {
		status = acpi_get_parent(current_device, &parent_device);
		if (ACPI_FAILURE(status)) {

			/* Must delete the list before exit */

			acpi_hw_delete_pci_list(*return_list_head);
			return (status);
		}

		/* Finished when we reach the PCI root device (PNP0A03 or PNP0A08) */

		if (parent_device == root_pci_device) {
			return (AE_OK);
		}

		list_element = ACPI_ALLOCATE(sizeof(struct acpi_pci_device));
		if (!list_element) {

			/* Must delete the list before exit */

			acpi_hw_delete_pci_list(*return_list_head);
			return (AE_NO_MEMORY);
		}

		/* Put new element at the head of the list */

		list_element->next = *return_list_head;
		list_element->device = parent_device;
		*return_list_head = list_element;

		current_device = parent_device;
	}
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_hw_process_pci_list
 *
 * PARAMETERS:  pci_id              - Initial values for the PCI ID. May be
 *                                    modified by this function.
 *              list_head           - Device list created by
 *                                    acpi_hw_build_pci_list
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Walk downward through the PCI device list, getting the device
 *              info for each, via the PCI configuration space and updating
 *              the PCI ID as necessary. Deletes the list during traversal.
 *
 ******************************************************************************/

static acpi_status
acpi_hw_process_pci_list(struct acpi_pci_id *pci_id,
			 struct acpi_pci_device *list_head)
{
	acpi_status status = AE_OK;
	struct acpi_pci_device *info;
	u16 bus_number;
	u8 is_bridge = TRUE;

	ACPI_FUNCTION_NAME(hw_process_pci_list);

	ACPI_DEBUG_PRINT((ACPI_DB_OPREGION,
			  "Input PciId:  Seg %4.4X Bus %4.4X Dev %4.4X Func %4.4X\n",
			  pci_id->segment, pci_id->bus, pci_id->device,
			  pci_id->function));

	bus_number = pci_id->bus;

	/*
	 * Descend down the namespace tree, collecting PCI device, function,
	 * and bus numbers. bus_number is only important for PCI bridges.
	 * Algorithm: As we descend the tree, use the last valid PCI device,
	 * function, and bus numbers that are discovered, and assign them
	 * to the PCI ID for the target device.
	 */
	info = list_head;
	while (info) {
		status = acpi_hw_get_pci_device_info(pci_id, info->device,
						     &bus_number, &is_bridge);
		if (ACPI_FAILURE(status)) {
			return (status);
		}

		info = info->next;
	}

	ACPI_DEBUG_PRINT((ACPI_DB_OPREGION,
			  "Output PciId: Seg %4.4X Bus %4.4X Dev %4.4X Func %4.4X "
			  "Status %X BusNumber %X IsBridge %X\n",
			  pci_id->segment, pci_id->bus, pci_id->device,
			  pci_id->function, status, bus_number, is_bridge));

	return (AE_OK);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_hw_delete_pci_list
 *
 * PARAMETERS:  list_head           - Device list created by
 *                                    acpi_hw_build_pci_list
 *
 * RETURN:      None
 *
 * DESCRIPTION: Free the entire PCI list.
 *
 ******************************************************************************/

static void acpi_hw_delete_pci_list(struct acpi_pci_device *list_head)
{
	struct acpi_pci_device *next;
	struct acpi_pci_device *previous;

	next = list_head;
	while (next) {
		previous = next;
		next = previous->next;
		ACPI_FREE(previous);
	}
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_hw_get_pci_device_info
 *
 * PARAMETERS:  pci_id              - Initial values for the PCI ID. May be
 *                                    modified by this function.
 *              pci_device          - Handle for the PCI device object
 *              bus_number          - Where a PCI bridge bus number is returned
 *              is_bridge           - Return value, indicates if this PCI
 *                                    device is a PCI bridge
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Get the device info for a single PCI device object. Get the
 *              _ADR (contains PCI device and function numbers), and for PCI
 *              bridge devices, get the bus number from PCI configuration
 *              space.
 *
 ******************************************************************************/

static acpi_status
acpi_hw_get_pci_device_info(struct acpi_pci_id *pci_id,
			    acpi_handle pci_device,
			    u16 *bus_number, u8 *is_bridge)
{
	acpi_status status;
	acpi_object_type object_type;
	u64 return_value;
	u64 pci_value;

	/* We only care about objects of type Device */

	status = acpi_get_type(pci_device, &object_type);
	if (ACPI_FAILURE(status)) {
		return (status);
	}

	if (object_type != ACPI_TYPE_DEVICE) {
		return (AE_OK);
	}

	/* We need an _ADR. Ignore device if not present */

	status = acpi_ut_evaluate_numeric_object(METHOD_NAME__ADR,
						 pci_device, &return_value);
	if (ACPI_FAILURE(status)) {
		return (AE_OK);
	}

	/*
	 * From _ADR, get the PCI Device and Function and
	 * update the PCI ID.
	 */
	pci_id->device = ACPI_HIWORD(ACPI_LODWORD(return_value));
	pci_id->function = ACPI_LOWORD(ACPI_LODWORD(return_value));

	/*
	 * If the previous device was a bridge, use the previous
	 * device bus number
	 */
	if (*is_bridge) {
		pci_id->bus = *bus_number;
	}

	/*
	 * Get the bus numbers from PCI Config space:
	 *
	 * First, get the PCI header_type
	 */
	*is_bridge = FALSE;
	status = acpi_os_read_pci_configuration(pci_id,
						PCI_CFG_HEADER_TYPE_REG,
						&pci_value, 8);
	if (ACPI_FAILURE(status)) {
		return (status);
	}

	/* We only care about bridges (1=pci_bridge, 2=card_bus_bridge) */

	pci_value &= PCI_HEADER_TYPE_MASK;

	if ((pci_value != PCI_TYPE_BRIDGE) &&
	    (pci_value != PCI_TYPE_CARDBUS_BRIDGE)) {
		return (AE_OK);
	}

	/* Bridge: Get the Primary bus_number */

	status = acpi_os_read_pci_configuration(pci_id,
						PCI_CFG_PRIMARY_BUS_NUMBER_REG,
						&pci_value, 8);
	if (ACPI_FAILURE(status)) {
		return (status);
	}

	*is_bridge = TRUE;
	pci_id->bus = (u16)pci_value;

	/* Bridge: Get the Secondary bus_number */

	status = acpi_os_read_pci_configuration(pci_id,
						PCI_CFG_SECONDARY_BUS_NUMBER_REG,
						&pci_value, 8);
	if (ACPI_FAILURE(status)) {
		return (status);
	}

	*bus_number = (u16)pci_value;
	return (AE_OK);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          /*******************************************************************************
 *
 * Module Name: hwregs - Read/write access functions for the various ACPI
 *                       control and status registers.
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
#include "acevents.h"

#define _COMPONENT          ACPI_HARDWARE
ACPI_MODULE_NAME("hwregs")

#if (!ACPI_REDUCED_HARDWARE)
/* Local Prototypes */
static acpi_status
acpi_hw_read_multiple(u32 *value,
		      struct acpi_generic_address *register_a,
		      struct acpi_generic_address *register_b);

static acpi_status
acpi_hw_write_multiple(u32 value,
		       struct acpi_generic_address *register_a,
		       struct acpi_generic_address *register_b);

#endif				/* !ACPI_REDUCED_HARDWARE */

/******************************************************************************
 *
 * FUNCTION:    acpi_hw_validate_register
 *
 * PARAMETERS:  reg                 - GAS register structure
 *              max_bit_width       - Max bit_width supported (32 or 64)
 *              address             - Pointer to where the gas->address
 *                                    is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Validate the contents of a GAS register. Checks the GAS
 *              pointer, Address, space_id, bit_width, and bit_offset.
 *
 ******************************************************************************/

acpi_status
acpi_hw_validate_register(struct acpi_generic_address *reg,
			  u8 max_bit_width, u64 *address)
{

	/* Must have a valid pointer to a GAS structure */

	if (!reg) {
		return (AE_BAD_PARAMETER);
	}

	/*
	 * Copy the target address. This handles possible alignment issues.
	 * Address must not be null. A null address also indicates an optional
	 * ACPI register that is not supported, so no error message.
	 */
	ACPI_MOVE_64_TO_64(address, &reg->address);
	if (!(*address)) {
		return (AE_BAD_ADDRESS);
	}

	/* Validate the space_ID */

	if ((reg->space_id != ACPI_ADR_SPACE_SYSTEM_MEMORY) &&
	    (reg->space_id != ACPI_ADR_SPACE_SYSTEM_IO)) {
		ACPI_ERROR((AE_INFO,
			    "Unsupported address space: 0x%X", reg->space_id));
		return (AE_SUPPORT);
	}

	/* Validate the bit_width */

	if ((reg->bit_width != 8) &&
	    (reg->bit_width != 16) &&
	    (reg->bit_width != 32) && (reg->bit_width != max_bit_width)) {
		ACPI_ERROR((AE_INFO,
			    "Unsupported register bit width: 0x%X",
			    reg->bit_width));
		return (AE_SUPPORT);
	}

	/* Validate the bit_offset. Just a warning for now. */

	if (reg->bit_offset != 0) {
		ACPI_WARNING((AE_INFO,
			      "Unsupported register bit offset: 0x%X",
			      reg->bit_offset));
	}

	return (AE_OK);
}

/******************************************************************************
 *
 * FUNCTION:    acpi_hw_read
 *
 * PARAMETERS:  value               - Where the value is returned
 *              reg                 - GAS register structure
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Read from either memory or IO space. This is a 32-bit max
 *              version of acpi_read, used internally since the overhead of
 *              64-bit values is not needed.
 *
 * LIMITATIONS: <These limitations also apply to acpi_hw_write>
 *      bit_width must be exactly 8, 16, or 32.
 *      space_ID must be system_memory or system_IO.
 *      bit_offset and access_width are currently ignored, as there has
 *          not been a need to implement these.
 *
 ******************************************************************************/

acpi_status acpi_hw_read(u32 *value, struct acpi_generic_address *reg)
{
	u64 address;
	u64 value64;
	acpi_status status;

	ACPI_FUNCTION_NAME(hw_read);

	/* Validate contents of the GAS register */

	status = acpi_hw_validate_register(reg, 32, &address);
	if (ACPI_FAILURE(status)) {
		return (status);
	}

	/* Initialize entire 32-bit return value to zero */

	*value = 0;

	/*
	 * Two address spaces supported: Memory or IO. PCI_Config is
	 * not supported here because the GAS structure is insufficient
	 */
	if (reg->space_id == ACPI_ADR_SPACE_SYSTEM_MEMORY) {
		status = acpi_os_read_memory((acpi_physical_address)
					     address, &value64, reg->bit_width);

		*value = (u32)value64;
	} else {		/* ACPI_ADR_SPACE_SYSTEM_IO, validated earlier */

		status = acpi_hw_read_port((acpi_io_address)
					   address, value, reg->bit_width);
	}

	ACPI_DEBUG_PRINT((ACPI_DB_IO,
			  "Read:  %8.8X width %2d from %8.8X%8.8X (%s)\n",
			  *value, reg->bit_width, ACPI_FORMAT_UINT64(address),
			  acpi_ut_get_region_name(reg->space_id)));

	return (status);
}

/******************************************************************************
 *
 * FUNCTION:    acpi_hw_write
 *
 * PARAMETERS:  value               - Value to be written
 *              reg                 - GAS register structure
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Write to either memory or IO space. This is a 32-bit max
 *              version of acpi_write, used internally since the overhead of
 *              64-bit values is not needed.
 *
 ******************************************************************************/

acpi_status acpi_hw_write(u32 value, struct acpi_generic_address *reg)
{
	u64 address;
	acpi_status status;

	ACPI_FUNCTION_NAME(hw_write);

	/* Validate contents of the GAS register */

	status = acpi_hw_validate_register(reg, 32, &address);
	if (ACPI_FAILURE(status)) {
		return (status);
	}

	/*
	 * Two address spaces supported: Memory or IO. PCI_Config is
	 * not supported here because the GAS structure is insufficient
	 */
	if (reg->space_id == ACPI_ADR_SPACE_SYSTEM_MEMORY) {
		status = acpi_os_write_memory((acpi_physical_address)
					      address, (u64)value,
					      reg->bit_width);
	} else {		/* ACPI_ADR_SPACE_SYSTEM_IO, validated earlier */

		status = acpi_hw_write_port((acpi_io_address)
					    address, value, reg->bit_width);
	}

	ACPI_DEBUG_PRINT((ACPI_DB_IO,
			  "Wrote: %8.8X width %2d   to %8.8X%8.8X (%s)\n",
			  value, reg->bit_width, ACPI_FORMAT_UINT64(address),
			  acpi_ut_get_region_name(reg->space_id)));

	return (status);
}

#if (!ACPI_REDUCED_HARDWARE)
/*******************************************************************************
 *
 * FUNCTION:    acpi_hw_clear_acpi_status
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Clears all fixed and general purpose status bits
 *
 ******************************************************************************/

acpi_status acpi_hw_clear_acpi_status(void)
{
	acpi_status status;
	acpi_cpu_flags lock_flags = 0;

	ACPI_FUNCTION_TRACE(hw_clear_acpi_status);

	ACPI_DEBUG_PRINT((ACPI_DB_IO, "About to write %04X to %8.8X%8.8X\n",
			  ACPI_BITMASK_ALL_FIXED_STATUS,
			  ACPI_FORMAT_UINT64(acpi_gbl_xpm1a_status.address)));

	raw_spin_lock_irqsave(acpi_gbl_hardware_lock, lock_flags);

	/* Clear the fixed events in PM1 A/B */

	status = acpi_hw_register_write(ACPI_REGISTER_PM1_STATUS,
					ACPI_BITMASK_ALL_FIXED_STATUS);

	raw_spin_unlock_irqrestore(acpi_gbl_hardware_lock, lock_flags);

	if (ACPI_FAILURE(status)) {
		goto exit;
	}

	/* Clear the GPE Bits in all GPE registers in all GPE blocks */

	status = acpi_ev_walk_gpe_list(acpi_hw_clear_gpe_block, NULL);

exit:
	return_ACPI_STATUS(status);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_hw_get_bit_register_info
 *
 * PARAMETERS:  register_id         - Index of ACPI Register to access
 *
 * RETURN:      The bitmask to be used when accessing the register
 *
 * DESCRIPTION: Map register_id into a register bitmask.
 *
 ******************************************************************************/

struct acpi_bit_register_info *acpi_hw_get_bit_register_info(u32 register_id)
{
	ACPI_FUNCTION_ENTRY();

	if (register_id > ACPI_BITREG_MAX) {
		ACPI_ERROR((AE_INFO, "Invalid BitRegister ID: 0x%X",
			    register_id));
		return (NULL);
	}

	return (&acpi_gbl_bit_register_info[register_id]);
}

/******************************************************************************
 *
 * FUNCTION:    acpi_hw_write_pm1_control
 *
 * PARAMETERS:  pm1a_control        - Value to be written to PM1A control
 *              pm1b_control        - Value to be written to PM1B control
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Write the PM1 A/B control registers. These registers are
 *              different than than the PM1 A/B status and enable registers
 *              in that different values can be written to the A/B registers.
 *              Most notably, the SLP_TYP bits can be different, as per the
 *              values returned from the _Sx predefined methods.
 *
 ******************************************************************************/

acpi_status acpi_hw_write_pm1_control(u32 pm1a_control, u32 pm1b_control)
{
	acpi_status status;

	ACPI_FUNCTION_TRACE(hw_write_pm1_control);

	status =
	    acpi_hw_write(pm1a_control, &acpi_gbl_FADT.xpm1a_control_block);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	if (acpi_gbl_FADT.xpm1b_control_block.address) {
		status =
		    acpi_hw_write(pm1b_control,
				  &acpi_gbl_FADT.xpm1b_control_block);
	}
	return_ACPI_STATUS(status);
}

/******************************************************************************
 *
 * FUNCTION:    acpi_hw_register_read
 *
 * PARAMETERS:  register_id         - ACPI Register ID
 *              return_value        - Where the register value is returned
 *
 * RETURN:      Status and the value read.
 *
 * DESCRIPTION: Read from the specified ACPI register
 *
 ******************************************************************************/
acpi_status acpi_hw_register_read(u32 register_id, u32 *return_value)
{
	u32 value = 0;
	acpi_status status;

	ACPI_FUNCTION_TRACE(hw_register_read);

	switch (register_id) {
	case ACPI_REGISTER_PM1_STATUS:	/* PM1 A/B: 16-bit access each */

		status = acpi_hw_read_multiple(&value,
					       &acpi_gbl_xpm1a_status,
					       &acpi_gbl_xpm1b_status);
		break;

	case ACPI_REGISTER_PM1_ENABLE:	/* PM1 A/B: 16-bit access each */

		status = acpi_hw_read_multiple(&value,
					       &acpi_gbl_xpm1a_enable,
					       &acpi_gbl_xpm1b_enable);
		break;

	case ACPI_REGISTER_PM1_CONTROL:	/* PM1 A/B: 16-bit access each */

		status = acpi_hw_read_multiple(&value,
					       &acpi_gbl_FADT.
					       xpm1a_control_block,
					       &acpi_gbl_FADT.
					       xpm1b_control_block);

		/*
		 * Zero the write-only bits. From the ACPI specification, "Hardware
		 * Write-Only Bits": "Upon reads to registers with write-only bits,
		 * software masks out all write-only bits."
		 */
		value &= ~ACPI_PM1_CONTROL_WRITEONLY_BITS;
		break;

	case ACPI_REGISTER_PM2_CONTROL:	/* 8-bit access */

		status =
		    acpi_hw_read(&value, &acpi_gbl_FADT.xpm2_control_block);
		break;

	case ACPI_REGISTER_PM_TIMER:	/* 32-bit access */

		status = acpi_hw_read(&value, &acpi_gbl_FADT.xpm_timer_block);
		break;

	case ACPI_REGISTER_SMI_COMMAND_BLOCK:	/* 8-bit access */

		status =
		    acpi_hw_read_port(acpi_gbl_FADT.smi_command, &value, 8);
		break;

	default:

		ACPI_ERROR((AE_INFO, "Unknown Register ID: 0x%X", register_id));
		status = AE_BAD_PARAMETER;
		break;
	}

	if (ACPI_SUCCESS(status)) {
		*return_value = value;
	}

	return_ACPI_STATUS(status);
}

/******************************************************************************
 *
 * FUNCTION:    acpi_hw_register_write
 *
 * PARAMETERS:  register_id         - ACPI Register ID
 *              value               - The value to write
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Write to the specified ACPI register
 *
 * NOTE: In accordance with the ACPI specification, this function automatically
 * preserves the value of the following bits, meaning that these bits cannot be
 * changed via this interface:
 *
 * PM1_CONTROL[0] = SCI_EN
 * PM1_CONTROL[9]
 * PM1_STATUS[11]
 *
 * ACPI References:
 * 1) Hardware Ignored Bits: When software writes to a register with ignored
 *      bit fields, it preserves the ignored bit fields
 * 2) SCI_EN: OSPM always preserves this bit position
 *
 ******************************************************************************/

acpi_status acpi_hw_register_write(u32 register_id, u32 value)
{
	acpi_status status;
	u32 read_value;

	ACPI_FUNCTION_TRACE(hw_register_write);

	switch (register_id) {
	case ACPI_REGISTER_PM1_STATUS:	/* PM1 A/B: 16-bit access each */
		/*
		 * Handle the "ignored" bit in PM1 Status. According to the ACPI
		 * specification, ignored bits are to be preserved when writing.
		 * Normally, this would mean a read/modify/write sequence. However,
		 * preserving a bit in the status register is different. Writing a
		 * one clears the status, and writing a zero preserves the status.
		 * Therefore, we must always write zero to the ignored bit.
		 *
		 * This behavior is clarified in the ACPI 4.0 specification.
		 */
		value &= ~ACPI_PM1_STATUS_PRESERVED_BITS;

		status = acpi_hw_write_multiple(value,
						&acpi_gbl_xpm1a_status,
						&acpi_gbl_xpm1b_status);
		break;

	case ACPI_REGISTER_PM1_ENABLE:	/* PM1 A/B: 16-bit access each */

		status = acpi_hw_write_multiple(value,
						&acpi_gbl_xpm1a_enable,
						&acpi_gbl_xpm1b_enable);
		break;

	case ACPI_REGISTER_PM1_CONTROL:	/* PM1 A/B: 16-bit access each */
		/*
		 * Perform a read first to preserve certain bits (per ACPI spec)
		 * Note: This includes SCI_EN, we never want to change this bit
		 */
		status = acpi_hw_read_multiple(&read_value,
					       &acpi_gbl_FADT.
					       xpm1a_control_block,
					       &acpi_gbl_FADT.
					       xpm1b_control_block);
		if (ACPI_FAILURE(status)) {
			goto exit;
		}

		/* Insert the bits to be preserved */

		ACPI_INSERT_BITS(value, ACPI_PM1_CONTROL_PRESERVED_BITS,
				 read_value);

		/* Now we can write the data */

		status = acpi_hw_write_multiple(value,
						&acpi_gbl_FADT.
						xpm1a_control_block,
						&acpi_gbl_FADT.
						xpm1b_control_block);
		break;

	case ACPI_REGISTER_PM2_CONTROL:	/* 8-bit access */
		/*
		 * For control registers, all reserved bits must be preserved,
		 * as per the ACPI spec.
		 */
		status =
		    acpi_hw_read(&read_value,
				 &acpi_gbl_FADT.xpm2_control_block);
		if (ACPI_FAILURE(status)) {
			goto exit;
		}

		/* Insert the bits to be preserved */

		ACPI_INSERT_BITS(value, ACPI_PM2_CONTROL_PRESERVED_BITS,
				 read_value);

		status =
		    acpi_hw_write(value, &acpi_gbl_FADT.xpm2_control_block);
		break;

	case ACPI_REGISTER_PM_TIMER:	/* 32-bit access */

		status = acpi_hw_write(value, &acpi_gbl_FADT.xpm_timer_block);
		break;

	case ACPI_REGISTER_SMI_COMMAND_BLOCK:	/* 8-bit access */

		/* SMI_CMD is currently always in IO space */

		status =
		    acpi_hw_write_port(acpi_gbl_FADT.smi_command, value, 8);
		break;

	default:

		ACPI_ERROR((AE_INFO, "Unknown Register ID: 0x%X", register_id));
		status = AE_BAD_PARAMETER;
		break;
	}

exit:
	return_ACPI_STATUS(status);
}

/******************************************************************************
 *
 * FUNCTION:    acpi_hw_read_multiple
 *
 * PARAMETERS:  value               - Where the register value is returned
 *              register_a           - First ACPI register (required)
 *              register_b           - Second ACPI register (optional)
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Read from the specified two-part ACPI register (such as PM1 A/B)
 *
 ******************************************************************************/

static acpi_status
acpi_hw_read_multiple(u32 *value,
		      struct acpi_generic_address *register_a,
		      struct acpi_generic_address *register_b)
{
	u32 value_a = 0;
	u32 value_b = 0;
	acpi_status status;

	/* The first register is always required */

	status = acpi_hw_read(&value_a, register_a);
	if (ACPI_FAILURE(status)) {
		return (status);
	}

	/* Second register is optional */

	if (register_b->address) {
		status = acpi_hw_read(&value_b, register_b);
		if (ACPI_FAILURE(status)) {
			return (status);
		}
	}

	/*
	 * OR the two return values together. No shifting or masking is necessary,
	 * because of how the PM1 registers are defined in the ACPI specification:
	 *
	 * "Although the bits can be split between the two register blocks (each
	 * register block has a unique pointer within the FADT), the bit positions
	 * are maintained. The register block with unimplemented bits (that is,
	 * those implemented in the other register block) always returns zeros,
	 * and writes have no side effects"
	 */
	*value = (value_a | value_b);
	return (AE_OK);
}

/******************************************************************************
 *
 * FUNCTION:    acpi_hw_write_multiple
 *
 * PARAMETERS:  value               - The value to write
 *              register_a           - First ACPI register (required)
 *              register_b           - Second ACPI register (optional)
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Write to the specified two-part ACPI register (such as PM1 A/B)
 *
 ******************************************************************************/

static acpi_status
acpi_hw_write_multiple(u32 value,
		       struct acpi_generic_address *register_a,
		       struct acpi_generic_address *register_b)
{
	acpi_status status;

	/* The first register is always required */

	status = acpi_hw_write(value, register_a);
	if (ACPI_FAILURE(status)) {
		return (status);
	}

	/*
	 * Second register is optional
	 *
	 * No bit shifting or clearing is necessary, because of how the PM1
	 * registers are defined in the ACPI specification:
	 *
	 * "Although the bits can be split between the two register blocks (each
	 * register block has a unique pointer within the FADT), the bit positions
	 * are maintained. The register block with unimplemented bits (that is,
	 * those implemented in the other register block) always returns zeros,
	 * and writes have no side effects"
	 */
	if (register_b->address) {
		status = acpi_hw_write(value, register_b);
	}

	return (status);
}

#endif				/* !ACPI_REDUCED_HARDWARE */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              /******************************************************************************
 *
 * Name: hwsleep.c - ACPI Hardware Sleep/Wake Support functions for the
 *                   original/legacy sleep/PM registers.
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
ACPI_MODULE_NAME("hwsleep")

#if (!ACPI_REDUCED_HARDWARE)	/* Entire module */
/*******************************************************************************
 *
 * FUNCTION:    acpi_hw_legacy_sleep
 *
 * PARAMETERS:  sleep_state         - Which sleep state to enter
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Enter a system sleep state via the legacy FADT PM registers
 *              THIS FUNCTION MUST BE CALLED WITH INTERRUPTS DISABLED
 *
 ******************************************************************************/
acpi_status acpi_hw_legacy_sleep(u8 sleep_state)
{
	struct acpi_bit_register_info *sleep_type_reg_info;
	struct acpi_bit_register_info *sleep_enable_reg_info;
	u32 pm1a_control;
	u32 pm1b_control;
	u32 in_value;
	acpi_status status;

	ACPI_FUNCTION_TRACE(hw_legacy_sleep);

	sleep_type_reg_info =
	    acpi_hw_get_bit_register_info(ACPI_BITREG_SLEEP_TYPE);
	sleep_enable_reg_info =
	    acpi_hw_get_bit_register_info(ACPI_BITREG_SLEEP_ENABLE);

	/* Clear wake status */

	status =
	    acpi_write_bit_register(ACPI_BITREG_WAKE_STATUS, ACPI_CLEAR_STATUS);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/* Clear all fixed and general purpose status bits */

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

	/* Get current value of PM1A control */

	status = acpi_hw_register_read(ACPI_REGISTER_PM1_CONTROL,
				       &pm1a_control);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}
	ACPI_DEBUG_PRINT((ACPI_DB_INIT,
			  "Entering sleep state [S%u]\n", sleep_state));

	/* Clear the SLP_EN and SLP_TYP fields */

	pm1a_control &= ~(sleep_type_reg_info->access_bit_mask |
			  sleep_enable_reg_info->access_bit_mask);
	pm1b_control = pm1a_control;

	/* Insert the SLP_TYP bits */

	pm1a_control |=
	    (acpi_gbl_sleep_type_a << sleep_type_reg_info->bit_position);
	pm1b_control |=
	    (acpi_gbl_sleep_type_b << sleep_type_reg_info->bit_position);

	/*
	 * We split the writes of SLP_TYP and SLP_EN to workaround
	 * poorly implemented hardware.
	 */

	/* Write #1: write the SLP_TYP data to the PM1 Control registers */

	status = acpi_hw_write_pm1_control(pm1a_control, pm1b_control);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/* Insert the sleep enable (SLP_EN) bit */

	pm1a_control |= sleep_enable_reg_info->access_bit_mask;
	pm1b_control |= sleep_enable_reg_info->access_bit_mask;

	/* Flush caches, as per ACPI specification */

	ACPI_FLUSH_CPU_CACHE();

	status = acpi_os_prepare_sleep(sleep_state, pm1a_control,
				       pm1b_control);
	if (ACPI_SKIP(status))
		return_ACPI_STATUS(AE_OK);
	if (ACPI_FAILURE(status))
		return_ACPI_STATUS(status);
	/* Write #2: Write both SLP_TYP + SLP_EN */

	status = acpi_hw_write_pm1_control(pm1a_control, pm1b_control);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	if (sleep_state > ACPI_STATE_S3) {
		/*
		 * We wanted to sleep > S3, but it didn't happen (by virtue of the
		 * fact that we are still executing!)
		 *
		 * Wait ten seconds, then try again. This is to get S4/S5 to work on
		 * all machines.
		 *
		 * We wait so long to allow chipsets that poll this reg very slowly
		 * to still read the right value. Ideally, this block would go
		 * away entirely.
		 */
		acpi_os_stall(10 * ACPI_USEC_PER_SEC);

		status = acpi_hw_register_write(ACPI_REGISTER_PM1_CONTROL,
						sleep_enable_reg_info->
						access_bit_mask);
		if (ACPI_FAILURE(status)) {
			return_ACPI_STATUS(status);
		}
	}

	/* Wait for transition back to Working State */

	do {
		status =
		    acpi_read_bit_register(ACPI_BITREG_WAKE_STATUS, &in_value);
		if (ACPI_FAILURE(status)) {
			return_ACPI_STATUS(status);
		}

	} while (!in_value);

	return_ACPI_STATUS(AE_OK);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_hw_legacy_wake_prep
 *
 * PARAMETERS:  sleep_state         - Which sleep state we just exited
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Perform the first state of OS-independent ACPI cleanup after a
 *              sleep.
 *              Called with interrupts ENABLED.
 *
 ******************************************************************************/

acpi_status acpi_hw_legacy_wake_prep(u8 sleep_state)
{
	acpi_status status = AE_OK;
	struct acpi_bit_register_info *sleep_type_reg_info;
	struct acpi_bit_register_info *sleep_enable_reg_info;
	u32 pm1a_control;
	u32 pm1b_control;

	ACPI_FUNCTION_TRACE(hw_legacy_wake_prep);

	/*
	 * Set SLP_TYPE and SLP_EN to state S0.
	 * This is unclear from the ACPI Spec, but it is required
	 * by some machines.
	 */
	if (acpi_gbl_sleep_type_a_s0 != ACPI_SLEEP_TYPE_INVALID) {
		sleep_type_reg_info =
		    acpi_hw_get_bit_register_info(ACPI_BITREG_SLEEP_TYPE);
		sleep_enable_reg_info =
		    acpi_hw_get_bit_register_info(ACPI_BITREG_SLEEP_ENABLE);

		/* Get current value of PM1A control */

		status = acpi_hw_register_read(ACPI_REGISTER_PM1_CONTROL,
					       &pm1a_control);
		if (ACPI_SUCCESS(status)) {

			/* Clear the SLP_EN and SLP_TYP fields */

			pm1a_control &= ~(sleep_type_reg_info->access_bit_mask |
					  sleep_enable_reg_info->
					  access_bit_mask);
			pm1b_control = pm1a_control;

			/* Insert the SLP_TYP bits */

			pm1a_control |= (acpi_gbl_sleep_type_a_s0 <<
					 sleep_type_reg_info->bit_position);
			pm1b_control |= (acpi_gbl_sleep_type_b_s0 <<
					 sleep_type_reg_info->bit_position);

			/* Write the control registers and ignore any errors */

			(void)acpi_hw_write_pm1_control(pm1a_control,
							pm1b_control);
		}
	}

	return_ACPI_STATUS(status);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_hw_legacy_wake
 *
 * PARAMETERS:  sleep_state         - Which sleep state we just exited
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Perform OS-independent ACPI cleanup after a sleep
 *              Called with interrupts ENABLED.
 *
 ******************************************************************************/

acpi_status acpi_hw_legacy_wake(u8 sleep_state)
{
	acpi_status status;

	ACPI_FUNCTION_TRACE(hw_legacy_wake);

	/* Ensure enter_sleep_state_prep -> enter_sleep_state ordering */

	acpi_gbl_sleep_type_a = ACPI_SLEEP_TYPE_INVALID;
	acpi_hw_execute_sleep_method(METHOD_PATHNAME__SST, ACPI_SST_WAKING);

	/*
	 * GPEs must be enabled before _WAK is called as GPEs
	 * might get fired there
	 *
	 * Restore the GPEs:
	 * 1) Disable/Clear all GPEs
	 * 2) Enable all runtime GPEs
	 */
	status = acpi_hw_disable_all_gpes();
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	status = acpi_hw_enable_all_runtime_gpes();
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/*
	 * Now we can execute _WAK, etc. Some machines require that the GPEs
	 * are enabled before the wake methods are executed.
	 */
	acpi_hw_execute_sleep_method(METHOD_PATHNAME__WAK, sleep_state);

	/*
	 * Some BIOS code assumes that WAK_STS will be cleared on resume
	 * and use it to determine whether the system is rebooting or
	 * resuming. Clear WAK_STS for compatibility.
	 */
	(void)acpi_write_bit_register(ACPI_BITREG_WAKE_STATUS,
				      ACPI_CLEAR_STATUS);
	acpi_gbl_system_awake_and_running = TRUE;

	/* Enable power button */

	(void)
	    acpi_write_bit_register(acpi_gbl_fixed_event_info
				    [ACPI_EVENT_POWER_BUTTON].
				    enable_register_id, ACPI_ENABLE_EVENT);

	(void)
	    acpi_write_bit_register(acpi_gbl_fixed_event_info
				    [ACPI_EVENT_POWER_BUTTON].
				    status_register_id, ACPI_CLEAR_STATUS);

	acpi_hw_execute_sleep_method(METHOD_PATHNAME__SST, ACPI_SST_WORKING);
	return_ACPI_STATUS(status);
}

#endif				/* !ACPI_REDUCED_HARDWARE */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         /******************************************************************************
 *
 * Name: hwtimer.c - ACPI Power Management Timer Interface
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
ACPI_MODULE_NAME("hwtimer")

#if (!ACPI_REDUCED_HARDWARE)	/* Entire module */
/******************************************************************************
 *
 * FUNCTION:    acpi_get_timer_resolution
 *
 * PARAMETERS:  resolution          - Where the resolution is returned
 *
 * RETURN:      Status and timer resolution
 *
 * DESCRIPTION: Obtains resolution of the ACPI PM Timer (24 or 32 bits).
 *
 ******************************************************************************/
acpi_status acpi_get_timer_resolution(u32 * resolution)
{
	ACPI_FUNCTION_TRACE(acpi_get_timer_resolution);

	if (!resolution) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	if ((acpi_gbl_FADT.flags & ACPI_FADT_32BIT_TIMER) == 0) {
		*resolution = 24;
	} else {
		*resolution = 32;
	}

	return_ACPI_STATUS(AE_OK);
}

ACPI_EXPORT_SYMBOL(acpi_get_timer_resolution)

/******************************************************************************
 *
 * FUNCTION:    acpi_get_timer
 *
 * PARAMETERS:  ticks               - Where the timer value is returned
 *
 * RETURN:      Status and current timer value (ticks)
 *
 * DESCRIPTION: Obtains current value of ACPI PM Timer (in ticks).
 *
 ******************************************************************************/
acpi_status acpi_get_timer(u32 * ticks)
{
	acpi_status status;

	ACPI_FUNCTION_TRACE(acpi_get_timer);

	if (!ticks) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	/* ACPI 5.0A: PM Timer is optional */

	if (!acpi_gbl_FADT.xpm_timer_block.address) {
		return_ACPI_STATUS(AE_SUPPORT);
	}

	status = acpi_hw_read(ticks, &acpi_gbl_FADT.xpm_timer_block);
	return_ACPI_STATUS(status);
}

ACPI_EXPORT_SYMBOL(acpi_get_timer)

/******************************************************************************
 *
 * FUNCTION:    acpi_get_timer_duration
 *
 * PARAMETERS:  start_ticks         - Starting timestamp
 *              end_ticks           - End timestamp
 *              time_elapsed        - Where the elapsed time is returned
 *
 * RETURN:      Status and time_elapsed
 *
 * DESCRIPTION: Computes the time elapsed (in microseconds) between two
 *              PM Timer time stamps, taking into account the possibility of
 *              rollovers, the timer resolution, and timer frequency.
 *
 *              The PM Timer's clock ticks at roughly 3.6 times per
 *              _microsecond_, and its clock continues through Cx state
 *              transitions (unlike many CPU timestamp counters) -- making it
 *              a versatile and accurate timer.
 *
 *              Note that this function accommodates only a single timer
 *              rollover. Thus for 24-bit timers, this function should only
 *              be used for calculating durations less than ~4.6 seconds
 *              (~20 minutes for 32-bit timers) -- calculations below:
 *
 *              2**24 Ticks / 3,600,000 Ticks/Sec = 4.66 sec
 *              2**32 Ticks / 3,600,000 Ticks/Sec = 1193 sec or 19.88 minutes
 *
 ******************************************************************************/
acpi_status
acpi_get_timer_duration(u32 start_ticks, u32 end_ticks, u32 * time_elapsed)
{
	acpi_status status;
	u32 delta_ticks;
	u64 quotient;

	ACPI_FUNCTION_TRACE(acpi_get_timer_duration);

	if (!time_elapsed) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	/* ACPI 5.0A: PM Timer is optional */

	if (!acpi_gbl_FADT.xpm_timer_block.address) {
		return_ACPI_STATUS(AE_SUPPORT);
	}

	/*
	 * Compute Tick Delta:
	 * Handle (max one) timer rollovers on 24-bit versus 32-bit timers.
	 */
	if (start_ticks < end_ticks) {
		delta_ticks = end_ticks - start_ticks;
	} else if (start_ticks > end_ticks) {
		if ((acpi_gbl_FADT.flags & ACPI_FADT_32BIT_TIMER) == 0) {

			/* 24-bit Timer */

			delta_ticks =
			    (((0x00FFFFFF - start_ticks) +
			      end_ticks) & 0x00FFFFFF);
		} else {
			/* 32-bit Timer */

			delta_ticks = (0xFFFFFFFF - start_ticks) + end_ticks;
		}
	} else {		/* start_ticks == end_ticks */

		*time_elapsed = 0;
		return_ACPI_STATUS(AE_OK);
	}

	/*
	 * Compute Duration (Requires a 64-bit multiply and divide):
	 *
	 * time_elapsed (microseconds) =
	 *  (delta_ticks * ACPI_USEC_PER_SEC) / ACPI_PM_TIMER_FREQUENCY;
	 */
	status = acpi_ut_short_divide(((u64)delta_ticks) * ACPI_USEC_PER_SEC,
				      ACPI_PM_TIMER_FREQUENCY, &quotient, NULL);

	*time_elapsed = (u32) quotient;
	return_ACPI_STATUS(status);
}

ACPI_EXPORT_SYMBOL(acpi_get_timer_duration)
#endif				/* !ACPI_REDUCED_HARDWARE */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    /******************************************************************************
 *
 * Module Name: hwvalid - I/O request validation
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
ACPI_MODULE_NAME("hwvalid")

/* Local prototypes */
static acpi_status
acpi_hw_validate_io_request(acpi_io_address address, u32 bit_width);

/*
 * Protected I/O ports. Some ports are always illegal, and some are
 * conditionally illegal. This table must remain ordered by port address.
 *
 * The table is used to implement the Microsoft port access rules that
 * first appeared in Windows XP. Some ports are always illegal, and some
 * ports are only illegal if the BIOS calls _OSI with a win_XP string or
 * later (meaning that the BIOS itelf is post-XP.)
 *
 * This provides ACPICA with the desired port protections and
 * Microsoft compatibility.
 *
 * Description of port entries:
 *  DMA:   DMA controller
 *  PIC0:  Programmable Interrupt Controller (8259A)
 *  PIT1:  System Timer 1
 *  PIT2:  System Timer 2 failsafe
 *  RTC:   Real-time clock
 *  CMOS:  Extended CMOS
 *  DMA1:  DMA 1 page registers
 *  DMA1L: DMA 1 Ch 0 low page
 *  DMA2:  DMA 2 page registers
 *  DMA2L: DMA 2 low page refresh
 *  ARBC:  Arbitration control
 *  SETUP: Reserved system board setup
 *  POS:   POS channel select
 *  PIC1:  Cascaded PIC
 *  IDMA:  ISA DMA
 *  ELCR:  PIC edge/level registers
 *  PCI:   PCI configuration space
 */
static const struct acpi_port_info acpi_protected_ports[] = {
	{"DMA", 0x0000, 0x000F, ACPI_OSI_WIN_XP},
	{"PIC0", 0x0020, 0x0021, ACPI_ALWAYS_ILLEGAL},
	{"PIT1", 0x0040, 0x0043, ACPI_OSI_WIN_XP},
	{"PIT2", 0x0048, 0x004B, ACPI_OSI_WIN_XP},
	{"RTC", 0x0070, 0x0071, ACPI_OSI_WIN_XP},
	{"CMOS", 0x0074, 0x0076, ACPI_OSI_WIN_XP},
	{"DMA1", 0x0081, 0x0083, ACPI_OSI_WIN_XP},
	{"DMA1L", 0x0087, 0x0087, ACPI_OSI_WIN_XP},
	{"DMA2", 0x0089, 0x008B, ACPI_OSI_WIN_XP},
	{"DMA2L", 0x008F, 0x008F, ACPI_OSI_WIN_XP},
	{"ARBC", 0x0090, 0x0091, ACPI_OSI_WIN_XP},
	{"SETUP", 0x0093, 0x0094, ACPI_OSI_WIN_XP},
	{"POS", 0x0096, 0x0097, ACPI_OSI_WIN_XP},
	{"PIC1", 0x00A0, 0x00A1, ACPI_ALWAYS_ILLEGAL},
	{"IDMA", 0x00C0, 0x00DF, ACPI_OSI_WIN_XP},
	{"ELCR", 0x04D0, 0x04D1, ACPI_ALWAYS_ILLEGAL},
	{"PCI", 0x0CF8, 0x0CFF, ACPI_OSI_WIN_XP}
};

#define ACPI_PORT_INFO_ENTRIES  ACPI_ARRAY_LENGTH (acpi_protected_ports)

/******************************************************************************
 *
 * FUNCTION:    acpi_hw_validate_io_request
 *
 * PARAMETERS:  Address             Address of I/O port/register
 *              bit_width           Number of bits (8,16,32)
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Validates an I/O request (address/length). Certain ports are
 *              always illegal and some ports are only illegal depending on
 *              the requests the BIOS AML code makes to the predefined
 *              _OSI method.
 *
 ******************************************************************************/

static acpi_status
acpi_hw_validate_io_request(acpi_io_address address, u32 bit_width)
{
	u32 i;
	u32 byte_width;
	acpi_io_address last_address;
	const struct acpi_port_info *port_info;

	ACPI_FUNCTION_TRACE(hw_validate_io_request);

	/* Supported widths are 8/16/32 */

	if ((bit_width != 8) && (bit_width != 16) && (bit_width