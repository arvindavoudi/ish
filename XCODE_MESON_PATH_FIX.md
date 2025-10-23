# Fix Xcode Meson PATH Issue

## The Problem
Xcode can't find `meson` because it doesn't use your terminal's PATH.

## Quick Fix (5 minutes)

### Step 1: Find Where Meson Is Installed

In Terminal:
```bash
which meson
# Will show something like:
# /opt/homebrew/bin/meson  (Apple Silicon)
# or
# /usr/local/bin/meson     (Intel)

# Also check ninja:
which ninja
# /opt/homebrew/bin/ninja
```

### Step 2: Create a Symbolic Link

```bash
# Create symlinks in a location Xcode can find
sudo mkdir -p /usr/local/bin
sudo ln -sf $(which meson) /usr/local/bin/meson
sudo ln -sf $(which ninja) /usr/local/bin/ninja

# Verify:
ls -la /usr/local/bin/meson
ls -la /usr/local/bin/ninja
```

### Step 3: Update Xcode Build Script

1. In Xcode, select **Meson** target in left sidebar
2. Go to **Build Phases** tab
3. Expand **Run Script** phase
4. Change the script to add PATH:

**Replace the script with:**
```bash
#!/bin/bash
set -e

# Add Homebrew paths
export PATH="/opt/homebrew/bin:/usr/local/bin:$PATH"

# Rest of the script (keep existing lines below)
cd "$PROJECT_DIR"
meson "$PROJECT_DIR" --cross-file cross.txt
```

### Step 4: Clean and Rebuild

In Xcode:
1. **Product → Clean Build Folder** (⌘⇧K)
2. **Product → Build** (⌘B)

---

## Alternative: Build Command Line First

If the above doesn't work, build with command line first:

```bash
cd ~/Development/ish

# Build with meson (already done!)
ninja -C build

# This creates all the necessary files
# Then Xcode can use them
```

Then in Xcode:
1. **File → Close Workspace**
2. **Re-open:** `open iSH.xcodeproj`
3. **Product → Build**

---

## Fix Deployment Target Warnings (Optional)

Those warnings won't stop the build, but to fix them:

1. In Xcode, select **iSH** project (top of left sidebar)
2. Select each target one by one
3. Go to **Build Settings** tab
4. Search for: **iOS Deployment Target**
5. Change from **iOS 11.0** to **iOS 12.0**

Do this for all targets with warnings.

---

## Expected Result

After fixing PATH:
```
✅ Meson configures successfully
✅ Build completes
✅ iSH launches in simulator
```

---

## Still Not Working?

If Xcode still can't find meson, try this nuclear option:

```bash
# Install meson in Xcode's expected location
brew link meson --force
brew link ninja --force

# Restart Xcode completely
killall Xcode
open iSH.xcodeproj
```

---

## Quick Verification

Before building in Xcode, verify meson works:

```bash
/usr/local/bin/meson --version
# Should show version number

# If command not found:
sudo ln -sf /opt/homebrew/bin/meson /usr/local/bin/meson
```

---

**Try the symbolic link fix first** - that usually solves it! 🚀
