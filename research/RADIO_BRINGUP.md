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

## Stage 18: physical BB-state check and bounded RF transport

`archer-identity-universal --bb-state` now reads reset, DMAC BB and HWSI
control/status registers inside the verified WLAN IO window. Physical ARM
observation: FEN_RST_ENABLE low reset-release bits are clear and
DMAC_SYS_CR32B=0. HWSI addresses returned 0xeaeaeaea/0xdeadbeef. The source's
normal BB-enable prerequisites are absent. WLAN IO returned to 0x001004fd.
These reads do not mean that any analog RF read or write succeeded; none was
requested. Evidence: `bb-state-observation.json`, `bb-state-evidence.json`.

The new `include/archer/rf_transport.hpp` provides a bounded RF-v2 transport
over RegisterBus, with no power/reset enabling behavior. Before RF commands,
it requires observed chip ID, active system state, WLAN IO enable/status,
BB reset release, cleared BB isolation and the expected PHY0 DMAC field.
Unclocked/inaccessible register sentinels fail closed. This conservative
prerequisite policy is not a complete MAC/BB readiness definition.

Analog reads use HWSI-v2: check command latch/idle, select control mode 1,
encode the eight-bit address, trigger read, wait for RDONE and read the 20-bit
value. Polling is bounded to six attempts separated by the bus pause (10 ms
on the native bus, intentionally slower than kernel microsecond polling).
Poll bits are cleared on completion, timeout and transfer exceptions. Command
address/trigger latches are not restored, matching the source cleanup boundary.
Cleanup failure is reported explicitly.

Full-write submission supports direct RF masked updates preserving upper bits,
and analog HWSI writes after idle polling. It does not claim write completion
or calibration. Masked analog writes are not exposed. No native CLI exposes
arbitrary RF writes, and this backend has only been exercised against the
simulated RegisterBus; physical RF execution remains unvalidated.

Nine C++ suites and 39 Python tests pass. The new transport tests cover direct
and analog encoding, upper-bit preservation, reset/isolation/DMAC/WLAN guards,
busy timeouts, inaccessible status, read completion timeout and transfer-failure
cleanup. Native identity inspection and the transport test compile for Intel
and ARM; hardware inspection was ARM only.

Next: implement the documented MAC/BB-enable boundary and account for BB MCU
and normal firmware/MAC initialization before using this RF backend. Current
resident firmware readiness alone did not initialize BB. Do not interpret this
stage as a working radio, scan or network interface.

## Stage 19: temporary BB-enable diagnostic and firmware-response blocker

Added `make bb-probe-universal` / `build/archer-bb-probe-universal`.
The diagnostic locks the adapter, confirms PHY_CAP and AID 0x6933, then is
prepared to release PHY0 BB reset and set DMAC_SYS_CR32B=0x7ff97ff9 according
to rtw8922a_mac_enable_bb_rf. Its temporary window accepts only the observed
uninitialized state, validates latched controls and restores BB reset before
the original DMAC value. Callback/transfer failures still attempt both control
restorations. This is not the full firmware/BB-MCU/MAC initialization sequence,
and restoring control bits does not promise restoration of internal BB state.
No RF configuration writes are exposed: the callback only inspects HWSI and
attempts RF register-0 reads if accessible.

The physical attempt on this date never entered that BB window. Its initial
GET_FEATURE/PHY_CAP mailbox query timed out. Thus no BB reset, DMAC configuration
or RF command was submitted. Hardware ID, power and firmware-state bits still
matched the earlier snapshot, demonstrating that firmware state=3 alone does
not establish a responsive command processor.

Refactored `with_wlan_io_window` from the PHY_CAP query wrapper so that an
active mailbox can be inspected without submitting another firmware request.
New `archer-mailbox-state-universal` showed H2C control=1, C2H control=0,
firmware-owned counter=0x24 and host-owned counter=0x2d: the request remained
pending. Old C2H data was not acknowledged or treated as a new reply.
The inspection wrote only the WLAN IO access bit and restored it to its
original 0x001004fd register value. It neither retried the request nor cleared
the trigger/counters or reset the adapter. Evidence:
`bb-window-attempt-observation.json`.

A physical USB disconnect/reconnect was requested to restore a responsive
baseline before continuing the BB experiment. This is an observed runtime
blocker rather than a signing, compilation or test failure. Keep the sequence
ledger; a reconnect is not authorization to recycle old sequence numbers.
After reconnection, inspect fresh state and confirm a new PHY_CAP response
before running the temporary BB diagnostic.

Ten C++ suites and 39 Python tests pass. Window tests cover callback failure,
partial configuration transfer failure, cleanup failure continuation, preservation
of unrelated reset bits and refusal of unknown/preinitialized states. Generic
WLAN-window tests prove inspection of a pending mailbox sends no new mailbox
command. Relevant native tools compile for both Intel and ARM.

## Stage 20: reconnect exposes the installer CD mode

After the user confirmed physical reconnection, 37ad:0104 was absent. Instead,
IORegistry reported Realtek DISK 0bda:1a2b at location 0x02100000. Its optical
media is RTK Driver Storage Media, LUN 0, disk16, 74661888 bytes, not writable.
The probe stopped before register access because the expected Wi-Fi ID was
absent. This is a mode-switch/recovery boundary, not a failed BB-enable result.

The upstream USB_ModeSwitch StandardEject sequence uses allow-removal followed
by START STOP UNIT with LOEJ=1/START=0, with no block-data transfer. Added pure
CBW/CSW encoders and an experimental native helper restricted to one
0bda:1a2b SCSI/Bulk-Only USB2 installer interface and endpoints 0x0b/0x8a.
Its normal and seize opens both failed with kIOReturnExclusiveAccess before
sending storage commands. It does not terminate/unload the storage driver.
Exit 3 means eject was submitted but response unavailable; enumeration must
independently establish Wi-Fi mode even if command status succeeds.

A second experimental helper traverses the optical medium's ancestors to
verify the Realtek USB identity and attempts the native MMC/SCSI interface.
The direct Type05 matching attempt found no registered service; traversal from
IOCDMedia found it, but the MMC client creation was unavailable. No SCSI task
executed. These tools compile for Intel/ARM, but mode switching remains
unvalidated and they are not integrated into normal probe/driver startup.

System recovery attempts: diskutil could not resolve disk16; direct raw-media
open was denied; noninteractive sudo reported authentication required. No
password was requested or captured by the agent. The user was asked to run
`sudo diskutil eject /dev/disk16` in Terminal and report the result. That disk
identifier was verified for this observation only; rediscover it after any
further USB/device changes before repeating manually.

Evidence: `reconnect-mode-observation.json`. Eleven C++ suites and 39 Python
tests pass. New checks cover zero-length/LUN-0 CBW fields, allow-removal/eject
opcodes, CSW signatures/tags/residue and phase errors. No mode-switch command,
BB enable, RF configuration, firmware upload or disk-data write succeeded in
this stage. Keep the existing sequence ledger when the adapter returns.

References used for the recovery command semantics:
https://github.com/NetworkManager/usb_modeswitch/blob/master/usb_modeswitch.c
https://www.draisberghof.de/usb_modeswitch/bb/viewtopic.php?t=3010

## Stage 21: optical ioctl recovery helper

The user reported that authenticated `diskutil eject /dev/disk16` also failed
to find the disk. Fresh IORegistry inspection still identified the same
read-only RTK Driver Storage Media, BSD disk16, major 1/minor 44, under the
Realtek 0bda:1a2b ancestor. Disk Arbitration resolution is therefore not a
usable recovery method in the current observed environment.

Added `make optical-eject-universal` and a guarded native helper:
`build/archer-optical-eject-universal --inspect | --eject-installer`.
Inspection succeeded on ARM. The eject operation has NOT been executed or
validated yet; it requires the user to authenticate with sudo in Terminal.

The helper discovers IOCDMedia dynamically, checks its exact RTK installer
name and Realtek USB ancestor, requires one matching whole/removable/read-only
medium, validates the BSD disk identifier, and opens only that block-device
path with read-only/nonblocking/no-follow flags. It verifies fstat device
numbers and USB ancestry again before issuing the macOS DKIOCEJECT ioctl.
No arbitrary path argument, SCSI write, block-data write, USB driver unload,
firmware command or BB configuration is exposed. An accepted ioctl still needs
independent confirmation that USB Wi-Fi ID 37ad:0104 reappeared.

The implementation uses the SDK's sys/disk.h DKIOCEJECT definition. It avoids
Disk Arbitration for discovery/dispatch, but kernel eject support or privileges
can still prevent it from succeeding. Any ioctl error is surfaced without a
forced unmount, alternative write-mode open, retry or device reset.

The manual command, run from the project root, is:
`sudo ./build/archer-optical-eject-universal --eject-installer`.
Unlike the earlier manual diskutil command, this does not hardcode disk16 and
will rediscover/revalidate the device after subsequent USB changes. Do not
remove the sequence ledger during recovery.

Twelve C++ suites and 39 Python tests pass. New pure tests cover supported BSD
identifiers and refusal of partitions, raw/arbitrary paths, writable media,
nonremovable media and non-whole media. The native helper compiles for Intel
and ARM; only discovery has been executed against hardware. Evidence:
`optical-eject-observation.json`.

## Stage 22: accepted optical eject did not restore Wi-Fi

The user reported DKIOCEJECT status 0. Subsequent inspection found a new
IORegistry device instance at the same port, but still Realtek DISK
0bda:1a2b. This proves reenumeration, not a switch to network ID 37ad:0104.
The latter remains absent, so no WLAN/register/BB command was attempted.

Improved `archer-mode-switch-universal --inspect` to read the cached device
configuration descriptor without opening the kernel-owned storage interface.
The real 32-byte descriptor is retained as a regression fixture. It reports
08/06/50 interface 0, bulk IN 0x84 and OUT 0x05, 512-byte packets. This replaces
the earlier unverified 0x8a/0x0b endpoint assumption taken from other Realtek
models. Eject mode now discovers exactly one bulk IN and OUT pipe after open;
it does not assume endpoint numbers from a different chip.

The upstream USB_ModeSwitch StandardEject reference defines allow-removal and
START STOP UNIT packets. Also inspected the already-downloaded vendor
WifiAutoInstallSrv.exe with LLVM's offline PE disassembler. Routine 0x402466
contains a branch building a six-byte SCSI CDB with opcode 0x1b and byte 4=2,
then IOCTL 0x4d014. This corroborates the CDB format but does not prove that
branch executes on this adapter. Another branch uses IOCTL 0x74808. No Windows
code was executed. Native REA decompilation was not performed: Ghidra is not
configured and the previously observed Hopper license expiry is unresolved.

An administrator launch through the macOS authorization dialog failed before
USB access while opening the sequence ledger; that build did not include
underlying errno. The ledger is intact. Added errno diagnostics; no alternate
ledger or permission changes were used to evade the failure. The validated
way to supply a fresh test result is to run from the user's Terminal:
`sudo ./build/archer-mode-switch-universal --eject-installer`.
This direct USB experiment is not yet proven to acquire the kernel-owned
interface even as root. If it remains exclusive, stop and retain the failure;
do not repeatedly eject, alter global storage drivers or invent vendor commands.

Twelve C++ suites and 39 Python tests pass. Added physical storage descriptor
replay. Eject mode compiles for Intel/ARM; full mode switching remains blocked.
Evidence: `installer-switch-stage22-evidence.json`. The original kernel
optical helper continues to report only eject acceptance, never Wi-Fi success.


## Stage 23 — Wi-Fi mode recovered; BB accessible, RF read incomplete

The user's 0xe00002e8 CSW read result is an overrun. A subsequent
512-byte receive captured 18 bytes (`700005000000000a00000000200000000000`),
with fixed-format SCSI sense shape, not a matching CSW. Its originating
transaction is unknown. The helper stopped before eject. CSW parsing now
requires exactly 13 bytes as specified by BOT section 5.3.3; padded transfers
are rejected. Regression coverage includes the observed sense packet.

Added explicit `--reset-bot-eject-installer`: class/interface request
21/ff/0000/0000/0000, then clear HALT on bulk IN and OUT in that order,
per [USB BOT sections 3.1 and 5.3.4](https://www.usb.org/sites/default/files/usbmassbulk_10.pdf).
It retains target and endpoint guards and bounded calls. Hardware execution
of this option stopped at target selection because the installer had already
disappeared; neither reset nor CBW was sent. The adapter was then observed
as 37ad:0104. Causality of the mode transition remains unestablished.

Fresh startup reads confirmed chip 0x71, digital revision 1, firmware field 3.
The BB probe completed PHY_CAP and AID 0x6933 checks, entered the temporary
BB window, and read HWSI status A/B=0 and controls A/B=0x39000. These no
longer returned inaccessible sentinels. Path A RF address 0 did not set the
completion bit within six bounded polls. Path B was not attempted. Cleanup
verified HWSI latch low bits cleared, FEN restored to 0, DMAC to 0 and WLAN
IO to 0x001004fd. This establishes BB register access, not functional RF.

Next: establish missing HWSI/RF initialization prerequisites from reference
source before further hardware RF commands. Do not extend timeouts or apply
whole radio tables without evidence. Twelve C++ suites and 39 Python tests
pass; native helper builds for Intel and ARM. Evidence hashes and limits:
`bringup-stage23-observation.json`.


## Stage 24 — Bounded HWSI control and BB clock hypotheses

Compared local Linux source and selected BB table against stage 23. BB table
pairs 240/241 program logical 0x2adc/0x2bdc to 0x08039000, whereas live
controls were 0x39000. The meaning of bit 27 is not named in the register
header. Added an explicit temporary control window, guarded against any
other baseline, with restoration on success, callback failure and partial
write failure. The physical candidate latched but RF path A address 0 still
timed out. Both controls restored to 0x39000 with readback.

A separate prerequisite snapshot read BBCLK=0x1a, asynchronous reset register
0x20704=0x601e0d02, MEM_PWR_CTRL=1, and HWSI status A/B=0. BBCLK bit 2 was
clear. The source's bb_postinit sets B_CLK_640M before RF initialization. A
second explicit experiment enabled only that clock bit alongside the same
HWSI controls. All candidate values latched, but RF completion again stayed
clear. Clock 0x1a, HWSI controls, FEN=0, DMAC=0 and WLAN IO=0x001004fd were
restored and verified. Path B RF was not attempted after path A failure.

These changes are insufficient individually/as tested together; they do not
establish that either change is unnecessary in a complete initialization.
The experiment does not initialize BBMCU, upload firmware, apply a full BB
or radio table, select a channel or transmit.

Reference order in rtw89_core_start is MAC preinit, optional BB preinit
(no-BBMCU chips), BB AFE, MAC init, BB table/gain setup, BB postinit, RF setup.
RTL8922A has bbmcu_nr=1; rtw89_mac_partial_init invokes BB preinit before
firmware download where requested, explicitly to avoid touching registers
affecting a later download. Its BB preinit includes reset sequencing,
memory sleep controls and BBMCU initialization registers. Therefore copying
that routine into the current resident-firmware diagnostic is not yet a
validated recovery path. Source/firmware pairing and resident BBMCU state
must be established first.

CLI options: `--inspect-prerequisites` reads the fixed control snapshot
inside the temporary BB window and sends no RF command;
`--table-hwsi-control` and `--clock-hwsi-control` are explicit experiments,
not production initialization. The default probe remains unchanged.
Thirteen C++ suites and 39 Python tests pass. Evidence and hashes:
`hwsi-stage24-observation.json`.
