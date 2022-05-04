; naskfunc
; TAB=4

[FORMAT "WCOFF"]                ; 制作目标文件的模式
[INSTRSET "i486p"]              ; 表示此程序用于486机器，否则会认为是8086，从而无法识别ECX等寄存器
[BITS 32]                       ; 制作32位模式用的机器语言
[FILE "naskfunc.nas"]           ; 源文件名

; 声明可被链接的函数
        GLOBAL  _io_hlt

; 以下是实际的函数

[SECTION .text]                 ; 目标文件中写了这些之后再写程序

_io_hlt:                        ; void io_hlt(void);
        HLT
        RET
