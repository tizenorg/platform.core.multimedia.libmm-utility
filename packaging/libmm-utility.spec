Name:       libmm-utility
Summary:    Multimedia Framework Utility Library
Version:    0.7
Release:    0
Group:      System/Libraries
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1001:  libmm-utility.manifest
Requires(post):    /sbin/ldconfig
Requires(postun):  /sbin/ldconfig
BuildRequires:  pkgconfig(mm-common)
BuildRequires:  pkgconfig(mm-log)
BuildRequires:  pkgconfig(mm-ta)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(gmodule-2.0)
BuildRequires:  libjpeg-turbo-devel

BuildRoot:  %{_tmppath}/%{name}-%{version}-build

%description
Multimedia Framework Utility Library package.

%package devel
Summary:    Multimedia Framework Utility Library (DEV)
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
Multimedia Framework Utility Library (DEV) package.

%package tool
Summary:    Multimedia Framework Utility Library
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description tool
Multimedia Framework Utility Library package.

%prep
%setup -q
cp %{SOURCE1001} .

%build
mkdir -p m4
CFLAGS="$CFLAGS -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\" -D_MM_PROJECT_FLOATER" \
LDFLAGS+="-Wl,--rpath=%{_libdir} -Wl,--hash-style=both -Wl,--as-needed" \
%reconfigure
%__make %{?jobs:-j%jobs}

sed -i -e "s#@IMGP_REQPKG@#$IMGP_REQPKG#g" imgp/mmutil-imgp.pc
sed -i -e "s#@JPEG_REQPKG@#$JPEG_REQPKG#g" jpeg/mmutil-jpeg.pc

%install
rm -rf %{buildroot}
%make_install
mkdir -p %{buildroot}%{_datadir}/license
cp LICENSE %{buildroot}%{_datadir}/license/%{name}

%clean
rm -rf %{buildroot}

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%manifest %{name}.manifest
%{_datadir}/license/%{name}
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
