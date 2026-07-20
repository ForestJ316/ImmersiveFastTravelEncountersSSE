## Requirements
* [CMake](https://cmake.org/)
	* Add this to your `PATH`
* [Vcpkg](https://github.com/microsoft/vcpkg)
	* Add the environment variable `VCPKG_ROOT` with the value as the path to the folder containing vcpkg
* [Visual Studio Community 2022](https://visualstudio.microsoft.com/vs/older-downloads/) or [Visual Studio Community 2026](https://visualstudio.microsoft.com/)
	* Desktop development with C++


## Building
```
git clone https://github.com/ForestJ316/ImmersiveFastTravelEncountersSSE
cd ImmersiveFastTravelEncountersSSE

git submodule update --init --recursive

cmake --preset vs2022
cmake --build build --config Release
```

## Optional
* [Skyrim Special Edition](https://store.steampowered.com/app/489830)
	* Add the environment variable `SkyrimPath` to point to the folder where you want the .dll to be copied after it's finished building