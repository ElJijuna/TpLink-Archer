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

## Stage 14: PHY trim decoding and firmware table discovery

`tools/phy_calibration.py` now interprets the physical PHY region. On this
adapter both thermal trim values are programmed: path A=-1, path B=0.
Thermal format uses bit 6 as positive sign and bits 5:0 as magnitude; bit 7
is not the sign. The PA/PAD programmed flag is derived from offset 0, and
PA/PAD values are split into low 2G/high 5G nibbles at the source offsets.
Path A PA=(0,1), PAD=(1,3); path B PA=(14,2), PAD=(2,1). These values are
raw calibration codes, not dB or transmit-power settings. They have not been
written to RF registers. Output: `phy-calibration-observation.json`.

The retained RTL8922A chip definition has NULL host BB/RF/NCTL tables and a
NULL FEM setup callback; RFE parameters are loaded from firmware elements.
RFE type 1 alone does not identify all external-component programming or
supply a complete table selection key.

Downloaded `rtw89/rtw8922a_fw-1.bin` from the upstream Linux Firmware GitLab
repository for offline analysis only. The 1583434-byte package contains five
MFW image records and nineteen appended elements. Structural inventory:
`linux-firmware-elements.json`; source hashes: `radio-stage14-evidence.json`.
No image was uploaded and no register-pair sequence was executed. The package
was fetched from main and is hash-bound but not commit-pinned. It is not
established to match the currently resident firmware or all newer source APIs.
WHENCE identifies the rtw89 license, retained locally from
`LICENSES/LICENCE.rtlwifi_firmware.txt`; binaries remain ignored local artifacts.

`tools/firmware_elements.py` validates image directory bounds/non-overlap,
32-byte element headers, payload bounds, register-pair divisibility, and
TX power entry sizes/counts. Located tables contain 533 BB pairs, 412 gain
pairs, 2067 radio-A pairs, 2089 radio-B pairs and 1200 NCTL pairs. These
include encoded table conditions/delays; they must not be treated as a flat
list of direct USB register writes.

The offline selection pass only considers generic AID=0 power elements. It
prefers matching RFE=1 over default RFE=0 with the source's last-match rule.
This package provides default candidates for nine power element types, not
RFE=1-specific candidates. Actual chip AID is still unknown and must be read
through the XTAL serial interface before validating final element selection.
AID is the silicon analog identifier here, not an association identifier.

Six C++ suites and 29 Python tests pass, including signed magnitude, erased
trim gates, page boundaries, malformed MFW extents, partial register entries,
TX-power size mismatch and RFE/default precedence. The next step is XTAL-SI
identity acquisition, then a condition-aware BB/RF table interpreter tied to
that identity, board calibration and the selected firmware feature set.

## Stage 15: physical analog identity and AID-aware element selection

Added `make identity-universal` and `build/archer-identity-universal` (run from
project root). It holds the shared adapter lock, uses the verified WLAN IO
window, and issues only normal BE XTAL-SI reads with chip selector 0 and zero
write bitmask. Only CV and CHIP_ID_L/H offsets are exposed. Busy/inaccessible
controls fail before command submission; bounded polling, reply address, mode
and chip selector are checked. There is no XTAL configuration write or timeout
reset. The command/data status latch is not restored to avoid retriggering it.

Physical ARM observations: XTAL CV raw=0x11, low ID=0x33, high ID=0x69.
Digital CV=1, analog CV=1, AID=0x6933. WLAN IO returned to 0x001004fd.
The three raw identity bytes are retained as a regression fixture; full trace
and source hashes are in `chip-identity-observation.json`. Intel compiles but
has not been hardware-tested.

The offline MFW inspector now accepts an optional physical AID argument:
`python3 tools/firmware_elements.py artifacts/rtw8922a_fw-1.bin 0x6933`.
It filters generic/matching AID elements and follows first-match register-table
selection independently from last RFE-match/default power selection. Four-slot
radio index bounds are enforced. Public output now uses `power_candidates`
instead of the obsolete `generic_aid_power_candidates` name. The physical
selection report is `linux-firmware-elements-aid6933.json`.

For this retained package, five register-table candidates are generic AID=0:
BB, BB gain, RADIO_A in slot rf_radio[1], RADIO_B in slot rf_radio[0], and NCTL.
The slot order comes from element metadata, not the radio name. All nine
selected power candidates use default RFE=0. Identity acceptance is not proof
of full runtime compatibility: the table interpreter must still handle its
conditional/delay instructions, and the source/firmware pairing and runtime
firmware feature set remain unvalidated.

Seven C++ suites and 31 Python tests pass. Tests include physical identity
replay, busy/timeout/mismatched XTAL replies, generic versus physical AID,
radio-slot first-match behavior, invalid AID/index bounds and power precedence.
No RF table or radio configuration was written in this stage.

The next dependency is the upstream phy.c condition/delay interpreter.
Its download was not executed: automatic approval review failed because of
an account usage limit, rather than deciding that the action was unsafe.
No alternate network path or approval bypass was attempted. Local selection,
regression tests and evidence recording were completed using available files.

## Stage 16: condition-aware PHY table interpreter

The previously blocked source download succeeded after the user's request to
continue. Retained upstream phy.c/phy.h are hash-bound in
`phy-table-plan-evidence.json`; no approval bypass or alternate download path
was used. They are not yet pinned to a Git commit.

`python3 tools/phy_table.py artifacts/rtw8922a_fw-1.bin 0x6933 1 1`
now resolves headline priorities and IF/ELIF/CHECK/ELSE/END using the original
Linux algorithm for physical AID 0x6933, RFE 1, digital CV 1. It emits an
offline action plan and rejects unsupported/nested/malformed programs rather
than attempting writes. BB delay sentinels and BYPASS_CR_DATA are decoded.
RF-v1 only treats address 0xfe as a delay, unlike the older generic RF
callback; addresses 0xfd..0xf9 remain RF writes in this mode. The planner also
marks RF entries at addresses >=0x100 for the later firmware-store path.

The chosen headline is 0x00010001. Selected action counts:
BB=174, RADIO_A=1085, RADIO_B=1083, NCTL=1200. Gain table entries are excluded
because their callback interprets structured gain fields rather than flat BB
writes. The full plan is local at `artifacts/phy-plan-stage16.json`; a compact
tracked summary is `phy-table-plan-observation.json`.

Independent check: compiled the original `rtw89_phy_sel_headline` and
`rtw89_phy_init_reg` bodies from the downloaded source with minimal type/macro
stubs and a recording callback. Every selected address/value pair and its
order match this Python interpreter for all four real tables. Retained harness:
`artifacts/phy-reference-stage16.cpp`; results:
`phy-table-reference-check.json`. This is algorithm parity on the retained
package, not verification of register accessibility or radio behavior.

Seven C++ suites and 35 Python tests pass. Cases include exact and fallback
headline priority, ELIF selection, skipped ELSE, post-branch common writes,
BB versus RF-v1 delay semantics, bypass handling and malformed branches.
No table has been executed and no RF setting was changed.

Next: implement the structured gain decoder and map BB/RF access semantics,
including RF-v1 address selection and firmware register-store commands. MAC
normal initialization, firmware compatibility and calibration completion still
need validation before this offline plan can become a hardware bring-up path.

## Stage 17: structured BE gains and physical RF-v2 encoding

Added `tools/bb_gain.py MFW.bin AID RFE`. The real package contains 412 gain
entries. BE decoding creates 1110 final signed fields for LNA/TIA gain,
OP1DB and RPL offsets. Sixteen bypass records are intentionally ignored by
the source callback; 48 external-FEM entries are not applicable to RFE=1.
None of these selected records requests a register write. BE configuration
kind 15 is represented separately as a masked BB write candidate, not silently
interpreted as gain data; the actual table does not use this kind.

Compared the full zero-initialized output arrays against the original BE gain
callbacks compiled with minimal standalone type/macro definitions. All 1512
array positions match, including zero/unset positions and all decoded fields.
Results: `bb-gain-reference-check.json`; summary: `bb-gain-observation.json`.
The reference harness and full decoded arrays remain local artifacts, hash-bound
in `rf-access-stage17-evidence.json`.

Important distinction: the RF table callback named config_rf_reg_v1 is used
for table delay/store semantics, but RTL8922A's chip operations select
read_rf_v2/write_rf_v2 for actual physical access. The new pure C++ header
`include/archer/rf_access.hpp` models this latter access without issuing I/O:

- BE BB logical registers map to device addresses with base 0x20000.
- RF selector bit 16 set: direct path uses RF bases 0xe000/0xf000,
  low-byte RF address shifted by 2, 20-bit masked access through BB space.
- Selector clear: analog HWSI full writes wait for bit 29 clear at logical
  0x2c24/0x2d24, then encode RF data in bits 27:8 and address in bits 7:0
  into logical 0x2ae0/0x2be0.

The helper rejects paths beyond A/B, unsupported address bits, values wider
than 20 bits and unaligned BB addresses. It only plans full RF writes; masked
RF writes require the documented read/modify/write protocol, which is not
implemented. No native RF executor, busy polling backend, or BB-power
transition is provided yet, so these plans cannot accidentally write hardware.

Eight C++ suites and 39 Python tests pass. New cases cover signed gain fields,
path/bandwidth selection, RPL halves and overwrites, ignored FEM data, malformed
gain types, direct RF address mapping, analog payload encoding and bounds.
The source files were fetched from master and hash-bound; revision pinning is
still needed before distributing an adapted driver.

Next required work: implement bounded hardware BB/RF access with explicit
power-state prerequisites, then normal MAC/BB initialization and firmware RF
parameter-store commands. Only after those are validated can the selected
tables and calibration be applied. There is still no radio initialization,
scan, association or usable network interface.
