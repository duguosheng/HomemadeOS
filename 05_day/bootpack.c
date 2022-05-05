#include <stdio.h>

void io_hlt(void);
void io_cli(void);
void io_out8(int port, int data);
int  io_load_eflags(void);

void io_store_eflags(int eflags);
void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void init_screen8(char *vram, int x, int y);
void init_mouse_cursor8(char *mouse, char bc);
void putblock8_8(char *vram, int vxsize, int pxsize,
    int pysize, int px0, int py0, char *buf, int bxsize);

#define COL8_000000     0
#define COL8_FF0000     1
#define COL8_00FF00     2
#define COL8_FFFF00     3
#define COL8_0000FF     4
#define COL8_FF00FF     5
#define COL8_00FFFF     6
#define COL8_FFFFFF     7
#define COL8_C6C6C6     8
#define COL8_840000     9
#define COL8_008400     10
#define COL8_848400     11
#define COL8_000084     12
#define COL8_840084     13
#define COL8_008484     14
#define COL8_848484     15

struct BOOTINFO {
    char cyls, leds, vmode, reserve;
    short scrnx, scrny;
    char *vram;
};

struct SEGMENT_DESCRIPTOR {
    short limit_low, base_low;
    char base_mid, access_right;
    char limit_high, base_high;
};

struct GATE_DESCRIPTOR {
    short offset_low, selector;
    char dw_count, access_right;
    short offset_high;
};

void init_gdtidt(void);
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);

/**
 * @brief 程序入口点
 *
 */
void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *)0x0ff0;
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

/**
 * @brief 初始化调色板，指定色号
 *
 */
void init_palette(void)
{
    static unsigned char table_rgb[16 * 3] = {
        0x00, 0x00, 0x00,	/*  0:黑 */
        0xff, 0x00, 0x00,	/*  1:亮红 */
        0x00, 0xff, 0x00,	/*  2:亮绿 */
        0xff, 0xff, 0x00,	/*  3:亮黄 */
        0x00, 0x00, 0xff,	/*  4:亮蓝 */
        0xff, 0x00, 0xff,	/*  5:亮紫 */
        0x00, 0xff, 0xff,	/*  6:浅亮蓝 */
        0xff, 0xff, 0xff,	/*  7:白色 */
        0xc6, 0xc6, 0xc6,	/*  8:亮灰 */
        0x84, 0x00, 0x00,	/*  9:暗红 */
        0x00, 0x84, 0x00,	/* 10:暗绿 */
        0x84, 0x84, 0x00,	/* 11:暗黄 */
        0x00, 0x00, 0x84,	/* 12:暗青 */
        0x84, 0x00, 0x84,	/* 13:暗紫 */
        0x00, 0x84, 0x84,	/* 14:浅暗蓝 */
        0x84, 0x84, 0x84	/* 15:暗灰 */
    };
    set_palette(0, 15, table_rgb);
}

/**
 * @brief 调用BIOS设置调色板
 *
 * @param start 起始色号
 * @param end 终止色号
 * @param rgb 色号RGB矩阵
 */
void set_palette(int start, int end, unsigned char *rgb)
{
    int i, eflags;
    eflags = io_load_eflags();  // 记录中断许可标志
    io_cli();                   // 将中断许可标志置为0，禁用中断

    // 写调色板：调色板号码->0x03c8，RGB->0x03c9
    io_out8(0x03c8, start);
    // 设定多个调色板
    for (i = start; i <= end; ++i) {
        io_out8(0x03c9, rgb[0] / 4);
        io_out8(0x03c9, rgb[1] / 4);
        io_out8(0x03c9, rgb[2] / 4);
        rgb += 3;
    }
    io_store_eflags(eflags);    // 恢复中断标志位
}

/**
 * @brief 绘制矩形
 *
 * @param vram 显存起始地址
 * @param xsize 行像素点数
 * @param c 颜色
 * @param x0 横坐标起始位置
 * @param y0 纵坐标起始位置，[x0,y0]构成矩形左上角坐标
 * @param x1 横坐标结束位置
 * @param y1 纵坐标结束位置，[x1,y1]构成矩阵右下角坐标
 */
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1)
{
    int x, y;
    for (y = y0; y <= y1; ++y) {
        for (x = x0; x <= x1; ++x)
            vram[y * xsize + x] = c;
    }
}

/**
 * @brief 初始化屏幕
 *
 * @param vram 显存起始地址
 * @param x 行像素点数
 * @param y 列像素点数
 */
void init_screen8(char *vram, int x, int y)
{
    boxfill8(vram, x, COL8_008484, 0, 0, x - 1, y - 29);
    boxfill8(vram, x, COL8_C6C6C6, 0, y - 28, x - 1, y - 28);
    boxfill8(vram, x, COL8_FFFFFF, 0, y - 27, x - 1, y - 27);
    boxfill8(vram, x, COL8_C6C6C6, 0, y - 26, x - 1, y - 1);

    boxfill8(vram, x, COL8_FFFFFF, 3, y - 24, 59, y - 24);
    boxfill8(vram, x, COL8_FFFFFF, 2, y - 24, 2, y - 4);
    boxfill8(vram, x, COL8_848484, 3, y - 4, 59, y - 4);
    boxfill8(vram, x, COL8_848484, 59, y - 23, 59, y - 5);
    boxfill8(vram, x, COL8_000000, 2, y - 3, 59, y - 3);
    boxfill8(vram, x, COL8_000000, 60, y - 24, 60, y - 3);

    boxfill8(vram, x, COL8_848484, x - 47, y - 24, x - 4, y - 24);
    boxfill8(vram, x, COL8_848484, x - 47, y - 23, x - 47, y - 4);
    boxfill8(vram, x, COL8_FFFFFF, x - 47, y - 3, x - 4, y - 3);
    boxfill8(vram, x, COL8_FFFFFF, x - 3, y - 24, x - 3, y - 3);
    return;
}

/**
 * @brief 绘制一个字符
 *
 * @param vram 显存起始地址
 * @param xsize 行像素点数
 * @param x 起始位置x
 * @param y 起始位置y，[x,y]是字符左上角像素点的坐标
 * @param c 颜色
 * @param font 字符
 */
void putfont8(char *vram, int xsize, int x, int y, char c, char *font)
{
    int i;
    char *p, d;
    for (i = 0; i < 16; ++i) {
        p = vram + (y + i) * xsize + x; // 起始坐标[y+i, x]
        d = font[i];
        if ((d & 0x80)) p[0] = c;
        if ((d & 0x40)) p[1] = c;
        if ((d & 0x20)) p[2] = c;
        if ((d & 0x10)) p[3] = c;
        if ((d & 0x08)) p[4] = c;
        if ((d & 0x04)) p[5] = c;
        if ((d & 0x02)) p[6] = c;
        if ((d & 0x01)) p[7] = c;
    }
}

/**
 * @brief 显示ASCII码字符串
 *
 * @param vram 显存起始地址
 * @param xsize 行像素点数
 * @param x 起始位置x
 * @param y 起始位置y，[x,y]是字符左上角像素点的坐标
 * @param c 颜色
 * @param s 待显示的字符串
 */
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s)
{
    extern char hankaku[4096];
    for (; *s != '\0'; ++s) {
        // hankaku中共有256个字符，每个字符占用16字节
        // 例如访问字符A，则访问地址为 hankaku+'A'*16 开始的16个字节
        putfont8(vram, xsize, x, y, c, hankaku + *s * 16);
        x += 8;
    }
}

/**
 * @brief 初始化鼠标图形数组
 *
 * @param mouse 图像保存地址
 * @param bc 背景色
 */
void init_mouse_cursor8(char *mouse, char bc)
{
    // 鼠标图像(16x16)
    static char cursor[16][16] = {
        "**************..",
        "*OOOOOOOOOOO*...",
        "*OOOOOOOOOO*....",
        "*OOOOOOOOO*.....",
        "*OOOOOOOO*......",
        "*OOOOOOO*.......",
        "*OOOOOOO*.......",
        "*OOOOOOOO*......",
        "*OOOO**OOO*.....",
        "*OOO*..*OOO*....",
        "*OO*....*OOO*...",
        "*O*......*OOO*..",
        "**........*OOO*.",
        "*..........*OOO*",
        "............*OO*",
        ".............***"
    };
    int i, j;
    for (i = 0; i < 16; ++i) {
        for (j = 0; j < 16; ++j) {
            if (cursor[i][j] == '*')
                mouse[i * 16 + j] = COL8_000000;
            if (cursor[i][j] == 'O')
                mouse[i * 16 + j] = COL8_FFFFFF;
            if (cursor[i][j] == '.')
                mouse[i * 16 + j] = bc;
        }
    }
}

/**
 * @brief 绘制矩阵
 *
 * @param vram 显存起始地址
 * @param vxsize 行像素点数
 * @param pxsize 图形矩阵列数（x方向）
 * @param pysize 图形矩阵行数（y方向）
 * @param px0 横坐标起始位置
 * @param py0 纵坐标起始位置，[px0,py0]是左上角像素点的坐标
 * @param buf 图形矩阵地址
 * @param bxsize 图形矩阵每行的像素数
 */
void putblock8_8(char *vram, int vxsize, int pxsize,
    int pysize, int px0, int py0, char *buf, int bxsize)
{
    int i, j;
    for (i = 0; i < pysize; ++i) {
        for (j = 0; j < pxsize; ++j) {
            vram[(py0 + i) * vxsize + (px0 + j)] = buf[i * bxsize + j];
        }
    }
}

void init_gdtidt(void)
{
    // 设置GDT起始地址为0x00270000
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *)0x00270000;
    // 设置IDT起始地址为0x0026f800
    struct GATE_DESCRIPTOR *idt = (struct GATE_DESCRIPTOR *)0x0026f800;
    int i;

    // 初始化GDT，段号范围为0~8191
    for (i = 0; i < 8192; ++i) {
        // 指针的加法隐含了乘法运算
        // 将GDT所有条目置为0（上界，基址，权限）
        set_segmdesc(gdt + i, 0, 0, 0);
    }
    // 设置段号1，上界是4GB-1，基址为0，即CPU能管理的全部内存
    set_segmdesc(gdt + 1, 0xffffffff, 0x00000000, 0x4092);
    // 设置段号2，上界是512KB-1，基址0x00280000，是为bootpack.hrb准备的
    set_segmdesc(gdt + 2, 0x0007ffff, 0x00280000, 0x409a);
    // 使用汇编向GDTR写值（因为C语言不能操作GDTR）
    load_gdtr(0xffff, 0x00270000);

    // 初始化IDT
    for (i = 0; i < 256; i++) {
        set_gatedesc(idt + i, 0, 0, 0);
    }
    load_idtr(0x7ff, 0x0026f800);
}

/**
 * @brief 设置GDT条目参数
 *
 * @param sd GDT条目地址
 * @param limit 段内地址上界（段字节数-1）
 * @param base 基址
 * @param ar 访问权限
 */
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar)
{
    if (limit > 0xfffff) {
        ar |= 0x8000;  // G_bit= 1
        limit /= 0x1000;
    }
    sd->limit_low = limit & 0xffff;
    // todo: 为何不是 (limit >> 16) & 0xff
    sd->limit_high = ((limit >> 16) & 0x0f) | ((ar >> 8) & 0xf0);
    sd->base_low = base & 0xffff;
    // todo: 为何不直接删除base_mid，然后sd->base_high = (base >> 16) & 0xffff;
    sd->base_mid = (base >> 16) & 0xff;
    sd->base_high = (base >> 24) & 0xff;
    sd->access_right = ar & 0xff;
}

/**
 * @brief 设置GDT参数
 *
 * @param gd GDT条目地址
 * @param offset
 * @param selector
 * @param ar
 */
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar)
{
    gd->offset_low = offset & 0xffff;
    gd->selector = selector;
    gd->dw_count = (ar >> 8) & 0xff;
    gd->access_right = ar & 0xff;
    gd->offset_high = (offset >> 16) & 0xffff;
    return;
}
