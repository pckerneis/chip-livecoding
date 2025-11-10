# Chip-Livecoding

A minimalistic musical livecoding environment designed to run on resource-constrained platforms such as the PocketCHIP.

Inspired on Bytebeat, it uses a simple audio synthesis approach based on a single audio generator function.

The generator function outputs a value in the range of [-1, 1] for each sample, which is then converted to a sound wave and played back. It's written in Lua.

The environment is designed to be simple and easy to use, with a focus on minimalism and accessibility.

## Installation

### Prerequisites

- C compiler (gcc or clang)
- Lua 5.1 development files
- PortAudio development files
- libsndfile development files

On Debian/Ubuntu, you can install them with:

```bash
sudo apt-get install build-essential lua5.1 liblua5.1-dev portaudio19-dev libsndfile1-dev
```

### Building

1. Clone the repository:

```bash
git clone https://github.com/pckerneis/chip-livecoding.git
cd chip-livecoding
```

2. Build the project:

```bash
make
```

3. Install (optional):

```bash
sudo make install
```

This will install the `chip-livecoding` executable to `/usr/local/bin`.

## Usage

```bash
chip-livecoding <script.lua>
```

This will run the script and play the audio output.

To stop the script, press Ctrl+C.

When the script is running, you can edit it and the changes will be applied automatically.

## Examples

### Sine

Create a file called `sine.lua` with the following content:

```lua
return sin(t, 440)
```

Run it with:

```bash
chip-livecoding sine.lua
```

## API

Chip-Livecoding provides a simple API for audio synthesis.

### spl

The `spl(path, pos)` function returns a sample value in the range of [-1, 1] from an audio file.

```lua
spl("/path/to/audio/file.wav", t) -- returns a sample value in the range of [-1, 1] from the audio file at position t
```

### sin

The `sin(pos, freq, phase)` function returns a sine wave value in the range of [-1, 1].

```lua
sin(pos, freq, phase) -- returns a sine wave value in the range of [-1, 1]
```

The `phase` parameter is optional and defaults to 0.

### saw

The `saw(pos, freq, phase)` function returns a sawtooth wave value in the range of [-1, 1].

```lua
saw(pos, freq, phase) -- returns a sawtooth wave value in the range of [-1, 1]
```

The `phase` parameter is optional and defaults to 0.

### sq

The `sq(pos, freq, phase)` function returns a square wave value in the range of [-1, 1].

```lua
sq(pos, freq, phase) -- returns a square wave value in the range of [-1, 1]
```

The `phase` parameter is optional and defaults to 0.

### tri

The `tri(pos, freq, phase)` function returns a triangle wave value in the range of [-1, 1].

```lua
tri(pos, freq, phase) -- returns a triangle wave value in the range of [-1, 1]
```

The `phase` parameter is optional and defaults to 0.

### rnd

The `rnd()` function returns a random float value in the range of [0, 1].

```lua
rnd() -- returns a random float value in the range of [0, 1]
```

### rndf

The `rndf(max)` function returns a random float value in the range of [0, max].

```lua
rndf(max) -- returns a random float value in the range of [0, max]
```

A max value of 1 is used by default.

The alternative signature `rndf(min, max)` returns a random float value in the range of [min, max].

```lua
rndf(min, max) -- returns a random float value in the range of [min, max]
```

### rndi

The `rndi(max)` function returns a random integer value in the range of [0, max].

```lua
rndi(max) -- returns a random integer value in the range of [0, max]
```

The alternative signature `rndi(min, max)` returns a random integer value in the range of [min, max].
