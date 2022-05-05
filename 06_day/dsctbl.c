struct SEGMENT_DESCRIPTOR {
    short limit_low, base_low;
    char base_mid, access_right;
    char limit_high, base_high;
};

struct GATE_DESCRIPTOR {
    short offset_low, selector;
    char dw_count, access_right;
    short offset_high;
};

void init_gdtidt(void);
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);

void init_gdtidt(void)
{
    // 设置GDT起始地址为0x00270000
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *)0x00270000;
    // 设置IDT起始地址为0x0026f800
    struct GATE_DESCRIPTOR *idt = (struct GATE_DESCRIPTOR *)0x0026f800;
    int i;

    // 初始化GDT，段号范围为0~8191
    for (i = 0; i < 8192; ++i) {
        // 指针的加法隐含了乘法运算
        // 将GDT所有条目置为0（上界，基址，权限）
        set_segmdesc(gdt + i, 0, 0, 0);
    }
    // 设置段号1，上界是4GB-1，基址为0，即CPU能管理的全部内存
    set_segmdesc(gdt + 1, 0xffffffff, 0x00000000, 0x4092);
    // 设置段号2，上界是512KB-1，基址0x00280000，是为bootpack.hrb准备的
    set_segmdesc(gdt + 2, 0x0007ffff, 0x00280000, 0x409a);
    // 使用汇编向GDTR写值（因为C语言不能操作GDTR）
    load_gdtr(0xffff, 0x00270000);

    // 初始化IDT
    for (i = 0; i < 256; i++) {
        set_gatedesc(idt + i, 0, 0, 0);
    }
    load_idtr(0x7ff, 0x0026f800);
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
    if (limit > 0xfffff) {
        ar |= 0x8000;  // G_bit= 1
        limit /= 0x1000;
    }
    sd->limit_low = limit & 0xffff;
    // todo: 为何不是 (limit >> 16) & 0xff
    sd->limit_high = ((limit >> 16) & 0x0f) | ((ar >> 8) & 0xf0);
    sd->base_low = base & 0xffff;
    // todo: 为何不直接删除base_mid，然后sd->base_high = (base >> 16) & 0xffff;
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
    return;
}
