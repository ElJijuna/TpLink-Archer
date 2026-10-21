# RTL8922A USB2 SCC HFC initialization

Implemented `include/archer/hfc_init.hpp` as a fixed-profile pure plan and a
bounded RegisterBus executor. No hardware accessed. Source is pinned
`artifacts/rtw89-mac.c:895-1049,1194-1245`, `rtw89-mac_be.c:108-214`,
`rtw8922a.c:60-116,254-268,3352`, `rtw89-core.h:5006,5117,5888` and
`rtw89-reg.h:6391-6447`.

## Exact profile

USB2 SCC selects chcfg_ch7, pubcfg_p7, preallocation c5, STF mode1 and
DMA skip mask0. Channels0/2/4/6/8/10 use min54,max222,group0; odd channels
use zero. All12 channels below H2C are configured, including zero entries.
Public group0/max492,group1/WP threshold0. Preallocations are channel18,
H2C32,WP07/WP811148; full conditions channel1,H2C1,WP07zero,WP811one.
The validator rejects any field differing from this source profile before I/O.

Order matches rtw89_mac_hfc_init(reset=true,en=true,h2c_en=true): disable
HFC/H2C, channel configurations, public group and WP threshold, mixed
preallocations/public maximum/full conditions/mode, enable HFC/H2C, delay10us,
then read channel/public/configuration state. Source has no poll in this
function. This implementation uses exactly one caller-supplied >=10us delay,
fixed loops and finite reads/writes. It adds immediate configuration readback;
it does not spin until a failed accounting invariant happens to pass.

Requested configuration fields (CH12 full condition is read back as effective state):

| Register | Expected configuration |
|---|---|
|0xb700 HCI_FC_CTRL|request low12 bits0x51b; verify low10 bits0x11b; retain/report bits11:10|
|0xb704 CH_PAGE_CTRL|0x200012|
|0xb718 +4*channel|even0x00de0036; odd0|
|0xb790 PUB_PAGE_CTRL1|492|
|0xb794 PUB_PAGE_CTRL2|492|
|0xb7a4 WP_PAGE_CTRL1|0x940094|
|0xb7a8 WP_PAGE_CTRL2|0|

Whole-register writes follow source where source writes full words; HCI control
writes preserve unrelated fields. Byte-wide accesses are not used here.

## Readback and failure behavior

HfcReadback contains each channel's available/used pages, public used and
available group counts, total public availability, WP availability, and all
mixed configuration registers. Common hfc_upd_ch_info uses the AX12-bit
available mask even though BE has a13-bit definition; this implementation
matches the common source extraction. The profile is below either field limit.
Group counters use13-bit fields. The source invariant is
`group0_used + group1_used + public_available == configured_public_maximum492`;
nonzero use counts are allowed if accounting balances.

Before writes, require exclusive lifecycle ownership and already initialized
USB2 SCC DLE; verify hardware ID/CV1, MAC/DMAC enable and WDE/PLE ready flags.
No register check proves exclusive ownership. The caller must coordinate
firmware and host producers. This API configures HFC; it does not reset or
repartition DLE and must not be used as an uncoordinated live transition.

Any execution/readback/accounting failure stops and attempts verified disable
of HFC/H2C bits. Failed cleanup reports reset required. It does not restore a
prior incompatible HFC profile over partial initialization. Successful return
supports the HFC-completed prerequisite of the DMAC remainder; it does not
establish CMAC, firmware/channel state or RF calibration success.

## Validation

```
c++ -std=c++17 -Wall -Wextra -Werror -Iinclude tests/hfc_init_test.cpp -o /tmp/archer-hfc-init-test
/tmp/archer-hfc-init-test
```

Tests pass for exact channel and mixed constants, unrelated HCI-bit preservation,
fixed10us delay, decoded counters, valid nonzero public use, accounting failure,
profile/prerequisite rejection, ignored writes, transfer failure and inaccessible
readback. Hardware behavior remains unverified by these simulated tests.

## Stage32 readback correction

`artifacts/radio-init-stage32.jsonl:527-532` records a successful USB write of
0x512 to0xb700 followed by0x112 readback. The difference is exactly bit10,
CH12_FULL_COND. Both BE and AX definitions assign the field bits11:10;
preallocation c5 requests value1. No alternate USB encoding or USB early-return
exists in the pinned source. The trace cannot distinguish a hardware-fixed/
normalized field, firmware update, or another register behavior; no cause is
claimed from the single observation.

The original executor imposed an immediate threshold equality check stronger
than Linux's source contract. Linux writes the request, enables HFC/H2C,
delays10us, then hfc_get_mix_info_be reads the effective CH12 condition back
into prec_cfg. It does not compare that field with the requested value.
The executor now follows that behavior for this field: still request1, exclude
bits11:10 from exact latch/final equality checks, and return requested/effective
conditions plus control_profile_difference. Every other requested field,
HFC/H2C enable, and the public-page accounting invariant remains checked.
Readback0x11b after enable is therefore accepted with effective condition0 and
reported control_profile_difference0x400; this does not assert condition1
latched. Test bus reproduces precisely that observed normalization.
