// SPDX-License-Identifier: GPL-2.0
/* Reversible ENCL test patterns, matching the stock lcd_debug_test table. */
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/sizes.h>

static void __iomem *vpu;
static unsigned int pattern;
static unsigned int dpi_polarity;
static u32 saved_dpi_polarity;
static bool dpi_changed;
static unsigned int hline_time;
static u32 saved_hline;
static bool hline_changed;
static unsigned int stock_sync;
static u32 saved_top_sync;
static bool top_changed;
static const unsigned int regs[] = {
    0x1cc7, /* ENCL_VIDEO_RGBIN_CTRL */
    0x1c99, 0x1c9a, 0x1c9b, 0x1c9c, /* mode, Y, Cb, Cr */
    0x1c9d, 0x1c9e, 0x1c98, 0x1ca8 /* start, width, enable, mode_adv */
};
static u32 saved[ARRAY_SIZE(regs)];
static u32 rd(unsigned int reg) { return readl(vpu + reg * 4); }
static void wr(unsigned int reg, u32 val) { writel(val, vpu + reg * 4); }

static void apply_pattern(void)
{
    int i;
    u32 colors[3] = { 0, 0, 0 };
    if (!pattern) {
        for (i = 0; i < ARRAY_SIZE(regs); i++)
            wr(regs[i], saved[i]);
        return;
    }
    /* 1=color bars, 2=red, 3=green, 4=blue. */
    if (pattern >= 2)
        colors[pattern - 2] = 0x3ff;
    wr(0x1cc7, pattern == 1 ? 1 : 3);
    wr(0x1c99, pattern == 1 ? 1 : 0);
    wr(0x1c9a, colors[0]);
    wr(0x1c9b, colors[1]);
    wr(0x1c9c, colors[2]);
    wr(0x1c9d, rd(0x1cb2)); /* HAVON_BEGIN */
    wr(0x1c9e, 800 / 9);
    wr(0x1c98, 1);
    wr(0x1ca8, rd(0x1ca8) & ~BIT(3));
    pr_info("c10h-lcd: hardware pattern %u applied\n", pattern);
}

static int pattern_set(const char *value, const struct kernel_param *kp)
{
    unsigned int next;
    int ret = kstrtouint(value, 0, &next);
    if (ret) return ret;
    if (next > 4) return -EINVAL;
    pattern = next;
    if (vpu) apply_pattern();
    return 0;
}
static const struct kernel_param_ops pattern_ops = {
    .set = pattern_set, .get = param_get_uint
};
module_param_cb(pattern, &pattern_ops, &pattern, 0600);
MODULE_PARM_DESC(pattern, "0=restore, 1=bars, 2=red, 3=green, 4=blue");

static unsigned int gamma_reload;
static int gamma_set(const char *value, const struct kernel_param *kp)
{
    unsigned int next, channel, index;
    u32 state, old;
    int ret = kstrtouint(value, 0, &next);
    if (ret) return ret;
    if (next != 1 || !vpu) return -EINVAL;
    old = rd(0x1400);
    wr(0x1400, old & ~BIT(0));
    for (channel = 0; channel < 3; channel++) {
        u32 mask = BIT(10 - channel);
        ret = readl_poll_timeout(vpu + 0x1400 * 4, state,
                                 state & BIT(5), 10, 10000);
        if (ret) goto fail;
        wr(0x1402, BIT(11) | mask);
        for (index = 0; index < 256; index++) {
            ret = readl_poll_timeout(vpu + 0x1400 * 4, state,
                                     state & BIT(4), 10, 10000);
            if (ret) goto fail;
            wr(0x1401, index * 4);
        }
        ret = readl_poll_timeout(vpu + 0x1400 * 4, state,
                                 state & BIT(5), 10, 10000);
        if (ret) goto fail;
        wr(0x1402, BIT(11) | mask | 0x23);
    }
    wr(0x1400, old | BIT(0));
    gamma_reload++;
    pr_info("c10h-lcd: default linear Gamma reloaded after ENCL ready\n");
    return 0;
fail:
    wr(0x1400, old);
    pr_err("c10h-lcd: late Gamma reload timed out\n");
    return ret;
}
static const struct kernel_param_ops gamma_ops = {
    .set = gamma_set, .get = param_get_uint
};
module_param_cb(gamma_reload, &gamma_ops, &gamma_reload, 0600);

static int dpi_set(const char *value, const struct kernel_param *kp)
{
    void __iomem *dsi;
    unsigned int next;
    u32 old;
    int ret = kstrtouint(value, 0, &next);
    if (ret) return ret;
    if (next > 0x1f || !vpu) return -EINVAL;
    dsi = ioremap(0xffd07000, 0x1000);
    if (!dsi) return -ENOMEM;
    old = readl(dsi + 0x14);
    if (!dpi_changed) saved_dpi_polarity = old;
    dpi_changed = true;
    dpi_polarity = next;
    writel(next, dsi + 0x14);
    pr_info("c10h-dsi: DPI polarity %x -> %x\n", old, next);
    iounmap(dsi);
    return 0;
}
static const struct kernel_param_ops dpi_ops = {
    .set = dpi_set, .get = param_get_uint
};
module_param_cb(dpi_polarity, &dpi_ops, &dpi_polarity, 0600);

static int hline_set(const char *value, const struct kernel_param *kp)
{
    void __iomem *dsi;
    unsigned int next;
    u32 old;
    int ret = kstrtouint(value, 0, &next);
    if (ret) return ret;
    if (!next || next > 0x7fff || !vpu) return -EINVAL;
    dsi = ioremap(0xffd07000, 0x1000);
    if (!dsi) return -ENOMEM;
    old = readl(dsi + 0x50);
    if (!hline_changed) saved_hline = old;
    hline_changed = true;
    hline_time = next;
    writel(next, dsi + 0x50);
    pr_info("c10h-dsi: HLINE %u -> %u\n", old, next);
    iounmap(dsi);
    return 0;
}
static const struct kernel_param_ops hline_ops = {
    .set = hline_set, .get = param_get_uint
};
module_param_cb(hline_time, &hline_ops, &hline_time, 0600);

static int stock_sync_set(const char *value, const struct kernel_param *kp)
{
    void __iomem *dsi;
    unsigned int next;
    u32 old;
    int ret = kstrtouint(value, 0, &next);
    if (ret) return ret;
    if (next != 1 || !vpu) return -EINVAL;
    dsi = ioremap(0xffd07000, 0x1000);
    if (!dsi) return -ENOMEM;
    old = readl(dsi + 0x3c8);
    if (!top_changed) saved_top_sync = old;
    if (!dpi_changed) saved_dpi_polarity = readl(dsi + 0x14);
    dpi_changed = true;
    top_changed = true;
    stock_sync = 1;
    writel(old | BIT(4) | BIT(5), dsi + 0x3c8);
    writel(0, dsi + 0x14);
    pr_info("c10h-dsi: stock TOP sync %x -> %x, DPI polarity -> 0\n",
             old, readl(dsi + 0x3c8));
    iounmap(dsi);
    return 0;
}
static const struct kernel_param_ops stock_sync_ops = {
    .set = stock_sync_set, .get = param_get_uint
};
module_param_cb(stock_sync, &stock_sync_ops, &stock_sync, 0600);

static int __init c10h_diagnostic_init(void)
{
    int i;
    vpu = ioremap(0xff900000, SZ_64K);
    if (!vpu) return -ENOMEM;
    for (i = 0; i < ARRAY_SIZE(regs); i++) saved[i] = rd(regs[i]);
    pr_info("c10h-lcd: ENCL_EN=%x MODE=%x ADV=%x MAX_PX=%u MAX_LN=%u GAMMA=%x RGBIN=%x\n",
            rd(0x1ca0), rd(0x1ca7), rd(0x1ca8), rd(0x1cb0), rd(0x1cbb),
            rd(0x1400), rd(0x1cc7));
    {
        void __iomem *dsi = ioremap(0xffd07000, 0x1000);
        pr_info("c10h-lcd: HAVON=%u..%u VAVON=%u..%u\n",
                rd(0x1cb2), rd(0x1cb1), rd(0x1cb4), rd(0x1cb3));
        if (dsi) {
            pr_info("c10h-dsi-reg: PWR=%x COLOR=%x MODE=%x VIDMODE=%x PKTSIZE=%u CHUNKS=%u NULL=%u\n",
                readl(dsi+4), readl(dsi+0x10), readl(dsi+0x34), readl(dsi+0x38),
                readl(dsi+0x3c), readl(dsi+0x40), readl(dsi+0x44));
            pr_info("c10h-dsi-reg: HSA=%u HBP=%u HLINE=%u VSA=%u VBP=%u VFP=%u VACT=%u LANES=%x PHY=%x TOP=%x\n",
                readl(dsi+0x48), readl(dsi+0x4c), readl(dsi+0x50), readl(dsi+0x54),
                readl(dsi+0x58), readl(dsi+0x5c), readl(dsi+0x60),
                readl(dsi+0xa4), readl(dsi+0xb0), readl(dsi+0x3c8));
            iounmap(dsi);
        }
    }
    if (pattern) apply_pattern();
    return 0;
}
static void __exit c10h_diagnostic_exit(void)
{
    if (top_changed) {
        void __iomem *dsi = ioremap(0xffd07000, 0x1000);
        if (dsi) { writel(saved_top_sync, dsi + 0x3c8); iounmap(dsi); }
    }
    if (hline_changed) {
        void __iomem *dsi = ioremap(0xffd07000, 0x1000);
        if (dsi) { writel(saved_hline, dsi + 0x50); iounmap(dsi); }
    }
    if (dpi_changed) {
        void __iomem *dsi = ioremap(0xffd07000, 0x1000);
        if (dsi) { writel(saved_dpi_polarity, dsi + 0x14); iounmap(dsi); }
    }
    pattern = 0;
    apply_pattern();
    iounmap(vpu);
    vpu = NULL;
}
module_init(c10h_diagnostic_init);
module_exit(c10h_diagnostic_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("C10H reversible ENCL diagnostic patterns");
