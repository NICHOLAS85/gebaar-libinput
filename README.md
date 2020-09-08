_Forked from Coffee2CodeNL/gebaar-libinput since original repo has been unmaintained for over a year, yet this is NOT OFFICIAL repo!_

***This fork merges the following into Gebaar***
- https://github.com/gabrielstedman/gebaar-libinput adding touch support and pinch in/out gestures
- https://github.com/Coffee2CodeNL/gebaar-libinput/pull/25 adding error catching when parsing options

***Other changes include:***
- Allowing different commands to be run depending on touchpad or touchscreen gestures
- Adding support to run commands on switch events for 2 in 1 laptops
- Adding support for pinch and rotate gestures
- Improve configuration specificity and options

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

### Configuration

```toml
[[swipe.commands]]
fingers =    integer (default 3)
type =       string (TOUCH|GESTURE) (default GESTURE)
# Entries below run string based on direction
left_up =    string
right_up =   string
up =         string
left_down =  string
right_down = string
down =       string
left =       string
right =      string

[[pinch.commands]]
fingers =      integer (default 2)
type =         string (ONESHOT|CONTINUOUS) (default ONESHOT)
# Entries below run string based on direction
in =           string
out =          string
rotate_left =  string
rotate_right = string

[switch.commands]
# Entries below run string when 2 in 1 devices switch modes
laptop = string
tablet = string

[settings]
pinch.threshold = double (default 0.25)
rotate.threshold = double (default 20)
interact.type =   string (TOUCH|GESTURE|BOTH) (default automatic)
gesture_swipe.threshold = double (default 0.5)
gesture_swipe.one_shot =  bool (default true)
gesture_swipe.trigger_on_release =        bool (default true)
touch_swipe.longswipe_screen_percentage = double (default 70)
```
* `swipe.commands.type` key determines whether gestures in current array are triggered by touchscreen (TOUCH) or trackpad (GESTURE) gestures.
* `pinch.commands.type` key determines if a pinch is triggered once (ONESHOT) or continuously (CONTINUOUS) as fingers get closer or farther apart.
* `settings.pinch.threshold` key sets the distance between fingers where it should trigger.
  Defaults to `0.25` which means fingers should travel exactly 25% distance from their initial position.
* `settings.rotate.threshold` key sets angle between fingers where it should trigger.
  Defaults to `20` which means fingers must travel 20 degrees from their initial position.
* `interact.type` key determines whether touchscreen (TOUCH) or trackpad (GESTURE) gestures are detected. In 2 and 1 devices, this key is set automatically depending on what mode the device is currently in, BOTH supersedes this behavior.
* `settings.gesture_swipe.threshold` sets the percentage fingers should travel to trigger a swipe.
* `settings.gesture_swipe.one_shot` key determines whether gestures are triggered once (ONESHOT) or continuously (CONTINOUS) as fingers travel across the trackpad.
* `settings.touch_swipe.longswipe_screen_percentage` key determines percentage of a screen dimension a swipe must cover to be
  interpreted as a longswipe. Only for 'fingers = 1'.

### Repository versions

![](https://img.shields.io/aur/version/gebaar.svg?style=flat)  

### Examples

**bspwm**
<details>
<summary>~/.config/gebaar/gebaard.toml</summary>

```toml
[[swipe.commands]]
fingers = 3
left_up = ""
right_up = ""
up = "bspc node -f north"
left_down = ""
right_down = ""
down = "bspc node -f south"
left = "bspc node -f west"
right = "bspc node -f east"

[[swipe.commands]]
fingers = 4
left_up = ""
right_up = ""
up = "rofi -show combi"
left_down = ""
right_down = ""
down = ""
left = "bspc desktop -f prev"
right = "bspc desktop -f next"

[[swipe.commands]]
fingers = 1
left_up = ""
right_up = ""
up = ""
left_down = ""
right_down = ""
down = "echo long_swipe_down"
left = ""
right = ""

[pinch.commands]
type = "ONESHOT"
in = "xdotool key Control_L+equal"
out = "xdotool key Control_L+minus"

[settings.pinch]
threshold=0.25

[settings.gesture_swipe]
threshold = 0.5
one_shot = true
trigger_on_release = false

[settings.touch_swipe]
longswipe_screen_percentage = 95
```

Add `gebaard -b` to `~/.config/bspwm/bspwmrc`
</details>

**kde**
<details>
<summary>~/.config/gebaar/gebaard.toml</summary>

```toml
[[swipe.commands]]
up = "~/bin/presentview --up"
down = "~/bin/presentview --down"
left = "xdotool key alt+Right"
right = "xdotool key alt+Left"

[[swipe.commands]]
fingers = 4
up = 'qdbus org.kde.kglobalaccel /component/kwin invokeShortcut "Window Maximize"'
down = 'qdbus org.kde.kglobalaccel /component/kwin invokeShortcut "MinimizeAll"'
left = 'qdbus org.kde.kglobalaccel /component/kwin invokeShortcut "Window Quick Tile Left"'
right = 'qdbus org.kde.kglobalaccel /component/kwin invokeShortcut "Window Quick Tile Right"'

[[swipe.commands]]
fingers = 2
type = "TOUCH"
up = "dbus-send --type=method_call --dest=org.onboard.Onboard /org/onboard/Onboard/Keyboard org.onboard.Onboard.Keyboard.ToggleVisible"
left = "xdotool key alt+Right"
right = "xdotool key alt+Left"


[[swipe.commands]]
type = "TOUCH"
up = 'qdbus org.kde.kglobalaccel /component/kwin invokeShortcut "Expose"'
down = 'qdbus org.kde.kglobalaccel /component/kwin invokeShortcut "Window Minimize"'

[[swipe.commands]]
fingers = 4
type = "TOUCH"
up = 'qdbus org.kde.kglobalaccel /component/kwin invokeShortcut "Window Maximize"'
down = 'qdbus org.kde.kglobalaccel /component/kwin invokeShortcut "MinimizeAll"'
left = 'qdbus org.kde.kglobalaccel /component/kwin invokeShortcut "Window Quick Tile Left"'
right = 'qdbus org.kde.kglobalaccel /component/kwin invokeShortcut "Window Quick Tile Right"'

[[pinch.commands]]
type = "ONESHOT"
in = "~/bin/firefoxorbust"
out = "xdotool key ctrl+shift+t"

[[pinch.commands]]
type = "CONTINUOUS"
rotate_left = 'qdbus org.kde.kglobalaccel /component/kmix invokeShortcut "increase_volume"'
rotate_right = 'qdbus org.kde.kglobalaccel /component/kmix invokeShortcut "decrease_volume"'

[switch.commands]
laptop = "pkill onboard; pkill screenrotator;"
tablet = "onboard & screenrotator &"

[settings]
pinch.threshold = 0.13
rotate.threshold = 20
interact.type = "BOTH"

[settings.gesture_swipe]
threshold = 0.7
trigger_on_release = false
```
</details>

**Starting and managing Gebaar with Systemd**
<details>
<summary>~/.config/systemd/user/gebaard.service</summary>

```ini
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
$ systemctl --user --now enable gebaard
```
</details>

<details>
<summary>~/.config/systemd/user/gebaard-watcher.service</summary>

```ini
[Unit]
Description=Gebaar restarter

[Service]
Type=oneshot
ExecStart=/usr/bin/systemctl --user is-active --quiet gebaard && /usr/bin/systemctl --user restart gebaard.servie
SuccessExitStatus=0 3

[Install]
WantedBy=default.target
```

Once the file is in place simply run the following
```sh
$ systemctl --user enable gebaard-watcher
```
</details>

<details>
<summary>~/.config/systemd/user/gebaard-watcher.path</summary>

```ini
[Path]
PathModified=%h/.config/gebaar/gebaard.toml

[Install]
WantedBy=default.target
```

Once the file is in place simply run the following to automatically restart Gebaar when it's configuration file is modified.
```sh
$ systemctl --user enable gebaard-watcher.path
```
</details>

### State of the project

- [x] Receiving swipe events from libinput
- [x] Swipe gesture have trigger threshold
- [x] Receiving pinch/zoom events from libinput
- [x] Support continuous pinch
- [x] Support pinch-and-rotate gestures
- [x] Support touchscreen devices
- [ ] Receiving rotation events from libinput
- [x] Receiving switch events from libinput
- [x] Converting libinput events to motions
- [x] Running commands based on motions
- [x] Refactor code to be up to Release standards, instead of testing-hell

