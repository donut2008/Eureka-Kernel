flush_fph (struct task_struct *);
  extern void ia64_sync_fph (struct task_struct *);
  extern void ia64_sync_krbs(void);
  extern long ia64_sync_user_rbs (struct task_struct *, struct switch_stack *,
				  unsigned long, unsigned long);

  /* get nat bits for scratch registers such that bit N==1 iff scratch register rN is a NaT */
  extern unsigned long ia64_get_scratch_nat_bits (struct pt_regs *pt, unsigned long scratch_unat);
  /* put nat bits for scratch registers such that scratch register rN is a NaT iff bit N==1 */
  extern unsigned long ia64_put_scratch_nat_bits (struct pt_regs *pt, unsigned long nat);

  extern void ia64_increment_ip (struct pt_regs *pt);
  extern void ia64_decrement_ip (struct pt_regs *pt);

  extern void ia64_ptrace_stop(void);
  #define arch_ptrace_stop(code, info) \
	ia64_ptrace_stop()
  #define arch_ptrace_stop_needed(code, info) \
	(!test_thread_flag(TIF_RESTORE_RSE))

  extern void ptrace_attach_sync_user_rbs (struct task_struct *);
  #define arch_ptrace_attach(child) \
	ptrace_attach_sync_user_rbs(child)

  #define arch_has_single_step()  (1)
  #define arch_has_block_step()   (1)

#endif /* !__ASSEMBLY__ */
#endif /* _ASM_IA64_PTRACE_H */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            