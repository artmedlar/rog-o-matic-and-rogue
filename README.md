# Rog-O-Matic XIV + Rogue-Clone for macOS

This repository contains Rog-O-Matic XIV, the legendary automatic Rogue player from CMU (1985), modified to work with rogue-clone on modern macOS (including Apple Silicon).

## What is Rog-O-Matic?

Rog-O-Matic is an AI that plays the classic dungeon-crawling game Rogue. It was developed at Carnegie Mellon University in the mid-1980s and became the first program to achieve "Total Winner" status in Rogue. It features:

- **Genetic learning algorithm** that evolves better strategies over time
- **Long-term memory** that remembers monster danger levels across games
- **Autonomous play** - watches the screen and makes decisions like a human player

## Source Origins

Both original source archives are available from [The Roguelike Archive](https://britzl.github.io/roguearchive/):

| File | Description |
|------|-------------|
| `rgm14.tar.Z` | Original Rog-O-Matic XIV source (CMU, 1985) |
| `rogue-libc5-ncurses.zip` | Rogue 5.3 Clone source (Tim Stoehr) |

## Directory Structure

```
rogue/           - Rogue-clone game source
rgm14/           - Rog-O-Matic XIV source  
rgm14/lib/       - Learning data (gene pool, long-term memory)
```

## Build Instructions

### Prerequisites (macOS)

```bash
# Install Xcode command line tools
xcode-select --install

# ncurses is included with macOS
```

### Build Both Projects

```bash
# From the top-level directory
make

# Or clean and rebuild
make clean
make
```

### Build Individually

```bash
# Rogue only
make rogue

# Rog-O-Matic only  
make rogomatic
```

## Running Rog-O-Matic

### Basic Usage

```bash
cd rgm14

# Watch mode - see Rog-O-Matic play in real-time
./rogomatic -w

# Watch mode with speed limit (5 moves per second)
./rogomatic -w -m 5

# Watch mode with debug logging
./rogomatic -wD
# Debug log written to /tmp/rgm.log
```

### Learning Loop

Run multiple games to train the genetic algorithm:

```bash
# Run 100 games at 10 moves/sec
./learn_loop.sh 100 10

# Run 1000 games at full speed (background)
./learn_loop.sh 1000 0 &
```

### Check Learning Progress

```bash
# View game count and scores
cat lib/ltm531

# View recent genetic activity  
tail -20 lib/GeneLog531

# Monitor in real-time
watch -n 5 'grep "^Count" lib/ltm531; tail -5 lib/GeneLog531'
```

### Command Line Options

| Option | Description |
|--------|-------------|
| `-w` | Watch mode - display the game |
| `-m N` | Limit to N moves per second |
| `-D` | Enable debug logging to /tmp/rgm.log |
| `-e` | Echo game log (for replay) |
| `-s` | Show scores only |
| `-h` | No halftime show |

## Changes Made for macOS Compatibility

### Rogue-Clone Changes

1. **Makefile updates** for modern clang/LLVM
2. **Fixed function prototypes** for C99 compatibility
3. **ncurses linking** - uses system ncurses

### Rog-O-Matic Changes

The original 1985 code required significant modifications to work with modern macOS and rogue-clone:

#### 1. Terminal/PTY Communication (`setup.c`, `io.c`)

- Use `forkpty()` for pseudo-terminal creation (replaces obsolete PTY methods)
- Set `TERMCAP` environment variable for rogue's terminal expectations
- Unbuffered I/O with `setvbuf()` for responsive communication

#### 2. Rogue-Clone Version Detection (`io.c`)

```c
// Added rogue-clone as version RV53A
else if (stlmatch (versionstr, "Clone")) version = RV53A;
```

#### 3. Movement Commands (`command.c`)

Rogue-clone uses different run commands than original Rogue:
- Original: `f` + direction = run
- Rogue-clone: `f` + direction = fight (attack)
- Fixed by setting `version = RV53A` which uses Ctrl+direction for running

#### 4. Prompt Handling (`io.c`)

Rogue-clone uses different prompts than original Rogue:

| Prompt | Original Rogue | Rogue-Clone |
|--------|----------------|-------------|
| More | `--More--` | `-more-` (lowercase) |
| Continue | N/A | `--press space to continue--` |
| Inventory | Shows and waits | Shows and waits (no prompt text) |

Added handlers for each:
```c
// Handle "-more-" prompt
if (ch == *rmore) { ... sendnow(" "); ... }

// Handle "continue--" prompt  
if (ch == *cont) { ... sendnow(" "); sendnow("%c", 12); ... }
```

#### 5. Timeout Mechanism (`io.c`)

Original code blocked forever waiting for rogue input. Added timeout:

```c
static char getroguechar_timeout() {
  // Use select() with 100ms timeout
  // Use raw read() instead of fgetc() to avoid stdio buffering issues
  // On timeout, send space to dismiss any waiting prompt
}
```

This prevents hangs during:
- "Recovering from severe beating" (rest)
- Inventory display
- Any screen waiting for acknowledgment

#### 6. Halftime Show Disabled (`titlepage.c`)

The animated title screen clears curses display but rogue-clone doesn't redraw. Disabled for compatibility.

#### 7. K&R Function Definitions (various files)

Kept original K&R-style function definitions for variadic functions (`sendnow`, `saynow`, `dwait`) - converting to ANSI varargs caused crashes on arm64 due to calling convention differences.

#### 8. Terminal Cleanup (`main.c`)

Added proper terminal reset on game exit:
```c
printf("\033[H\033[J");  // Clear screen
system("stty sane");     // Reset terminal
```

## Learning System

Rog-O-Matic uses two learning mechanisms:

### Long-Term Memory (ltm.c)
- Remembers monster danger levels
- Tracks what killed the player
- Persists in `lib/ltm531`

### Genetic Algorithm (learn.c, gene.c)
- Maintains a pool of "genotypes" with different strategy weights
- Each game uses one genotype
- Successful genotypes breed to create offspring
- Mutations introduce variation
- Persists in `lib/GenePool531`

## Troubleshooting

### Game hangs
The timeout mechanism should prevent hangs. If stuck:
```bash
killall rogue player Rgm
```

### Check debug log
```bash
./rogomatic -wD -m 5
# In another terminal:
tail -f /tmp/rgm.log
```

### Reset learning data
```bash
rm lib/ltm531 lib/GenePool531 lib/GeneLog531
# Fresh start with default gene pool
```

## Credits

- **Rog-O-Matic XIV** (1985): Andrew Appel, Leonard Hamey, Guy Jacobson, Michael Mauldin - Carnegie Mellon University
- **Rogue-Clone**: Tim Stoehr and the Roguelike Restoration Project
- **macOS Port** (2026): Modifications for modern macOS/arm64 compatibility

## License

Rog-O-Matic: Copyright (C) 1985 by A. Appel, G. Jacobson, L. Hamey, and M. Mauldin

Rogue-Clone: Public domain / BSD-style license

## References

- [Rog-O-Matic: A Belligerent Expert System](https://www.cs.cmu.edu/~mmv/papers/RogOMatic.pdf) - Original paper
- [Rogue (video game) - Wikipedia](https://en.wikipedia.org/wiki/Rogue_(video_game))

