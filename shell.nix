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
    python313
    python313Packages.pytest
    python313Packages.pytest
    python313Packages.requests
  ];

}
