#include "bootpack.h"
#include <stdio.h>

void init_pic(void)
{
    io_out8(PIC0_IMR, 0xff);    // 禁用所有中断
    io_out8(PIC1_IMR, 0xff);    // 禁用所有中断

    io_out8(PIC0_ICW1, 0x11);   // 边沿触发模式
    io_out8(PIC0_ICW2, 0x20);   // IRQ0-7由INT20-27接收
    io_out8(PIC0_ICW3, 1 << 2); // PIC1由IRQ2连接
    io_out8(PIC0_ICW4, 0x01);   // 无缓冲区模式

    io_out8(PIC1_ICW1, 0x11);   // 边沿触发模式
    io_out8(PIC1_ICW2, 0x28);   // IRQ8-15由INT28-2f接收
    io_out8(PIC1_ICW3, 2);      // PIC1由IRQ2连接
    io_out8(PIC1_ICW4, 0x01);   // 无缓冲区模式

    io_out8(PIC0_IMR, 0xfb);    // 11111011 禁用PIC1以外的所有中断
    io_out8(PIC1_IMR, 0xff);    // 禁用所有中断
}

#define PORT_KEYDAT     0x0060  // 编号为0x0060的设备是键盘

struct FIFO8 keyfifo;

/**
 * @brief 来自PS/2键盘的中断(IRQ1, INT 0x21)
 *
 * @param esp
 */
void inthandler21(int *esp)
{
    unsigned char data;
    // 通知PIC: 收到IRQ1的中断(IRQn对应0x60+n)
    // 执行这句话之后，PIC继续时刻监视IRQ1中断是否发生。
    // 如果忘记了执行这句话，PIC就不再监视IRQ1中断，不管下次由键盘输入什么信息，系统都感知不到了
    io_out8(PIC0_OCW2, 0x61);
    // 从键盘处获取8位的按键编码
    data = io_in8(PORT_KEYDAT);
    fifo8_put(&keyfifo, data);
}

struct FIFO8 mousefifo;

/**
 * @brief 来自PS/2鼠标的中断(IRQ12, INT 0x2c)
 *
 * @param esp
 */
void inthandler2c(int *esp)
{
    unsigned char data;
    io_out8(PIC1_OCW2, 0x64);   // 通知PIC1（从PIC）收到IRQ12
    io_out8(PIC0_OCW2, 0x62);   // 通知PIC0（主PIC）收到IRQ2，从PIC通过主PIC的IRQ2进行级联
    data = io_in8(PORT_KEYDAT);
    fifo8_put(&mousefifo, data);
}

/**
 * @brief 处理IRQ7中断
 *        对于部分机器，随着PIC初始化会产生一次IRQ7中断
 *        必须对该中断处理程序执行STI（设置中断标志位），否则系统启动会失败
 *
 * @param esp
 */
void inthandler27(int *esp)
{
    io_out8(PIC0_OCW2, 0x67); /* 通知PIC：IRQ7处理完成，参见7-1节 */
}
