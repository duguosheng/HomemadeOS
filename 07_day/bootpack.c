#include "bootpack.h"
#include <stdio.h>

extern struct FIFO8 keyfifo;
void enable_mouse(void);
void init_keyboard(void);

/**
 * @brief 程序入口点
 *
 */
void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
    char s[40], mcursor[256], keybuf[32];
    int mx, my, i, j;

    init_gdtidt();
    init_pic();
    io_sti();

    fifo8_init(&keyfifo, 32, keybuf);
    io_out8(PIC0_IMR, 0xf9); // 键盘: IRQ1(INT 0x21)  PIC0允许PIC1和键盘的中断(11111001)
    io_out8(PIC1_IMR, 0xef); // 鼠标: IRQ12(INT 0x2c) PIC1允许鼠标中断(11101111)

    init_keyboard();

    init_palette();
    init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);
    mx = (binfo->scrnx - 16) / 2;       // mid_x，16是鼠标宽度
    my = (binfo->scrny - 28 - 16) / 2;  // mid_y，
    init_mouse_cursor8(mcursor, COL8_008484);
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
    sprintf(s, "(%d, %d)", mx, my);
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);

    enable_mouse();

    for (;;) {
        io_cli();           // 禁用中断
        if (fifo8_status(&keyfifo) == 0) {
            io_stihlt();    // 启用中断，并使CPU暂停
        } else {
            i = fifo8_get(&keyfifo);
            io_sti();       // 启用中断
            sprintf(s, "%02X", i);
            boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
            putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
        }
    }
}

// PS/2设备参考网址https://wiki.osdev.org/%228042%22_PS/2_Controller
// PS/2的连接器1控制键盘，连接器2控制鼠标

#define PORT_KEYDAT             0x0060  // Data，用于与PS/2设备进行数据交互(读/写)
#define PORT_KEYSTA             0x0064  // Status，0x64用作读操作时是状态寄存器
#define PORT_KEYCMD             0x0064  // Command，0x64用作写操作时命令寄存器
#define KEYSTA_SEND_NOTREADY    0x02    // 用于检测KCB是否准备好接收指令
#define KEYCMD_WRITE_MODE       0x60    // 将下一个字节写入内部RAM的“字节0”（即控制器配置字节）
#define KBC_MODE                0x47

/**
 * @brief 让键盘控制电路(KBC, keyboard controller)做好准备动作
 *        这是因为CPU速度远高于外设速度，因此CPU发送指令前必须确定外设已经处理完成上一条指令，
 *        并且准备好接收下一条指令了
 */
void wait_KBC_sendready(void)
{
    for (;;) {
        // 设备准备好接收指令：从0x64处读取的8位数据的第2位为0
        if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
            break;
        }
    }
}

/**
 * @brief 初始化键盘控制电路
 *
 */
void init_keyboard(void)
{
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, KBC_MODE);
}

#define KEYCMD_SENDTO_MOUSE		0xd4        // 将下一个字节写入PS/2第2端口（即鼠标）输入缓冲区
#define MOUSECMD_ENABLE			0xf4

/**
 * @brief 使能鼠标
 *
 */
void enable_mouse(void)
{
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
    // 顺利的话,键盘控制其会返送回ACK(0xfa)
}
