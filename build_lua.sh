#!/bin/bash
# Build Lua from source for OCCE

LUA_VERSION="5.4.7"
LUA_DIR="lua-${LUA_VERSION}"
LUA_ARCHIVE="${LUA_DIR}.tar.gz"
LUA_URL="http://www.lua.org/ftp/${LUA_ARCHIVE}"

# Download Lua if not already present
if [ ! -f "${LUA_ARCHIVE}" ]; then
    echo "Downloading Lua ${LUA_VERSION}..."
    wget -q "${LUA_URL}" || curl -sL -o "${LUA_ARCHIVE}" "${LUA_URL}"
fi

# Extract if not already extracted
if [ ! -d "${LUA_DIR}" ]; then
    echo "Extracting Lua..."
    tar xzf "${LUA_ARCHIVE}"
fi

# Build Lua
cd "${LUA_DIR}"
echo "Building Lua..."
make clean 2>/dev/null
make linux MYCFLAGS="-Os -fPIC" 2>&1 | grep -v "warning:"

# Install locally
make local 2>&1 | grep -v "warning:"

cd ..
echo "Lua built successfully in ${LUA_DIR}/install"
