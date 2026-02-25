{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    devkitNix.url = "github:bandithedoge/devkitNix";
    devkitNix.inputs.nixpkgs.follows = "nixpkgs";
    pineapple.url = "github:p1n3appl3/config";
    pineapple.inputs.nixpkgs.follows = "nixpkgs";
  };

  outputs = { self, nixpkgs, flake-utils, devkitNix, pineapple }:
    flake-utils.lib.eachDefaultSystem (
      system: let
        pkgs = import nixpkgs {
          inherit system;
          overlays = [
            devkitNix.overlays.default
            pineapple.overlays.default
          ];
        };
      in
      {
        devShells.default = pkgs.mkShell.override { stdenv = pkgs.devkitNix.stdenvPPC; } {
          buildInputs = with pkgs; [
            gc-fst
            hgecko
            hmex
            xdelta
          ];
        };
      }
    );
}
