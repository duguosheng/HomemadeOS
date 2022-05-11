// 内存管理

#include "bootpack.h"

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
 * @return int 成功返回0，失败返回-1
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

/**
 * @brief 以4KB为基本单位申请内存
 *
 * @param man 内存管理结构体
 * @param size 申请分配的大小
 * @return unsigned int 分配的起始地址，失败返回0
 */
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size)
{
    unsigned int a;
    size = (size + 0xfff) & 0xfffff000;
    a = memman_alloc(man, size);
    return a;
}

/**
 * @brief 以4KB为基本单位释放内存空间
 *
 * @param man 内存管理结构体
 * @param addr 起始地址
 * @param size 字节数
 * @return int 成功返回0，失败返回-1
 */
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
    int i;
    size = (size + 0xfff) & 0xfffff000;
    i = memman_free(man, addr, size);
    return i;
}
