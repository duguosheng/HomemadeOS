#include "bootpack.h"
#include <stdio.h>

/**
 * @brief 程序入口点
 *
 */
void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
    char s[40], keybuf[32], mousebuf[128];
    int mx, my;  // 鼠标坐标
    int i;
    struct MOUSE_DEC mdec;
    unsigned int memtotal;
    struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;   // 将内存管理结构体放在MEMMAN_ADDR处（高地址）
    struct SHTCTL *shtctl;
    struct SHEET *sht_back, *sht_mouse;         // back=background
    unsigned char *buf_back, buf_mouse[256];

    init_gdtidt();
    init_pic();
    io_sti();

    fifo8_init(&keyfifo, 32, keybuf);
    fifo8_init(&mousefifo, 128, mousebuf);
    io_out8(PIC0_IMR, 0xf9); // 键盘: IRQ1(INT 0x21)  PIC0允许PIC1和键盘的中断(11111001)
    io_out8(PIC1_IMR, 0xef); // 鼠标: IRQ12(INT 0x2c) PIC1允许鼠标中断(11101111)

    init_keyboard();
    enable_mouse(&mdec);
    memtotal = memtest(0x00400000, 0xbfffffff);
    memman_init(memman);
    memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff */
    memman_free(memman, 0x00400000, memtotal - 0x00400000);

    init_palette();
    shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
    sht_back = sheet_alloc(shtctl);
    sht_mouse = sheet_alloc(shtctl);
    buf_back = (unsigned char *)memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
    sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);   // 没有透明色
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);                     // 透明色号99
    init_screen8(buf_back, binfo->scrnx, binfo->scrny);
    init_mouse_cursor8(buf_mouse, 99);      // 将背景设置为99
    sheet_slide(shtctl, sht_back, 0, 0);    // 绘制背景图层
    mx = (binfo->scrnx - 16) / 2;           // mid_x，16是鼠标宽度
    my = (binfo->scrny - 28 - 16) / 2;      // mid_y，16是鼠标宽度，28是底部横条高度
    sheet_slide(shtctl, sht_mouse, mx, my); // 绘制鼠标图层，起始点位于屏幕中央
    sheet_updown(shtctl, sht_back, 0);      // 背景高度为0
    sheet_updown(shtctl, sht_mouse, 1);     // 鼠标高度为1（所有图层中最高）
    sprintf(s, "(%3d, %3d)", mx, my);
    putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
    sprintf(s, "memory %dMB   free : %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
    putfonts8_asc(buf_back, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
    sheet_refresh(shtctl);

    for (;;) {
        io_cli();           // 禁用中断
        if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) {
            io_stihlt();    // 启用中断，并使CPU暂停
        } else {
            if (fifo8_status(&keyfifo) != 0) {
                i = fifo8_get(&keyfifo);
                io_sti();       // 启用中断
                sprintf(s, "%02X", i);
                boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
                putfonts8_asc(buf_back, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
                sheet_refresh(shtctl);
            } else if (fifo8_status(&mousefifo) != 0) {
                i = fifo8_get(&mousefifo);
                io_sti();       // 启用中断
                if (mouse_decode(&mdec, i) == 1) {
                    // 3字节一组显示
                    sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
                    // 如果鼠标指定的按键按下，则将s中字符改为大写
                    if ((mdec.btn & 0x01) != 0) {
                        s[1] = 'L';
                    }
                    if ((mdec.btn & 0x02) != 0) {
                        s[3] = 'R';
                    }
                    if ((mdec.btn & 0x04) != 0) {
                        s[2] = 'C';
                    }
                    boxfill8(buf_back, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
                    putfonts8_asc(buf_back, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
                    // 鼠标移动
                    mx += mdec.x;
                    my += mdec.y;
                    // 边界检查
                    if (mx < 0) {
                        mx = 0;
                    }
                    if (my < 0) {
                        my = 0;
                    }
                    if (mx > binfo->scrnx - 16) {
                        mx = binfo->scrnx - 16;
                    }
                    if (my > binfo->scrny - 16) {
                        my = binfo->scrny - 16;
                    }
                    sprintf(s, "(%3d, %3d)", mx, my);
                    boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 0, 79, 15);    // 隐藏坐标，即清空之前的坐标显示
                    putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);    // 显示坐标，显示更新后的坐标
                    sheet_slide(shtctl, sht_mouse, mx, my);                         // 函数内部调用sheet_refresh
                }
            }
        }
    }
}
