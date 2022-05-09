#include "bootpack.h"
#include <stdio.h>

#define MEMMAN_FREES		4090	/* 大约32KB */
#define MEMMAN_ADDR			0x003c0000

struct FREEINFO {	/* 可用内存信息 */
    unsigned int addr;  // 起始地址
    unsigned int size;  // 大小
};

struct MEMMAN {		/* Memory Manager, 内存管理 */
    int frees;      // 空闲块的数量
    int maxfrees;   // 在分配和释放过程中，frees出现的最大值
    int lostsize;   // 释放失败的内存大小总和
    int losts;      // 释放失败的次数
    struct FREEINFO free[MEMMAN_FREES]; // 空闲块数组
};

unsigned int memtest(unsigned int start, unsigned int end);
void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);

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
    unsigned int memtotal;
    struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;   // 将内存管理结构体放在MEMMAN_ADDR处（高地址）

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

    memtotal = memtest(0x00400000, 0xbfffffff);
    memman_init(memman);
    memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff */
    memman_free(memman, 0x00400000, memtotal - 0x00400000);

    sprintf(s, "memory %dMB   free : %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
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
 * @brief 初始化MEMMAN结构体
 *
 * @param man 待初始化的结构体
 */
void memman_init(struct MEMMAN *man)
{
    man->frees = 0;			/* あき情報の個数 */
    man->maxfrees = 0;		/* 状況観察用：freesの最大値 */
    man->lostsize = 0;		/* 解放に失敗した合計サイズ */
    man->losts = 0;			/* 解放に失敗した回数 */
    return;
}

/**
 * @brief 报告总空闲内存大小
 *
 * @param man 内存管理结构体
 * @return unsigned int 空闲内存字节数
 */
unsigned int memman_total(struct MEMMAN *man)
{
    unsigned int i, t = 0;
    for (i = 0; i < man->frees; i++) {
        t += man->free[i].size;
    }
    return t;
}

/**
 * @brief 分配内存空间
 *
 * @param man 内存管理结构体
 * @param size 分配字节数
 * @return unsigned int 分配的起始地址，失败返回0
 */
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size)
{
    unsigned int i, a;
    for (i = 0; i < man->frees; i++) {
        if (man->free[i].size >= size) {
            // 找到了足够大的内存
            a = man->free[i].addr;
            man->free[i].addr += size;
            man->free[i].size -= size;
            if (man->free[i].size == 0) {
                // 如果free[i]的内存全部分配完毕，就移除一条可用信息
                man->frees--;
                for (; i < man->frees; i++) {
                    man->free[i] = man->free[i + 1]; // 将free[i]之后的所有数据前移
                }
            }
            return a;
        }
    }
    return 0; // 没有可用空间
}

/**
 * @brief 释放内存空间
 *
 * @param man 内存管理结构体
 * @param addr 起始地址
 * @param size 字节数
 * @return int
 */
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
    int i, j;
    // 为便于归纳内存，将free[]按照addr（起始地址大小）的顺序排列
    // 所以，先决定应该放在哪里
    for (i = 0; i < man->frees; i++) {
        if (man->free[i].addr > addr) {
            break;
        }
    }
    /* free[i - 1].addr < addr < free[i].addr */
    if (i > 0) {
        // 前面有空闲块
        if (man->free[i - 1].addr + man->free[i - 1].size == addr) {
            // 可以与前面的内存归并到一起
            man->free[i - 1].size += size;
            if (i < man->frees) {
                // 后面也有空闲块
                if (addr + size == man->free[i].addr) {
                    // 可以与后面的内存归并到一起
                    man->free[i - 1].size += man->free[i].size;
                    // 删除man->free[i]
                    // 将man->free[i-1]，当前要释放区域，man->free[i]合并到一起
                    man->frees--;
                    for (; i < man->frees; i++) {
                        man->free[i] = man->free[i + 1]; // 将free[i]之后的所有数据前移
                    }
                }
            }
            return 0; // 结束
        }
    }
    // 无法与前面的可用空间归纳到一起
    if (i < man->frees) {
        // 后面有空闲块
        if (addr + size == man->free[i].addr) {
            // 可以与后面的内存归并到一起
            man->free[i].addr = addr;
            man->free[i].size += size;
            return 0; // 结束
        }
    }
    // 既不能合并到前面，也不能合并到后面
    if (man->frees < MEMMAN_FREES) {
        // free[i]之后的向后移动，腾出可用空间
        for (j = man->frees; j > i; j--) {
            man->free[j] = man->free[j - 1];
        }
        man->frees++;
        if (man->maxfrees < man->frees) {
            man->maxfrees = man->frees; // 更新最大值
        }
        man->free[i].addr = addr;
        man->free[i].size = size;
        return 0; // 结束
    }
    // free[]空间不足
    man->losts++;
    man->lostsize += size;
    return -1; // 释放失败
}
