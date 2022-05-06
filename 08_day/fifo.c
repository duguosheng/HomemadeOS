#include "bootpack.h"

#define FLAGS_OVERRUN       0x0001      // 溢出标志位

/**
 * @brief 初始化FIFO
 *
 * @param fifo 待初始化的结构体
 * @param size 缓冲区大小
 * @param buf 缓冲区地址
 */
void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf)
{
    fifo->size = size;
    fifo->buf = buf;
    fifo->free = size;
    fifo->flags = 0;
    fifo->p = 0;
    fifo->q = 0;
}

/**
 * @brief 向FIFO中写入一条数据
 *
 * @param fifo 指定的FIFO
 * @param data 待写入的数据
 * @return int -1:失败 0:成功
 */
int fifo8_put(struct FIFO8 *fifo, unsigned char data)
{
    if (fifo->free == 0) {  // 没有剩余空间
        fifo->flags |= FLAGS_OVERRUN;
        return -1;
    }
    fifo->buf[fifo->p] = data;
    fifo->p++;
    if (fifo->p == fifo->size) {
        fifo->p = 0;
    }
    fifo->free--;
    return 0;
}
/**
 * @brief 从FIFO中读出一条数据
 *
 * @param fifo 指定的FIFO
 * @return int -1:读取失败 其他:读出的数据值
 */
int fifo8_get(struct FIFO8 *fifo)
{
    int data;
    if (fifo->size == fifo->free) {  // 没有剩余空间
        return -1;
    }
    data = fifo->buf[fifo->q];
    fifo->q++;
    if (fifo->q == fifo->size) {
        fifo->q = 0;
    }
    fifo->free++;
    return data;
}

/**
 * @brief 获取当前存入的数据条数
 *
 * @param fifo 指定的FIFO
 * @return int FIFO中的数据数量
 */
int fifo8_status(struct FIFO8 *fifo)
{
    return fifo->size - fifo->free;
}
