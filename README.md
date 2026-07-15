# Grub, Boot and Screen

> Rust + QEMU + GRUB 기반 x86 커널 개발  
> 환경: Ubuntu 24.04 (서버) / macOS M1 (클라이언트)

---

## 목차

1. [환경 개요](#1-환경-개요)
2. [디스크 이미지 생성 및 GRUB 설치](#2-디스크-이미지-생성-및-grub-설치)
3. [Rust 프로젝트 설정](#3-rust-프로젝트-설정)
4. [커널 소스 코드](#4-커널-소스-코드)
5. [빌드](#5-빌드)
6. [이미지에 커널 배포 및 GRUB 설정](#6-이미지에-커널-배포-및-grub-설정)
7. [실행 및 결과 확인](#7-실행-및-결과-확인)
8. [프로젝트 구조](#8-프로젝트-구조)
9. [트러블슈팅](#9-트러블슈팅)
10. [핵심 개념 정리](#10-핵심-개념-정리)

---

## 1. 환경 개요

| 항목 | 내용 |
|------|------|
| OS (서버) | Ubuntu 24.04 |
| 클라이언트 | macOS M1 |
| 에뮬레이터 | QEMU 8.2.2 (`qemu-system-i386`) |
| 부트로더 | GRUB 2.12 |
| 언어 | Rust (nightly) |
| 아키텍처 | i386 (x86 32bit) |
| 디스플레이 | XQuartz (X11 포워딩) + `-display curses` |

### 사전 설치 확인

```bash
qemu-system-i386 --version
grub-install --version
rustc --version
cargo --version
```

> GRUB과 QEMU가 없다면:
> ```bash
> sudo apt install grub-pc-bin grub-common qemu-system-x86
> ```

---

## 2. 디스크 이미지 생성 및 GRUB 설치

GRUB은 VM 위에 올리는 게 아니라 **가상 디스크 이미지 파일(.img) 안에 설치**된다.  
QEMU는 이 이미지 파일을 가상 하드디스크처럼 실행하는 도구다.

### 2.1 자동 초기화 스크립트

`scripts/init-image.sh`는 빈 이미지 생성부터 GRUB 설정까지 한 번에 수행한다.

```bash
./scripts/init-image.sh kfs.img
```

이미 같은 이름의 이미지가 있으면 덮어쓰지 않고 중단한다. 기존 이미지를 다시 만들려면 직접 삭제한 뒤 실행한다.

스크립트가 수행하는 작업:

```text
dd로 kfs.img 생성
→ loop device 연결
→ MBR 파티션 생성
→ ext2 포맷
→ GRUB 설치
→ /boot/grub/grub.cfg 생성
→ loop device 해제
```

아래 2.2~2.5는 같은 과정을 수동으로 수행하는 절차다.

### 2.2 빈 디스크 이미지 생성

```bash
dd if=/dev/zero of=kfs.img bs=1M count=10
```

| 옵션 | 설명 |
|------|------|
| `if=/dev/zero` | 입력 소스. `0x00`을 무한히 출력하는 가상 장치 |
| `of=kfs.img` | 출력 파일 |
| `bs=1M` | 블록 사이즈 1MB |
| `count=10` | 10번 반복 → 총 10MB |

> `/dev/zero`와 `/dev/null`은 다르다.  
> `/dev/null`은 읽으면 즉시 EOF이라 빈 파일을 만들 수 없다. 반드시 `/dev/zero`를 사용해야 한다.

### 2.3 루프 디바이스 연결

이미지 파일을 블록 디바이스처럼 다루기 위해 루프 디바이스에 연결한다.

```bash
sudo losetup -fP kfs.img
sudo losetup -a | grep kfs.img   # 할당된 번호 확인 (예: /dev/loop8)
```

> 같은 이미지가 두 개 이상의 루프 디바이스에 잡히면:
> ```bash
> sudo losetup -d /dev/loopN   # 중복 해제 후 재연결
> ```

### 2.4 파티션 및 파일시스템 설정

```bash
sudo parted /dev/loop8 mklabel msdos
sudo parted /dev/loop8 mkpart primary ext2 1MiB 100%
sudo parted /dev/loop8 set 1 boot on
sudo mkfs.ext2 /dev/loop8p1
```

| 명령어 | 설명 |
|--------|------|
| `mklabel msdos` | MBR 파티션 테이블 생성. GRUB i386-pc는 MBR 방식 필요 |
| `mkpart primary ext2 1MiB 100%` | 1MiB부터 끝까지 파티션 생성. 앞 1MiB는 GRUB 부트코드 공간 |
| `set 1 boot on` | boot 플래그 설정. BIOS가 부팅 파티션으로 인식 |
| `mkfs.ext2` | ext2 파일시스템으로 포맷 |

### 2.5 GRUB 설치

```bash
mkdir -p /tmp/kfs_mount
sudo mount /dev/loop8p1 /tmp/kfs_mount
sudo grub-install --target=i386-pc --boot-directory=/tmp/kfs_mount/boot /dev/loop8
sudo umount /tmp/kfs_mount
sudo losetup -d /dev/loop8
```

`grub-install`은 두 곳에 GRUB을 설치한다:
- **MBR**: `/dev/loop8`의 첫 446바이트에 1단계 부트코드
- **`/boot/grub/`**: GRUB 모듈 파일들

### 2.6 부팅 확인

```bash
qemu-system-i386 -drive file=kfs.img,format=raw -nographic
```

GRUB 프롬프트(`grub>`)가 뜨면 성공. 종료는 `Ctrl+A` 후 `X`.

---

## 3. Rust 프로젝트 설정

### 3.1 프로젝트 생성 및 nightly 전환

```bash
cargo new --bin kernel
cd kernel
rustup override set nightly
rustup component add rust-src --toolchain nightly-x86_64-unknown-linux-gnu
rustup target add i686-unknown-linux-gnu
```

`-Z build-std` 등 커스텀 타겟 빌드 기능은 nightly 전용이다.

### 3.2 커스텀 타겟 파일 (`i386-kernel.json`)

Rust는 `i386-unknown-none` 타겟을 기본 지원하지 않으므로 직접 정의한다.

```json
{
    "llvm-target": "i386-unknown-none",
    "data-layout": "e-m:e-p:32:32-p270:32:32-p271:32:32-p272:64:64-i128:128-f64:32:64-f80:32-n8:16:32-S128",
    "arch": "x86",
    "target-endian": "little",
    "target-pointer-width": 32,
    "target-c-int-width": 32,
    "os": "none",
    "executables": true,
    "linker-flavor": "ld.lld",
    "linker": "rust-lld",
    "panic-strategy": "abort",
    "disable-redzone": true,
    "features": "-mmx,-sse"
}
```

> - `target-pointer-width`는 반드시 숫자 `32`여야 한다. 문자열 `"32"`로 쓰면 오류 발생.
> - `+soft-float`은 최신 nightly에서 ABI 비호환 오류가 나므로 제거한다.

### 3.3 `Cargo.toml`

```toml
[package]
name = "kernel"
version = "0.1.0"
edition = "2021"

[profile.dev]
panic = "abort"

[profile.release]
panic = "abort"
```

### 3.4 `.cargo/config.toml`

```toml
[unstable]
build-std = ["core"]
build-std-features = ["compiler-builtins-mem"]

[build]
target = "i386-kernel.json"

[target.'cfg(target_arch = "x86")']
rustflags = [
    "-C", "embed-bitcode=no",
]
```

### 3.5 `build.rs`

`boot.o`를 링크하고 링커 스크립트를 적용한다.  
`boot.o`는 반드시 링커 스크립트보다 먼저 전달해야 한다.

```rust
fn main() {
    println!("cargo:rustc-link-arg=src/boot.o");
    println!("cargo:rustc-link-arg=-Tsrc/linker.ld");
}
```

---

## 4. 커널 소스 코드

### 4.1 `src/main.rs`

```rust
#![no_std]
#![no_main]

use core::panic::PanicInfo;

// VGA 텍스트 버퍼 물리 주소
const VGA_BUFFER: *mut u8 = 0xb8000 as *mut u8;

#[no_mangle]
pub extern "C" fn kernel_main() {
    unsafe {
        *VGA_BUFFER = b'4';
        *VGA_BUFFER.add(1) = 0x0f;  // 색상: 흰 글자(f) + 검정 배경(0)
        *VGA_BUFFER.add(2) = b'2';
        *VGA_BUFFER.add(3) = 0x0f;
    }
    loop {}
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}
```

> VGA 텍스트 모드에서 각 문자는 2바이트다.
> - 바이트 0: ASCII 문자 코드
> - 바이트 1: 색상 속성 (`0x0f` = 흰 글자, 검정 배경)

### 4.2 `src/boot.s`

GRUB은 커널 파일의 **첫 8KB 안에서 매직 넘버 `0x1BADB002`**를 찾는다.  
`.text` 섹션 시작 부분에 직접 삽입하여 파일 앞부분에 위치하도록 한다.

```asm
.set MAGIC,    0x1BADB002
.set FLAGS,    (1 << 1)
.set CHECKSUM, -(MAGIC + FLAGS)

.section .text
.align 4
    .long MAGIC
    .long FLAGS
    .long CHECKSUM

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

### 4.3 `src/linker.ld`

커널은 1MB(`0x100000`)에 로드된다.  
`KEEP(src/boot.o(.text))`로 boot.o의 `.text`가 반드시 맨 앞에 오도록 강제한다.

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

    .rodata :
    {
        *(.rodata)
        *(.rodata.*)
    }

    .data :
    {
        *(.data)
        *(.data.*)
    }

    .bss :
    {
        *(.bss)
        *(.bss.*)
        *(COMMON)
    }

    /DISCARD/ :
    {
        *(.eh_frame)
        *(.debug_*)
        *(.note.*)
        *(.comment)
    }
}
```

---

## 5. 빌드

빌드는 `kernel/Makefile`이 담당한다.  
Makefile은 ASM만 직접 오브젝트로 만들고, Rust 최종 링크는 Cargo와 `build.rs`에 맡긴다.

```bash
make -C kernel build
```

내부 흐름:

```text
src/boot.s
 └→ src/boot.o
     └→ cargo +nightly build --release -Zjson-target-spec
         └→ target/i386-kernel/release/kernel
             └→ kernel.bin
```

`build.rs`가 `src/boot.o`와 `src/linker.ld`를 링크 인자로 전달하므로, `ld`를 직접 호출하거나 Cargo 내부의 `.o`/`.rlib` 파일을 찾지 않는다.

### 5.1 빌드 결과 검증

Makefile은 `kernel.bin` 생성 후 multiboot 검증을 실행한다.

```bash
grub-file --is-x86-multiboot kernel/kernel.bin
```

심볼 주소를 확인하려면:

```bash
nm kernel/kernel.bin | grep "_start\|kernel_main"
```

---

## 6. 이미지에 커널 배포 및 GRUB 설정

현재 `kfs.img`에는 이미 GRUB과 `/boot/grub/grub.cfg`가 들어 있다.  
새 커널을 이미지에 반영할 때는 Makefile의 `install` 타겟으로 `/boot/kernel`만 교체한다.

```bash
make -C kernel install
```

이미지 안의 GRUB 설정은 다음 경로를 부팅한다.

```cfg
menuentry "kfs" {
    insmod multiboot
    multiboot /boot/kernel
    boot
}
```

---

## 7. 실행 및 결과 확인

### 7.1 XQuartz 설정 (macOS 클라이언트)

VGA 텍스트 출력을 보려면 GUI 창이 필요하다. SSH X11 포워딩을 사용한다.

1. [https://www.xquartz.org](https://www.xquartz.org) 에서 XQuartz 설치
2. XQuartz 실행 후 터미널에서 접속:

```bash
ssh -X -p <포트> <유저명>@<서버IP>
echo $DISPLAY   # localhost:10.0 처럼 출력되어야 함
```

### 7.2 QEMU 실행

SDL/GTK가 GLX 오류로 실패할 경우 `-display curses`를 사용한다.

```bash
make -C kernel run
```

GRUB 메뉴에서 `kfs` 항목 선택 → 화면에 `42` 출력 확인.

> `-nographic` 모드에서는 VGA 텍스트 출력이 보이지 않는다.  
> 반드시 GUI 창(`curses` 또는 `GTK`/`SDL`)으로 확인해야 한다.

---

## 8. 프로젝트 구조

```
kfs1/
├── README.md
├── .gitignore
├── kfs.img                  # 가상 디스크 이미지 (GRUB 설치됨)
├── scripts/
│   └── init-image.sh        # kfs.img 초기 생성 및 GRUB 설치
└── kernel/
    ├── Cargo.toml
    ├── Cargo.lock
    ├── Makefile             # 빌드, 이미지 설치, QEMU 실행
    ├── build.rs             # boot.o 링크 및 링커 스크립트 적용
    ├── i386-kernel.json     # 커스텀 타겟 정의
    ├── kernel.bin           # 생성 산출물: 최종 커널 바이너리
    ├── .gitignore
    ├── .cargo/
    │   └── config.toml      # nightly 빌드 설정
    └── src/
        ├── main.rs          # 커널 메인 (VGA 출력)
        ├── boot.s           # ASM 부트 코드 (multiboot 헤더 + _start)
        ├── boot.o           # 생성 산출물: 컴파일된 ASM
        └── linker.ld        # 링커 스크립트
```

---

## 9. 트러블슈팅

| 오류 | 원인 및 해결 |
|------|-------------|
| `gtk initialization failed` | SSH 환경에서 GTK 디스플레이 없음. Makefile의 `run`은 `-display curses` 사용 |
| `no multiboot header found` | multiboot 헤더가 파일 첫 8KB 밖에 위치. `linker.ld`에서 `KEEP(src/boot.o(.text))`로 boot.o 섹션을 맨 앞에 강제 배치 |
| `ELF section header region is larger than the file size` | ELF 파일 손상. `/DISCARD/` 섹션에 `debug`/`note`/`comment` 추가하여 제거 |
| `cannot use executable file as input to a link` | Cargo 결과물을 다시 `ld` 입력으로 넣은 경우. 현재 Makefile 정책에서는 직접 `ld`를 호출하지 않는다 |
| `target-pointer-width: invalid type: string` | JSON 타겟 파일에서 `32`를 문자열이 아닌 숫자로 작성 |
| `soft-float is incompatible with the ABI` | `features`에서 `+soft-float` 제거 |
| `-Z flag only accepted on nightly` | `rustup override set nightly`로 nightly 채널 전환 |
| `json-target-spec` 관련 오류 | `make -C kernel build` 또는 `cargo +nightly build --release -Zjson-target-spec` 사용 |
| `Is another process using the image` | 이전 QEMU 프로세스가 이미지 점유 중. `pkill qemu` 후 재실행 |
| `losetup -a`에 동일 이미지 중복 | `sudo losetup -d /dev/loopN`으로 중복 해제 후 재연결 |

---

## 10. 핵심 개념 정리

### 10.1 부팅 순서

```
BIOS
 └→ MBR (GRUB 1단계)
     └→ GRUB 2단계
         └→ grub.cfg 로드
             └→ multiboot 헤더 탐색
                 └→ _start 호출
                     └→ kernel_main 호출
```

### 10.2 Multiboot 헤더

GRUB이 커널을 인식하기 위한 식별자. 커널 파일의 **첫 8KB 안에 4바이트 정렬**로 위치해야 한다.

| 필드 | 값 | 설명 |
|------|----|------|
| `MAGIC` | `0x1BADB002` | GRUB이 찾는 고정 식별자 |
| `FLAGS` | `(1 << 1)` | GRUB에 메모리 맵 제공 요청 |
| `CHECKSUM` | `-(MAGIC + FLAGS)` | 세 값의 합이 반드시 0이어야 함 |

### 10.3 VGA 텍스트 모드

물리 주소 `0xB8000`부터 시작하는 80×25 문자 버퍼. 각 문자는 **2바이트**로 구성된다.

```
┌──────────┬──────────┐
│ ASCII(1B)│ 색상(1B) │
└──────────┴──────────┘
색상 바이트: [배경색(4bit)][글자색(4bit)]
0x0f = 흰 글자(f) + 검정 배경(0)
```

### 10.4 링커 스크립트의 역할

커널은 표준 링커 스크립트를 사용할 수 없다. `linker.ld`가 담당하는 것:

- 커널 로드 주소를 1MB(`0x100000`)로 설정
- 섹션 배치 순서 결정 (`boot.o`의 `.text`가 반드시 맨 앞)
- 불필요한 섹션(`debug`, `note` 등) 제거로 바이너리 크기 축소

### 10.5 `no_std` 커널의 제약

커널 환경에서는 Rust 표준 라이브러리(`std`)를 사용할 수 없다.

- `std`는 OS 기능(파일, 메모리 할당 등)에 의존하는데, 커널이 곧 OS이기 때문
- 대신 `core` 크레이트만 사용 가능 (순수 언어 기능만 포함)
- `panic_handler`를 직접 정의해야 함
