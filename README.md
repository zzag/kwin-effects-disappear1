![Slow motion](demo/slow-motion.gif)

"Disappear 1" is a KWin effect that animates the disappearing of windows.

## Installation

### Arch Linux

For Arch Linux [kwin-effects-disappear1](https://aur.archlinux.org/packages/kwin-effects-disappear1/)
is available in the AUR.

### Fedora

```sh
sudo dnf copr enable zzag/kwin-effects
sudo dnf refresh
sudo dnf install kwin-effects-disappear1
```

### Ubuntu

```sh
sudo add-apt-repository ppa:vladzzag/kwin-effects
sudo apt install libkwin4-effect-disappear1
```

### From source

```sh
git clone https://github.com/zzag/kwin-effects-disappear1.git
cd kwin-effects-disappear1
mkdir build && cd build
cmake ..
make -jN
sudo make install
```
