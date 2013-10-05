Name: libwhiteboard
Summary: smart-m3 component
Version: 1.0
Release: 1
Group: Applications/System
License: GPLv3
URL: https://github.com/smart-m3/
Source0: libwhiteboard-1.0.tar.bz2
BuildRoot: %{_tmppath}/libwhiteboard-root
Requires: libuuid, expat


%description

%prep
%setup -q -n %{name}-%{version}

%build
./autogen.sh
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install

%clean
make clean
rm -rf $RPM_BUILD_ROOT

%post
echo /usr/local/lib >> /etc/ld.so.conf
ldconfig

%files
%defattr(-, root, root)
/usr/
%changelog
* Fri Oct 20 2013 Gubin Pavel
- Initial build


