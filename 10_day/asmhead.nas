; haribote-os boot asm
; TAB=4

; 内存分布如下
; 0x00000000 - 0x000fffff : 启动中会多次使用，之后就变空（但仍然保存着BIOS,VRAM等内容）。（1MB）
; 0x00100000 - 0x00267fff : 用于保存软盘的内容。（1440KB）
; 0x00268000 - 0x0026f7ff : 空（30KB）
; 0x0026f800 - 0x0026ffff : IDT （2KB）
; 0x00270000 - 0x0027ffff : GDT （64KB）
; 0x00280000 - 0x002fffff : bootpack.hrb（512KB）
; 0x00300000 - 0x003fffff : 栈及其他（1MB）
; 0x00400000 - : 空

BOTPAK	EQU		0x00280000		; bootpack装载地址
DSKCAC	EQU		0x00100000		; 用于保存软盘的内容
DSKCAC0	EQU		0x00008000		; 用于保存软盘的内容（实模式下）

; BOOT_INFO相关（将启动信息存入内存中留作以后备用）
CYLS	EQU     0x0ff0			; 设定启动区
LEDS	EQU		0x0ff1
VMODE	EQU		0x0ff2			; 关于颜色数目的信息，颜色的位数
SCRNX	EQU		0x0ff4			; 分辨率的X（screen x）
SCRNY	EQU		0x0ff6			; 分辨率的Y（screen y）
VRAM	EQU		0x0ff8			; 图像缓冲区的开始地址

		ORG		0xc200			; 这个程序要读取到哪里

; 画面设定
		MOV		AL,0x13			; VGA显卡，320x200x8位颜色
		MOV		AH,0x00
		INT		0x10

; 记录画面模式到内存中
		MOV		BYTE [VMODE],8
		MOV		WORD [SCRNX],320
		MOV		WORD [SCRNY],200
		MOV		DWORD [VRAM],0x000a0000

; 用BIOS取得键盘上各种LED指示灯的状态，存入内存
		MOV		AH,0x02
		INT		0x16			; 键盘BIOS
		MOV		[LEDS],AL

; 确保PIC初始化前不处理任何中断
; 	根据AT兼容机的规格，如果要初始化PIC，
; 	必须在CLI之前禁用PIC中断，否则有时会挂起
;   然后再进行PIC初始化

		MOV		AL,0xff
		OUT		0x21,AL			; 禁用主PIC全部中断
		NOP						; 如果连续执行OUT指令，有些机型会产生错误
		OUT		0xa1,AL			; 禁用从PIC全部中断

		CLI						; 禁止CPU级别的中断

; 为了能让CPU能够访问1MB以上的内存空间，设定A20GATE

		CALL	waitkbdout
		MOV		AL,0xd1
		OUT		0x64,AL
		CALL	waitkbdout
		MOV		AL,0xdf			; enable A20
		OUT		0x60,AL
		CALL	waitkbdout

; 切换到保护模式

[INSTRSET "i486p"]				; 使用486指令集

		LGDT	[GDTR0]			; 设定临时GDT
		MOV		EAX,CR0			; CR0=Control Register 0，一个重要的控制寄存器
		AND		EAX,0x7fffffff	; 设置第31位为0（禁止分页）
		OR		EAX,0x00000001	; 设置第0位为1（切换到保护模式）
		MOV		CR0,EAX
		JMP		pipelineflush	; 在切换到保护模式后，机器语言的解释发生改变，需要调用JMP以重新解释指令
								; 否则由于流水线的原因，还在执行前一条指令时后面指令已经被取出，就会导致出错
pipelineflush:
		MOV		AX,1*8			;  将除了CS的段寄存器都改为0x8（即gdt+1，表示gdt后面的段）
		MOV		DS,AX
		MOV		ES,AX
		MOV		FS,AX
		MOV		GS,AX
		MOV		SS,AX

; bootpack的转送，memcpy(bootpack, BOTPAK, 512*1024/4);
; 从bootpack的地址开始的512KB内容复制到0x00280000号地址去（为了足够容纳bootpack，使用了512KB）

		MOV		ESI,bootpack	; 转送源
		MOV		EDI,BOTPAK		; 转送目的地
		MOV		ECX,512*1024/4	; memcpy拷贝的单位是双字，因此要除以4
		CALL	memcpy

; 磁盘数据转送到它本来的位置去

; 首先从启动扇区开始，memcpy(0x7c00, DSKCAC, 512/4);

		MOV		ESI,0x7c00		; 转送源
		MOV		EDI,DSKCAC		; 转送目的地
		MOV		ECX,512/4		; 启动扇区的512B复制到内存地址的1M位置
		CALL	memcpy

; 所有剩下的，memcpy(DSKCAC0+512, DSKCAC+512, cyls * 512*18*2/4 - 512/4);
; 将始于0x00008200的磁盘内容，复制到0x00100200那里。

		MOV		ESI,DSKCAC0+512	; 转送源
		MOV		EDI,DSKCAC+512	; 转送目的地
		MOV		ECX,0
		MOV		CL,BYTE [CYLS]
		IMUL	ECX,512*18*2/4	; 从柱面数变换为字节数/4
		SUB		ECX,512/4		; 减去 IPL 
		CALL	memcpy

; 必须由asmhead来完成的工作，至此全部完毕
; 	以后就交由bootpack来完成

; bootpack的启动
; 会将bootpack.hrb第0x10c8字节开始的0x11a8字节复制到0x00310000号地址去。
		MOV		EBX,BOTPAK
		MOV		ECX,[EBX+16]
		ADD		ECX,3			; ECX += 3;
		SHR		ECX,2			; ECX /= 4;
		JZ		skip			; 没有要转送的东西时
		MOV		ESI,[EBX+20]	; 转送源
		ADD		ESI,EBX
		MOV		EDI,[EBX+12]	; 转送目的地
		CALL	memcpy
skip:
		MOV		ESP,[EBX+12]	; 栈初始值
		JMP		DWORD 2*8:0x0000001b	; gdt+2:0x1b，即bootpack的0x1b号地址

; 等待外设做好接收指令的准备
waitkbdout:
		IN		 AL,0x64
		AND		 AL,0x02
		JNZ		waitkbdout		; AND的结果如果不是0，就跳到waitkbdout继续循环
		RET

memcpy:
		MOV		EAX,[ESI]
		ADD		ESI,4
		MOV		[EDI],EAX
		ADD		EDI,4
		SUB		ECX,1
		JNZ		memcpy			; 减法运算的结果如果不是0，就跳转到memcpy继续拷贝
		RET
; memcpy也可以用串指令来写，只要不忘记加入地址大小前缀

		ALIGNB	16				; 16字节对齐
GDT0:							; GDT0也是一种特定的GDT
		RESB	8				; NULL selector
		DW		0xffff,0x0000,0x9200,0x00cf	; 可以读写的段（segment）32bit
		DW		0xffff,0x0000,0x9a28,0x0047	; 可以执行的段（segment）32bit（bootpack用）

		DW		0
GDTR0:
		DW		8*3-1
		DD		GDT0

		ALIGNB	16
bootpack:
