# Offline RTL8922A TSSI NORMAL payload preparation

Stage42 follow-up: the zero-initialized Linux host trim state has now been
proven through the allocation chain, and the separate typed route in
`tx_rfk.hpp` has executed TSSI SCAN/NORMAL with bound board/tracking data.
See [STAGE42.md](STAGE42.md) and [TX_RFK.md](TX_RFK.md). The original general
send guard remains closed; alignment-state success semantics remain unknown.

`include/archer/tssi_protocol.hpp` ports the signed DE/group interpolation and thermal-table construction for firmware0.35.41.0 into a pure offline300-byte payload builder. It does not transmit H2C and does not lift the existing TSSI guard in `rfk_protocol.hpp`.

## Concrete inputs available

The physical capture `artifacts/efuse-stage13/physical-ddv.bin` is4864 bytes (SHA256 b4ca726d25b04ca152d69e0c77a2f73f49916fb9f22ffd8c261380e99608c5df). It must be decoded through the existing logical efuse-map parser before using factory offsets. The already decoded `rf-logical.bin` is the0x240-byte logical RF map accepted by `decode_tssi_board`.

`artifacts/rtw8922a.h` packed map and `rtw8922a.c:743-777` establish the TSSI fields, also independently decoded by `tools/rf_calibration.py`:

| Field | Logical RF map offset |
| --- | --- |
| PathA CCK6 / 2G MCS5 / 5G MCS14 | 0x10 / 0x16 / 0x22 |
| PathB CCK6 / 2G MCS5 / 5G MCS14 | 0x3a / 0x40 / 0x4c |
| RFE | 0xca |
| PathA/B thermal | 0xd0 / 0xd1 |
| PathA/B6G MCS32 | 0x100 / 0x12a |

The observed map has RFE1, thermalA0x6f/B0x71. Channel6 has raw CCK DE A6/B18 and2G OFDM DE A12/B21. Signed5G pathA values begin -15,-9,-13,-10; these must not be treated as positive unsigned factory offsets. Decoder rejects an erased map/RFE and wrong extent. Individual0xff signed DE codes retain source meaning -1; erased thermal255 uses the explicitly sourced fallback below.

`phycap.bin` is56 bytes at physical0x1700..0x1737 (SHA256 ca1de3fdd127c861ab5f3b8fccefeef916a109f437b7de50e23f5ea08486bfd2). `tools/phy_calibration.py` decodes thermal, PA and PAD trim from it. **PHY thermal trim is not TSSI DE trim.** No assignment to `tssi_trim`/`tssi_trim_6g` exists in the pinned RTL8922A efuse/PHY-cap parser source. Those arrays are separate host state in `rtw89-core.h:6939-6940`; this port therefore requires explicit `TssiTrimData` rather than silently replacing them with thermal trim or assumed factory zeros. Tests label their zero arrays as synthetic zero host state. Verifying the appropriate initialized host trim state for this driver path remains a caller prerequisite.

## Firmware TX power tracking element

The pinned `artifacts/rtw8922a_fw-1.bin` contains one ID18 TXPWR_TRK candidate:

- Header offset1580656; payload offset1580688; payload1080 bytes; full element1112 bytes.
- AID0 (generic, accepted for physical AID0x6933); version bytes0.37.0.0.
- Header+24 bitmap0x0000ffff; payload SHA256 f95a86676fd251e503f4c89c77af2152bedb95525de28c1fef701e26918d15ce.
- This element has no RFE selection field. Do not interpret its bitmap bytes as the power-limit element's RFE metadata.

`rtw89-fw.c:1225-1294` filters generic/matching AID, checks required types, and maps contents using30 signed bytes per delta row. `rtw89-fw.h:4493-4524` numbers types:6G BN/BP/AN/AP0..3 (four subband rows each);5G BN/BP/AN/AP4..7 (three rows each);2G BN/BP/AN/AP8..11 and CCK BN/BP/AN/AP12..15 (one row each). Total(4×4+4×3+8)×30=1080. `decode_tssi_tracking_element` deliberately requires this complete bitmap and exact extent, rejects foreign AID and unsupported bitmap/size, and decodes all rows as signed codes.

## Exact transformations

`rtw89-phy.c:4318-4758` supplies the CCK, non6G OFDM,6G OFDM, non6G trim and6G trim channel-group ranges. Each is ported verbatim as interval data. EXTRA_GROUP entries average the adjacent signed codes using C/C++ integer division toward zero. Source's uncovered-channel fallback group0 is preserved; this helper does not authorize such a channel. A separate regulatory/channel-plan validator is still required.

`rtw89-phy.c:4760-4821` fills two-path DE arrays: RTL8922A's generic CCK and OFDM DE fields remain zero; CCK20/40 equal signed(efuse CCK+trim), OFDM20/40/80/160/320 equal signed(efuse OFDM+trim), original efuse CCK/OFDM and trim occupy their own fields. Four OFDM difference arrays remain zero as in the initialized source payload. Signed sums wrap to their low8 bits; the codec converts through uint8_t explicitly.

`rtw89-phy.c:4823-4947` selects the tracking rows from channel subband, uses multiplier+1 for8922A, saturates the positive half at delta index29, uses down indices1..29 in descending locations127..64, negates down values, and reverses every four-byte output chunk. Thermal255 produces pg_thermal0x38 and a zero128-byte table **because the pinned source explicitly defines that fallback**. Otherwise pg_thermal is the raw factory byte, and both128-byte tables are filled from firmware tracking deltas. No live thermal measurement is required by this exact payload constructor.

`rtw89-fw.c:8523-8561` supplies LE16 length300, PHY, center channel, bandwidth, band, HWTX1 for8922A, CV, NORMAL mode0 and RFE. HWTX1 is a calibration transmit capability, so payload preparation does not authorize transmission.

## What remains caller-owned

`prepare_tssi_normal_payload` accepts explicit thermal_group0 for2G;0/1/2 for5G source bands1/3/4;0/0/1/1/2/2/3/3 for6G subbands0..7. It checks group bounds against band but does not derive `subband_type` from the complete channel context. The caller must provide a validated channel's subband mapping, selected firmware version/AID and silicon CV, board map identity, explicit initialized TSSI DE trim state, and applicable radio/gain/power/regulatory tables. MAC scheduler stop/resume, RF receive-mode waiting, coexistence notifications and serialized RFK completion remain outside this codec.

The existing `rfk_tssi_normal_payload` and `encode_rfk_h2c(RfkCommand::tssi,...)` remain blocked. A successful offline payload build alone is not proof that all runtime prerequisites or factory provenance have been established.

## Verification

`c++ -std=c++17 -Wall -Wextra -Werror -Iinclude tests/tssi_protocol_test.cpp -o /tmp/archer-tssi-protocol-test` and the binary pass. Tests use the local real RF map and TXPWR_TRK element to verify field extraction and board values, then synthetic tracking rows for signed interpolation, positive saturation, negative indexing and chunk reversal. Tests also exercise erased thermal's sourced fallback, missing/foreign/malformed inputs, invalid subband/signed values, and the unchanged send guard. No hardware I/O was performed.
