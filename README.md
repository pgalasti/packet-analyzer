# packet-analyzer

A console based packet analyzer.

1. Pick your interface device
2. Watch the packets roll in
3. ???
4. Profit

## Build

### Prereqs
I'm trying out [FTXUI](https://github.com/ArthurSonzogni/FTXUI) for some cool UI stuff and libpcap for network capture.

```bash
# Debian apt
sudo apt install libftxui-dev libpcap-dev
```

```bash
# Mac via Homebrew
brew install ftxui libpcap
```

### Make
```bash
cd packet-analyzer

# Default release binary
make

# Debug symbol build
make debug
```

# Running
Note since you're using what is likely privileged operations for your device 
interfaces, you'll likely need to grant you'll need to setup a program rule
for it via whatever method you prefer.

I'm fairly lazy so I just:
```bash
sudo ./packet-analyzer
```
