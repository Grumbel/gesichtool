{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-23.05";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in {
        packages = rec {
          default = gesichtool;

          gesichtool = pkgs.stdenv.mkDerivation rec {
            pname = "gesichtool";
            version = "0.0.0";

            src = ./.;

            nativeBuildInputs = with pkgs; [
              cmake
              pkg-config
            ];

            buildInputs = with pkgs; [
              fmt
              opencv4
              dlib
            ];
          };
        };
      }
    );
}
