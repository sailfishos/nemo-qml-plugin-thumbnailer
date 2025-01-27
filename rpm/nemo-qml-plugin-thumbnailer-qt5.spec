Name:       nemo-qml-plugin-thumbnailer-qt5
Summary:    Thumbnail provider plugin for Nemo Mobile
Version:    1.0.7
Release:    1
License:    BSD
URL:        https://github.com/sailfishos/nemo-qml-plugin-thumbnailer
Source0:    %{name}-%{version}.tar.bz2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(mlite5)
BuildRequires:  sailfish-qdoc-template
Requires: thumbnaild
Provides: nemo-qml-plugin-thumbnailer-qt5-video

%description
%{summary}.

%package devel
Summary:    Thumbnail support for C++ applications
Requires:   %{name} = %{version}-%{release}

%description devel
%{summary}.

%package doc
Summary:    Thumbnailer plugin documentation

%description doc
%{summary}.

%prep
%setup -q -n %{name}-%{version}

%build
%qmake5 "VERSION=%{version}"
%make_build

%install
%qmake5_install

# org.nemomobile.thumbnailer legacy import
mkdir -p %{buildroot}%{_libdir}/qt5/qml/org/nemomobile/thumbnailer/
ln -sf %{_libdir}/qt5/qml/Nemo/Thumbnailer/libnemothumbnailer.so %{buildroot}%{_libdir}/qt5/qml/org/nemomobile/thumbnailer/
sed 's/Nemo.Thumbnailer/org.nemomobile.thumbnailer/' < src/plugin/qmldir > %{buildroot}%{_libdir}/qt5/qml/org/nemomobile/thumbnailer/qmldir

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%license LICENSE.BSD
%{_libdir}/libnemothumbnailer-qt5.so.*
%dir %{_libdir}/qt5/qml/Nemo/Thumbnailer
%{_libdir}/qt5/qml/Nemo/Thumbnailer/libnemothumbnailer.so
%{_libdir}/qt5/qml/Nemo/Thumbnailer/qmldir
%{_libdir}/qt5/qml/Nemo/Thumbnailer/plugins.qmltypes

# org.nemomobile.thumbnailer legacy import
%dir %{_libdir}/qt5/qml/org/nemomobile/thumbnailer
%{_libdir}/qt5/qml/org/nemomobile/thumbnailer/libnemothumbnailer.so
%{_libdir}/qt5/qml/org/nemomobile/thumbnailer/qmldir

%files devel
%{_libdir}/libnemothumbnailer-qt5.so
%{_libdir}/libnemothumbnailer-qt5.prl
%{_includedir}/nemothumbnailer-qt5/*.h
%{_libdir}/pkgconfig/nemothumbnailer-qt5.pc

%files doc
%dir %{_datadir}/doc/nemo-qml-plugin-thumbnailer
%{_datadir}/doc/nemo-qml-plugin-thumbnailer/nemo-qml-plugin-thumbnailer.qch
