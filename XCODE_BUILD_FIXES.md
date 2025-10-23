# Quick Fix Guide: Xcode Build Errors

## 🔴 Your Errors Explained

The errors you're seeing are related to **Meson build system** that iSH uses for building some components. Xcode is trying to run Meson but it's failing.

---

## 🔍 Step 1: See the Full Error Details

First, let's see what the actual error is:

### In Xcode:

1. Click the **error/warning icon** (triangle/circle) in the top-right of Xcode
2. OR press **⌘9** to open **Report Navigator**
3. Click the latest failed build
4. Expand the **Meson** or **libiSHApp** section
5. Look for red error text

**Screenshot the full error and we can debug it, OR continue with the fixes below:**

---

## ✅ Fix Option 1: Install Missing Dependencies (Most Likely Fix)

### The Issue:
Xcode can't find `meson` or `ninja` build tools.

### The Solution:

```bash
# Open Terminal (outside Xcode)

# 1. Install Homebrew if you don't have it
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 2. Install meson and ninja
brew install meson ninja pkg-config

# 3. Verify installation
which meson
# Should show: /opt/homebrew/bin/meson or /usr/local/bin/meson

which ninja
# Should show: /opt/homebrew/bin/ninja or /usr/local/bin/ninja

# 4. Make sure Xcode can find them
# Add to PATH for Xcode
echo 'export PATH="/opt/homebrew/bin:/usr/local/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc

# 5. Restart Xcode completely
# Quit Xcode (Cmd+Q)
# Reopen: open iSH.xcodeproj
```

### Then Try Building Again:

1. In Xcode: **Product → Clean Build Folder** (⌘⇧K)
2. **Product → Build** (⌘B)

---

## ✅ Fix Option 2: Run Meson Manually First

Sometimes Xcode's integration with Meson fails. Run it manually:

```bash
# In Terminal, navigate to iSH directory
cd ~/Development/ish

# Remove old build directory
rm -rf build

# Run meson setup
meson setup build --buildtype=debug

# Build with ninja
ninja -C build

# If this succeeds, try Xcode again
open iSH.xcodeproj
# Then: Product → Clean Build Folder → Build
```

---

## ✅ Fix Option 3: Check Meson Subprojects

iSH uses meson subprojects that need to be downloaded:

```bash
cd ~/Development/ish

# Download subprojects
meson subprojects download

# If that fails, manually check subprojects
ls subprojects/

# Should see files like:
# - libiSHApp.wrap
# - etc.

# Update subprojects
meson subprojects update

# Then try building again
meson setup build --wipe
ninja -C build
```

---

## ✅ Fix Option 4: Skip Meson (Build Subset Only)

If Meson keeps failing, we can try building just the main target:

### In Xcode:

1. Click **iSH** project in left sidebar
2. Select the **iSH** scheme (top-left)
3. **Edit Scheme...** (click and hold on scheme)
4. Go to **Build** tab
5. **Uncheck** the **Meson** target (if listed)
6. Keep only **iSH** target checked
7. Click **Close**
8. Try building again (⌘B)

---

## ✅ Fix Option 5: Check Xcode Command Line Tools

Meson needs Xcode Command Line Tools:

```bash
# Check if installed
xcode-select -p
# Should show: /Applications/Xcode.app/Contents/Developer

# If not found, install:
xcode-select --install

# If already installed, try resetting:
sudo xcode-select --reset
sudo xcodebuild -license accept

# Then restart Xcode
```

---

## ✅ Fix Option 6: Build Script Path Issues

The build script might not find tools. Let's fix the PATH:

### Create a build script helper:

```bash
# In Terminal:
cd ~/Development/ish

# Create a helper script
cat > fix_build_path.sh << 'EOF'
#!/bin/bash
export PATH="/opt/homebrew/bin:/usr/local/bin:$PATH"
export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig:/usr/local/lib/pkgconfig"
exec "$@"
EOF

chmod +x fix_build_path.sh
```

### Update Xcode Build Settings:

1. In Xcode, select **iSH** project
2. Select **Meson** target (in sidebar)
3. Go to **Build Phases** tab
4. Find **Run Script** phase
5. Change script to:
   ```bash
   cd "$PROJECT_DIR"
   ./fix_build_path.sh meson setup build
   ```

---

## 🔧 Most Common Solution (Try This First!)

**90% of the time, this fixes it:**

```bash
# 1. Install tools
brew install meson ninja

# 2. Clean everything
cd ~/Development/ish
rm -rf build
git clean -fdx  # ⚠️ Removes ALL untracked files!

# 3. Run meson manually
meson setup build

# 4. Build to verify
ninja -C build

# 5. Open in Xcode
open iSH.xcodeproj

# 6. In Xcode:
# - Product → Clean Build Folder (⌘⇧K)
# - Product → Build (⌘B)
```

---

## 🐛 If Still Failing: Get Detailed Error

Run this to see the exact error:

```bash
cd ~/Development/ish

# Try manual build with verbose output
meson setup build --wipe -Dwerror=false 2>&1 | tee meson_error.log

# Check the log
cat meson_error.log
```

**Send me the output** and I can help with the specific error!

---

## 🎯 Checklist - Do These in Order

- [ ] Install Homebrew
- [ ] Install meson and ninja (`brew install meson ninja`)
- [ ] Verify installation (`which meson ninja`)
- [ ] Delete build folder (`rm -rf build`)
- [ ] Run meson manually (`meson setup build`)
- [ ] Build with ninja (`ninja -C build`)
- [ ] Clean Xcode (⌘⇧K)
- [ ] Build in Xcode (⌘B)

---

## 💡 Alternative: Use Pre-built iSH

If Meson continues to be problematic, you can:

1. Build the dependencies manually outside Xcode
2. Use a pre-built iSH as base
3. Or focus on testing with command-line builds instead of Xcode

**For testing 64-bit specifically:**

```bash
# Build command-line version only
cd ~/Development/ish
meson setup build -Dbuildtype=debug
ninja -C build

# Run directly (no Xcode needed)
./build/iSH
```

---

## 📞 Need More Help?

**Share these details:**

1. **Homebrew installed?**
   ```bash
   brew --version
   ```

2. **Meson installed?**
   ```bash
   meson --version
   ```

3. **Full error from Xcode:**
   - Open Report Navigator (⌘9)
   - Expand the Meson error
   - Copy the complete error text

4. **macOS version:**
   ```bash
   sw_vers
   ```

---

## 🎯 Expected Working State

After fixes, you should see:

```bash
# Successful meson setup
$ meson setup build
The Meson build system
Version: 1.x.x
Build targets in project: 15

# Successful ninja build
$ ninja -C build
[234/234] Linking target iSH

# Successful Xcode build
Build Succeeded
```

---

**Start with Fix Option 1** (install dependencies) - that fixes it 90% of the time!

Let me know what happens! 🚀
