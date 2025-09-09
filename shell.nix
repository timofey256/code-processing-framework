{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    gcc
    clang
    clang-tools   
    ccls          
    cmake
    gnumake
    pkg-config

    docker
  ];

}
