Name:           harbour-foilpics
Summary:        Encrypted pictures
Version:        1.1.3
Release:        1
License:        BSD
Group:          Applications/File
URL:            https://github.com/monich/harbour-foilpics
Source0:        %{name}-%{version}.tar.gz

Requires:       sailfishsilica-qt5
Requires:       qt5-qtsvg-plugin-imageformat-svg
BuildRequires:  pkgconfig(zlib)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(libcrypto)
BuildRequires:  pkgconfig(sailfishapp)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Svg)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(mlite5)
BuildRequires:  qt5-qttools-linguist

%{!?qtc_qmake5:%define qtc_qmake5 %qmake5}
%{!?qtc_make:%define qtc_make make}
%{?qtc_builddir:%define _builddir %qtc_builddir}

%description
Picture encryption application.

%if "%{?vendor}" == "chum"
Categories:
 - Utility
Icon: https://raw.githubusercontent.com/monich/harbour-foilpics/master/icons/harbour-foilpics.svg
Screenshots:
- https://home.monich.net/chum/harbour-foilpics/screenshots/screenshot-001.png
- https://home.monich.net/chum/harbour-foilpics/screenshots/screenshot-002.png
- https://home.monich.net/chum/harbour-foilpics/screenshots/screenshot-003.png
- https://home.monich.net/chum/harbour-foilpics/screenshots/screenshot-004.png
- https://home.monich.net/chum/harbour-foilpics/screenshots/screenshot-005.png
- https://home.monich.net/chum/harbour-foilpics/screenshots/screenshot-006.png
- https://home.monich.net/chum/harbour-foilpics/screenshots/screenshot-007.png
Url:
  Homepage: https://openrepos.net/content/slava/foil-pics
%endif

%prep
%setup -q -n %{name}-%{version}

%build
%qtc_qmake5 CONFIG+=openrepos %{name}.pro
%qtc_make %{?_smp_mflags}

%install
rm -rf %{buildroot}
%qmake5_install -f Makefile.app

desktop-file-install --delete-original \
  --dir %{buildroot}%{_datadir}/applications \
   %{buildroot}%{_datadir}/applications/*.desktop

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
%{_datadir}/jolla-settings/entries/%{name}.json
%{_datadir}/translations/%{name}*.qm
