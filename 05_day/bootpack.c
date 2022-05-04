void io_hlt(void);
void io_cli(void);
void io_out8(int port, int data);
int  io_load_eflags(void);

void io_store_eflags(int eflags);
void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void init_screen(char *vram, int x, int y);

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

/**
 * @brief 程序入口点
 *
 */
void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *)0x0ff0;
    static char font_A[16] = {
        0x00, 0x18, 0x18, 0x18, 0x18, 0x24, 0x24, 0x24,
        0x24, 0x7e, 0x42, 0x42, 0x42, 0xe7, 0x00, 0x00
    };

    init_palette();
    init_screen(binfo->vram, binfo->scrnx, binfo->scrny);
    putfonts8_asc(binfo->vram, binfo->scrnx, 10, 10, COL8_FFFFFF, "ABC 123");
    putfonts8_asc(binfo->vram, binfo->scrnx, 31, 31, COL8_000000, "Haribote OS."); // 作为阴影
    putfonts8_asc(binfo->vram, binfo->scrnx, 30, 30, COL8_FFFFFF, "Haribote OS."); // 作为原字

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
void init_screen(char *vram, int x, int y)
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
