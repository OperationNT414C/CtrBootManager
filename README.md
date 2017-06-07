#CtrBootManager

CtrBootManager is a boot manager for some 3DS homebrew applications, like HBL and CFW.

##Dependencies
- devkitpro-4.5 at least (ctrulib-1.2.1 at least)
- libconfig in portlibs (tried with v1.5)

##Building

###The recommended way:
 1. `mkdir build/`
 2. `cd build/`
 3. `cmake -DCMAKE_TOOLCHAIN_FILE=../DevkitArm3DS.cmake ../`
 4. `cmake --build . --target screeninit_a9lh`
 5. `bin2h -c screeninit < screeninit.bin > screeninit.h`
 6. `cmake --build . --target chainloader_a9lh`
 7. `bin2h -c chainloader < chainloader.bin > chainloader.h`
 8. `cmake --build . --target CtrBootManager_a9lh`

Binaries should now be in the `build` folder.

##Credits

###To original developer:
 * Cpasjuste

###For contributions to hb_menu:
 * smea : code
 * GEMISIS : code
 * fincs : code
 * mtheall : code
 * Fluto : graphics
 * Arkhandar : graphics
 * dotjasp : graphics (regionfree icon)

###For screeninit code:
 * Aurora Wright
 * dark_samus
 * bil1s
 * Normmatt
 * delebile 
 * tiniVi

###For FIRM loader and CtrNAND reading code:
 * Aurora Wright
 * TuxSH

###For movies support code example:
 * Wolfvak
 * d0k3 : compressed movies

###Other credits:

Readme by gemarcano
This project also uses [Brahma](https://github.com/patois/Brahma). See its repository for its credits.
