<h1 align="center">
<img align="center" src="https://cdn.cloudflare.steamstatic.com/steam/apps/736260/header.jpg" width="50%"><br>
Baba Is You · PSVita Port
</h1>
<p align="center">
  <a href="#setup-instructions-for-end-users">How to install</a> •
  <a href="#controls">Controls</a> •
  <a href="#build-instructions-for-developers">How to compile</a> •
  <a href="#credits">Credits</a> •
  <a href="#license">License</a>
</p>

Baba Is You is an award-winning puzzle game where you can change the rules
by which you play. In every level, the rules themselves are present as
blocks you can interact with; by manipulating them, you can change how
the level works and cause surprising, unexpected interactions!

This repository contains a loader of **the Android release of Baba Is You**, based
on the [Android SO Loader by TheFloW](https://github.com/TheOfficialFloW/gtasa_vita).
The loader provides a tailored, minimalistic Android-like environment to run
the official ARMv7 game executables on the PS Vita.

**This software does not contain the original code, executables, assets, or other
not redistributable parts of the game. The authors do not promote or condone piracy
in any way. To launch and play the game on their PS Vita device, users must provide
their own legally obtained copy of the game in form of an .apk file.**

## Setup Instructions (For End Users)

In order to properly install the game, you'll have to follow these steps precisely:

- Install [kubridge](https://github.com/TheOfficialFloW/kubridge/releases/) and [FdFix](https://github.com/TheOfficialFloW/FdFix/releases/) by copying `kubridge.skprx` and `fd_fix.skprx` to your taiHEN plugins folder (usually `ur0:tai`) and adding two entries to your `config.txt` under `*KERNEL`:
  
```
  *KERNEL
  ur0:tai/kubridge.skprx
  ur0:tai/fd_fix.skprx
```

**Note** Don't install fd_fix.skprx if you're using rePatch plugin!

- <u>Legally</u> obtain your copy of [Baba Is You](https://play.google.com/store/apps/details?id=org.hempuli.baba&hl=en_US&gl=US)
for Android in form of an `.apk` file. [You can get all the required files directly from your phone](https://stackoverflow.com/questions/11012976/how-do-i-get-the-apk-of-an-installed-app-without-root-access)
or by using any APK extractor you can find on Google Play.
- Open the `.apk` with any zip explorer (like [7-Zip](https://www.7-zip.org/)) and extract all folders from the `.apk` into `ux0:data/baba`. Example of resulting path: `ux0:data/baba/assets/Arrow.png`
- Install `BABA.vpk` (from [Releases](https://github.com/v-atamanenko/vita_is_you/releases/latest)).

Controls
-----------------

|             Button             | Action        |
|:------------------------------:|:--------------|
| ![joysl] / ![dpadh] / ![dpadv] | Move Baba     |
|            ![cross]            | Select / OK   |
|      ![circl] / ![squar]       | Undo          |
|            ![trian]            | Wait          |
|            ![selec]            | Restart level |
|            ![start]            | Open Menu     |

## Build Instructions (For Developers)

In order to build the loader, you'll need a [vitasdk](https://github.com/vitasdk) build fully compiled with softfp usage.  
You can find a precompiled version [here](https://github.com/vitasdk/buildscripts/releases).  
Additionally, you'll need these libraries to be compiled as well with `-mfloat-abi=softfp` added to their CFLAGS.
You can find precompiled versions [here](https://github.com/Rinnegatamante/vitasdk-packages-softfp/releases).

- OpenSLES
- mathneon
- vita2d
- VitaGL

Additionally, you'll need to manually compile [Northfear's fork of SDL2](https://github.com/Northfear/SDL) with VitaGL backend.

After all these requirements are met, you can compile the loader with the following commands:

```bash
cmake -Bbuild .
cmake --build build
```

Also note that this CMakeLists has two "convenience targets". While developing, I highly recommed using them, like this:
```bash
cmake --build build --target send # Build, upload eboot.bin and run (requires vitacompanion)
cmake --build build --target dump # Fetch latest coredump and parse
```

For more information and build options, read the [CMakeLists.txt](CMakeLists.txt).

## Credits
- [Andy "The FloW" Nguyen](https://github.com/TheOfficialFloW/) for the original .so loader.
- [Rinnegatamante](https://github.com/Rinnegatamante/) for VitaGL and lots of help with understanding and debugging the loader.
- [psykana](https://github.com/psykana/) for technical advice, help with testing, patching the side buttons, moral support, and memes.
- [Northfear](https://github.com/Northfear/) for the VitaGL SDL fork and shared experience of porting SDL-powered Android games.
- [GrapheneCt](https://github.com/GrapheneCt/) for PVR_PSP2, sfp2hfp, and pthread wrapper functions.
- Everybody on the Vita scene who answered my occasional stupid questions on Discord.

## License
This software may be modified and distributed under the terms of
the MIT license. See the [LICENSE](LICENSE) file for details.

[cross]: https://raw.githubusercontent.com/v-atamanenko/sdl2sand/master/img/cross.svg "Cross"
[circl]: https://raw.githubusercontent.com/v-atamanenko/sdl2sand/master/img/circle.svg "Circle"
[squar]: https://raw.githubusercontent.com/v-atamanenko/sdl2sand/master/img/square.svg "Square"
[trian]: https://raw.githubusercontent.com/v-atamanenko/sdl2sand/master/img/triangle.svg "Triangle"
[joysl]: https://raw.githubusercontent.com/v-atamanenko/sdl2sand/master/img/joystick-left.svg "Left Joystick"
[dpadh]: https://raw.githubusercontent.com/v-atamanenko/sdl2sand/master/img/dpad-left-right.svg "D-Pad Left/Right"
[dpadv]: https://raw.githubusercontent.com/v-atamanenko/sdl2sand/master/img/dpad-top-down.svg "D-Pad Up/Down"
[selec]: https://raw.githubusercontent.com/v-atamanenko/sdl2sand/master/img/dpad-select.svg "Select"
[start]: https://raw.githubusercontent.com/v-atamanenko/sdl2sand/master/img/dpad-start.svg "Start"
[trigl]: https://raw.githubusercontent.com/v-atamanenko/sdl2sand/master/img/trigger-left.svg "Left Trigger"
[trigr]: https://raw.githubusercontent.com/v-atamanenko/sdl2sand/master/img/trigger-right.svg "Right Trigger"
