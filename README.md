[![Contributors][contributors-shield]][contributors-url]
[![Forks][forks-shield]][forks-url]
[![Stargazers][stars-shield]][stars-url]
[![Issues][issues-shield]][issues-url]
<!-- [![MIT License][license-shield]][license-url] -->


## Nintento Entertainment System Emulator

An NES emulator using C++23.

This is very much an in progress learning project for me, and is missing many features. If you are looking for an emulator to play your favorite games, Mesen or FCEUX are much better options.

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


[contributors-shield]: https://img.shields.io/github/contributors/oracleoftroy/nesem.svg?style=for-the-badge
[contributors-url]: https://github.com/oracleoftroy/nesem/graphs/contributors
[forks-shield]: https://img.shields.io/github/forks/oracleoftroy/nesem.svg?style=for-the-badge
[forks-url]: https://github.com/oracleoftroy/nesem/network/members
[stars-shield]: https://img.shields.io/github/stars/oracleoftroy/nesem.svg?style=for-the-badge
[stars-url]: https://github.com/oracleoftroy/nesem/stargazers
[issues-shield]: https://img.shields.io/github/issues/oracleoftroy/nesem.svg?style=for-the-badge
[issues-url]: https://github.com/oracleoftroy/nesem/issues
<!-- [license-shield]: https://img.shields.io/github/license/oracleoftroy/nesem.svg?style=for-the-badge -->
<!-- [license-url]: https://github.com/oracleoftroy/nesem/blob/master/LICENSE.txt -->
