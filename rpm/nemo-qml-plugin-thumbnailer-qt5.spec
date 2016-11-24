Name:       nemo-qml-plugin-thumbnailer-qt5
Summary:    Thumbnail provider plugin for Nemo Mobile
Version:    0.0.0
Release:    1
Group:      System/Libraries
License:    BSD
URL:        https://github.com/nemomobile/nemo-qml-plugin-thumbnailer
Source0:    %{name}-%{version}.tar.bz2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(mlite5)
BuildRequires:  mer-qdoc-template
BuildRequires:  oneshot
%{_oneshot_requires_post}

%description
%{summary}.

%package devel
Summary:    Thumbnail support for C++ applications
Group:      System/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
%{summary}.

%package doc
Summary:    Thumbnailer plugin documentation
Group:      System/Libraries

%description doc
%{summary}.

%prep
%setup -q -n %{name}-%{version}

%build
%qmake5
make %{?jobs:-j%jobs}


%install
rm -rf %{buildroot}
%qmake5_install
chmod +x %{buildroot}/%{_oneshotdir}/*


%files
%defattr(-,root,root,-)
%{_libdir}/libnemothumbnailer-qt5.so.*
%{_libdir}/qt5/qml/org/nemomobile/thumbnailer/libnemothumbnailer.so
%{_libdir}/qt5/qml/org/nemomobile/thumbnailer/qmldir
%{_libdir}/qt5/qml/org/nemomobile/thumbnailer/plugins.qmltypes
%{_oneshotdir}/remove-obsolete-nemothumbs-cache-dir

%files devel
%defattr(-,root,root,-)
%{_libdir}/libnemothumbnailer-qt5.so
%{_libdir}/libnemothumbnailer-qt5.prl
%{_includedir}/nemothumbnailer-qt5/*.h
%{_libdir}/pkgconfig/nemothumbnailer-qt5.pc

%files doc
%defattr(-,root,root,-)
%dir %{_datadir}/doc/nemo-qml-plugin-thumbnailer
%{_datadir}/doc/nemo-qml-plugin-thumbnailer/nemo-qml-plugin-thumbnailer.qch

%post
/sbin/ldconfig
%{_bindir}/add-oneshot --now remove-obsolete-nemothumbs-cache-dir

%postun -p /sbin/ldconfig

