#include "bootpack.h"
#include <stdio.h>

unsigned int memtest(unsigned int start, unsigned int end);
unsigned int memtest_sub(unsigned int start, unsigned int end);

/**
 * @brief 程序入口点
 *
 */
void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
    char s[40], mcursor[256], keybuf[32], mousebuf[128];
    int mx, my;  // 鼠标坐标
    int i;
    struct MOUSE_DEC mdec;

    init_gdtidt();
    init_pic();
    io_sti();

    fifo8_init(&keyfifo, 32, keybuf);
    fifo8_init(&mousefifo, 128, mousebuf);
    io_out8(PIC0_IMR, 0xf9); // 键盘: IRQ1(INT 0x21)  PIC0允许PIC1和键盘的中断(11111001)
    io_out8(PIC1_IMR, 0xef); // 鼠标: IRQ12(INT 0x2c) PIC1允许鼠标中断(11101111)

    init_keyboard();
    enable_mouse(&mdec);

    init_palette();
    init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);
    mx = (binfo->scrnx - 16) / 2;       // mid_x，16是鼠标宽度
    my = (binfo->scrny - 28 - 16) / 2;  // mid_y，16是鼠标宽度，28是底部横条高度
    init_mouse_cursor8(mcursor, COL8_008484);
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
    sprintf(s, "(%d, %d)", mx, my);
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);

    i = memtest(0x00400000, 0xbfffffff) / (1024 * 1024);
    sprintf(s, "memory %dMB", i);
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 32, COL8_FFFFFF, s);

    for (;;) {
        io_cli();           // 禁用中断
        if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) {
            io_stihlt();    // 启用中断，并使CPU暂停
        } else {
            if (fifo8_status(&keyfifo) != 0) {
                i = fifo8_get(&keyfifo);
                io_sti();       // 启用中断
                sprintf(s, "%02X", i);
                boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
                putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
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
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
                    putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
                    // 鼠标移动
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15); // 隐藏之前的鼠标
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
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 79, 15);     // 隐藏坐标，即清空之前的坐标显示
                    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);     // 显示坐标，显示更新后的坐标
                    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);// 描绘鼠标
                }
            }
        }
    }
}

#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000

/**
 * @brief 禁用486的cache并检查内存字节数，调用memtest_sub实现
 *
 * @param start 起始地址
 * @param end 结束地址
 * @return unsigned int 内存字节数
 */
unsigned int memtest(unsigned int start, unsigned int end)
{
    char flg486 = 0;
    unsigned int eflg, cr0, i;

    // 查看CPU型号，是386还是486以上，386无AC标志位，486在第18位
    eflg = io_load_eflags();
    // 设置EFLAGS的AC为1
    eflg |= EFLAGS_AC_BIT;
    io_store_eflags(eflg);
    eflg = io_load_eflags();
    // 再次检测AC位，如果确实设置成功，则为486机器
    if ((eflg & EFLAGS_AC_BIT) != 0) {
        flg486 = 1;
    }
    eflg &= ~EFLAGS_AC_BIT; // 重新置AC标志为0
    io_store_eflags(eflg);

    // 禁用486中缓存
    if (flg486 != 0) {
        cr0 = load_cr0();
        cr0 |= CR0_CACHE_DISABLE;
        store_cr0(cr0);
    }

    // 实际的容量检查过程
    i = memtest_sub(start, end);

    // 恢复允许486机器使用cache
    if (flg486 != 0) {
        cr0 = load_cr0();
        cr0 &= ~CR0_CACHE_DISABLE;
        store_cr0(cr0);
    }

    return i;
}

/**
 * @brief 检查内存字节数
 *
 * @param start 起始地址
 * @param end 结束地址
 * @return unsigned int 内存字节数
 */
unsigned int memtest_sub(unsigned int start, unsigned int end)
{
    unsigned int i, *p, old, pat0 = 0xaa55aa55, pat1 = 0x55aa55aa;
    for (i = start; i <= end; i += 0x1000) {
        p = (unsigned int *)(i + 0xffc);    // 检查末尾的4字节
        old = *p;			// 先记住修改前的值
        *p = pat0;			// 试写
        *p ^= 0xffffffff;	// 反转
        if (*p != pat1) {	// 检查反转结果
        not_memory:
            *p = old;
            break;
        }
        *p ^= 0xffffffff;	// 再次反转
        if (*p != pat0) {	// 检查是否恢复
            goto not_memory;
        }
        *p = old;			// 恢复为修改前的值
    }
    return i;
}
