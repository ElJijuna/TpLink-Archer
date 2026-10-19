# RTL8922AU transport initialization: stage 7

## Physical observation

`make mailbox-universal` builds Intel/ARM. Run
`build/archer-mailbox-universal --inspect-transport` to query PHY_CAP and read
15 transport/MAC registers inside the verified WLAN IO window. No DMA or queue
register is written. The window and mailbox writes have the same effects as
stage 5. ARM execution succeeded; Intel execution remains untested.

The register trace was decoded by `tools/transport_state.py` into
`transport-state-observation.json`. Trace/source hashes are retained in
`transport-state-evidence.json`. The access register returned to 0x001004fd;
mailbox counters advanced 0x22 to 0x33.

Observed HCI_FUNC_EN=7: TX/RX DMA and TRX buffer enable bits are already set.
HAXI_INIT_CFG1=0x00080436 has USB mode code 4 and TX/RX DMA bits set.
RXAGG_0=0 and CMAC_FUNC_EN=0: RX aggregation and CMAC enable bits are clear.
WDE packet buffer configuration is zero; PLE configuration is 0x0b701a01,
whose free-page field is 2928. These are observations, not proof that all
queue allocations or firmware consumers are ready. DMAC clock value
0xffffffff is retained without assuming every clock is operational.

## Source-backed implications

In the retained Linux usb.c, `rtw89_usb_ops_mac_pre_init` returns immediately
for RTL8922A. It does NOT run the older AX USB reset sequence. Post-init skips
older endpoint/packet-size programming and calls RX aggregation v3, whose
source values are 0x80ff2014 at 0x6000 and 0x1f at 0x6004. These values have
not been applied by this project.

In mac_be.c, `rtw89_mac_dmac_func_pre_en_be` selects USB DMA mode, enables HAXI
DMA and clears channel stop bits. `rtw89_mac_hci_func_en_be` enables HCI DMA.
Their mode/enable fields already match this physical snapshot; that alone
cannot establish completion of those routines.

`trx_init_be` calls `dmac_init_be`, then `cmac_init_be`. DMAC initialization
starts with DLE allocation, preload and HCI flow control, then scheduler,
MPDU and security setup. Enabling a single DMA bit cannot replace this chain.
The lack of spontaneous bulk RX remains inconclusive because no bulk H2C
request has yet been sent and queue initialization is incomplete/unverified.

## Next implementation boundary

Map RTL8922A USB2 DLE quotas and H2C/firmware command queue requirements from
the retained source, including firmware-download versus normal modes. Build a
validated initialization plan before writing queue configuration. A CMAC
bring-up additionally needs board/efuse, RF and calibration work; it is not a
prerequisite we can assume for every firmware diagnostic command.

The Linux snapshots are hash-bound, but were fetched from master rather than
a commit-pinned source tree. Pin the source revision before distributing an
adapted implementation. The existing Hopper license expiry still prevents
new native REA decompilation; no new binary analysis is claimed here.
