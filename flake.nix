{
  description = "A very basic flake";

  inputs = {
    flake-utils.url = "github:numtide/flake-utils";
    flake-compat = {
      url = "github:edolstra/flake-compat";
      flake = false;
    };
  };
  outputs = { self, nixpkgs, flake-utils, flake-compat }:
  # let
  #   overlay = (final: prev: {
  #     euforiaPackages = final.lib.makeScope final.newScope (self: with self; {
  #       immer = callPackage ./immer.nix {};
  #       fmt = callPackage ./fmtlib.nix {};
  #       z3 = callPackage ./z3.nix {};
  #       mathsat = callPackage ./mathsat.nix {};
  #       outputCheck = callPackage ./outputcheck.nix {};
  #       propagateConst = callPackage ./propagate-const.nix {};
  #       euforia = callPackage ./euforia.nix { inherit self; };
  #     });
  #   });
  # in
  # {
  #   inherit overlay;
  # } //
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
        bitfunc_pkg = { lib, gccStdenv, python2, automake, autoconf, autoconf-archive, libtool, flex, bison, picosat, pkg-config, darwin, ncurses, zlib, libintl }: gccStdenv.mkDerivation rec {
          name = "bitfunc";
          version = "1.0.3";
          src = ./.;
          bitfunc-python-env = python2.withPackages (ps: with ps; [
            cython
          ]);
          nativeBuildInputs = [ automake autoconf autoconf-archive pkg-config flex bison libtool ];
          # libintl and ncurses are required  as "python extra libraries" when
          # ./configure tries to link with the dev version of python. I don't
          # know of an easy way to get these inputs programmatically.
          buildInputs = [ python2 bitfunc-python-env ncurses zlib libintl ]
          ++ lib.optionals gccStdenv.isDarwin [ darwin.apple_sdk.frameworks.CoreFoundation ];
        };
        bitfunc = pkgs.callPackage bitfunc_pkg {};
      in {
        packages = { inherit bitfunc; };
        defaultPackage = bitfunc;
        devShell =
          # let
          #   euforiaDev = pkgs.euforiaPackages.euforia.override { debugVersion = true; };
          # in
          pkgs.mkShell {
            inputsFrom = [ bitfunc ];
            packages = with pkgs; [ creduce universal-ctags ];
            inherit (bitfunc) configureFlags;
            hardeningDisable = [ "all" ];
            # TAGS = "${euforiaDev.tags}/tags";
            CXXFLAGS = "-Werror";
          };
        });

}
