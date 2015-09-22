Name:       iotivity-extensions-crosswalk
Version:    0.1
Release:    0
License:    BSD-3-Clause and Apache-2.0
Group:      Development/Libraries
Summary:    IoTivity Web APIs implemented using Crosswalk
URL:        https://github.com/crosswalk-project/iotivity-extensions-crosswalk
Source0:    %{name}-%{version}.tar.gz

BuildRequires: boost-devel
BuildRequires: iotivity-devel >= 0.9.2
BuildRequires: python
BuildRequires: pkgconfig(openssl)
BuildRequires: zip
BuildRequires: vim-base
Requires:      crosswalk
Requires:      iotivity >= 0.9.2

%description
IoTivity Web APIs implemented using Crosswalk.

%prep
%setup -q

%build
make %{?_smp_mflags}

%install
make install DESTDIR=%{buildroot}/%{_prefix}/lib/tizen-extensions-crosswalk

%files
%{_libdir}/tizen-extensions-crosswalk/libiotivity-extension.so
