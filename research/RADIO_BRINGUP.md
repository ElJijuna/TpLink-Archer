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
