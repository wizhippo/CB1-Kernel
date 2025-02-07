/*
 * linux/drivers/net/ethernet/allwinner/sunxi_gmac.h
 *
 * Copyright © 2016-2018, fuzhaoke
 *		Author: fuzhaoke <fuzhaoke@allwinnertech.com>
 *
 * This file is provided under a dual BSD/GPL license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 */
#ifndef __SUNXI_GETH_H__
#define __SUNXI_GETH_H__

#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/phy.h>
#include <linux/module.h>
#include <linux/init.h>

/* GETH_FRAME_FILTER  register value */
#define GETH_FRAME_FILTER_PR	0x00000001	/* Promiscuous Mode */
#define GETH_FRAME_FILTER_HUC	0x00000002	/* Hash Unicast */
#define GETH_FRAME_FILTER_HMC	0x00000004	/* Hash Multicast */
#define GETH_FRAME_FILTER_DAIF	0x00000008	/* DA Inverse Filtering */
#define GETH_FRAME_FILTER_PM	0x00000010	/* Pass all multicast */
#define GETH_FRAME_FILTER_DBF	0x00000020	/* Disable Broadcast frames */
#define GETH_FRAME_FILTER_SAIF	0x00000100	/* Inverse Filtering */
#define GETH_FRAME_FILTER_SAF	0x00000200	/* Source Address Filter */
#define GETH_FRAME_FILTER_HPF	0x00000400	/* Hash or perfect Filter */
#define GETH_FRAME_FILTER_RA	0x80000000	/* Receive all mode */

/* Default tx descriptor */
#define TX_SINGLE_DESC0		0x80000000
#define TX_SINGLE_DESC1		0x63000000

/* Default rx descriptor */
#define RX_SINGLE_DESC0		0x80000000
#define RX_SINGLE_DESC1		0x83000000

#define EPHY_ID			0x00441400
#define AC300_ID		0xc0000000
#define EPHY_ID_RTL8211F	0x001CC916
#define IP101G_ID		0x02430c54

typedef union {
	struct {
		/* TDES0 */
		unsigned int deferred:1;	/* Deferred bit (only half-duplex) */
		unsigned int under_err:1;	/* Underflow error */
		unsigned int ex_deferral:1;	/* Excessive deferral */
		unsigned int coll_cnt:4;	/* Collision count */
		unsigned int vlan_tag:1;	/* VLAN Frame */
		unsigned int ex_coll:1;		/* Excessive collision */
		unsigned int late_coll:1;	/* Late collision */
		unsigned int no_carr:1;		/* No carrier */
		unsigned int loss_carr:1;	/* Loss of collision */
		unsigned int ipdat_err:1;	/* IP payload error */
		unsigned int frm_flu:1;		/* Frame flushed */
		unsigned int jab_timeout:1;	/* Jabber timeout */
		unsigned int err_sum:1;		/* Error summary */
		unsigned int iphead_err:1;	/* IP header error */
		unsigned int ttss:1;		/* Transmit time stamp status */
		unsigned int reserved0:13;
		unsigned int own:1;		/* Own bit. CPU:0, DMA:1 */
	} tx;

	/* bits 5 7 0 | Frame status
	 * ----------------------------------------------------------
	 *      0 0 0 | IEEE 802.3 Type frame (length < 1536 octects)
	 *      1 0 0 | IPv4/6 No CSUM errorS.
	 *      1 0 1 | IPv4/6 CSUM PAYLOAD error
	 *      1 1 0 | IPv4/6 CSUM IP HR error
	 *      1 1 1 | IPv4/6 IP PAYLOAD AND HEADER errorS
	 *      0 0 1 | IPv4/6 unsupported IP PAYLOAD
	 *      0 1 1 | COE bypassed.. no IPv4/6 frame
	 *      0 1 0 | Reserved.
	 */
	struct {
		/* RDES0 */
		unsigned int chsum_err:1;	/* Payload checksum error */
		unsigned int crc_err:1;		/* CRC error */
		unsigned int dribbling:1;	/* Dribble bit error */
		unsigned int mii_err:1;		/* Received error (bit3) */
		unsigned int recv_wt:1;		/* Received watchdog timeout */
		unsigned int frm_type:1;	/* Frame type */
		unsigned int late_coll:1;	/* Late Collision */
		unsigned int ipch_err:1;	/* IPv header checksum error (bit7) */
		unsigned int last_desc:1;	/* Laset descriptor */
		unsigned int first_desc:1;	/* First descriptor */
		unsigned int vlan_tag:1;	/* VLAN Tag */
		unsigned int over_err:1;	/* Overflow error (bit11) */
		unsigned int len_err:1;		/* Length error */
		unsigned int sou_filter:1;	/* Source address filter fail */
		unsigned int desc_err:1;	/* Descriptor error */
		unsigned int err_sum:1;		/* Error summary (bit15) */
		unsigned int frm_len:14;	/* Frame length */
		unsigned int des_filter:1;	/* Destination address filter fail */
		unsigned int own:1;		/* Own bit. CPU:0, DMA:1 */
	#define RX_PKT_OK		0x7FFFB77C
	#define RX_LEN			0x3FFF0000
	} rx;

	unsigned int all;
} desc0_u;

typedef union {
	struct {
		/* TDES1 */
		unsigned int buf1_size:11;	/* Transmit buffer1 size */
		unsigned int buf2_size:11;	/* Transmit buffer2 size */
		unsigned int ttse:1;		/* Transmit time stamp enable */
		unsigned int dis_pad:1;		/* Disable pad (bit23) */
		unsigned int adr_chain:1;	/* Second address chained */
		unsigned int end_ring:1;	/* Transmit end of ring */
		unsigned int crc_dis:1;		/* Disable CRC */
		unsigned int cic:2;		/* Checksum insertion control (bit27:28) */
		unsigned int first_sg:1;	/* First Segment */
		unsigned int last_seg:1;	/* Last Segment */
		unsigned int interrupt:1;	/* Interrupt on completion */
	} tx;

	struct {
		/* RDES1 */
		unsigned int buf1_size:11;	/* Received buffer1 size */
		unsigned int buf2_size:11;	/* Received buffer2 size */
		unsigned int reserved1:2;
		unsigned int adr_chain:1;	/* Second address chained */
		unsigned int end_ring:1;		/* Received end of ring */
		unsigned int reserved2:5;
		unsigned int dis_ic:1;		/* Disable interrupt on completion */
	} rx;

	unsigned int all;
} desc1_u;

typedef struct dma_desc {
	desc0_u desc0;
	desc1_u desc1;
	/* The address of buffers */
	unsigned int	desc2;
	/* Next desc's address */
	unsigned int	desc3;
} __attribute__((packed)) dma_desc_t;

enum rx_frame_status { /* IPC status */
	good_frame = 0,
	discard_frame = 1,
	csum_none = 2,
	llc_snap = 4,
};

enum tx_dma_irq_status {
	tx_hard_error = 1,
	tx_hard_error_bump_tc = 2,
	handle_tx_rx = 3,
};

struct geth_extra_stats {
	/* Transmit errors */
	unsigned long tx_underflow;
	unsigned long tx_carrier;
	unsigned long tx_losscarrier;
	unsigned long vlan_tag;
	unsigned long tx_deferred;
	unsigned long tx_vlan;
	unsigned long tx_jabber;
	unsigned long tx_frame_flushed;
	unsigned long tx_payload_error;
	unsigned long tx_ip_header_error;

	/* Receive errors */
	unsigned long rx_desc;
	unsigned long sa_filter_fail;
	unsigned long overflow_error;
	unsigned long ipc_csum_error;
	unsigned long rx_collision;
	unsigned long rx_crc;
	unsigned long dribbling_bit;
	unsigned long rx_length;
	unsigned long rx_mii;
	unsigned long rx_multicast;
	unsigned long rx_gmac_overflow;
	unsigned long rx_watchdog;
	unsigned long da_rx_filter_fail;
	unsigned long sa_rx_filter_fail;
	unsigned long rx_missed_cntr;
	unsigned long rx_overflow_cntr;
	unsigned long rx_vlan;

	/* Tx/Rx IRQ errors */
	unsigned long tx_undeflow_irq;
	unsigned long tx_process_stopped_irq;
	unsigned long tx_jabber_irq;
	unsigned long rx_overflow_irq;
	unsigned long rx_buf_unav_irq;
	unsigned long rx_process_stopped_irq;
	unsigned long rx_watchdog_irq;
	unsigned long tx_early_irq;
	unsigned long fatal_bus_error_irq;

	/* Extra info */
	unsigned long threshold;
	unsigned long tx_pkt_n;
	unsigned long rx_pkt_n;
	unsigned long poll_n;
	unsigned long sched_timer_n;
	unsigned long normal_irq_n;
};

struct mii_reg_dump {
	u32 addr;
	u16 reg;
	u16 value;
};

int sunxi_mdio_read(void *,  int, int);
int sunxi_mdio_write(void *, int, int, unsigned short);
int sunxi_mdio_reset(void *);
void sunxi_set_link_mode(void *iobase, int duplex, int speed);
void sunxi_int_disable(void *);
int sunxi_int_status(void *, struct geth_extra_stats *x);
int sunxi_mac_init(void *, int txmode, int rxmode);
void sunxi_set_umac(void *, unsigned char *, int);
void sunxi_mac_enable(void *);
void sunxi_mac_disable(void *);
void sunxi_tx_poll(void *);
void sunxi_int_enable(void *);
void sunxi_start_rx(void *, unsigned long);
void sunxi_start_tx(void *, unsigned long);
void sunxi_stop_tx(void *);
void sunxi_stop_rx(void *);
void sunxi_hash_filter(void *iobase, unsigned long low, unsigned long high);
void sunxi_set_filter(void *iobase, unsigned long flags);
void sunxi_flow_ctrl(void *iobase, int duplex, int fc, int pause);
void sunxi_mac_loopback(void *iobase, int enable);

void desc_buf_set(struct dma_desc *p, unsigned long paddr, int size);
void desc_set_own(struct dma_desc *p);
void desc_init_chain(struct dma_desc *p, unsigned long paddr,  unsigned int size);
void desc_tx_close(struct dma_desc *first, struct dma_desc *end, int csum_insert);
void desc_init(struct dma_desc *p);
int desc_get_tx_status(struct dma_desc *desc, struct geth_extra_stats *x);
int desc_buf_get_len(struct dma_desc *desc);
int desc_buf_get_addr(struct dma_desc *desc);
int desc_get_rx_status(struct dma_desc *desc, struct geth_extra_stats *x);
int desc_get_own(struct dma_desc *desc);
int desc_get_tx_ls(struct dma_desc *desc);
int desc_rx_frame_len(struct dma_desc *desc);

int sunxi_mac_reset(void *iobase, void (*mdelay)(int), int n);
int sunxi_geth_register(void *iobase, int version, unsigned int div);

int sunxi_parse_read_str(char *str, u16 *addr, u16 *reg);
int sunxi_parse_write_str(char *str, u16 *addr, u16 *reg, u16 *val);

#if defined(CONFIG_SUNXI_EPHY)
extern int ephy_is_enable(void);
extern int gmac_ephy_shutdown(void);
#endif

#if defined(CONFIG_ARCH_SUN8IW3) \
	|| defined(CONFIG_ARCH_SUN9IW1) \
	|| defined(CONFIG_ARCH_SUN7I)
#define HW_VERSION	0
#else
#define HW_VERSION	1
#endif

#endif
