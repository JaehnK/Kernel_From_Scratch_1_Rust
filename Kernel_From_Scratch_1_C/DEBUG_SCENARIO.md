# KFS C 커널 디버깅 시나리오

이 문서는 `make debug`와 `make gdb-rich`를 이용해서 커널 부팅 흐름을 직접 따라가는 실습 시나리오입니다.

목표는 다음 흐름을 눈으로 확인하는 것입니다.

```text
GRUB -> _start -> stack 설정 -> kernel_main 호출 -> VGA 메모리에 42 출력 -> hlt 루프
```

## 1. QEMU를 디버그 모드로 실행

터미널 1에서 실행합니다.

```sh
make debug
```

이 명령은 QEMU를 실행하지만 CPU를 바로 시작하지 않습니다.

중요한 옵션은 다음입니다.

```text
-S              CPU를 정지 상태로 시작
-gdb tcp::1234  GDB가 붙을 수 있는 포트 열기
```

이 터미널은 그대로 둡니다.

## 2. GDB 접속

터미널 2에서 실행합니다.

```sh
make gdb-rich
```

`gdb-rich`는 일반 GDB에 커널 디버깅용 표시 기능을 추가한 모드입니다.

정지할 때마다 자동으로 다음 정보를 보여줍니다.

```text
registers
code
stack
vga text buffer
```

## 3. 커널 진입점 `_start`에서 멈추기

GDB 안에서 실행합니다.

```gdb
boot
```

이 명령은 내부적으로 다음과 같습니다.

```gdb
break _start
continue
```

확인할 것:

```gdb
info registers eip esp
x/10i $eip
```

관찰 포인트:

```text
eip = 현재 실행 중인 명령어 주소
esp = 현재 스택 포인터
현재 위치는 src/boot.s의 _start 근처
```

## 4. 스택 설정 확인

현재 `_start`의 첫 명령은 보통 다음과 같습니다.

```asm
mov $stack_top, %esp
```

한 명령 실행합니다.

```gdb
si
```

스택 포인터를 확인합니다.

```gdb
info registers esp
```

`esp`가 `stack_top` 근처 주소로 바뀌었다면 부트 코드가 커널용 스택을 잡은 것입니다.

## 5. C 커널 함수로 진입

현재 위치 주변 명령을 봅니다.

```gdb
x/5i $eip
```

다음 명령이 보일 것입니다.

```asm
call kernel_main
```

한 명령 실행합니다.

```gdb
si
```

이제 C 함수 `kernel_main` 안으로 들어갑니다.

만약 이미 지나쳤다면 다음 명령으로 다시 잡을 수 있습니다.

```gdb
main
```

또는 직접 입력해도 됩니다.

```gdb
break kernel_main
continue
```

## 6. VGA 메모리 쓰기 전 상태 확인

VGA 텍스트 버퍼 앞 4바이트를 확인합니다.

```gdb
vga
```

직접 명령으로는 다음과 같습니다.

```gdb
x/4xb 0xb8000
```

VGA 텍스트 모드에서 각 문자는 2바이트입니다.

```text
문자 1바이트 + 색상 1바이트
```

## 7. C 코드 한 줄씩 실행하면서 VGA 확인

C 코드 한 줄을 실행합니다.

```gdb
next
```

그 다음 VGA 메모리를 확인합니다.

```gdb
vga
```

이 과정을 반복합니다.

```gdb
next
vga
next
vga
next
vga
next
vga
```

기대 흐름:

```text
VGA_BUFFER[0] = '4'   -> 0xb8000에 0x34
VGA_BUFFER[1] = 0x0f  -> 0xb8001에 0x0f
VGA_BUFFER[2] = '2'   -> 0xb8002에 0x32
VGA_BUFFER[3] = 0x0f  -> 0xb8003에 0x0f
```

최종 확인:

```gdb
x/4xb 0xb8000
```

기대 결과:

```text
0x34 0x0f 0x32 0x0f
```

의미:

```text
0x34 = '4'
0x0f = 흰 글자 / 검은 배경
0x32 = '2'
0x0f = 흰 글자 / 검은 배경
```

## 8. GDB에서 화면 직접 조작

GDB로 VGA 메모리에 직접 값을 써봅니다.

```gdb
set {char}0xb8000 = 'A'
set {char}0xb8001 = 0x0e
```

화면 첫 글자가 `A`로 바뀌고 색상도 바뀝니다.

다른 글자를 써볼 수도 있습니다.

```gdb
set {char}0xb8002 = 'B'
set {char}0xb8003 = 0x0c
```

## 9. 무한 루프와 `hlt` 확인

커널은 출력 후 무한 루프에 들어갑니다.

계속 실행합니다.

```gdb
continue
```

CPU를 다시 멈추려면 GDB에서 `Ctrl+C`를 누릅니다.

현재 명령어를 확인합니다.

```gdb
x/i $eip
info registers eip eflags
```

보통 `hlt` 또는 그 근처에 있습니다.

이 상태는 커널이 할 일을 끝내고 CPU를 정지시키는 루프에 들어갔다는 뜻입니다.

## 10. 한 번에 따라 해보기

처음에는 아래 명령 묶음을 그대로 실행해보면 됩니다.

```gdb
boot
si
info registers esp
si
vga
next
vga
next
vga
next
vga
next
vga
set {char}0xb8000 = 'A'
set {char}0xb8001 = 0x0e
continue
```

## 11. 감이 잡힌 뒤 해볼 실험

`src/kernel.c`에서 출력 문자를 바꿔봅니다.

```c
VGA_BUFFER[0] = 'O';
VGA_BUFFER[2] = 'K';
```

다시 빌드하고 디버깅합니다.

```sh
make check
make debug
make gdb-rich
```

`src/boot.s`에서 스택 설정을 일부러 잘못 바꿔보는 것도 좋은 실험입니다.

예를 들어 다음 코드를 주석 처리하면:

```asm
mov $stack_top, %esp
```

`call kernel_main` 이후 스택이 불안정해질 수 있습니다.

이 실험은 커널에서 스택 설정이 왜 중요한지 확인하는 데 도움이 됩니다.

## 자주 쓰는 GDB 명령

```gdb
boot
main
vga
code
stack
si
ni
next
continue
info registers
x/i $eip
x/16i $eip
x/16wx $esp
x/4xb 0xb8000
```

## 종료

GDB 종료:

```gdb
detach
quit
```

QEMU 종료:

```text
Ctrl+A
X
```

또는 monitor를 쓰는 경우:

```sh
make monitor
```

monitor 안에서:

```text
quit
```
