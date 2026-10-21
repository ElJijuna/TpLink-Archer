# RTL8922A transmission calibration, source-derived implementation

Pinned Linux commit: `c03c1a14507dfaf8b856c2608f9ac216021fc222`. These are source observations and offline tests, not physical calibration evidence.

## Exact channel calibration order

`artifacts/rtw8922a.c:2388` invokes BTC channel-RFK START, saves/stops all scheduler TX, waits each RF path's register 0 mode field bits 19:16 to differ from TX mode 2 (poll 2 us, maximum 5000 us), then PRE_NOTIFY, TXGAPK, IQK, TSSI NORMAL, DPK, RXDCK. Finally restores scheduler state and sends BTC STOP. The respective source waits are 5, 54, 84, 20, 34, 32 ms. `artifacts/rtw89-phy.c:4011` multiplies all RFK waits by four for USB. The driver does not inspect intermediate return values in this top-level chip callback; the reconstruction should stop on failure and restore scheduler/coexistence guards.

The existing generic TSSI send guard remains closed. `encode_prepared_tssi_normal_h2c` is a separate route that always calls the board/trim/tracking preparation codec and requires explicit CMAC, channel, scheduler, RF-mode and coexistence prerequisite evidence. It accepts complete typed inputs, not a hand-made arbitrary payload. It still cannot prove that callers' prerequisite declarations represent hardware truth.

## Host-zero TSSI trim provenance

The source-backed helper `rtl8922a_linux_host_default_tssi_trim()` reproduces software initialization, **not factory calibration values**. The complete allocation chain is:

1. `artifacts/rtw89-core.c:7679`: private size includes `sizeof(struct rtw89_dev)` and calls `ieee80211_alloc_hw`.
2. `artifacts/mac80211-main.c:851`: private allocation size contains the driver's private data, passed to `wiphy_new_nm`; line 931 places `hw.priv` in this allocation.
3. `artifacts/cfg80211-core.c:597`: size includes `sizeof_priv`; line 599 uses `kzalloc`.
4. `artifacts/rtw8922a.c:744` efuse parser assigns thermal and CCK/MCS arrays only. Searching all locally pinned `artifacts/rtw*` snapshots for `tssi_trim[` and `tssi_trim_6g[` finds declarations and reads, no assignments. This negative result is bounded to the retained snapshots; it does not establish behavior of every upstream driver or unseen binary.

New source hashes, obtained directly from the same commit:

| Artifact | SHA-256 |
|---|---|
| `mac80211-main.c` | `de553e5322cff805514ddf7494be96ba42464df5a517b93f2ebb49a5c74ce779` |
| `cfg80211-core.c` | `a1e6ebc52f7634585e5c7e8cb29b2e106de5af12da68cf9e5ed0794ad94fcb9a` |
| `rtw89-coex.c` | `7e9785539dc9098217e3f1fc01b28bddbb6f6ed3b6b622f3465bb5beacebfefa` |

## Coexistence boundary

`rtw89_btc_ntfy_wl_rfk` is not automatically a no-op when Bluetooth is absent. RTL8922A declares `scbd=1`, `para_ver=0xf`; RFE1 selects shared two-antenna context. START/STOP updates host RFK state and WLRFK scoreboard shadow; `_run_coex` normally commits hardware changes through policy execution. Its explicit early return for `!wl.status.init_ok` (`artifacts/rtw89-coex.c:8451`) means a freshly zero-initialized and deliberately uninitialized BTC host profile performs no corresponding hardware policy writes. This narrow research context must be declared explicitly; it does not implement full production coexistence or prove that Bluetooth cannot interfere. Full coexistence initialization also changes RF LUT masks and MAC PTA routing.

## Native decoder API

Include `archer/tx_rfk.hpp` and use `decode_iqk_log_reports`, `decode_dpk_log_reports`, `decode_txgapk_log_reports`, `decode_tssi_log_reports` on an entire C2H message. These return report vectors and reject unknown extents. Corresponding single-chunk helpers are available. The pinned packed reports have lengths IQK 516, DPK 34, TXGAPK 100 and TSSI 56 bytes. No speculative extension is accepted.

`flags_clear()` on IQK checks both TX/RX failure arrays. DPK exposes known timeout/overflow through `error_flags_clear()` and keeps path success unknown when its producer semantics are not proven; see the correction below. TXGAPK requires its explicit overall success and, only when present, zero failcodes. IQK/DPK `matches()` verifies both reported path channels/bands/bandwidths; IQK also verifies PHY. Keep terminal RFK state OK and report flags as separate requirements. TXGAPK report has no explicit channel, so it cannot independently verify channel attribution. TSSI reports preserve alignment states with no success predicate: pinned Linux prints these bytes without defining their state values. Successful terminal transport state alone is not independent RF quality verification.

Strict decoder synthetic tests cover negative flags, extents, LE failcodes, signed alignment, channel matching, prepared TSSI wire length, prerequisite rejection and retention of the general TSSI guard. No hardware access was made in this subtask.

## Stage38 historical TXGAPK report support

Physical capture `artifacts/tx-rfk-stage38/rx-218.bin` contains an 84-byte TXGAPK report, not the later 100-byte layout. This is an exact historical primary-source layout from [wireless-next fw.h commit 8f7aa3d3c7323f4ca2768a9e74ebbe359c4f8f88](https://kernel.googlesource.com/pub/scm/linux/kernel/git/wireless/wireless-next/%2B/8f7aa3d3c7323f4ca2768a9e74ebbe359c4f8f88/drivers/net/wireless/realtek/rtw89/fw.h), lines4798–4807. The retained decoded source `artifacts/rtw89-fw-wireless-next-8f7aa3d.h` has SHA-256 `bae6675fc95c3f6edd348aaa78c2bbb5cc7768f1961a91fb95ff49624aa9e345`.

Both layouts place `is_txgapk_ok` at80, checkID81 and version82. In the older format byte83 is reserved, not dual-band success. The older format has no stage/failcode fields; these are now optional and absent values stay unknown. `flags_clear()` for84-byte reports means only explicit overall success (`is_txgapk_ok==1`), while100-byte reports additionally require both present failcodes zero. Neither implies independently measured RF performance.

A retained119-byte USB fixture `tests/fixtures/txgapk-stage38-usb.bin` (SHA-256 `8adc3a86cc3fd3c1dfa42a11fc4e52eaaeb151f338551ff090dddf7854d25b02`) verifies the actual observed84-byte report passes its explicit overall success flag and keeps missing failcodes absent. Tests also reject unsupported extents and legacy overall-failure values. No additional hardware command was issued by this decoder task.

## Stage39 historical IQK report support

Physical `artifacts/tx-rfk-stage39/rx-7.bin` contains184 bytes of IQK content. The same historical primary header above, lines4704–4738, defines exactly this extent: common32-byte flag/context prefix; reload/failure counts8 bytes; two-path LOK IDAC/VBUF16 bytes; four two-path four-word arrays128 bytes. The later516-byte report preserves the32-byte prefix and extends coefficient arrays/adds RF register fields. The decoder reads only the common documented prefix and preserves the complete raw report without inventing unavailable later fields.

The captured report has clear TX/RX failure flags and both-path channel1/2GHz/20MHz/PHY0 context. Fixture `tests/fixtures/iqk-stage39-usb.bin` SHA-256 `c3020ec82818072a0fc90d5140742493faa701076856e92be9dc95dcae94b523` verifies those observations. Only the exact184 and516-byte sizes are accepted. The historical DPK34-byte and TSSI56-byte structures (lines4740–4757 and4790–4796) are identical to the current layouts; no extension or relaxed extent is necessary for them.

## Stage40 DPK status correction: path mask unknown

The initial reconstruction's requirement `path_ok==1` was an unsupported assumption and is withdrawn. Stage40 has a terminal RFK OK and a source-shaped34-byte DPK report, version13, path values `[4,4]`, no timeout/overflow flags and matching2GHz/channel1/20MHz context. This is neither proven failure nor independently proven successful path calibration.

The pinned primary RTL8922A host only prints the `u8 path_ok[2]` report values; it does not decode their producer semantics. A related-chip primary implementation, [RTL8851B at the same Linux commit](https://github.com/torvalds/linux/blob/c03c1a14507dfaf8b856c2608f9ac216021fc222/drivers/net/wireless/realtek/rtw89/rtw8851b_rfk.c#L2559), explicitly accumulates `BIT(cur_k_set)` in `path_ok`, demonstrating that this name cannot safely be assumed boolean. Retained `artifacts/rtw8851b_rfk.c` SHA-256 `4ca49ba0e08ebffda9777141c954e22ecb7f656cb770c6249d885529ac914338`. That is comparative evidence, **not proof** of the RTL8922A v13 producer or active set. The retained8922A firmware contains a matching path-status format string and K-set log strings, but strings alone do not establish its executable mapping. A bounded web/source search found no public8922A producer implementation.

The decoder now exposes `error_flags_clear()` separately from `path_success()` (optional boolean). Nonzero path status remains unknown until its exact semantics are established. `flags_clear()` fails closed on unknown success. Terminal OK and no error flags must be reported accurately without calling `[4,4]` a failure or silently converting any nonzero status to success.

Physical fixture `tests/fixtures/dpk-stage40-usb.bin`, SHA-256 `3269da0577cd93508fe28fac42a66af6c2123d9342a2908564f3d35fb48b59d8`, verifies version13, `[4,4]`, matching channel, clear timeout/overflow, and unknown path success. Synthetic status1 is also no longer assumed proven success. No additional hardware access was performed.

## Stage41 runtime K-set corroboration, bounded conclusion

All retained `artifacts/tx-rfk-stage41/rx-*.bin` captures were parsed offline, including captures acquired while draining before RXDCK. DPK class8/function1/type0 run-log records expose the following path-A observations in `rx-10.bin`:

| Firmware dictionary format index | Arguments | Observable meaning from its format string |
|---|---|---|
|18|`[0,2,62,54]`|path0 thermal/TXAGC for K2|
|35|`[0,0,4,0]`|path0/index0 path_ok hexadecimal4|
|36|`[0,0,2,0]`|path0/index0 K2 numeric outcome0|
|37|`[0,0,4,0]`|path0/index0 DPK numeric status4|

The format dictionary belongs to retained firmware element RFKLOG_DPK (header offset1581776,43 formats). Path-A K2 and status4 satisfy the arithmetic relationship `4 == BIT(2)`, corroborating the related-source calibrated-set-mask hypothesis. The numeric outcome0 string does not define its enum/boolean polarity. Neither this observation nor a neighboring-chip implementation proves the exact RTL8922A v13 status producer. No corresponding path-B final K-set/status run logs were found in the retained captures; the detailed DPK report independently reports both path bytes as4.

Consequently, the source/runtime ledger is stronger but `path_success()` stays unknown, and `all_calibration_quality_validated` must remain false. Successfully completing all firmware commands and receiving terminal OK must remain distinct from independently validated DPK mask semantics and TSSI alignment semantics. No extra hardware commands were issued in this analysis.
