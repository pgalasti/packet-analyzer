# packet-analyzer

A console based packet analyzer.

1. Pick your interface device
2. Watch the packets roll in
3. ???
4. Profit

## Build

### Prereqs
I'm trying out [FTXUI](https://github.com/ArthurSonzogni/FTXUI) for some cool UI stuff and libpcap for network capture.

I decided to go with [PcapPlusPlus](https://github.com/seladb/PcapPlusPlus) to not re-invent the steering wheel of parsing different network packet types.
I added it as a submodule so be sure to fetch submodules before you attempt to build:
```bash
git clone --recurse-submodules https://github.com/pgalasti/packet-analyzer.git

# If you already cloned
git submodule update --init --recursive
```

`make` builds the submodule for you (static, into `third_party/install`), so
CMake is needed but PcapPlusPlus itself does not have to be installed.

```bash
# Debian apt
sudo apt install libftxui-dev libpcap-dev cmake
```

```bash
# Mac via Homebrew
brew install ftxui libpcap cmake
```

### Make
```bash
cd packet-analyzer

# Default release binary
make

# Debug symbol build
make debug

# Clean build
make clean

# Clean everything
make distclean
```

# Running
Note since you're using what is likely privileged operations for your device 
interfaces, you'll likely need to setup a program rule for it via whatever method you prefer.

I'm fairly lazy so I just:
```bash
sudo ./packet-analyzer
```
