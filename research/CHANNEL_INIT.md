# Intended channel 1 / 20 MHz setup

Status: MAC/BB/RF tuning subplans implemented; complete coordinator blocked before I/O. `include/archer/channel_init.hpp` defines an intended
PHY0/MAC0 SCC 2.4GHz primary/center channel1, 2412MHz, 20MHz context. It is not
an applied-channel token. Its initializer always rejects until the complete
channel transaction coordinator exists, even when all caller evidence flags are true. This
prevents a requested channel in an RFK payload from being mistaken for actual
RF configuration. No device was accessed.

## Pinned source order

`artifacts/rtw8922a.c:2232` calls set_channel_mac, set_channel_bb,
set_channel_rf. `set_channel_help` at2321 brackets this with pre BB/RF and
hal_reset entry, followed by hal_reset exit and post BB/RF. For SCC the BB
pre/post DBCC sections are no-ops, but that does not establish the RF ones.
`hal_reset` at2300 pauses scheduler TX, disables PPDU reporting/DFS/TSSI/ADC,
delays40us and asserts BB reset. Exit reverses those operations and restores
scheduler TX. A diagnostic retaining a no-network-TX policy must keep scheduler
TX blocked instead of blindly restoring a source runtime's previous TX state.
No SSID, association or network packet generation is needed to define context.

MAC channel setup at1048 sets 20MHz RFMOD, subband and primary20 bitmap,
2GHz rate checking and SIFS timing. BB channel setup at2179 runs CCK SCO,
channel control, bandwidth, CCK enable, spur elimination, async-reset release,
TSSI reset. Channel control at1490 writes board-dependent gains **before**
center frequency. For channel1 the derived center frequency is2412 and SCO
is round(262144/2412)=109. Known BB fields (logical addresses, add0x20000 for
this chip's host register transport) include FC0=0x6b4c mask0x1fff,
FC0_INV=0x6b50 mask0x7f, channel-index=0x0734 mask0xff0000. These facts are not
a complete executable register plan.

20MHz branch at1550 sets CHBW0, SMALLBW0, primary subband0, DAC_CLK1 and both
gain-map enable bits0; later bandwidth handling also adjusts
BW40_2XFFT. CCK SCO at1149 indexes channel-1 into the two chip threshold arrays.
CCK parameters at1466 select the non-channel14 coefficient set. Spur handling
at1718 calls CSI and both NBI paths; it cannot be omitted solely because the
requested channel is simple.

## Required data and missing implementation

- `set_lna_tia_gain` at1316 reads bb_gain.be band/path LNA/TIA/op1dB arrays;
  `set_rpl_gain` at1271 writes rpl_ofst20/40/80/160 arrays for both paths,
  even when channel width is20. Board table selection/parser state is required.
- Efuse gain compensation at1390-1463 uses offset_valid and per-path gain
  offsets. An explicit validated absent/invalid-efuse decision is different
  from inventing all-zero offsets. RF trim application is a separate board
  prerequisite; complete trim/channel sequencing has not been ported here.
- TX-power/channel coordinator context must be established, including any
  thermal/TSSI inputs; no default TX-power tables or temperature are supplied.
- Pinned RFK source is now available as artifacts/rtw8922a_rfk.c/.h,
  with identity recorded by root in research/rfk-source-stage30.json.
  Its ctl_band_ch_bw reads A18/A10018/B18/B10018 fully before updates,
  obtains a separate synthesizer18 validation read, and writes those four
  registers with100us pauses. For CV1,2G20MHz the merge is
  `(old & ~0x31fff) | 1`; CAV LUT workaround is skipped. The pure
  channel1_rf_tuning_plan validates all four values before emitting writes.
  A future executor must also validate the source's synthesizer read and
  perform readback verification; plan construction does neither bus operation.
- SCC pre_set_channel_rf is a no-op, but post_set_channel_rf invokes
  chlk_reload even withDBCCoff. It chooses RFK MCC table per path, updates
  host context and sets BB coefficient selectors plus RF_MODOPT TXG selector.
  A new source-zero RFK MCC table normally selects slot0; do not assume that
  after a previous calibration/channel transaction. Required per-path writes:
  BB0x8104/0x8204 bit31=1,bits1:0=table,bits9:8=table;
  RF0x01 mask0xe0000 field4|table; BB0x8154/0x8254 bit3=tablebit0,
  bit5=tablebit1. Validate table0..2 and update PRE_NOTIFY host context to match.

## RF V2 semantics

`artifacts/rtw89-phy.c:1039-1064,1208-1265`: RF address ADSEL chooses direct
RF access vs analog HWSI. Analog reads obtain full20-bit data then apply
`(value & mask) >> ffs(mask)`; partial analog writes read full20-bit state,
merge `(old & ~mask) | ((data << ffs(mask)) & mask)`, then full-write through
HWSI. The latter polls pathA/B0x2c24/0x2d24 bit29 idle at1us up to3800us and
writes0x2ae0/0x2be0. Host BB base translation is additional. A full-write-only
RF transport cannot execute masked channel updates without validated reads.

The pure `merge_channel_rf_field` helper tests this merge for contiguous
nonzero20-bit masks and rejects overflow, invalid source values and masks.
It does not perform RF reads or claim RF readiness.

## Offline validation

```
c++ -std=c++17 -Wall -Wextra -Werror -Iinclude tests/channel_init_test.cpp -o /tmp/archer-channel-init-test
/tmp/archer-channel-init-test
```

Passing tests verify intended context, rejection of each unsupported context
field, explicit prerequisite assessment, zero bus calls on refusal even with
all evidence flags supplied, and RF field preservation/validation. They do not
verify channel hardware programming. Actual channel setup remains unresolved.

## Implemented plan interfaces

`channel1_mac_plan(context)` returns six raw MAC writes (including byte-width
RFMOD/rate-check operations), following set_channel_mac. `channel1_bb_plan`
returns113 masked physical-BB writes if efuse compensation was explicitly
skipped,133 if it is enabled. It covers the complete SCC PHY0
set_channel_bb function; hal_reset/pre/post coordination remains external.
Masked values are already shifted; apply `(old & ~mask) | value` at the given
address. BB plan order matches source, including source's pathB CCK writes
replacing the shared CCK compensation registers after pathA.

`Channel1BbInputs.gain[2]` requires optional signed8 cells for each path:
lna_gain7,tia_gain2,lna_op1db7,tia_lna_op1db8,rpl1608,rpl804,rpl402,rpl201.
`efuse_offset_valid` must explicitly contain true or false. True requires
signed8 `efuse_cck[2]` and `efuse_ofdm[2]`. There are no zero defaults for gain
cells. `artifacts/bb-gain-stage17.json` contains band0/bw_type0 entries for
2GHz; rpl entries have no bandwidth selector. It lacks band0 rpl80/rpl160
entries. A binding must establish source initialization of absent cells before
supplying explicit zeroes. Signed values are encoded exactly as Linux masks;
CCK negation/shift and OFDM saturation match source signed8 behavior.

`channel1_rf_tuning_plan` emits four full20-bit RF writes with100us trailing
delays; address0x18 uses HWSI,0x10018 uses direct mapped access. This is a tuning
subplan, not complete RFK post handling. `initialize_channel1` deliberately
continues to refuse complete hardware execution until the transaction/reset/
post-RF coordinator is implemented. Root may execute verified subplans inside
its own complete lifecycle; no input flags turn this placeholder into an
unimplemented hardware action.

Expanded tests cover MAC encoding, BB source-order/count, missing gain cells,
explicit efuse skip/required offsets, signed encoding, RF unrelated-bit
preservation, both addresses/both paths, invalid snapshots/cut/prerequisites.
