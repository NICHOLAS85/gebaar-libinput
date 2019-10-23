***This fork merges the following into Gebaar***
- https://github.com/Coffee2CodeNL/gebaar-libinput/pull/10 adding pinch in/out gestures
- https://github.com/Coffee2CodeNL/gebaar-libinput/pull/25 adding error catching to config parse
- https://github.com/gabrielstedman/gebaar-libinput adding touch support

***Other changes include:***
- Allowing different commands to be run depending on touchpad or touchscreen mode
- Adding a config option which determines the initial mode gebaard runs in
- Adding support for switch events for 2 in 1 laptops

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
[[command-swipe-gesture]]
fingers = 3
left_up = ""
right_up = ""
up = ""
left_down = ""
right_down = ""
down = ""
left = ""
right = ""

[[command-swipe-gesture]]
fingers = 4
left_up = ""
right_up = ""
up = ""
left_down = ""
right_down = ""
down = ""
left = ""
right = ""

[[command-swipe-touch]]
fingers = 3
left_up = ""
right_up = ""
up = ""
left_down = ""
right_down = ""
down = ""
left = ""
right = ""

[[command-swipe-touch]]
fingers = 4
left_up = ""
right_up = ""
up = ""
left_down = ""
right_down = ""
down = ""
left = ""
right = ""

[commands-pinch]
in = ""
out = ""

[command-switch]
laptop = ""
tablet = ""
```

### Fork Notes
Additional config options can be added which are listed below:

```toml
[interact-type]
type = ""
```

This manually sets whether Gebaar attempts to recognize touchscreen gestures or touchpad gestures on startup. It is overwritten by the real mode when a switch event occurs. </br>Options are `"TOUCH"` (touchscreen) or `"GESTURE"` (touchpad). <br>Any other values cause Gebaar to attempt to auto-detect which is provided, which usually falls back to `"GESTURE"` if available.

```toml
[[command-swipe-gesture]]
fingers =

[[command-swipe-touch]]
fingers =
```
`command-swipe-gesture` commands are run on detected touchpad gestures. `command-swipe-touch` commands are run on detected touchscreen gestures. Gebaar only runs in one mode at a time but automatically detects switches, changing modes accordingly.
`fingers` option can be 2 or greater

```toml
[command-switch]
laptop = ""
tablet = ""
```
Sets what commands to run when computer switches from laptop to tablet mode. Programs which do not fork must be backgrounded or else gebaar will hang until the program exits. ie `tablet = "onboard &"`


### Examples

**bspwm**

_~/.config/gebaar/gebaard.toml_
```toml
[[command-swipe-gesture]]
fingers = 3
up = "bspc node -f north"
down = "bspc node -f south"
left = "bspc node -f west"
right = "bspc node -f east"

[[command-swipe-gesture]]
fingers = 4
up = "rofi -show combi"
down = ""
left = "bspc desktop -f prev"
right = "bspc desktop -f next"
```

Add `gebaard -b` to `~/.config/bspwm/bspwmrc`

**Starting gebaar at boot with Systemd**

_~/.config/systemd/user/gebaard.service_
```sh
[Unit]
Description=Gebaar Daemon
Documentation=https://github.com/NICHOLAS85/gebaar-libinput

[Service]
ExecStart=/usr/local/bin/gebaard
Environment=DISPLAY=:0
Restart=always

[Install]
WantedBy=default.target
```

Once the file is in place simply run the following once to enable and run gebaar automatically.
```sh
$ systemctl --user enable gebaard; systemctl --user start gebaard
```

### Repository versions

![](https://img.shields.io/aur/version/gebaar.svg?style=flat)  


### State of the project

- [x] Receiving swipe events from libinput
- [x] Receiving touch swipe events from libinput
- [x] Receiving pinch/zoom events from libinput
- [ ] Receiving touch pinch/zoom events from libinput
- [ ] Receiving rotation events from libinput
- [ ] Receiving touch rotation events from libinput
- [ ] Adjusting swipe events based on device orientation
- [x] Separate config options for touchpad and touchscreen gestures
- [x] Receiving switch events from libinput
- [x] Converting libinput events to motions
- [x] Running commands based on motions
- [x] Refactor code to be up to Release standards, instead of testing-hell
