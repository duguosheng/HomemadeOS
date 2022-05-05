// GTT、IDT等描述表相关

#include "bootpack.h"

void init_gdtidt(void)
{
    // 设置GDT起始地址为0x00270000
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *)ADR_GDT;
    // 设置IDT起始地址为0x0026f800
    struct GATE_DESCRIPTOR *idt = (struct GATE_DESCRIPTOR *)ADR_IDT;
    int i;

    // 初始化GDT，GDT长度为上界+1=65535+1，一个GDT条目为8字节，因此(LIMIT_GDT+1)/8
    for (i = 0; i < (LIMIT_GDT + 1) / 8; ++i) {
        // 指针的加法隐含了乘法运算
        // 将GDT所有条目置为0（上界，基址，权限）
        set_segmdesc(gdt + i, 0, 0, 0);
    }
    // 设置段号1，上界是4GB-1，基址为0，即CPU能管理的全部内存
    set_segmdesc(gdt + 1, 0xffffffff, 0x00000000, AR_DATA32_RW);
    // 设置段号2，上界是512KB-1，基址0x00280000，是为bootpack.hrb准备的
    set_segmdesc(gdt + 2, LIMIT_BOTPAK, ADR_BOTPAK, AR_CODE32_ER);
    // 使用汇编向GDTR写值（因为C语言不能操作GDTR）
    load_gdtr(LIMIT_GDT, ADR_GDT);

    // 初始化IDT
    for (i = 0; i < (LIMIT_IDT + 1) / 8; i++) {
        set_gatedesc(idt + i, 0, 0, 0);
    }
    load_idtr(LIMIT_IDT, ADR_IDT);
}

/**
 * @brief 设置GDT条目参数
 *
 * @param sd GDT条目地址
 * @param limit 段内地址上界（段字节数-1）
 * @param base 基址
 * @param ar 访问权限
 */
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar)
{
    if (limit > 0xfffff) {  // 段上限大于20位能表示的范围
        ar |= 0x8000;       // G_bit=1，即设置段上限单位为页
        limit /= 0x1000;    // 1页=4KB=0x1000B，因此将上限比例缩小
    }
    sd->limit_low = limit & 0xffff;
    sd->limit_high = ((limit >> 16) & 0x0f) | ((ar >> 8) & 0xf0);
    sd->base_low = base & 0xffff;
    sd->base_mid = (base >> 16) & 0xff;
    sd->base_high = (base >> 24) & 0xff;
    sd->access_right = ar & 0xff;
}

/**
 * @brief 设置GDT参数
 *
 * @param gd GDT条目地址
 * @param offset
 * @param selector
 * @param ar
 */
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar)
{
    gd->offset_low = offset & 0xffff;
    gd->selector = selector;
    gd->dw_count = (ar >> 8) & 0xff;
    gd->access_right = ar & 0xff;
    gd->offset_high = (offset >> 16) & 0xffff;
}
