# RTL8922A RFK wire protocol

Implemented as a pure, offline codec in `include/archer/rfk_protocol.hpp` with synthetic fixtures in `tests/rfk_protocol_test.cpp`. Codec development performed no hardware I/O; a later stage30 PRE_NOTIFY response is documented below. The selected target is USB 37ad:0104 / RTL8922AU, firmware 0.35.41.0; this codec follows the pinned rtw89 source snapshots in `artifacts/`, not a claimed hardware capture.

## Version and payload evidence

- `artifacts/rtw89-fw.c:908-930`: RTL8922A PRE_NOTIFY V0 starts at all versions, V1 at 0.35.31.0, V2 at 0.35.49.0; RXDCK_V0 applies strictly before 0.35.42.0. Later feature versions override earlier PRE_NOTIFY versions in `rtw89_fw_h2c_rf_pre_ntfy` (8295 onward). Codec intentionally supports only 0.35.31.x through 0.35.48.x, rejecting earlier/later versions instead of guessing their PRE_NOTIFY layouts. Current firmware selects PRE_NOTIFY **V1**, RXDCK **V0**.
- `artifacts/rtw89-fw.h:4949-5003`, `rtw89-fw.c:8343-8409`: PRE_NOTIFY V1 is 76 bytes, all LE32: `channels[2][3]`, `bands[2][3]`, MLO mode, current channel[2], current band[2], PHY index, MLO 1+1 boolean. Arrays are path-major. Current entries are selected from each path's RFK MCC table index. Zeroed unused table slots are valid (`rtw8922a_rfk_init`, 2340 onward).
- `artifacts/rtw89-fw.h:5102-5126`, `rtw89-fw.c:8722-8800`: DACK is `{length=3, phy, type=0}`. RXDCK V0 is `{length=8, phy, is_afe=0, kpath=RF_AB(3), band, bandwidth, center_channel, debug}`. From 0.35.42.0 RXDCK adds `is_chl_k` and length becomes 9. On V0, `is_chl_k` is intentionally omitted, matching source.
- `artifacts/rtw89-fw.h:4877-4905`: H2C category 2 (OUTSRC), class 0xb (RF_FW_RFK), functions PRE_NOTIFY=8, DACK=5, RXDCK=6. Source asks for neither receive ACK nor done ACK. `encode_rfk_h2c` returns the 8-byte H2C header plus payload; use existing `wrap_h2c_usb_be` only at the transport boundary.

## Completion and prerequisites

`artifacts/rtw89-phy.h:185-206`, `rtw89-fw.h:5333-5337`, `rtw89-phy.c:4061-4076` establish C2H category 2, class 9, function 0: 8-byte C2H header then state byte and report-version byte as a minimum source prefix. The decoder accepts exactly the10-byte source shape or14-byte stage30 observed shape, retaining the extra four bytes as opaque data. States from `rtw89-core.h:6570-6576` are START=0, OK=1, FAIL=2, TIMEOUT=3, H2C_CMD_ERR=4. Only OK proves reported success. MAC transport REC_ACK/DONE_ACK and RFK log reports do not prove calibration completion. The codec rejects malformed state reports and ignores valid foreign C2H categories/classes/functions. Tracker treats START as nonterminal and rejects conflicting terminal reports; Linux's handler signals its completion object for every state but its waiter accepts only OK.

The RFK state message has **no sequence, function being calibrated, or PHY identity**. Serialization, receive draining, and arming the waiter before submitting H2C are mandatory; a stale RFK state cannot be distinguished on wire. One tracker per sole outstanding calibration is a software prerequisite, not a demonstrated device guarantee. Unknown report-version values are exposed to callers; the supplied source does not gate parsing by report version.

`artifacts/rtw8922a.c:2349-2366` late init performs PRE_NOTIFY (5 ms), DACK (58 ms), then RXDCK (128 ms, except SER handling), using PHY0 and channel context 0; DBCC also uses PHY1. `rtw89-phy.c:3999-4034` multiplies these waits by four for USB (20/232/512 ms). A timeout is failure, never inferred success from elapsed delay. No timeout runner is introduced here.

RFK requires an initialized MAC/DLE command/data path, firmware and BB MCU readiness, loaded RF/BB tables, the correct hardware/RFE profile, and valid channel/MCC/MLO context prepared by the caller. The codec checks wire ranges but does not validate a regulatory channel plan, derive the channel context, program RF registers, install USB receive processing, or prove those prerequisites. Firmware boot alone does not establish them. CV1/RFE1/AID6933 identity does not itself supply the missing channel context. DACK has no channel field, while late-init sequencing and RXDCK still require that context.

## Verification

`c++ -std=c++17 -Wall -Wextra -Werror -Iinclude tests/rfk_protocol_test.cpp -o /tmp/archer-rfk-protocol-test` followed by the binary passes. Coverage includes exact PRE_NOTIFY word order/current-table selection, H2C identifiers and ACK bits, DACK/RXDCK lengths and the 0.35.42 boundary, unsupported firmware, invalid PHY/channel/table fields, unrelated ACK/log messages, malformed/truncated reports, START, all terminal failure states, duplicates, and conflicting completion states. Initial fixtures are synthetic; the later physical PRE_NOTIFY regression below verifies one completion shape, while calibration commands remain unverified.

## Channel RFK extension (firmware 0.35.41.0)

`rtw89-fw.c:8576-8720` and `rtw89-fw.h:5073-5100` define three additional 8-byte payloads, now encoded and tested:

| Command | Function | Ordered bytes |
| --- | --- | --- |
| TXGAPK | 4 | length8, ktype2, PHY, RF_AB3, band, bandwidth, center channel, cut version |
| IQK | 1 | length8, ktype0, PHY, selected kpath, band, bandwidth, center channel, cut version |
| DPK | 3 | length8, PHY, enabled1, RF_AB3, band, bandwidth, center channel, debug flag |

IQK's legacy format is two LE32 words (PHY and DBCC), selected only for RTL8922A versions strictly before0.35.28.0 (`rtw89-fw.c:916`). Firmware0.35.41.0 therefore uses the newer 8-byte format. The codec's previously documented0.35.31.x–0.35.48.x bound excludes legacy IQK. No further TXGAPK/DPK payload feature gate appears in the pinned firmware feature map. All three commands use category2/class0xb and no transport ACK flags.

The IQK caller must explicitly supply the kpath bitmap derived from its MLO/DBCC context. `rtw89-phy.c:8865-8897` returns RF_A/RF_B for MLO1+1 one-RF mode, RF_AB for one-RF0+2/2+0 modes, and other mappings for two-RF modes. This RTL8922A codec accepts only A=1, B=2, AB=3 and never infers an MLO mode or applies another chip's C/D paths. TXGAPK and DPK use RF_AB unconditionally in the source. CV1 must come from verified silicon identity.

### TSSI NORMAL remains blocked

`rtw89-fw.c:8523-8556` builds TSSI using the packed300-byte `rtw89_h2c_rf_tssi` (`rtw89-fw.h:5034-5071`). Its length is LE16, followed by PHY/channel/bandwidth/band/HWTX/cut, sixteen signed two-path DE arrays, two thermal bytes, two128-byte thermal-offset tables, mode, and RFE type. NORMAL mode is0 (`rtw89-core.h:6911-6914`); RTL8922A sets HWTX enable true. This command can cause firmware hardware transmissions as part of calibration.

A valid TSSI payload requires the source's channel-specific transformations, not just CV/RFE/channel scalars:

- `rtw8922a.c:752-775`: per-path efuse thermal, CCK and MCS TSSI group data.
- `rtw89-phy.c:4760-4821`: channel-group selection/interpolation, per-path trim, efuse CCK/OFDM DE and bandwidth-specific signed sums.
- `rtw89-phy.c:4823-4947`: selected firmware TX power tracking delta tables by subband/path, thermal-dependent offsets and scale, with each four-byte output chunk reversed into the two128-byte tables.

Those host-data preparation functions are not implemented here. `rfk_tssi_normal_payload` fails closed with a prerequisite error; `encode_rfk_h2c` explicitly rejects TSSI to prevent accidentally sending a fabricated zero-filled payload through this API. No default efuse, thermal value, or tracking table is invented. A future implementation must validate both the efuse and firmware table inputs against this physical adapter, prepare all dependent host state, and test the resulting300-byte payload before enabling TSSI.

`rtw8922a.c:2389-2411` orders channel RFK as coexistence start notification → stop scheduled TX → wait RF A/B receive mode → PRE_NOTIFY5ms → TXGAPK54ms → IQK84ms → TSSI NORMAL20ms → DPK34ms → RXDCK32ms on PHY0 → resume scheduled TX → coexistence stop notification. USB uses the same4x timeout multiplier described earlier. Separate channel RFK from the shorter late-init sequence. TSSI's missing preparation means the complete channel RFK flow remains unavailable even though TXGAPK/IQK/DPK wire codecs exist. Callers must also establish the gain/radio/power tables, valid regulatory channel context, MLO/coexistence state, scheduler suspension/resumption, and RF receive-mode prerequisites; payload encoders do not perform them.

Extended tests cover exact TXGAPK/IQK/DPK bytes and H2C identifiers, explicit IQK path validation, cut-version overflow, legacy/unsupported firmware rejection, and TSSI fail-closed behavior. The full RFK protocol test binary passes under C++17 with `-Wall -Wextra -Werror`; the stage30 PRE_NOTIFY physical regression capture is documented below.


## Stage30 physical PRE_NOTIFY response correction

`artifacts/rfk-stage30/rx-0.bin` contains38 bytes: a24-byte BE USB RX descriptor with packet type10 and14-byte C2H payload. The exact C2H bytes are `26 00 01 49 0e 00 00 00 01 00 08 00 00 00`. Category2/class9/function0 identify an RFK state report; declared length14 matches the USB descriptor. State1 is OK and report version0. Bytes10..13 are `08 00 00 00`.

The original strict10-byte codec incorrectly rejected this genuine extension. The pinned `rtw89_c2h_rfk_report` source struct (`rtw89-fw.h:5333-5337`) defines only the state/version prefix, and `rtw89_phy_c2h_rfk_report_state` (`rtw89-phy.c:4061-4076`) reads that prefix without enforcing an exact struct size; it logs the entire remaining payload. Thus the observed14-byte report is consistent with the source handler's prefix parsing. No binary evidence presently establishes what the extra four bytes mean. Their value8 happens to equal the PRE_NOTIFY H2C function, but that coincidence is insufficient to establish command correlation, byte order, PHY identity, or a sequence field.

`decode_rfk_report` now accepts only the bounded10/14-byte shapes, still verifies the C2H declared length and valid state range, and preserves the extension as `RfkReport::opaque_extension`. It does not use that extension for command matching. Duplicate terminal reports must agree on state, version and opaque bytes; conflicting duplicates are rejected. Serialization/draining requirements remain in force because the tail's identity semantics are unknown.

`tests/fixtures/rfk-pre-notify-stage30-usb.bin` is an exact copy of the38-byte physical capture. Regression tests decode its USB envelope, check OK/version0 and the preserved four-byte tail, verify identical duplicates, reject conflicting tails, and reject unobserved11/12/13/15-byte state shapes. The C++17 Werror protocol tests pass. This establishes that firmware0.35.41.0 returned an RFK OK response following PRE_NOTIFY; it does not demonstrate DACK/RXDCK/IQK/TSSI calibration success or overall radio readiness.
