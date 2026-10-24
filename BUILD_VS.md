# Сборка в Visual Studio 2022 (экспериментально)

В корне лежат `nesca.sln` + `nesca.vcxproj`, сгенерированные под `src/`-layout
этого форка. Проект **не проверен** на Windows: это заготовка для этапа
«Windows later». Если что-то не собирается — issue/PR приветствуются.

Qt VS Tools не нужен: `moc`/`uic`/`rcc` вызываются через Custom Build Steps,
сгенерированные файлы попадают в `<Platform>\<Configuration>\obj\GeneratedFiles\`.

## Что нужно

| Зависимость | Откуда |
|---|---|
| Visual Studio 2022 (v143, Windows 10/11 SDK) | visualstudio.microsoft.com |
| Qt 5.15.2 для MSVC (`msvc2019_64` и/или `msvc2019`) | qt.io / архивы Qt |
| libssh, libcurl, OpenSSL | vcpkg |

Qt 6 не подойдёт: код использует `QTextCodec`.

## vcpkg

```bat
git clone https://github.com/microsoft/vcpkg C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
setx VCPKG_ROOT C:\vcpkg
C:\vcpkg\vcpkg install --triplet x86-windows
:: опционально:
C:\vcpkg\vcpkg install --triplet x64-windows
```

Проект ищет `$(VCPKG_ROOT)\installed\<triplet>\include|lib`, а если
`VCPKG_ROOT` не задан — `vcpkg_installed\<triplet>` рядом с `.vcxproj`.

## Сборка

```bat
set QTDIR=C:\Qt\5.15.2\msvc2019
msbuild nesca.sln /m /p:Configuration=Release /p:Platform=Win32
```

Для x64:

```bat
set QTDIR=C:\Qt\5.15.2\msvc2019_64
msbuild nesca.sln /m /p:Configuration=Release /p:Platform=x64
```

Готовый `nesca.exe` — в `Win32\Release\` или `x64\Release\`. `windeployqt`
запускается автоматически.

## Нюансы

- **Hikvision:** `HCNetSDK.lib` не линкуется. Код загружает
  `HCNetSDK.dll` в рантайме через `LoadLibrary`/`GetProcAddress`; положи
  x86-версию DLL рядом с `nesca.exe`. Без неё Hikvision-функции отключаются.
- Рабочая директория отладки — `src/`, потому что `pwd_lists` и `restore`
  ищутся относительно неё.
- `nesca.rc` использует относительный `nesca.ico` и не требует MFC.
- Проект использует C++14 (`stdcpp14`), `/utf-8`, `ConformanceMode=false` —
  код старый и собирался с `-fpermissive`.
- Legacy DLL (`libcurl`, `libeay32`, `ssh`, `zlib1`) не входят в git; см.
  [`docs/WINDOWS_DEPS.md`](docs/WINDOWS_DEPS.md).
