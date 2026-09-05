# packet-analyzer

## Build

### Prereqs
I'm trying out [FTXUI](https://github.com/ArthurSonzogni/FTXUI) for some cool UI and libpcap for network capture.

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
