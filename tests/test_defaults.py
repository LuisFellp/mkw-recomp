"""The memory caps a player gets with nothing set (runtime/gfx/vk/device/vk_defaults.c), built alone with the C
compiler at hand and checked on the GPUs players have reported. A cap measured on our Radeon 660M must not reach every
GPU: 768 MiB of tile slots everywhere left an RTX 4070 SUPER's ground flat (issue #31), and half a 16 GB card's VRAM
for the texture pool was 92% of the runtime's own budget there (issue #43).

Usage: uv run pytest tests
"""

import shutil
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "runtime/gfx/vk/device/vk_defaults.c"

# (name, discrete, VRAM MiB as the driver reports the largest device-local heap, RAM MiB) from players' logs and ours
GPUS = [
    ("Radeon 660M, 16 GB (ours)", 0, 1365, 15300),
    ("integrated, 32 GB", 0, 2048, 31000),
    ("integrated, 8 GB", 0, 512, 7600),
    ("GTX 1050 Ti 4 GB (our Windows PC)", 1, 4035, 16300),
    ("RTX 2070 SUPER 8 GB (#28)", 1, 7989, 40922),
    ("RTX 4070 SUPER 12 GB (#31)", 1, 12011, 32000),
    ("RX 9070 XT 16 GB (#43)", 1, 16304, 48000),
    ("RTX 4090 24 GB", 1, 24156, 65000),
]

MAIN = r"""
#include <stdio.h>
#include <stdlib.h>
#include "vk_defaults.h"
int main(int argc, char **argv) {
    (void)argc;
    printf("%llu %llu %u\n", (unsigned long long)vk_default_sparse_mib(atoi(argv[1]), strtoull(argv[2], 0, 10)),
           (unsigned long long)vk_default_mem_limit_mib(strtoull(argv[3], 0, 10)),
           vk_default_tex_pool_blocks(atoi(argv[1]), strtoull(argv[2], 0, 10)));
    return 0;
}
"""


@pytest.fixture(scope="module")
def defaults(tmp_path_factory: pytest.TempPathFactory) -> Path:
    cc = shutil.which("clang") or shutil.which("cc") or shutil.which("gcc")
    if not cc:
        pytest.skip("no C compiler")
    d = tmp_path_factory.mktemp("defaults")
    (d / "main.c").write_text(MAIN)
    exe = d / "defaults.exe"
    subprocess.run([cc, "-O1", f"-I{SRC.parent}", str(SRC), str(d / "main.c"), "-o", str(exe)], check=True)
    return exe


def caps(exe: Path, discrete: int, vram: int, ram: int) -> tuple[int, int, int]:
    out = subprocess.run([str(exe), str(discrete), str(vram), str(ram)], capture_output=True, text=True, check=True)
    sparse, limit, pool = out.stdout.split()
    return int(sparse), int(limit), int(pool)


@pytest.mark.parametrize(("name", "discrete", "vram", "ram"), GPUS)
def test_caps_follow_the_gpu(defaults: Path, name: str, discrete: int, vram: int, ram: int) -> None:
    sparse, limit, pool = caps(defaults, discrete, vram, ram)
    assert sparse >= 768, name  # the 660M's figure is the floor, measured with the arrays' coarse levels resident
    assert limit >= 4096, name  # 3072 makes the race black (sp-v0)
    if discrete:
        assert sparse <= max(768, vram // 4), name  # the rest of the VRAM holds render targets and mirrors
        assert 0 < pool <= 8, name  # half the VRAM, capped at 8 blocks (2 GiB): issue #43's 31 were 92% of the budget
        if vram >= 7900:
            assert sparse >= 1472, name  # what the title binds uncapped (run-sp3): nothing left non-resident
    else:
        assert sparse == 768, name  # 1024 hung the 660M with MKW_ASTC_BC1 (run-race41)
        assert pool == 0, name  # the pool is a discrete-GPU path (vk_tex_image.c)
        assert limit <= max(4096, ram // 4), name  # system RAM, outside the process's cgroup


def test_the_660m_keeps_its_measured_caps(defaults: Path) -> None:
    assert caps(defaults, 0, 1365, 15300) == (768, 4096, 0)


def test_a_16_gb_card_reserves_2_gib_not_half(defaults: Path) -> None:
    _, _, pool = caps(defaults, 1, 16304, 48000)
    assert pool == 8  # 2 GiB; 31 blocks (7 750 MiB) were 92% of the runtime's budget (issue #43)
