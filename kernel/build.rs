fn main() {
    println!("cargo:rustc-link-arg=src/boot.o");
    println!("cargo:rustc-link-arg=-Tsrc/linker.ld");
}
