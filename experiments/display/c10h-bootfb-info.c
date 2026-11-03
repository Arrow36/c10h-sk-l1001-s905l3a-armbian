// SPDX-License-Identifier: GPL-2.0
/* Read the firmware's OSD canvas; RD_EN selects a LUT entry, never writes it. */
#include <linux/io.h>
#include <linux/module.h>

static int __init c10h_bootfb_info_init(void)
{
    void __iomem *vpu = ioremap(0xff900000, 0x10000);
    void __iomem *canvas = ioremap(0xff638048, 0x14);
    u32 cfg, low, high, idx, pitch, height, address;
    int osd;
    if (!vpu || !canvas) {
        if (vpu) iounmap(vpu);
        if (canvas) iounmap(canvas);
        return -ENOMEM;
    }
    for (osd = 0; osd < 2; osd++) {
        unsigned int reg = osd ? 0x1a3b : 0x1a1b;
        cfg = readl(vpu + reg * 4);
        idx = (cfg >> 16) & 0xff;
        writel(BIT(8) | idx, canvas + 8);
        low = readl(canvas + 0x0c);
        high = readl(canvas + 0x10);
        address = (low & 0x1fffffff) << 3;
        pitch = (((low >> 29) & 7) | ((high & 0x1ff) << 3)) * 8;
        height = (high >> 9) & 0x1fff;
        pr_info("c10h-bootfb: OSD%d cfg=%08x canvas=%u raw=%08x:%08x address=%08x pitch=%u height=%u\n",
                osd + 1, cfg, idx, high, low, address, pitch, height);
        pr_info("c10h-bootfb: OSD%d srcX=%08x srcY=%08x dstX=%08x dstY=%08x\n",
                osd + 1, readl(vpu + (reg + 1) * 4),
                readl(vpu + (reg + 2) * 4), readl(vpu + (reg + 3) * 4),
                readl(vpu + (osd ? 0x1a64 : 0x1a13) * 4));
    }
    pr_info("c10h-bootfb: ENCL=%x HMAX=%u VMAX=%u HACTIVE=%u..%u VACTIVE=%u..%u\n",
            readl(vpu + 0x1ca0 * 4), readl(vpu + 0x1cb0 * 4),
            readl(vpu + 0x1cbb * 4), readl(vpu + 0x1cb2 * 4),
            readl(vpu + 0x1cb1 * 4), readl(vpu + 0x1cb4 * 4),
            readl(vpu + 0x1cb3 * 4));
    {
      const unsigned int r[] = {0x1a10,0x1a14,0x1a15,0x1a2d,0x1a30,0x1a65,0x1a66,0x1a4d,0x3d80,0x3d81,0x3d88,0x3d8c,0x3d90,0x3d98,0x3d99,0x3d9a,0x1c98,0x1c99,0x1ca8,0x1cc7,0x271a,0x1e10,0x1e1b,0x1e14,0x1e15,0x39b0,0x39b1,0x39b2,0x1d26,0x1df1,0x1df5,0x1df6,0x1dfb,0x1dfc,0x1dfd,0x1dfe,0x1da5,0x1a0c,0x1a0e,0x1a20,0x1a2b,0x1dc0,0x1dc1,0x1dc2,0x1dc3,0x1dc4,0x1dc5,0x1dc6,0x1dc7,0x1dc8,0x1dc9,0x1dca,0x1dcb,0x39bb,0x39bc,0x3d6d,0x1d21,0x1d22,0x1d27,0x1c90,0x1c91,0x1c92,0x1c93,0x1c94,0x1c95,0x1c96,0x1c97,0x1cad,0x1cae,0x1caf,0x1cbf,0x1cc0,0x1cc1,0x1cc2,0x38a0,0x1dff,0x1da4,0x3130,0x3131,0x1a2e,0x1d00,0x1d69,0x3968,0x3969,0x1cbc,0x1cbd,0x1cbe};
      int i;
      for (i = 0; i < ARRAY_SIZE(r); i++)
        pr_info("c10h-bootfb: reg[%04x]=%08x\n",r[i],readl(vpu+r[i]*4));
    }
    {
      const unsigned int offsets[] = {0x04,0x08,0x10,0x14,0x18,0x2c,0x34,0x38,0x3c,0x40,0x44,0x48,0x4c,0x50,0x54,0x58,0x5c,0x60,0x94,0x98,0x9c,0xa0,0xa4,0xb0,0x3c8};
      const unsigned int phy_regs[] = {0,4,8,12,16,20,24,28,32,36,40,44,48,52,56,60,64,68};
      void __iomem *dsi=ioremap(0xffd07000,0x1000);
      void __iomem *phy=ioremap(0xff644000,0x2000);
      int i;
      if(dsi) {
       for(i=0;i<ARRAY_SIZE(offsets);i++)pr_info("c10h-snapshot: DSI[%03x]=%08x\n",offsets[i],readl(dsi+offsets[i]));
       iounmap(dsi);
      }
      if(phy) {
       for(i=0;i<ARRAY_SIZE(phy_regs);i++)pr_info("c10h-snapshot: PHY[%03x]=%08x\n",phy_regs[i],readl(phy+phy_regs[i]));
       iounmap(phy);
      }
    }
    iounmap(canvas);
    iounmap(vpu);
    return 0;
}
static void __exit c10h_bootfb_info_exit(void) { }
module_init(c10h_bootfb_info_init);
module_exit(c10h_bootfb_info_exit);
MODULE_LICENSE("GPL");
