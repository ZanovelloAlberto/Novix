{ pkgs ? # import <nixpkgs> { }
  import (fetchTarball "https://github.com/NixOS/nixpkgs/tarball/release-23.11") { }
}:
let

  # crossPkgs = import <nixpkgs> {
  #   crossSystem = { config = "i686-linux-gnueabihf"; };
  # };
in
pkgs.mkShell {
  # Define the build inputs (dependencies) for the environment
  buildInputs = with pkgs; [
    dosfstools # Provides mkfs.fat
    nasm # Provides nasm assembler
    # gcc # Base gcc, used as a fallback for some utilities
    # crossPkgs.stdenv.cc
    pkgsCross.i686-embedded.buildPackages.gcc
    pkgsCross.i686-embedded.buildPackages.libgcc
    # pkgsCross.i686-embedded.stdenv.cc
    binutils # Provides ld (linker), though we'll configure for i686-elf
    pkg-config
    qemu # For running the OS with ./run.sh
    bochs # For running the OS with ./runbochs
  ];

  # Custom environment setup
  # shellHook = ''
  #   # Set up the cross-compiler toolchain for i686-elf
  #   export CC=${pkgs.pkgsCross.i686-embedded.gcc}/bin/i686-elf-gcc
  #   export LD=${pkgs.pkgsCross.i686-embedded.binutils}/bin/i686-elf-ld
  #   export ASM=${pkgs.nasm}/bin/nasm
  #   export FAT=${pkgs.dosfstools}/bin/mkfs.fat

  #   # Path to libgcc.a for i686-elf-gcc
  #   export LIBGCC_PATH=${pkgs.pkgsCross.i686-embedded.gcc.lib}/lib/gcc/i686-elf/*/libgcc.a

  #   # Ensure run.sh and runbochs are executable
  #   if [ -f ./run.sh ]; then
  #     chmod +x ./run.sh
  #   fi
  #   if [ -f ./runbochs ]; then
  #     chmod +x ./runbochs
  #   fi

  #   echo "NOVIX Development Environment"
  #   echo "-----------------------------"
  #   echo "CC: $CC"
  #   echo "LD: $LD"
  #   echo "ASM: $ASM"
  #   echo "FAT: $FAT"
  #   echo "LIBGCC_PATH: $LIBGCC_PATH"
  #   echo "QEMU: ${pkgs.qemu}/bin/qemu-system-i386"
  #   echo "Bochs: ${pkgs.bochs}/bin/bochs"
  #   echo "-----------------------------"
  #   echo "Run 'make' to build the project."
  #   echo "Use './run.sh' to launch with QEMU or './runbochs' to launch with Bochs."
  # '';
}
