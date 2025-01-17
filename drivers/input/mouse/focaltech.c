sh_request__SHIFT 0x12
#define GDS_DEBUG_REG0__wr_buffer_wr_complete_MASK 0x80000
#define GDS_DEBUG_REG0__wr_buffer_wr_complete__SHIFT 0x13
#define GDS_DEBUG_REG0__wbuf_fifo_empty_MASK 0x100000
#define GDS_DEBUG_REG0__wbuf_fifo_empty__SHIFT 0x14
#define GDS_DEBUG_REG0__wbuf_fifo_full_MASK 0x200000
#define GDS_DEBUG_REG0__wbuf_fifo_full__SHIFT 0x15
#define GDS_DEBUG_REG0__spare_MASK 0xffc00000
#define GDS_DEBUG_REG0__spare__SHIFT 0x16
#define GDS_DEBUG_REG1__tag_hit_MASK 0x1
#define GDS_DEBUG_REG1__tag_hit__SHIFT 0x0
#define GDS_DEBUG_REG1__tag_miss_MASK 0x2
#define GDS_DEBUG_REG1__tag_miss__SHIFT 0x1
#define GDS_DEBUG_REG1__pixel_addr_MASK 0x1fffc
#define GDS_DEBUG_REG1__pixel_addr__SHIFT 0x2
#define GDS_DEBUG_REG1__pixel_vld_MASK 0x20000
#define GDS_DEBUG_REG1__pixel_vld__SHIFT 0x11
#define GDS_DEBUG_REG1__data_ready_MASK 0x40000
#define GDS_DEBUG_REG1__data_ready__SHIFT 0x12
#define GDS_DEBUG_REG1__awaiting_data_MASK 0x80000
#define GDS_DEBUG_REG1__awaiting_data__SHIFT 0x13
#define GDS_DEBUG_REG1__addr_fifo_full_MASK 0x100000
#define GDS_DEBUG_REG1__addr_fifo_full__SHIFT 0x14
#define GDS_DEBUG_REG1__addr_fifo_empty_MASK 0x200000
#define GDS_DEBUG_REG1__addr_fifo_empty__SHIFT 0x15
#define GDS_DEBUG_REG1__buffer_loaded_MASK 0x400000
#define GDS_DEBUG_REG1__buffer_loaded__SHIFT 0x16
#define GDS_DEBUG_REG1__buffer_invalid_MASK 0x800000
#define GDS_DEBUG_REG1__buffer_invalid__SHIFT 0x17
#define GDS_DEBUG_REG1__spare_MASK 0xff000000
#define GDS_DEBUG_REG1__spare__SHIFT 0x18
#define GDS_DEBUG_REG2__ds_full_MASK 0x1
#define GDS_DEBUG_REG2__ds_full__SHIFT 0x0
#define GDS_DEBUG_REG2__ds_credit_avail_MASK 0x2
#define GDS_DEBUG_REG2__ds_credit_avail__SHIFT 0x1
#define GDS_DEBUG_REG2__ord_idx_free_MASK 0x4
#define GDS_DEBUG_REG2__ord_idx_free__SHIFT 0x2
#define GDS_DEBUG_REG2__cmd_write_MASK 0x8
#define GDS_DEBUG_REG2__cmd_write__SHIFT 0x3
#define GDS_DEBUG_REG2__app_sel_MASK 0xf0
#define GDS_DEBUG_REG2__app_sel__SHIFT 0x4
#define GDS_DEBUG_REG2__req_MASK 0x7fff00
#define GDS_DEBUG_REG2__req__SHIFT 0x8
#define GDS_DEBUG_REG2__spare_MASK 0xff800000
#define GDS_DEBUG_REG2__spare__SHIFT 0x17
#define GDS_DEBUG_REG3__pipe_num_busy_MASK 0x7ff
#define GDS_DEBUG_REG3__pipe_num_busy__SHIFT 0x0
#define GDS_DEBUG_REG3__pipe0_busy_num_MASK 0x7800
#define GDS_DEBUG_REG3__pipe0_busy_num__SHIFT 0xb
#define GDS_DEBUG_REG3__spare_MASK 0xffff8000
#define GDS_DEBUG_REG3__spare__SHIFT 0xf
#define GDS_DEBUG_REG4__gws_busy_MASK 0x1
#define GDS_DEBUG_REG4__gws_busy__SHIFT 0x0
#define GDS_DEBUG_REG4__gws_req_MASK 0x2
#define GDS_DEBUG_REG4__gws_req__SHIFT 0x1
#define GDS_DEBUG_REG4__gws_out_stall_MASK 0x4
#define GDS_DEBUG_REG4__gws_out_stall__SHIFT 0x2
#define GDS_DEBUG_REG4__cur_reso_MASK 0x1f8
#define GDS_DEBUG_REG4__cur_reso__SHIFT 0x3
#define GDS_DEBUG_REG4__cur_reso_head_valid_MASK 0x200
#define GDS_DEBUG_REG4__cur_reso_head_valid__SHIFT 0x9
#define GDS_DEBUG_REG4__cur_reso_head_dirty_MASK 0x400
#define GDS_DEBUG_REG4__cur_reso_head_dirty__SHIFT 0xa
#define GDS_DEBUG_REG4__cur_reso_head_flag_MASK 0x800
#define GDS_DEBUG_REG4__cur_reso_head_flag__SHIFT 0xb
#define GDS_DEBUG_REG4__cur_reso_fed_MASK 0x1000
#define GDS_DEBUG_REG4__cur_reso_fed__SHIFT 0xc
#define GDS_DEBUG_REG4__cur_reso_barrier_MASK 0x2000
#define GDS_DEBUG_REG4__cur_reso_barrier__SHIFT 0xd
#define GDS_DEBUG_REG4__cur_reso_flag_MASK 0x4000
#define GDS_DEBUG_REG4__cur_reso_flag__SHIFT 0xe
#define GDS_DEBUG_REG4__cur_reso_cnt_gt0_MASK 0x8000
#define GDS_DEBUG_REG4__cur_reso_cnt_gt0__SHIFT 0xf
#define GDS_DEBUG_REG4__credit_cnt_gt0_MASK 0x10000
#define GDS_DEBUG_REG4__credit_cnt_gt0__SHIFT 0x10
#define GDS_DEBUG_REG4__cmd_write_MASK 0x20000
#define GDS_DEBUG_REG4__cmd_write__SHIFT 0x11
#define GDS_DEBUG_REG4__grbm_gws_reso_wr_MASK 0x40000
#define GDS_DEBUG_REG4__grbm_gws_reso_wr__SHIFT 0x12
#define GDS_DEBUG_REG4__grbm_gws_reso_rd_MASK 0x80000
#define GDS_DEBUG_REG4__grbm_gws_reso_rd__SHIFT 0x13
#define GDS_DEBUG_REG4__ram_read_busy_MASK 0x100000
#define GDS_DEBUG_REG4__ram_read_busy__SHIFT 0x14
#define GDS_DEBUG_REG4__gws_bulkfree_MASK 0x200000
#define GDS_DEBUG_REG4__gws_bulkfree__SHIFT 0x15
#define GDS_DEBUG_REG4__ram_gws_re_MASK 0x400000
#define GDS_DEBUG_REG4__ram_gws_re__SHIFT 0x16
#define GDS_DEBUG_REG4__ram_gws_we_MASK 0x800000
#define GDS_DEBUG_REG4__ram_gws_we__SHIFT 0x17
#define GDS_DEBUG_REG4__spare_MASK 0xff000000
#define GDS_DEBUG_REG4__spare__SHIFT 0x18
#define GDS_DEBUG_REG5__write_dis_MASK 0x1
#define GDS_DEBUG_REG5__write_dis__SHIFT 0x0
#define GDS_DEBUG_REG5__dec_error_MASK 0x2
#define GDS_DEBUG_REG5__dec_error__SHIFT 0x1
#define GDS_DEBUG_REG5__alloc_opco_error_MASK 0x4
#define GDS_DEBUG_REG5__alloc_opco_error__SHIFT 0x2
#define GDS_DEBUG_REG5__dealloc_opco_error_MASK 0x8
#define GDS_DEBUG_REG5__dealloc_opco_error__SHIFT 0x3
#define GDS_DEBUG_REG5__wrap_opco_error_MASK 0x10
#define GDS_DEBUG_REG5__wrap_opco_error__SHIFT 0x4
#define GDS_DEBUG_REG5__spare_MASK 0xe0
#define GDS_DEBUG_REG5__spare__SHIFT 0x5
#define GDS_DEBUG_REG5__error_ds_address_MASK 0x3fff00
#define GDS_DEBUG_REG5__error_ds_address__SHIFT 0x8
#define GDS_DEBUG_REG5__spare1_MASK 0xffc00000
#define GDS_DEBUG_REG5__spare1__SHIFT 0x16
#define GDS_DEBUG_REG6__oa_busy_MASK 0x1
#define GDS_DEBUG_REG6__oa_busy__SHIFT 0x0
#define GDS_DEBUG_REG6__counters_enabled_MASK 0x1e
#define GDS_DEBUG_REG6__counters_enabled__SHIFT 0x1
#define GDS_DEBUG_REG6__counters_busy_MASK 0x1fffe0
#define GDS_DEBUG_REG6__counters_busy__SHIFT 0x5
#define GDS_DEBUG_REG6__spare_MASK 0xffe00000
#define GDS_DEBUG_REG6__spare__SHIFT 0x15
#define GDS_PERFCOUNTER0_SELECT__PERFCOUNTER_SELECT_MASK 0x3ff
#define GDS_PERFCOUNTER0_SELECT__PERFCOUNTER_SELECT__SHIFT 0x0
#define GDS_PERFCOUNTER0_SELECT__PERFCOUNTER_SELECT1_MASK 0xffc00
#define GDS_PERFCOUNTER0_SELECT__PERFCOUNTER_SELECT1__SHIFT 0xa
#define GDS_PERFCOUNTER0_SELECT__CNTR_MODE_MASK 0xf00000
#define GDS_PERFCOUNTER0_SELECT__CNTR_MODE__SHIFT 0x14
#define GDS_PERFCOUNTER1_SELECT__PERFCOUNTER_SELECT_MASK 0x3ff
#define GDS_PERFCOUNTER1_SELECT__PERFCOUNTER_SELECT__SHIFT 0x0
#define GDS_PERFCOUNTER1_SELECT__PERFCOUNTER_SELECT1_MASK 0xffc00
#define GDS_PERFCOUNTER1_SELECT__PERFCOUNTER_SELECT1__SHIFT 0xa
#define GDS_PERFCOUNTER1_SELECT__CNTR_MODE_MASK 0xf00000
#define GDS_PERFCOUNTER1_SELECT__CNTR_MODE__SHIFT 0x14
#define GDS_PERFCOUNTER2_SELECT__PERFCOUNTER_SELECT_MASK 0x3ff
#define GDS_PERFCOUNTER2_SELECT__PERFCOUNTER_SELECT__SHIFT 0x0
#define GDS_PERFCOUNTER2_SELECT__PERFCOUNTER_SELECT1_MASK 0xffc00
#define GDS_PERFCOUNTER2_SELECT__PERFCOUNTER_SELECT1__SHIFT 0xa
#define GDS_PERFCOUNTER2_SELECT__CNTR_MODE_MASK 0xf00000
#define GDS_PERFCOUNTER2_SELECT__CNTR_MODE__SHIFT 0x14
#define GDS_PERFCOUNTER3_SELECT__PERFCOUNTER_SELECT_MASK 0x3ff
#define GDS_PERFCOUNTER3_SELECT__PERFCOUNTER_SELECT__SHIFT 0x0
#define GDS_PERFCOUNTER3_SELECT__PERFCOUNTER_SELECT1_MASK 0xffc00
#define GDS_PERFCOUNTER3_SELECT__PERFCOUNTER_SELECT1__SHIFT 0xa
#define GDS_PERFCOUNTER3_SELECT__CNTR_MODE_MASK 0xf00000
#define GDS_PERFCOUNTER3_SELECT__CNTR_MODE__SHIFT 0x14
#define GDS_PERFCOUNTER0_LO__PERFCOUNTER_LO_MASK 0xffffffff
#define GDS_PERFCOUNTER0_LO__PERFCOUNTER_LO__SHIFT 0x0
#define GDS_PERFCOUNTER1_LO__PERFCOUNTER_LO_MASK 0xffffffff
#define GDS_PERFCOUNTER1_LO__PERFCOUNTER_LO__SHIFT 0x0
#define GDS_PERFCOUNTER2_LO__PERFCOUNTER_LO_MASK 0xffffffff
#define GDS_PERFCOUNTER2_LO__PERFCOUNTER_LO__SHIFT 0x0
#define GDS_PERFCOUNTER3_LO__PERFCOUNTER_LO_MASK 0xffffffff
#define GDS_PERFCOUNTER3_LO__PERFCOUNTER_LO__SHIFT 0x0
#define GDS_PERFCOUNTER0_HI__PERFCOUNTER_HI_MASK 0xffffffff
#define GDS_PERFCOUNTER0_HI__PERFCOUNTER_HI__SHIFT 0x0
#define GDS_PERFCOUNTER1_HI__PERFCOUNTER_HI_MASK 0xffffffff
#define GDS_PERFCOUNTER1_HI__PERFCOUNTER_HI__SHIFT 0x0
#define GDS_PERFCOUNTER2_HI__PERFCOUNTER_HI_MASK 0xffffffff
#define GDS_PERFCOUNTER2_HI__PERFCOUNTER_HI__SHIFT 0x0
#define GDS_PERFCOUNTER3_HI__PERFCOUNTER_HI_MASK 0xffffffff
#define GDS_PERFCOUNTER3_HI__PERFCOUNTER_HI__SHIFT 0x0
#define GDS_PERFCOUNTER0_SELECT1__PERFCOUNTER_SELECT2_MASK 0x3ff
#define GDS_PERFCOUNTER0_SELECT1__PERFCOUNTER_SELECT2__SHIFT 0x0
#define GDS_PERFCOUNTER0_SELECT1__PERFCOUNTER_SELECT3_MASK 0xffc00
#define GDS_PERFCOUNTER0_SELECT1__PERFCOUNTER_SELECT3__SHIFT 0xa
#define GDS_VMID0_BASE__BASE_MASK 0xffff
#define GDS_VMID0_BASE__BASE__SHIFT 0x0
#define GDS_VMID1_BASE__BASE_MASK 0xffff
#define GDS_VMID1_BASE__BASE__SHIFT 0x0
#define GDS_VMID2_BASE__BASE_MASK 0xffff
#define GDS_VMID2_BASE__BASE__SHIFT 0x0
#define GDS_VMID3_BASE__BASE_MASK 0xffff
#define GDS_VMID3_BASE__BASE__SHIFT 0x0
#define GDS_VMID4_BASE__BASE_MASK 0xffff
#define GDS_VMID4_BASE__BASE__SHIFT 0x0
#define GDS_VMID5_BASE__BASE_MASK 0xffff
#define GDS_VMID5_BASE__BASE__SHIFT 0x0
#define GDS_VMID6_BASE__BASE_MASK 0xffff
#define GDS_VMID6_BASE__BASE__SHIFT 0x0
#define GDS_VMID7_BASE__BASE_MASK 0xffff
#define GDS_VMID7_BASE__BASE__SHIFT 0x0
#define GDS_VMID8_BASE__BASE_MASK 0xffff
#define GDS_VMID8_BASE__BASE__SHIFT 0x0
#define GDS_VMID9_BASE__BASE_MASK 0xffff
#define GDS_VMID9_BASE__BASE__SHIFT 0x0
#define GDS_VMID10_BASE__BASE_MASK 0xffff
#define GDS_VMID10_BASE__BASE__SHIFT 0x0
#define GDS_VMID11_BASE__BASE_MASK 0xffff
#define GDS_VMID11_BASE__BASE__SHIFT 0x0
#define GDS_VMID12_BASE__BASE_MASK 0xffff
#define GDS_VMID12_BASE__BASE__SHIFT 0x0
#define GDS_VMID13_BASE__BASE_MASK 0xffff
#define GDS_VMID13_BASE__BASE__SHIFT 0x0
#define GDS_VMID14_BASE__BASE_MASK 0xffff
#define GDS_VMID14_BASE__BASE__SHIFT 0x0
#define GDS_VMID15_BASE__BASE_MASK 0xffff
#define GDS_VMID15_BASE__BASE__SHIFT 0x0
#define GDS_VMID0_SIZE__SIZE_MASK 0x1ffff
#define GDS_VMID0_SIZE__SIZE__SHIFT 0x0
#define GDS_VMID1_SIZE__SIZE_MASK 0x1ffff
#define GDS_VMID1_SIZE__SIZE__SHIFT 0x0
#define GDS_VMID2_SIZE__SIZE_MASK 0x1ffff
#define GDS_VMID2_SIZE__SIZE__SHIFT 0x0
#define GDS_VMID3_SIZE__SIZE_MASK 0x1ffff
#define GDS_VMID3_SIZE__SIZE__SHIFT 0x0
#define GDS_VMID4_SIZE__SIZE_MASK 0x1ffff
#define GDS_VMID4_SIZE__SIZE__SHIFT 0x0
#define GDS_VMID5_SIZE__SIZE_MASK 0x1ffff
#define GDS_VMID5_SIZE__SIZE__SHIFT 0x0
#define GDS_VMID6_SIZE__SIZE_MASK 0x1ffff
#define GDS_VMID6_SIZE__SIZE__SHIFT 0x0
#define GDS_VMID7_SIZE__SIZE_MASK 0x1ffff
#define GDS_VMID7_SIZE__SIZE__SHIFT 0x0
#define GDS_VMID8_SIZE__SIZE_MASK 0x1ffff
#define GDS_VMID8_SIZE__SIZE__SHIFT 0x0
#define GDS_VMID9_SIZE__SIZE_MASK 0x1ffff
#define GDS_VMID9_SIZE__SIZE__SHIFT 0x0
#define GDS_VMID10_SIZE__SIZE_MASK 0x1ffff
#define GDS_VMID10_SIZE__SIZE__SHIFT 0x0
#define GDS_VMID11_SIZE__SIZE_MASK 0x1ffff
#define GDS_VMID11_SIZE__SIZE__SHIFT 0x0
#define GDS_VMID12_SIZE__SIZE_MASK 0x1ffff
#define GDS_VMID12_SIZE__SIZE__SHIFT 0x0
#define GDS_VMID13_SIZE__SIZE_MASK 0x1ffff
#define GDS_VMID13_SIZE__SIZE__SHIFT 0x0
#define GDS_VMID14_SIZE__SIZE_MASK 0x1ffff
#define GDS_VMID14_SIZE__SIZE__SHIFT 0x0
#define GDS_VMID15_SIZE__SIZE_MASK 0x1ffff
#define GDS_VMID15_SIZE__SIZE__SHIFT 0x0
#define GDS_GWS_VMID0__BASE_MASK 0x3f
#define GDS_GWS_VMID0__BASE__SHIFT 0x0
#define GDS_GWS_VMID0__SIZE_MASK 0x7f0000
#define GDS_GWS_VMID0__SIZE__SHIFT 0x10
#define GDS_GWS_VMID1__BASE_MASK 0x3f
#define GDS_GWS_VMID1__BASE__SHIFT 0x0
#define GDS_GWS_VMID1__SIZE_MASK 0x7f0000
#define GDS_GWS_VMID1__SIZE__SHIFT 0x10
#define GDS_GWS_VMID2__BASE_MASK 0x3f
#define GDS_GWS_VMID2__BASE__SHIFT 0x0
#define GDS_GWS_VMID2__SIZE_MASK 0x7f0000
#define GDS_GWS_VMID2__SIZE__SHIFT 0x10
#define GDS_GWS_VMID3__BASE_MASK 0x3f
#define GDS_GWS_VMID3__BASE__SHIFT 0x0
#define GDS_GWS_VMID3__SIZE_MASK 0x7f0000
#define GDS_GWS_VMID3__SIZE__SHIFT 0x10
#define GDS_GWS_VMID4__BASE_MASK 0x3f
#define GDS_GWS_VMID4__BASE__SHIFT 0x0
#define GDS_GWS_VMID4__SIZE_MASK 0x7f0000
#define GDS_GWS_VMID4__SIZE__SHIFT 0x10
#define GDS_GWS_VMID5__BASE_MASK 0x3f
#define GDS_GWS_VMID5__BASE__SHIFT 0x0
#define GDS_GWS_VMID5__SIZE_MASK 0x7f0000
#define GDS_GWS_VMID5__SIZE__SHIFT 0x10
#define GDS_GWS_VMID6__BASE_MASK 0x3f
#define GDS_GWS_VMID6__