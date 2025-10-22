# Complete Build Guide: iSH 64-bit on macOS

This guide walks you through cloning, building, and running the 64-bit iSH on macOS with Xcode and iOS Simulator.

---

## Prerequisites

### What You Need

✅ **macOS** (Big Sur 11.0 or later recommended)
✅ **Xcode** (14.0 or later recommended)
✅ **Xcode Command Line Tools**
✅ **Git** (comes with Xcode CLT)
✅ **Apple Developer Account** (free account works!)

### Step 0: Verify Prerequisites

Open Terminal and run:

```bash
# Check Xcode version
xcodebuild -version
# Should show: Xcode 14.x or later

# Check git
git --version
# Should show: git version 2.x.x

# Check command line tools
xcode-select -p
# Should show: /Applications/Xcode.app/Contents/Developer
```

**If any command fails:**

```bash
# Install Xcode Command Line Tools
xcode-select --install

# If Xcode isn't installed
# Download from Mac App Store or developer.apple.com
```

---

## Part 1: Clone the Repository

### Step 1.1: Create a Working Directory

```bash
# Open Terminal (Cmd+Space, type "Terminal")

# Create a development folder
mkdir -p ~/Development
cd ~/Development
```

### Step 1.2: Clone iSH Repository

**Option A: If you have the original iSH repo URL:**

```bash
# Replace with your actual repository URL
git clone https://github.com/YOUR_USERNAME/ish.git
cd ish
```

**Option B: If cloning from a specific remote:**

```bash
# Example with the original iSH repository
git clone https://github.com/ish-app/ish.git
cd ish
```

### Step 1.3: Checkout the 64-bit Branch

```bash
# Fetch all branches
git fetch --all

# Checkout our 64-bit transformation branch
git checkout claude/ish-64bit-port-011CUN2o2wGVYoigGBk1Yh98

# Verify you're on the correct branch
git branch
# Should show: * claude/ish-64bit-port-011CUN2o2wGVYoigGBk1Yh98

# Check recent commits
git log --oneline -5
# Should show:
# 6a97f0c Critical fixes for compilation compatibility
# 1e8a270 Phase 5: Testing infrastructure and transformation completion
# 5fea022 Phase 4: 64-bit system call handler
# d38f5bc Phase 3: 64-bit gadget support (iOS-safe threading)
# 2c8b13a Phase 2: 64-bit instruction decoder implementation
```

### Step 1.4: Verify Source Code

```bash
# List key files to confirm 64-bit changes
ls -la emu/cpu.h kernel/calls.c asbestos/gen.c

# Quick check: ensure addr_t is 64-bit
grep "typedef.*addr_t" misc.h
# Should show: typedef qword_t addr_t;

# Check for 64-bit syscall table
grep "syscall_table_64" kernel/calls.c
# Should show: syscall_t syscall_table_64[] = {
```

---

## Part 2: Build Dependencies (Optional but Recommended)

iSH uses **meson** build system. While Xcode project is available, building dependencies ensures everything works.

### Step 2.1: Install Homebrew (if not installed)

```bash
# Check if Homebrew is installed
which brew

# If not found, install Homebrew
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

### Step 2.2: Install Build Dependencies

```bash
# Install meson and ninja (build tools)
brew install meson ninja

# Install other dependencies (may already be present)
brew install pkg-config

# Verify installation
meson --version
ninja --version
```

### Step 2.3: Build with Meson (Test Build)

```bash
# Navigate to iSH directory
cd ~/Development/ish

# Create build directory
meson setup build

# Build the project
ninja -C build

# This will show if there are any compilation errors
# Expected: Successful build OR minor warnings (acceptable)
```

**Expected Output:**

```
[1/234] Compiling C object emu/libemu.a.p/cpu.c.o
[2/234] Compiling C object kernel/libkernel.a.p/calls.c.o
...
[234/234] Linking target iSH
```

**If build fails:** Note the error and skip to Part 4 (Troubleshooting)

---

## Part 3: Build with Xcode (iOS Simulator)

### Step 3.1: Open Xcode Project

```bash
# Still in ~/Development/ish directory

# Open the Xcode project
open iSH.xcodeproj
```

**Alternative:** Double-click `iSH.xcodeproj` in Finder

### Step 3.2: Configure Xcode Project

#### A. Select the iSH Target

1. In Xcode, click the **scheme selector** (top-left, near Play button)
2. Select **iSH** as the target
3. Select **Any iOS Simulator** or a specific simulator (e.g., **iPad Pro 12.9"**)

#### B. Configure Signing & Capabilities

1. Click **iSH** project in left sidebar
2. Select **iSH** target
3. Go to **Signing & Capabilities** tab
4. Check **Automatically manage signing**
5. Select your **Team** (Apple ID / Developer account)
   - If no team: Click "Add Account" and sign in with Apple ID
   - Free accounts work fine for simulator testing!

**Important Settings:**

- **Bundle Identifier:** Change to something unique like `com.yourname.ish64`
  - This avoids conflicts with official iSH app
- **Deployment Target:** iOS 14.0 or later

#### C. Update Build Settings (Optional but Recommended)

1. Click **iSH** project → **Build Settings** tab
2. Search for: **ENABLE_BITCODE**
   - Set to **NO** (if not already)
3. Search for: **VALID_ARCHS**
   - Should include: `arm64` and `x86_64` (for simulator)

### Step 3.3: Select iOS Simulator

1. Click scheme selector (top-left)
2. Choose a simulator:
   - **iPad Pro 12.9" (6th generation)** - Best for terminal use
   - **iPad Air (5th generation)** - Good alternative
   - **iPhone 14 Pro** - Works but smaller screen

**If simulators are missing:**

```bash
# Open Simulator separately
open -a Simulator

# Then go to: File → Open Simulator → [Choose device]
```

### Step 3.4: Build the Project

**Method 1: Build Only**

- Press **Cmd+B** (Build)
- Wait for build to complete
- Check for errors in **Report Navigator** (Cmd+9)

**Method 2: Build and Run**

- Press **Cmd+R** (Run)
- This builds AND launches in simulator

**Expected Build Time:** 2-5 minutes (first build)

**Expected Output:**

```
Build Succeeded
```

**If build fails:** See Part 4 (Troubleshooting)

---

## Part 4: Run iSH in iOS Simulator

### Step 4.1: Launch the App

If you used **Cmd+R**, the app should already be running.

**Manual Launch:**

1. Build completed successfully (Cmd+B)
2. Press **Cmd+R** to run
3. Simulator window opens
4. iSH app launches automatically

### Step 4.2: Initial Setup

When iSH first launches:

1. **Grant Permissions** (if asked)
   - File access: Allow
   - Notifications: Allow (optional)

2. **Terminal Appears**
   - You should see a Linux-like terminal prompt
   - Default: Alpine Linux

**Expected Prompt:**

```
localhost:~#
```

### Step 4.3: Basic Verification

Test that iSH is working:

```bash
# Check OS version
uname -a
# Should show: Linux localhost ... i686 (32-bit for now)

# Check available tools
ls /bin

# Test simple command
echo "Hello from iSH!"

# Check current directory
pwd
```

---

## Part 5: Test 64-bit Support

Now let's verify the 64-bit transformation works!

### Step 5.1: Check Current Architecture

```bash
# In iSH terminal
uname -m
# Currently shows: i686 (32-bit)

# This is expected! The rootfs is still 32-bit
# We need to test with a 64-bit binary
```

### Step 5.2: Create Test Programs

**We can't compile directly in iSH yet**, so we'll prepare test binaries on macOS.

#### On macOS Terminal (NOT in iSH):

```bash
# Navigate to a temp directory
cd ~/Development

# Create a simple C test program
cat > test_hello.c << 'EOF'
#include <stdio.h>
#include <unistd.h>

int main() {
    printf("Hello from 64-bit program!\n");
    printf("Process ID: %d\n", getpid());

    // Verify we're 64-bit
    printf("sizeof(void*) = %zu (should be 8 for 64-bit)\n", sizeof(void*));
    printf("sizeof(long) = %zu (should be 8 for 64-bit)\n", sizeof(long));

    return 0;
}
EOF

# Compile as 64-bit static binary
# (Need to use a Linux cross-compiler or Docker)
```

**Issue:** macOS can't directly create Linux binaries!

### Step 5.3: Use Docker to Create Linux Binaries

```bash
# Install Docker Desktop for Mac (if not installed)
# Download from: https://www.docker.com/products/docker-desktop

# Create 64-bit Linux binary using Docker
docker run --rm -v "$PWD":/work -w /work gcc:latest \
  gcc -m64 -static -o test_hello_64 test_hello.c

# Create 32-bit Linux binary for comparison
docker run --rm -v "$PWD":/work -w /work gcc:latest \
  gcc -m32 -static -o test_hello_32 test_hello.c

# Verify the binaries
file test_hello_32
# Should show: ELF 32-bit LSB executable, Intel 80386

file test_hello_64
# Should show: ELF 64-bit LSB executable, x86-64
```

### Step 5.4: Transfer Binaries to iSH

**Method 1: Using iSH's Files App Integration**

1. Copy test binaries to **iCloud Drive** or **Files app**
2. In iSH, mount iOS filesystem:
   ```bash
   # In iSH terminal
   mount -t ios . /mnt
   cd /mnt
   ls
   ```

**Method 2: Using Simulator File Sharing**

```bash
# On macOS, find simulator container
xcrun simctl get_app_container booted com.yourname.ish64 data

# This shows a path like:
# /Users/you/Library/Developer/CoreSimulator/Devices/.../data

# Copy files there
cp test_hello_32 test_hello_64 [path_from_above]/Documents/
```

**Method 3: Create Binaries Inside iSH (Simpler!)**

In iSH terminal:

```bash
# Install Alpine Linux development tools
apk add alpine-sdk

# Note: This installs 32-bit compiler
# We'll create assembly test instead
```

### Step 5.5: Assembly Test for 64-bit (Best Method!)

Create a minimal 64-bit assembly program directly:

**On macOS:**

```bash
cd ~/Development

# Create 64-bit assembly test
cat > test64.asm << 'EOF'
; Minimal 64-bit test program
section .data
    msg db 'Hello from 64-bit!', 0x0A
    len equ $ - msg

section .text
global _start

_start:
    ; write(1, msg, len)
    mov rax, 1          ; syscall number for write (64-bit)
    mov rdi, 1          ; fd = 1 (stdout)
    lea rsi, [rel msg]  ; buf = &msg (RIP-relative!)
    mov rdx, len        ; count = len
    syscall             ; Use SYSCALL instruction

    ; exit(0)
    mov rax, 60         ; syscall number for exit (64-bit)
    xor rdi, rdi        ; status = 0
    syscall
EOF

# Assemble with Docker
docker run --rm -v "$PWD":/work -w /work nasm:latest \
  nasm -f elf64 test64.asm -o test64.o

docker run --rm -v "$PWD":/work -w /work gcc:latest \
  ld -o test64 test64.o

# Verify
file test64
# Should show: ELF 64-bit LSB executable, x86-64
```

### Step 5.6: Test in iSH (THE MOMENT OF TRUTH!)

Transfer `test64` to iSH using one of the methods above, then:

```bash
# In iSH terminal
chmod +x test64

# Run the 64-bit program!
./test64
```

**Expected Result:**

✅ **Success:** "Hello from 64-bit!"

**Possible Issues:**

❌ **"Exec format error"** → ELF64 loader not working
❌ **"Illegal instruction"** → REX prefix decoder issue
❌ **Crashes** → Runtime bug, check Xcode console

---

## Part 6: Debugging and Logs

### Step 6.1: View Xcode Console Output

While iSH is running in simulator:

1. In Xcode, open **Console** (Cmd+Shift+C)
2. Or click **Debug Area** button (Cmd+Shift+Y)

**Look for:**

```
Loading ELF: test64
ELF class: 64-bit
Entry point: 0x401000
REX.W=1 REX.R=0 REX.X=0 REX.B=0
IP: 0x401000: MOV rax, 1
...
1234 call64 1   = 0x13
```

### Step 6.2: Enable Debug Logging (If Needed)

If you need more verbose output:

1. In Xcode, edit **kernel/calls.c** or **debug.h**
2. Find: `#define STRACE_ENABLED`
3. Change to: `#define STRACE_ENABLED 1`
4. Rebuild (Cmd+B)

### Step 6.3: Check for Crashes

If iSH crashes:

1. Xcode shows **Thread 1: signal SIGABRT** or similar
2. Check **Console** for crash log
3. Look at **Call Stack** in Debug Navigator

---

## Part 7: Troubleshooting

### Issue 1: Build Fails - "No such file or directory"

**Symptom:**
```
fatal error: 'xxx.h' file not found
```

**Solution:**
```bash
# Clean build folder
rm -rf build
meson setup build
ninja -C build

# Or in Xcode:
# Product → Clean Build Folder (Cmd+Shift+K)
# Product → Build (Cmd+B)
```

### Issue 2: Code Signing Error

**Symptom:**
```
Signing for "iSH" requires a development team
```

**Solution:**

1. Xcode → Preferences → Accounts
2. Click **+** → Add Apple ID
3. Sign in with Apple ID (free account OK!)
4. In project settings, select your team

### Issue 3: Simulator Not Found

**Symptom:**
```
No simulators available
```

**Solution:**
```bash
# Open Xcode → Preferences → Components
# Download iOS 16.x Simulator (or latest)

# Or via command line:
xcodebuild -downloadPlatform iOS
```

### Issue 4: App Crashes on Launch

**Symptom:** iSH opens then immediately closes

**Check:**

1. Xcode Console for error messages
2. Simulator → Device → Trigger Screenshot (captures state)
3. Check if permissions are needed

**Common Fix:**
```bash
# Reset simulator
xcrun simctl erase all

# Rebuild and run
```

### Issue 5: "Exec format error" for 64-bit Binary

**Symptom:**
```bash
./test64
sh: ./test64: Exec format error
```

**This means:**
- ELF64 loader may not be working correctly
- Check Xcode console for detailed error
- Verify binary is actually 64-bit: `file test64`

**Debugging:**

1. Check if Phase 1 changes are present:
   ```bash
   grep "ELF_CLASS_64" kernel/elf.c
   ```

2. Enable debug logging in ELF loader

### Issue 6: "Illegal Instruction"

**Symptom:**
```
Illegal instruction (core dumped)
```

**This means:**
- 64-bit instruction not decoded correctly
- REX prefix parser issue
- Check Xcode console for opcode

**Debugging:**

1. Look for unimplemented instructions in log
2. Verify Phase 2 changes to `emu/decode.h`

### Issue 7: Build Warnings

**Symptom:**
```
warning: format specifies type 'int' but argument has type 'addr_t'
```

**Solution:** These are likely harmless, but can be fixed:

```c
// Change:
printf("%x", addr);

// To:
printf("%llx", (unsigned long long)addr);
```

---

## Part 8: Testing Checklist

### ✅ Basic Functionality

- [ ] iSH launches in simulator
- [ ] Terminal appears and is interactive
- [ ] Can run basic commands (ls, pwd, echo)
- [ ] Can install packages (apk add ...)

### ✅ 32-bit Compatibility (Regression Test)

- [ ] 32-bit binaries still work
- [ ] No crashes with existing programs
- [ ] Performance is similar to before

### ✅ 64-bit Support (New Features)

- [ ] Can load ELF64 binaries
- [ ] 64-bit programs execute
- [ ] SYSCALL instruction works
- [ ] 64-bit syscalls dispatch correctly
- [ ] Output is correct

---

## Part 9: Running on Real iPad (Optional)

### Step 9.1: Configure for Device

1. Connect iPad to Mac via USB
2. In Xcode, select iPad from device list (instead of simulator)
3. Ensure **Signing & Capabilities** is set up correctly

### Step 9.2: Trust Developer

1. On iPad: Settings → General → Device Management
2. Trust your developer account
3. Run app from Xcode (Cmd+R)

### Step 9.3: Test on Device

Same testing procedure as simulator, but:
- Performance will be better
- More realistic testing environment
- Easier file management via Files app

---

## Quick Reference

### Essential Commands

```bash
# Clone and setup
git clone <repo-url>
cd ish
git checkout claude/ish-64bit-port-011CUN2o2wGVYoigGBk1Yh98

# Build with meson
meson setup build
ninja -C build

# Open in Xcode
open iSH.xcodeproj

# Build in Xcode
Cmd+B

# Run in Simulator
Cmd+R

# Clean build
Cmd+Shift+K

# View console
Cmd+Shift+Y
```

### File Locations

```
iSH Source Code:      ~/Development/ish/
Xcode Project:        ~/Development/ish/iSH.xcodeproj
Build Output:         ~/Development/ish/build/
Simulator Container:  ~/Library/Developer/CoreSimulator/Devices/
```

---

## Summary

### You've Successfully:

1. ✅ Cloned the 64-bit iSH repository
2. ✅ Configured Xcode project
3. ✅ Built iSH with 64-bit support
4. ✅ Run iSH in iOS Simulator
5. ✅ Tested 64-bit binary execution (if test passed)

### What's Next:

- **Test more complex programs**: Try real 64-bit utilities
- **Performance testing**: Benchmark 32-bit vs 64-bit
- **Report issues**: Document any problems found
- **Contribute**: Help improve the 64-bit implementation

---

## Getting Help

### If Something Goes Wrong:

1. **Check Xcode Console** - Most errors show detailed info there
2. **Review build logs** - Look for specific error messages
3. **Consult documentation** - Read PHASE*.md files in repo
4. **Create minimal test case** - Simplify the problem
5. **Report the issue** - With full error details and logs

### Important Files for Debugging:

- `kernel/calls.c` - System call handler (Phase 4)
- `emu/decode.h` - Instruction decoder (Phase 2)
- `emu/cpu.h` - CPU state structure (Phase 1)
- `asbestos/gen.c` - Gadget generator (Phase 3)

---

**Date:** 2025-10-22
**Branch:** `claude/ish-64bit-port-011CUN2o2wGVYoigGBk1Yh98`
**Status:** Ready for testing! 🚀

Good luck, and happy testing! 🎉
