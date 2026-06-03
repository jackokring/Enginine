# Enginine JUCE Plugin

So with my `VCVRack > KRTPluginA` plugins doing over 15,000+ downloads, I thought I'd give JUCE a go. So this is generic code. (2026-05-22) `VCVRack` can load VST3 plugins. The embedding into `VCVRack` allows for upto 8 outputs. The 24 parameters minus two inputs for a pre-filter insert is quite nice for options. 

So I've dumped `KDevelop` and started using `Zed` (with `bear` to make the `compile_commands.json` symlinked from the `Builds/LinuxMakefile` directory where `bear -- make` was run as a command). Yes, now all the strange errors are gone. (2026-05-26) `KDevelop` used to be good when it had the `Kt` toolkit.

## TODO

 - [X] Basic Audio Pass Volume
 - [X] ParameterAttachment Mechanism (for visual updates on parameter change)
 - [X] Load/Save State Mechanism
 - [X] Look and Feel
 - [ ] Audio Processing
  - [ ] Filter Input
  - [ ] Extra Outputs
  - [ ] Polyphonic Setup
  - [ ] Oscillator
  - [ ] Envelope
  - [ ] Modulation Routing
 - [X] Layout Using Own Grid Mehtod (27 GUI)
 - [X] Keyboard
 - [X] Simple Legacy MIDI In and Out (good for standalone MIDI)
 - [X] Parameter Preset Mechanism
 - [X] Python Utilities
 - [X] Parameter Skew Mechanism
 - [X] Background
 - [X] Zed/Bear Build
 - [ ] Hopefully Some Cool Sounding Stuff
 - [ ] Preset Filling

## MIDI CC Assignments

Testing with `Carla` as the host restricts use of CCs 2 (dry/wet), 7 (mix volume) and 8 (balance). Also CC 6 (RPN/NRPN data entry) would be somewhat of a pain to use in many studio kinds of settings. Also reserving CC 6 for RPN/NRPN data entry, allows for some hidden parameters to be accessed via CC.

The modulation wheel CC 1 is specially routed and not part of any preset information. This then leaves the rest of the 14 bit CCs for use as the 27 knob slots. In `C`/`C++` this was expressed in the 9 across and 3 down grid layout like below for matching with the front panel layout.

```C
    // MIDI CC 14 bit controller numbers n and n+32
    // 2 - Carla Dry/Wet (breath)
    // 6 - NRPN, RPN Data Entry (reserved)
    // 7 - Carla Volume
    // 8 - Carla Balance
    // parameter to MIDI CC
    int cc[3][9] = {
        {  0,  3,  4,  5,  9, 10, 11, 12, 13 },
        { 14, 15, 16, 17, 18, 19, 20, 21, 22 },
        { 23, 24, 25, 26, 27, 28, 29, 30, 31 }
    };
```

## Notes on Bear Setup and Building

The `JUCE` (8.0.13) is in `~/JUCE` and `Enginine` is in `~/Enginine`. All paths are absolute. A `bear.yml` file is used to configure the bear tool so this should be so. Note bear version schema `4.1` is used.

After cloning the repository. Do `cd Enginine` and `make clean`. Then maybe `sudo apt install bear` (for the bear tool) and `bear -- make` (instead of `make` just this once). Run `make user` to change `$HOME` (hardcoded to me as `/home/jacko`) to `/home/$USER` (your home directory) within `compile_commands.json` (for syntax completion). It might work. (2026-06-03)

The following `make` targets are available:

 - `make build` - build the code (default `make`)
 - `make clean` - clean the build
 - `make run` - run the executable standalone build
 - `make juce` - renew binary resource information (assumes `Projucer` built and `../JUCE` git repo)
 - `make user` - change `$HOME` to `/home/$USER` for syntax completion (do you need `bear`?)

 You need to `make juce` if you edit the `.png` or save an updated `presets.xml` as factory defaults.

## Comercial Stuff

 - [ ] Might not be in this repository (JUCE has reasonable terms)

I'd like to make some cash on this one. So I might do some kind of mix between open source with commercial terms. I'll think about it. (2026-06-02)
 
## Thanks

*Simon Jackson*

Project Director
