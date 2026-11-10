# runtime/probes

Self-tests of each subsystem, one command-line option each (`build/mkw --probe-…`). They come from totk-pc-recomp
except `texpre_probe.c`.

| File | Option | What it checks |
|---|---|---|
| `astc_probe.c` | `--probe-astc` | The compute ASTC decoder (`vk_astc.c`) against the CPU one, bit for bit, with synthetic blocks |
| `audio_probe.c` | `--probe-audio` | MKW's own: the SDK's translated `nn::audio` client against the objects of `audren_sf.c`: parameter, open, 200 updates at 5 ms, device and close (formerly `--probe-ipc`) |
| `cfg_probe.c` | `--probe-cfg` | Reading the `.ini`, environment variables and a missing file (`cfg.c`) |
| `gfx_abi_probe.c` | `--probe-gfx-abi` | Our objects fit in NVN's and the fields the game reads are where it expects them |
| `gfx_window_probe.c` | `--probe-window` | Window present, vsync, focus, cursor and resize |
| `kernel_probe.c` | `--probe-kernel` | Waits, wakeups, virtual cores, fibers and translated code on four threads |
| `nvn_probe.c` | `--probe-nvn` | The NVN front end over the null back end, called as the game calls it |
| `nvn_tex_probe.c` | `--probe-nvn-tex` | Size, alignment and mip offsets of textures (`nvn_tex.c`) |
| `ov_probe.c`, `ov_probe.h` | `--probe-services` | Each replacement of `runtime/sdk/` called through its synthetic address; all 187 must be bound by some MKW module |
| `ov_mkw_probe.c`, `ov_mkw_list.h` | `--probe-services` | MKW's own: the 96 replacements only MKW imports (offline network, account, `oe`, clock, touch and the VP9 decoder with frames the test encodes itself) |
| `tex_probe.c` | `--probe-vkmem` | Texture memory: block-linear copies, ASTC, format table, imported pools |
| `texpre_probe.c` | `--probe-texpre` | MKW's own: quality of the GPU BC7 against `bc7enc` on the game's textures, listed in `MKW_TEXPRE_PROBE_LIST` |
| `vk_exec_probe.c` | `--probe-vkexec` | Vulkan replay: fence values, completion order, empty submits; `MKW_RTSTATS`'s rules on cleared targets |
