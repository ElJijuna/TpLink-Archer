CXX := clang++
CXXFLAGS := -std=c++17 -Wall -Wextra -Werror -Iinclude
.PHONY: test probe universal receive-universal decode decode-universal mailbox-universal efuse-universal identity-universal bb-probe-universal mailbox-state-universal mode-switch-universal scsi-eject-universal optical-eject-universal
build:
	mkdir -p build
build/usb-request-test: tests/usb_request_test.cpp include/archer/usb_request.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/usb-descriptors-test: tests/usb_descriptors_test.cpp include/archer/usb_descriptors.hpp tests/fixtures/37ad-0104-usb2-config.bin tests/fixtures/0bda-1a2b-usb2-config.bin | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/firmware-channel-test: tests/firmware_channel_test.cpp include/archer/firmware_channel.hpp include/archer/command_tracker.hpp $(wildcard tests/fixtures/log-disable*.bin) | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/register-mailbox-test: tests/register_mailbox_test.cpp include/archer/register_mailbox.hpp tests/fixtures/phy-cap-part0.bin | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/sequence-ledger-test: tests/sequence_ledger_test.cpp include/archer/sequence_ledger.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/efuse-reader-test: tests/efuse_reader_test.cpp include/archer/efuse_reader.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
test: build/channel-init-test build/tssi-protocol-test build/mac-reset-test build/bb-postinit-test build/radio-init-test build/dle-init-test build/rfk-protocol-test build/bb-table-window-test build/rf-power-test build/firmware-boot-test build/firmware-state-test build/hwsi-window-test build/installer-media-test build/storage-eject-test build/bb-window-test build/rf-transport-test build/rf-access-test build/xtal-identity-test build/efuse-reader-test build/sequence-ledger-test build/usb-request-test build/usb-descriptors-test build/firmware-channel-test build/register-mailbox-test
	./build/channel-init-test
	./build/rx-descriptor-test
	./build/beacon-test
	./build/rx-scan-window-test
	./build/rx-counters-test
	./build/ple-probe-test
	./build/phy-report-window-test
	./build/physts-window-test
	./build/port-rx-window-test
	./build/interface-role-test
	./build/interface-join-test
	./build/interface-cam-test
	./build/interface-tables-test
	./build/interface-lifecycle-test
	./build/tssi-protocol-test
	./build/channel-transition-test
	./build/dmac-init-test
	./build/hfc-init-test
	./build/rfk-log-test
	./build/channel-bus-test
	./build/cmac-init-test
	./build/channel-power-test
	./build/trx-path-test
	./build/tx-rfk-test
	./build/tx-rfk-physical-test
	./build/mac-reset-test
	./build/bb-postinit-test
	./build/radio-init-test
	./build/dle-init-test
	./build/rfk-protocol-test
	./build/bb-table-window-test
	./build/rf-power-test
	./build/firmware-boot-test
	./build/firmware-state-test
	./build/hwsi-window-test
	./build/installer-media-test
	./build/storage-eject-test
	./build/bb-window-test
	./build/rf-transport-test
	./build/rf-access-test
	./build/xtal-identity-test
	./build/efuse-reader-test
	./build/sequence-ledger-test
	./build/usb-request-test
	./build/usb-descriptors-test
	./build/firmware-channel-test
	./build/register-mailbox-test
	python3 -m unittest discover -s tests -p 'test_*.py'
build/archer-probe: src/macos_probe.cpp include/archer/usb_descriptors.hpp include/archer/usb_request.hpp | build
	$(CXX) $(CXXFLAGS) $< -framework IOKit -framework CoreFoundation -o $@
probe: build/archer-probe
	./build/archer-probe
universal: | build
	$(CXX) $(CXXFLAGS) -arch arm64 -arch x86_64 src/macos_probe.cpp -framework IOKit -framework CoreFoundation -o build/archer-probe-universal

receive-universal: | build
	$(CXX) $(CXXFLAGS) -arch arm64 -arch x86_64 src/macos_receive.cpp -framework IOKit -framework CoreFoundation -o build/archer-receive-universal

build/archer-decode: src/channel_decode.cpp include/archer/firmware_channel.hpp include/archer/rx_descriptor.hpp include/archer/beacon.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
decode: build/archer-decode

decode-universal: | build
	$(CXX) $(CXXFLAGS) -arch arm64 -arch x86_64 src/channel_decode.cpp -o build/archer-decode-universal

mailbox-universal: | build
	$(CXX) $(CXXFLAGS) -arch arm64 -arch x86_64 src/macos_mailbox.cpp -framework IOKit -framework CoreFoundation -o build/archer-mailbox-universal

efuse-universal: | build
	$(CXX) $(CXXFLAGS) -arch arm64 -arch x86_64 src/macos_efuse.cpp -framework IOKit -framework CoreFoundation -o build/archer-efuse-universal

identity-universal: | build
	$(CXX) $(CXXFLAGS) -arch arm64 -arch x86_64 src/macos_identity.cpp -framework IOKit -framework CoreFoundation -o build/archer-identity-universal
build/xtal-identity-test: tests/xtal_identity_test.cpp include/archer/xtal_identity.hpp tests/fixtures/xtal-identity.bin | build
	$(CXX) $(CXXFLAGS) $< -o $@

build/rf-access-test: tests/rf_access_test.cpp include/archer/rf_access.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@

build/rf-transport-test: tests/rf_transport_test.cpp include/archer/rf_transport.hpp include/archer/rf_access.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@

bb-probe-universal: | build
	python3 tools/boot_bundle.py
	$(CXX) $(CXXFLAGS) -Iartifacts -arch arm64 -arch x86_64 src/macos_bb_probe.cpp -framework IOKit -framework CoreFoundation -o build/archer-bb-probe-universal
build/bb-window-test: tests/bb_window_test.cpp include/archer/bb_window.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@

mailbox-state-universal: | build
	$(CXX) $(CXXFLAGS) -arch arm64 -arch x86_64 src/macos_mailbox_state.cpp -framework IOKit -framework CoreFoundation -o build/archer-mailbox-state-universal

mode-switch-universal: | build
	$(CXX) $(CXXFLAGS) -arch arm64 -arch x86_64 src/macos_mode_switch.cpp -framework IOKit -framework CoreFoundation -o build/archer-mode-switch-universal
build/storage-eject-test: tests/storage_eject_test.cpp include/archer/storage_eject.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@

scsi-eject-universal: | build
	$(CXX) $(CXXFLAGS) -arch arm64 -arch x86_64 src/macos_scsi_eject.cpp -framework IOKit -framework CoreFoundation -o build/archer-scsi-eject-universal

optical-eject-universal: | build
	$(CXX) $(CXXFLAGS) -arch arm64 -arch x86_64 src/macos_optical_eject.cpp -framework IOKit -framework CoreFoundation -o build/archer-optical-eject-universal
build/installer-media-test: tests/installer_media_test.cpp include/archer/installer_media.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@

build/hwsi-window-test: tests/hwsi_window_test.cpp include/archer/hwsi_window.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@

.PHONY: firmware-state-universal
firmware-state-universal: | build
	$(CXX) $(CXXFLAGS) -arch arm64 -arch x86_64 src/macos_firmware_state.cpp -framework IOKit -framework CoreFoundation -o build/archer-firmware-state-universal
build/firmware-state-test: tests/firmware_state_test.cpp include/archer/firmware_state.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@

.PHONY: firmware-boot-universal
firmware-boot-universal: | build
	python3 tools/boot_bundle.py
	$(CXX) $(CXXFLAGS) -Iartifacts -arch arm64 -arch x86_64 src/macos_firmware_boot.cpp -framework IOKit -framework CoreFoundation -o build/archer-firmware-boot-universal
build/firmware-boot-test: tests/firmware_boot_test.cpp include/archer/firmware_boot.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@

build/rf-power-test: tests/rf_power_test.cpp include/archer/rf_power.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@

build/bb-table-window-test: tests/bb_table_window_test.cpp include/archer/bb_table_window.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@

build/radio-init-test: tests/radio_init_test.cpp include/archer/radio_init.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/dle-init-test: tests/dle_init_test.cpp include/archer/dle_init.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/rfk-protocol-test: tests/rfk_protocol_test.cpp include/archer/rfk_protocol.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@

build/bb-postinit-test: tests/bb_postinit_test.cpp include/archer/bb_postinit.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
.PHONY: radio-init-universal
radio-init-universal: | build
	python3 tools/boot_bundle.py
	python3 tools/radio_bundle.py
	$(CXX) $(CXXFLAGS) -Iartifacts -arch arm64 -arch x86_64 src/macos_radio_init.cpp -framework IOKit -framework CoreFoundation -o build/archer-radio-init-universal

build/mac-reset-test: tests/mac_reset_test.cpp include/archer/mac_reset.hpp include/archer/dle_init.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@

build/channel-init-test: tests/channel_init_test.cpp include/archer/channel_init.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/tssi-protocol-test: tests/tssi_protocol_test.cpp include/archer/tssi_protocol.hpp include/archer/rfk_protocol.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
.PHONY: rfk-universal radio-verify-universal
rfk-universal: | build
	$(CXX) $(CXXFLAGS) -arch arm64 -arch x86_64 src/macos_rfk.cpp -framework IOKit -framework CoreFoundation -o build/archer-rfk-universal
radio-verify-universal: | build
	$(CXX) $(CXXFLAGS) -arch arm64 -arch x86_64 src/macos_radio_verify.cpp -framework IOKit -framework CoreFoundation -o build/archer-radio-verify-universal

test: build/channel-transition-test build/dmac-init-test build/hfc-init-test build/rfk-log-test
build/channel-transition-test: tests/channel_transition_test.cpp include/archer/channel_transition.hpp include/archer/channel_init.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/dmac-init-test: tests/dmac_init_test.cpp include/archer/dmac_init.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/hfc-init-test: tests/hfc_init_test.cpp include/archer/hfc_init.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/rfk-log-test: tests/rfk_log_test.cpp include/archer/rfk_log.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@

test: build/channel-bus-test build/cmac-init-test build/channel-power-test build/trx-path-test build/tx-rfk-test
build/channel-bus-test: tests/channel_bus_test.cpp include/archer/channel_bus.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/cmac-init-test: tests/cmac_init_test.cpp include/archer/cmac_init.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/channel-power-test: tests/channel_power_test.cpp include/archer/channel_power.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/trx-path-test: tests/trx_path_test.cpp include/archer/trx_path.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/tx-rfk-test: tests/tx_rfk_test.cpp include/archer/tx_rfk.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
test: build/tx-rfk-physical-test
build/tx-rfk-physical-test: tests/tx_rfk_physical_test.cpp include/archer/tx_rfk.hpp $(wildcard tests/fixtures/*-stage42-usb.bin) | build
	$(CXX) $(CXXFLAGS) $< -o $@
test: build/rx-descriptor-test
test: build/beacon-test
test: build/rx-scan-window-test
test: build/rx-counters-test
test: build/ple-probe-test
test: build/phy-report-window-test
test: build/physts-window-test
test: build/port-rx-window-test
test: build/interface-role-test
test: build/interface-join-test
test: build/interface-cam-test
test: build/interface-tables-test
test: build/interface-lifecycle-test
build/interface-lifecycle-test: tests/interface_lifecycle_test.cpp include/archer/interface_lifecycle.hpp include/archer/interface_cam.hpp include/archer/interface_tables.hpp include/archer/interface_role.hpp include/archer/command_tracker.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/interface-tables-test: tests/interface_tables_test.cpp include/archer/interface_tables.hpp include/archer/firmware_channel.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/interface-cam-test: tests/interface_cam_test.cpp include/archer/interface_cam.hpp include/archer/interface_role.hpp include/archer/firmware_channel.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/interface-join-test: tests/interface_join_test.cpp include/archer/interface_role.hpp include/archer/command_tracker.hpp include/archer/firmware_channel.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/interface-role-test: tests/interface_role_test.cpp include/archer/interface_role.hpp include/archer/firmware_channel.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/port-rx-window-test: tests/port_rx_window_test.cpp include/archer/port_rx_window.hpp include/archer/channel_bus.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/physts-window-test: tests/physts_window_test.cpp include/archer/physts_window.hpp include/archer/channel_bus.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/phy-report-window-test: tests/phy_report_window_test.cpp include/archer/phy_report_window.hpp include/archer/channel_bus.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/ple-probe-test: tests/ple_probe_test.cpp include/archer/ple_probe.hpp include/archer/channel_bus.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/rx-counters-test: tests/rx_counters_test.cpp include/archer/rx_counters.hpp include/archer/channel_bus.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/rx-scan-window-test: tests/rx_scan_window_test.cpp include/archer/rx_scan_window.hpp include/archer/channel_bus.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/beacon-test: tests/beacon_test.cpp include/archer/beacon.hpp include/archer/rx_descriptor.hpp include/archer/firmware_channel.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/rx-descriptor-test: tests/rx_descriptor_test.cpp include/archer/rx_descriptor.hpp include/archer/firmware_channel.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
.PHONY: cmac-init-universal tx-calibrate-universal
.PHONY: passive-rx-universal
passive-rx-universal: | build
	$(CXX) $(CXXFLAGS) -arch arm64 -arch x86_64 src/macos_passive_rx.cpp -framework IOKit -framework CoreFoundation -o build/archer-passive-rx-universal
cmac-init-universal: | build
	$(CXX) $(CXXFLAGS) -arch arm64 -arch x86_64 src/macos_cmac_init.cpp -framework IOKit -framework CoreFoundation -o build/archer-cmac-init-universal
tx-calibrate-universal: | build
	python3 tools/channel_bundle.py
	python3 tools/tx_calibration_bundle.py
	$(CXX) $(CXXFLAGS) -Iartifacts -arch arm64 -arch x86_64 src/macos_tx_calibrate.cpp -framework IOKit -framework CoreFoundation -o build/archer-tx-calibrate-universal
