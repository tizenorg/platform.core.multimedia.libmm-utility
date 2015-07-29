Name:       libmm-utility
Summary:    Multimedia Framework Utility Library
Version:    0.12
Release:    0
Group:      System/Libraries
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1001: libmm-utility.manifest
Requires(post):    /sbin/ldconfig
Requires(postun):  /sbin/ldconfig
BuildRequires:  pkgconfig(mm-common)
BuildRequires:  pkgconfig(mm-log)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(gmodule-2.0)
BuildRequires:  libjpeg-turbo-devel
BuildRequires:  pkgconfig(libtzplatform-config)
BuildRequires:  pkgconfig(capi-media-tool)
BuildRequires:  pkgconfig(libtbm)
BuildRequires:  pkgconfig(libexif)
BuildRequires:  pkgconfig(capi-system-info)
BuildRequires:  pkgconfig(ttrace)
BuildRoot:  %{_tmppath}/%{name}-%{version}-build

%description
Multimedia Framework Utility Library - Main package.

%package devel
Summary:    Multimedia Framework Utility Library (DEV)
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
Multimedia Framework Utility Library - Development files.

%package tool
Summary:    Multimedia Framework Utility tools
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description tool
Multimedia Framework Utility Library - Tools.

%prep
%setup -q
cp %{SOURCE1001} .

%build
mkdir -p m4
export CFLAGS+=" -Wextra -Wno-array-bounds"
export CFLAGS+=" -Wno-ignored-qualifiers -Wno-unused-parameter -Wshadow"
export CFLAGS+=" -Wwrite-strings -Wswitch-default"
CFLAGS="$CFLAGS -DENABLE_TTRACE -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\" -D_MM_PROJECT_FLOATER" \
LDFLAGS="$LDFLAGS -Wl,--rpath=%{_libdir} -Wl,--hash-style=both -Wl,--as-needed" \
%reconfigure
%__make %{?_smp_mflags}

sed -i -e "s#@IMGP_REQPKG@#$IMGP_REQPKG#g" imgp/mmutil-imgp.pc
sed -i -e "s#@JPEG_REQPKG@#$JPEG_REQPKG#g" jpeg/mmutil-jpeg.pc

%install
rm -rf %{buildroot}
%make_install

%clean
rm -rf %{buildroot}

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%manifest %{name}.manifest
%license LICENSE LICENSE.APLv2.0
%defattr(-,root,root,-)
%{_libdir}/*.so*

%files devel
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_includedir}/*
%{_libdir}/pkgconfig/*

%files tool
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_bindir}/*_testsuite

