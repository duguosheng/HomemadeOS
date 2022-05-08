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
 * @param port IO端口编号
 * @return int 读取到的数值
 */
int  io_in8(int port);
/**
 * @brief 向外部设备写入8位数据
 *
 * @param port IO端口编号
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
// 加载CR0寄存器
int load_cr0(void);
// 设置CR0寄存器
void store_cr0(int cr0);
// 键盘中断处理函数
void asm_inthandler21(void);
// 鼠标中断处理函数
void asm_inthandler27(void);
// PIC响应中断
void asm_inthandler2c(void);
/**
 * @brief 检查内存字节数
 *
 * @param start 起始地址
 * @param end 结束地址
 * @return unsigned int 内存字节数
 */
unsigned int memtest_sub(unsigned int start, unsigned int end);

/* fifo.c */
struct FIFO8 {
    unsigned char *buf; // 缓冲区地址
    int p;              // 下一个写入位置的下标
    int q;              // 下一个读出位置的下标
    int size;           // 缓冲区大小
    int free;           // 缓冲区空闲空间大小
    int flags;          // 标志位
};
void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf);
int  fifo8_put(struct FIFO8 *fifo, unsigned char data);
int  fifo8_get(struct FIFO8 *fifo);
int  fifo8_status(struct FIFO8 *fifo);

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

// keyboard.c
void inthandler21(int *esp);
void wait_KBC_sendready(void);
void init_keyboard(void);
extern struct FIFO8 keyfifo;
#define PORT_KEYDAT     0x0060  // Data，用于与PS/2设备进行数据交互(读/写)
#define PORT_KEYSTA     0x0064  // Status，0x64用作读操作时是状态寄存器
#define PORT_KEYCMD     0x0064  // Command，0x64用作写操作时命令寄存器

// mouse.c
struct MOUSE_DEC {  // Mouse Decode
    // 鼠标发来的数据3个字节一组
    unsigned char buf[3];
    // 0: 等待初始化完成（即等待鼠标返回0xfa）
    // 1: 等待接收第1个字节，鼠标发来的数据3字节一组
    // 2: 等待接收第2个字节
    // 3: 等待接收第3个字节
    unsigned char phase;
    int x;      // 鼠标x坐标
    int y;      // 鼠标y坐标
    int btn;    // 鼠标按键状态
};
void inthandler2c(int *esp);
void enable_mouse(struct MOUSE_DEC *mdec);
int  mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);
extern struct FIFO8 mousefifo;
