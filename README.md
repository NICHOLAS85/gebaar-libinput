***This fork merges the following into Gebaar***
- https://github.com/Coffee2CodeNL/gebaar-libinput/pull/10 adding pinch in/out gestures
- https://github.com/Coffee2CodeNL/gebaar-libinput/pull/25 adding error catching to config parse
- https://github.com/gabrielstedman/gebaar-libinput adding touch support

***Other changes include adding a config option which determines whether touchpad or touchscreen gestures are used***

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

### Fork Notes
An additional config option can be added which is listed below. This manually sets whether Gebaar attempts to recognize touchscreen gestures or touchpad gestures:

```toml
[interact.type]
type = ""
```

Options are `"TOUCH"` (touchscreen) or `"GESTURE"` (touchpad). <br>Any other values cause Gebaar to attempt to auto-detect which is provided, which usually falls back to `"GESTURE"` if available.

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

**Switching between touchpad and touchscreen**

Using the following two user systemd units you can have a dedicated configuration file for touchscreen mode and touchpad mode as well as a command to run each when switching.
<details>
  <summary>~/.config/systemd/user/gebaard-laptop.service</summary>

  ```sh
[Unit]
Description=Gebaar Daemon
Documentation=https://github.com/NICHOLAS85/gebaar-libinput
Before=gebaard-tablet.service
Conflicts=gebaard-tablet.service

[Service]
ExecStartPre=/usr/bin/cp -f %h/.config/gebaar/gebaard-laptop.toml %h/.config/gebaar/gebaard.toml
ExecStart=/usr/local/bin/gebaard
Environment=DISPLAY=:0
Restart=always

[Install]
WantedBy=default.target
  ```

</details>
<details>
  <summary>~/.config/systemd/user/gebaard-tablet.service</summary>

```sh
[Unit]
Description=Gebaar Daemon
Documentation=https://github.com/NICHOLAS85/gebaar-libinput
After=gebaard-laptop.service
Conflicts=gebaard-laptop.service

[Service]
ExecStartPre=/usr/bin/cp -f %h/.config/gebaar/gebaard-tablet.toml %h/.config/gebaar/gebaard.toml
ExecStart=/usr/local/bin/gebaard
Environment=DISPLAY=:0
Restart=always

[Install]
WantedBy=default.target
```

</details>

Use the following config files as a basis for your own configuration, it is important ```[interact.type]``` is set to the corresponding value for each config and that they are named like listed.

<details>
  <summary>~/.config/gebaar/gebaard-laptop.toml</summary>

```toml
[[command-swipe]]
fingers = 3
left = "xdotool key alt+Right"
right = "xdotool key alt+Left"

[[command-swipe]]
fingers = 4
up = 'qdbus org.kde.kglobalaccel /component/kwin invokeShortcut "Window Maximize"'
down = 'qdbus org.kde.kglobalaccel /component/kwin invokeShortcut "MinimizeAll"'
left = 'qdbus org.kde.kglobalaccel /component/kwin invokeShortcut "Window Quick Tile Left"'
right = 'qdbus org.kde.kglobalaccel /component/kwin invokeShortcut "Window Quick Tile Right"'

[commands.pinch]
in = "xdotool key ctrl+shift+t"

[interact.type]
type = "GESTURE"
```

</details>

<details>
  <summary>~/.config/gebaar/gebaard-tablet.toml</summary>

```toml
[[command-swipe]]
fingers = 2
up = "dbus-send --type=method_call --dest=org.onboard.Onboard /org/onboard/Onboard/Keyboard org.onboard.Onboard.Keyboard.ToggleVisible"
left = "xdotool key alt+Right"
right = "xdotool key alt+Left"


[[command-swipe]]
fingers = 3
up = 'qdbus org.kde.kglobalaccel /component/kwin invokeShortcut "Expose"'
down = 'qdbus org.kde.kglobalaccel /component/kwin invokeShortcut "Window Minimize"'

[[command-swipe]]
fingers = 4
up = 'qdbus org.kde.kglobalaccel /component/kwin invokeShortcut "Window Maximize"'
down = 'qdbus org.kde.kglobalaccel /component/kwin invokeShortcut "MinimizeAll"'
left = 'qdbus org.kde.kglobalaccel /component/kwin invokeShortcut "Window Quick Tile Left"'
right = 'qdbus org.kde.kglobalaccel /component/kwin invokeShortcut "Window Quick Tile Right"'

[interact.type]
type = "TOUCH"
```

</details>

Once these files are in place simply run the following once to enable and run gebaar automatically at boot.
```sh
$ systemctl --user enable gebaard-laptop gebaard-tablet
```
To switch to touchscreen mode run:
```sh
$ systemctl --user start gebaard-tablet
```
and to switch to touchpad mode run (This mode is started at boot):
```sh
$ systemctl --user start gebaard-laptop
```

These commands can be placed in scripts for example in https://github.com/alesguzik/linux_detect_tablet_mode to automatically switch between modes when switching a convertible laptop.

### Repository versions

![](https://img.shields.io/aur/version/gebaar.svg?style=flat)  


### State of the project

- [x] Receiving swipe events from libinput
- [x] Receiving touch swipe events from libinput
- [x] Receiving pinch/zoom events from libinput
- [ ] Receiving touch pinch/zoom events from libinput
- [ ] Receiving rotation events from libinput
- [ ] Receiving touch rotation events from libinput
- [ ] Separate config files for touchpad and touchscreen
- [x] Converting libinput events to motions
- [x] Running commands based on motions
- [x] Refactor code to be up to Release standards, instead of testing-hell
