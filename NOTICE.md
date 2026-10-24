# Notice and licensing scope

This repository is a fork of [`S0Ulle33/nesca`](https://github.com/S0Ulle33/nesca),
which in turn is a fork of [`pantyusha/nesca`](https://github.com/pantyusha/nesca).
The upstream repositories do **not** contain a license file. All rights to the
original code remain with its authors.

## What GPL-3.0-only covers

`GPL-3.0-only` (see `LICENSES/GPL-3.0-only.txt`) applies only to **new files
created in this fork**, including:

- `src/ResultSink.h`, `src/ResultSink.cpp`
- `src/HeadlessOptions.h`
- `.dockerignore`
- `BUILD_VS.md`, `nesca.sln`, `nesca.vcxproj`, `vcpkg.json`
- `docs/**`
- `testlab/**`
- `LICENSES/**`
- `NOTICE.md`

## What it does not cover

Existing upstream files, and modifications made to them, are **not**
relicensed by this fork. They remain under the terms (or absence of terms) of
the original authors. This includes, among others:

- `src/main.cpp`, `src/nesca.cpp`, `src/nesca.h`, `src/MainStarter.cpp`
- `src/Connector.cpp`, `src/SSHAuth.cpp`, `src/BasicAuth.cpp`,
  `src/FTPAuth.cpp`, `src/RTSP.cpp`, `src/HikvisionLogin.cpp`,
  `src/IPCAuth.cpp`, `src/IPRandomizer.cpp`, `src/finder.cpp`
- `src/nesca.pro`, `Dockerfile`, `README.md`

If you redistribute the combined work, obtain permission from the original
authors. This notice is not legal advice; consult a lawyer before publishing
binaries or a combined GPL distribution.

## Third-party components

- **HCNetSDK.dll** is proprietary Hikvision software. It is not present in the
  current tree or in the release assets. The upstream history still contains
  it; this fork does not add or redistribute it. Obtain it from Hikvision and
  comply with their license.
- The legacy Windows DLLs (`libcurl.dll`, `libeay32.dll`, `ssh.dll`,
  `zlib1.dll`) are provided only as optional release assets for compatibility
  with old builds. They are covered by their own licenses (curl, OpenSSL,
  libssh, zlib). They are old and should not be used for new deployments.
- The Docker test lab uses third-party images (`nginx`, `fauria/vsftpd`,
  `bluenviron/mediamtx`, `ubuntu`); each image carries its own license.

## No warranty

The software is provided "as is", without warranty of any kind. Use it only
on systems and networks for which you have explicit authorization.
