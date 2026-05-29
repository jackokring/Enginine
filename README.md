# Enginine JUCE Plugin

So with my `VCVRack > KRTPluginA` plugins doing over 15,000+ downloads, I thought I'd give JUCE a go. So this is generic code (2026-05-22)

So I've dumped `KDevelop` and started using `Zed` (with `bear` to make the `compile_commands.json` symlinked from the `Builds/LinuxMakefile` directory where `bear -- make` was run as a command). Yes, now all the strange errors are gone.
(2026-05-26)

## TODO

 - [X] Basic audio pass volume
 - [X] ParameterAttachment mechanism (for visual updates on parameter change)
 - [X] Load/save state mechanism
 - [ ] Look and Feel
 - [ ] Audio Processing
 - [X] Layout Using Own Grid Mehtod (27 GUI)
 - [X] Keyboard
 - [ ] Simple MIDI out
 - [X] Parameter Preset Mechanism (`preset.zlib` packaging by `projucer` save)
 - [X] Python utility `makezlib.py` (`preset.xml` to `preset.zlib` conversion)
 - [X] Parameter Skew Mechanism
 - [ ] SVG/vector background
 - [X] Zed/Bear build (if your home is `/home/jacko`)

## Notes on Bear Setup

After cloning the repository, delete the `.json` file in the root. Goto `Builds/LinuxMakefile` and `make clean`.
Then maybe `sudo apt install bear` (for the bear tool) and `bear -- make` instead of `make`. The made file
`compile_commands.json` needs linking to the root by `ln -s compile_commands.json ../../compile_commands.json`
within the `LinuxMakefile` directory. The settings of open terminals for me are project root (`git`),
`LinuxMakefile` (`make`) and a final terminal open in `Builds/LinuxMakefile/build` (for `./Enginine` launching).

The `JUCE` is in `~/JUCE` and `Enginine` is in `~/Enginine`. So only the hard codded `$HOME` path in the project might cause problems. Can't be helped as far as I've checked.

I'll work more on this setup later on as the project evolves.

## Comercial Stuff

 - [ ] Might not be in this repository (JUCE has reasonable terms)
 
## Thanks

Simon Jackson

Project Director
