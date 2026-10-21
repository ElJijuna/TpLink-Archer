# RTL8922A BB postinit and RF_NCTL primitives

Pure register plans and an injectable bus executor live in `include/archer/bb_postinit.hpp`. Tests use a fake register bus; no device I/O or calibration was performed.

## Source mapping

`artifacts/rtw8922a.c:1820-1849` defines the BB postinit sequence. The port preserves its order and masked writes, including the final UDP coefficient 0→1 edge. For PHY0 the host sets FEN BOOT_RDY0 (0x84 bit2), then BBPLAT_RSTB (bit0). For PHY1 only BB1PLAT_RSTB (bit8) is set. These masks are defined at `rtw89-reg.h:4137-4157`; this function does not read an independently produced MCU liveness indicator. Setting BOOT_RDY0 cannot demonstrate that BBMCU is executing.

`rtw89-phy.c:2147-2157` makes `set_phy_regs` write PHY0 and, when DBCC is enabled, PHY1. It does so independently of the passed `phy_idx`. The BBCLK and TXSCALE operations are common writes; UDP coefficient alone uses the passed PHY. The port retains this distinction. `rtw89-phy_be.c:238-254` supplies the BE PHY1 0x1000 offset for pages 04–0f, 20–2b, 40–4f, 60–6f, e4–e5 and e8–ed. BB addresses map to physical 0x20000 plus logical address, consistent with the existing transport.

Numeric postinit registers/masks are pinned in `rtw89-reg.h:8815-8816`, `9046-9047`, `9976-9979`, `10026-10053`. The slope B unshifted value 0x3fe0 becomes 0x0ff80000 under GENMASK(27,14). The API stores **positioned** values alongside masks, avoiding ambiguity with Linux's unshifted masked-write helper.

`rtw89-phy_be.c:475-496` supplies BE NCTL preinit, using masks at `rtw89-reg.h:9019-9039`, `9076-9077`, `9394-9395`, `10491-10500`. Two extra reset operations occur with DBCC. The standard BE registers apply to RTL8922A; this is not the separate BE-v1/BE4 preinit at source line498.

`rtw89-phy_be.c:453-473` supplies the NCTL register check: write logical0x8000=8; up to10000 iterations write logical0x8080=4, delay10µs, and read for equality4. `check_be_nctl` returns attempts used or throws on exhausted bound. The source logs a warning then continues; this implementation exposes failure to the caller so it can stop table application. The check establishes readback access, not autonomous MCU progress. The 100ms nominal delay budget excludes USB transaction latency.

## Selected table input and execution

`selected_rf_nctl_plan` accepts logical `(address,value)` pairs **after** the existing `tools/phy_table.py` headline/conditional interpreter has selected firmware CV/RFE entries. It intentionally rejects remaining conditional opcodes and out-of-range or unaligned addresses. It supports BB callback delay sentinels f9/fa/fb/fc/fd/fe =1/5/50/1000/5000/50000µs and skips value0xbabecafe after validating the address, matching `rtw89-phy.c:1350-1376`. Full writes go through the BB aperture; RF_NCTL is not an RF A/B register-table upload.

The sequence at `rtw89-phy.c:2065-2077` is preinit → NCTL check → selected RF_NCTL table → optional chip NCTL posttable. RTL8922A's compiled fallback and posttable are NULL (`rtw8922a.c:3245-3246`), so the selected firmware element is required for actual NCTL table initialization.

The executor validates the complete plan before its first read, write, or delay, requires an explicit microsecond delay callback for delay entries, preserves unrelated bits on masked writes, and rejects known inaccessible register sentinels for read-modify-write. Table writes are full-width. A bypass-only selected table returns an empty plan; callers should skip execution for that case. Input tables are bounded to4096 selected pairs and execution plans to8192 operations. No rollback is claimed for write-only/microcode effects or a runtime bus error.

Callers must establish hardware identity, initialized MAC/DLE and BB access, the selected firmware/CV/RFE profile, and applicable firmware readiness before applying these plans. These primitives do not initialize gain host arrays, regulatory/country data, channel context, radio tables A/B, or perform RFK. BB postinit/NCTL register writes alone do not complete radio bringup.

## Verification

`c++ -std=c++17 -Wall -Wextra -Werror -Iinclude tests/bb_postinit_test.cpp -o /tmp/archer-bb-postinit-test` and the binary pass. Tests cover resulting masked values and preserved bits, PHY0/PHY1/DBCC distinctions, the UDP edge, NCTL preinit, all six delay opcodes, bypass, malformed trailing operations before effects, missing delay callbacks, bounded successful and failed NCTL polls, and inaccessible read-modify-write baselines. Hardware behavior remains unverified.
