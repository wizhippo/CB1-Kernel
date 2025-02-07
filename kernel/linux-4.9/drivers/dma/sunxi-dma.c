/*
 * drivers/dma/sunxi-dma.c
 *
 * Copyright (C) 2015-2020 Allwinnertech Co., Ltd
 *
 * Author: Sugar <shuge@allwinnertech.com>
 * Author: Wim Hwang <huangwei@allwinnertech.com>
 *
 * Sunxi DMA controller driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */


#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/dmapool.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/dma/sunxi-dma.h>
#include <linux/sunxi-smc.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/kernel.h>

#include "dmaengine.h"
#include "virt-dma.h"

#if defined(CONFIG_ARCH_SUN8IW1) \
	|| defined(CONFIG_ARCH_SUN8IW11) \
	|| defined(CONFIG_ARCH_SUN50IW6)\
	|| defined(CONFIG_ARCH_SUN8IW12) \
	|| defined(CONFIG_ARCH_SUN50IW9)
#define NR_MAX_CHAN	16			/* total of channels */
#elif defined(CONFIG_ARCH_SUN8IW7) \
	|| defined(CONFIG_ARCH_SUN50IW2) \
	|| defined(CONFIG_ARCH_SUN50IW3) \
	|| defined(CONFIG_ARCH_SUN8IW17)
#define NR_MAX_CHAN	12			/* total of channels */
#elif defined(CONFIG_ARCH_SUN50IW8)
#define NR_MAX_CHAN	10			/* total of channels */
#else
#define NR_MAX_CHAN	8			/* total of channels */
#endif

#define HIGH_CHAN	8

#define DMA_IRQ_EN(x)	(0x000 + ((x) << 2))	/* Interrupt enable register */
#define DMA_IRQ_STAT(x)	(0x010 + ((x) << 2))	/* Inetrrupt status register */

#if defined(CONFIG_ARCH_SUN9I) \
	|| defined(CONFIG_ARCH_SUN8IW7) \
	|| defined(CONFIG_ARCH_SUN50I)\
	|| defined(CONFIG_ARCH_SUN8IW12) \
	|| defined(CONFIG_ARCH_SUN8IW15) \
	|| defined(CONFIG_ARCH_SUN8IW17) \
	|| defined(CONFIG_ARCH_SUN50IW9) \
	|| defined(CONFIG_ARCH_SUN50IW10)
#define DMA_SECU	0x20			/* DMA security register */
#endif

#if defined(CONFIG_ARCH_SUN8IW1)
#undef DMA_GATE					/* no gating register */
#elif defined(CONFIG_ARCH_SUN8IW3) \
	|| defined(CONFIG_ARCH_SUN8IW5) \
	|| defined(CONFIG_ARCH_SUN8IW6) \
	|| defined(CONFIG_ARCH_SUN8IW8) \
	|| defined(CONFIG_ARCH_SUN8IW9)
#define DMA_GATE	0x20			/* DMA gating rgister */
#else
#define DMA_GATE	0x28			/* DMA gating rgister */
#endif
#define DMA_MCLK_GATE	0x04
#define DMA_COMMON_GATE 0x02
#define DMA_CHAN_GATE	0x01

#define DMA_STAT	0x30			/* DMA Status Register RO */
#define DMA_ENABLE(x)	(0x100 + ((x) << 6))	/* Channels enable register */
#define DMA_PAUSE(x)	(0x104 + ((x) << 6))	/* DMA Channels pause register */
#define DMA_LLI_ADDR(x)	(0x108 + ((x) << 6))	/* Descriptor address register */
#define DMA_CFG(x)	(0x10C + ((x) << 6))	/* Configuration register RO */
#define DMA_CUR_SRC(x)	(0x110 + ((x) << 6))	/* Current source address RO */
#define DMA_CUR_DST(x)	(0x114 + ((x) << 6))	/* Current destination address RO */
#define DMA_CNT(x)	(0x118 + ((x) << 6))	/* Byte counter left register RO */
#define DMA_PARA(x)	(0x11C + ((x) << 6))	/* Parameter register RO */

#if defined(CONFIG_ARCH_SUN9I)
#define LINK_END	0x1FFFF800		/* lastest link must be 0x1ffff800 */
#else
#define LINK_END	0xFFFFF800		/* lastest link must be 0xfffff800 */
#endif

/* DMA opertions mode */
#if defined(CONFIG_ARCH_SUN8IW1) \
	|| defined(CONFIG_ARCH_SUN8IW3) \
	|| defined(CONFIG_ARCH_SUN8IW5) \
	|| defined(CONFIG_ARCH_SUN8IW6) \
	|| defined(CONFIG_ARCH_SUN8IW8) \
	|| defined(CONFIG_ARCH_SUN8IW9)

#define DMA_OP_MODE(x)
#define SRC_HS_MASK
#define DST_HS_MASK
#define SET_OP_MODE(d, x, val)  do { } while (0)

#else

#define DMA_OP_MODE(x)	(0x128 + ((x) << 6))	/* DMA mode options register */
#define SRC_HS_MASK	(0x1 << 2)		/* bit 2: Source handshark mode */
#define DST_HS_MASK	(0x1 << 3)		/* bit 3: Destination handshark mode */

#define SET_OP_MODE(d, x, val)	({	\
		writel(val, d->base + DMA_OP_MODE(x));	\
		})

#endif

#define SHIFT_IRQ_MASK(val, ch) ({	\
		(ch) >= HIGH_CHAN	\
		? (val) << ((ch - HIGH_CHAN) << 2) \
		: (val) << ((ch) << 2);	\
		})

#define IRQ_HALF	0x01		/* Half package transfer interrupt pending */
#define IRQ_PKG		0x02		/* One package complete interrupt pending */
#define IRQ_QUEUE	0x04		/* All list complete transfer interrupt pending */

/* The detail information of DMA configuration */
#define SRC_WIDTH(x)	((x) << 9)
#if defined(CONFIG_ARCH_SUN8IW1) \
	|| defined(CONFIG_ARCH_SUN8IW3) \
	|| defined(CONFIG_ARCH_SUN8IW5) \
	|| defined(CONFIG_ARCH_SUN8IW6) \
	|| defined(CONFIG_ARCH_SUN8IW8) \
	|| defined(CONFIG_ARCH_SUN8IW9)
#define SRC_BURST(x)	((x) << 7)
#else
#define SRC_BURST(x)	((x) << 6)
#endif

#if defined(CONFIG_ARCH_SUN50IW3) \
	|| defined(CONFIG_ARCH_SUN50IW6)\
	|| defined(CONFIG_ARCH_SUN8IW12)\
	|| defined(CONFIG_ARCH_SUN8IW15)\
	|| defined(CONFIG_ARCH_SUN50IW8)\
	|| defined(CONFIG_ARCH_SUN8IW17)\
	|| defined(CONFIG_ARCH_SUN50IW5T)\
	|| defined(CONFIG_ARCH_SUN50IW9) \
	|| defined(CONFIG_ARCH_SUN50IW10)
#define SRC_IO_MODE	(0x01 << 8)
#define SRC_LINEAR_MODE	(0x00 << 8)
#else
#define SRC_IO_MODE	(0x01 << 5)
#define SRC_LINEAR_MODE	(0x00 << 5)
#endif
#define SRC_DRQ(x)	((x) << 0)

#define DST_WIDTH(x)	((x) << 25)
#if defined(CONFIG_ARCH_SUN8IW1) \
	|| defined(CONFIG_ARCH_SUN8IW3) \
	|| defined(CONFIG_ARCH_SUN8IW5) \
	|| defined(CONFIG_ARCH_SUN8IW6) \
	|| defined(CONFIG_ARCH_SUN8IW8) \
	|| defined(CONFIG_ARCH_SUN8IW9)
#define DST_BURST(x)	((x) << 23)
#else
#define DST_BURST(x)	((x) << 22)
#endif

#if defined(CONFIG_ARCH_SUN50IW3) \
	|| defined(CONFIG_ARCH_SUN50IW6)\
	|| defined(CONFIG_ARCH_SUN8IW12) \
	|| defined(CONFIG_ARCH_SUN8IW15)\
	|| defined(CONFIG_ARCH_SUN50IW8)\
	|| defined(CONFIG_ARCH_SUN8IW17)\
	|| defined(CONFIG_ARCH_SUN50IW5T)\
	|| defined(CONFIG_ARCH_SUN50IW9) \
	|| defined(CONFIG_ARCH_SUN50IW10)
#define DST_IO_MODE	(0x01 << 24)
#define DST_LINEAR_MODE	(0x00 << 24)
#else
#define DST_IO_MODE	(0x01 << 21)
#define DST_LINEAR_MODE	(0x00 << 21)
#endif
#define DST_DRQ(x)	((x) << 16)

#define CHAN_START	1
#define CHAN_STOP	0
#define CHAN_PAUSE	1
#define CHAN_RESUME	0

#define NORMAL_WAIT	(8 << 0)

#define BMODE_SEL	(0x01 << 30)
#define SET_DST_HIGH_ADDR(x) (((x >> 32) & 0x3UL) << 18)
#define SET_SRC_HIGH_ADDR(x) (((x >> 32) & 0x3UL) << 16)
#define SET_DESC_HIGH_ADDR(x) (((x >> 32) & 0x3UL) | (x & 0xFFFFFFFC))
#define SYSCFG_VER	0x03000024
#define SYS_VER_MASK	(0x07 << 0)
/*
 * struct sunxi_dma_lli - linked list ltem, the DMA block descriptor
 * @cfg:	DMA configuration
 * @src:	Source address
 * @dst:	Destination address
 * @len:	Length of buffers
 * @para:	Parameter register
 * @p_lln:	Next lli physical address
 * @v_lln:	Next lli virtual address (only for cpu)
 * @this_phy:	Physical address of this lli
 */
struct sunxi_dma_lli {
	u32	cfg;
	u32	src;
	u32	dst;
	u32	len;
	u32	para;
	u32	p_lln;
	struct sunxi_dma_lli *v_lln;
#ifdef DEBUG
	dma_addr_t	this_phy;
	#define set_this_phy(li, addr)	\
		((li)->this_phy = (addr))
#else
	#define set_this_phy(li, addr)
#endif
} __packed;

struct sunxi_dmadev {
	struct dma_device	dma_dev;
	void __iomem		*base;
	phys_addr_t             pbase;
	struct clk		*ahb_clk;	/* AHB clock gate for DMA */

	spinlock_t		lock;
	struct tasklet_struct	task;
	struct list_head	pending;	/* the pending channels list */
	struct dma_pool		*lli_pool;	/* Pool of lli */
};

struct sunxi_desc {
	struct virt_dma_desc	vd;
	u32			lli_phys;	/* physical start for llis */
	struct sunxi_dma_lli	*lli_virt;	/* virtual start for lli */
};

struct sunxi_chan {
	struct virt_dma_chan	vc;

	struct list_head	node;		/* queue it to pending list */
	struct dma_slave_config	cfg;
	bool			cyclic;

	struct sunxi_desc	*desc;
	u32			irq_type;
};

static u64 sunxi_dma_mask = DMA_BIT_MASK(32);
static u32 sunxi_dma_channel_bitmap;
static u32 syscfg_version;

static inline struct sunxi_dmadev *to_sunxi_dmadev(struct dma_device *d)
{
	return container_of(d, struct sunxi_dmadev, dma_dev);
}

static inline struct sunxi_chan *to_sunxi_chan(struct dma_chan *chan)
{
	return container_of(chan, struct sunxi_chan, vc.chan);
}

static inline struct sunxi_desc *to_sunxi_desc(struct dma_async_tx_descriptor *tx)
{
	return container_of(tx, struct sunxi_desc, vd.tx);
}

static struct device *chan2dev(struct dma_chan *chan)
{
	return &chan->dev->device;
}
static struct device *chan2parent(struct dma_chan *chan)
{
	return chan->dev->device.parent;
}

/*
 * Fix sconfig's burst size according to sunxi_dmac. We need to convert them as:
 * 1 -> 0, 4 -> 1, 8 -> 2, 16->3
 *
 * NOTE: burst size 2 is not supported by controller.
 *
 * This can be done by finding least significant bit set: n & (n - 1)
 */
static inline void convert_burst(u32 *maxburst)
{
	if (*maxburst > 1)
		*maxburst = fls(*maxburst) - 2;
	else
		*maxburst = 0;
}

/*
 * Fix sconfig's bus width according to at_dmac.
 * 1 byte -> 0, 2 bytes -> 1, 4 bytes -> 2.
 */
static inline u8 convert_buswidth(enum dma_slave_buswidth addr_width)
{
	switch (addr_width) {
	case DMA_SLAVE_BUSWIDTH_2_BYTES:
		return 1;
	case DMA_SLAVE_BUSWIDTH_4_BYTES:
		return 2;
	default:
		/* For 1 byte width or fallback */
		return 0;
	}
}

static size_t sunxi_get_desc_size(struct sunxi_desc *txd)
{
	struct sunxi_dma_lli *lli;
	size_t size = 0;

	for (lli = txd->lli_virt; lli != NULL; lli = lli->v_lln)
		size += lli->len;

	return size;
}

/*
 * sunxi_get_chan_size - get the bytes left of one channel.
 * @ch: the channel
 */
static size_t sunxi_get_chan_size(struct sunxi_chan *ch)
{
	struct sunxi_dma_lli *lli;
	struct sunxi_desc *txd;
	struct sunxi_dmadev *sdev;
	size_t size = 0;
	u32 pos;
	bool count = false;

	txd = ch->desc;

	if (!txd)
		return 0;

	sdev = to_sunxi_dmadev(ch->vc.chan.device);
	pos = readl(sdev->base + DMA_LLI_ADDR(ch->vc.chan.chan_id));
	size = readl(sdev->base + DMA_CNT(ch->vc.chan.chan_id));

	/* It is the last package, and just read count register */
	if (pos == LINK_END)
		return size;

	for (lli = txd->lli_virt; lli != NULL; lli = lli->v_lln) {
		/* Ok, found next lli that is ready be transported */
		if (lli->p_lln == pos) {
			count = true;
			continue;
		}

		if (count)
			size += lli->len;
	}

	return size;
}

/*
 * sunxi_free_desc - free the struct sunxi_desc.
 * @vd: the virt-desc for this chan
 */
static void sunxi_free_desc(struct virt_dma_desc *vd)
{
	struct sunxi_desc *txd = to_sunxi_desc(&vd->tx);
	struct sunxi_dmadev *sdev = to_sunxi_dmadev(vd->tx.chan->device);
	struct sunxi_dma_lli *li_adr, *next_virt;
	u32 phy, next_phy;

	if (unlikely(!txd))
		return;

	phy = txd->lli_phys;
	li_adr = txd->lli_virt;

	while (li_adr) {
		next_virt = li_adr->v_lln;
		next_phy = li_adr->p_lln;
		dma_pool_free(sdev->lli_pool, li_adr, phy);
		li_adr = next_virt;
		phy = next_phy;
	}

	txd->vd.tx.callback = NULL;
	txd->vd.tx.callback_param = NULL;
	kfree(txd);
	txd = NULL;
}

static inline void sunxi_dump_com_regs(struct sunxi_chan *ch)
{
	struct sunxi_dmadev *sdev;

	sdev = to_sunxi_dmadev(ch->vc.chan.device);

	pr_debug("Common register:\n"
			"\tmask0(%04x): 0x%08x\n"
			"\tmask1(%04x): 0x%08x\n"
			"\tpend0(%04x): 0x%08x\n"
			"\tpend1(%04x): 0x%08x\n"
#ifdef DMA_SECU
			"\tsecur(%04x): 0x%08x\n"
#endif
#ifdef DMA_GATE
			"\t_gate(%04x): 0x%08x\n"
#endif
			"\tstats(%04x): 0x%08x\n",
			DMA_IRQ_EN(0),  readl(sdev->base + DMA_IRQ_EN(0)),
			DMA_IRQ_EN(1),  readl(sdev->base + DMA_IRQ_EN(1)),
			DMA_IRQ_STAT(0), readl(sdev->base + DMA_IRQ_STAT(0)),
			DMA_IRQ_STAT(1), readl(sdev->base + DMA_IRQ_STAT(1)),
#ifdef DMA_SECU
			DMA_SECU, readl(sdev->base + DMA_SECU),
#endif
#ifdef DMA_GATE
			DMA_GATE, readl(sdev->base + DMA_GATE),
#endif
			DMA_STAT, readl(sdev->base + DMA_STAT));
}

static inline void sunxi_dump_chan_regs(struct sunxi_chan *ch)
{
	struct sunxi_dmadev *sdev = to_sunxi_dmadev(ch->vc.chan.device);
	u32 chan_num = ch->vc.chan.chan_id;

	pr_debug("Chan %d reg:\n"
			"\t___en(%04x): \t0x%08x\n"
			"\tpause(%04x): \t0x%08x\n"
			"\tstart(%04x): \t0x%08x\n"
			"\t__cfg(%04x): \t0x%08x\n"
			"\t__src(%04x): \t0x%08x\n"
			"\t__dst(%04x): \t0x%08x\n"
			"\tcount(%04x): \t0x%08x\n"
			"\t_para(%04x): \t0x%08x\n\n",
			chan_num,
			DMA_ENABLE(chan_num),
			readl(sdev->base + DMA_ENABLE(chan_num)),
			DMA_PAUSE(chan_num),
			readl(sdev->base + DMA_PAUSE(chan_num)),
			DMA_LLI_ADDR(chan_num),
			readl(sdev->base + DMA_LLI_ADDR(chan_num)),
			DMA_CFG(chan_num),
			readl(sdev->base + DMA_CFG(chan_num)),
			DMA_CUR_SRC(chan_num),
			readl(sdev->base + DMA_CUR_SRC(chan_num)),
			DMA_CUR_DST(chan_num),
			readl(sdev->base + DMA_CUR_DST(chan_num)),
			DMA_CNT(chan_num),
			readl(sdev->base + DMA_CNT(chan_num)),
			DMA_PARA(chan_num),
			readl(sdev->base + DMA_PARA(chan_num)));
}


/*
 * sunxi_dma_resume - resume channel, which is pause sate.
 * @chan: the channel to resume
 */
static int sunxi_dma_resume(struct dma_chan *chan)
{
	struct sunxi_chan *schan = to_sunxi_chan(chan);
	struct sunxi_dmadev *sdev = to_sunxi_dmadev(schan->vc.chan.device);
	u32 chan_num = schan->vc.chan.chan_id;

	writel(CHAN_RESUME, sdev->base + DMA_PAUSE(chan_num));
	return 0;
}

static int sunxi_dma_pause(struct dma_chan *chan)
{
	struct sunxi_chan *schan = to_sunxi_chan(chan);
	struct sunxi_dmadev *sdev = to_sunxi_dmadev(schan->vc.chan.device);
	u32 chan_num = schan->vc.chan.chan_id;

	writel(CHAN_PAUSE, sdev->base + DMA_PAUSE(chan_num));
	return 0;
}

/*
 * sunxi_terminate_all - stop all descriptors that waiting transfer on chan.
 * @ch: the channel to stop
 */
static int sunxi_terminate_all(struct dma_chan *chan)
{
	struct sunxi_chan *schan = to_sunxi_chan(chan);
	struct sunxi_dmadev *sdev = to_sunxi_dmadev(schan->vc.chan.device);
	struct virt_dma_desc *vd = NULL;
	struct virt_dma_chan *vc = NULL;
	u32 chan_num = schan->vc.chan.chan_id;
	unsigned long flags;
	LIST_HEAD(head);

	spin_lock_irqsave(&schan->vc.lock, flags);

	spin_lock(&sdev->lock);
	list_del_init(&schan->node);
	spin_unlock(&sdev->lock);

	/* We should entry PAUSE state first to avoid missing data
	 * count which transferring on bus.
	 */
	writel(CHAN_PAUSE, sdev->base + DMA_PAUSE(chan_num));
	writel(CHAN_STOP, sdev->base + DMA_ENABLE(chan_num));
	writel(CHAN_RESUME, sdev->base + DMA_PAUSE(chan_num));

	/* At cyclic mode, desc is not be managed by virt-dma,
	 * we need to add it to desc_completed
	 */
	if (schan->cyclic) {
		schan->cyclic = false;
		if (schan->desc) {
			vd = &(schan->desc->vd);
			vc = &(schan->vc);
			list_add_tail(&vd->node, &vc->desc_completed);
		}
	}
	schan->desc = NULL;

	vchan_get_all_descriptors(&schan->vc, &head);
	spin_unlock_irqrestore(&schan->vc.lock, flags);
	vchan_dma_desc_free_list(&schan->vc, &head);

	return 0;
}

/*
 * sunxi_start_desc - begin to transport the descriptor
 * @ch: the channel of descriptor
 */
static void sunxi_start_desc(struct sunxi_chan *ch)
{
	struct virt_dma_desc *vd = vchan_next_desc(&ch->vc);
	struct sunxi_dmadev *sdev = to_sunxi_dmadev(ch->vc.chan.device);
	struct sunxi_desc *txd = NULL;
	u32 chan_num = ch->vc.chan.chan_id;
	u32 irq_val;
	u32 high;

	if (!vd) {
		while (readl(sdev->base + DMA_STAT) & (1 << chan_num))
			cpu_relax();
		writel(CHAN_STOP, sdev->base + DMA_ENABLE(chan_num));
		return;
	}

	/* Delete this desc from the desc_issued list */
	list_del(&vd->node);

	txd = to_sunxi_desc(&vd->tx);
	ch->desc = txd;

	if (ch->cyclic)
		ch->irq_type = IRQ_PKG;
	else
		ch->irq_type = IRQ_QUEUE;

	high = (chan_num >= HIGH_CHAN) ? 1 : 0;

	irq_val = readl(sdev->base + DMA_IRQ_EN(high));
	irq_val |= SHIFT_IRQ_MASK(ch->irq_type, chan_num);
	writel(irq_val, sdev->base + DMA_IRQ_EN(high));

	/* Set the DMA opertions mode */
	SET_OP_MODE(sdev, chan_num, SRC_HS_MASK | DST_HS_MASK);

	/* write the first lli address to register, and start to transfer */
	writel(txd->lli_phys, sdev->base + DMA_LLI_ADDR(chan_num));
	writel(CHAN_START, sdev->base + DMA_ENABLE(chan_num));

	sunxi_dump_com_regs(ch);
	sunxi_dump_chan_regs(ch);
}

/*
 * sunxi_alloc_lli - Allocate a sunxi_lli
 * @sdev: the sunxi_dmadev
 * @phy_addr: return the physical address
 */
void *sunxi_alloc_lli(struct sunxi_dmadev *sdev, u32 *phy_addr)
{
	struct sunxi_dma_lli *l_item;
	dma_addr_t phy;

	WARN_TAINT(!sdev->lli_pool, TAINT_WARN, "The dma pool is empty!!\n");
	if (unlikely(!sdev->lli_pool))
		return NULL;

	l_item = dma_pool_alloc(sdev->lli_pool, GFP_ATOMIC, &phy);
#ifdef CONFIG_DMA_SUNXI_SUPPORT_4G
	*phy_addr = (u32)SET_DESC_HIGH_ADDR(phy);
#else
	*phy_addr = (u32)phy;
#endif
	set_this_phy(l_item, phy);

	return l_item;
}

/*
 * sunxi_dump_lli - dump the information for one lli
 * @shcan: the channel
 * @lli: a lli to dump
 */
static inline void sunxi_dump_lli(struct sunxi_chan *schan,
		struct sunxi_dma_lli *lli)
{
#ifdef	DEBUG
	dev_dbg(chan2dev(&schan->vc.chan),
			"\n\tdesc:   p - 0x%p v - 0x%p\n"
			"\t\tc - 0x%08x s - 0x%08x d - 0x%08x\n"
			"\t\tl - 0x%08x p - 0x%08x n - 0x%08x\n",
			(void *)lli->this_phy, lli,
			lli->cfg, lli->src, lli->dst,
			lli->len, lli->para, lli->p_lln);
#endif
}

static void *sunxi_lli_list(struct sunxi_dma_lli *prev,
		struct sunxi_dma_lli *next, u32 next_phy,
		struct sunxi_desc *txd)
{
	if ((!prev && !txd) || !next)
		return NULL;

	if (!prev) {
		txd->lli_phys = next_phy;
		txd->lli_virt = next;
	} else {
		prev->p_lln = next_phy;
		prev->v_lln = next;
	}

	next->p_lln = LINK_END;
	next->v_lln = NULL;

	return next;
}

static inline void sunxi_cfg_lli(struct sunxi_dma_lli *lli, dma_addr_t src,
		dma_addr_t dst, u32 len, struct dma_slave_config *config)
{
	u32 src_width, dst_width;

	if (!config)
		return;

	/* Get the data width */
	src_width = convert_buswidth(config->src_addr_width);
	dst_width = convert_buswidth(config->dst_addr_width);

	lli->cfg = SRC_BURST(config->src_maxburst)
			| SRC_WIDTH(src_width)
			| DST_BURST(config->dst_maxburst)
			| DST_WIDTH(dst_width);

	lli->src = (u32)src;
	lli->dst = (u32)dst;
	lli->len = len;
#ifdef CONFIG_DMA_SUNXI_SUPPORT_4G
	lli->para = SET_DST_HIGH_ADDR(dst)
			| SET_SRC_HIGH_ADDR(src)
			| NORMAL_WAIT;
#else
	lli->para = NORMAL_WAIT;
#endif
}


/*
 * sunxi_dma_tasklet - ensure that the desc's lli be putted into hardware.
 * @data: sunxi_dmadev
 */
static void sunxi_dma_tasklet(unsigned long data)
{
	struct sunxi_dmadev *sdev = (struct sunxi_dmadev *)data;
	LIST_HEAD(head);

	spin_lock_irq(&sdev->lock);
	list_splice_tail_init(&sdev->pending, &head);
	spin_unlock_irq(&sdev->lock);

	while (!list_empty(&head)) {
		struct sunxi_chan *c = list_first_entry(&head,
			struct sunxi_chan, node);

		spin_lock_irq(&c->vc.lock);
		list_del_init(&c->node);
		sunxi_start_desc(c);
		spin_unlock_irq(&c->vc.lock);
	}
}

/*
 * sunxi_dma_interrupt - interrupt handle.
 * @irq: irq number
 * @dev_id: sunxi_dmadev
 */
static irqreturn_t sunxi_dma_interrupt(int irq, void *dev_id)
{
	struct sunxi_dmadev *sdev = (struct sunxi_dmadev *)dev_id;
	struct sunxi_chan *ch = NULL;
	struct sunxi_desc *desc;
	unsigned long flags;
	u32 status_lo = 0, status_hi = 0;

	/* Get the status of irq */
	status_lo = readl(sdev->base + DMA_IRQ_STAT(0));
#if NR_MAX_CHAN > HIGH_CHAN
	status_hi = readl(sdev->base + DMA_IRQ_STAT(1));
#endif

	dev_dbg(sdev->dma_dev.dev,
		"[sunxi_dma]: DMA irq status_lo: 0x%08x, status_hi: 0x%08x\n",
		status_lo, status_hi);

	/* Clear the bit of irq status */
	writel(status_lo, sdev->base + DMA_IRQ_STAT(0));
#if NR_MAX_CHAN > HIGH_CHAN
	writel(status_hi, sdev->base + DMA_IRQ_STAT(1));
#endif

	list_for_each_entry(ch, &sdev->dma_dev.channels, vc.chan.device_node) {
		u32 chan_num = ch->vc.chan.chan_id;
		u32 status;

		status = (chan_num >= HIGH_CHAN)
			? (status_hi >> ((chan_num - HIGH_CHAN) << 2))
			: (status_lo >> (chan_num << 2));

		spin_lock_irqsave(&ch->vc.lock, flags);
		if (!(ch->irq_type & status))
			goto unlock;

		if (!ch->desc)
			goto unlock;

		desc = ch->desc;
		if (ch->cyclic) {
			struct virt_dma_desc *vd;
			dma_async_tx_callback cb = NULL;
			void *cb_data = NULL;

			vd = &desc->vd;
			if (vd) {
				cb = vd->tx.callback;
				cb_data = vd->tx.callback_param;
			}
			spin_unlock_irqrestore(&ch->vc.lock, flags);
			if (cb)
				cb(cb_data);
			spin_lock_irqsave(&ch->vc.lock, flags);
		} else {
			ch->desc = NULL;
			vchan_cookie_complete(&desc->vd);
			sunxi_start_desc(ch);
		}
unlock:
		spin_unlock_irqrestore(&ch->vc.lock, flags);
	}

	return IRQ_HANDLED;
}

static struct dma_async_tx_descriptor *sunxi_prep_dma_memcpy(
		struct dma_chan *chan, dma_addr_t dest, dma_addr_t src,
		size_t len, unsigned long flags)
{
	struct sunxi_chan *schan = to_sunxi_chan(chan);
	struct sunxi_desc *txd;
	struct sunxi_dma_lli *l_item;
	struct sunxi_dmadev *sdev = to_sunxi_dmadev(chan->device);
	struct dma_slave_config *sconfig = &schan->cfg;
	u32	phy;

	dev_dbg(chan2dev(chan),
		"chan: %d, dest: 0x%08lx, src: 0x%08lx, len: 0x%08zx, flags: 0x%08lx\n",
		schan->vc.chan.chan_id, (unsigned long)dest,
		(unsigned long)src, len, flags);

	if (unlikely(!len)) {
		dev_dbg(chan2dev(chan), "memcpy length is zero!\n");
		return NULL;
	}

	txd = kzalloc(sizeof(*txd), GFP_NOWAIT);
	if (!txd)
		return NULL;
	vchan_tx_prep(&schan->vc, &txd->vd, flags);

	l_item = sunxi_alloc_lli(sdev, &phy);
	if (!l_item) {
		sunxi_free_desc(&txd->vd);
		dev_err(sdev->dma_dev.dev, "Failed to alloc lli memory!\n");
		return NULL;
	}

	sunxi_cfg_lli(l_item, src, dest, len, sconfig);
	l_item->cfg |= SRC_DRQ(DRQSRC_SDRAM)
			| DST_DRQ(DRQDST_SDRAM)
			| DST_LINEAR_MODE
			| SRC_LINEAR_MODE;

	sunxi_lli_list(NULL, l_item, phy, txd);

	sunxi_dump_lli(schan, l_item);

	return &txd->vd.tx;
}

static struct dma_async_tx_descriptor *sunxi_prep_dma_sg(
		struct dma_chan *chan,
		struct scatterlist *dst_sg, unsigned int dst_nents,
		struct scatterlist *src_sg, unsigned int src_nents,
		unsigned long flags)
{
	struct sunxi_chan *schan = to_sunxi_chan(chan);
	struct sunxi_dmadev *sdev = to_sunxi_dmadev(chan->device);
	struct dma_slave_config *sconfig = &schan->cfg;
	struct sunxi_desc *txd;
	struct sunxi_dma_lli *l_item, *prev = NULL;
	u32	phy;

	if (dst_nents != src_nents)
		return NULL;

	if (!dst_nents || !src_nents)
		return NULL;

	if (dst_sg == NULL || src_sg == NULL)
		return NULL;

	txd = kzalloc(sizeof(*txd), GFP_NOWAIT);
	if (!txd)
		return NULL;
	vchan_tx_prep(&schan->vc, &txd->vd, flags);

	while ((src_sg != NULL) && (dst_sg != NULL)) {
		l_item = sunxi_alloc_lli(sdev, &phy);
		if (!l_item) {
			sunxi_free_desc(&txd->vd);
			return NULL;
		}

		sunxi_cfg_lli(l_item, sg_dma_address(src_sg),
				sg_dma_address(dst_sg), sg_dma_len(dst_sg),
				sconfig);
		l_item->cfg |= SRC_LINEAR_MODE
			| DST_LINEAR_MODE
			| GET_DST_DRQ(sconfig->slave_id)
			| GET_SRC_DRQ(sconfig->slave_id);

		prev = sunxi_lli_list(prev, l_item, phy, txd);
		src_sg = sg_next(src_sg);
		dst_sg = sg_next(dst_sg);
	}

#ifdef DEBUG
	pr_debug("[sunxi_dma]: First: 0x%08x\n", txd->lli_phys);
	for (prev = txd->lli_virt; prev != NULL; prev = prev->v_lln)
		sunxi_dump_lli(schan, prev);
#endif

	return &txd->vd.tx;
}

static struct dma_async_tx_descriptor *sunxi_prep_slave_sg(
		struct dma_chan *chan, struct scatterlist *sgl,
		unsigned int sg_len, enum dma_transfer_direction dir,
		unsigned long flags, void *context)
{
	struct sunxi_chan *schan = to_sunxi_chan(chan);
	struct sunxi_desc *txd;
	struct sunxi_dma_lli *l_item, *prev = NULL;
	struct sunxi_dmadev *sdev = to_sunxi_dmadev(chan->device);
	struct dma_slave_config *sconfig = &schan->cfg;

	struct scatterlist *sg = NULL;
	u32	phy;
	unsigned int i = 0;

	if (unlikely(!sg_len)) {
		dev_dbg(chan2dev(chan), "sg length is zero!!\n");
		return NULL;
	}

	txd = kzalloc(sizeof(*txd), GFP_NOWAIT);
	if (!txd)
		return NULL;
	vchan_tx_prep(&schan->vc, &txd->vd, flags);

	for_each_sg(sgl, sg, sg_len, i) {
		l_item = sunxi_alloc_lli(sdev, &phy);
		if (!l_item) {
			sunxi_free_desc(&txd->vd);
			return NULL;
		}

		if (dir == DMA_MEM_TO_DEV) {
			sunxi_cfg_lli(l_item, sg_dma_address(sg),
					sconfig->dst_addr, sg_dma_len(sg),
					sconfig);
			l_item->cfg |= DST_IO_MODE
					| SRC_LINEAR_MODE
					| SRC_DRQ(DRQSRC_SDRAM)
					| GET_DST_DRQ(sconfig->slave_id);

		} else if (dir == DMA_DEV_TO_MEM) {
			sunxi_cfg_lli(l_item, sconfig->src_addr,
					sg_dma_address(sg), sg_dma_len(sg),
					sconfig);
			l_item->cfg |= DST_LINEAR_MODE
					| SRC_IO_MODE
					| DST_DRQ(DRQDST_SDRAM)
					| GET_SRC_DRQ(sconfig->slave_id);
		}

		prev = sunxi_lli_list(prev, l_item, phy, txd);
	}

#ifdef DEBUG
	pr_debug("[sunxi_dma]: First: 0x%08x\n", txd->lli_phys);
	for (prev = txd->lli_virt; prev != NULL; prev = prev->v_lln)
		sunxi_dump_lli(schan, prev);
#endif

	return &txd->vd.tx;
}

/**
 * sunxi_prep_dma_cyclic - prepare the cyclic DMA transfer
 * @chan: the DMA channel to prepare
 * @buf_addr: physical DMA address where the buffer starts
 * @buf_len: total number of bytes for the entire buffer
 * @period_len: number of bytes for each period
 * @dir: transfer direction, to or from device
 *
 * Must be called before trying to start the transfer. Returns a valid struct
 * sunxi_cyclic_desc if successful or an ERR_PTR(-errno) if not successful.
 */
struct dma_async_tx_descriptor *sunxi_prep_dma_cyclic(struct dma_chan *chan,
		dma_addr_t buf_addr, size_t buf_len, size_t period_len,
		enum dma_transfer_direction dir, unsigned long flags)
{
	struct sunxi_desc *txd;
	struct sunxi_chan *schan = to_sunxi_chan(chan);
	struct sunxi_dmadev *sdev = to_sunxi_dmadev(chan->device);
	struct sunxi_dma_lli *l_item, *prev = NULL;
	struct dma_slave_config *sconfig = &schan->cfg;

	u32 phy;
	unsigned int periods = buf_len / period_len;
	unsigned int i;

	/*
	 * Not allow duplicate prep dma on cyclic channel.
	 */
	if (schan->desc && schan->cyclic)
		return NULL;

	txd = kzalloc(sizeof(*txd), GFP_NOWAIT);
	if (!txd)
		return NULL;
	vchan_tx_prep(&schan->vc, &txd->vd, flags);

	for (i = 0; i < periods; i++) {
		l_item = sunxi_alloc_lli(sdev, &phy);
		if (!l_item) {
			sunxi_free_desc(&txd->vd);
			return NULL;
		}


		if (dir == DMA_MEM_TO_DEV) {
			sunxi_cfg_lli(l_item, (buf_addr + period_len * i),
					sconfig->dst_addr, period_len,
					sconfig);
			l_item->cfg |= GET_DST_DRQ(sconfig->slave_id)
					| SRC_LINEAR_MODE
					| DST_IO_MODE
					| SRC_DRQ(DRQSRC_SDRAM);
		} else if (dir == DMA_DEV_TO_MEM) {
			sunxi_cfg_lli(l_item, sconfig->src_addr,
					(buf_addr + period_len * i),
					period_len, sconfig);
			if (schan->vc.chan.private && syscfg_version) {
				l_item->cfg |= GET_SRC_DRQ(sconfig->slave_id)
						| DST_LINEAR_MODE
						| SRC_IO_MODE
						| DST_DRQ(DRQDST_SDRAM)
						| BMODE_SEL;
			} else {
				l_item->cfg |= GET_SRC_DRQ(sconfig->slave_id)
						| DST_LINEAR_MODE
						| SRC_IO_MODE
						| DST_DRQ(DRQDST_SDRAM);
			}
		} else if (dir == DMA_DEV_TO_DEV) {
			sunxi_cfg_lli(l_item, sconfig->src_addr,
					sconfig->dst_addr, period_len,
					sconfig);
			l_item->cfg |= GET_SRC_DRQ(sconfig->slave_id)
					| DST_IO_MODE
					| SRC_IO_MODE
					| GET_DST_DRQ(sconfig->slave_id);
		}

		prev = sunxi_lli_list(prev, l_item, phy, txd);

	}

	/* Make a cyclic list */
	prev->p_lln = txd->lli_phys;
	schan->cyclic = true;

#ifdef DEBUG
	pr_debug("[sunxi_dma]: First: 0x%08x\n", txd->lli_phys);
	for (prev = txd->lli_virt; prev != NULL; prev = prev->v_lln)
		sunxi_dump_lli(schan, prev);
#endif

	return &txd->vd.tx;
}

static int sunxi_set_runtime_config(struct dma_chan *chan,
		struct dma_slave_config *config)
{
	struct sunxi_chan *schan = to_sunxi_chan(chan);

	memcpy(&schan->cfg, config, sizeof(struct dma_slave_config));

	convert_burst(&schan->cfg.src_maxburst);
	convert_burst(&schan->cfg.dst_maxburst);

	return 0;
}

static enum dma_status sunxi_tx_status(struct dma_chan *chan,
	      dma_cookie_t cookie, struct dma_tx_state *txstate)
{
	struct sunxi_chan *schan = to_sunxi_chan(chan);
	struct virt_dma_desc *vd;
	enum dma_status ret;
	unsigned long flags;
	size_t bytes = 0;

	ret = dma_cookie_status(chan, cookie, txstate);
	if (ret == DMA_COMPLETE || !txstate)
		return ret;

	spin_lock_irqsave(&schan->vc.lock, flags);
	vd = vchan_find_desc(&schan->vc, cookie);
	if (vd)
		bytes = sunxi_get_desc_size(to_sunxi_desc(&vd->tx));
	else if (schan->desc && schan->desc->vd.tx.cookie == cookie)
		bytes = sunxi_get_chan_size(to_sunxi_chan(chan));

	/*
	 * This cookie not complete yet
	 * Get number of bytes left in the active transactions and queue
	 */
	dma_set_residue(txstate, bytes);
	spin_unlock_irqrestore(&schan->vc.lock, flags);

	return ret;
}

/*
 * sunxi_issue_pending - try to finish work
 * @chan: target DMA channel
 *
 * It will call vchan_issue_pending(), which can move the desc_submitted
 * list to desc_issued list. And we will move the chan to pending list of
 * sunxi_dmadev.
 */
static void sunxi_issue_pending(struct dma_chan *chan)
{
	struct sunxi_chan *schan = to_sunxi_chan(chan);
	struct sunxi_dmadev *sdev = to_sunxi_dmadev(chan->device);
	unsigned long flags;

	spin_lock_irqsave(&schan->vc.lock, flags);
	if (vchan_issue_pending(&schan->vc) && !schan->desc) {
		if (schan->cyclic) {
			sunxi_start_desc(schan);
			goto out;
		}

		spin_lock(&sdev->lock);
		if (list_empty(&schan->node))
			list_add_tail(&schan->node, &sdev->pending);
		spin_unlock(&sdev->lock);
		tasklet_schedule(&sdev->task);
	}
out:
	spin_unlock_irqrestore(&schan->vc.lock, flags);
}

static int sunxi_alloc_chan_resources(struct dma_chan *chan)
{
	struct sunxi_chan *schan = to_sunxi_chan(chan);
	u32 chan_num = schan->vc.chan.chan_id;

	dev_dbg(chan2parent(chan), "%s: Now alloc chan resources!\n", __func__);
	schan->cyclic = false;
	sunxi_dma_channel_bitmap |= 1 << chan_num;

	return 0;
}

/*
 * sunxi_free_chan_resources - free the resources of channel
 * @chan: the channel to free
 */
static void sunxi_free_chan_resources(struct dma_chan *chan)
{
	struct sunxi_chan *schan = to_sunxi_chan(chan);
	u32 chan_num = schan->vc.chan.chan_id;

	sunxi_dma_channel_bitmap &= ~(1 << chan_num);

	vchan_free_chan_resources(&schan->vc);

	dev_dbg(chan2parent(chan), "%s: Now free chan resources!!\n", __func__);
}

/*
 * sunxi_chan_free - free the channel on dmadevice
 * @sdev: the dmadevice of sunxi
 */
static inline void sunxi_chan_free(struct sunxi_dmadev *sdev)
{
	struct sunxi_chan *ch;

	tasklet_kill(&sdev->task);
	while (!list_empty(&sdev->dma_dev.channels)) {
		ch = list_first_entry(&sdev->dma_dev.channels,
				struct sunxi_chan, vc.chan.device_node);
		list_del(&ch->vc.chan.device_node);
		tasklet_kill(&ch->vc.task);
		kfree(ch);
	}

}

static void sunxi_dma_hw_init(struct sunxi_dmadev *sunxi_dev)
{
#if defined(CONFIG_SUNXI_SMC)
	sunxi_smc_writel(0xff, sunxi_dev->pbase + DMA_SECU);
#endif

#if defined(CONFIG_ARCH_SUN8IW3) || \
	defined(CONFIG_ARCH_SUN8IW5) || \
	defined(CONFIG_ARCH_SUN8IW6) || \
	defined(CONFIG_ARCH_SUN8IW8)
	writel(DMA_MCLK_GATE|DMA_COMMON_GATE|DMA_CHAN_GATE,
						sunxi_dev->base + DMA_GATE);
#endif
}

static int sunxi_probe(struct platform_device *pdev)
{
	struct sunxi_dmadev *sunxi_dev;
	struct sunxi_chan *schan;
	struct resource *res;
	int irq;
	int ret, i;

	pdev->dev.dma_mask = &sunxi_dma_mask;
	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);

	sunxi_dev = kzalloc(sizeof(struct sunxi_dmadev), GFP_KERNEL);
	if (!sunxi_dev)
		return -ENOMEM;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		ret = -EINVAL;
		goto io_err;
	}

	sunxi_dev->pbase = res->start;
	sunxi_dev->base = ioremap(res->start, resource_size(res));
	if (!sunxi_dev->base) {
		dev_err(&pdev->dev, "Remap I/O memory failed!\n");
		ret = -ENOMEM;
		goto io_err;
	}
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		ret = irq;
		goto irq_err;
	}

	ret = request_irq(irq, sunxi_dma_interrupt, IRQF_SHARED,
			dev_name(&pdev->dev), sunxi_dev);
	if (ret) {
		dev_err(&pdev->dev, "NO IRQ found!!!\n");
		goto irq_err;
	}

	sunxi_dev->ahb_clk = clk_get(&pdev->dev, "dma");
	if (!sunxi_dev->ahb_clk) {
		dev_err(&pdev->dev, "NO clock to dma!!!\n");
		ret = -EINVAL;
		goto clk_err;
	}
	sunxi_dev->lli_pool = dma_pool_create(dev_name(&pdev->dev), &pdev->dev,
			sizeof(struct sunxi_dma_lli), 4/* word alignment */, 0);
	if (!sunxi_dev->lli_pool) {
		ret = -ENOMEM;
		goto pool_err;
	}

	platform_set_drvdata(pdev, sunxi_dev);
	INIT_LIST_HEAD(&sunxi_dev->pending);
	spin_lock_init(&sunxi_dev->lock);

	/* Initialize dmaengine */
	dma_cap_set(DMA_MEMCPY, sunxi_dev->dma_dev.cap_mask);
	dma_cap_set(DMA_SLAVE, sunxi_dev->dma_dev.cap_mask);
	dma_cap_set(DMA_CYCLIC, sunxi_dev->dma_dev.cap_mask);
	dma_cap_set(DMA_SG, sunxi_dev->dma_dev.cap_mask);

	INIT_LIST_HEAD(&sunxi_dev->dma_dev.channels);
	sunxi_dev->dma_dev.device_alloc_chan_resources	= sunxi_alloc_chan_resources;
	sunxi_dev->dma_dev.device_free_chan_resources	= sunxi_free_chan_resources;
	sunxi_dev->dma_dev.device_tx_status		= sunxi_tx_status;
	sunxi_dev->dma_dev.device_issue_pending		= sunxi_issue_pending;
	sunxi_dev->dma_dev.device_prep_dma_sg		= sunxi_prep_dma_sg;
	sunxi_dev->dma_dev.device_prep_slave_sg		= sunxi_prep_slave_sg;
	sunxi_dev->dma_dev.device_prep_dma_cyclic	= sunxi_prep_dma_cyclic;
	sunxi_dev->dma_dev.device_prep_dma_memcpy	= sunxi_prep_dma_memcpy;
	sunxi_dev->dma_dev.device_config		= sunxi_set_runtime_config;
	sunxi_dev->dma_dev.device_pause			= sunxi_dma_pause;
	sunxi_dev->dma_dev.device_resume		= sunxi_dma_resume;
	sunxi_dev->dma_dev.device_terminate_all		= sunxi_terminate_all;

	sunxi_dev->dma_dev.dev = &pdev->dev;

	tasklet_init(&sunxi_dev->task, sunxi_dma_tasklet,
			(unsigned long)sunxi_dev);

	for (i = 0; i < NR_MAX_CHAN; i++) {
		schan = kzalloc(sizeof(*schan), GFP_KERNEL);
		if (!schan) {
			dev_err(&pdev->dev, "no memory for channel\n");
			ret = -ENOMEM;
			goto chan_err;
		}
		INIT_LIST_HEAD(&schan->node);
		sunxi_dev->dma_dev.chancnt++;
		schan->vc.desc_free = sunxi_free_desc;
		vchan_init(&schan->vc, &sunxi_dev->dma_dev);
	}

	/* Register the sunxi-dma to dmaengine */
	ret = dma_async_device_register(&sunxi_dev->dma_dev);
	if (ret) {
		dev_warn(&pdev->dev, "Failed to register DMA engine device\n");
		goto chan_err;
	}

	/* All is ok, and open the clock */
	clk_prepare_enable(sunxi_dev->ahb_clk);
	/* init hw dma */
	sunxi_dma_hw_init(sunxi_dev);

	/*
	 * Determine whether to support the BMODE Mode according to
	 * SYSCFG Version Register from bit[2:0] for IC version,if support
	 * this mode and it will be read as 0x2
	 */
	syscfg_version = readl((void __iomem *)ioremap(SYSCFG_VER, 4));
	syscfg_version &= SYS_VER_MASK;

	return 0;

chan_err:
	sunxi_chan_free(sunxi_dev);
	platform_set_drvdata(pdev, NULL);
	dma_pool_destroy(sunxi_dev->lli_pool);
pool_err:
	clk_put(sunxi_dev->ahb_clk);
clk_err:
	free_irq(irq, sunxi_dev);
irq_err:
	iounmap(sunxi_dev->base);
io_err:
	kfree(sunxi_dev);
	return ret;
}

static int sunxi_remove(struct platform_device *pdev)
{
	struct sunxi_dmadev *sunxi_dev = platform_get_drvdata(pdev);

	dma_async_device_unregister(&sunxi_dev->dma_dev);

	sunxi_chan_free(sunxi_dev);

	free_irq(platform_get_irq(pdev, 0), sunxi_dev);
	dma_pool_destroy(sunxi_dev->lli_pool);
	clk_disable_unprepare(sunxi_dev->ahb_clk);
	clk_put(sunxi_dev->ahb_clk);
	iounmap(sunxi_dev->base);
	kfree(sunxi_dev);

	return 0;
}

static void sunxi_shutdown(struct platform_device *pdev)
{
	struct sunxi_dmadev *sdev = platform_get_drvdata(pdev);

	clk_disable_unprepare(sdev->ahb_clk);
}

static int sunxi_suspend_noirq(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_dmadev *sunxi_dev = platform_get_drvdata(pdev);

	if (!sunxi_dma_channel_bitmap)
		clk_disable_unprepare(sunxi_dev->ahb_clk);
	return 0;
}

static int sunxi_resume_noirq(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_dmadev *sunxi_dev = platform_get_drvdata(pdev);

	if (!sunxi_dma_channel_bitmap)
		clk_prepare_enable(sunxi_dev->ahb_clk);
	sunxi_dma_hw_init(sunxi_dev);
	return 0;
}

static const struct dev_pm_ops sunxi_dev_pm_ops = {
	.suspend_noirq = sunxi_suspend_noirq,
	.resume_noirq = sunxi_resume_noirq,
	.freeze_noirq = sunxi_suspend_noirq,
	.thaw_noirq = sunxi_resume_noirq,
	.restore_noirq = sunxi_resume_noirq,
	.poweroff_noirq = sunxi_suspend_noirq,
};

#ifndef CONFIG_OF
static struct resource sunxi_dma_reousce[] = {
	[0] = {
		.start = DMA_PHYS_BASE,
		.end = DMA_PHYS_BASE + DMA_PARA(15),
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = DMA_IRQ_ID,
		.end = DMA_IRQ_ID,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device sunxi_dma_device = {
	.name = "sunxi_dmac",
	.id = -1,
	.resource = sunxi_dma_reousce,
	.num_resources = ARRAY_SIZE(sunxi_dma_reousce),
	.dev = {
		.dma_mask = &sunxi_dma_mask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
};
#else
static const struct of_device_id sunxi_dma_match[] = {
	{ .compatible = "allwinner,sun50i-dma", },
	{ .compatible = "allwinner,sun8i-dma", },
	{},
};
#endif

static struct platform_driver sunxi_dma_driver = {
	.probe		= sunxi_probe,
	.remove		= sunxi_remove,
	.shutdown	= sunxi_shutdown,
	.driver = {
		.name	= "sunxi_dmac",
		.pm	= &sunxi_dev_pm_ops,
		.of_match_table = sunxi_dma_match,
	},
};

bool sunxi_dma_filter_fn(struct dma_chan *chan, void *param)
{
	bool ret = false;

	if (chan->device->dev->driver == &sunxi_dma_driver.driver) {
		const char *p = param;

		ret = !strcmp("sunxi_dmac", p);
		pr_debug("[sunxi_rdma]: sunxi_dma_filter_fn: %s\n", p);
	}
	return ret;
}
EXPORT_SYMBOL_GPL(sunxi_dma_filter_fn);

static int __init sunxi_dma_init(void)
{
	int ret;
#ifndef CONFIG_OF
	platform_device_register(&sunxi_dma_device);
#endif
	ret = platform_driver_register(&sunxi_dma_driver);
	return ret;
}
subsys_initcall(sunxi_dma_init);

static void __exit sunxi_dma_exit(void)
{
	platform_driver_unregister(&sunxi_dma_driver);
#ifndef CONFIG_OF
	platform_device_unregister(&sunxi_dma_device);
#endif

}
module_exit(sunxi_dma_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Sunxi DMA Controller driver");
MODULE_AUTHOR("Shuge");
MODULE_AUTHOR("Wim Hwang");
MODULE_ALIAS("platform:sunxi_dmac");
