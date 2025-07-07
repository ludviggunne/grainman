{
  pkgs ? import <nixpkgs> {},
}:

pkgs.stdenv.mkDerivation {
  name = "grainman";
  src = ./.;
  buildInputs = with pkgs; [
    jack2
    libsndfile
    libsamplerate
  ];
  installPhase = ''
    make install prefix=$out
  '';
}
