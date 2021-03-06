# Toxic [![Build Status](https://travis-ci.org/Tox/toxic.png?branch=master)](https://travis-ci.org/Tox/toxic)
Toxic is a [Tox](https://tox.im)-based instant messenging client which formerly resided in the [Tox core repository](https://github.com/irungentoo/toxcore), and is now available as a standalone application.

![Toxic Screenshot](https://i.imgur.com/ryaEmQZ.png "Home Screen").

## Installation

### Dependencies
##### Base
* [libtoxcore](https://github.com/irungentoo/toxcore)
* [ncurses](https://www.gnu.org/software/ncurses) (for Debian based systems, 'libncursesw5-dev')
* [libconfig](http://www.hyperrealm.com/libconfig) (for Debian based systems, 'libconfig-dev')

##### Audio
* libtoxav ([libtoxcore](https://github.com/irungentoo/toxcore) compiled with audio support)
* [openal](http://openal.org) (for Debian based systems, 'libopenal-dev')

##### Sound notifications
* [openal](http://openal.org) (for Debian based systems, 'libopenal-dev')
* [openalut](http://openal.org) (for Debian based systems, 'libalut-dev')

##### Desktop notifications
* [libnotify](https://developer.gnome.org/libnotify) (for Debian based systems, 'libnotify-dev')

### Compiling
1. `cd build/`
2. `make PREFIX="/where/to/install"`
3. `sudo make install PREFIX="/where/to/install"`

### Compilation Notes
* You can add specific flags to the Makefile with `USER_CFLAGS=""` and/or `USER_LDFLAGS=""`
* You can pass your own flags to the Makefile with `CFLAGS=""` and/or `LDFLAGS=""` (this will supersede the default ones)
* Additional features are automatically enabled if all dependencies are found, but you can disable them by using special variables:
  * `DISABLE_AV=1` → build toxic without audio call support
  * `DISABLE_SOUND_NOTIFY=1` → build toxic without sound notifications support
  * `DISABLE_DESKTOP_NOTIFY=1` → build toxic without desktop notifications support

### Packaging
* For packaging purpose, you can use `DESTDIR=""` to specify a directory where to store installed files
* `DESTDIR=""` can be used in addition to `PREFIX=""`:
  * `DESTDIR=""` is meant to specify a directory where to store installed files (ex: "/tmp/build/pkg")
  * `PREFIX=""` is meant to specify a prefix directory for binaries and data files (ex: "/usr/local")

### Troubleshooting
If your default prefix is "/usr/local" and you receive the following:
```
error while loading shared libraries: libtoxcore.so.0: cannot open shared object file: No such file or directory
```
you can attempt to correct it by running `sudo ldconfig`. If that doesn't work, run:
```
echo '/usr/local/lib/' | sudo tee -a /etc/ld.so.conf.d/locallib.conf
sudo ldconfig
```

## Precompiled binaries
You can download precompiled binaries from [jenkins](https://jenkins.libtoxcore.so):
* [Linux 32 bit](https://jenkins.libtoxcore.so/job/toxic_linux_i386/lastSuccessfulBuild/artifact/toxic_linux_i386.tar.xz)
* [Linux 64 bit](https://jenkins.libtoxcore.so/job/toxic_linux_amd64/lastSuccessfulBuild/artifact/toxic_linux_amd64.tar.xz)

## Settings
Running Toxic for the first time creates an empty file called toxic.conf in your home configuration directory ("~/.config/tox" for Linux users). Adding options to this file allows you to enable auto-logging, change the time format (12/24 hour), and much more.
You can view our example config file [here](misc/toxic.conf.example).

