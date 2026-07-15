# Kernel From Scratch 1 - C

Rust 버전 커널을 C로 포팅한 프로젝트입니다.

동작은 원본과 동일합니다.

- GRUB Multiboot 헤더를 가진 32비트 i686 ELF 커널을 빌드합니다.
- 부트 ASM이 스택을 설정하고 `kernel_main`을 호출합니다.
- C 커널이 VGA 텍스트 버퍼 `0xB8000`에 `42`를 출력합니다.

## 툴체인

기본값은 사용자가 준비한 크로스 컴파일러 경로입니다.

```sh
export PREFIX=/home/jinseo/opt/cross
export TARGET=i686-elf
export PATH=/home/jinseo/opt/cross/bin:$PATH
```

`Makefile`에도 같은 기본값이 들어 있습니다.

## 빌드

```sh
make
```

생성물:

```text
kernel.bin
```

Multiboot 인식 여부를 확인하려면:

```sh
make check
```

## 이미지 설치 및 실행

기본 이미지 경로는 Rust 프로젝트의 기존 이미지입니다.

```text
../Kernel_From_Scratch_1_Rust/kfs.img
```

커널을 이미지에 설치하려면:

```sh
make install
```

QEMU로 실행하려면:

```sh
make run
```

다른 이미지를 쓰려면 `IMG`를 넘기면 됩니다.

```sh
make run IMG=/path/to/kfs.img
```

## 구조

```text
.
├── Makefile
├── README.md
└── src
    ├── boot.s
    ├── kernel.c
    └── linker.ld
```
