{
  description = "John Bot";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem (system:
    let
      overlays = [];
      pkgs = import nixpkgs {
        inherit system overlays;
      };
      llvm = pkgs.llvmPackages_18;
      stdenv = llvm.libcxxStdenv;
    in
    with pkgs; pkgs.mkShell.override { stdenv = stdenv; } {
      devShells.default = mkShell rec {
        packages = with pkgs; [
          ripgrep cmake-language-server
          llvm.libcxx llvm.clang-tools llvm.clangUseLLVM llvm_18
          pkg-config cmake ninja codespell
          lldb

          openssl sqlite
        ];
      };
    }
  );
}

