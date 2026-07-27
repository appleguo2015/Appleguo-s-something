CXX := c++
TARGET := out/virtual_singer
SOURCE := src/main.cpp
ASSET_DIR := out/assets

CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -Wno-missing-field-initializers $(shell pkg-config --cflags raylib)
LDLIBS := $(shell pkg-config --libs raylib)

.PHONY: all run assets clean

all: $(TARGET) assets

$(TARGET): $(SOURCE) | out
	$(CXX) $(CXXFLAGS) $(SOURCE) -o $@ $(LDLIBS)

out:
	mkdir -p out

assets: | out
	mkdir -p $(ASSET_DIR)
	cp -R assets/. $(ASSET_DIR)/

run: all
	cd out && ./virtual_singer

clean:
	rm -rf out
