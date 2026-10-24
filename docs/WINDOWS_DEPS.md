# Windows runtime dependencies

The upstream Windows build expects several runtime DLLs next to `nesca.exe`.
They are no longer tracked in git. Use current dependencies where possible.

## Recommended: vcpkg

The prepared Visual Studio project (see `docs/WINDOWS_BUILD.md`) uses vcpkg
for `libssh`, `curl` and `openssl`. Build the x86 triplet because the
Hikvision SDK is x86-only:

```powershell
vcpkg install libssh:x86-windows curl:x86-windows openssl:x86-windows
```

Then copy the required DLLs next to the executable, or run `windeployqt` and
the vcpkg `applocal` deployment step.

## Legacy DLLs (release assets)

The old DLLs from the 2020 dev branch are kept as release assets for
compatibility only: [deps-2020.07](https://github.com/fedorov0av/nesca/releases/tag/deps-2020.07).
They are old and should not be used for new deployments.

| File | SHA-256 |
|---|---|
| `libcurl.dll` | `e8659849e1bf5e7a302b05235159b9f484b43deac73ac4769fad589dc39203c1` |
| `libeay32.dll` | `bcb7bf47bdee4248038cae0781fe2975475e59f696b265cef1d211c8c0a9cbce` |
| `ssh.dll` | `1a12c971454116b444e6647b0507a92e10257bce81376d5be692d16f41cd7e69` |
| `zlib1.dll` | `1a694212565dcc1413b2cbcc9c77fec9017b892dd08effc5784681d1fb699415` |

The release asset does **not** include `HCNetSDK.dll`.

## Hikvision SDK

`HCNetSDK.dll` is proprietary Hikvision software and is not present in the
current tree or the release assets. The upstream history still contains it;
this fork does not add or redistribute it. Obtain the Hikvision SDK from
Hikvision, then place the x86 `HCNetSDK.dll` next to `nesca.exe`. The
application loads it at runtime:

```cpp
LoadLibraryW(L".\\HCNetSDK.dll");
```

Without it, Hikvision-specific functionality is disabled; the rest of the
scanner works.

## Verification

```powershell
Get-FileHash .\libcurl.dll -Algorithm SHA256
Get-FileHash .\HCNetSDK.dll -Algorithm SHA256
```

Compare the first four hashes with the table above. Do not redistribute
`HCNetSDK.dll` unless your Hikvision license allows it.
