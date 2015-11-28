# soundscope #

`soundscope`, an X-Y oscilloscope, was written by Rusty Wagner and contains very minor additions (plus an example
song) by Alexander Taylor.

## Building ##

The following describes build requirements for each platform. Once the requirements are satisfied, you should
simply need to run `make` to build `soundscope`.

### OS X ###

To build on OS X, you'll need the OS X Developer Tools from Xcode. If you have Xcode installed, you can force the
installation of the command-line utilities by doing `xcode-select --install`. Specifically, you'll need `clang`
and the OpenGL framework.

You will also need SDL. The easiest way to obtain this is through [Homebrew](http://brew.sh/). Once you have
Homebrew installed, simply do `brew install sdl`.

### Linux ###

To build on Linux (and probably any other Unix-like OS), you'll need a C++ compiler (`g++` recommended), OpenGL
libraries, and SDL. If you're on a Debian-based system, the following should install what you need:

```
sudo apt-get install build-essential mesa-common-dev libsdl1.2-dev
```

## Running ##

You should be able to simply run `soundscope` on the command-line with any WAV or RAW audio file. An example
has been provided with the repository:

```
./soundscope alpha_molecule.wav
```
