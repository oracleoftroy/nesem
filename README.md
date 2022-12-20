[![Contributors][contributors-shield]][contributors-url]
[![Forks][forks-shield]][forks-url]
[![Stargazers][stars-shield]][stars-url]
[![Issues][issues-shield]][issues-url]
<!-- [![MIT License][license-shield]][license-url] -->


## Nintendo Entertainment System Emulator

An NES emulator using C++23. It tries to be a cycle accurate emulation of the CPU, PPU and cartridge memory mappers. At this time, NTSC/PAL or 100% accurate audio emulation is not a goal as long as the output still looks reasonable, but I may consider it in the future.

This is very much an in progress project for learning about emulator development and how the NES works. Many features are missing or incomplete. If you are looking for an emulator to play your favorite games, Mesen or FCEUX are much better options. I hope my code can serve as inspiration or ideas for writing your own emulator, and I welcome feedback and suggestions for improvement.


![NES Emulator preview](doc/smb.webp)

## Getting Started

You'll need a very recent C++ compiler. The latest MSVC will work, and I try to keep the code compatible with the latest GCC. Dependencies are handled by vcpkg by default.

On linux, you'll need to make sure appropriate development libraries for X11 and/or wayland are installed for SDL2 to compile and run properly.

### Prerequisites

* [cmake](https://cmake.org/download/)
* a recent c++ compiler

### Build

To clone and build the repo, run:

   ```sh
   git clone https://github.com/oracleoftroy/nesem.git
   cd nesem
   mkdir build
   cmake . -B build
   cmake --build build
   ```

<!-- ## License

Distributed under the MIT License. See `LICENSE.txt` for more information.

<p align="right">(<a href="#top">back to top</a>)</p> -->


Project Link: [https://github.com/oracleoftroy/nesem](https://github.com/oracleoftroy/nesem)


[contributors-shield]: https://img.shields.io/github/contributors/oracleoftroy/nesem.svg?style=flat
[contributors-url]: https://github.com/oracleoftroy/nesem/graphs/contributors
[forks-shield]: https://img.shields.io/github/forks/oracleoftroy/nesem.svg?style=flat
[forks-url]: https://github.com/oracleoftroy/nesem/network/members
[stars-shield]: https://img.shields.io/github/stars/oracleoftroy/nesem.svg?style=flat
[stars-url]: https://github.com/oracleoftroy/nesem/stargazers
[issues-shield]: https://img.shields.io/github/issues/oracleoftroy/nesem.svg?style=flat
[issues-url]: https://github.com/oracleoftroy/nesem/issues
<!-- [license-shield]: https://img.shields.io/github/license/oracleoftroy/nesem.svg?style=flat -->
<!-- [license-url]: https://github.com/oracleoftroy/nesem/blob/master/LICENSE.txt -->
