fine PA_CL_CNTL_STATUS__CL_BUSY_MASK 0x80000000
#define PA_CL_CNTL_STATUS__CL_BUSY__SHIFT 0x1f
#define PA_SU_CNTL_STATUS__SU_BUSY_MASK 0x80000000
#define PA_SU_CNTL_STATUS__SU_BUSY__SHIFT 0x1f
#define PA_SC_FIFO_DEPTH_CNTL__DEPTH_MASK 0x3ff
#define PA_SC_FIFO_DEPTH_CNTL__DEPTH__SHIFT 0x0
#define CGTT_PA_CLK_CTRL__ON_DELAY_MASK 0xf
#define CGTT_PA_CLK_CTRL__ON_DELAY__SHIFT 0x0
#define CGTT_PA_CLK_CTRL__OFF_HYSTERESIS_MASK 0xff0
#define CGTT_PA_CLK_CTRL__OFF_HYSTERESIS__SHIFT 0x4
#define CGTT_PA_CLK_CTRL__SOFT_OVERRIDE7_MASK 0x1000000
#define CGTT_PA_CLK_CTRL__SOFT_OVERRIDE7__SHIFT 0x18
#define CGTT_PA_CLK_CTRL__SOFT_OVERRIDE6_MASK 0x2000000
#define CGTT_PA_CLK_CTRL__SOFT_OVERRIDE6__SHIFT 0x19
#define CGTT_PA_CLK_CTRL__SOFT_OVERRIDE5_MASK 0x4000000
#define CGTT_PA_CLK_CTRL__SOFT_OVERRIDE5__SHIFT 0x1a
#define CGTT_PA_CLK_CTRL__SOFT_OVERRIDE4_MASK 0x8000000
#define CGTT_PA_CLK_CTRL__SOFT_OVERRIDE4__SHIFT 0x1b
#define CGTT_PA_CLK_CTRL__SOFT_OVERRIDE3_MASK 0x10000000
#define CGTT_PA_CLK_CTRL__SOFT_OVERRIDE3__SHIFT 0x1c
#define CGTT_PA_CLK_CTRL__SU_CLK_OVERRIDE_MASK 0x20000000
#define CGTT_PA_CLK_CTRL__SU_CLK_OVERRIDE__SHIFT 0x1d
#define CGTT_PA_CLK_CTRL__CL_CLK_OVERRIDE_MASK 0x40000000
#define CGTT_PA_CLK_CTRL__CL_CLK_OVERRIDE__SHIFT 0x1e
#define CGTT_PA_CLK_CTRL__REG_CLK_OVERRIDE_MASK 0x80000000
#define CGTT_PA_CLK_CTRL__REG_CLK_OVERRIDE__SHIFT 0x1f
#define CGTT_SC_CLK_CTRL__ON_DELAY_MASK 0xf
#define CGTT_SC_CLK_CTRL__ON_DELAY__SHIFT 0x0
#define CGTT_SC_CLK_CTRL__OFF_HYSTERESIS_MASK 0xff0
#define CGTT_SC_CLK_CTRL__OFF_HYSTERESIS__SHIFT 0x4
#define CGTT_SC_CLK_CTRL__SOFT_OVERRIDE7_MASK 0x1000000
#define CGTT_SC_CLK_CTRL__SOFT_OVERRIDE7__SHIFT 0x18
#define CGTT_SC_CLK_CTRL__SOFT_OVERRIDE6_MASK 0x2000000
#define CGTT_SC_CLK_CTRL__SOFT_OVERRIDE6__SHIFT 0x19
#define CGTT_SC_CLK_CTRL__SOFT_OVERRIDE5_MASK 0x4000000
#define CGTT_SC_CLK_CTRL__SOFT_OVERRIDE5__SHIFT 0x1a
#define CGTT_SC_CLK_CTRL__SOFT_OVERRIDE4_MASK 0x8000000
#define CGTT_SC_CLK_CTRL__SOFT_OVERRIDE4__SHIFT 0x1b
#define CGTT_SC_CLK_CTRL__SOFT_OVERRIDE3_MASK 0x10000000
#define CGTT_SC_CLK_CTRL__SOFT_OVERRIDE3__SHIFT 0x1c
#define CGTT_SC_CLK_CTRL__SOFT_OVERRIDE2_MASK 0x20000000
#define CGTT_SC_CLK_CTRL__SOFT_OVERRIDE2__SHIFT 0x1d
#define CGTT_SC_CLK_CTRL__SOFT_OVERRIDE1_MASK 0x40000000
#define CGTT_SC_CLK_CTRL__SOFT_OVERRIDE1__SHIFT 0x1e
#define CGTT_SC_CLK_CTRL__SOFT_OVERRIDE0_MASK 0x80000000
#define CGTT_SC_CLK_CTRL__SOFT_OVERRIDE0__SHIFT 0x1f
#define PA_SU_DEBUG_CNTL__SU_DEBUG_INDX_MASK 0x1f
#define PA_SU_DEBUG_CNTL__SU_DEBUG_INDX__SHIFT 0x0
#define PA_SU_DEBUG_DATA__DATA_MASK 0xffffffff
#define PA_SU_DEBUG_DATA__DATA__SHIFT 0x0
#define PA_SC_DEBUG_CNTL__SC_DEBUG_INDX_MASK 0x3f
#define PA_SC_DEBUG_CNTL__SC_DEBUG_INDX__SHIFT 0x0
#define PA_SC_DEBUG_DATA__DATA_MASK 0xffffffff
#define PA_SC_DEBUG_DATA__DATA__SHIFT 0x0
#define CLIPPER_DEBUG_REG00__ALWAYS_ZERO_MASK 0xff
#define CLIPPER_DEBUG_REG00__ALWAYS_ZERO__SHIFT 0x0
#define CLIPPER_DEBUG_REG00__clip_ga_bc_fifo_write_MASK 0x100
#define CLIPPER_DEBUG_REG00__clip_ga_bc_fifo_write__SHIFT 0x8
#define CLIPPER_DEBUG_REG00__su_clip_baryc_free_MASK 0x600
#define CLIPPER_DEBUG_REG00__su_clip_baryc_free__SHIFT 0x9
#define CLIPPER_DEBUG_REG00__clip_to_ga_fifo_write_MASK 0x800
#define CLIPPER_DEBUG_REG00__clip_to_ga_fifo_write__SHIFT 0xb
#define CLIPPER_DEBUG_REG00__clip_to_ga_fifo_full_MASK 0x1000
#define CLIPPER_DEBUG_REG00__clip_to_ga_fifo_full__SHIFT 0xc
#define CLIPPER_DEBUG_REG00__primic_to_clprim_fifo_empty_MASK 0x2000
#define CLIPPER_DEBUG_REG00__primic_to_clprim_fifo_empty__SHIFT 0xd
#define CLIPPER_DEBUG_REG00__primic_to_clprim_fifo_full_MASK 0x4000
#define CLIPPER_DEBUG_REG00__primic_to_clprim_fifo_full__SHIFT 0xe
#define CLIPPER_DEBUG_REG00__clip_to_outsm_fifo_empty_MASK 0x8000
#define CLIPPER_DEBUG_REG00__clip_to_outsm_fifo_empty__SHIFT 0xf
#define CLIPPER_DEBUG_REG00__clip_to_outsm_fifo_full_MASK 0x10000
#define CLIPPER_DEBUG_REG00__clip_to_outsm_fifo_full__SHIFT 0x10
#define CLIPPER_DEBUG_REG00__vgt_to_clipp_fifo_empty_MASK 0x20000
#define CLIPPER_DEBUG_REG00__vgt_to_clipp_fifo_empty__SHIFT 0x11
#define CLIPPER_DEBUG_REG00__vgt_to_clipp_fifo_full_MASK 0x40000
#define CLIPPER_DEBUG_REG00__vgt_to_clipp_fifo_full__SHIFT 0x12
#define CLIPPER_DEBUG_REG00__vgt_to_clips_fifo_empty_MASK 0x80000
#define CLIPPER_DEBUG_REG00__vgt_to_clips_fifo_empty__SHIFT 0x13
#define CLIPPER_DEBUG_REG00__vgt_to_clips_fifo_full_MASK 0x100000
#define CLIPPER_DEBUG_REG00__vgt_to_clips_fifo_full__SHIFT 0x14
#define CLIPPER_DEBUG_REG00__clipcode_fifo_fifo_empty_MASK 0x200000
#define CLIPPER_DEBUG_REG00__clipcode_fifo_fifo_empty__SHIFT 0x15
#define CLIPPER_DEBUG_REG00__clipcode_fifo_full_MASK 0x400000
#define CLIPPER_DEBUG_REG00__clipcode_fifo_full__SHIFT 0x16
#define CLIPPER_DEBUG_REG00__vte_out_clip_fifo_fifo_empty_MASK 0x800000
#define CLIPPER_DEBUG_REG00__vte_out_clip_fifo_fifo_empty__SHIFT 0x17
#define CLIPPER_DEBUG_REG00__vte_out_clip_fifo_fifo_full_MASK 0x1000000
#define CLIPPER_DEBUG_REG00__vte_out_clip_fifo_fifo_full__SHIFT 0x18
#define CLIPPER_DEBUG_REG00__vte_out_orig_fifo_fifo_empty_MASK 0x2000000
#define CLIPPER_DEBUG_REG00__vte_out_orig_fifo_fifo_empty__SHIFT 0x19
#define CLIPPER_DEBUG_REG00__vte_out_orig_fifo_fifo_full_MASK 0x4000000
#define CLIPPER_DEBUG_REG00__vte_out_orig_fifo_fifo_full__SHIFT 0x1a
#define CLIPPER_DEBUG_REG00__ccgen_to_clipcc_fifo_empty_MASK 0x8000000
#define CLIPPER_DEBUG_REG00__ccgen_to_clipcc_fifo_empty__SHIFT 0x1b
#define CLIPPER_DEBUG_REG00__ccgen_to_clipcc_fifo_full_MASK 0x10000000
#define CLIPPER_DEBUG_REG00__ccgen_to_clipcc_fifo_full__SHIFT 0x1c
#define CLIPPER_DEBUG_REG00__clip_to_outsm_fifo_write_MASK 0x20000000
#define CLIPPER_DEBUG_REG00__clip_to_outsm_fifo_write__SHIFT 0x1d
#define CLIPPER_DEBUG_REG00__vte_out_orig_fifo_fifo_write_MASK 0x40000000
#define CLIPPER_DEBUG_REG00__vte_out_orig_fifo_fifo_write__SHIFT 0x1e
#define CLIPPER_DEBUG_REG00__vgt_to_clipp_fifo_write_MASK 0x80000000
#define CLIPPER_DEBUG_REG00__vgt_to_clipp_fifo_write__SHIFT 0x1f
#define CLIPPER_DEBUG_REG01__ALWAYS_ZERO_MASK 0xff
#define CLIPPER_DEBUG_REG01__ALWAYS_ZERO__SHIFT 0x0
#define CLIPPER_DEBUG_REG01__clip_extra_bc_valid_MASK 0x700
#define CLIPPER_DEBUG_REG01__clip_extra_bc_valid__SHIFT 0x8
#define CLIPPER_DEBUG_REG01__clip_vert_vte_valid_MASK 0x3800
#define CLIPPER_DEBUG_REG01__clip_vert_vte_valid__SHIFT 0xb
#define CLIPPER_DEBUG_REG01__clip_to_outsm_vertex_deallocate_MASK 0x1c000
#define CLIPPER_DEBUG_REG01__clip_to_outsm_vertex_deallocate__SHIFT 0xe
#define CLIPPER_DEBUG_REG01__clip_to_outsm_deallocate_slot_MASK 0xe0000
#define CLIPPER_DEBUG_REG01__clip_to_outsm_deallocate_slot__SHIFT 0x11
#define CLIPPER_DEBUG_REG01__clip_to_outsm_null_primitive_MASK 0x100000
#define CLIPPER_DEBUG_REG01__clip_to_outsm_null_primitive__SHIFT 0x14
#define CLIPPER_DEBUG_REG01__vte_positions_vte_clip_vte_naninf_kill_2_MASK 0x200000
#define CLIPPER_DEBUG_REG01__vte_positions_vte_clip_vte_naninf_kill_2__SHIFT 0x15
#define CLIPPER_DEBUG_REG01__vte_positions_vte_clip_vte_naninf_kill_1_MASK 0x400000
#define CLIPPER_DEBUG_REG01__vte_positions_vte_clip_vte_naninf_kill_1__SHIFT 0x16
#define CLIPPER_DEBUG_REG01__vte_positions_vte_clip_vte_naninf_kill_0_MASK 0x800000
#define CLIPPER_DEBUG_REG01__vte_positions_vte_clip_vte_naninf_kill_0__SHIFT 0x17
#define CLIPPER_DEBUG_REG01__vte_out_clip_rd_extra_bc_valid_MASK 0x1000000
#define CLIPPER_DEBUG_REG01__vte_out_clip_rd_extra_bc_valid__SHIFT 0x18
#define CLIPPER_DEBUG_REG01__vte_out_clip_rd_vte_naninf_kill_MASK 0x2000000
#define CLIPPER_DEBUG_REG01__vte_out_clip_rd_vte_naninf_kill__SHIFT 0x19
#define CLIPPER_DEBUG_REG01__vte_out_clip_rd_vertex_store_indx_MASK 0xc000000
#define CLIPPER_DEBUG_REG01__vte_out_clip_rd_vertex_store_indx__SHIFT 0x1a
#define CLIPPER_DEBUG_REG01__clip_ga_bc_fifo_write_MASK 0x10000000
#define CLIPPER_DEBUG_REG01__clip_ga_bc_fifo_write__SHIFT 0x1c
#define CLIPPER_DEBUG_REG01__clip_to_ga_fifo_write_MASK 0x20000000
#define CLIPPER_DEBUG_REG01__clip_to_ga_fifo_write__SHIFT 0x1d
#define CLIPPER_DEBUG_REG01__vte_out_clip_fifo_fifo_advanceread_MASK 0x40000000
#define CLIPPER_DEBUG_REG01__vte_out_clip_fifo_fifo_advanceread__SHIFT 0x1e
#define CLIPPER_DEBUG_REG01__vte_out_clip_fifo_fifo_empty_MASK 0x80000000
#define CLIPPER_DEBUG_REG01__vte_out_clip_fifo_fifo_empty__SHIFT 0x1f
#define CLIPPER_DEBUG_REG02__clip_extra_bc_valid_MASK 0x7
#define CLIPPER_DEBUG_REG02__clip_extra_bc_valid__SHIFT 0x0
#define CLIPPER_DEBUG_REG02__clip_vert_vte_valid_MASK 0x38
#define CLIPPER_DEBUG_REG02__clip_vert_vte_valid__SHIFT 0x3
#define CLIPPER_DEBUG_REG02__clip_to_outsm_clip_seq_indx_MASK 0xc0
#define CLIPPER_DEBUG_REG02__clip_to_outsm_clip_seq_indx__SHIFT 0x6
#define CLIPPER_DEBUG_REG02__clip_to_outsm_vertex_store_indx_2_MASK 0xf00
#define CLIPPER_DEBUG_REG02__clip_to_outsm_vertex_store_indx_2__SHIFT 0x8
#define CLIPPER_DEBUG_REG02__clip_to_outsm_vertex_store_indx_1_MASK 0xf000
#define CLIPPER_DEBUG_REG02__clip_to_outsm_vertex_store_indx_1__SHIFT 0xc
#define CLIPPER_DEBUG_REG02__clip_to_outsm_vertex_store_indx_0_MASK 0xf0000
#define CLIPPER_DEBUG_REG02__clip_to_outsm_vertex_store_indx_0__SHIFT 0x10
#define CLIPPER_DEBUG_REG02__clip_to_clipga_extra_bc_coords_MASK 0x100000
#define CLIPPER_DEBUG_REG02__clip_to_clipga_extra_bc_coords__SHIFT 0x14
#define CLIPPER_DEBUG_REG02__clip_to_clipga_vte_naninf_kill_MASK 0x200000
#define CLIPPER_DEBUG_REG02__clip_to_clipga_vte_naninf_kill__SHIFT 0x15
#define CLIPPER_DEBUG_REG02__clip_to_outsm_end_of_packet_MASK 0x400000
#define CLIPPER_DEBUG_REG02__clip_to_outsm_end_of_packet__SHIFT 0x16
#define CLIPPER_DEBUG_REG02__clip_to_outsm_first_prim_of_slot_MASK 0x800000
#define CLIPPER_DEBUG_REG02__clip_to_outsm_first_prim_of_slot__SHIFT 0x17
#define CLIPPER_DEBUG_REG02__clip_to_outsm_clipped_prim_MASK 0x1000000
#define CLIPPER_DEBUG_REG02__clip_to_outsm_clipped_prim__SHIFT 0x18
#define CLIPPER_DEBUG_REG02__clip_to_outsm_null_primitive_MASK 0x2000000
#define CLIPPER_DEBUG_REG02__clip_to_outsm_null_primitive__SHIFT 0x19
#define CLIPPER_DEBUG_REG02__clip_ga_bc_fifo_full_MASK 0x4000000
#define CLIPPER_DEBUG_REG02__clip_ga_bc_fifo_full__SHIFT 0x1a
#define CLIPPER_DEBUG_REG02__clip_to_ga_fifo_full_MASK 0x8000000
#define CLIPPER_DEBUG_REG02__clip_to_ga_fifo_full__SHIFT 0x1b
#define CLIPPER_DEBUG_REG02__clip_ga_bc_fifo_write_MASK 0x10000000
#define CLIPPER_DEBUG_REG02__clip_ga_bc_fifo_write__SHIFT 0x1c
#define CLIPPER_DEBUG_REG02__clip_to_ga_fifo_write_MASK 0x20000000
#define CLIPPER_DEBUG_REG02__clip_to_ga_fifo_write__SHIFT 0x1d
#define CLIPPER_DEBUG_REG02__clip_to_outsm_fifo_advanceread_MASK 0x40000000
#define CLIPPER_DEBUG_REG02__clip_to_outsm_fifo_advanceread__SHIFT 0x1e
#define CLIPPER_DEBUG_REG02__clip_to_outsm_fifo_empty_MASK 0x80000000
#define CLIPPER_DEBUG_REG02__clip_to_outsm_fifo_empty__SHIFT 0x1f
#define CLIPPER_DEBUG_REG03__clipsm0_clprim_to_clip_clip_code_or_MASK 0x3fff
#define CLIPPER_DEBUG_REG03__clipsm0_clprim_to_clip_clip_code_or__SHIFT 0x0
#define CLIPPER_DEBUG_REG03__clipsm0_clprim_to_clip_event_id_MASK 0xfc000
#define CLIPPER_DEBUG_REG03__clipsm0_clprim_to_clip_event_id__SHIFT 0xe
#define CLIPPER_DEBUG_REG03__clipsm0_clprim_to_clip_state_var_indx_MASK 0x700000
#define CLIPPER_DEBUG_REG03__clipsm0_clprim_to_clip_state_var_indx__SHIFT 0x14
#define CLIPPER_DEBUG_REG03__clipsm0_clprim_to_clip_clip_primitive_MASK 0x800000
#define CLIPPER_DEBUG_REG03__clipsm0_clprim_to_clip_clip_primitive__SHIFT 0x17
#define CLIPPER_DEBUG_REG03__clipsm0_clprim_to_clip_deallocate_slot_MASK 0x7000000
#define CLIPPER_DEBUG_REG03__clipsm0_clprim_to_clip_deallocate_slot__SHIFT 0x18
#define CLIPPER_DEBUG_REG03__clipsm0_clprim_to_clip_first_prim_of_slot_MASK 0x8000000
#define CLIPPER_DEBUG_REG03__clipsm0_clprim_to_clip_first_prim_of_slot__SHIFT 0x1b
#define CLIPPER_DEBUG_REG03__clipsm0_clprim_to_clip_end_of_packet_MASK 0x10000000
#define CLIPPER_DEBUG_REG03__clipsm0_clprim_to_clip_end_of_packet__SHIFT 0x1c
#define CLIPPER_DEBUG_REG03__clipsm0_clprim_to_clip_event_MASK 0x20000000
#define CLIPPER_DEBUG_REG03__clipsm0_clprim_to_clip_event__SHIFT 0x1d
#define CLIPPER_DEBUG_REG03__clipsm0_clprim_to_clip_null_primitive_MASK 0x40000000
#define CLIPPER_DEBUG_REG03__clipsm0_clprim_to_clip_null_primitive__SHIFT 0x1e
#define CLIPPER_DEBUG_REG03__clipsm0_clprim_to_clip_prim_valid_MASK 0x80000000
#define CLIPPER_DEBUG_REG03__clipsm0_clprim_to_clip_prim_valid__SHIFT 0x1f
#define CLIPPER_DEBUG_REG04__clipsm0_clprim_to_clip_param_cache_indx_0_MASK 0x7fe
#define CLIPPER_DEBUG_REG04__clipsm0_clprim_to_clip_param_cache_indx_0__SHIFT 0x1
#define CLIPPER_DEBUG_REG04__clipsm0_clprim_to_clip_vertex_store_indx_2_MASK 0x1f800
#define CLIPPER_DEBUG_REG04__clipsm0_clprim_to_clip_vertex_store_indx_2__SHIFT 0xb
#define CLIPPER_DEBUG_REG04__clipsm0_clprim_to_clip_vertex_store_indx_1_MASK 0x7e0000
#define CLIPPER_DEBUG_REG04__clipsm0_clprim_to_clip_vertex_store_indx_1__SHIFT 0x11
#define CLIPPER_DEBUG_REG04__clipsm0_clprim_to_clip_vertex_store_indx_0_MASK 0x1f800000
#define CLIPPER_DEBUG_REG04__clipsm0_clprim_to_clip_vertex_store_indx_0__SHIFT 0x17
#define CLIPPER_DEBUG_REG04__clipsm0_clprim_to_clip_event_MASK 0x20000000
#define CLIPPER_DEBUG_REG04__clipsm0_clprim_to_clip_event__SHIFT 0x1d
#define CLIPPER_DEBUG_REG04__clipsm0_clprim_to_clip_null_primitive_MASK 0x40000000
#define CLIPPER_DEBUG_REG04__clipsm0_clprim_to_clip_null_primitive__SHIFT 0x1e
#define CLIPPER_DEBUG_REG04__clipsm0_clprim_to_clip_prim_valid_MASK 0x80000000
#define CLIPPER_DEBUG_REG04__clipsm0_clprim_to_clip_prim_valid__SHIFT 0x1f
#define CLIPPER_DEBUG_REG05__clipsm1_clprim_to_clip_clip_code_or_MASK 0x3fff
#define CLIPPER_DEBUG_REG05__clipsm1_clprim_to_clip_clip_code_or__SHIFT 0x0
#define CLIPPER_DEBUG_REG05__clipsm1_clprim_to_clip_event_id_MASK 0xfc000
#define CLIPPER_DEBUG_REG05__clipsm1_clprim_to_clip_event_id__SHIFT 0xe
#define CLIPPER_DEBUG_REG05__clipsm1_clprim_to_clip_state_var_indx_MASK 0x700000
#define CLIPPER_DEBUG_REG05__clipsm1_clprim_to_clip_state_var_indx__SHIFT 0x14
#define CLIPPER_DEBUG_REG05__clipsm1_clprim_to_clip_clip_primitive_MASK 0x800000
#define CLIPPER_DEBUG_REG05__clipsm1_clprim_to_clip_clip_primitive__SHIFT 0x17
#define CLIPPER_DEBUG_REG05__clipsm1_clprim_to_clip_deallocate_slot_MASK 0x7000000
#define CLIPPER_DEBUG_REG05__clipsm1_clprim_to_clip_deallocate_slot__SHIFT 0x18
#define CLIPPER_DEBUG_REG05__clipsm1_clprim_to_clip_first_prim_of_slot_MASK 0x8000000
#define CLIPPER_DEBUG_REG05__clipsm1_clprim_to_clip_first_prim_of_slot__SHIFT 0x1b
#define CLIPPER_DEBUG_REG05__clipsm1_clprim_to_clip_end_of_packet_MASK 0x10000000
#define CLIPPER_DEBUG_REG05__clipsm1_clprim_to_clip_end_of_packet__SHIFT 0x1c
#define CLIPPER_DEBUG_REG05__clipsm1_clprim_to_clip_event_MASK 0x20000000
#define CLIPPER_DEBUG_REG05__clipsm1_clprim_to_clip_event__SHIFT 0x1d
#define CLIPPER_DEBUG_REG05__clipsm1_clprim_to_clip_null_primitive_MASK 0x40000000
#define CLIPPER_DEBUG_REG05__clipsm1_clprim_to_clip_null_primitive__SHIFT 0x1e
#define CLIPPER_DEBUG_REG05__clipsm1_clprim_to_clip_prim_valid_MASK 0x80000000
#define CLIPPER_DEBUG_REG05__clipsm1_clprim_to_clip_prim_valid__SHIFT 0x1f
#define CLIPPER_DEBUG_REG06__clipsm1_clprim_to_clip_param_cache_indx_0_MASK 0x7fe
#define CLIPPER_DEBUG_REG06__clipsm1_clprim_to_clip_param_cache_indx_0__SHIFT 0x1
#define CLIPPER_DEBUG_REG06__clipsm1_clprim_to_clip_vertex_store_indx_2_MASK 0x1f800
#define CLIPPER_DEBUG_REG06__clipsm1_clprim_to_clip_vertex_store_indx_2__SHIFT 0xb
#define CLIPPER_DEBUG_REG06__clipsm1_clprim_to_clip_vertex_store_indx_1_MASK 0x7e0000
#define CLIPPER_DEBUG_REG06__clipsm1_clprim_to_clip_vertex_store_indx_1__SHIFT 0x11
#define CLIPPER_DEBUG_REG06__clipsm1_clprim_to_clip_vertex_store_indx_0_MASK 0x1f800000
#define CLIPPER_DEBUG_REG06__clipsm1_clprim_to_clip_vertex_store_indx_0__SHIFT 0x17
#define CLIPPER_DEBUG_REG06__clipsm1_clprim_to_clip_event_MASK 0x20000000
#define CLIPPER_DEBUG_REG06__clipsm1_clprim_to_clip_event__SHIFT 0x1d
#define CLIPPER_DEBUG_REG06__clipsm1_clprim_to_clip_null_primitive_MASK 0x40000000
#define CLIPPER_DEBUG_REG06__clipsm1_clprim_to_clip_null_primitive__SHIFT 0x1e
#define CLIPPER_DEBUG_REG06__clipsm1_clprim_to_clip_prim_valid_MASK 0x80000000
#define CLIPPER_DEBUG_REG06__clipsm1_clprim_to_clip_prim_valid__SHIFT 0x1f
#define CLIPPER_DEBUG_REG07__clipsm2_clprim_to_clip_clip_code_or_MASK 0x3fff
#define CLIPPER_DEBUG_REG07__clipsm2_clprim_to_clip_clip_code_or__SHIFT 0x0
#define CLIPPER_DEBUG_REG07__clipsm2_clprim_to_clip_event_id_MASK 0xfc000
#define CLIPPER_DEBUG_REG07__clipsm2_clprim_to_clip_event_id__SHIFT 0xe
#define CLIPPER_DEBUG_REG07__clipsm2_clprim_to_clip_state_var_indx_MASK 0x700000
#define CLIPPER_DEBUG_REG07__clipsm2_clprim_to_clip_state_var_indx__SHIFT 0x14
#define CLIPPER_DEBUG_REG07__clipsm2_clprim_to_clip_clip_primitive_MASK 0x800000
#define CLIPPER_DEBUG_REG07__clipsm2_clprim_to_clip_clip_primitive__SHIFT 0x17
#define CLIPPER_DEBUG_REG07__clipsm2_clprim_to_clip_deallocate_slot_MASK 0x7000000
#define CLIPPER_DEBUG_REG07__clipsm2_clprim_to_clip_deallocate_slot__SHIFT 0x18
#define CLIPPER_DEBUG_REG07__clipsm2_clprim_to_clip_first_prim_of_slot_MASK 0x8000000
#define CLIPPER_DEBUG_REG07__clipsm2_clprim_to_clip_first_prim_of_slot__SHIFT 0x1b
#define CLIPPER_DEBUG_REG07__clipsm2_clprim_to_clip_end_of_packet_MASK 0x10000000
#define CLIPPER_DEBUG_REG07__clipsm2_clprim_to_clip_end_of_packet__SHIFT 0x1c
#define CLIPPER_DEBUG_REG07__clipsm2_clprim_to_clip_event_MASK 0x20000000
#define CLIPPER_DEBUG_REG07__clipsm2_clprim_to_clip_event__SHIFT 0x1d
#define CLIPPER_DEBUG_REG07__clipsm2_clprim_to_clip_null_primitive_MASK 0x40000000
#define CLIPPER_DEBUG_REG07__clipsm2_clprim_to_clip_null_primitive__SHIFT 0x1e
#define CLIPPER_DEBUG_REG07__clipsm2_clprim_to_clip_prim_valid_MASK 0x80000000
#define CLIPPER_DEBUG_REG07__clipsm2_clprim_to_clip_prim_valid__SHIFT 0x1f
#define CLIPPER_DEBUG_REG08__clipsm2_clprim_to_clip_param_cache_indx_0_MASK 0x7fe
#define CLIPPER_DEBUG_REG08__clipsm2_clprim_to_clip_param_cache_indx_0__SHIFT 0x1
#define CLIPPER_DEBUG_REG08__clipsm2_clprim_to_clip_vertex_store_indx_2_MASK 0x1f800
#define CLIPPER_DEBUG_REG08__clipsm2_clprim_to_clip_vertex_store_indx_2__SHIFT 0xb
#define CLIPPER_DEBUG_REG08__clipsm2_clprim_to_clip_vertex_store_indx_1_MASK 0x7e0000
#define CLIPPER_DEBUG_REG08__clipsm2_clprim_to_clip_vertex_store_indx_1__SHIFT 0x11
#define CLIPPER_DEBUG_REG08__clipsm2_clprim_to_clip_vertex_store_indx_0_MASK 0x1f800000
#define CLIPPER_DEBUG_REG08__clipsm2_clprim_to_clip_vertex_store_indx_0__SHIFT 0x17
#define CLIPPER_DEBUG_REG08__clipsm2_clprim_to_clip_event_MASK 0x20000000
#define CLIPPER_DEBUG_REG08__clipsm2_clprim_to_clip_event__SHIFT 0x1d
#define CLIPPER_DEBUG_REG08__clipsm2_clprim_to_clip_null_primitive_MASK 0x40000000
#define CLIPPER_DEBUG_REG08__clipsm2_clprim_to_clip_null_primitive__SHIFT 0x1e
#define CLIPPER_DEBUG_REG08__clipsm2_clprim_to_clip_prim_valid_MASK 0x80000000
#define CLIPPER_DEBUG_REG08__clipsm2_clprim_to_clip_prim_valid__SHIFT 0x1f
#define CLIPPER_DEBUG_REG09__clipsm3_clprim_to_clip_clip_code_or_MASK 0x3fff
#define CLIPPER_DEBUG_REG09__clipsm3_clprim_to_clip_clip_code_or__SHIFT 0x0
#define CLIPPER_DEBUG_REG09__clipsm3_clprim_to_clip_event_id_MASK 0xfc000
#define CLIPPER_DEBUG_REG09__clipsm3_clprim_to_clip_event_id__SHIFT 0xe
#define CLIPPER_DEBUG_REG09__clipsm3_clprim_to_clip_state_var_indx_MASK 0x700000
#define CLIPPER_DEBUG_REG09__clipsm3_clprim_to_clip_state_var_indx__SHIFT 0x14
#define CLIPPER_DEBUG_REG09__clipsm3_clprim_to_clip_clip_primitive_MASK 0x800000
#define CLIPPER_DEBUG_REG09__clipsm3_clprim_to_clip_clip_primitive__SHIFT 0x17
#define CLIPPER_DEBUG_REG09__clipsm3_clprim_to_clip_deallocate_slot_MASK 0x7000000
#define CLIPPER_DEBUG_REG09__clipsm3_clprim_to_clip_deallocate_slot__SHIFT 0x18
#define CLIPPER_DEBUG_REG09__clipsm3_clprim_to_clip_first_prim_of_slot_MASK 0x8000000
#define CLIPPER_DEBUG_REG09__clipsm3_clprim_to_clip_first_prim_of_slot__SHIFT 0x1b
#define CLIPPER_DEBUG_REG09__clipsm3_clprim_to_clip_end_of_packet_MASK 0x10000000
#define CLIPPER_DEBUG_REG09__clipsm3_clprim_to_clip_end_of_packet__SHIFT 0x1c
#define CLIPPER_DEBUG_REG09__clipsm3_clprim_to_clip_event_MASK 0x20000000
#define CLIPPER_DEBUG_REG09__clipsm3_clprim_to_clip_event__SHIFT 0x1d
#define CLIPPER_DEBUG_REG09__clipsm3_clprim_to_clip_null_primitive_MASK 0x40000000
#define CLIPPER_DEBUG_REG09__clipsm3_clprim_to_clip_null_primitive__SHIFT 0x1e
#define CLIPPER_DEBUG_REG09__clipsm3_clprim_to_clip_prim_valid_MASK 0x80000000
#define CLIPPER_DEBUG_REG09__clipsm3_clprim_to_clip_prim_valid__SHIFT 0x1f
#define CLIPPER_DEBUG_REG10__clipsm3_clprim_to_clip_param_cache_indx_0_MASK 0x7fe
#define CLIPPER_DEBUG_REG10__clipsm3_clprim_to_clip_param_cache_indx_0__SHIFT 0x1
#define CLIPPER_DEBUG_REG10__clipsm3_clprim_to_clip_vertex_store_indx_2_MASK 0x1f800
#define CLIPPER_DEBUG_REG10__clipsm3_clprim_to_clip_vertex_store_indx_2__SHIFT 0xb
#define CLIPPER_DEBUG_REG10__clipsm3_clprim_to_clip_vertex_store_indx_1_MASK 0x7e0000
#define CLIPPER_DEBUG_REG10__clipsm3_clprim_to_clip_vertex_store_indx_1__SHIFT 0x11
#define CLIPPER_DEBUG_REG10__clipsm3_clprim_to_clip_vertex_store_indx_0_MASK 0x1f800000
#define CLIPPER_DEBUG_REG10__clipsm3_clprim_to_clip_vertex_store_indx_0__SHIFT 0x17
#define CLIPPER_DEBUG_REG10__clipsm3_clprim_to_clip_event_MASK 0x20000000
#define CLIPPER_DEBUG_REG10__clipsm3_clprim_to_clip_event__SHIFT 0x1d
#define CLIPPER_DEBUG_REG10__clipsm3_clprim_to_clip_null_primitive_MASK 0x40000000
#define CLIPPER_DEBUG_REG10__clipsm3_clprim_to_clip_null_primitive__SHIFT 0x1e
#define CLIPPER_DEBUG_REG10__clipsm3_clprim_to_clip_prim_valid_MASK 0x80000000
#define CLIPPER_DEBUG_REG10__clipsm3_clprim_to_clip_prim_valid__SHIFT 0x1f
#define CLIPPER_DEBUG_REG11__clipsm3_clip_to_clipga_event_MASK 0x1
#define CLIPPER_DEBUG_REG11__clipsm3_clip_to_clipga_event__SHIFT 0x0
#define CLIPPER_DEBUG_REG11__clipsm2_clip_to_clipga_event_MASK 0x2
#define CLIPPER_DEBUG_REG11__clipsm2_clip_to_clipga_event__SHIFT 0x1
#define CLIPPER_DEBUG_REG11__clipsm1_clip_to_clipga_event_MASK 0x4
#define CLIPPER_DEBUG_REG11__clipsm1_clip_to_clipga_event__SHIFT 0x2
#define CLIPPER_DEBUG_REG11__clipsm0_clip_to_clipga_event_MASK 0x8
#define CLIPPER_DEBUG_REG11__clipsm0_clip_to_clipga_event__SHIFT 0x3
#define CLIPPER_DEBUG_REG11__clipsm3_clip_to_clipga_clip_primitive_MASK 0x10
#define CLIPPER_DEBUG_REG11__clipsm3_clip_to_clipga_clip_primitive__SHIFT 0x4
#define CLIPPER_DEBUG_REG11__clipsm2_clip_to_clipga_clip_primitive_MASK 0x20
#define CLIPPER_DEBUG_REG11__clipsm2_clip_to_clipga_clip_primitive__SHIFT 0x5
#define CLIPPER_DEBUG_REG11__clipsm1_clip_to_clipga_clip_primitive_MASK 0x40
#define CLIPPER_DEBUG_REG11__clipsm1_clip_to_clipga_clip_primitive__SHIFT 0x6
#define CLIPPER_DEBUG_REG11__clipsm0_clip_to_clipga_clip_primitive_MASK 0x80
#define CLIPPER_DEBUG_REG11__clipsm0_clip_to_clipga_clip_primitive__SHIFT 0x7
#define CLIPPER_DEBUG_REG11__clipsm3_clip_to_clipga_clip_to_outsm_cnt_MASK 0xf00
#define CLIPPER_DEBUG_REG11__clipsm3_clip_to_clipga_clip_to_outsm_cnt__SHIFT 0x8
#define CLIPPER_DEBUG_REG11__clipsm2_clip_to_clipga_clip_to_outsm_cnt_MASK 0xf000
#define CLIPPER_DEBUG_REG11__clipsm2_clip_to_clipga_clip_to_outsm_cnt__SHIFT 0xc
#define CLIPPER_DEBUG_REG11__clipsm1_clip_to_clipga_clip_to_outsm_cnt_MASK 0xf0000
#define CLIPPER_DEBUG_REG11__clipsm1_clip_to_clipga_clip_to_outsm_cnt__SHIFT 0x10
#define CLIPPER_DEBUG_REG11__clipsm0_clip_to_clipga_clip_to_outsm_cnt_MASK 0xf00000
#define CLIPPER_DEBUG_REG11__clipsm0_clip_to_clipga_clip_to_outsm_cnt__SHIFT 0x14
#define CLIPPER_DEBUG_REG11__clipsm3_clip_to_clipga_prim_valid_MASK 0x1000000
#define CLIPPER_DEBUG_REG11__clipsm3_clip_to_clipga_prim_valid__SHIFT 0x18
#define CLIPPER_DEBUG_REG11__clipsm2_clip_to_clipga_prim_valid_MASK 0x2000000
#define CLIPPER_DEBUG_REG11__clipsm2_clip_to_clipga_prim_valid__SHIFT 0x19
#define CLIPPER_DEBUG_REG11__clipsm1_clip_to_clipga_prim_valid_MASK 0x4000000
#define CLIPPER_DEBUG_REG11__clipsm1_clip_to_clipga_prim_valid__SHIFT 0x1a
#define CLIPPER_DEBUG_REG11__clipsm0_clip_to_clipga_prim_valid_MASK 0x8000000
#define CLIPPER_DEBUG_REG11__clipsm0_clip_to_clipga_prim_valid__SHIFT 0x1b
#define CLIPPER_DEBUG_REG11__clipsm3_inc_clip_to_clipga_clip_to_outsm_cnt_MASK 0x10000000
#define CLIPPER_DEBUG_REG11__clipsm3_inc_clip_to_clipga_clip_to_outsm_cnt__SHIFT 0x1c
#define CLIPPER_DEBUG_REG11__clipsm2_inc_clip_to_clipga_clip_to_outsm_cnt_MASK 0x20000000
#define CLIPPER_DEBUG_REG11__clipsm2_inc_clip_to_clipga_clip_to_outsm_cnt__SHIFT 0x1d
#define CLIPPER_DEBUG_REG11__clipsm1_inc_clip_to_clipga_clip_to_outsm_cnt_MASK 0x40000000
#define CLIPPER_DEBUG_REG11__clipsm1_inc_clip_to_clipga_clip_to_outsm_cnt__SHIFT 0x1e
#define CLIPPER_DEBUG_REG11__clipsm0_inc_clip_to_clipga_clip_to_outsm_cnt_MASK 0x80000000
#define CLIPPER_DEBUG_REG11__clipsm0_inc_clip_to_clipga_clip_to_outsm_cnt__SHIFT 0x1f
#define CLIPPER_DEBUG_REG12__ALWAYS_ZERO_MASK 0xff
#define CLIPPER_DEBUG_REG12__ALWAYS_ZERO__SHIFT 0x0
#define CLIPPER_DEBUG_REG12__clip_priority_available_vte_out_clip_MASK 0x1f00
#define CLIPPER_DEBUG_REG12__clip_priority_available_vte_out_clip__SHIFT 0x8
#define CLIPPER_DEBUG_REG12__clip_priority_available_clip_verts_MASK 0x3e000
#define CLIPPER_DEBUG_REG12__clip_priority_available_clip_verts__SHIFT 0xd
#define CLIPPER_DEBUG_REG12__clip_priority_seq_indx_out_MASK 0xc0000
#define CLIPPER_DEBUG_REG12__clip_priority_seq_indx_out__SHIFT 0x12
#define CLIPPER_DEBUG_REG12__clip_priority_seq_indx_vert_MASK 0x300000
#define CLIPPER_DEBUG_REG12__clip_priority_seq_indx_vert__SHIFT 0x14
#define CLIPPER_DEBUG_REG12__clip_priority_seq_indx_load_MASK 0xc00000
#define CLIPPER_DEBUG_REG12__clip_priority_seq_indx_load__SHIFT 0x16
#define CLIPPER_DEBUG_REG12__clipsm3_clprim_to_clip_clip_primitive_MASK 0x1000000
#define CLIPPER_DEBUG_REG12__clipsm3_clprim_to_clip_clip_primitive__SHIFT 0x18
#define CLIPPER_DEBUG_REG12__clipsm3_clprim_to_clip_prim_valid_MASK 0x2000000
#define CLIPPER_DEBUG_REG12__clipsm3_clprim_to_clip_prim_valid__SHIFT 0x19
#define CLIPPER_DEBUG_REG12__clipsm2_clprim_to_clip_clip_primitive_MASK 0x4000000
#define CLIPPER_DEBUG_REG12__clipsm2_clprim_to_clip_clip_primitive__SHIFT 0x1a
#define CLIPPER_DEBUG_REG12__clipsm2_clprim_to_clip_prim_valid_MASK 0x8000000
#define CLIPPER_DEBUG_REG12__clipsm2_clprim_to_clip_prim_valid__SHIFT 0x1b
#define CLIPPER_DEBUG_REG12__clipsm1_clprim_to_clip_clip_primitive_MASK 0x10000000
#define CLIPPER_DEBUG_REG12__clipsm1_clprim_to_clip_clip_primitive__SHIFT 0x1c
#define CLIPPER_DEBUG_REG12__clipsm1_clprim_to_clip_prim_valid_MASK 0x20000000
#define CLIPPER_DEBUG_REG12__clipsm1_clprim_to_clip_prim_valid__SHIFT 0x1d
#define CLIPPER_DEBUG_REG12__clipsm0_clprim_to_clip_clip_primitive_MASK 0x40000000
#define CLIPPER_DEBUG_REG12__clipsm0_clprim_to_clip_clip_primitive__SHIFT 0x1e
#define CLIPPER_DEBUG_REG12__clipsm0_clprim_to_clip_prim_valid_MASK 0x80000000
#define CLIPPER_DEBUG_REG12__clipsm0_clprim_to_clip_prim_valid__SHIFT 0x1f
#define CLIPPER_DEBUG_REG13__clprim_in_back_state_var_indx_MASK 0x7
#define CLIPPER_DEBUG_REG13__clprim_in_back_state_var_indx__SHIFT 0x0
#define CLIPPER_DEBUG_REG13__point_clip_candidate_MASK 0x8
#define CLIPPER_DEBUG_REG13__point_clip_candidate__SHIFT 0x3
#define CLIPPER_DEBUG_REG13__prim_nan_kill_MASK 0x10
#define CLIPPER_DEBUG_REG13__prim_nan_kill__SHIFT 0x4
#define CLIPPER_DEBUG_REG13__clprim_clip_primitive_MASK 0x20
#define CLIPPER_DEBUG_REG13__clp