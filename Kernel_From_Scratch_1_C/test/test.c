// test.c
void _start() {
    int a = 1 + 2;
    int b = a * 3;
    (void)b;  // 사용하지 않는 변수 경고 억제

    while (1);
}
