# ============================================================================
# Section 39.2 : Creation de paquets RPM (RedHat/CentOS)
# Description : Fichier spec RPM pour syswatch
# Fichier source : 02-paquets-rpm.md
# ============================================================================

Name:           syswatch
Version:        1.2.0
Release:        1%{?dist}
Summary:        Outil de monitoring systeme haute performance

License:        Proprietary
URL:            https://github.com/exemple/syswatch
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc-c++ >= 12
BuildRequires:  cmake >= 3.25
BuildRequires:  ninja-build

%description
Syswatch collecte des metriques CPU, memoire et disque
en temps reel et les expose via un endpoint Prometheus.

Concu pour les environnements de production Linux.

# Preparation : decompression des sources
%prep
%autosetup -n %{name}-%{version}

# Compilation
%build
%cmake -G Ninja -DCMAKE_BUILD_TYPE=Release
%cmake_build

# Installation dans le buildroot
%install
%cmake_install

install -Dm 0644 config/syswatch.yaml \
    %{buildroot}/etc/syswatch/syswatch.yaml

install -Dm 0644 systemd/syswatch.service \
    %{buildroot}%{_unitdir}/syswatch.service

install -d -m 0750 %{buildroot}/var/lib/syswatch
install -d -m 0750 %{buildroot}/var/log/syswatch

# Scripts de cycle de vie
%pre
getent group syswatch > /dev/null 2>&1 || groupadd -r syswatch
getent passwd syswatch > /dev/null 2>&1 || \
    useradd -r -g syswatch -d /nonexistent -s /sbin/nologin \
            -c "Syswatch monitoring agent" syswatch

%post
%systemd_post syswatch.service

%preun
%systemd_preun syswatch.service

%postun
%systemd_postun_with_restart syswatch.service

# Liste des fichiers installes
%files
%{_bindir}/syswatch
%{_unitdir}/syswatch.service

%dir /etc/syswatch
%config(noreplace) /etc/syswatch/syswatch.yaml

%attr(0750, syswatch, syswatch) %dir /var/lib/syswatch
%attr(0750, syswatch, syswatch) %dir /var/log/syswatch

# Changelog
%changelog
* Tue Mar 17 2026 Equipe SRE <sre@exemple.com> - 1.2.0-1
- Version initiale du paquet RPM
- Support Prometheus endpoint
- Configuration hot-reload via SIGHUP
