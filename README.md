Wasm Raytracing
====================================================================================================
LINK: https://justinliang1020.github.io/raytracing.github.io/

Introduction
------------------
This is a webassembly-based raytracer that allows for real-time camera movement and rendering all done in-browser. It is a fork of the [Raytracing in One Weekend](https://raytracing.github.io/books/RayTracingInOneWeekend.html) book series/code by Peter Shirley.

Stuff I added:
SDL2 compatibility
WebAssembly compatibility through Emscripten
Real-time camera movement on lower render settings (NOTE: VERTICAL CAMERA ROTATION IS BUGGED WILL FIX)
World editing through adding spheres through GUI
Varied quality rendering settings through GUI


Building and Running
---------------------
Will update with more specific instructions later!

1. [Install Emscripten] (https://emscripten.org/docs/getting_started/downloads.html)
2. [Install SDL2] (https://www.libsdl.org/download-2.0.php)
3. cd into the docs/ folder (this folder has been renamed from src/ to docs/ for github pages compatibility)
4. run this command: em++ main.cc -o index_template.html --shell-file index.html -s USE_SDL=2
