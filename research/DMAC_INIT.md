# RTL8922A USB2 SCC DMAC remainder

`include/archer/dmac_init.hpp` implements the STA scheduler, MPDU processor,
security, TX packet controller and MLO portions of pinned
`artifacts/rtw89-mac_be.c:973-1129`. It is called **after** DLE, USB-skipped
preload and completed HFC setup, matching dmac_init_be:1132. It performs no
firmware stop/reset, CMAC initialization, association or network transmission.
The pure plan and RegisterBus coordinator are separate APIs.

## Metadata and prerequisites

The caller must establish exclusive initialization ownership, USB2 SCC DLE
readiness, completed USB2 SCC HFC configuration and supply
`DmacReservedMpduInfo{2059,256,1,18}`. No default metadata is inferred from
live packet-buffer values. These fixed values are sourced from:

- rtw8922a.c:200 USB2 SCC selects ple_size6_v1 and ple_rsvd_qt2, no dle_input.
- mac.c:1770/1890 PLE linked2059,page256,reserved MPDU table1page.
- mac.c:1934 get_dle_rsvd_qt_cfg(MPDU_INFO) sets pktid=ple_free_pg;
  get_dle_mem_cfg:1908 establishes that from linked page count.
- reg.h:6120 fallback MPDU_INFO_B1_OFST=18; a non-null dle_input would instead
  require ceil(mpdu_info_tbl_b0/8), which this fixed profile rejects.

TXPKTCTL_MPDUINFO_CFG0x9f10 becomes masked0x880b0012 under0x8fff003f;
unrelated register bits are preserved. The supplied metadata must agree with
the actual DLE configuration established by the initialization coordinator.
Before any writes the executor checks chip ID/CV1, MAC+DMAC enable and both
DLE ready flags. It does not prove exclusive ownership or completed HFC from
those flags; those remain lifecycle requirements.

## Ordered operations and polling

STA scheduler sets byte0xa310 bit0, polls bit31 at1us intervals up to2000us,
sets warm init bit29 and clears band triggers bits28/9. Then MPDU appendFCS,
AMSDU cut0x210e05f0, header shortcut bits2:0, RX header conversion8, dispatch
lowbyte0x55; security bits0xf3f and appendICV/MIC; reserved MPDU metadata; MLO
reinit bit23 pulse and bit31 poll at1us up to1000us; finally MLO HW link change
and MACID acquisition check enable. Register constants are in reg.h:5465,
5974-6076,6116,6208,6243,6462; timeout2000 is mac.h:599.

The source header shortcut code clears bit4 in a temporary value but passes
that value to write32_set, which ORs with a fresh register read. Its effective
operation preserves bit4 and sets bits2:0. This port matches that effective
behavior rather than silently changing it.

Configuration writes are read back. Warm-init and reinit command assertions
are not treated as persistent latch requirements; completion is established
by the source's bounded readiness polls. Polling performs at most2000/1000
caller-supplied >=1us delays; USB transfer latency adds wall time. Missing
required inputs reject before I/O. A runtime failure stops subsequent stages;
there is no unsafe partial rollback. Keep producers paused and reset through
the owning lifecycle. In contrast to Linux mlo_init_be, which still enables
HW link change after timeout before returning the error, this executor stops
at that timeout and leaves later enabling writes unperformed.

## HFC is not a USB no-op

`rtw89-mac.c:1194` has no USB early return. It resets parameters, checks DMAC,
disables HFC, configures per-channel/public/mixed parameters, enables HFC/H2C,
delays10us and reads channel/public counters. USB2 SCC selects actual tables
at rtw8922a.c:110: chcfg_ch7, pubcfg_p7, hfc_prec_cfg_c5, STF mode. chcfg_ch7
contains min54,max222 for ACH0/2/4/6 and B0/B1MGQ; public group0/max492.
This file deliberately requires that work to have already completed; it does
not mistake missing HFC implementation for a no-op. USB **preload** is skipped
because is_qta_poh(mac.c:2370) accepts PCIe only.

## Validation

```
c++ -std=c++17 -Wall -Wextra -Werror -Iinclude tests/dmac_init_test.cpp -o /tmp/archer-dmac-init-test
/tmp/archer-dmac-init-test
```

Passing simulated tests cover source ordering, exact constants, byte access,
register-bit preservation, delayed readiness, both bounded timeouts, missing/
wrong MPDU metadata, missing HFC prerequisite, transfer/latch failures and
inaccessible prerequisite reads. No hardware was accessed. Passing tests and
DMAC readiness do not establish full MAC, channel or RFK completion.

## Stage32c missing source power-on prerequisite

Trace `artifacts/radio-init-stage32c.jsonl:125-126` writes0x210e05f0 to0x9c94
and reads0x200e05f0. Missing bit24 is **MPDU_CUT_CTRL_EN**;
CA_CHK_ADDRCAM_EN is bit29 and did latch. Pinned source has no CV1/CAV branch
around this write and does not read-verify it. This trace alone does not prove
why MPDU cut control did not latch.

Source `artifacts/rtw8922a.c:619-622` power-on tail explicitly enables shared
CMAC functions before later initialization. In contrast,
cmac_share_func_en_be(mac_be.c:799) immediately returns for RTL8922A, relying
on the earlier chip-specific power-on path. Root's reduced enable_cmac0 helper
previously omitted that shared function register.

Added `enable_8922a_shared_cmac(bus, exclusive_owner)` to port the missing
source step: R_BE_CMAC_SHARE_FUNC_EN0xe000 |=0x40000007, comprising SHAREbit30,
RESPBA bit2, ADDRSRCH bit1 and BTCOEX bit0 (reg.h:6452-6460). It checks chipID,
CV1 and MAC/DMAC enables, preserves unrelated bits and verifies these enable
bits. DMAC remainder now requires those bits before any writes. Invoke this
step inside the source power-on/sys-init lifecycle, and enable CMAC0 power/
clocks/functions before DMAC remainder, matching sys_init before trx_init.

No new blanket latch relaxation was made. The causal relationship between
shared address-search enable and MPDU cut control remains unproven; the next
controlled attempt with source prerequisites established can test it. If
bit24 remains absent, report that evidence separately from bit29 and consider
the source's lack of latch enforcement explicitly rather than assuming an
address-CAM field definition error. Tests cover shared enables, preservation,
missing ownership, ignored enable writes and missing shared prerequisite with
zero DMAC writes.

## Physical follow-up stage32d/e

With shared CMAC and CMAC0 enables verified, bit24 still did not latch:
0x9c94 request0x210e05f0/read0x200e05f0. This disproves treating shared
enable as a sufficient fix for that readback. Source writes the value without
requiring equality. The executor now exempts only that observed bit from its
additional equality check, retains the source request, and returns the complete
effective value. All other fields and bounded scheduler/MLO polls remain
checked. It does not claim MPDU cut support. Stage32e completed the remainder;
stage34 then produced clean detailed DACK/RXDCK reports. The cause or network
impact of the cut-control difference remains unknown.
