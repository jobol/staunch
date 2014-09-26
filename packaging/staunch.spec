Name:           staunch
Version:        1
Release:        0
License:        MIT
Summary:        Secure Tizen Launcher
Url:            https://github.com/jobol/staunch
Group:          System/Configuration
Source0:        %{name}-%{version}.tar.xz
Source1001:     %{name}.manifest
BuildRequires:  libattr-devel
BuildRequires:  pkgconfig(security-manager)
Requires(post): /sbin/setcap

%description
Secure Tizen Launcher is an utility to set
secure environment to native programs.

%prep
%setup -q
cp %{SOURCE1001} .

%build
%reconfigure
make %{?_smp_mflags}

%install
%make_install

%post
/sbin/setcap cap_mac_admin,cap_sys_admin,cap_setgid=+pe-i /sbin/staunch

%docs_package

%files
%manifest %{name}.manifest
%{_bindir}/staunch
