#
# spec file for package exempi
#
# Copyright (c) 2007 Novell Inc.
# This file and all modifications and additions to the pristine
# package are under the same license as the package itself.
#

# norootforbuild


Summary: XMP support library
Name: exempi
Version: 1.99.2
Release: 1
License: BSD
Group: System/Libraries
Source0: http://libopenraw.freedesktop.org/download/%name-%{version}.tar.gz
BuildRoot: /var/tmp/%{name}-buildroot
BuildRequires: boost-devel, expat
%define prefix   /usr

%description
XMP parsing and IO library

%prep
%setup -q


%build
CFLAGS="$RPM_OPT_FLAGS" \
    ./configure --prefix=%prefix \
    --libdir=/usr/%_lib
make

%install
rm -rf $RPM_BUILD_ROOT
DESTDIR=$RPM_BUILD_ROOT make install

%post
%run_ldconfig

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc README COPYING ChangeLog

%{prefix}/%{_lib}/libexempi.*
%{prefix}/include/exempi-2.0/*
%{prefix}/%{_lib}/pkgconfig/*.pc

%changelog
