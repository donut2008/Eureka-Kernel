EXAGON_V6_vgtw llvm.hexagon.V6.vgtw __builtin_ia32_pcmpgtw getw __builtin_HEXAGON_V6_extractw llvm.hexagon.V6.extractw __builtin_HEXAGON_V6_lvsplatw llvm.hexagon.V6.lvsplatw __builtin_altivec_vmaxsw llvm.ppc.altivec.vmaxsw __builtin_ia32_pmaxsw __builtin_ia32_paddusw __builtin_ia32_psubusw __builtin_altivec_vcmpgtsw llvm.ppc.altivec.vcmpgtsw __builtin_ia32_pmulhrsw __builtin_altivec_vmulosw llvm.ppc.altivec.vmulosw llvm.ppc.altivec.vinsw __builtin_altivec_vminsw llvm.ppc.altivec.vminsw __builtin_ia32_pminsw  nsw __builtin_altivec_vupklsw llvm.ppc.altivec.vupklsw __builtin_altivec_vmulhsw llvm.ppc.altivec.vmulhsw __builtin_altivec_vupkhsw llvm.ppc.altivec.vupkhsw __builtin_altivec_vavgsw llvm.ppc.altivec.vavgsw __builtin_altivec_vdivesw llvm.ppc.altivec.vdivesw __builtin_altivec_vmulesw llvm.ppc.altivec.vmulesw __builtin_ia32_paddsw __builtin_ia32_phaddsw __builtin_ia32_psubsw __builtin_ia32_phsubsw __builtin_ia32_pmaddubsw __builtin_HEXAGON_V6_vabsw __builtin_HEXAGON_A2_vabsw llvm.hexagon.V6.vabsw llvm.hexagon.A2.vabsw __builtin_ia32_pabsw llvm.x86.ssse3.pmul.hr.sw llvm.x86.avx2.pmul.hr.sw llvm.x86.ssse3.phadd.sw llvm.x86.avx2.phadd.sw llvm.x86.ssse3.phsub.sw llvm.x86.avx2.phsub.sw llvm.x86.ssse3.pmadd.ub.sw llvm.x86.avx2.pmadd.ub.sw __builtin_HEXAGON_M7_wcmpyrw llvm.hexagon.M7.wcmpyrw __builtin_HEXAGON_M7_dcmpyrw llvm.hexagon.M7.dcmpyrw llvm.ppc.altivec.vsrw __builtin_HEXAGON_V6_vlsrw llvm.hexagon.V6.vlsrw __builtin_HEXAGON_V6_vasrw llvm.hexagon.V6.vasrw __builtin_ia32_pmulhrw llvm.x86.3dnow.pmulhrw llvm.aarch64.sve.adrw ocl_image2d_array_rw ocl_image1d_array_rw ocl_image1d_buffer_rw ocl_image2d_array_depth_rw ocl_image2d_depth_rw ocl_image2d_array_msaa_depth_rw ocl_image2d_msaa_depth_rw ocl_image3d_rw ocl_image2d_rw ocl_image1d_rw ocl_image2d_array_msaa_rw ocl_image2d_msaa_rw __builtin_HEXAGON_V6_vprefixqw llvm.hexagon.V6.vprefixqw __builtin_HEXAGON_V6_veqw llvm.hexagon.V6.veqw __builtin_ia32_pcmpeqw __builtin_HEXAGON_V6_shuffeqw llvm.hexagon.V6.shuffeqw -analyzer-checker=security.insecureAPI.getpw https://clang-analyzer.llvm.org/available_checks.html#security.insecureAPI.getpw llvm.arm.mve.vcvt.narrow lsr-exp-narrow FatArrow isArrow TrailingReturnArrow LambdaArrow  [[gnu::nothrow  __declspec(nothrow  __attribute__((nothrow  nothrow objc_exception_rethrow __builtin_wasm_rethrow __cxa_rethrow finally.rethrow llvm.wasm.rethrow _Unwind_Resume_or_Rethrow finally.shouldthrow objc_exception_throw __builtin_wasm_throw __cxa_throw @throw llvm.wasm.throw isNoThrow attr::NoThrow __builtin_wasm_memory_grow llvm.wasm.memory.grow matrix row not undoing last redundant row __builtin_cpow _ZGVdN4vv_pow _ZGVbN2vv_pow __builtin_pow llvm.pow llvm.experimental.constrained.pow -m3dnow -mno-3dnow ignoring directive .dump for now ignoring directive .load for now yellow nooverflow float_cast_overflow pointer_overflow divrem_overflow __builtin_umul_overflow __builtin_smul_overflow __builtin_mul_overflow __builtin_umull_overflow __builtin_smull_overflow __builtin_umulll_overflow __builtin_smulll_overflow __builtin_uaddll_overflow __builtin_saddll_overflow __builtin_usubll_overflow __builtin_ssubll_overflow __builtin_uaddl_overflow __builtin_saddl_overflow __builtin_usubl_overflow __builtin_ssubl_overflow using_overflow negate_overflow __builtin_uadd_overflow __builtin_sadd_overflow __builtin_add_overflow __builtin_usub_overflow __builtin_ssub_overflow __builtin_sub_overflow mul.overflow llvm.umul.with.overflow llvm.smul.with.overflow llvm.uadd.with.overflow llvm.sadd.with.overflow llvm.usub.with.overflow llvm.ssub.with.overflow sadd.overflow -fstrict-float-cast-overflow -fno-strict-float-cast-overflow -fstrict-overflow -fno-strict-overflow pointer-overflow unsigned-integer-overflow loop-flatten-assume-no-overflow the computation of the size of the memory allocation may overflow  cannot overflow Assume that the product of the two iteration limits will never overflow Counter overflow Trap on integer overflow Call to function 'gets' is extremely insecure as it can always result in a buffer overflow Undefined behavior: Buffer overflow Specify the function to be called on overflow malloc() size overflow canOverflow https://clang-analyzer.llvm.org/alpha_checks.html#alpha.security.MallocOverflow dataflow dfsan-track-select-control-flow adce-remove-control-flow unable to calculate the loop count due to complex control flow Lower the guard intrinsic to normal control flow Structurize control flow __riscv_cmodel_medlow -mcmodel=medlow vfmlsl_low vfmlal_low llvm.wasm.promote.low polly-show instcombine-guard-widening-window NSWindow __hwasan_shadow __asan_shadow param_shadow retval_shadow va_arg_shadow .hwasan.shadow .asan.shadow msan-check-constant-shadow asan-force-dynamic-shadow report accesses through a pointer which has poisoned shadow ConstructorUsingShadow __builtin_HEXAGON_V6_vminw __builtin_HEXAGON_A2_vminw llvm.hexagon.V6.vminw llvm.hexagon.A2.vminw __builtin_HEXAGON_A4_vrminw llvm.hexagon.A4.vrminw __builtin_ia32_psignw .balignw .p2alignw __builtin_altivec_crypto_vpmsumw llvm.ppc.altivec.crypto.vpmsumw __builtin_HEXAGON_V6_vscattermw llvm.hexagon.V6.vscattermw __builtin_HEXAGON_V6_vgathermw llvm.hexagon.V6.vgathermw expected binary operation in atomicrmw __builtin_ia32_vpcomw llvm.ppc.altivec.vslw __builtin_HEXAGON_V6_vaslw llvm.hexagon.V6.vaslw __builtin_altivec_vrlw llvm.ppc.altivec.vrlw __builtin_ia32_psrlw __builtin_ia32_pmullw __builtin_ia32_psllw __builtin_ia32_vpshlw llvm.x86.xop.vpshlw __builtin_ia32_pshuflw __builtin_HEXAGON_M7_wcmpyiw llvm.hexagon.M7.wcmpyiw __builtin_HEXAGON_M7_dcmpyiw llvm.hexagon.M7.dcmpyiw avx5124vnniw __builtin_ia32_pf2iw llvm.x86.3dnowa.pf2iw __builtin_HEXAGON_V6_vadduhw llvm.hexagon.V6.vadduhw __builtin_HEXAGON_V6_vsubuhw llvm.hexagon.V6.vsubuhw __builtin_HEXAGON_S2_vzxthw llvm.hexagon.S2.vzxthw __builtin_HEXAGON_S2_vsxthw llvm.hexagon.S2.vsxthw __builtin_HEXAGON_V6_vscattermhw llvm.hexagon.V6.vscattermhw __builtin_HEXAGON_V6_vgathermhw llvm.hexagon.V6.vgathermhw __builtin_ia32_pmulhw __builtin_s390_vuplhw llvm.s390.vuplhw __builtin_ia32_pshufhw __builtin_HEXAGON_V6_vaddhw llvm.hexagon.V6.vaddhw -mprfchw -mno-prfchw +prfchw __builtin_HEXAGON_V6_vsubhw llvm.hexagon.V6.vsubhw __builtin_HEXAGON_V6_vavgw __builtin_HEXAGON_A2_vavgw llvm.hexagon.V6.vavgw llvm.hexagon.A2.vavgw __builtin_ia32_pavgw __builtin_HEXAGON_V6_vnavgw __builtin_HEXAGON_A2_vnavgw llvm.hexagon.V6.vnavgw llvm.hexagon.A2.vnavgw __builtin_ia32_pshufw __builtin_sve_svprfw objfw __builtin_HEXAGON_V6_vabsdiffw __builtin_HEXAGON_M2_vabsdiffw llvm.hexagon.V6.vabsdiffw llvm.hexagon.M2.vabsdiffw __builtin_ia32_pi2fw llvm.x86.3dnowa.pi2fw __builtin_altivec_vcmpnew llvm.ppc.altivec.vcmpnew __builtin_HEXAGON_A2_combinew llvm.hexagon.A2.combinew __builtin_operator_new __cpp_aligned_new  OMF_new .new -fassume-sane-operator-new -fno-assume-sane-operator-new -fcheck-new -fno-check-new flang-new -faligned-new -fno-aligned-new Insufficient storage for placement new Bad align storage for placement new default new operator new -gcodeview DW_AT_IBM_alt_srcview DW_AT_GNU_entry_view basic_string_view -ast-view UITextView UIAlertView CodeView NSView UIView QCView https://clang-analyzer.llvm.org/available_checks.html#cplusplus.PlacementNew /Zc:alignedNew __builtin_HEXAGON_V6_vsatdw llvm.hexagon.V6.vsatdw llvm.x86.avx2.packusdw llvm.x86.sse41.packusdw __builtin_ia32_packssdw llvm.x86.mmx.packssdw llvm.x86.avx2.packssdw __builtin_brev_ldw __builtin_circ_ldw llvm.hexagon.circ.ldw __builtin_HEXAGON_V6_vaddw __builtin_HEXAGON_A2_vaddw llvm.hexagon.V6.vaddw llvm.hexagon.A2.vaddw __builtin_ia32_paddw __builtin_ia32_phaddw __builtin_HEXAGON_S4_vxsubaddw llvm.hexagon.S4.vxsubaddw llvm.aarch64.sve.uqincw llvm.aarch64.sve.sqincw llvm.aarch64.sve.uqdecw llvm.aarch64.sve.sqdecw __builtin_arm_crc32cw llvm.arm.crc32cw llvm.aarch64.crc32cw __builtin_altivec_vprtybw llvm.ppc.altivec.vprtybw __builtin_HEXAGON_V6_vsubw __builtin_HEXAGON_A2_vsubw llvm.hexagon.V6.vsubw llvm.hexagon.A2.vsubw __builtin_ia32_psubw __builtin_ia32_phsubw __builtin_HEXAGON_S4_vxaddsubw llvm.hexagon.S4.vxaddsubw __builtin_ia32_vphaddubw llvm.x86.xop.vphaddubw __builtin_altivec_vcntmbw llvm.ppc.altivec.vcntmbw __builtin_ia32_punpcklbw llvm.x86.mmx.punpcklbw __builtin_HEXAGON_V6_vaddclbw llvm.hexagon.V6.vaddclbw __builtin_ia32_punpckhbw llvm.x86.mmx.punpckhbw __builtin_ia32_vphaddbw llvm.x86.xop.vphaddbw llvm.x86.avx2.mpsadbw llvm.x86.sse41.mpsadbw __builtin_ia32_psadbw __builtin_ia32_vphsubbw llvm.x86.xop.vphsubbw -mavx512bw -mno-avx512bw avx512vl,avx512bw +avx512bw llvm.x86.mmx.psad.bw llvm.x86.avx2.psad.bw llvm.x86.sse2.psad.bw float2int-max-integer-bw __builtin_altivec_vsraw llvm.ppc.altivec.vsraw __builtin_ia32_psraw llvm.ppc.darnraw -plugin-opt=cs-profile-path=default_%m.profraw memprof.profraw __builtin_darn_raw __builtin_altivec_crypto_vshasigmaw llvm.ppc.altivec.crypto.vshasigmaw __builtin_ia32_vpshaw llvm.x86.xop.vpshaw __builtin_msa_bnz_w __builtin_msa_bz_w __builtin_msa_fmax_w __builtin_msa_mulv_w __builtin_msa_fdiv_w __builtin_msa_ilvev_w __builtin_msa_pckev_w __builtin_msa_maddv_w __builtin_msa_addv_w __builtin_msa_msubv_w __builtin_msa_subv_w __builtin_msa_copy_u_w __builtin_msa_max_u_w __builtin_msa_div_u_w __builtin_msa_ftint_u_w __builtin_msa_ffint_u_w __builtin_msa_clt_u_w __builtin_msa_sat_u_w __builtin_msa_subsus_u_w __builtin_msa_adds_u_w __builtin_msa_subs_u_w __builtin_msa_aver_u_w __builtin_msa_dotp_u_w __builtin_msa_min_u_w __builtin_msa_maxi_u_w __builtin_msa_clti_u_w __builtin_msa_mini_u_w __builtin_msa_clei_u_w __builtin_msa_ave_u_w __builtin_msa_cle_u_w __builtin_msa_mod_u_w __builtin_msa_dpadd_u_w __builtin_msa_hadd_u_w __builtin_msa_ftrunc_u_w __builtin_msa_dpsub_u_w __builtin_msa_hsub_u_w __builtin_msa_asub_u_w __builtin_msa_st_w __builtin_msa_frsqrt_w __builtin_msa_fsqrt_w __builtin_msa_insert_w __builtin_msa_frint_w __builtin_msa_pcnt_w __builtin_msa_fsult_w __builtin_msa_fcult_w __builtin_msa_fslt_w __builtin_msa_fclt_w __builtin_msa_bset_w __builtin_msa_splat_w __builtin_msa_fclass_w __builtin_mips_extr_rs_w __builtin_mips_mulq_rs_w __builtin_msa_copy_s_w __builtin_msa_max_s_w __builtin_msa_div_s_w __builtin_msa_subsuu_s_w __builtin_msa_ftint_s_w __builtin_msa_ffint_s_w __builtin_msa_clt_s_w __builtin_msa_sat_s_w __builtin_msa_adds_s_w __builtin_msa_subs_s_w __builtin_msa_aver_s_w __builtin_mips_absq_s_w __builtin_mips_mulq_s_w __builtin_mips_addq_s_w __builtin_mips_subq_s_w __builtin_msa_dotp_s_w __builtin_msa_min_s_w __builtin_mips_shll_s_w __builtin_msa_maxi_s_w __builtin_msa_clti_s_w __builtin_msa_mini_s_w __builtin_msa_clei_s_w __builtin_msa_ave_s_w __builtin_msa_cle_s_w __builtin_msa_mod_s_w __builtin_msa_dpadd_s_w __builtin_msa_hadd_s_w __builtin_msa_ftrunc_s_w __builtin_msa_dpsub_s_w __builtin_msa_hsub_s_w __builtin_msa_asub_s_w __builtin_msa_ilvr_w __builtin_mips_extr_w __builtin_msa_str_w __builtin_msa_binsr_w __builtin_msa_ffqr_w __builtin_msa_fexupr_w __builtin_msa_fsor_w __builtin_msa_fcor_w __builtin_msa_srlr_w __builtin_msa_bclr_w __builtin_msa_ldr_w __builtin_msa_srar_w __builtin_mips_extr_r_w __builtin_mips_addqh_r_w __builtin_mips_subqh_r_w __builtin_mips_shra_r_w __builtin_msa_ftq_w __builtin_msa_fsueq_w __builtin_msa_fcueq_w __builtin_msa_fseq_w __builtin_msa_fceq_w __builtin_msa_ceq_w __builtin_msa_mulr_q_w __builtin_msa_maddr_q_w __builtin_msa_msubr_q_w __builtin_msa_mul_q_w __builtin_msa_madd_q_w __builtin_msa_msub_q_w __builtin_msa_frcp_w __builtin_msa_fexdo_w __builtin_msa_fsun_w __builtin_msa_fcun_w __builtin_msa_fmin_w __builtin_riscv_xperm_w __builtin_msa_ilvl_w __builtin_msa_fmul_w __builtin_msa_binsl_w __builtin_msa_srl_w __builtin_msa_ffql_w __builtin_msa_fexupl_w __builtin_msa_sll_w __builtin_msa_fill_w __builtin_mips_dpsq_sa_l_w __builtin_mips_dpaq_sa_l_w __builtin_msa_addvi_w __builtin_msa_subvi_w __builtin_msa_bseti_w __builtin_msa_splati_w __builtin_msa_binsri_w __builtin_msa_srlri_w __builtin_msa_bclri_w __builtin_msa_srari_w __builtin_msa_ceqi_w __builtin_msa_binsli_w __builtin_msa_srli_w __builtin_msa_slli_w __builtin_msa_bnegi_w __builtin_msa_sldi_w __builtin_msa_ldi_w __builtin_msa_srai_w __builtin_mips_addqh_w __builtin_mips_subqh_w __builtin_mips_precrq_rs_ph_w __builtin_mips_precr_sra_r_ph_w __builtin_mips_precrq_ph_w __builtin_mips_precr_sra_ph_w __builtin_msa_bneg_w __builtin_msa_vshf_w __builtin_msa_shf_w __builtin_msa_fsaf_w __builtin_msa_fcaf_w __builtin_msa_insve_w __builtin_msa_fsune_w __builtin_msa_fcune_w __builtin_msa_fsne_w __builtin_msa_fcne_w __builtin_msa_fsule_w __builtin_msa_fcule_w __builtin_msa_fsle_w __builtin_msa_fcle_w __builtin_msa_ilvod_w __builtin_msa_pckod_w __builtin_msa_sld_w __builtin_msa_ld_w __nvvm_read_ptx_sreg_ntid_w __nvvm_read_ptx_sreg_tid_w __nvvm_read_ptx_sreg_nctaid_w __nvvm_read_ptx_sreg_ctaid_w __builtin_msa_fmadd_w __builtin_msa_fadd_w __builtin_msa_nlzc_w __builtin_msa_nloc_w __builtin_riscv_crc32c_w __builtin_msa_fmsub_w __builtin_msa_fsub_w __builtin_msa_sra_w __builtin_msa_fmax_a_w __builtin_msa_max_a_w __builtin_msa_adds_a_w __builtin_msa_fmin_a_w __builtin_msa_min_a_w __builtin_msa_add_a_w __builtin_msa_fexp2_w __builtin_msa_flog2_w __builtin_riscv_crc32_w /Gw Dw -m:w r9w r8w r15w __builtin_HEXAGON_V6_vdealb4w llvm.hexagon.V6.vdealb4w r14w r13w __builtin_altivec_vextsh2w llvm.ppc.altivec.vextsh2w __builtin_HEXAGON_F2_conv_sf2w llvm.hexagon.F2.conv.sf2w __builtin_HEXAGON_F2_conv_df2w llvm.hexagon.F2.conv.df2w __builtin_altivec_vextsb2w llvm.ppc.altivec.vextsb2w __builtin_arm_crc32w llvm.arm.crc32w llvm.aarch64.crc32w r12w r11w __builtin_HEXAGON_V6_vcl0w llvm.hexagon.V6.vcl0w r10w /w llvm.x86.avx512.ktestz.w llvm.mips.bnz.w llvm.mips.bz.w llvm.mips.fmax.w llvm.riscv.vfncvt.f.x.w llvm.mips.mulv.w llvm.mips.fdiv.w llvm.mips.ilvev.w llvm.mips.pckev.w llvm.mips.maddv.w llvm.mips.addv.w llvm.mips.msubv.w llvm.mips.subv.w llvm.riscv.vfncvt.f.xu.w llvm.x86.mmx.pmulhu.w llvm.x86.avx2.pmulhu.w llvm.x86.sse2.pmulhu.w llvm.riscv.vwaddu.w llvm.riscv.vwsubu.w llvm.mips.copy.u.w llvm.mips.max.u.w llvm.mips.div.u.w llvm.mips.ftint.u.w llvm.mips.ffint.u.w llvm.mips.clt.u.w llvm.mips.sat.u.w llvm.mips.subsus.u.w llvm.mips.adds.u.w llvm.mips.subs.u.w llvm.mips.aver.u.w llvm.mips.dotp.u.w llvm.mips.min.u.w llvm.mips.maxi.u.w llvm.mips.clti.u.w llvm.mips.mini.u.w llvm.mips.clei.u.w llvm.mips.ave.u.w llvm.mips.cle.u.w llvm.mips.mod.u.w llvm.mips.dpadd.u.w llvm.mips.hadd.u.w llvm.mips.ftrunc.u.w llvm.mips.dpsub.u.w llvm.mips.hsub.u.w llvm.mips.asub.u.w llvm.mips.st.w llvm.mips.frsqrt.w llvm.mips.fsqrt.w llvm.mips.insert.w llvm.mips.frint.w llvm.mips.pcnt.w llvm.mips.fsult.w llvm.mips.fcult.w llvm.mips.fslt.w llvm.mips.fclt.w llvm.x86.mmx.pcmpgt.w llvm.mips.bset.w llvm.mips.splat.w llvm.x86.mmx.pmaxs.w llvm.x86.mmx.paddus.w llvm.x86.mmx.psubus.w llvm.mips.fclass.w llvm.mips.extr.rs.w llvm.mips.m