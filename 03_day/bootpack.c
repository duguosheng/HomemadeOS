// 声明一个函数
void io_hlt(void);

void HariMain(void)
{
fin:
    io_hlt();   // 执行naskfunc.nas中的_io_hlt
    goto fin;
}