Name:       libmm-utility
Summary:    Multimedia Framework Utility Library
Version:    0.17
Release:    0
Group:      System/Libraries
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1001: libmm-utility.manifest
Requires(post):    /sbin/ldconfig
Requires(postun):  /sbin/ldconfig
BuildRequires:  pkgconfig(mm-common)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(gmodule-2.0)
BuildRequires:  libjpeg-turbo-devel
BuildRequires:  pkgconfig(libtzplatform-config)
BuildRequires:  pkgconfig(capi-media-tool)
BuildRequires:  pkgconfig(libtbm)
BuildRequires:  pkgconfig(libexif)
BuildRequires:  pkgconfig(opencv)
BuildRequires:  pkgconfig(capi-system-info)
BuildRequires:  pkgconfig(ttrace)
BuildRequires:  libpng-devel
BuildRequires:  giflib-devel
BuildRequires:  libbmp-devel
BuildRequires:  libnsbmp-devel
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
export CFLAGS+=" -Werror"
CFLAGS="$CFLAGS -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\" -D_MM_PROJECT_FLOATER" \
LDFLAGS="$LDFLAGS -Wl,--rpath=%{_libdir} -Wl,--hash-style=both -Wl,--as-needed" \
%reconfigure
%__make %{?_smp_mflags}

sed -i -e "s#@IMGP_REQPKG@#$IMGP_REQPKG#g" imgp/mmutil-imgp.pc
sed -i -e "s#@JPEG_REQPKG@#$JPEG_REQPKG#g" jpeg/mmutil-jpeg.pc
sed -i -e "s#@IMGCV_REQPKG@#$IMGCV_REQPKG#g" imgcv/mmutil-imgcv.pc
sed -i -e "s#@PNG_REQPKG@#$PNG_REQPKG#g" png/mmutil-png.pc
sed -i -e "s#@GIF_REQPKG@#$GIF_REQPKG#g" gif/mmutil-gif.pc
sed -i -e "s#@BMP_REQPKG@#$BMP_REQPKG#g" bmp/mmutil-bmp.pc

%install
rm -rf %{buildroot}
%make_install
mkdir -p %{buildroot}/usr/share/license
cp LICENSE %{buildroot}/usr/share/license/%{name}

%clean
rm -rf %{buildroot}

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
/usr/share/license/%{name}
%manifest %{name}.manifest
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

