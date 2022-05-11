// 键盘相关
// PS/2设备参考网址https://wiki.osdev.org/%228042%22_PS/2_Controller
// PS/2的连接器1控制键盘，连接器2控制鼠标

#include "bootpack.h"

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
