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
