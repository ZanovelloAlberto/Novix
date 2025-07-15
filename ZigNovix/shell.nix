{ pkgs ?
import <nixpkgs> { }
  # import (fetchTarball "https://github.com/NixOS/nixpkgs/tarball/release-23.11") { }
}:
pkgs.mkShell {
  # Define the build inputs (dependencies) for the environment
  buildInputs = with pkgs; [
    # dosfstools # Provides mkfs.fat
    # zig_0_13
    zls
    gemini-cli
    
    # crossPkgs.stdenv.cc
    # binutils # Provides ld (linker), though we'll configure for i686-elf
    # pkg-config
    qemu # For running the OS with ./run.sh
  ];

}
