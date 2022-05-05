#include "bootpack.h"
#include <stdio.h>

/**
 * @brief 程序入口点
 *
 */
void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
    char s[40], mcursor[256];
    int mx, my;

    init_palette();
    init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);
    mx = (binfo->scrnx - 16) / 2;       // mid_x，16是鼠标宽度
    my = (binfo->scrny - 28 - 16) / 2;  // mid_y，
    init_mouse_cursor8(mcursor, COL8_008484);
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
    sprintf(s, "(%d, %d)", mx, my);
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);

    for (;;) {
        io_hlt();
    }
}
