# DACK report log compatibility and quality

The firmware's terminal RFK OK message does not establish that every detailed calibration substep succeeded. Physical stage30 DACK logs contain nonzero timeout flags even though a separate terminal OK arrived. `include/archer/rfk_log.hpp` now parses bounded RFK log chunks and source-supported DACK version2 reports without turning that combination into a clean-calibration claim.

## Version and layout evidence

Pinned `artifacts/rtw89-fw.h:5157-5166` defines RUN_LOG type0 and RPT_LOG type1, each framed by a packed byte type plus LE16 content length. `rtw89-phy.h:170-206` identifies RFK log C2H category2/class8 and DACK function2. `rtw89-phy.c:3860-3892` iterates those chunks.

The local latest DACK struct (`rtw89-fw.h:5232-5254`) has118 bytes and an appended `adgaink_ex_d`. An earlier [primary wireless-next fw.h at commit8f7aa3d3c7323f4ca2768a9e74ebbe359c4f8f88](https://kernel.googlesource.com/pub/scm/linux/kernel/git/wireless/wireless-next/%2B/8f7aa3d3c7323f4ca2768a9e74ebbe359c4f8f88/drivers/net/wireless/realtek/rtw89/fw.h) contains the otherwise identical117-byte structure ending at `rck_d`. This supplies an exact historical layout for the captured117-byte content. Its coefficient arrays total106 bytes including the two version bytes; seven timeout bytes then precede `dack_fail`, two WBDCK values and RCK. The later118th byte adds ADGAINK extra data without shifting these fields.

The physical content begins `0d 02`: calibration implementation version13 and report-info version2. The pinned handler logs its expected report version2 at `rtw89-phy.c:3642-3646`. The codec requires info version2 and content length117 or118. It does not infer an exact firmware release from the calibration implementation version, does not reinterpret older report-version1 layouts, and rejects unsupported versions/sizes as unknown quality. Historical source body was inspected through the indexed primary-source result; a direct full-page fetch timed out, so no new local complete historical source artifact is claimed.

## Source offsets and physical stage30 values

Content offset0 is `fwdack_ver`,1 is `fwdack_info_ver`; offsets2..105 hold source coefficient arrays. The supported scalar fields are:

| Offset | Field | Stage30 |
| --- | --- | --- |
| 106 | ADDCK timeout | 0 |
| 107 | CDACK timeout | 0 |
| 108 | DADCK timeout | 0 |
| 109 | MSBK timeout | 3 |
| 110 | ADGAINK timeout | 0 |
| 111 | WBADCDCK timeout | 0 |
| 112 | DRCK timeout | 1 |
| 113 | DACK fail | 0 |
| 114..115 | WBDCK A/B | 0x11 / 1 |
| 116 | RCK | 0 |
| 117 | ADGAINK extra | absent in117-byte format |

`artifacts/rfk-stage30-late/rx-0.bin` is152 bytes:24-byte BE USB RX descriptor,8-byte C2H header,3-byte RPT_LOG header and117-byte content. Its C2H payload is128 bytes. The log header is `01 75 00` and its source version2 interpretation is bounded as above. All coefficient bytes2..105 are zero in this capture. MSBK=3 and DRCK=1 are nonzero source-defined timeout fields; bit-level path meaning is not established here. DACK_FAIL=0 cannot cancel those flags. No RF quality or calibration success is inferred from the terminal OK.

## Stage31 repeated DACK

`artifacts/rfk-stage31/rx-1.bin` has the same152-byte framing,117-byte report and versions13/2. It has70 nonzero coefficient bytes, DRCK timeout0, and remaining MSBK timeout3. Other timeout fields and DACK_FAIL are0. This is an observed difference after the caller's power/isolation changes; this offline decoder does not establish causality. The repeated report still has failure flags. The source inspected does not prove whether these MSBK bits are sticky from a prior run or newly raised, so the codec neither clears nor discounts them. A fresh controlled initialization and independent source/hardware evidence would be needed to resolve that question.

## API behavior and verification

`decode_rfk_log` verifies outer C2H length and chunk bounds, retains RUN_LOG/RPT_LOG content, ignores valid foreign C2H classes, and rejects unknown types, empty or truncated chunks. `decode_dack_log_reports` selects only class8/function2 RPT_LOG chunks. `decode_dack_report_v2` exposes the scalar fields plus raw content and an optional118th byte. `has_failure_flags()` returns true if **any** of the seven timeout fields or DACK_FAIL is nonzero. `flags_clear()` means only those fields are zero; it never represents terminal completion, table correctness, RF quality, or complete radio readiness.

`tests/rfk_log_test.cpp` uses both physical captures, checks stage30 and stage31 timeout flags, and exercises every individual timeout/fail field, supported117/118-byte layouts, unknown versions, foreign C2H, malformed/truncated chunks and multiple chunks. `c++ -std=c++17 -Wall -Wextra -Werror -Iinclude tests/rfk_log_test.cpp -o /tmp/archer-rfk-log-test` and the binary pass. No hardware operations were performed by this work.
