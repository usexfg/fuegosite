# swapxfg M6: Wallet `swap` Command Rewrite

> **For agentic workers:** Use superpowers:executing-plans to implement task-by-task.

**Goal:** Replace the existing `swap` command in `SimpleWallet` (and `TestnetWallet`) with a launcher that starts `swapxfg` as a subprocess. Users type `swap` at the wallet prompt and get the full TUI.

**Prerequisite:** M2 — swapxfg must support `--wallet` flag so it can connect back to the running wallet RPC.

**Not in scope:** Any new swap logic — this is purely a launcher change.

---

## What currently exists

- `src/SimpleWallet/SimpleWallet.cpp` — has `swap`-related command entries (search for "swap" in command dispatch)
- `swapxfg` binary — built separately, installed alongside `fuego-wallet` / `fire_wallet`
- No IPC between wallet and swapxfg today

---

## The launch strategy

When user types `swap` at the wallet prompt:

1. SimpleWallet suspends its own terminal (restores cooked mode)
2. Forks `swapxfg --wallet http://127.0.0.1:<walletRpcPort> --daemon http://127.0.0.1:<daemonPort>`
3. Waits for swapxfg to exit
4. Restores raw terminal mode and resumes wallet prompt

This is the same pattern used by tools like `git` launching `vim` for commit messages.

---

## Task 1: Find and update SimpleWallet swap command

**File:** `src/SimpleWallet/SimpleWallet.cpp`

- [ ] **Step 1: Locate the swap command handler**

Search for the existing `swap` command dispatch. Replace its body with the launcher logic below.

- [ ] **Step 2: Implement the launcher**

```cpp
void SimpleWallet::launchSwapxfg() {
  // Build swapxfg command
  // Wallet RPC port is available as m_walletRpcPort (or equivalent member)
  // Daemon port is available from m_node or config

  std::string walletEndpoint = "http://127.0.0.1:" + std::to_string(m_walletRpcPort);
  std::string daemonEndpoint = "http://" + m_daemonHost + ":" + std::to_string(m_daemonPort);

  // Find swapxfg binary: same directory as current executable, then PATH
  std::string swapxfgPath = findSwapxfg();
  if (swapxfgPath.empty()) {
    logger(Logging::WARNING) << "swapxfg not found. Install it alongside fuego-wallet.";
    logger(Logging::INFO) << "Download: https://github.com/usexfg/fuego/releases";
    return;
  }

  // Restore terminal before forking
  Tools::PasswordContainer::restoreTerminal();

  // Fork + exec
  pid_t pid = fork();
  if (pid == 0) {
    // child
    execlp(swapxfgPath.c_str(), "swapxfg",
           "--wallet", walletEndpoint.c_str(),
           "--daemon", daemonEndpoint.c_str(),
           nullptr);
    _exit(1);  // exec failed
  } else if (pid > 0) {
    // parent: wait for swapxfg to exit
    int status;
    waitpid(pid, &status, 0);
  } else {
    logger(Logging::ERROR) << "fork() failed: " << strerror(errno);
  }

  // Restore raw mode for wallet prompt
  Tools::PasswordContainer::setRawTerminal();
}
```

- [ ] **Step 3: Implement `findSwapxfg()`**

```cpp
static std::string findSwapxfg() {
  // 1. Same directory as current executable
  char self[PATH_MAX];
  ssize_t len = readlink("/proc/self/exe", self, sizeof(self)-1);
  if (len > 0) {
    self[len] = '\0';
    std::string dir = std::string(self);
    dir = dir.substr(0, dir.rfind('/'));
    std::string candidate = dir + "/swapxfg";
    if (access(candidate.c_str(), X_OK) == 0) return candidate;
  }

  // 2. PATH lookup
  if (system("which swapxfg > /dev/null 2>&1") == 0) {
    return "swapxfg";  // found on PATH, let execlp resolve it
  }

  return "";
}
```

**Windows note:** On Windows, use `CreateProcess` instead of `fork`/`exec`. Add `#ifdef _WIN32` guards. `swapxfg.exe` in same directory.

---

## Task 2: Update TestnetWallet

**File:** `src/TestnetWallet/TestnetWallet.cpp`

- [ ] Same change as Task 1. TestnetWallet should pass `--testnet` flag to swapxfg:

```cpp
execlp(swapxfgPath.c_str(), "swapxfg",
       "--wallet", walletEndpoint.c_str(),
       "--daemon", daemonEndpoint.c_str(),
       "--testnet",
       nullptr);
```

---

## Task 3: Graceful fallback message

If `swapxfg` is not found, print a helpful message instead of silently failing:

```
swapxfg not found. To use the swap terminal, install swapxfg:
  - Download from https://github.com/usexfg/fuego/releases
  - Place swapxfg in the same directory as fuego-wallet

Alternatively, run swapxfg manually:
  swapxfg --wallet http://127.0.0.1:18182 --daemon http://127.0.0.1:18180
```

---

## Task 4: Verify

- [ ] Build `fuego-wallet` and `swapxfg` — both compile cleanly
- [ ] `swap` in wallet with `swapxfg` present → swapxfg launches, wallet resumes after quit
- [ ] `swap` in wallet without `swapxfg` → helpful error message with download URL
- [ ] Ctrl+C in swapxfg → wallet prompt returns cleanly (no broken terminal state)
- [ ] `swap` in testnet wallet → swapxfg launches with `--testnet` flag
