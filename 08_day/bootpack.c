#include "bootpack.h"
#include <stdio.h>

struct MOUSE_DEC {  // Mouse Decode
    // 鼠标发来的数据3个字节一组
    unsigned char buf[3];
    // 0: 等待初始化完成（即等待鼠标返回0xfa）
    // 1: 等待接收第1个字节，鼠标发来的数据3字节一组
    // 2: 等待接收第2个字节
    // 3: 等待接收第3个字节
    unsigned char phase;
    int x;      // 鼠标x坐标
    int y;      // 鼠标y坐标
    int btn;    // 鼠标按键状态
};

extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;
void enable_mouse(struct MOUSE_DEC *mdec);
void init_keyboard(void);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);

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

    init_palette();
    init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);
    mx = (binfo->scrnx - 16) / 2;       // mid_x，16是鼠标宽度
    my = (binfo->scrny - 28 - 16) / 2;  // mid_y，16是鼠标宽度，28是底部横条高度
    init_mouse_cursor8(mcursor, COL8_008484);
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
    sprintf(s, "(%d, %d)", mx, my);
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);

    enable_mouse(&mdec);

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

// 鼠标参考网址https://wiki.osdev.org/PS/2_Mouse

#define KEYCMD_SENDTO_MOUSE		0xd4    // 将下一个字节写入PS/2第2端口（即鼠标）输入缓冲区
#define MOUSECMD_ENABLE			0xf4    // 启用数据报告

/**
 * @brief 使能鼠标
 *
 */
void enable_mouse(struct MOUSE_DEC *mdec)
{
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
    // 顺利的话,键盘控制器会返送回ACK(0xfa)
    mdec->phase = 0;
}

/**
 * @brief 解析鼠标返回的数据
 *
 * @param mdec 数据存储位置
 * @param dat 鼠标发送来的数据
 * @return int -1：错误 0：未达到3字节 1：数据装填完成
 */
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat)
{
    if (mdec->phase == 0) {
        // 等待鼠标初始化完成返回0xfa的阶段
        if (dat == 0xfa) {
            mdec->phase = 1;
        }
        return 0;
    } else if (mdec->phase == 1) {
        // 等待鼠标第1字节的阶段
        // 0xc8=11001000
        // 在鼠标返回的第1字节中，0xc8中三个1从左到右的含义为：
        // 第1个1：y坐标溢出；第2个1：x坐标溢出；第3个1：必须永远为1
        if ((dat & 0xc8) == 0x08) {
            // 第1字节正确（无溢出且第3位为1）
            mdec->buf[0] = dat;
            mdec->phase = 2;
        }
        return 0;
    } else if (mdec->phase == 2) {
        // 等待鼠标第2字节的阶段
        mdec->buf[1] = dat;
        mdec->phase = 3;
        return 0;
    } else if (mdec->phase == 3) {
        // 等待鼠标第3字节的阶段
        mdec->buf[2] = dat;
        mdec->phase = 1;
        // 第1字节后3位为按键状态（依次为中键 右键 左键）
        mdec->btn = mdec->buf[0] & 0x07;
        // 第2字节存储x移动坐标（相对位置）
        mdec->x = mdec->buf[1];
        // 第3字节存储y移动坐标（相对位置）
        mdec->y = mdec->buf[2];
        // 第1字节第4位为x符号位
        if ((mdec->buf[0] & 0x10) != 0) {
            // 注意不能mdec->x=-mdec->x，因为第2字节的内容是直接以补码形式存储的（而非绝对值）
            // 例如想要表达左移2，那么buf[0]中xxx1xxxx，buf[1]中11111110
            // 此时需要把高位全部置为1
            mdec->x |= 0xffffff00;
        }
        // 第1字节第5位为y符号位
        if ((mdec->buf[0] & 0x20) != 0) {
            mdec->y |= 0xffffff00;
        }
        mdec->y = -mdec->y;     // 鼠标的y方向与画面符号相反
        return 1;
    }
    return -1;
}
