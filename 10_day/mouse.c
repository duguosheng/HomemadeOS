// 鼠标相关
// 鼠标参考网址https://wiki.osdev.org/PS/2_Mouse

#include "bootpack.h"

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
