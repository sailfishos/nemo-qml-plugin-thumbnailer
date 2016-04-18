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
BuildRequires:  pkgconfig(gstreamer-0.10)
BuildRequires:  pkgconfig(gstreamer-app-0.10)

%description
%{summary}.

%package video
Summary:    Video thumbnailer provider
Group:      System/Libraries
Requires:   %{name} = %{version}-%{release}

%description video
%{summary}.

%prep
%setup -q -n %{name}-%{version}


%build

%qmake5

make %{?jobs:-j%jobs}


%install
rm -rf %{buildroot}
%qmake5_install


%files
%defattr(-,root,root,-)
%{_libdir}/qt5/qml/org/nemomobile/thumbnailer/libnemothumbnailer.so
%{_libdir}/qt5/qml/org/nemomobile/thumbnailer/qmldir
%{_libdir}/qt5/qml/org/nemomobile/thumbnailer/plugins.qmltypes

%files video
%defattr(-,root,root,-)
%{_libdir}/qt5/qml/org/nemomobile/thumbnailer/thumbnailers/libvideothumbnailer.so
