# SCC channel 1 reset bracket and RF post handling

`include/archer/channel_transition.hpp` supplies pure ordered plans around the
MAC/BB/RF channel subplans. It performs no I/O. Sources are pinned
`artifacts/rtw8922a.c:1851,2243-2337`, `rtw8922a_rfk.c:202-355,409-438`,
`rtw89-phy.c:8381-8409` and `rtw89-reg.h`.

Use order: reset_enter plan, MAC plan, BB plan, collect valid full RF18
snapshots (plus source's synthesizerA18 read), RF tuning plan (delay100us after
each), reset_exit plan, post_RF plan. Pre-set-channel BB and RF are no-ops for
SCC. Reset entry pauses scheduler and disables PPDU reporting through explicit
coordinator callbacks; those two operations must not be skipped. All masked BB
operations carry physical addresses and already encoded values. Each write
must preserve unmasked bits and be verified before proceeding.

Reset entry disables DFS on both paths, holds TSSI, disables both ADC FIFO
paths, waits40us, disables CCK/OFDM packet detection, waits1us and asserts async
reset. The ADC source performs two masked RMW operations setting bits0 then1
inside its bits31:24 field; the plan preserves those independent setting
operations at physical0x210fc with masks0x01000000 then0x02000000. No read-only status or W1C bits
are involved in that field. Exit clears those two bits, enables DFS/TSSI/BB
reset release and packet detection after re-enabling PPDU reporting. The
source subsequently resumes saved scheduler state. Diagnostic plans default
to **keeping scheduler TX paused**; explicit resume requires the separate true
argument. This is a deliberate no-network-TX policy difference from Linux.

## RFK table selection

Firmware0.35.41.0 uses pre-MCC-V1 per-path host tables (the V1 feature threshold
in fw.c:931 is0.35.84.0). Post RF always invokes chlk_reload, even withDBCC off.
It looks up a slot based on channel/band/width and active entity channels,
updates host MCC channel/band/width/rf18/table index, and programs both paths'
coefficient selectors. `channel1_fresh_mcc_slots(true,true)` returns{0,0}
only for source-zeroed MCC state with channel1 as the sole active entity.
Reused MCC state must run the real lookup. Passing arbitrary booleans does not
establish freshness; the root coordinator owns this lifecycle evidence.

`channel1_post_rf_plan` accepts validated slot indices0..2 and two full RF01
snapshots, validates all inputs before returning writes, sets coefficient enable
and IQC/MDPD indices, writes RF01 TXG selector field4|slot while preserving
other20-bit RF data, then sets CFIR LUT G3/G5 from the selected slot bits.
Linux reads those bits back after programming. The plan's equivalent requires
the executor to verify selector writes before applying LUT bits. Snapshot reads
must occur immediately before the post transaction under exclusive ownership.

The returned selected_table/channel/band/bandwidth/rf18 fields define the host
MCC changes to commit after successful programming. PRE_NOTIFY payload must
reflect exactly those table values. No applied-channel token is produced by a
pure plan, nor should one be committed after a partial write/failure. No generic
rollback is safe after an RF transition failure; keep scheduler paused and
require controlled reset/reinitialization. Successful programming still needs
RF readback and actual RFK completion; it is not proof calibration succeeded.

## Validation

```
c++ -std=c++17 -Wall -Wextra -Werror -Iinclude tests/channel_transition_test.cpp -o /tmp/archer-channel-transition-test
/tmp/archer-channel-transition-test
```

Tests pass for entry/exit order, delays, ADC field preservation mask, default
TX pause, explicit resume, fresh-MCC-only guard, both path coefficient/RF
encodings, and invalid slots/read snapshots. No hardware was accessed.
