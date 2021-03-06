#include "bootpack.h"

/**
 * @brief 初始化图层管理
 *
 * @param memman 内存管理结构体
 * @param vram 显存地址
 * @param xsize 图像宽度（x方向）
 * @param ysize 图像高度（y方向）
 * @return struct SHTCTL* 初始化的图层管理结构体
 */
struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize)
{
    struct SHTCTL *ctl;
    int i;
    ctl = (struct SHTCTL *)memman_alloc_4k(memman, sizeof(struct SHTCTL));
    if (ctl == 0) {
        goto err;
    }
    ctl->vram = vram;
    ctl->xsize = xsize;
    ctl->ysize = ysize;
    ctl->top = -1;  // 暂无图层
    for (i = 0; i < MAX_SHEETS; ++i) {
        ctl->sheets0[i].flags = 0;  // 标记为未使用
    }

err:
    return ctl;
}

#define SHEET_USE       1

/**
 * @brief 请求分配一个图层
 *
 * @param ctl 图层管理结构体
 * @return struct SHEET* 成功返回分配的图层，失败返回0
 */
struct SHEET *sheet_alloc(struct SHTCTL *ctl)
{
    struct SHEET *sht;
    int i;
    for (i = 0; i < MAX_SHEETS; ++i) {
        if (ctl->sheets0[i].flags == 0) {
            sht = &ctl->sheets0[i];
            sht->flags = SHEET_USE; // 标记为正在使用
            sht->height = -1;       // 隐藏
            return sht;
        }
    }
    return 0;   // 所有的SHEET都已被使用
}

/**
 * @brief 设置图层缓存区
 *
 * @param sht 目标图层
 * @param buf 指定的缓冲区
 * @param xsize 宽度
 * @param ysize 高度
 * @param col_inv 带有透明度的色号，相当于RGBA
 */
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv)
{
    sht->buf = buf;
    sht->bxsize = xsize;
    sht->bysize = ysize;
    sht->col_inv = col_inv;
}


/**
 * @brief 刷新描绘指定坐标范围内的所有图层
 *
 * @param ctl 图层管理结构体
 * @param vx0 左上角的x坐标
 * @param vy0 左上角的y坐标
 * @param vx1 右下角的x坐标
 * @param vy1 右下角的y坐标
 */
void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1)
{
    int h, bx, by, vx, vy, bx0, by0, bx1, by1;
    unsigned char *buf, c, *vram = ctl->vram;
    struct SHEET *sht;
    // 从下往上绘制所有图层，上面的就会覆盖掉下面的，达到层级效果
    for (h = 0; h <= ctl->top; h++) {
        sht = ctl->sheets[h];
        buf = sht->buf;
        // 使用vx0~vy1，对bx0~by1进行倒推
        bx0 = vx0 - sht->vx0;
        by0 = vy0 - sht->vy0;
        bx1 = vx1 - sht->vx0;
        by1 = vy1 - sht->vy0;
        if (bx0 < 0) { bx0 = 0; }
        if (by0 < 0) { by0 = 0; }
        if (bx1 > sht->bxsize) { bx1 = sht->bxsize; }
        if (by1 > sht->bysize) { by1 = sht->bysize; }
        for (by = by0; by < by1; by++) {
            vy = sht->vy0 + by;
            for (bx = bx0; bx < bx1; bx++) {
                vx = sht->vx0 + bx;
                c = buf[by * sht->bxsize + bx];
                // 只有当前的颜色不是透明色号时才进行渲染
                // 例如对于鼠标来说，箭头以外的色号是99，而col_inv也是99
                // 那么就不对其进行渲染，从而不覆盖下面图层的颜色
                if (c != sht->col_inv) {
                    vram[vy * ctl->xsize + vx] = c;
                }
            }
        }
    }
}

/**
 * @brief 设置图层高度
 *
 * @param ctl 图层管理结构体
 * @param sht 指定的图层
 * @param height 想要设定的高度
 */
void sheet_updown(struct SHTCTL *ctl, struct SHEET *sht, int height)
{
    int h;
    int old = sht->height;   // 存储设置前的高度信息

    // 如果指定的高度过高或过低，则进行修正
    if (height > ctl->top + 1) {
        height = ctl->top + 1;
    }
    if (height < -1) {
        height = -1;
    }
    sht->height = height;   // 设定高度

    // 进行sheets的重新排列
    if (old > height) {
        // 比之前低
        if (height >= 0) {
            // sheets后移腾出空间
            for (h = old; h > height; --h) {
                ctl->sheets[h] = ctl->sheets[h - 1];
                ctl->sheets[h]->height = h;
            }
            ctl->sheets[height] = sht;
        } else {
            // 隐藏
            if (ctl->top > old) {
                // 把上面的整体前移
                for (h = old; h < ctl->top; ++h) {
                    ctl->sheets[h] = ctl->sheets[h + 1];
                    ctl->sheets[h]->height = h;
                }
            }
            ctl->top--;
        }
        sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize);
    } else if (old < height) {
        // 比之前高
        if (old >= 0) {
            // sheets后移腾出空间
            for (h = old; h < height; ++h) {
                ctl->sheets[h] = ctl->sheets[h + 1];
                ctl->sheets[h]->height = h;
            }
            ctl->sheets[height] = sht;
        } else {
            // 由隐藏状态转为显示状态
                // 把上面的整体前移
            for (h = ctl->top; h >= height; --h) {
                ctl->sheets[h + 1] = ctl->sheets[h];
                ctl->sheets[h + 1]->height = h + 1;
            }
            ctl->sheets[height] = sht;
            ctl->top++; // 由于显示状态的图层增加了一个，所以最上面的图层高度增加
        }
        sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize);
    }
}

/**
 * @brief 刷新描绘相对位置范围内的所有图层
 *
 * @param ctl 图层管理结构体
 * @param sht 指定的图层（用于相对位置参考）
 * @param bx0 图层内左上角的x坐标
 * @param by0 图层内左上角的y坐标
 * @param bx1 图层内右下角的x坐标
 * @param by1 图层内右下角的y坐标
 */
void sheet_refresh(struct SHTCTL *ctl, struct SHEET *sht, int bx0, int by0, int bx1, int by1)
{
    if (sht->height >= 0) { /* 如果正在显示，则按新图层的信息刷新画面 */
        sheet_refreshsub(ctl, sht->vx0 + bx0, sht->vy0 + by0, sht->vx0 + bx1, sht->vy0 + by1);
    }
}

/**
 * @brief 不改变图层高度，而仅仅是移动图层位置
 *
 * @param ctl 图层管理结构体
 * @param sht 指定图层
 * @param vx0 移动后的x坐标
 * @param vy0 移动后的y坐标
 */
void sheet_slide(struct SHTCTL *ctl, struct SHEET *sht, int vx0, int vy0)
{
    int old_vx0 = sht->vx0;
    int old_vy0 = sht->vy0;
    sht->vx0 = vx0;
    sht->vy0 = vy0;
    if (sht->height >= 0) { /* 如果正在显示，则按新图层的信息刷新画面 */
        // 图层位置改变后需要重绘两个区域
        // 1.图层原来所在区域
        sheet_refreshsub(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize);
        // 2.图层当前所在区域
        sheet_refreshsub(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize);
    }
    return;
}

/**
 * @brief 释放已使用图层的内存
 *
 * @param ctl 图层管理结构体
 * @param sht 待回收的图层
 */
void sheet_free(struct SHTCTL *ctl, struct SHEET *sht)
{
    if (sht->height >= 0) {
        sheet_updown(ctl, sht, -1); /* 如果处于显示状态，则先设定为隐藏 */ 
    }
    sht->flags = 0; /* 标记为"未使用" */
}
