# Radio bring-up boundary

The current driver experiment has working register and bounded USB bulk
command/ACK exchange, but no initialized radio, scan or network interface.

## Stage 12 physical preflight

The existing transport inspection also reads SYS_WL_EFUSE_CTRL at 0x000a
(width 16) and three 16-bit USB MAC-address registers starting at 0x4078,
as used by rtw8922a_read_efuse_usb. Autoload status bit 5 is set. The USB MAC
is nonzero, unicast and not broadcast. A hash is recorded in
radio-preflight-observation.json; raw local traces are ignored by Git.
These observations do not validate the complete factory calibration map.

## Offline efuse preparation

`tools/efuse_map.py PHYSICAL_MAP NEW_RF_MAP` requires an exact 0x1300-byte
RTL8922A DDV physical map, skips the four-byte security-control prefix and
extracts page 1's 0x240-byte RF logical block. It follows the retained Linux BE
three-byte header, active-low word mask, page/offset and later-record overwrite
rules. It checks every header/word access, handles odd logical ranges and
rejects DAV requests. Tests use explicitly synthetic maps; no physical efuse
map has yet been acquired or validated with this parser.

## Required next hardware work

1. Implement bounded DDV efuse acquisition with cleanup for every error path.
   Linux enables power/isolation controls, sets burst mode, triggers reads,
   polls readiness and obtains four bytes at a time. It also changes the
   efuse access state. These controls need save/restore semantics before use
   against the resident firmware. This is not ordinary flat USB register RAM.
2. Decode board RFE type, crystal trim, country code, TSSI and gain offsets
   from the logical RF block; read the separate PHY-capability calibration
   region at 0x1700, length 0x38.
3. Reconcile firmware identity/features, normal MAC initialization, baseband
   tables and RF settings with that board data before sending RF calibration.
   The source late calibration calls pre-notification, DACK and RXDCK with
   firmware waits. Do not substitute zero calibration data or choose a board
   type from the USB product ID.
4. Add bounded completion tracking for each RF operation before scanning.

Source boundaries: rtw89-core.c rtw89_chip_efuse_info_setup;
rtw89-efuse_be.c physical dump and logical parser; rtw8922a.c USB MAC read,
efuse RF parsing, efuse block table and rfk_init_late. Source snapshots remain
hash-bound local references; a pinned revision and the relevant chip tables
are still required for a distributable implementation.

## Stage 13: physical efuse acquisition and raw RF interpretation

`make efuse-universal` builds the new Intel/ARM executable. Run from the project
root: `build/archer-efuse-universal NEW_DIRECTORY`. It holds the same adapter
ledger lock as the bulk diagnostic and uses the verified WLAN IO window.
Only RTL8922A revision code 1, active system state and the observed idle efuse
isolation configuration are accepted. The DDV control write contains only a
16-bit read address: no OTP programming mode or data-write operation exists.
The read window enables the documented isolation/power/burst controls, reads
the full 0x1300-byte DDV region plus 0x38 bytes at 0x1700, and checks control
restoration before saving complete maps. Each read is bounded; acquisition
has a 30-second deadline checked between words. Cleanup attempts every control
restoration even after an earlier cleanup transfer failure; errors are surfaced.
The read command/status and data latches are not restored or retriggered.

Physical ARM acquisition succeeded. The 576-byte logical RF extraction is
populated. `tools/rf_calibration.py` follows the packed rtw8922a_efuse layout
and reports raw factory fields and per-path TSSI, thermal and RX gain arrays:
RFE type code 1, crystal trim code 66 (0x42), board-info code 33 (0x21).
Country bytes are ff/ff and do not provide a country assignment. Raw codes
are not yet transformed into board/radio programming parameters.

Observed original/final controlled fields in isolation, debug, burst, power
and WLAN IO match; hashes and restoration checks are retained in
`efuse-acquisition-evidence.json`. Raw maps and register traces remain local
ignored artifacts. The logical extraction and raw interpretation tools do not
write the adapter. Unit tests exercise acquisition length, read-only command
words, timeout, data transfer failure, cleanup failure continuation, unsupported
revision, and synthetic logical decoding/field offsets. Six C++ suites and
23 Python tests pass. Intel is compile-tested; hardware execution is ARM only.

The next radio step is to interpret PHY calibration and RFE/FEM selection,
then implement the required baseband/RF tables and firmware calibration waits
for this board. Radio, scanning and association are still absent. The source
header was fetched from Linux master and hash-bound alongside retained source;
revision pinning and cross-file compatibility remain a distribution prerequisite.
