# KFS1 Defense Keywords

이 문서는 KFS1 과제 디펜스에서 설명할 수 있어야 하는 핵심 개념을 정리한다.

가정하는 범위:

- Rust로 작성한 i386 커널
- GRUB Multiboot v1 부팅
- QEMU raw disk image 사용
- `kfs.img` 안의 `/boot/kernel`을 부팅 대상으로 사용

---

## 1. 전체 부팅 흐름

핵심 흐름:

```text
QEMU
-> kfs.img
-> MBR
-> GRUB
-> /boot/grub/grub.cfg
-> /boot/kernel
-> Multiboot header
-> _start
-> kernel_main
```

설명 포인트:

- QEMU는 커널을 직접 실행하지 않는다.
- QEMU는 `kfs.img`를 가상 디스크로 부팅한다.
- `kfs.img`의 MBR에 설치된 GRUB이 먼저 실행된다.
- GRUB은 `/boot/grub/grub.cfg`를 읽고 `/boot/kernel`을 로드한다.
- `/boot/kernel` 안의 Multiboot header를 보고 커널로 인식한다.
- 최종 진입점은 `boot.s`의 `_start`다.

리뷰어 질문:

- QEMU가 실행하는 것은 `kernel.bin`인가, `kfs.img`인가?
- GRUB은 어떤 설정 파일을 읽는가?
- `/boot/kernel`은 호스트 파일 경로인가, 이미지 내부 경로인가?

---

## 2. Disk Image and GRUB

키워드:

- `dd`
- raw disk image
- MBR
- partition table
- loop device
- ext2
- mount
- `grub-install`
- `/boot/grub/grub.cfg`
- `/boot/kernel`

설명 포인트:

- `dd`는 빈 디스크 이미지 파일을 만든다.
- `losetup`은 이미지 파일을 블록 디바이스처럼 다루게 해준다.
- `parted`는 이미지 안에 MBR 파티션 테이블과 파티션을 만든다.
- `mkfs.ext2`는 파티션을 ext2 파일시스템으로 포맷한다.
- `grub-install`은 MBR과 `/boot/grub/`에 GRUB 부트 코드를 설치한다.
- `make install`은 이미지를 다시 만들지 않고 `/boot/kernel`만 교체한다.

리뷰어 질문:

- 왜 `dd`를 매번 실행하면 안 되는가?
- `losetup -fP`에서 `-P`는 왜 필요한가?
- GRUB은 이미지의 어디에 설치되는가?
- `make install`은 정확히 무엇을 복사하는가?

---

## 3. Multiboot

키워드:

- Multiboot v1
- `MAGIC = 0x1BADB002`
- `FLAGS`
- `CHECKSUM`
- 4-byte alignment
- first 8 KiB
- `grub-file --is-x86-multiboot`
- `eax = 0x2BADB002`
- `ebx = multiboot info address`

`boot.s`의 Multiboot header:

```asm
.set MAGIC,    0x1BADB002
.set FLAGS,    (1 << 1)
.set CHECKSUM, -(MAGIC + FLAGS)

.section .text
.align 4
    .long MAGIC
    .long FLAGS
    .long CHECKSUM
```

설명 포인트:

- GRUB은 커널 파일 앞부분에서 Multiboot header를 찾는다.
- `MAGIC` 값은 Multiboot v1 커널임을 나타낸다.
- `CHECKSUM`은 `MAGIC + FLAGS + CHECKSUM == 0`이 되도록 만든 값이다.
- Multiboot header는 4바이트 정렬되어 있어야 한다.
- 보통 파일 첫 8KiB 안에 있어야 GRUB이 찾을 수 있다.
- 현재 코드는 GRUB이 넘기는 `eax`, `ebx` 값을 아직 사용하지 않는다.

리뷰어 질문:

- 왜 `CHECKSUM`이 `-(MAGIC + FLAGS)`인가?
- Multiboot header가 파일 뒤쪽으로 밀리면 무슨 일이 생기는가?
- `grub-file --is-x86-multiboot`는 무엇을 검증하는가?
- GRUB이 넘겨주는 `ebx`에는 무엇이 들어 있는가?

---

## 4. boot.s

키워드:

- `.text`
- `.bss`
- `.global _start`
- `ENTRY(_start)`
- `stack_bottom`
- `stack_top`
- `%esp`
- `call kernel_main`
- `cli`
- `hlt`
- AT&T syntax

핵심 코드:

```asm
.global _start
.type _start, @function
_start:
    mov $stack_top, %esp
    call kernel_main
    cli
1:  hlt
    jmp 1b

.section .bss
.align 16
stack_bottom:
    .skip 16384
stack_top:
```

설명 포인트:

- `_start`는 커널의 첫 실행 지점이다.
- `mov $stack_top, %esp`는 스택 포인터를 설정한다.
- x86 스택은 높은 주소에서 낮은 주소로 자란다.
- `.bss`의 16KiB 공간은 스택으로 쓰기 위해 예약한 공간이다.
- `call kernel_main`은 Rust 커널 함수로 진입한다.
- `kernel_main`이 리턴하면 갈 곳이 없으므로 `cli` 후 `hlt` 루프에 들어간다.

리뷰어 질문:

- `_start`는 누가 호출하는가?
- 왜 스택을 직접 잡아야 하는가?
- `stack_top`이 `stack_bottom`보다 뒤에 있는 이유는?
- `hlt` 앞에 `cli`를 두는 이유는?

---

## 5. linker.ld

키워드:

- linker script
- `ENTRY(_start)`
- `. = 1M`
- section layout
- `.text`
- `.rodata`
- `.data`
- `.bss`
- `KEEP(src/boot.o(.text))`
- `/DISCARD/`

핵심 코드:

```ld
ENTRY(_start)

SECTIONS {
    . = 1M;

    .text :
    {
        KEEP(src/boot.o(.text))
        *(.text)
        *(.text.*)
    }
}
```

설명 포인트:

- `ENTRY(_start)`는 최종 ELF의 진입점을 `_start`로 지정한다.
- `. = 1M`은 커널 섹션을 1MiB 주소부터 배치한다.
- 1MiB 아래 영역은 BIOS, VGA, real mode 관련 영역과 겹칠 수 있어 피한다.
- `KEEP(src/boot.o(.text))`는 `boot.o`의 `.text`를 최종 `.text` 앞쪽에 강제로 둔다.
- Multiboot header가 `boot.o(.text)` 앞에 있으므로 이 배치가 중요하다.
- `/DISCARD/`는 커널에 불필요한 디버그, note, comment 섹션 등을 버린다.

리뷰어 질문:

- 왜 커널을 1MiB에 배치하는가?
- `KEEP(src/boot.o(.text))`를 빼면 어떤 문제가 생길 수 있는가?
- `.bss`는 실제 파일에 16KiB가 그대로 들어가는가?
- `/DISCARD/`는 왜 필요한가?

---

## 6. Rust Kernel

키워드:

- `#![no_std]`
- `#![no_main]`
- `panic_handler`
- `extern "C"`
- `#[no_mangle]`
- `unsafe`
- volatile write
- VGA text buffer
- `0xb8000`

설명 포인트:

- OS가 없기 때문에 Rust 표준 라이브러리 `std`를 사용할 수 없다.
- 일반 Rust 런타임이 없기 때문에 `main`도 자동 호출되지 않는다.
- 어셈블리에서 심볼 이름으로 `kernel_main`을 호출하므로 `#[no_mangle]`이 필요하다.
- C ABI로 호출하기 위해 `extern "C"`를 사용한다.
- VGA text buffer는 메모리 주소 `0xb8000`에 직접 쓰는 방식이다.
- raw pointer 접근은 Rust 안전성 규칙 밖의 작업이므로 `unsafe`가 필요하다.
- 컴파일러가 메모리 쓰기를 최적화로 제거하지 못하게 volatile write를 사용한다.

리뷰어 질문:

- 왜 `std`를 못 쓰는가?
- `#[no_mangle]`이 없으면 어떤 일이 생기는가?
- `extern "C"`는 왜 필요한가?
- VGA 메모리 쓰기는 왜 `unsafe`인가?

---

## 7. Build Pipeline

키워드:

- `as --32`
- `boot.o`
- `cargo +nightly`
- custom target json
- `rust-src`
- `build.rs`
- `rustc-link-arg`
- `-Zjson-target-spec`
- `kernel.bin`
- `grub-file`

빌드 흐름:

```text
src/boot.s
-> as --32
-> src/boot.o
-> cargo +nightly build
-> build.rs가 boot.o와 linker.ld를 링커에 전달
-> target/i386-kernel/release/kernel
-> kernel.bin
-> grub-file --is-x86-multiboot
```

설명 포인트:

- `as --32`는 32-bit x86 오브젝트 파일을 만든다.
- `cargo +nightly`는 nightly Rust로 커널을 빌드한다.
- 커스텀 타겟을 쓰기 때문에 nightly 기능과 `rust-src`가 필요하다.
- `build.rs`는 링크 단계에 `src/boot.o`와 `src/linker.ld`를 전달한다.
- `kernel.bin`은 GRUB이 로드할 최종 커널 ELF다.
- `grub-file`은 결과물이 Multiboot 커널로 인식되는지 검증한다.

리뷰어 질문:

- 왜 nightly가 필요한가?
- 왜 `rust-src`가 필요한가?
- Cargo는 `boot.o`와 `linker.ld`의 존재를 어떻게 알게 되는가?
- `kernel.bin`과 `target/i386-kernel/release/kernel`의 관계는?

---

## 8. Makefile Responsibility

키워드:

- `make`
- `make install`
- `make run`
- `scripts/init-image.sh`
- idempotent
- dangerous operation

현재 역할 분리:

```text
scripts/init-image.sh
-> 최초 이미지 생성
-> 파티션 생성
-> 파일시스템 포맷
-> GRUB 설치
-> grub.cfg 생성

make
-> 커널 바이너리 빌드

make install
-> 기존 이미지의 /boot/kernel 교체

make run
-> install 후 QEMU 실행
```

설명 포인트:

- `make`는 커널 바이너리만 빌드한다.
- `make install`은 기존 `kfs.img`를 마운트해서 `/boot/kernel`만 교체한다.
- `make run`은 `make install` 후 QEMU를 실행한다.
- `dd`, `mkfs`, `grub-install`은 이미지를 파괴할 수 있으므로 기본 `make`에 넣지 않는다.
- 초기 이미지 생성은 `scripts/init-image.sh`로 분리했다.

리뷰어 질문:

- 왜 이미지 초기화를 기본 `make`에 넣지 않았는가?
- `make`와 `make install`의 차이는 무엇인가?
- `make run`이 실행하기 전에 `install`을 수행하는 이유는?
- 기존 `kfs.img`가 있을 때 `init-image.sh`가 멈추는 이유는?

---

## 9. High-Pressure Questions

아래 질문에 막히지 않으면 디펜스가 꽤 단단해진다.

1. GRUB은 `kernel.bin`의 무엇을 보고 Multiboot 커널이라고 판단하는가?
2. `_start`는 누가 호출하는가?
3. `stack_top`은 왜 `.bss`의 끝에 있는가?
4. `. = 1M`이 없으면 어떤 문제가 생길 수 있는가?
5. `KEEP(src/boot.o(.text))`를 빼면 무슨 일이 생길 수 있는가?
6. `kfs.img`와 `kernel.bin`은 어떤 관계인가?
7. `make`와 `make install`의 차이는 무엇인가?
8. `grub.cfg`의 `multiboot /boot/kernel`은 호스트 경로인가, 이미지 내부 경로인가?
9. Rust에서 `std`를 못 쓰는 이유는 무엇인가?
10. `unsafe` 없이 VGA 메모리에 쓸 수 있는가?
11. `rust-src`가 없으면 왜 빌드가 실패하는가?
12. GRUB이 넘겨주는 Multiboot info를 현재 커널이 사용하고 있는가?

---

## 10. One-Sentence Defense

```text
Rust 커널 코드를 직접 실행한 것이 아니라, GRUB이 인식할 수 있는 Multiboot ELF 커널을 만들고, 이를 디스크 이미지 안의 /boot/kernel로 배포해서 QEMU에서 BIOS -> GRUB -> 커널 순서로 부팅되도록 구성했다.
```
