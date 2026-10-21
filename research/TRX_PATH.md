# RTL8922A SCC AB TX/RX path

`include/archer/trx_path.hpp` is a source reconstruction of the selected `rtw8922a_bb_cfg_txrx_path()` -> `rtw8922a_ctrl_trx_path()` call chain at Linux commit `c03c1a14507dfaf8b856c2608f9ac216021fc222`. Source snapshots: `artifacts/rtw8922a.c` lines 1868–2010, 2560–2571, 2621–2657, and `artifacts/rtw89-phy.c` lines 2111–2118. No new binary/runtime REA observation is claimed.

The plan requires caller-owned scheduler pause and PHY reset, PHY0/SCC (DBCC false), TX and RX AB antenna masks 3 and two spatial streams. This matches the observed physical adapter PHY capability, but the caller must preserve this invariant when constructing inputs. The pure plan performs no hardware access. The root channel transaction provides the exact `hal_reset` entry/exit and keeps TX paused during initialization. Exceptions must keep the producer paused pending recovery/reset.

The plan contains 55 ordered 32-bit operations. Seven full writes to MAC addresses 0x11a00–0x11a18 select AB TX paths. Remaining operations are masked BB writes, with physical addresses already increased by 0x20000 and values already shifted into masks. They configure TMAC ownership, RX AB selector (zeroed first to avoid abnormal EDCCA), HT/VHT/HE/EHT two-stream receive limits, maximum HE/EHT users 4, and TSSI reset pulses for both RF paths.

A subtle source behavior is intentionally preserved: `ctrl_trx_path()` loops over PHY0 and PHY1 even in SCC; `rtw89_phy_write32_idx()` adds a PHY1 offset only with DBCC enabled. Therefore SCC's second iteration targets PHY0 BB registers again. The second TX helper writes MAC_SEL then returns before its MAC path table, whereas RX selection, receive limits and TSSI reset pulses repeat. Deduplicating those operations would change the source sequence.

`rtw8922a_post_set_channel_bb()` immediately returns when DBCC is disabled (`rtw8922a.c` 2221–2231). The SCC callback therefore has an explicitly empty plan. DBCC digital power compensation and MLO changes must not be substituted into this profile.

Validation: strict C++17 compilation and execution of `tests/trx_path_test.cpp` passes. Tests check operation count, MAC path values, mask encoding, RX/NSS results, duplicate second-iteration order, invalid context rejection before writes, inaccessible first register handling, and the empty SCC post-channel callback. This scoped worker performed no physical writes.

## Initial SCC BB hardware setup

`rtl8922a_scc_bb_sethw_plan()` now ports `rtw8922a_bb_sethw()` and its selected `ctrl_mlo()` path. The default is the actual fresh SCC mode from `rtw89-core.c` line 7022: `MLO_DBCC_NOT_SUPPORT = 1`. With both channel contexts at 20 MHz it emits 12 physical masked writes: clears sounding without NDP for both BB banks, clears CMAC0 power-control selector, disables DBCC, enables DBCC_FA, sets both AFE DAC selectors to 14/7, then writes EMLSR parameters 0x6180, 0x180, 0. This sequence belongs before the TX/RX path setup, matching BB initialization order.

Mode encoding must not be confused with antenna masks: `MLO_MODE_FOR_BB0_BB1_RF(bb0,bb1,rf)` is `(rf << 12) | (bb1 << 4) | bb0`. Thus 2+0 2RF is 0x2002. RTL8922A's pinned `ctrl_mlo()` rejects that mode before any of its writes; the caller `bb_sethw()` ignores its return. The optional 0x2002 enum therefore yields exactly the first three hardware-setup writes and `rtl8922a_ctrl_mlo_supported()` returns false. It must never be reported as a completed MLO setup. The accepted 2+0 1RF enum 0x1002 has its separate 14-operation source path. Default fresh SCC mode1 is the intended executable baseline here. Tests verify these distinct branches and reject missing/non20MHz source channel contexts.
