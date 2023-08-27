ARCH=$1
VERSION=2023.08.11
rm -rf deb/flrexuizlauncher
test -z "$ARCH" && ARCH=`uname -m`
case "$ARCH" in
x86_64)
	ARCH=amd64
	;;
x?86)
	ARCH=i386
	;;
aarch64)
	ARCH=arm64
	;;
esac
mkdir -p -m 755 deb/flrexuizlauncher/usr/bin/
install -m 755 flrexuizlauncher deb/flrexuizlauncher/usr/bin/flrexuizlauncher
mkdir -p -m 755 deb/flrexuizlauncher/usr/share/applications
install -m 644 flrexuizlauncher.desktop deb/flrexuizlauncher/usr/share/applications/flrexuizlauncher.desktop
mkdir -p -m 755 deb/flrexuizlauncher/usr/share/icons/hicolor/128x128/apps/
install -m 644 rexuiz_icon.png deb/flrexuizlauncher/usr/share/icons/hicolor/128x128/apps/flrexuizlauncher.png
mkdir -p -m 755 deb/flrexuizlauncher/DEBIAN
cat > deb/flrexuizlauncher/DEBIAN/control << EOF
Package: flrexuizlauncher
Version: 2023.08.27
Section: games
Priority: optional
Architecture: $ARCH
Maintainer: None <none@none.none>
Description: Launcher for rexuiz
EOF
cd deb || exit 1
dpkg-deb --root-owner-group --build flrexuizlauncher
mv flrexuizlauncher.deb flrexuizlauncher_"$VERSION.$ARCH".deb
