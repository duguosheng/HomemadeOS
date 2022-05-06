struct BOOTINFO {       // 0x0ff0-0x0fff
    char cyls;          // 启动区读硬盘读到何处为止
    char leds;          // 启动时键盘LED状态
    char vmode;         // 显卡模式彩色位数
    char reserve;
    short scrnx, scrny; // 屏幕分辨率
    char *vram;         // 显存地址
};

#define ADR_BOOTINFO	0x00000ff0

// naskfunc.nas
// 系统进入暂停状态
void io_hlt(void);
// 禁用外部中断                
void io_cli(void);
// 允许外部中断             
void io_sti(void);
// 允许外部中断，然后系统进入暂停状态
void io_stihlt(void);
/**
 * @brief 从外部设备读入8位数据
 *
 * @param port 地址编号
 * @return int 读取到的数值
 */
int  io_in8(int port);
/**
 * @brief 向外部设备写入8位数据
 *
 * @param port 地址编号
 * @param data 待写入数据
 */
void io_out8(int port, int data);
// 获取EFLAGS寄存器状态
int  io_load_eflags(void);
// 设置EFLAGS寄存器状态
void io_store_eflags(int eflags);
/**
 * @brief 设置GDTR寄存器
 *
 * @param limit 地址上界（长度-1）
 * @param addr 基址
 */
void load_gdtr(int limit, int addr);
/**
 * @brief 设置IDTR寄存器
 *
 * @param limit 地址上界（长度-1）
 * @param addr 基址
 */
void load_idtr(int limit, int addr);
// 键盘中断处理函数
void asm_inthandler21(void);
// 鼠标中断处理函数
void asm_inthandler27(void);
// PIC响应中断
void asm_inthandler2c(void);

// graphic.c
void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void init_screen8(char *vram, int x, int y);
void putfont8(char *vram, int xsize, int x, int y, char c, char *font);
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s);
void init_mouse_cursor8(char *mouse, char bc);
void putblock8_8(char *vram, int vxsize, int pxsize,
    int pysize, int px0, int py0, char *buf, int bxsize);

#define COL8_000000     0
#define COL8_FF0000     1
#define COL8_00FF00     2
#define COL8_FFFF00     3
#define COL8_0000FF     4
#define COL8_FF00FF     5
#define COL8_00FFFF     6
#define COL8_FFFFFF     7
#define COL8_C6C6C6     8
#define COL8_840000     9
#define COL8_008400     10
#define COL8_848400     11
#define COL8_000084     12
#define COL8_840084     13
#define COL8_008484     14
#define COL8_848484     15

// dsctbl.c
/**
 * @brief 描述段，64位，包含32位段基址，20位段上限，12位段属性
 *
 * @param base_low/mid/high 将基址拆分为low(2字节) mid(1字节) high(1字节)以兼容80286机器
 * @param limit_low 2字节，存储段上限的低16位
 * @param limit_high 1字节，但被拆分为2个4位，低4位存储段上限的高4位
 *                   高4位存放段属性的高4位，这4位段属性由GD00构成
 *                   G表示G bit G=1:段上限的单位为页(page)，1页=4KB，因此20位x4KB最多表示4G的地址空间
 *                              G=0:段上限的单位字节(Byte)，因此20位最多表示1MB的地址空间
 *                   D表示段的模式 D=0:16位模式 D=1:32位模式
 * @param access_right 段属性的低8位，主要具有以下值
 *                     0x00(未使用) 0x92(系统专用RW) 0x9a(系统专用RX) 0xf2(应用程序用RW) 0xfa(应用程序用RX)
 */
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

#define ADR_IDT			0x0026f800
#define LIMIT_IDT		0x000007ff
#define ADR_GDT			0x00270000
#define LIMIT_GDT		0x0000ffff
#define ADR_BOTPAK		0x00280000
#define LIMIT_BOTPAK	0x0007ffff
#define AR_DATA32_RW	0x4092
#define AR_CODE32_ER	0x409a
#define AR_INTGATE32	0x008e  // 用于中断处理的有效设定

// int.c
struct KEYBUF {
    unsigned char data[32]; // 按键编码
    int next_r;             // 下一个读位置的下标
    int next_w;             // 下一个写位置的下标
    int len;                // FIFO长度
};
void init_pic(void);
void inthandler21(int *esp);
void inthandler27(int *esp);
void inthandler2c(int *esp);
#define PIC0_ICW1		0x0020  // Initial Control Word，初始化控制数据
#define PIC0_OCW2		0x0020
#define PIC0_IMR		0x0021  // Interrupt Mask Register，中断屏蔽寄存器，8位对应8路IRQ，1屏蔽，0使能
#define PIC0_ICW2		0x0021
#define PIC0_ICW3		0x0021
#define PIC0_ICW4		0x0021
#define PIC1_ICW1		0x00a0
#define PIC1_OCW2		0x00a0
#define PIC1_IMR		0x00a1
#define PIC1_ICW2		0x00a1
#define PIC1_ICW3		0x00a1
#define PIC1_ICW4		0x00a1
