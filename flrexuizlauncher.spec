Name: flrexuizlauncher
Version: 2023.09.10
Release: 1
Summary: FLRexuizLauncher
License: WTFPL

%description
Launcher application for Rexuiz

%install
rm -ifr %{buildroot}
mkdir -p -m 755 %{buildroot}/usr/bin/
install -m 755 flrexuizlauncher %{buildroot}/usr/bin/flrexuizlauncher
mkdir -p -m 755 %{buildroot}/usr/share/applications
install -m 644 flrexuizlauncher.desktop %{buildroot}/usr/share/applications/flrexuizlauncher.desktop
mkdir -p -m 755 %{buildroot}/usr/share/icons/hicolor/128x128/apps/
install -m 644 rexuiz_icon.png %{buildroot}/usr/share/icons/hicolor/128x128/apps/flrexuizlauncher.png

%clean
rm -ifr %{buildroot}

%files
%defattr(0755,root,root,0755)
/usr/bin/flrexuizlauncher
%defattr(0644,root,root,0755)
/usr/share/applications/flrexuizlauncher.desktop
/usr/share/icons/hicolor/128x128/apps/flrexuizlauncher.png
