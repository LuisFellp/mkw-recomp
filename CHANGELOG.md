# Changelog

Each version's section is that release's notes: `tools/build/notes.py` puts it into `docs/release-template.md` (install,
updating, known issues, reporting) for the GitHub release. A section has a summary paragraph, `### Changes` (each with
its issue, #N), `### Tested` (what was run from the packages before publishing) and optionally `### Known issues`.

## 0.1.7-luis.1

Unofficial build of `main` (784a87d, after 0.1.7) with three open pull requests merged, for testing the GPU hang on
AMD cards. Not an upstream release.

### Changes

- Merged #68: the texture pool preallocation is capped at 8 blocks (2 GiB) instead of half the VRAM.
- Merged #80: `MKW_RTSTATS` and `MKW_DUMP_DRAWS`, diagnostics for rendering bugs that log no error.
- Merged #81: `MKW_PIPE_TRACE` names the shader of each pipeline library before it compiles.
- Includes `main` after 0.1.7: the FSR image resize waits for the device under the queue lock, and the shader
  translation fixes (FMUL scale, pixel shader colour targets, MEMBAR/HADD2.SAT, FSWZADD).

### Tested

Built by CI from the merge, with the Linux AppImage and the Windows zip; the setup was not run from these packages
yet.

## 0.1.7

The game starts on AMD graphics cards under Windows, where the window did not open or the title froze.

### Changes

- On AMD's Windows drivers the window did not open after Play, or opened minutes later and froze on the title (#36,
  #38, #54, #20). Some of the game's shaders hold a `switch`, and our translation gave every instruction of them an
  entry point; AMD's compiler took minutes on those (with AMD's own offline compiler, 50 of 81 did not finish in 30 s;
  now none, 84 s for the 81). The translation now takes the switch's own cases. After updating, the setup's shader
  step runs again. Also: a shader the driver does not return holds the game at most 30 s, once, and the start no
  longer waits minutes for the whole prewarm. `GPL=0` in `mkw.ini` is not needed: remove it.
- The window opens black at once instead of after the first image (#36, #20).
- A crash in a race, `no function at guest pc main+0x35bdf8` (#40): the recompiler took an address loaded into a
  register that was overwritten in between, and split a game function in 67 pieces.
- A crash after choosing the character with a 4096 shadow map at 1440p (#48): when that map does not fit, it keeps
  the game's size.
- 2160p in Graphics > Resolution (#55).
- A 64-bit integer compare in 6 shaders was done on 32 bits.
- Diagnostics for graphics-card crashes: the log names the clear, copy or blit after the frame's last pass (#43), and
  `GPU_CRUMBS_DRAW=1` in `mkw.ini` names the shader of each draw (#37, #40, NVIDIA's "instruction pointer invalid").

### Tested

From these packages, before publishing:

- The whole setup on Linux (Radeon 660M, RADV) and on Windows 11 (GTX 1050 Ti), with its one-minute warm-up and no
  shader left behind. We have no AMD card on Windows: the shader fix was checked with AMD's own offline compiler.

## 0.1.6

Graphics cards with more video memory now get more of it for the course's textures.

### Changes

- On a graphics card with lots of video memory the course's ground came out flat and some textures plain (#31): the
  memory for the large tiled textures was capped at 768 MiB on every PC, the figure of the integrated GPU we develop
  on. It now comes from the card: a quarter of its video memory, from 768 MiB to 3 GiB (an 8 GB card gets 1792 MiB;
  uncapped, the title screen took 1472 MiB in our tests). Integrated GPUs keep 768 MiB. `SPARSE_MIB` in `mkw.ini` still sets it by hand.
- `GPU_MEM_LIMIT` in `mkw.ini` on a dedicated graphics card capped the video memory at a small share of it (#28: 6400
  left an 8 GB card 1040 MiB), since it was split with the system memory: it now caps the video memory alone. Without
  it, an integrated GPU on a PC with more than 16 GB of RAM may use a quarter of it instead of 4 GiB.
- `GPU_MEM_LIMIT`, `SPARSE_MIB` and `VRAM_MARGIN` in `mkw.ini` no longer log "unknown key".

### Tested

From these packages, before publishing:

- The whole setup on Linux (Radeon 660M, RADV) and the warm-up step on Windows 11 (GTX 1050 Ti), where the tile memory
  stays at 768 MiB as before (a 4 GB card). The caps for larger cards are checked by the tests on the GPUs players
  reported; we have no card of 6 GB or more to run them on.

## 0.1.5

Local multiplayer, the game no longer runs in slow motion below 60 fps, and the start screen now updates itself.

### Changes

- Local multiplayer with one controller per player (#23): each connected controller is its own player (the first one
  and the keyboard are player 1), the console's controller screen gets the number connected, and rumble goes to each
  player's own controller. Before, Multiplayer went back to the home screen.
- Below 60 fps the race ran in slow motion with Frame rate on Automatic: the game picks 30 or 60 fps from the time the
  GPU reports for each frame, and ours reported the time the work was sent, so it kept choosing 60. It now reports when
  the GPU finishes, and the game stays at 30 (on a Radeon 660M: 110 switches between 30 and 60 in 20 s of race to 0).
- The start screen offers a newer release and installs it (Settings > Menu > Check for updates turns it off).
- "Translate the shaders" failed with `FileNotFoundError` when the game folder's path had a space (#29): the compiler's
  path was split at the space on its way to the shader step.
- The 4096 shadow map (Settings > Graphics) no longer crashes at the kart screen (#9).
- On scaled displays the mouse no longer lands off its place on the start screen (#27).

### Tested

From these packages, before publishing:

- The whole setup on Linux (Radeon 660M, RADV) and on Windows 11 (GTX 1050 Ti), and the update from the start screen on
  both.

## 0.1.4

Fixes the setup that stopped at 359/360 on many Windows PCs, and the shader step failing when a worker runs out of
memory.

### Changes

- "Compile the game code" stopped at 359/360 on Windows (#2, #12): zig, the C compiler in the package, builds its own C
  runtime the first time it links, and on those PCs that build never ended (`setup.log` had no `link probe` line). The
  package now carries that runtime already built and the setup links with it, so zig builds nothing of its own on the
  player's PC, neither for the game library nor for the shader step's helper DLL, where #24 stopped after 360/360.
- The shader step failed with `BrokenProcessPool` when the system ended a worker, usually when memory ran out (#24): it
  now goes on with half the workers, down to one, and `setup.log` says so.

### Tested

From these packages, before publishing:

- The whole setup (generate, compile, translate the shaders, the first run of the game) on Linux (Radeon 660M, RADV)
  with no system tools on PATH, and on Windows 11 (GTX 1050 Ti) with zig's cache emptied first.

## 0.1.3

Fixes two ways the setup could fail or look stuck and a GPU crash on NVIDIA under Linux, keeps a log of the setup, and
makes a GPU crash say where it happened.

### Changes

- "Compile the game code" sat at 359/359 with no sign of life (#2): after the last file the game library is linked,
  and on its first link zig also builds its C runtime for the target (76 s on a 6-thread Windows PC, more on a slow
  disk or with an antivirus scanning). That now runs while the files compile, the link counts in the progress bar,
  and a link that takes over 30 minutes stops with an error instead of waiting forever.
- `unable to find zig installation directory`: zig now gets its `lib/` folder named, so it no longer depends on the
  path a launcher or sandbox gives its executable.
- The setup writes `logs/setup.log`: every tool's output and how long each step took; a failed setup points to it.
- NVIDIA on Linux lost the device as soon as sparse textures were in use (at boot or when a race started): a texture
  was destroyed while a bind of its memory was still queued (#7, thanks to @guillo1990; likely #6).
- When the GPU is lost (#4, #5: AMD discrete cards that went black after the startup logo), `logs/mkw.log` now says
  what the driver knows (the faulting address, `VK_EXT_device_fault`) and which upload batch, render pass or dispatch
  the GPU had started and finished last (`VK_AMD_buffer_marker` marks). Nothing to set: attach the log.

### Tested

From these packages, before publishing:

- The whole setup (generate, compile, translate the 11 561 shaders, the first run of the game) on Linux (Radeon 660M,
  RADV) with no system tools on PATH, and on Windows 11 (GTX 1050 Ti).
- Races on both with the GPU marks on and off (no measurable cost), and #7's fix on AMD (a 60 s race, no GPU
  faults).

## 0.1.2

Fixes the setup, which could not build the game in 0.1.0 and 0.1.1.

### Changes

- It stopped right after "Generate the game code" with `fatal error: 'gamelib.h' file not found` (#1), on Linux and
  Windows.
- Translating the shaders needed `glslc` and `spirv-val` installed on the system.

### Tested

The whole setup (generate, compile, shaders and the first run of the game) from these packages on Linux, with no
system tools on PATH, and on Windows.

## 0.1.1

The Windows zip; its setup builds the shader spec too.

### Changes

- Windows 10/11 x64 package.

### Tested

The start screen on Linux and Windows.

## 0.1.0

First release: the start screen for Linux as an AppImage.

### Changes

- Linux AppImage of the start screen and its setup tools.

### Tested

The start screen on Linux.
