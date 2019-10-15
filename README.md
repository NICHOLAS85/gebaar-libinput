***This fork merges https://github.com/Coffee2CodeNL/gebaar-libinput/pull/10 adding pinch in/out gestures, https://github.com/Coffee2CodeNL/gebaar-libinput/pull/25 adding error catching to config parse, and https://github.com/gabrielstedman/gebaar-libinput adding touch support***

Gebaar
=========

WM Independent Touchpad Gesture Daemon for libinput

_Gebaar means Gesture in Dutch_

Run any command by simply gesturing on your touchpad!

### What makes this different over the other implementations?

[libinput-gestures](https://github.com/bulletmark/libinput-gestures) and [fusuma](https://github.com/iberianpig/fusuma) both parse the output of the shell command `libinput debug-events` which is an unstable API and the output just keeps coming, so it'll eat (some) RAM.

Gebaar directly interfaces with libinput to receive and react to the events.   
This is more stable, faster, and more efficient as it **does not parse the output of a program** like the aforementioned projects do.

### Getting support (or talking about the project's future)

Click to join: [![Discord](https://img.shields.io/discord/548978799136473106.svg?label=Discord)](https://discord.gg/9mbKhFR)

### How to build and install

1. Clone the repository via `git clone https://github.com/NICHOLAS85/gebaar-libinput`
2. Run `git submodule update --init` in the root folder
3. Run `mkdir build && cd build`
4. Run `cmake ..`
5. Run `make -j$(nproc)`
6. Run `sudo make install` to install
7. Run `mkdir -p ~/.config/gebaar`
8. Run `nano ~/.config/gebaar/gebaard.toml` (or vim, if you like it better)
9. Add the snippet below to `gebaard.toml`
10. Configure commands to run per direction
11. Add yourself to the `input` group with `usermod -a -G input $USER`
12. Run Gebaar via some startup file by adding `gebaard -b` to it
13. Reboot and see the magic

```toml
[[command-swipe]]
fingers = 3
left_up = ""
right_up = ""
up = ""
left_down = ""
right_down = ""
down = ""
left = ""
right = ""

[[command-swipe]]
fingers = 4
left_up = ""
right_up = ""
up = ""
left_down = ""
right_down = ""
down = ""
left = ""
right = ""

[commands.pinch]
in = ""
out = ""
```

### Repository versions

![](https://img.shields.io/aur/version/gebaar.svg?style=flat)  

### Examples

**bspwm**

_~/.config/gebaar/gebaard.toml_
```toml
[[command-swipe]]
fingers = 3
up = "bspc node -f north"
down = "bspc node -f south"
left = "bspc node -f west"
right = "bspc node -f east"

[[command-swipe]]
fingers = 4
up = "rofi -show combi"
down = ""
left = "bspc desktop -f prev"
right = "bspc desktop -f next"
```

Add `gebaard -b` to `~/.config/bspwm/bspwmrc`

### State of the project

- [x] Receiving swipe events from libinput
- [x] Receiving pinch/zoom events from libinput
- [ ] Receiving rotation events from libinput
- [x] Converting libinput events to motions
- [x] Running commands based on motions
- [x] Refactor code to be up to Release standards, instead of testing-hell
