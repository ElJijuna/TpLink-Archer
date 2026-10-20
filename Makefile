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
test: build/firmware-state-test build/hwsi-window-test build/installer-media-test build/storage-eject-test build/bb-window-test build/rf-transport-test build/rf-access-test build/xtal-identity-test build/efuse-reader-test build/sequence-ledger-test build/usb-request-test build/usb-descriptors-test build/firmware-channel-test build/register-mailbox-test
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

build/archer-decode: src/channel_decode.cpp include/archer/firmware_channel.hpp | build
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
	$(CXX) $(CXXFLAGS) -arch arm64 -arch x86_64 src/macos_bb_probe.cpp -framework IOKit -framework CoreFoundation -o build/archer-bb-probe-universal
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
