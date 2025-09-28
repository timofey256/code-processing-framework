{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    gcc
    clang
    clang-tools   
    ccls          
    asio
    cmake
    gnumake
    pkg-config
    nodejs_22
    openssl
    nlohmann_json
    rabbitmq-c

    docker
    python313
    python313Packages.pytest
    python313Packages.requests
  ];

}

