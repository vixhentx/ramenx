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
      #TODO: 暂时开地图炮来防止g++的query问题, 后续会在上游仓库breeze里面处理
      shellHook = ''
        export LC_ALL=C
        export LANG=C
        export CLANGD_FLAGS="--query-driver=${pkgs.gcc_multi}/bin/g++"
      '';
    });
  };
}
