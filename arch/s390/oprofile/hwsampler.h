VECTOR));

			/*
			 * Corrected errors will still be corrected, but
			 * make sure there's a log somewhere that indicates
			 * something is generating more than we can handle.
			 */
			printk(KERN_WARNING "WARNING: Switching to polling CPE handler; error records may be lost\n");

			mod_timer(&cpe_poll_timer, jiffies + MIN_CPE_POLL_INTERVAL);

			/* lock already released, get out now */
			goto out;
		} else {
			cpe_history[index++] = now;
			if (index == CPE_HISTORY_LENGTH)
				index = 0;
		}
	}
	spin_unlock(&cpe_history_lock);
out:
	/* Get the CPE error record and log it */
	ia64_mca_log_sal_error_record(SAL_INFO_TYPE_CPE);

	local_irq_disable();

	return IRQ_HANDLED;
}

#endif /* CONFIG_ACPI */

#ifdef CONFIG_ACPI
/*
 * ia64_mca_register_cpev
 *
 *  Register the corrected platform error vector with SAL.
 *
 *  Inputs
 *      cpev        Corrected Platform Error Vector number
 *
 *  Outputs
 *      None
 */
void
ia64_mca_register_cpev (int cpev)
{
	/* Register the CPE interrupt vector with SAL */
	struct ia64_sal_retval isrv;

	isrv = ia64_sal_mc_set_params(SAL_MC_PARAM_CPE_INT, SAL_MC_PARAM_MECHANISM_INT, cpev, 0, 0);
	if (isrv.status) {
		printk(KERN_ERR "Failed to register Corrected Platform "
		       "Error interrupt vector with SAL (status %ld)\n", isrv.status);
		return;
	}

	IA64_MCA_DEBUG("%s: corrected platform error "
		       "vector %#x registered\n", __func__, cpev);
}
#endif /* CONFIG_ACPI */

/*
 * ia64_mca_cmc_vector_setup
 *
 *  Setup the corrected machine check vector register in the processor.
 *  (The interrupt is masked on boot. ia64_mca_late_init unmask this.)
 *  This function is invoked on a per-processor basis.
 *
 * Inputs
 *      None
 *
 * Outputs
 *	None
 */
void
ia64_mca_cmc_vector_setup (void)
{
	cmcv_reg_t	cmcv;

	cmcv.cmcv_regval	= 0;
	cmcv.cmcv_mask		= 1;        /* Mask/disable interrupt at first */
	cmcv.cmcv_vector	= IA64_CMC_VECTOR;
	ia64_setreg(_IA64_REG_CR_CMCV, cmcv.cmcv_regval);

	IA64_MCA_DEBUG("%s: CPU %d corrected machine check vector %#x registered.\n",
		       __func__, smp_processor_id(), IA64_CMC_VECTOR);

	IA64_MCA_DEBUG("%s: CPU %d CMCV = %#016lx\n",
		       __func__, smp_processor_id(), ia64_getreg(_IA64_REG_CR_CMCV));
}

/*
 * ia64_mca_cmc_vector_disable
 *
 *  Mask the corrected machine check vector register in the processor.
 *  This function is invoked on a per-processor basis.
 *
 * Inputs
 *      dummy(unused)
 *
 * Outputs
 *	None
 */
static void
ia64_mc