{ ... }:
{
  perSystem = { inputs', pkgs, ... }:
  let
    inherit (inputs') breeze;
  in {
    devShells.default = breeze.devShells.development.overrideAttrs (old: {
      nativeBuildInputs = (old.nativeBuildInputs or []) ++ [
        pkgs.pkgsi686Linux.glibc
        pkgs.gcc_multi
      ];
      NIX_LDFLAGS = (old.NIX_LDFLAGS or "") + " -L${pkgs.pkgsi686Linux.gcc.cc.lib}/lib -L${pkgs.pkgsi686Linux.gcc.cc.libgcc}/lib";
    });
  };
}
