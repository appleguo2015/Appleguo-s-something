CXX ?= c++
PKG_CONFIG ?= pkg-config
WINDRES ?= windres

ifeq ($(OS),Windows_NT)
EXE_SUFFIX := .exe
WINDOWS_ICON := scripts/windows/appleguo.ico
WINDOWS_RESOURCE := out/appleguo.res
BANANA_WINDOWS_ICON := scripts/windows/banana.ico
BANANA_WINDOWS_RESOURCE := out/banana.res
else
EXE_SUFFIX :=
endif

TARGET := out/virtual_singer$(EXE_SUFFIX)
SOURCE := src/main.cpp
BANANA_TARGET := out/banana_dance$(EXE_SUFFIX)
BANANA_SOURCE := src/banana_main.cpp
ASSET_DIR := out/assets
DIST_DIR := dist
SITE_DIST_DIR := $(DIST_DIR)/site

CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -Wno-missing-field-initializers $(shell $(PKG_CONFIG) --cflags raylib)
ifeq ($(STATIC),1)
LDLIBS := $(shell $(PKG_CONFIG) --static --libs raylib)
else
LDLIBS := $(shell $(PKG_CONFIG) --libs raylib)
endif

.PHONY: all run assets app package banana banana-run banana-app banana-package site clean

all: $(TARGET) assets

$(TARGET): $(SOURCE) $(WINDOWS_RESOURCE) | out
	$(CXX) $(CXXFLAGS) $(SOURCE) $(WINDOWS_RESOURCE) -o $@ $(LDLIBS)

$(BANANA_TARGET): $(BANANA_SOURCE) $(BANANA_WINDOWS_RESOURCE) | out
	$(CXX) $(CXXFLAGS) $(BANANA_SOURCE) $(BANANA_WINDOWS_RESOURCE) -o $@ $(LDLIBS)

ifeq ($(OS),Windows_NT)
$(WINDOWS_RESOURCE): scripts/windows/app.rc $(WINDOWS_ICON) | out
	$(WINDRES) $< -O coff -o $@

$(BANANA_WINDOWS_RESOURCE): scripts/windows/banana.rc $(BANANA_WINDOWS_ICON) | out
	$(WINDRES) $< -O coff -o $@
endif

out:
	mkdir -p out

ifeq ($(OS),Windows_NT)
assets: | out
	rm -rf $(ASSET_DIR)
	cp -R assets $(ASSET_DIR)
else
assets: | out
	mkdir -p $(ASSET_DIR)
	rsync -a --delete assets/ $(ASSET_DIR)/
endif

run: all
	cd out && ./virtual_singer

banana: $(BANANA_TARGET) assets

banana-run: banana
	cd out && ./banana_dance

banana-app: banana
	./scripts/package-banana-macos.sh

banana-package: banana-app

app: all
	./scripts/package-macos.sh

package: app

site:
	mkdir -p $(SITE_DIST_DIR)/assets/images
	cp index.html $(SITE_DIST_DIR)/index.html
	cp 404.html $(SITE_DIST_DIR)/404.html
	rsync -a --delete assets/images/ $(SITE_DIST_DIR)/assets/images/

clean:
	rm -rf out $(DIST_DIR)
