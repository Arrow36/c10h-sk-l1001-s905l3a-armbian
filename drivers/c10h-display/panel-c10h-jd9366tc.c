// SPDX-License-Identifier: GPL-2.0
/* C10H lcd_8 / JD9366TC: mode and commands taken from its stock DT. */
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/io.h>
#include <drm/drm_connector.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include "c10h-panel-init.h"

struct c10h_panel {
    struct drm_panel panel;
    struct mipi_dsi_device *dsi;
    struct gpio_desc *control[3];
};

static struct c10h_panel *to_c10h(struct drm_panel *panel)
{
    return container_of(panel, struct c10h_panel, panel);
}

static int c10h_sequence(struct c10h_panel *ctx, const u8 *seq, size_t size)
{
    size_t i = 0;
    int ret;
    while (i + 2 <= size) {
        u8 type = seq[i++], count = seq[i++];
        if (type == 0xff) {
            if (!count || count == 0xff)
                return 0;
            msleep(count);
            continue;
        }
        if (i + count > size ||
            !((type == 0x05 && count == 1) ||
              (type == 0x15 && count == 2) ||
              (type == 0x39 && count > 2)))
            return -EINVAL;
        ret = mipi_dsi_dcs_write_buffer(ctx->dsi, seq + i, count);
        if (ret < 0) {
            dev_err(&ctx->dsi->dev, "DCS command offset %zu failed: %d\n", i, ret);
            return ret;
        }
        i += count;
    }
    return -EINVAL;
}

static int c10h_control(struct c10h_panel *ctx, unsigned int index,
                        int value, unsigned int delay)
{
    int ret = gpiod_direction_output(ctx->control[index], value);
    if (ret)
        return ret;
    if (delay)
        msleep(delay);
    return 0;
}

static void c10h_power_off(struct c10h_panel *ctx)
{
    c10h_control(ctx, 2, 0, 5);
    c10h_control(ctx, 1, 0, 5);
    c10h_control(ctx, 0, 1, 40);
}

static int c10h_prepare(struct drm_panel *panel)
{
    struct c10h_panel *ctx = to_c10h(panel);
    int ret;
    void __iomem *vpu;
    void __iomem *dsi;

    /* Stock power_on_step: controls C1, C2, A11, signal, then C1. */
    ret = c10h_control(ctx, 2, 0, 10);
    if (ret) goto fail;
    ret = c10h_control(ctx, 0, 1, 100);
    if (ret) goto fail;
    ret = c10h_control(ctx, 0, 0, 40);
    if (ret) goto fail;
    ret = c10h_control(ctx, 1, 1, 30);
    if (ret) goto fail;
    ret = c10h_control(ctx, 1, 0, 30);
    if (ret) goto fail;
    ret = c10h_control(ctx, 1, 1, 120);
    if (ret) goto fail;
    ret = c10h_sequence(ctx, c10h_init_on, sizeof(c10h_init_on));
    if (ret) goto fail;
    ret = c10h_control(ctx, 2, 1, 0);
    if (ret) goto fail;

    /* Experimental C10H register settings; validate ordering and output on the target kernel. */
    vpu = ioremap(0xff900000, 0x40000);
    if (vpu) {
        writel_relaxed(0x0001c040, vpu + 0x38a0 * 4); // OSD1 HDR2 RGB->YUV709l matrix
        writel_relaxed(0x05000320, vpu + 0x1df1 * 4); // VPP OSD1 input size: 800x1280
        writel_relaxed(0x82820501, vpu + 0x1a2b * 4); // VIU OSD1 FIFO: burst 96, hold 8 lines
        writel_relaxed(0x0000077f, vpu + 0x1df5 * 4); // VPP OFIFO size: 1919
        writel_relaxed(0x08080808, vpu + 0x1d22 * 4); // VPP hold lines: 8 lines
        iounmap(vpu);
    }
    dsi = ioremap(0xffd07000, 0x1000);
    if (dsi) {
        writel_relaxed(0x00512430, dsi + 0x3c8);       // DSI TOP HS/VS active-high
        writel_relaxed(0x00000000, dsi + 0x014);       // DSI DPI polarity
        writel_relaxed(0x000002ff, dsi + 0x050);       // VID_HLINE_TIME = 767
        iounmap(dsi);
    }

    dev_info(&ctx->dsi->dev, "C10H stock panel initialization & hardware MMIO fix complete\n");
    return 0;
fail:
    c10h_power_off(ctx);
    return ret;
}

static int c10h_unprepare(struct drm_panel *panel)
{
    struct c10h_panel *ctx = to_c10h(panel);
    int ret = c10h_sequence(ctx, c10h_init_off, sizeof(c10h_init_off));
    msleep(120);
    c10h_power_off(ctx);
    return ret;
}

static const struct drm_display_mode c10h_mode = {
    .clock = 75857,
    .hdisplay = 800, .hsync_start = 840, .hsync_end = 848, .htotal = 876,
    .vdisplay = 1280, .vsync_start = 1420, .vsync_end = 1428, .vtotal = 1448,
    .flags = DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC,
};

static int c10h_get_modes(struct drm_panel *panel, struct drm_connector *connector)
{
    struct drm_display_mode *mode = drm_mode_duplicate(connector->dev, &c10h_mode);
    if (!mode)
        return -ENOMEM;
    drm_mode_set_name(mode);
    mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
    drm_mode_probed_add(connector, mode);
    connector->display_info.bpc = 8;
    return 1;
}

static const struct drm_panel_funcs c10h_funcs = {
    .prepare = c10h_prepare, .unprepare = c10h_unprepare,
    .get_modes = c10h_get_modes,
};

static int c10h_probe(struct mipi_dsi_device *dsi)
{
    static const char * const names[] = { "c2", "a11", "c1" };
    struct c10h_panel *ctx;
    int i, ret;
    ctx = devm_kzalloc(&dsi->dev, sizeof(*ctx), GFP_KERNEL);
    if (!ctx)
        return -ENOMEM;
    ctx->dsi = dsi;
    mipi_dsi_set_drvdata(dsi, ctx);
    dsi->lanes = 4;
    dsi->format = MIPI_DSI_FMT_RGB888;
    dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST |
                      MIPI_DSI_MODE_LPM;
    for (i = 0; i < ARRAY_SIZE(names); i++) {
        ctx->control[i] = devm_gpiod_get(&dsi->dev, names[i], GPIOD_ASIS);
        if (IS_ERR(ctx->control[i]))
            return dev_err_probe(&dsi->dev, PTR_ERR(ctx->control[i]),
                                 "failed to get %s GPIO\n", names[i]);
    }
    drm_panel_init(&ctx->panel, &dsi->dev, &c10h_funcs, DRM_MODE_CONNECTOR_DSI);
    ret = drm_panel_of_backlight(&ctx->panel);
    if (ret)
        return ret;
    drm_panel_add(&ctx->panel);
    ret = mipi_dsi_attach(dsi);
    if (ret)
        drm_panel_remove(&ctx->panel);
    return ret;
}

static void c10h_remove(struct mipi_dsi_device *dsi)
{
    struct c10h_panel *ctx = mipi_dsi_get_drvdata(dsi);
    mipi_dsi_detach(dsi);
    drm_panel_remove(&ctx->panel);
}

static const struct of_device_id c10h_of_match[] = {
    { .compatible = "arrow36,c10h-jd9366tc" }, { }
};
MODULE_DEVICE_TABLE(of, c10h_of_match);
static struct mipi_dsi_driver c10h_driver = {
    .probe = c10h_probe, .remove = c10h_remove,
    .driver = { .name = "panel-c10h-jd9366tc", .of_match_table = c10h_of_match },
};
module_mipi_dsi_driver(c10h_driver);
MODULE_DESCRIPTION("Experimental C10H stock JD9366TC panel");
MODULE_LICENSE("GPL");
