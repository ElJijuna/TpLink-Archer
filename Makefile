CXX := clang++
CXXFLAGS := -std=c++17 -Wall -Wextra -Werror -Iinclude
.PHONY: test probe universal
build:
	mkdir -p build
build/usb-request-test: tests/usb_request_test.cpp include/archer/usb_request.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/usb-descriptors-test: tests/usb_descriptors_test.cpp include/archer/usb_descriptors.hpp tests/fixtures/37ad-0104-usb2-config.bin | build
	$(CXX) $(CXXFLAGS) $< -o $@
test: build/usb-request-test build/usb-descriptors-test
	./build/usb-request-test
	./build/usb-descriptors-test
	python3 -m unittest discover -s tests -p 'test_*.py'
build/archer-probe: src/macos_probe.cpp include/archer/usb_descriptors.hpp include/archer/usb_request.hpp | build
	$(CXX) $(CXXFLAGS) $< -framework IOKit -framework CoreFoundation -o $@
probe: build/archer-probe
	./build/archer-probe
universal: | build
	$(CXX) $(CXXFLAGS) -arch arm64 -arch x86_64 src/macos_probe.cpp -framework IOKit -framework CoreFoundation -o build/archer-probe-universal
