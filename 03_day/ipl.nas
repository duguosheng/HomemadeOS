; hello-os
; TAB=4

CYLS	EQU		10				; 读取的柱面数，EQU声明常数

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

readloop:
		MOV		SI,0			; 记录失败次数

retry:
		MOV		AH,0x02			; AH=0x02: 读盘（软/磁盘->内存）
		MOV		AL,1			; 1个扇区
		MOV		BX,0
		MOV		DL,0x00			; A驱动器
		INT		0x13			; 调用磁盘BIOS
		JNC		next			; 没出错的话跳转到next
		ADD		SI,1			; 错误计数加1
		CMP		SI,5			; 是否达到5次
		JAE		error			; 达到5次输出错误信息
		MOV		AH,0x00			; 以下3行命令复位软盘状态
		MOV		DL,0x00
		INT		0x13
		JMP 	retry			; 再次尝试

next:
		MOV 	AX,ES			; 把内存地址后移0x20
		ADD 	AX,0x20			; 0x20=512/16，由于最终地址=ES*16+BX，因此相当于移动到下一扇区
		MOV 	ES,AX			; 因为没有ADD ES,0x20指令，因此这里绕个弯
		ADD 	CL,1			; CL加1
		CMP 	CL,18			; 比较CL与18
		JBE 	readloop		; CL<=18时循环，BE=below or equal
		MOV 	CL,1			; 从扇区1开始
		ADD 	DH,1			; 磁头号加1
		CMP 	DH,2			; 磁头号和2进行比较
		JB		readloop		; DH<2，跳转到readloop（读反面）
		MOV		DH,0			; 磁头号设为0（读正面）
		ADD		CH,1			; 柱面位置加1
		CMP		CH,CYLS			; 是否读完10个柱面
		JB		readloop		; CH<CYLS，跳转至readloop

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
