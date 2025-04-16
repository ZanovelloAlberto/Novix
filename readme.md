# 🛠️ NOVIX Compilation Guide

## 📦 Prerequisites

Before compiling this project, ensure the following tools are installed on your system:

- `mkfs.fat`
- `nasm`
- `i686-elf-gcc`
- `i686-elf-ld`

> ⚠️ **Important**: These tools **must be referenced using their absolute paths** in the main `Makefile`.

---

## ⚙️ Configuration

### 1. Edit the Makefile

Before starting the build process, open the main `Makefile` and make sure to:

- Set the **absolute path** to each required tool:
  - `mkfs.fat`
  - `nasm`
  - `i686-elf-gcc`
  - `i686-elf-ld`
- Specify the **absolute path** to `libgcc.a`.  
  This is necessary because `i686-elf-gcc` **does not include it automatically** during linking.

#### Example Configuration

```makefile
FAT=mkfs.fat
ASM=nasm
CC=/home/novice/cross/i686-elf/bin/i686-elf-gcc
LD=/home/novice/cross/i686-elf/bin/i686-elf-ld
LIBGCC_PATH=/home/novice/cross/i686-elf/lib/gcc/i686-elf/14.2.0
```

---

### 2. Building the Project

Once all paths are correctly configured, run the following command to build the project:

```bash
make
```

The compiled binary will be generated in the output directory specified in your `Makefile`.

---

## 🚀 Running the OS

After compilation, you can run the NOVIX operating system using one of the following options:

### ✅ Using QEMU

Run the following script to launch NOVIX with QEMU:

```bash
./run.sh
```

### ✅ Using Bochs

Alternatively, to launch NOVIX using Bochs, run:

```bash
./runbochs
```

> Make sure that both scripts (`run.sh` and `runbochs`) are executable:
>
> ```bash
> chmod +x run.sh runbochs
> ```

---

## 📝 Notes

- ✅ You **must use** the cross-compiler `i686-elf-gcc`.  
  ❌ **Do not** use your system’s default `gcc`.

- 🔍 If you're unsure where `libgcc.a` is located, you can search for it with:

  ```bash
  find / -name "libgcc.a" 2>/dev/null
  ```

  Then, update the `LIBGCC_PATH` variable in your `Makefile` with the absolute path found.

---

## 💬 Feedback

I would love to hear your suggestions and feedback on my [Discord]() 🚀
