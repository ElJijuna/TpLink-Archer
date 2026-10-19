CXX := clang++
CXXFLAGS := -std=c++17 -Wall -Wextra -Werror -Iinclude
.PHONY: test probe universal receive-universal decode decode-universal mailbox-universal efuse-universal
build:
	mkdir -p build
build/usb-request-test: tests/usb_request_test.cpp include/archer/usb_request.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/usb-descriptors-test: tests/usb_descriptors_test.cpp include/archer/usb_descriptors.hpp tests/fixtures/37ad-0104-usb2-config.bin | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/firmware-channel-test: tests/firmware_channel_test.cpp include/archer/firmware_channel.hpp include/archer/command_tracker.hpp $(wildcard tests/fixtures/log-disable*.bin) | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/register-mailbox-test: tests/register_mailbox_test.cpp include/archer/register_mailbox.hpp tests/fixtures/phy-cap-part0.bin | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/sequence-ledger-test: tests/sequence_ledger_test.cpp include/archer/sequence_ledger.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/efuse-reader-test: tests/efuse_reader_test.cpp include/archer/efuse_reader.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
test: build/efuse-reader-test build/sequence-ledger-test build/usb-request-test build/usb-descriptors-test build/firmware-channel-test build/register-mailbox-test
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
