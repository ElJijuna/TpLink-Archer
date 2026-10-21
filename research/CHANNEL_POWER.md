# Channel 1 board inputs and power prerequisites

Scope: PHY0/MAC0 SCC, 2412 MHz channel 1, 20 MHz; RTL8922A CV1, AID 0x6933, RFE1. Source is the retained Linux commit `c03c1a14507dfaf8b856c2608f9ac216021fc222`, not a new REA decompilation. No hardware operations were performed by this implementation pass.

`tools/channel_bundle.py` validates the complete main MFW SHA-256 plus the captured logical RF map and PHY capability region. It reconstructs BB gains from the actual selected firmware element via `bb_gain.inspect_gain`, rather than trusting the earlier JSON export. It emits ignored `artifacts/channel_payload.hpp`:

- `trusted_channel1_bb`: exact band0/BW20-40 gain cells, efuse CCK/OFDM offsets and global validity decision.
- `trusted_channel1_trim`: source-decoded PA/PAD programming flag and bytes, thermal programming flag and signed trims.

All gain cells must exist except 2G RPL80/RPL160, which are absent in this selected table. Those fields remain zero in a fresh source device allocation (`rtw89_alloc_ieee80211_hw` uses `ieee80211_alloc_hw`, zeroed private driver memory); their explicit zero binding is restricted to this fresh initialization flow. It does not infer that omitted gain entries on a reused initialized device are zero. Band0 RPL20/RPL40 come from the firmware. Source `rtw8922a_rx_gain` stores OFDM before CCK, so CCK uses RF-map offsets 0xd5/0xda and OFDM uses 0xd4/0xd9. Global `offset_valid` considers all 26 cells including 6 GHz, before sign-magnitude conversion, matching `rtw8922a_efuse_parsing_gain_offset`.

`channel1_board_trim_plan` reproduces `rtw8922a_power_trim`: PA fields on RF0x60 for both paths first, then PAD fields for both paths. The source gates both PA/PAD operations on PHY-cap byte0 != 0xff. Thermal trim is retained as host compensation input; this function does not invent a thermal RF register write. Plans use unshifted RF mask fields and require real RF reads and merge at execution.

`channel1_txpwr_unit_plan` is intentionally empty because RTL8922A `.init_txpwr_unit = NULL` at the pinned source. `channel1_txpwr_reference_plan` ports only `set_txpwr_diff` and `set_txpwr_ref` for CV1. Antenna offset must be the actual source decision, not an assumed zero: RTL8922A supports antenna gain and this decision also depends on regulatory domain, firmware features and RFE antenna data. Zero references are established source constants in `set_txpwr_ref`, not guessed power limits.

## Complete world/unconfigured diagnostic power profile

The generator now also emits `trusted_channel1_world_power`, 197 ordered masked/full32 writes implementing the source `rtw8922a_set_txpwr` flow for an explicit **world, fresh unconfigured diagnostic profile**. Call `validate_world_diagnostic_power` with world regd0, SAR source NONE, antenna `regd_enabled=0`, fresh host state, and scheduler TX paused before executing this vector. This profile is a diagnostic choice, not an inference of the adapter's country or authorization to transmit network packets.

The sequence includes all five bandwidth by-rate storage regions (107 packed writes, even though the active channel is 20 MHz), source rate offsets, OFDM shape, two 76-byte normal limit pages, two 80-byte RU pages, antenna difference/reference fields, and per-path SAR maximum. Firmware element IDs 9/10/13/16 are selected with existing AID/RFE semantics. Rate storage loading and output ordering follow `rtw89_fw_load_txpwr_byrate` plus `__phy_set_txpwr_byrate_be`; RF-to-MAC units use signed floor division by2. Unlisted by-rate storage retains fresh private-state zero. Source RFE allocation is `kzalloc_obj(*rtwdev->rfe_data)` (`fw.c:1191`), so omitted WW shape and omitted BF table combinations retain zero. No non-BF limits are copied into absent BF combinations.

Normal channel1 limits come from WW/ch_idx0 rows, source 20MHz memset-plus-fill yields four pairs (CCK20, CCK40, OFDM20, MCS20) and zero remainder. RU source memset-plus-fill installs five RU types at page offsets0/16/32/48/64. WW shape is omitted by firmware and stays zero. Both are source-defined zero initialization, not guessed board calibration values.

For the profile's fresh antenna `regd_enabled=0`, `rtw89_can_apply_ant_gain` returns false before DA table access; antenna difference is therefore0. For SAR source NONE, pinned `sar.c:287-288` returns `RTW89_SAR_TXPWR_MAC_MAX`, defined63 in `sar.h:10`; per-path SAR RF fields receive126. The 2GHz TPE constraint is63 after source dbm-to-MAC clamping. Limits use `min(firmware_rf_limit >> 1,63)`. These defaults must not be reused after configuring country/antenna/SAR state.

No normal scheduler resume is included. Root integration must keep network TX paused, apply/verify these writes, finish the channel transaction and validate firmware calibration completions. Supporting other channels, countries, runtime SAR policies, antenna gain or network transmission requires the general power pipeline rather than this fixed profile.

Validation: strict-warning C++ tests exercise identity/context rejection, PA/PAD ordering/masks, explicit empty init-unit decision, positive antenna-offset path selection, and diagnostic profile preconditions. The generated real-board BB/power bindings compile. The complete diagnostic vector has 197 writes. Hardware validation remains the root integration task.
