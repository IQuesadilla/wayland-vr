let
  nixpkgs = fetchTarball "https://github.com/NixOS/nixpkgs/tarball/nixos-24.11";
  unstable = import (fetchTarball "https://nixos.org/channels/nixos-unstable/nixexprs.tar.xz") { };
  pkgs = import nixpkgs { config = {}; overlays = []; };
in

pkgs.mkShellNoCC {
  #packages = with pkgs; [
  packages = [
    pkgs.zig
    pkgs.gcc
    pkgs.zls
    pkgs.sdl3
    pkgs.freetype
    pkgs.doxygen
    pkgs.neovim
    pkgs.lunarvim
    pkgs.lua
    pkgs.apr
    unstable.sdl3-ttf
  ];
}
