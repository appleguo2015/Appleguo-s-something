CXX ?= c++
PKG_CONFIG ?= pkg-config
WINDRES ?= windres

ifeq ($(OS),Windows_NT)
EXE_SUFFIX := .exe
WINDOWS_ICON := scripts/windows/appleguo.ico
WINDOWS_RESOURCE := out/appleguo.res
else
EXE_SUFFIX :=
endif

TARGET := out/virtual_singer$(EXE_SUFFIX)
SOURCE := src/main.cpp
ASSET_DIR := out/assets
DIST_DIR := dist

CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -Wno-missing-field-initializers $(shell $(PKG_CONFIG) --cflags raylib)
ifeq ($(STATIC),1)
LDLIBS := $(shell $(PKG_CONFIG) --static --libs raylib)
else
LDLIBS := $(shell $(PKG_CONFIG) --libs raylib)
endif

.PHONY: all run assets app package clean

all: $(TARGET) assets

$(TARGET): $(SOURCE) $(WINDOWS_RESOURCE) | out
	$(CXX) $(CXXFLAGS) $(SOURCE) $(WINDOWS_RESOURCE) -o $@ $(LDLIBS)

ifeq ($(OS),Windows_NT)
$(WINDOWS_RESOURCE): scripts/windows/app.rc $(WINDOWS_ICON) | out
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

app: all
	./scripts/package-macos.sh

package: app

clean:
	rm -rf out $(DIST_DIR)
