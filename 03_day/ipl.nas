; hello-os
; TAB=4

		ORG		0x7c00			; 指明程序的装载地址

; 以下这段是标准FAT32格式软盘专用的代码

		JMP		entry
		DB		0x90
		DB		"HARIBOTE"		; 启动区的名称可以是任意字符串（8字节）
		DW		512				; 每个扇区sector的大小，必须512字节
		DB		1				; 簇cluster的大小，必须为1个扇区
		DW		1				; FAT的起始位置，一般从第一个扇区开始
		DB		2				; FAT个数，必须为2
		DW		224				; 根目录大小，一般设成224项
		DW		2880			; 该磁盘的大小，必须是2880扇区
		DB		0xf0			; 磁盘种类，必须是0xf0
		DW		9				; FAT长度，必须是9扇区
		DW		18				; 1个磁道track有几个扇区，必须为18
		DW		2				; 磁头数，必须为2
		DD		0				; 不使用分区，必须是0
		DD		2880			; 重写一次磁盘大小，2880个扇区
		DB		0,0,0x29		; 意义不明，固定
		DD		0xffffffff		; 卷标号码（可能是）
		DB		"HARIBOTEOS "	; 磁盘名称（11字节）
		DB		"FAT12   "		; 磁盘格式名称（8字节）
		RESB	18				; 先空出18字节

; 程序主体

entry:
		MOV		AX,0			; 初始化寄存器
		MOV		SS,AX
		MOV		SP,0x7c00
		MOV		DS,AX

; 读磁盘

		MOV		AX,0x0820
		MOV		ES,AX			; [ES:BX]指定内存地址
		MOV		CH,0			; 柱面0
		MOV		DH,0			; 磁头0
		MOV		CL,2			; 扇区2

		MOV		AH,0x02			; AH=0x02: 读盘
		MOV		AL,1			; 1个扇区
		MOV		BX,0
		MOV		DL,0x00			; A驱动器
		INT		0x13			; 调用磁盘BIOS
		JC		error			; 读取正常: CF=0, AH=0; 错误: CF=1, 错误代码存入AH

; 虽然读完了，但是因为没有要做的事情所以休眠

fin:
		HLT						; 让CPU停止，等待命令
		JMP		fin				; 无限循环

error:
		MOV		SI,msg
putloop:
		MOV		AL,[SI]
		ADD		SI,1			; 给SI加1
		CMP		AL,0
		JE		fin
		MOV		AH,0x0e			; 显示一个文字
		MOV		BX,15			; 指定字符颜色
		INT		0x10			; 调用显卡BIOS
		JMP		putloop
msg:
		DB		0x0a, 0x0a		; 两个换行
		DB		"load error"
		DB		0x0a			; 换行
		DB		0

		RESB	0x7dfe-$		; 填充0x00，直到0x7dfe

		DB		0x55, 0xaa
