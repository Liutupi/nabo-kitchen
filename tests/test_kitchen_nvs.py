"""Exercise the kitchen journal against ESP-IDF 5.5.3's real NVS engine.

Only partition I/O, logging, attributes and ROM CRC are host shims. This uses a
synthetic 24 KiB NOR image, never a USB device or a user's stored data.
"""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    idf = Path(os.environ["IDF_PATH"])
    version = (idf / "tools/cmake/version.cmake").read_text()
    if not all(f"set(IDF_VERSION_{part} {value})" in version
               for part, value in (("MAJOR", 5), ("MINOR", 5), ("PATCH", 3))):
        raise RuntimeError("The NVS capacity test requires ESP-IDF 5.5.3")
    nvs = idf / "components/nvs_flash"
    with tempfile.TemporaryDirectory(prefix="kitchen-nvs-") as directory:
        temp = Path(directory)
        shims = {
            "sdkconfig.h": "",
            "esp_attr.h": "#define IDF_DEPRECATED(x)\n",
            "esp_log.h": "\n".join(f"#define ESP_LOG{x}(...) ((void)0)" for x in "EWIDV"),
            "esp_heap_caps.h": "",
            "esp_partition.h": "",
            "spi_flash_mmap.h": "#define SPI_FLASH_SEC_SIZE 4096\n#define ESP_ERR_FLASH_OP_FAIL 0x6001\n",
            "esp_rom_crc.h": """#pragma once
#include <stdint.h>
#include <stddef.h>
static inline uint32_t esp_rom_crc32_le(uint32_t crc, const uint8_t *p, size_t n) {
    crc = ~crc;
    while (n--) {
        crc ^= *p++;
        for (unsigned i = 0; i < 8; ++i)
            crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}
""",
        }
        for name, content in shims.items():
            (temp / name).write_text(content)
        includes = [temp, ROOT / "main/kitchen", nvs / "include", nvs / "src",
                    nvs / "private_include", idf / "components/esp_common/include"]
        flags = [f"-I{p}" for p in includes]
        objects = []
        c_sources = [ROOT / "tests/test_kitchen_nvs.c"] + [
            ROOT / "main/kitchen" / f"{name}.c" for name in
            ("nabo_kitchen_store", "kitchen_journal", "kitchen_save",
             "kitchen_session", "kitchen_timer", "kitchen_metadata")]
        for source in c_sources:
            obj = temp / (source.stem + ".o")
            subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-O2", "-Wall",
                            "-Wextra", "-Werror", *flags, "-c", str(source), "-o", str(obj)],
                           check=True)
            objects.append(obj)
        engine = [nvs / "src" / f"nvs_{name}.cpp" for name in
                  ("storage", "page", "pagemanager", "item_hash_list", "types")]
        executable = temp / "test_nvs"
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++17", "-O2", "-Wall",
                        "-Wextra", "-Wno-unused-variable", "-Wno-unused-parameter",
                        "-DNO_DEBUG_STORAGE", *flags, *map(str, engine),
                        str(ROOT / "tests/kitchen_nvs_host.cpp"), *map(str, objects),
                        "-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True, cwd=ROOT)


if __name__ == "__main__":
    main()
