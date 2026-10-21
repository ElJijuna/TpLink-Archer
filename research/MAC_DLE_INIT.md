# RTL8922A USB2 SCC DLE primitive

`include/archer/dle_init.hpp` implements only the DLE portion of a future cold
MAC initialization coordinator. It is deliberately disconnected from the live
firmware CLI. Stage29 firmware must fail its prerequisite check before writes.
No device was accessed for implementation or testing.

## Source evidence

Pinned local Linux snapshots (source evidence, not observed device execution):

- `artifacts/rtw8922a.c:200`: USB2 SCC selects wde_size7_v1,
  ple_size6_v1, wde_qt7_v1, ple_qt12_v1/13_v1; USB3 uses other tables.
- `artifacts/rtw89-mac.c:1739,1770,1806,1837,1841,1894`: WDE
  64*(508+4)=32768 bytes; PLE 256*(2059+109)=555008 bytes;
  reserved tail 2048 bytes; total 589824. WDE quota2 is zeroed by BE code;
  the four-field source WDE tuple maps to HIF/WCPU/pkt_in/cpu_io.
- `artifacts/rtw89-mac.c:2274`: check DMAC, validate configuration size,
  disable DLE functions, enable clocks, program layout, program quotas,
  enable functions, poll WDE, then poll PLE.
- `artifacts/rtw89-mac_be.c:216-371`: RTL8922A clock gating, field
  encodings, WDE quotas0-4, PLE quotas0-12, readiness poll 1us/2000us.
- `artifacts/rtw89-reg.h:4893,4918,5483,5579,5604,5700`: BE registers
  and bit masks. Readiness deliberately uses AX names at 0x8d00/0x9100
  as the BE Linux function does.

## Safety and limitations

The Linux DLE function is called inside a coordinated MAC initialization
lifecycle. It does **not** establish firmware/queue quiescence. The primitive
therefore requires explicit externally established exclusive reset ownership,
all firmware CPUs stopped, all HCI DMA/queue producers stopped, and USB2 SCC
selection. These facts cannot be reconstructed from packet buffer values.
They are an API contract, not a proven shutdown implementation. It also rejects
nonzero firmware download state, non-CV1 hardware, enabled DLE engines, and
missing MAC/DMAC enable bits. No current live caller can establish its contract.

A caller must supply a delay of at least one microsecond. Polling performs
at most 2000 delays per engine; USB I/O time adds latency. RegisterBus pause
has no timing guarantee and is not used. Configuration writes are read back;
unrelated packet-buffer and enable/clock bits are preserved. On any execution
failure it attempts and verifies DLE disable. Unlike Linux's readiness timeout
return, this leaves engines disabled; a cleanup failure reports reset required.
It does not restore an earlier partition after partial initialization.

Success establishes only DLE readiness flags. Full MAC, USB HFC, descriptor
rings, H2C transport, firmware restart, radio tables, RF calibration and traffic
readiness remain outside this primitive. The layout is sourced from USB2 Linux
SCC tables; it has not been validated against this adapter on macOS.

## Offline validation

```
c++ -std=c++17 -Wall -Wextra -Werror -Iinclude tests/dle_init_test.cpp -o /tmp/archer-dle-init-test
/tmp/archer-dle-init-test
```

Simulated bus covers ordering, preserved bits, quota/layout write sequence,
WDE-before-PLE polling, delayed readiness, both timeouts, rejected live firmware,
missing lifecycle evidence, inaccessible reads, ignored writes, transfer failure
and cleanup failure. This verifies software behavior, not hardware readiness.

## Follow-up: full MAC order and quiesce boundary

`rtw89-mac.c:4359` calls partial_init (firmware boot), chip_enable_bb_rf,
`sys_init_be`, `trx_init_be`, feature init, USB mac_post_init, early H2Cs,
offload config. `rtw89-core.c:6658` then resets BB/RF, initializes BB tables,
BB postinit and RF tables, coex/DM/PHY reports, starts HCI and finally RFK late.
This is the source-supported order; no smaller sufficient RFK MAC subset has
been established.

`sys_init_be` (mac_be.c:912) runs DMAC enable, shared CMAC enable, CMAC0 power,
CMAC0 function. The first two return immediately for RTL8922A (lines775,799),
so they assume earlier chip power/preinit did that work. CMAC0 power sets
AFE_CTRL1(0x24) bits28:24, clears FEN_RST_ENABLE(0x84) isolation bit24, sets
CMAC0_FEN bit16. CMAC enable sets CK_EN(0x10004) mask0x400001ff then
CMAC_FUNC_EN(0x10000) mask0xf00001ff (mac_be.c:814-905; reg.h:3960,4137,6574).
Power/function flags in Linux prevent duplicate transitions; register-only
reconstruction needs equivalent lifecycle tracking.

`dmac_init_be` (1132) runs DLE, preload (USB skip), HFC, STA scheduler, MPDU
processor, security engine, TX packet controller, MLO. `cmac_init_be` (1756)
runs scheduler, address CAM reset, RX filters, CCA, NAV, spatial reuse, TMAC,
TRX protocol, RMAC, response packet control, CMAC common, PTCL and CMAC DMA.
`trx_init_be` (2302) runs those two, optional DBCC, DMAC/CMAC IMRs, error IMR,
host release-report configuration and RTL8922A RSP_CHK_SIG adjustment. DLE
readiness alone proves none of these configurations.

Relevant shutdown/start constants are evidence of controls, not proof of a
complete drain sequence:

- `mac_be.c:603` disable_cpu: 0x88 WCPU_EN bit1 clear; HOLD_AFTER_RESET bit11
  set; WCPU_EN set. 0x1e0 is overwritten with old RUN_ENV bits31:30 only.
  RTL8922A then **sets** 0x888 DCPU_PLATFORM_EN bit0; clears UDM0/HALT_C2H/UDM2.
  It does not poll a BBMCU stopped acknowledgment.
- `mac_be.c:369` HCI enable sets 0x7880 bits1:0 (RXDMA/TXDMA).
- `mac_be.c:375` USB pre-enable sets 0xb000 DMA_MODE bits10:8=4, clears
  STOP_AXI_MST bit7, sets RXDMA bit5/TXDMA bit4; clears 0xb010 TX_STOP1
  bits14:0=0x7fff; sets DMAC_TABLE_CTRL address mode. The inverse writes
  are plausible stop requests, but this function is not a shutdown protocol.
- `reg.h:4357` exposes HCI_TXDMA_BUSY at 0x170 bit2. That single bit says
  nothing sufficient about RXDMA, CMAC producers, BBMCU or all DLE queues.
- `usb.c:742-765` start/stop/pause/deinit are no-ops. mac_pre_init at767
  returns immediately for RTL8922A; its RX/TX toggle workaround applies to
  other chips. mac_post_init at851 jumps directly to RX aggregation for 8922A.

Consequently: save/restore HCI bits is insufficient evidence to set
`firmware_cpus_stopped` and `hci_dma_and_queue_producers_stopped`. A true
reset-owned cold lifecycle or a verified coordinated drain/reset protocol is
still required. Clearing firmware status creates a zero value; it cannot prove
that contract by itself. Restore/re-enable HCI only after valid layout/HFC and
all relevant MAC producers are configured, following a lifecycle with explicit
failure handling; no exact live stop/restart sequence is established here.

RFK completion requires the H2C/C2H path. phy.c:3999 waits for completion,
requires state OK and multiplies USB deadlines by four. phy.c:4061 completes
that wait from a C2H RFK report. rtw8922a.c:2349 late init sends pre-notify,
DACK, RXDCK. Channel RFK at2388 additionally pauses scheduler TX, waits RF mode,
then pre-notify/TXGAPK/IQK/TSSI/DPK/RXDCK and resumes TX. Missing HFC/C2H routing,
RX path, CMAC setup or BB/RF initialization can block completion even when both
DLE ready flags are set. This is a dependency inference, not identification of
a specific observed calibration failure.
