/* SPDX-License-Identifier: GPL-2.0 */
/**
 * rawnand_chip.c
 *
 * Copyright (C) 2019 Allwinner.
 *
 * 2019.9.11 cuizhikui<cuizhikui@allwinnertech.com>
 */

#ifndef __RAWNAND_CHIP_H__
#define __RAWNAND_CHIP_H__

#include "rawnand.h"
#include "controller/ndfc_base.h"

extern struct _nand_storage_info *g_nsi;
extern struct _nand_storage_info g_nsi_data;
extern rawnand_storage_info_t *g_nand_storage_info;
extern rawnand_storage_info_t g_nand_storage_info_data;
extern struct nand_chip_info nci_data[MAX_CHIP_PER_CHANNEL * 2];

extern struct _nand_super_storage_info *g_nssi;
extern struct _nand_super_storage_info g_nssi_data;
extern struct nand_super_chip_info nsci_data[MAX_CHIP_PER_CHANNEL * 2];

void nand_enable_chip(struct nand_chip_info *nci);
void nand_disable_chip(struct nand_chip_info *nci);
int nand_read_chip_status_ready(struct nand_chip_info *nci);
int nand_wait_all_rb_ready(void);
int nand_set_feature(struct nand_chip_info *nci, u8 *addr, u8 *feature);
int nand_get_feature(struct nand_chip_info *nci, u8 *addr, u8 *feature);
int nand_reset_chip(struct nand_chip_info *nci);

struct nand_chip_info *nci_get_from_nsi(struct _nand_storage_info *nsi,
					unsigned int num);
void nci_delete_from_nctri(struct nand_controller_info *nctri);
struct nand_super_chip_info *nsci_get_from_nssi(
    struct _nand_super_storage_info *nssi, unsigned int num);
int switch_ddrtype_from_ddr_to_sdr(struct nand_controller_info *nctri);
void set_default_batch_read_cmd_seq(struct _nctri_cmd_seq *cmd_seq);
void set_default_batch_write_cmd_seq(struct _nctri_cmd_seq *cmd_seq,
				     u32 write_cmd1, u32 write_cmd2);
unsigned int get_row_addr(unsigned int page_offset_for_next_blk,
			  unsigned int block, unsigned int page);
unsigned int get_row_addr_2(unsigned int page_offset_for_next_blk,
			  unsigned int block, unsigned int page);
int fill_cmd_addr(u32 col_addr, u32 col_cycle, u32 row_addr, u32 row_cycle,
		  u8 *abuf);
int get_data_block_cnt_for_boot0_ecccode(struct nand_chip_info *nci,
					 u8 ecc_mode);
int get_random_cmd2(struct _nand_physic_op_par *npo);
int get_dummy_byte(int physic_page_size, int ecc_mode, int ecc_block_cnt,
		   int user_data_size);
struct nand_chip_info *nci_get_from_nctri(struct nand_controller_info *nctri,
					  unsigned int num);
s32 _change_all_nand_parameter(struct nand_controller_info *nctri,
			       u32 ddr_type, u32 pre_ddr_type, u32 dclk);
s32 _get_right_timing_para(struct nand_controller_info *nctri, u32 ddr_type,
			   u32 *good_sdr_edo, u32 *good_ddr_edo, u32 *good_ddr_delay);
s32 _setup_ddr_nand_force_to_sdr_para(struct nand_chip_info *nci);
s32 _check_scan_data(u32 first_check, u32 chip, u32 *scan_good_blk_no, u8 *main_buf);
void delete_nsi(void);
int delete_nssi(void);

extern int rawnand_read_parameter_page(struct nand_chip_info *nci, unsigned char *p);

#endif /*RAWNADN_CHIP_H*/
