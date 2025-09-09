{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = [
    pkgs.docker_28
    pkgs.xorg.xhost
    pkgs.gcc
    pkgs.nodejs_22
    pkgs.asio
  ];
}


