// 声明函数
void io_hlt(void);

void HariMain(void)
{
    int i;
    char *p;
    for (i = 0xa0000; i < 0xaffff; ++i) {
        p = (char *)i;
        *p = i & 0x0f;  // 替换write_mem8(i, i & 0x0f)
    }
    for (;;) {
        io_hlt();
    }
}
