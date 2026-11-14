#!/bin/bash
# Builds the Linux version of PDFDocumentScannerM as an AppImage and puts the release files next to
# it. The build runs in a Podman container with Ubuntu 22.04 (glibc 2.35): a program only runs on
# systems with the same or a newer glibc, so a build on a new system (Arch ...) would not start on
# older ones (Ubuntu LTS, Debian). Qt 6.12 is loaded with aqtinstall (Ubuntu 22.04 has no Qt 6.12),
# the AppImage is made with linuxdeploy and its Qt plugin (Wayland plugins and the platform theme
# of the desktop portal included). Same procedure as in MovieMetaM.
#
# Call from anywhere:  tools/build-linux.sh
# Needs:               podman, python3, network (first run: Ubuntu image, Qt, linuxdeploy; later cached)
# Result in build/:    PDFDocumentScannerM-<version>-linux.AppImage, updates.xml, CHANGELOG.md,
#                      README.md, doc/ (documentation pages from the help, tools/help-to-html.py)
# Cache:               build/linux-cache (Qt and linuxdeploy; delete it to download them again)

set -euo pipefail

APP_NAME="PDFDocumentScannerM"
SOURCE_DIR="$(cd "$(dirname "$0")/.." && pwd)"
QT_VERSION="6.12.0"
IMAGE="ubuntu:22.04"

if [ "${1:-}" != "--inside" ]; then
    # ---- On the host: start this script again inside the container, then the release files
    VERSION="$(sed -n 's/^set(PROJECT_VERSION "\(.*\)")/\1/p' "$SOURCE_DIR/CMakeLists.txt")"
    if [ -z "$VERSION" ]; then
        echo "PROJECT_VERSION not found in CMakeLists.txt" >&2
        exit 1
    fi
    RESULT="$APP_NAME-$VERSION-linux.AppImage"

    mkdir -p "$SOURCE_DIR/build/linux-cache"
    podman run --rm \
        -v "$SOURCE_DIR:/src" \
        -e QT_VERSION="$QT_VERSION" \
        -e RESULT="$RESULT" \
        "$IMAGE" bash /src/tools/build-linux.sh --inside

    echo "== 7. Release files"
    cd "$SOURCE_DIR/build"
    cp ../CHANGELOG.md ../README.md .
    # Read by the update check; the date is the day of the build
    cat > updates.xml <<EOF
<?xml version="1.0" encoding="utf-8"?>
<updates>
    <update>
        <name>$APP_NAME</name>
        <version>$VERSION</version>
        <date>$(date +%F)</date>
        <infourl>https://github.com/SpeckenbachNet/$APP_NAME/releases/tag/v$VERSION</infourl>
        <downloads>
            <downloadurl platform="lnx">https://github.com/SpeckenbachNet/$APP_NAME/releases/download/v$VERSION/$RESULT</downloadurl>
        </downloads>
    </update>
</updates>
EOF
    python3 "$SOURCE_DIR/tools/help-to-html.py" "$SOURCE_DIR/help" "$SOURCE_DIR/build/doc"

    echo
    echo "Done in $SOURCE_DIR/build:"
    ls -l "$RESULT" updates.xml CHANGELOG.md README.md doc
    exit 0
fi

# ---- Inside the container (Ubuntu 22.04, user root, the files belong to the host user afterwards)
CACHE=/src/build/linux-cache
BUILD_DIR=/src/build/linux-release
APPDIR=/src/build/linux-appdir
QT_DIR="$CACHE/Qt/$QT_VERSION/gcc_64"
export DEBIAN_FRONTEND=noninteractive

echo "== 1. Tools and libraries of the system"
apt-get update -qq
# Build tools; the libraries are the ones Qt's GUI and its platform plugins link against
apt-get install -y -qq --no-install-recommends \
    build-essential ninja-build git ca-certificates wget file patchelf python3-pip \
    desktop-file-utils libgl1-mesa-dev libglx-dev libegl1-mesa-dev libxkbcommon-dev \
    libxkbcommon-x11-0 libfontconfig1 libfreetype6 libdbus-1-3 libglib2.0-0 libx11-xcb1 \
    libxcb-cursor0 libxcb-icccm4 libxcb-image0 libxcb-keysyms1 libxcb-randr0 \
    libxcb-render-util0 libxcb-shape0 libxcb-xfixes0 libxcb-xinerama0 libxcb-xkb1 \
    libxcb-sync1 libxcb-shm0 libxcb-render0 libxcb-util1 libwayland-client0 libwayland-cursor0 \
    libwayland-egl1 >/dev/null

# Qt 6.12 wants CMake 3.25 or newer, Ubuntu 22.04 has 3.22: CMake from pip (in /usr/local/bin)
pip3 install --quiet aqtinstall cmake

echo "== 2. Qt $QT_VERSION (cached in build/linux-cache)"
if [ ! -x "$QT_DIR/bin/qmake" ]; then
    aqt install-qt linux desktop "$QT_VERSION" linux_gcc_64 --outputdir "$CACHE/Qt"
fi

echo "== 3. linuxdeploy (cached in build/linux-cache)"
mkdir -p "$CACHE/tools"
fetch() {   # fetch <file name> <url>
    if [ ! -x "$CACHE/tools/$1" ]; then
        wget -q -O "$CACHE/tools/$1" "$2"
        chmod +x "$CACHE/tools/$1"
    fi
}
fetch linuxdeploy-x86_64.AppImage \
    https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
fetch linuxdeploy-plugin-qt-x86_64.AppImage \
    https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
fetch linuxdeploy-plugin-appimage-x86_64.AppImage \
    https://github.com/linuxdeploy/linuxdeploy-plugin-appimage/releases/download/continuous/linuxdeploy-plugin-appimage-x86_64.AppImage

echo "== 4. Release build"
cmake -S /src -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$QT_DIR"
cmake --build "$BUILD_DIR"

echo "== 5. Install into the AppDir"
rm -rf "$APPDIR"
DESTDIR="$APPDIR" cmake --install "$BUILD_DIR" --prefix /usr

echo "== 6. Qt and the AppImage (linuxdeploy)"
cd /src/build
rm -f "$APP_NAME"*.AppImage                # older versions, else the mv below finds two
export APPIMAGE_EXTRACT_AND_RUN=1          # no FUSE in the container
export PATH="$CACHE/tools:$PATH"           # linuxdeploy finds its plugins here
export QMAKE="$QT_DIR/bin/qmake"
export LD_LIBRARY_PATH="$QT_DIR/lib"
export EXTRA_PLATFORM_PLUGINS="libqwayland.so"   # Qt 6.12: one plugin (before: -generic, -egl)
export LINUXDEPLOY_OUTPUT_VERSION=""
DESKTOP="$APPDIR/usr/share/applications/net.speckenbach.$APP_NAME.desktop"
ICON="$APPDIR/usr/share/icons/hicolor/256x256/apps/net.speckenbach.$APP_NAME.png"
# a) Qt and the libraries into the AppDir
"$CACHE/tools/linuxdeploy-x86_64.AppImage" --appdir "$APPDIR" --desktop-file "$DESKTOP" \
    --icon-file "$ICON" --plugin qt
# Qt 6.12 links libOpenGL.so.0 (GLVND, the vendor neutral dispatch library). linuxdeploy leaves it
# out (exclude list), but a system without the package (libopengl0) would not start the app (seen
# with MovieMetaM on a minimal Debian 12): so it goes along. The drivers still come from the system.
cp -L /usr/lib/x86_64-linux-gnu/libOpenGL.so.0 "$APPDIR/usr/lib/"
# b) The platform theme of the desktop portal (the file dialog of the desktop, "Save PDF"). Only
#    this one: DEPLOY_PLATFORM_THEMES would also take the GTK theme, which needs the whole of GTK.
THEMES="$APPDIR/usr/plugins/platformthemes"
mkdir -p "$THEMES"
cp "$QT_DIR/plugins/platformthemes/libqxdgdesktopportal.so" "$THEMES/"
patchelf --set-rpath '$ORIGIN/../../lib' "$THEMES/libqxdgdesktopportal.so"
# c) The AppImage
"$CACHE/tools/linuxdeploy-x86_64.AppImage" --appdir "$APPDIR" --output appimage
# The plugin names the file after the program; the versioned name is the one for the release
mv -f "$APP_NAME"*-x86_64.AppImage "$RESULT"
ls -l "$RESULT"
