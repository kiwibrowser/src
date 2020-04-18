## Building on Linux (Ubuntu Lucid x86\_64)

*   Check out the Native Client sources using gclient
*   Uninstall Automake and Autoconf (they are not required and are likely the
    wrong version)
*   On Ubuntu Lucid: `sudo apt-get install gcc-multilib g++-multilib
    libsdl1.2-dev texinfo libcrypto++-dev libssl-dev lib32ncurses5-dev m4
    libelf-dev
    `
*   [Build the x86 toolchain]
    (http://www.chromium.org/nativeclient/how-tos/building-and-testing-gcc-and-gnu-binutils)
