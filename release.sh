#!/bin/bash

# vmanager Release Script
# Usage: ./release.sh <version>

set -e

VERSION=${1:-"4.0.0"}
RELEASE_DIR="release-v${VERSION}"

echo "Creating release v${VERSION}..."

# Clean and build
cd v4
make clean
make

# Create release directory
cd ..
mkdir -p "${RELEASE_DIR}"

# Copy binary
cp v4/vmanager "${RELEASE_DIR}/vmanager"
chmod +x "${RELEASE_DIR}/vmanager"

# Copy documentation
cp README.md "${RELEASE_DIR}/"
cp LICENSE "${RELEASE_DIR}/"
cp CHANGELOG.md "${RELEASE_DIR}/"
cp API_PERMISSIONS.md "${RELEASE_DIR}/"

# Create installation script
cat > "${RELEASE_DIR}/install.sh" << 'INSTALL'
#!/bin/bash
set -e

echo "Installing vmanager..."

# Check dependencies
if ! command -v curl &> /dev/null; then
    echo "Error: curl is required"
    exit 1
fi

# Install binary
sudo cp vmanager /usr/local/bin/
sudo chmod +x /usr/local/bin/vmanager

echo "vmanager installed successfully!"
echo "Run 'vmanager --help' to get started"
INSTALL

chmod +x "${RELEASE_DIR}/install.sh"

# Generate checksums
cd "${RELEASE_DIR}"
sha256sum vmanager > SHA256SUMS
cd ..

# Create tarball
tar czf "vmanager-v${VERSION}.tar.gz" "${RELEASE_DIR}"

echo "Release created: vmanager-v${VERSION}.tar.gz"
echo "Contents:"
ls -lh "${RELEASE_DIR}"
echo ""
echo "Checksum:"
cat "${RELEASE_DIR}/SHA256SUMS"
