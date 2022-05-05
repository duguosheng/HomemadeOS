; naskfunc
; TAB=4

[FORMAT "WCOFF"]                ; 制作目标文件的模式
[INSTRSET "i486p"]              ; 表示此程序用于486机器，否则会认为是8086，从而无法识别ECX等寄存器
[BITS 32]                       ; 制作32位模式用的机器语言
[FILE "naskfunc.nas"]           ; 源文件名

; 声明可被链接的函数
        GLOBAL  _io_hlt, _io_cli, _io_sti, _io_stihlt
        GLOBAL  _io_in8, _io_in16, _io_in32
        GLOBAL  _io_out8, _io_out16, _io_out32
        GLOBAL  _io_load_eflags, _io_store_eflags
        GLOBAL  _load_gdtr, _load_idtr
        GLOBAL  _asm_inthandler21, _asm_inthandler27, _asm_inthandler2c
; 声明来自于外部的函数
        EXTERN  _inthandler21, _inthandler27, _inthandler2c

; 以下是实际的函数

[SECTION .text]                 ; 目标文件中写了这些之后再写程序

_io_hlt:        ; void io_hlt(void);
        HLT
        RET

_io_cli:        ; void io_cli(void)
        CLI
        RET

 _io_sti:       ; void io_sti(void);
        STI
        RET

_io_stihlt:     ; void io_stihlt(void);
        STI
        HLT
        RET

_io_in8:        ; int io_in8(int port);
        MOV     EDX,[ESP+4]     ; port
        MOV     EAX,0
        IN      AL,DX
        RET

_io_in16:       ; int io_in16(int port);
        MOV     EDX,[ESP+4]     ; port
        MOV     EAX,0
        IN      AX,DX
        RET

_io_in32:       ; int io_in32(int port);
        MOV     EDX,[ESP+4]     ; port
        IN      EAX,DX
        RET

_io_out8:       ; void io_out8(int port, int data);
        MOV     EDX,[ESP+4]     ; port
        MOV     AL,[ESP+8]      ; data
        OUT     DX,AL
        RET

_io_out16:      ; void io_out16(int port, int data);
        MOV     EDX,[ESP+4]     ; port
        MOV     EAX,[ESP+8]     ; data
        OUT     DX,AX
        RET

_io_out32:      ; void io_out32(int port, int data);
        MOV     EDX,[ESP+4]     ; port
        MOV     EAX,[ESP+8]     ; data
        OUT     DX,EAX
        RET

_io_load_eflags:        ; int io_load_eflags(void);
        PUSHFD          ; 指PUSH EFLAGS
        POP     EAX     ; 将EFLAGS存入EAX，同时EAX也是返回值
        RET

_io_store_eflags:       ; void io_store_eflags(int eflags);
        MOV     EAX,[ESP+4]
        PUSH    EAX
        POPFD           ; 指POP EFLAGS
        RET

_load_gdtr:             ; void load_gdtr(int limit, int addr);
        MOV     AX,[ESP+4]      ; limit
        MOV     [ESP+6],AX
        LGDT    [ESP+6]
        RET

_load_idtr:             ; void load_idtr(int limit, int addr);
        MOV     AX,[ESP+4]      ; limit
        MOV     [ESP+6],AX
        LIDT    [ESP+6]
        RET

_asm_inthandler21:
        PUSH    ES              ; 将寄存器的值保存到栈中
        PUSH    DS
        PUSHAD
        MOV     EAX,ESP
        PUSH    EAX
        MOV     AX,SS           ; 调整使得DS=SS, ES=SS，C语言认为这三个寄存器指向同一段，因此需要相等
        MOV     DS,AX
        MOV     ES,AX
        CALL    _inthandler21   ; 调用_inthandler21
        POP     EAX             ; 恢复寄存器
        POPAD
        POP     DS
        POP     ES
        IRETD                   ; Interrupt Return DWord，中断返回双字，用于32位模式下中断返回

_asm_inthandler27:
        PUSH    ES
        PUSH    DS
        PUSHAD
        MOV     EAX,ESP
        PUSH    EAX
        MOV     AX,SS
        MOV     DS,AX
        MOV     ES,AX
        CALL    _inthandler27
        POP     EAX
        POPAD
        POP     DS
        POP     ES
        IRETD

_asm_inthandler2c:
        PUSH    ES
        PUSH    DS
        PUSHAD
        MOV     EAX,ESP
        PUSH    EAX
        MOV     AX,SS
        MOV     DS,AX
        MOV     ES,AX
        CALL    _inthandler2c
        POP     EAX
        POPAD
        POP     DS
        POP     ES
        IRETD
