# Nesca

Сканер сети. Легендарный. Твой.

Этот репозиторий — форк [`S0Ulle33/nesca`](https://github.com/S0Ulle33/nesca),
который, в свою очередь, является форком
[`pantyusha/nesca`](https://github.com/pantyusha/nesca).

> **Используй сканер только на своих системах и сетях, где у тебя есть явное
> разрешение.** Автор форка не несёт ответственности за использование
> инструмента против чужих систем.

## Что добавлено в этом форке

- Headless CLI с JSONL-выводом (`--headless`), пригодный для скриптов и
  воспроизводимых проверок.
- Docker-сборка на Ubuntu 24.04 с Qt 5.15 и системными libssh/curl/openssl.
- Исправления Linux-сборки: OpenSSL 3, импорт списков целей, обнаружение
  SSH/FTP без брута, последний IP в диапазоне, гонка запуска воркеров.
- Фиксированный thread pool и атомарные счётчики вместо потока на каждый IP;
  бенчмарк — в [`docs/BENCHMARK.md`](docs/BENCHMARK.md).
- Изолированный Docker-полигон для проверки HTTP Basic, SSH, FTP и RTSP.
- Legacy Windows DLL вынесены из git; `HCNetSDK.dll` не распространяется.

Подробности:

- [`docs/CLI.md`](docs/CLI.md) — флаги, JSONL-схема, wordlists.
- [`docs/TESTLAB.md`](docs/TESTLAB.md) — Docker-полигон.
- [`docs/WINDOWS_DEPS.md`](docs/WINDOWS_DEPS.md) — DLL и Hikvision SDK.
- [`NOTICE.md`](NOTICE.md) — границы лицензии.

## Сборка

### Docker (рекомендуется)

```bash
docker build -t nesca:dev .
docker run --rm -e DISPLAY="$DISPLAY" \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  nesca:dev
```

Для headless-режима GUI не нужен:

```bash
docker run --rm --network host nesca:dev \
  ./nesca --headless --target 127.0.0.1 --ports 8080 --jsonl -
```

### Linux (нативно)

Нужны Qt 5.15, libssh, OpenSSL и libcurl:

```bash
sudo apt-get install qtbase5-dev qtbase5-dev-tools qtmultimedia5-dev \
  libqt5multimedia5-plugins libssh-dev libssl-dev libcurl4-openssl-dev \
  build-essential

git clone https://github.com/fedorov0av/nesca.git
cd nesca/src
qmake
make -j"$(nproc)"
```

`qmake` запускается из `src/`, а не из корня репозитория.

### Windows

Основная историческая сборка — Qt 5.14/5.15 + MinGW 32-bit. Инструкция:

1. Установи Qt с компонентами _MinGW 32-bit_, _Sources_ и _Qt Creator_.
2. Добавь в `PATH` каталоги `Qt\5.15.x\mingw81_32\bin` и
   `Qt\Tools\mingw810_32\bin` (версии могут отличаться).
3. Открой `src/nesca.pro` в Qt Creator, выбери MinGW, конфигурацию Release и
   собери проект.
4. Скопируй `nesca.exe` в отдельную папку и выполни
   `windeployqt.exe --compiler-runtime nesca.exe`.
5. Скопируй рядом нужные DLL из `src/3rdparty` (см.
   [`docs/WINDOWS_DEPS.md`](docs/WINDOWS_DEPS.md)) и папку `src/pwd_lists`.

VS2022-проект (`nesca.sln` + `nesca.vcxproj`) подготовлен под `src/`-layout,
но **не проверен** на Windows. Инструкция — в
[`BUILD_VS.md`](BUILD_VS.md). Для проверенной сборки пока используй MinGW
или Qt Creator.

## CLI

```bash
./nesca --headless --target 192.0.2.0/28 --ports 22,80,443 --jsonl -
```

Брут включается только явно:

```bash
./nesca --headless --target 192.0.2.10 --ports 22 \
  --brute --ssh-passlist ./wordlists/sshpass.txt --jsonl results.jsonl
```

Без `--brute` credential-списки не загружаются, и сканер выполняет только
обнаружение сервисов. Полная документация — в [`docs/CLI.md`](docs/CLI.md).

## Тестовый полигон

```bash
cd testlab
docker compose up -d --build
cd ..
./testlab/run-scan.sh
```

Полигон использует `172.28.0.0/24` и dummy-креды `test:test`. Не направляй
сканер на публичные или чужие диапазоны.

## Лицензия

Оригинальный код не содержит файла лицензии. `GPL-3.0-only` применяется
только к новым файлам этого форка; upstream-файлы и правки в них не
перелицензируются. Полный текст — в
[`LICENSES/GPL-3.0-only.txt`](LICENSES/GPL-3.0-only.txt), границы — в
[`NOTICE.md`](NOTICE.md).

`HCNetSDK.dll` — проприетарная библиотека Hikvision; она не входит в
репозиторий и релизы. Старые Windows DLL доступны только как
[release assets](https://github.com/fedorov0av/nesca/releases/tag/deps-2020.07)
для совместимости.

## Аудит безопасности

Пользователь [enemy-submarine](https://github.com/enemy-submarine) провёл
[частичный аудит](https://github.com/enemy-submarine/nesca_audit) upstream-кода.
Результаты относятся к исходной кодовой базе и не являются аудитом этого
форка.
