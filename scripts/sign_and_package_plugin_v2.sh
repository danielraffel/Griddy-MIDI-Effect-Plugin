#!/bin/bash
set -e

# Enable better error reporting
set -o pipefail
trap 'echo "❌ Script failed at line $LINENO with exit code $?"' ERR

# === Load environment ===
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
ENV_FILE="${ROOT_DIR}/.env"

if [ ! -f "$ENV_FILE" ]; then
  echo "❌ Missing .env file. Please create one based on .env.example"
  exit 1
fi

set -a
source "$ENV_FILE"
set +a

REQUIRED_VARS=(PROJECT_NAME APPLE_ID APP_SPECIFIC_PASSWORD APP_CERT INSTALLER_CERT)
for var in "${REQUIRED_VARS[@]}"; do
  if [ -z "${!var}" ]; then
    echo "❌ $var is not set in .env"
    exit 1
  fi
done

# === Load version information ===
# Auto-bump version based on recent commits (unless explicitly disabled)
if [ "$SKIP_VERSION_BUMP" != "1" ]; then
  echo "📦 Auto-bumping version based on recent commits..."
  python3 "${ROOT_DIR}/scripts/bump_version.py" --auto
else
  echo "📦 Skipping version bump (SKIP_VERSION_BUMP=1)"
fi

# Load the (potentially updated) version information
eval $(python3 "${ROOT_DIR}/scripts/bump_version.py" --export-only)

if [ -z "$PROJECT_VERSION" ]; then
  echo "❌ Could not load version information"
  echo "   Please ensure .build_version.json exists in project root"
  exit 1
fi

# Use semantic version for all packaging
PLUGIN_VERSION="${PROJECT_VERSION}"

echo "✅ Version: $PLUGIN_VERSION"

# === Parse command line arguments ===
PUBLISH_TO_RELEASES=false
CLEAN_AFTER_PUBLISH=false
RELEASE_TYPE="dev"  # dev, beta, or release
SKIP_BUILD=false

while [[ $# -gt 0 ]]; do
  case $1 in
    --publish)
      PUBLISH_TO_RELEASES=true
      shift
      ;;
    --clean)
      CLEAN_AFTER_PUBLISH=true
      shift
      ;;
    --release)
      RELEASE_TYPE="release"
      shift
      ;;
    --beta)
      RELEASE_TYPE="beta"
      shift
      ;;
    --skip-build)
      SKIP_BUILD=true
      shift
      ;;
    *)
      echo "Unknown option: $1"
      echo "Usage: $0 [--publish] [--clean] [--release|--beta] [--skip-build]"
      exit 1
      ;;
  esac
done

# === Fix Sparkle Authentication ===
# Replace the PAT placeholder in SparkleUpdater.mm with actual token
if [ -n "$GITHUB_UPDATE_PAT" ]; then
  echo "🔐 Injecting GitHub PAT for Sparkle updater authentication..."
  SPARKLE_FILE="${ROOT_DIR}/Source/SparkleUpdater.mm"
  SPARKLE_BACKUP="${ROOT_DIR}/Source/SparkleUpdater.mm.backup"
  
  # Create backup
  cp "$SPARKLE_FILE" "$SPARKLE_BACKUP"
  
  # Replace placeholder with actual token
  sed -i '' "s/@GITHUB_UPDATE_PAT@/${GITHUB_UPDATE_PAT}/g" "$SPARKLE_FILE"
  echo "✅ GitHub PAT injected for private repo access"
else
  echo "⚠️  GITHUB_UPDATE_PAT not found in .env - Sparkle updates may fail for private repo"
fi

# === Build Release Version ===
if [ "$SKIP_BUILD" = false ]; then
  echo "🚀 Building Release version of plugins..."
  echo "⚙️  This ensures we package optimized, log-free binaries"

  # Clean previous build artifacts to ensure a fresh release build
  rm -rf "${ROOT_DIR}/build"

  # Generate and build in Release mode (skip version bump since we already did it)
  cd "${ROOT_DIR}"
  SKIP_VERSION_BUMP=1 ./generate_and_open_xcode.sh release

  # Build all targets in Release configuration (excluding tests)
  echo "🏗️  Building all plugin formats in Release mode..."
  cd "${ROOT_DIR}/build"
  # Build specific targets to avoid test failures during packaging
  # Note: Only AU and Standalone are configured in CMakeLists.txt
  xcodebuild -project ${PROJECT_NAME}.xcodeproj \
    -target ${PROJECT_NAME}_AU \
    -target ${PROJECT_NAME}_Standalone \
    -configuration Release \
    CODE_SIGN_IDENTITY="" \
    CODE_SIGNING_REQUIRED=NO \
    build

  # Return to root directory
  cd "${ROOT_DIR}"
  
  # Restore SparkleUpdater.mm from backup to avoid committing token
  if [ -f "${ROOT_DIR}/Source/SparkleUpdater.mm.backup" ]; then
    echo "🔒 Restoring SparkleUpdater.mm to remove injected token..."
    mv "${ROOT_DIR}/Source/SparkleUpdater.mm.backup" "${ROOT_DIR}/Source/SparkleUpdater.mm"
  fi
fi

# Define Desktop first
DESKTOP="$HOME/Desktop"

# Now use Release artifacts instead of Debug
TEMP_SIGNING_DIR="${DESKTOP}/temp_signing_$$"
mkdir -p "$TEMP_SIGNING_DIR"
COMPONENT_PATH="$TEMP_SIGNING_DIR/${PROJECT_NAME}.component"
VST3_BUILD_PATH="${ROOT_DIR}/build/${PROJECT_NAME}_artefacts/Release/VST3/${PROJECT_NAME}.vst3"
VST3_PATH="$TEMP_SIGNING_DIR/${PROJECT_NAME}.vst3"
STANDALONE_BUILD_PATH="${ROOT_DIR}/build/${PROJECT_NAME}_artefacts/Release/Standalone/${PROJECT_NAME}.app"

# === Use versioned filenames ===
ZIP_PATH="${DESKTOP}/${PROJECT_NAME}.${PLUGIN_VERSION}.zip"
COMPONENT_PKG_PATH="${DESKTOP}/${PROJECT_NAME}_component.${PLUGIN_VERSION}.pkg"
VST3_PKG_PATH="${DESKTOP}/${PROJECT_NAME}_vst3.${PLUGIN_VERSION}.pkg"
STANDALONE_PKG_PATH="${DESKTOP}/${PROJECT_NAME}_standalone.${PLUGIN_VERSION}.pkg"
PKG_PATH="${DESKTOP}/${PROJECT_NAME}.${PLUGIN_VERSION}.pkg"
DMG_PATH="${DESKTOP}/${PROJECT_NAME}.${PLUGIN_VERSION}.dmg"
STAGING_DMG_DIR="${DESKTOP}/${PROJECT_NAME}_Installer"
SCRIPTS_DIR="${DESKTOP}/install_scripts"
RESOURCES_DIR="${DESKTOP}/pkg_resources"
TEMP_ROOT="${DESKTOP}/temp_component_root"
DISTRIBUTION_FILE="${DESKTOP}/Distribution.xml"
BINARIES_BUNDLE_DIR="${ROOT_DIR}/installer_binaries"

# Copy Release AU to temp directory for signing
echo "📋 Copying Release AU to temp directory for signing..."
AU_BUILD_PATH="${ROOT_DIR}/build/${PROJECT_NAME}_artefacts/Release/AU/${PROJECT_NAME}.component"
if [ -d "$AU_BUILD_PATH" ]; then
  rm -rf "$COMPONENT_PATH"
  # Use ditto to preserve bundle structure properly
  ditto "$AU_BUILD_PATH" "$COMPONENT_PATH"
  echo "✅ Release AU copied to temp directory"
else
  echo "❌ Release AU not found at $AU_BUILD_PATH"
  exit 1
fi

echo "🔏 Signing plugins..."

# Sign AU component
echo "  🎵 Signing Audio Unit..."
codesign --timestamp --options runtime --force --deep \
  --sign "$APP_CERT" "$COMPONENT_PATH"

# Copy and sign VST3 if it exists
if [ -d "$VST3_BUILD_PATH" ]; then
  echo "  🎹 Copying and signing VST3..."
  # Clean up any existing VST3 to avoid double-nesting
  rm -rf "$VST3_PATH"
  
  # Copy VST3 from build directory to temp directory for signing
  ditto "$VST3_BUILD_PATH" "$VST3_PATH"
  
  # Clean any potential extended attributes or hidden files that could cause signing issues
  xattr -cr "$VST3_PATH" 2>/dev/null || true
  find "$VST3_PATH" -name ".DS_Store" -delete 2>/dev/null || true
  
  codesign --timestamp --options runtime --force --deep \
    --sign "$APP_CERT" "$VST3_PATH"
  echo "  ✅ VST3 signed and ready"
else
  echo "  ⚠️  VST3 not found at $VST3_BUILD_PATH - will skip VST3 installation"
fi

# Copy and sign Standalone app if it exists
if [ -d "$STANDALONE_BUILD_PATH" ]; then
  echo "  🖥️  Copying and signing Standalone app..."
  # Copy standalone app to temp signing directory - PRESERVE SYMLINKS
  STANDALONE_DESKTOP_PATH="${TEMP_SIGNING_DIR}/${PROJECT_NAME}.app"
  rm -rf "$STANDALONE_DESKTOP_PATH"
  # Use ditto to preserve bundle structure and symlinks properly
  ditto "$STANDALONE_BUILD_PATH" "$STANDALONE_DESKTOP_PATH"
  
  # Clean any potential extended attributes or hidden files
  xattr -cr "$STANDALONE_DESKTOP_PATH" 2>/dev/null || true
  find "$STANDALONE_DESKTOP_PATH" -name ".DS_Store" -delete 2>/dev/null || true
  
  # Fix Sparkle.framework structure if needed
  SPARKLE_PATH="$STANDALONE_DESKTOP_PATH/Contents/Frameworks/Sparkle.framework"
  if [ -d "$SPARKLE_PATH" ]; then
    echo "  🔧 Fixing Sparkle.framework structure..."
    
    # Check if symlinks are broken (Current is a directory instead of symlink)
    if [ -d "$SPARKLE_PATH/Versions/Current" ] && [ ! -L "$SPARKLE_PATH/Versions/Current" ]; then
      echo "     Recreating proper symlink structure..."
      
      # Fix the Current symlink
      rm -rf "$SPARKLE_PATH/Versions/Current"
      cd "$SPARKLE_PATH/Versions"
      ln -s B Current
      cd - > /dev/null
      
      # Remove any files/folders that should be symlinks in root
      cd "$SPARKLE_PATH"
      for item in Headers Modules PrivateHeaders Resources Sparkle Updater.app XPCServices; do
        if [ -e "$item" ] && [ ! -L "$item" ]; then
          rm -rf "$item"
        fi
      done
      
      # Remove any extra files in root (like Autoupdate)
      for file in *; do
        if [ -f "$file" ] && [ ! -L "$file" ]; then
          echo "     Removing extra file: $file"
          rm -f "$file"
        fi
      done
      
      # Create proper symlinks
      ln -s Versions/Current/Headers Headers
      ln -s Versions/Current/Modules Modules  
      ln -s Versions/Current/PrivateHeaders PrivateHeaders
      ln -s Versions/Current/Resources Resources
      ln -s Versions/Current/Sparkle Sparkle
      ln -s Versions/Current/Updater.app Updater.app
      ln -s Versions/Current/XPCServices XPCServices
      
      cd - > /dev/null
      echo "     ✅ Fixed Sparkle.framework structure"
    fi
  fi
  
  # Sign the standalone app
  echo "  🔏 Signing standalone app..."
  
  # Use --deep to sign everything in one go
  echo "     Signing app bundle with embedded frameworks..."
  if codesign --force --deep --timestamp --options runtime \
    --sign "$APP_CERT" \
    "$STANDALONE_DESKTOP_PATH" 2>&1; then
    echo "  ✅ Standalone app signed successfully"
  else
    echo "  ❌ Signing failed - trying to continue anyway..."
  fi
  
  # Embed and fix library dependencies
  echo "  📚 Embedding library dependencies..."
  STANDALONE_BINARY="$STANDALONE_DESKTOP_PATH/Contents/MacOS/$PROJECT_NAME"
  FRAMEWORKS_PATH="$STANDALONE_DESKTOP_PATH/Contents/Frameworks"
  
  # Create Frameworks directory if it doesn't exist
  if [ ! -d "$FRAMEWORKS_PATH" ]; then
    echo "     Creating Frameworks directory..."
    mkdir -p "$FRAMEWORKS_PATH"
  fi
  
  # List of libraries to embed
  RUBBERBAND_LIB="/opt/homebrew/opt/rubberband/lib/librubberband.3.dylib"
  SAMPLERATE_LIB="/opt/homebrew/opt/libsamplerate/lib/libsamplerate.0.dylib"
  
  # Embed rubberband
  if [ -f "$RUBBERBAND_LIB" ]; then
    echo "     Copying rubberband library..."
    cp "$RUBBERBAND_LIB" "$FRAMEWORKS_PATH/"
    
    # Update the library path in the binary
    echo "     Updating rubberband path in binary..."
    install_name_tool -change "$RUBBERBAND_LIB" "@executable_path/../Frameworks/librubberband.3.dylib" "$STANDALONE_BINARY"
  else
    echo "  ⚠️  Rubberband library not found at $RUBBERBAND_LIB"
  fi
  
  # Embed libsamplerate
  if [ -f "$SAMPLERATE_LIB" ]; then
    echo "     Copying libsamplerate library..."
    cp "$SAMPLERATE_LIB" "$FRAMEWORKS_PATH/"
    
    # Update the library path in the binary
    echo "     Updating libsamplerate path in binary..."
    install_name_tool -change "$SAMPLERATE_LIB" "@executable_path/../Frameworks/libsamplerate.0.dylib" "$STANDALONE_BINARY"
    
    # Also fix the dependency in rubberband
    echo "     Updating libsamplerate path in rubberband..."
    install_name_tool -change "$SAMPLERATE_LIB" "@executable_path/../Frameworks/libsamplerate.0.dylib" "$FRAMEWORKS_PATH/librubberband.3.dylib"
    
    # Fix rubberband's own install name
    echo "     Fixing rubberband install name..."
    install_name_tool -id "@executable_path/../Frameworks/librubberband.3.dylib" "$FRAMEWORKS_PATH/librubberband.3.dylib"
  else
    echo "  ⚠️  Libsamplerate library not found at $SAMPLERATE_LIB"
  fi
  
  # Sign all embedded libraries
  echo "     Signing embedded libraries..."
  for lib in "$FRAMEWORKS_PATH"/lib*.dylib; do
    if [ -f "$lib" ]; then
      codesign --force --timestamp --sign "$APP_CERT" "$lib"
    fi
  done
  
  # Fix rpath to use relative paths instead of absolute build paths
  echo "  🔧 Fixing rpath in standalone app..."
  
  # Get current rpaths (may be empty)
  CURRENT_RPATHS=$(otool -l "$STANDALONE_BINARY" 2>/dev/null | grep -A 2 LC_RPATH | grep path | awk '{print $2}' || true)
  
  # Only process if there are rpaths to remove
  if [ -n "$CURRENT_RPATHS" ]; then
    # Remove absolute build paths
    for rpath in $CURRENT_RPATHS; do
      if [[ "$rpath" == /Users/* ]] || [[ "$rpath" == /private/* ]]; then
        echo "     Removing absolute rpath: $rpath"
        install_name_tool -delete_rpath "$rpath" "$STANDALONE_BINARY" 2>/dev/null || true
      fi
    done
  else
    echo "     No existing rpaths found"
  fi
  
  # Add proper relative rpaths
  echo "     Adding relative rpath: @executable_path/../Frameworks"
  install_name_tool -add_rpath "@executable_path/../Frameworks" "$STANDALONE_BINARY" 2>/dev/null || true
  
  # Re-sign after all modifications (rpath and library embedding)
  echo "     Re-signing entire app bundle after modifications..."
  codesign --force --deep --timestamp --options runtime --sign "$APP_CERT" "$STANDALONE_DESKTOP_PATH"
  
  # Verify the signature
  echo "  🔍 Verifying signature..."
  if codesign --verify --deep --verbose=2 "$STANDALONE_DESKTOP_PATH" 2>&1; then
    echo "  ✅ Signature verified successfully"
  else
    echo "  ⚠️  Signature verification had warnings, but continuing..."
  fi
else
  echo "  ⚠️  Standalone app not found at $STANDALONE_BUILD_PATH - will skip standalone installation"
fi

echo "📦 Creating ZIP for notarization..."
# Work from Desktop to avoid confusion
cd "$DESKTOP"

# Create ZIP with all available components
TEMP_ZIP_DIR="${DESKTOP}/temp_plugins_$$"
mkdir -p "$TEMP_ZIP_DIR"

# Always include AU - copy from its installed location
cp -R "$COMPONENT_PATH" "$TEMP_ZIP_DIR/${PROJECT_NAME}.component"

# Include VST3 if available
if [ -d "$VST3_PATH" ]; then
  cp -R "$VST3_PATH" "$TEMP_ZIP_DIR/${PROJECT_NAME}.vst3"
fi

# Include Standalone if available
if [ -d "$STANDALONE_DESKTOP_PATH" ]; then
  cp -a "$STANDALONE_DESKTOP_PATH" "$TEMP_ZIP_DIR/${PROJECT_NAME}.app"
fi

ditto -c -k --sequesterRsrc "$TEMP_ZIP_DIR" "$ZIP_PATH"
rm -rf "$TEMP_ZIP_DIR"

echo "☁️ Notarizing ZIP..."
# Retry notarization up to 3 times on timeout
NOTARY_ATTEMPTS=0
NOTARY_SUCCESS=false

while [ $NOTARY_ATTEMPTS -lt 3 ] && [ "$NOTARY_SUCCESS" = false ]; do
  NOTARY_ATTEMPTS=$((NOTARY_ATTEMPTS + 1))
  echo "  Attempt $NOTARY_ATTEMPTS of 3..."
  
  if xcrun notarytool submit "$ZIP_PATH" \
    --apple-id "$APPLE_ID" \
    --team-id "$TEAM_ID" \
    --password "$APP_SPECIFIC_PASSWORD" \
    --wait \
    --timeout 30m; then
    NOTARY_SUCCESS=true
    echo "  ✅ Notarization succeeded!"
  else
    NOTARY_EXIT=$?
    if [ $NOTARY_ATTEMPTS -lt 3 ]; then
      echo "  ⚠️ Notarization failed (exit code: $NOTARY_EXIT), retrying in 30 seconds..."
      sleep 30
    else
      echo "  ❌ Notarization failed after 3 attempts"
      exit 1
    fi
  fi
done

echo "📎 Stapling plugins..."
# Ensure we're on Desktop and create a temp directory for unzipping
cd "$DESKTOP"
TEMP_STAPLE_DIR="${DESKTOP}/temp_staple_$$"
mkdir -p "$TEMP_STAPLE_DIR"
cd "$TEMP_STAPLE_DIR"
unzip -o "$ZIP_PATH"
xcrun stapler staple "${PROJECT_NAME}.component"

STAPLED_ITEMS="AU"
if [ -d "${PROJECT_NAME}.vst3" ]; then
  xcrun stapler staple "${PROJECT_NAME}.vst3"
  STAPLED_ITEMS="$STAPLED_ITEMS + VST3"
fi

if [ -d "${PROJECT_NAME}.app" ]; then
  xcrun stapler staple "${PROJECT_NAME}.app"
  STAPLED_ITEMS="$STAPLED_ITEMS + Standalone"
fi

echo "✅ Stapled: $STAPLED_ITEMS"

# Copy stapled items back to temp signing directory (remove existing first to avoid permission issues)
rm -rf "$TEMP_SIGNING_DIR/${PROJECT_NAME}.component"
ditto "${PROJECT_NAME}.component" "$TEMP_SIGNING_DIR/${PROJECT_NAME}.component"
if [ -d "${PROJECT_NAME}.vst3" ]; then
  rm -rf "$TEMP_SIGNING_DIR/${PROJECT_NAME}.vst3"
  ditto "${PROJECT_NAME}.vst3" "$TEMP_SIGNING_DIR/${PROJECT_NAME}.vst3"
fi
if [ -d "${PROJECT_NAME}.app" ]; then
  rm -rf "$TEMP_SIGNING_DIR/${PROJECT_NAME}.app"
  ditto "${PROJECT_NAME}.app" "$TEMP_SIGNING_DIR/${PROJECT_NAME}.app"
fi

# Clean up temp directory
cd "$DESKTOP"
rm -rf "$TEMP_STAPLE_DIR"

# === BINARY BUNDLING INTEGRATION (Optional for projects that need it) ===
echo ""
echo "🔍 Checking for optional binary dependencies..."

# Check if we have pre-built binaries ready for packaging (optional)
HAS_BINARIES=false
if [ -d "$BINARIES_BUNDLE_DIR" ]; then
  echo "📦 Found pre-built binaries at: $BINARIES_BUNDLE_DIR"
  BINARY_COUNT=$(find "$BINARIES_BUNDLE_DIR" -type f | wc -l | tr -d ' ')
  echo "   📊 Ready to package: $BINARY_COUNT binary files"
  echo "   🐍 Including Python/FFmpeg/yt-dlp for self-contained environment"
  HAS_BINARIES=true
else
  echo "ℹ️  No installer binaries found - proceeding without bundled dependencies"
  echo "   This is normal for plugins that don't need Python/FFmpeg"
fi

# Create temporary roots for plugins
echo "📁 Preparing plugins for packaging..."
rm -rf "$TEMP_ROOT"
mkdir -p "$TEMP_ROOT"

# Ensure we're in the right directory and plugins exist
cd "$TEMP_SIGNING_DIR"
if [ ! -d "${PROJECT_NAME}.component" ]; then
  echo "❌ Error: ${PROJECT_NAME}.component not found in $TEMP_SIGNING_DIR"
  echo "   Expected: $TEMP_SIGNING_DIR/${PROJECT_NAME}.component"
  echo "   Available files:"
  ls -la "$TEMP_SIGNING_DIR" | grep -E "\.(component|vst3|app|pkg)$" || echo "   No components found"
  exit 1
fi

# Create separate staging areas for AU, VST3, and Standalone
AU_STAGING_ROOT="$TEMP_ROOT/au_staging"
VST3_STAGING_ROOT="$TEMP_ROOT/vst3_staging"
STANDALONE_STAGING_ROOT="$TEMP_ROOT/standalone_staging"
mkdir -p "$AU_STAGING_ROOT"

# Stage AU component
echo "📦 Staging Audio Unit..."
ditto "${PROJECT_NAME}.component" "$AU_STAGING_ROOT/${PROJECT_NAME}.component"

# Stage VST3 if it exists
HAS_VST3=false
if [ -d "${PROJECT_NAME}.vst3" ]; then
  echo "📦 Staging VST3..."
  mkdir -p "$VST3_STAGING_ROOT"
  ditto "${PROJECT_NAME}.vst3" "$VST3_STAGING_ROOT/${PROJECT_NAME}.vst3"
  HAS_VST3=true
else
  echo "ℹ️  VST3 not available"
fi

# Stage Standalone if it exists
HAS_STANDALONE=false
if [ -d "${PROJECT_NAME}.app" ]; then
  echo "📦 Staging Standalone app..."
  mkdir -p "$STANDALONE_STAGING_ROOT"
  ditto "${PROJECT_NAME}.app" "$STANDALONE_STAGING_ROOT/${PROJECT_NAME}.app"
  HAS_STANDALONE=true
else
  echo "ℹ️  Standalone app not available"
fi

# Summary of staged items
STAGED_ITEMS="AU"
[ "$HAS_VST3" = true ] && STAGED_ITEMS="$STAGED_ITEMS + VST3"
[ "$HAS_STANDALONE" = true ] && STAGED_ITEMS="$STAGED_ITEMS + Standalone"
echo "✅ Staged: $STAGED_ITEMS"

# Prepare install scripts if they exist
mkdir -p "$SCRIPTS_DIR"
SCRIPTS_ARGS=""

if [ -f "${ROOT_DIR}/preinstall" ]; then
  echo "📋 Found preinstall script"
  cp "${ROOT_DIR}/preinstall" "$SCRIPTS_DIR/"
  chmod +x "${SCRIPTS_DIR}/preinstall"
  SCRIPTS_ARGS="$SCRIPTS_ARGS --scripts $SCRIPTS_DIR"
fi

if [ -f "${ROOT_DIR}/postinstall" ]; then
  echo "📋 Found postinstall script" 
  cp "${ROOT_DIR}/postinstall" "$SCRIPTS_DIR/"
  chmod +x "${SCRIPTS_DIR}/postinstall"
  SCRIPTS_ARGS="$SCRIPTS_ARGS --scripts $SCRIPTS_DIR"
fi

echo "📦 Building plugin packages..."

# Build AU package
echo "🎵 Building Audio Unit package..."
if [ -n "$SCRIPTS_ARGS" ]; then
  echo "🔧 Including install scripts for AU"
  pkgbuild \
    --root "$AU_STAGING_ROOT" \
    --install-location "/Library/Audio/Plug-Ins/Components" \
    --identifier "com.${PROJECT_NAME}.component" \
    --version "$PLUGIN_VERSION" \
    $SCRIPTS_ARGS \
    --sign "$INSTALLER_CERT" \
    "$COMPONENT_PKG_PATH"
else
  echo "📦 Creating basic AU package"
  pkgbuild \
    --root "$AU_STAGING_ROOT" \
    --install-location "/Library/Audio/Plug-Ins/Components" \
    --identifier "com.${PROJECT_NAME}.component" \
    --version "$PLUGIN_VERSION" \
    --sign "$INSTALLER_CERT" \
    "$COMPONENT_PKG_PATH"
fi

# Verify AU package was created
if [ ! -f "$COMPONENT_PKG_PATH" ]; then
  echo "❌ Failed to create Audio Unit package"
  exit 1
fi
echo "✅ Audio Unit package created"

# Build VST3 package if VST3 is available
if [ "$HAS_VST3" = true ]; then
  echo "🎹 Building VST3 package..."
  pkgbuild \
    --root "$VST3_STAGING_ROOT" \
    --install-location "/Library/Audio/Plug-Ins/VST3" \
    --identifier "com.${PROJECT_NAME}.vst3" \
    --version "$PLUGIN_VERSION" \
    --sign "$INSTALLER_CERT" \
    "$VST3_PKG_PATH"
  
  if [ ! -f "$VST3_PKG_PATH" ]; then
    echo "❌ Failed to create VST3 package"
    exit 1
  fi
  echo "✅ VST3 package created"
fi

# Build Standalone package if Standalone is available
if [ "$HAS_STANDALONE" = true ]; then
  echo "🖥️  Building Standalone package..."
  pkgbuild \
    --root "$STANDALONE_STAGING_ROOT" \
    --install-location "/Applications" \
    --identifier "com.${PROJECT_NAME}.standalone" \
    --version "$PLUGIN_VERSION" \
    --sign "$INSTALLER_CERT" \
    "$STANDALONE_PKG_PATH"
  
  if [ ! -f "$STANDALONE_PKG_PATH" ]; then
    echo "❌ Failed to create Standalone package"
    exit 1
  fi
  echo "✅ Standalone package created"
fi

# === INCLUDE BUNDLED BINARIES IN RESOURCES ===
mkdir -p "$RESOURCES_DIR"

# Include bundled binaries if they exist
if [ -d "$BINARIES_BUNDLE_DIR" ]; then
  echo "📦 Including bundled binaries in installer resources..."
  
  # Use rsync for better permission handling and to avoid copy failures
  if command -v rsync >/dev/null 2>&1; then
    echo "   Using rsync for reliable copying..."
    rsync -a --chmod=u+rw "$BINARIES_BUNDLE_DIR/" "$RESOURCES_DIR/installer_binaries/"
  else
    # Fallback to cp with permission fixes
    echo "   Using cp with permission fixes..."
  cp -R "$BINARIES_BUNDLE_DIR" "$RESOURCES_DIR/"
    
    # Fix any permission issues that might prevent copying
    echo "   Fixing file permissions..."
    find "$RESOURCES_DIR/installer_binaries" -type f -exec chmod u+rw {} \; 2>/dev/null || true
    find "$RESOURCES_DIR/installer_binaries" -type d -exec chmod u+rwx {} \; 2>/dev/null || true
  fi
  
  # Verify critical binaries were bundled
  CRITICAL_BINARIES=(
    "$RESOURCES_DIR/installer_binaries/frameworks/Python.framework/Versions/3.12/bin/python3.12"
    "$RESOURCES_DIR/installer_binaries/bin/ffmpeg"
    "$RESOURCES_DIR/installer_binaries/bin/yt-dlp"
  )
  
  MISSING_BINARIES=()
  for binary in "${CRITICAL_BINARIES[@]}"; do
    if [ ! -f "$binary" ]; then
      MISSING_BINARIES+=("$(basename "$binary")")
    fi
  done
  
  if [ ${#MISSING_BINARIES[@]} -gt 0 ]; then
    echo "⚠️  WARNING: Missing critical binaries in bundle:"
    printf '   - %s\n' "${MISSING_BINARIES[@]}"
    echo "   Installation may fail or require fallback methods"
  else
    echo "✅ All critical binaries verified in bundle"
  fi
  
  echo "✅ Bundled binaries included: $(find "$RESOURCES_DIR/installer_binaries" -type f 2>/dev/null | wc -l | tr -d ' ') files"
  
  # Debug: Show what's actually in the resources directory
  echo "  📁 Resources directory structure:"
  if [ -d "$RESOURCES_DIR/installer_binaries" ]; then
    echo "    installer_binaries/ ($(find "$RESOURCES_DIR/installer_binaries" -type f | wc -l | tr -d ' ') files)"
    if [ -d "$RESOURCES_DIR/installer_binaries/bin" ]; then
      echo "      bin/ ($(ls "$RESOURCES_DIR/installer_binaries/bin" | wc -l | tr -d ' ') files)"
    fi
    if [ -d "$RESOURCES_DIR/installer_binaries/frameworks" ]; then
      echo "      frameworks/ ($(find "$RESOURCES_DIR/installer_binaries/frameworks" -type f | wc -l | tr -d ' ') files)"
    fi
    if [ -d "$RESOURCES_DIR/installer_binaries/lib" ]; then
      echo "      lib/ ($(ls "$RESOURCES_DIR/installer_binaries/lib" | wc -l | tr -d ' ') files)"
    fi
  else
    echo "    ❌ installer_binaries directory not found!"
    echo "    Available in resources:"
    ls -la "$RESOURCES_DIR/" 2>/dev/null || echo "    Resources directory empty or missing"
  fi
fi

# Include setup_uv_environment.sh script for postinstall to use
if [ -f "${ROOT_DIR}/scripts/setup_uv_environment.sh" ]; then
    echo "📄 Including setup_uv_environment.sh script..."
    cp "${ROOT_DIR}/scripts/setup_uv_environment.sh" "$RESOURCES_DIR/"
    chmod +x "$RESOURCES_DIR/setup_uv_environment.sh"
    echo "✅ setup_uv_environment.sh included in installer resources"
else
    echo "⚠️  setup_uv_environment.sh not found - postinstall will use built-in venv creation"
fi

# Include project Resources directory (scripts, words.txt, etc.)
if [ -d "${ROOT_DIR}/Resources" ]; then
  echo "📄 Including project scripts and resources..."
  
  # Define essential scripts (exclude test/development scripts)
  ESSENTIAL_SCRIPTS=(
    "au_script_wrapper_uv.sh"
    "sonicgarbage.py"
    "essentia_analyzer.py"  # Now mandatory for slice mode onset detection
  )
  
  # Add CLAP manager if CLAP is enabled
  if [ "${ENABLE_CLAP_FEATURES}" = "ON" ] || [ "${ENABLE_CLAP_FEATURES}" = "1" ]; then
    ESSENTIAL_SCRIPTS+=("clap_manager.py")
  fi
  
  # Copy essential scripts only
  for script_name in "${ESSENTIAL_SCRIPTS[@]}"; do
    if [ -f "${ROOT_DIR}/Resources/$script_name" ]; then
      cp "${ROOT_DIR}/Resources/$script_name" "$RESOURCES_DIR/"
      echo "✅ Included $script_name"
    else
      echo "⚠️  Essential script not found: $script_name"
    fi
  done
  
  # Copy essential text files and documentation
  for resource_file in "${ROOT_DIR}/Resources"/*.txt "${ROOT_DIR}/Resources"/*.md; do
    if [ -f "$resource_file" ]; then
      cp "$resource_file" "$RESOURCES_DIR/"
      echo "✅ Included $(basename "$resource_file")"
    fi
  done
  
  echo "ℹ️  Excluded test scripts: *test*.sh, *sandbox*.sh, *v2*.sh"
else
  echo "ℹ️  No Resources directory found at ${ROOT_DIR}/Resources"
fi

# Prepare installer resources and screen files
HAS_LICENSE=false
LICENSE_FILE=""

# Check for license file for PKG installer (prioritize TERMS.txt)
if [ -f "${ROOT_DIR}/TERMS.txt" ]; then
  echo "📄 Found TERMS.txt, will create license screen"
  cp "${ROOT_DIR}/TERMS.txt" "$RESOURCES_DIR/"
  LICENSE_FILE="TERMS.txt"
  HAS_LICENSE=true
elif [ -f "${ROOT_DIR}/LICENSE.txt" ]; then
  echo "📄 Found LICENSE.txt, will create license screen"
  cp "${ROOT_DIR}/LICENSE.txt" "$RESOURCES_DIR/"
  LICENSE_FILE="LICENSE.txt"
  HAS_LICENSE=true
fi

# Note: LICENSES.md will be included automatically with other resource files for plugin settings

# Copy Welcome and ReadMe files for installer screens
if [ -f "${ROOT_DIR}/Resources/Welcome.txt" ]; then
  echo "📄 Including Welcome screen"
  cp "${ROOT_DIR}/Resources/Welcome.txt" "$RESOURCES_DIR/"
fi

if [ -f "${ROOT_DIR}/Resources/ReadMe.txt" ]; then
  echo "📄 Including ReadMe screen" 
  cp "${ROOT_DIR}/Resources/ReadMe.txt" "$RESOURCES_DIR/"
fi

echo "📦 Building final .pkg installer..."

# FIXED APPROACH: Component goes in payload, binaries go in PKG Resources (where postinstall expects them)
echo "🔧 Using clean staging approach (no signature corruption)..."

# Create clean staging roots for plugins (no binaries embedded)
CLEAN_AU_STAGING_ROOT="$TEMP_ROOT/clean_au_staging"
CLEAN_VST3_STAGING_ROOT="$TEMP_ROOT/clean_vst3_staging"
CLEAN_STANDALONE_STAGING_ROOT="$TEMP_ROOT/clean_standalone_staging"
rm -rf "$CLEAN_AU_STAGING_ROOT" "$CLEAN_VST3_STAGING_ROOT" "$CLEAN_STANDALONE_STAGING_ROOT"
mkdir -p "$CLEAN_AU_STAGING_ROOT"

echo "📦 Staging plugins only (no embedded binaries)..."
cp -R "$TEMP_SIGNING_DIR/${PROJECT_NAME}.component" "$CLEAN_AU_STAGING_ROOT/"

# Count AU files
AU_FILES=$(find "$CLEAN_AU_STAGING_ROOT" -type f | wc -l)
echo "📊 Audio Unit files staged: $AU_FILES"

# Stage VST3 if available
if [ "$HAS_VST3" = true ]; then
  mkdir -p "$CLEAN_VST3_STAGING_ROOT"
  cp -R "$TEMP_SIGNING_DIR/${PROJECT_NAME}.vst3" "$CLEAN_VST3_STAGING_ROOT/"
  VST3_FILES=$(find "$CLEAN_VST3_STAGING_ROOT" -type f | wc -l)
  echo "📊 VST3 files staged: $VST3_FILES"
fi

# Stage Standalone if available
if [ "$HAS_STANDALONE" = true ]; then
  mkdir -p "$CLEAN_STANDALONE_STAGING_ROOT"
  cp -a "$TEMP_SIGNING_DIR/${PROJECT_NAME}.app" "$CLEAN_STANDALONE_STAGING_ROOT/"
  STANDALONE_FILES=$(find "$CLEAN_STANDALONE_STAGING_ROOT" -type f | wc -l)
  echo "📊 Standalone app files staged: $STANDALONE_FILES"
fi

# Remove old packages (we'll rebuild with clean plugins)
rm -f "$COMPONENT_PKG_PATH" "$VST3_PKG_PATH" "$STANDALONE_PKG_PATH"

# Build clean AU package (NEVER EXTRACT THIS AGAIN)
echo "📦 Building clean Audio Unit package (no embedded binaries)..."
if [ -n "$SCRIPTS_ARGS" ]; then
  echo "🔧 Including install scripts for AU"
  pkgbuild \
    --root "$CLEAN_AU_STAGING_ROOT" \
    --install-location "/Library/Audio/Plug-Ins/Components" \
    --identifier "com.${PROJECT_NAME}.component" \
    --version "$PLUGIN_VERSION" \
    $SCRIPTS_ARGS \
    --sign "$INSTALLER_CERT" \
    "$COMPONENT_PKG_PATH"
else
  echo "📦 Creating basic AU package"
  pkgbuild \
    --root "$CLEAN_AU_STAGING_ROOT" \
    --install-location "/Library/Audio/Plug-Ins/Components" \
    --identifier "com.${PROJECT_NAME}.component" \
    --version "$PLUGIN_VERSION" \
    --sign "$INSTALLER_CERT" \
    "$COMPONENT_PKG_PATH"
fi

# Verify AU package was created
if [ ! -f "$COMPONENT_PKG_PATH" ]; then
    echo "❌ Failed to create clean Audio Unit package"
    exit 1
fi
echo "✅ Clean Audio Unit package created (no embedded binaries)"

# Build clean VST3 package if available
if [ "$HAS_VST3" = true ]; then
  echo "📦 Building clean VST3 package (no embedded binaries)..."
  pkgbuild \
    --root "$CLEAN_VST3_STAGING_ROOT" \
    --install-location "/Library/Audio/Plug-Ins/VST3" \
    --identifier "com.${PROJECT_NAME}.vst3" \
    --version "$PLUGIN_VERSION" \
    --sign "$INSTALLER_CERT" \
    "$VST3_PKG_PATH"
  
  if [ ! -f "$VST3_PKG_PATH" ]; then
    echo "❌ Failed to create clean VST3 package"
    exit 1
  fi
  echo "✅ Clean VST3 package created (no embedded binaries)"
fi

# Build clean Standalone package if available
if [ "$HAS_STANDALONE" = true ]; then
  echo "📦 Building clean Standalone package (no embedded binaries)..."
  
  # Create component property list to disable bundle relocation
  # This prevents the installer from finding and "upgrading" the app in the build directory
  STANDALONE_PLIST="${TEMP_ROOT}/standalone_component.plist"
  echo "🔧 Creating component property list to disable bundle relocation..."
  cat > "$STANDALONE_PLIST" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<array>
    <dict>
        <key>BundleHasStrictIdentifier</key>
        <true/>
        <key>BundleIsRelocatable</key>
        <false/>
        <key>BundleIsVersionChecked</key>
        <true/>
        <key>BundleOverwriteAction</key>
        <string>upgrade</string>
        <key>RootRelativeBundlePath</key>
        <string>PlunderTube.app</string>
    </dict>
</array>
</plist>
EOF
  echo "📝 BundleIsRelocatable set to false - installer will only install to /Applications"
  
  pkgbuild \
    --root "$CLEAN_STANDALONE_STAGING_ROOT" \
    --install-location "/Applications" \
    --identifier "com.${PROJECT_NAME}.standalone" \
    --version "$PLUGIN_VERSION" \
    --component-plist "$STANDALONE_PLIST" \
    --sign "$INSTALLER_CERT" \
    "$STANDALONE_PKG_PATH"
  
  if [ ! -f "$STANDALONE_PKG_PATH" ]; then
    echo "❌ Failed to create clean Standalone package"
    exit 1
  fi
  echo "✅ Clean Standalone package created (no embedded binaries)"
fi

# NOW: Prepare PKG Resources with binaries (where postinstall expects them)
echo "📦 Preparing PKG Resources with signed binaries..."

# CRITICAL: Clean and sign the binaries for PKG Resources
if [ -d "$RESOURCES_DIR/installer_binaries" ]; then
    echo "🧹 Removing object files from PKG Resources binaries..."
    find "$RESOURCES_DIR/installer_binaries" -name '*.o' -delete
    OBJ_COUNT=$(find "$RESOURCES_DIR/installer_binaries" -name '*.o' | wc -l)
    echo "✅ Object files remaining in PKG Resources: $OBJ_COUNT (should be 0)"
    
    # CRITICAL: Sign Python framework in PKG Resources (where it will be installed)
    echo "🔏 Signing Python framework in PKG Resources..."
    PYTHON_FRAMEWORK_PATH="$RESOURCES_DIR/installer_binaries/frameworks/Python.framework"
    if [ -d "$PYTHON_FRAMEWORK_PATH" ]; then
        codesign --deep --force --timestamp --options runtime \
                 -s "$APP_CERT" \
                 "$PYTHON_FRAMEWORK_PATH"
        
        echo "🔍 Verifying Python framework signature..."
        codesign --verify --deep --strict --verbose=2 "$PYTHON_FRAMEWORK_PATH"
        echo "✅ Python framework signature verified in PKG Resources"
        
        # Count PKG Resources files
        PKG_RESOURCES_FILES=$(find "$RESOURCES_DIR/installer_binaries" -type f | wc -l)
        echo "📊 PKG Resources binary files: $PKG_RESOURCES_FILES"
    else
        echo "❌ Python framework not found in PKG Resources"
        exit 1
    fi
else
    echo "❌ No installer binaries found for PKG Resources"
    exit 1
fi

# Create TWO packages: component + resources
echo "📦 Creating separate resources package for postinstall to find..."

# Create staging for Resources that will be installed to a temporary location
RESOURCES_STAGING_ROOT="$TEMP_ROOT/resources_staging"
rm -rf "$RESOURCES_STAGING_ROOT"
mkdir -p "$RESOURCES_STAGING_ROOT/tmp/PlunderTube_installer_resources"

# Stage the resources where postinstall expects them
echo "📦 Staging resources for /tmp/PlunderTube_installer_resources..."
if [ -d "$RESOURCES_DIR/installer_binaries" ]; then
    rsync -a "$RESOURCES_DIR/installer_binaries/" "$RESOURCES_STAGING_ROOT/tmp/PlunderTube_installer_resources/installer_binaries/"
    echo "✅ Staged $(find "$RESOURCES_STAGING_ROOT/tmp/PlunderTube_installer_resources/installer_binaries" -type f | wc -l) binary files for temp install"
fi

# Copy other resources too
# Use the same essential scripts list as defined above
STAGING_SCRIPTS=(
    "setup_uv_environment.sh"
    "au_script_wrapper_uv.sh" 
    "sonicgarbage.py"
    "essentia_analyzer.py"
    "words.txt"
    "LICENSE.txt"
)

# Add CLAP manager if it was included
if [ -f "$RESOURCES_DIR/clap_manager.py" ]; then
    STAGING_SCRIPTS+=("clap_manager.py")
fi

for item in "${STAGING_SCRIPTS[@]}"; do
    if [ -f "$RESOURCES_DIR/$item" ]; then
        cp "$RESOURCES_DIR/$item" "$RESOURCES_STAGING_ROOT/tmp/PlunderTube_installer_resources/"
        echo "✅ Staged $item for temp install"
    fi
done

# Create resources package
RESOURCES_PKG_PATH="${DESKTOP}/${PROJECT_NAME}_resources.${PLUGIN_VERSION}.pkg"
echo "📦 Building resources package..."
pkgbuild \
  --root "$RESOURCES_STAGING_ROOT" \
  --install-location "/" \
  --identifier "com.${PROJECT_NAME}.resources" \
  --version "$PLUGIN_VERSION" \
  --sign "$INSTALLER_CERT" \
  "$RESOURCES_PKG_PATH"

echo "✅ Resources package created: $RESOURCES_PKG_PATH"

# Create the final PKG with enhanced installer screens
echo "📦 Creating installer with plugins + resources + enhanced screens..."

# Always use distribution file for enhanced installer experience
cat > "$DISTRIBUTION_FILE" << EOF
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="1">
    <title>${PROJECT_NAME} ${PLUGIN_VERSION}</title>
    <welcome file="Welcome.txt"/>
    <readme file="ReadMe.txt"/>
EOF

# Add license if available
if [ "$HAS_LICENSE" = true ]; then
  cat >> "$DISTRIBUTION_FILE" << EOF
    <license file="${LICENSE_FILE}"/>
EOF
fi

# Add package references
cat >> "$DISTRIBUTION_FILE" << EOF
    <pkg-ref id="com.${PROJECT_NAME}.component"/>
    <pkg-ref id="com.${PROJECT_NAME}.resources"/>
EOF

# Add VST3 package reference if available
if [ "$HAS_VST3" = true ]; then
  cat >> "$DISTRIBUTION_FILE" << EOF
    <pkg-ref id="com.${PROJECT_NAME}.vst3"/>
EOF
fi

# Add Standalone package reference if available
if [ "$HAS_STANDALONE" = true ]; then
  cat >> "$DISTRIBUTION_FILE" << EOF
    <pkg-ref id="com.${PROJECT_NAME}.standalone"/>
EOF
fi

# Configure installer options and choices outline
cat >> "$DISTRIBUTION_FILE" << EOF
    <options customize="allow" require-scripts="false"/>
    <choices-outline>
        <line choice="plugins">
            <line choice="com.${PROJECT_NAME}.component"/>
EOF

# Add VST3 choice if available
if [ "$HAS_VST3" = true ]; then
  cat >> "$DISTRIBUTION_FILE" << EOF
            <line choice="com.${PROJECT_NAME}.vst3"/>
EOF
fi

cat >> "$DISTRIBUTION_FILE" << EOF
        </line>
EOF

# Add Standalone choice if available
if [ "$HAS_STANDALONE" = true ]; then
  cat >> "$DISTRIBUTION_FILE" << EOF
        <line choice="com.${PROJECT_NAME}.standalone"/>
EOF
fi

cat >> "$DISTRIBUTION_FILE" << EOF
        <line choice="com.${PROJECT_NAME}.resources"/>
    </choices-outline>
    
    <!-- Plugin choices group -->
    <choice id="plugins" title="Plugin Formats" description="Choose which plugin formats to install">
        <pkg-ref id="com.${PROJECT_NAME}.component"/>
EOF

# Add VST3 to plugins group if available
if [ "$HAS_VST3" = true ]; then
  cat >> "$DISTRIBUTION_FILE" << EOF
        <pkg-ref id="com.${PROJECT_NAME}.vst3"/>
EOF
fi

cat >> "$DISTRIBUTION_FILE" << EOF
    </choice>
    
    <!-- Audio Unit choice -->
    <choice id="com.${PROJECT_NAME}.component" 
            title="Audio Unit (AU)" 
            description="Install the Audio Unit plugin for use in Logic Pro, GarageBand, and other AU-compatible DAWs"
            selected="true"
            enabled="true">
        <pkg-ref id="com.${PROJECT_NAME}.component"/>
    </choice>
EOF

# Add VST3 choice definition if available
if [ "$HAS_VST3" = true ]; then
  cat >> "$DISTRIBUTION_FILE" << EOF
    
    <!-- VST3 choice -->
    <choice id="com.${PROJECT_NAME}.vst3" 
            title="VST3" 
            description="Install the VST3 plugin for use in most modern DAWs including Cubase, FL Studio, Reaper, and others"
            selected="true"
            enabled="true">
        <pkg-ref id="com.${PROJECT_NAME}.vst3"/>
    </choice>
EOF
fi

# Add Standalone choice definition if available
if [ "$HAS_STANDALONE" = true ]; then
  cat >> "$DISTRIBUTION_FILE" << EOF
    
    <!-- Standalone choice -->
    <choice id="com.${PROJECT_NAME}.standalone" 
            title="Standalone Application" 
            description="Install the standalone PlunderTube application that runs without a DAW"
            selected="true"
            enabled="true">
        <pkg-ref id="com.${PROJECT_NAME}.standalone"/>
    </choice>
EOF
fi

cat >> "$DISTRIBUTION_FILE" << EOF
    
    <!-- Hidden resources choice (always installed) -->
    <choice id="com.${PROJECT_NAME}.resources" visible="false" selected="true">
        <pkg-ref id="com.${PROJECT_NAME}.resources"/>
    </choice>
    
    <!-- Package references with file paths -->
    <pkg-ref id="com.${PROJECT_NAME}.component" version="${PLUGIN_VERSION}" onConclusion="none">$(basename "$COMPONENT_PKG_PATH")</pkg-ref>
EOF

# Add VST3 package reference if available
if [ "$HAS_VST3" = true ]; then
  cat >> "$DISTRIBUTION_FILE" << EOF
    <pkg-ref id="com.${PROJECT_NAME}.vst3" version="${PLUGIN_VERSION}" onConclusion="none">$(basename "$VST3_PKG_PATH")</pkg-ref>
EOF
fi

# Add Standalone package reference if available
if [ "$HAS_STANDALONE" = true ]; then
  cat >> "$DISTRIBUTION_FILE" << EOF
    <pkg-ref id="com.${PROJECT_NAME}.standalone" version="${PLUGIN_VERSION}" onConclusion="none">$(basename "$STANDALONE_PKG_PATH")</pkg-ref>
EOF
fi

cat >> "$DISTRIBUTION_FILE" << EOF
    <pkg-ref id="com.${PROJECT_NAME}.resources" version="${PLUGIN_VERSION}" onConclusion="none">$(basename "$RESOURCES_PKG_PATH")</pkg-ref>
    
    <!-- Ensure at least one plugin is selected -->
    <script>
        function choiceChanges() {
            var au_selected = choices['com.${PROJECT_NAME}.component'].selected;
EOF

if [ "$HAS_VST3" = true ] && [ "$HAS_STANDALONE" = true ]; then
  cat >> "$DISTRIBUTION_FILE" << EOF
            var vst3_selected = choices['com.${PROJECT_NAME}.vst3'].selected;
            var standalone_selected = choices['com.${PROJECT_NAME}.standalone'].selected;
            var any_selected = au_selected || vst3_selected || standalone_selected;
            
            // Ensure at least one item is selected
            if (!any_selected) {
                // Re-enable AU as the default
                choices['com.${PROJECT_NAME}.component'].selected = true;
            }
EOF
elif [ "$HAS_VST3" = true ]; then
  cat >> "$DISTRIBUTION_FILE" << EOF
            var vst3_selected = choices['com.${PROJECT_NAME}.vst3'].selected;
            var any_plugin_selected = au_selected || vst3_selected;
            
            // Ensure at least one plugin is selected
            if (!any_plugin_selected) {
                // Re-enable the last deselected choice
                if (!au_selected) {
                    choices['com.${PROJECT_NAME}.component'].selected = true;
                } else {
                    choices['com.${PROJECT_NAME}.vst3'].selected = true;
                }
            }
EOF
elif [ "$HAS_STANDALONE" = true ]; then
  cat >> "$DISTRIBUTION_FILE" << EOF
            var standalone_selected = choices['com.${PROJECT_NAME}.standalone'].selected;
            var any_selected = au_selected || standalone_selected;
            
            // Ensure at least one item is selected
            if (!any_selected) {
                choices['com.${PROJECT_NAME}.component'].selected = true;
            }
EOF
else
  cat >> "$DISTRIBUTION_FILE" << EOF
            // For AU-only installation, ensure AU is always selected
            if (!au_selected) {
                choices['com.${PROJECT_NAME}.component'].selected = true;
            }
EOF
fi

cat >> "$DISTRIBUTION_FILE" << EOF
        }
    </script>
</installer-gui-script>
EOF

echo "🔧 Running productbuild with enhanced installer..."
productbuild \
  --distribution "$DISTRIBUTION_FILE" \
  --resources "$RESOURCES_DIR" \
  --package-path "$(dirname "$COMPONENT_PKG_PATH")" \
  --sign "$INSTALLER_CERT" \
  "$PKG_PATH"

# Verify final package was created
if [ ! -f "$PKG_PATH" ]; then
  echo "❌ Failed to create final package"
  exit 1
fi

echo "☁️ Notarizing .pkg..."
xcrun notarytool submit "$PKG_PATH" \
  --apple-id "$APPLE_ID" \
  --team-id "$TEAM_ID" \
  --password "$APP_SPECIFIC_PASSWORD" \
  --wait

echo "📎 Stapling .pkg..."
xcrun stapler staple "$PKG_PATH"

echo "💽 Creating DMG with .pkg and uninstaller inside..."
mkdir -p "$STAGING_DMG_DIR"
cp "$PKG_PATH" "$STAGING_DMG_DIR/"

# Include uninstaller script in DMG
if [ -f "${ROOT_DIR}/Resources/uninstall_plundertube.sh" ]; then
    echo "📄 Including uninstaller script in DMG..."
    cp "${ROOT_DIR}/Resources/uninstall_plundertube.sh" "$STAGING_DMG_DIR/Uninstall PlunderTube.command"
    chmod +x "$STAGING_DMG_DIR/Uninstall PlunderTube.command"
    codesign --timestamp --options runtime --force \
      --sign "$APP_CERT" "$STAGING_DMG_DIR/Uninstall PlunderTube.command"

    echo "✅ Uninstaller included: 'Uninstall PlunderTube.command'"
else
    echo "⚠️  Uninstaller script not found at ${ROOT_DIR}/Resources/uninstall_plundertube.sh"
fi

# Calculate size needed (staging dir size + 20% buffer)
STAGING_SIZE=$(du -sm "$STAGING_DMG_DIR" | cut -f1)
DMG_SIZE=$((STAGING_SIZE + STAGING_SIZE/5 + 10))m

# Create final DMG with calculated size
# Change to parent directory to avoid path issues
cd "$(dirname "$STAGING_DMG_DIR")"
hdiutil create -volname "${PROJECT_NAME} Installer" \
  -srcfolder "$(basename "$STAGING_DMG_DIR")" \
  -ov -format UDZO -size "$DMG_SIZE" "$DMG_PATH"

echo "🗜️ Creating ZIP of DMG for distribution..."
ZIP_DMG_PATH="${DESKTOP}/${PROJECT_NAME}.${PLUGIN_VERSION}.dmg.zip"
cd "$DESKTOP"
zip -9 "$(basename "$ZIP_DMG_PATH")" "$(basename "$DMG_PATH")"

# === PUBLISH TO RELEASES REPO ===
if [ "$PUBLISH_TO_RELEASES" = true ]; then
  echo ""
  echo "📤 Publishing to pt-releases repository..."
  
  # Check if GitHub CLI is authenticated
  if ! gh auth status &>/dev/null; then
    echo "❌ GitHub CLI not authenticated. Run: gh auth login"
    exit 1
  fi
  
  # Set release tag based on type
  case $RELEASE_TYPE in
    dev)
      RELEASE_TAG="v${PLUGIN_VERSION}-dev"
      RELEASE_TITLE="Development Build ${PLUGIN_VERSION}"
      IS_PRERELEASE="--prerelease"
      ;;
    beta)
      RELEASE_TAG="v${PLUGIN_VERSION}-beta"
      RELEASE_TITLE="Beta Release ${PLUGIN_VERSION}"
      IS_PRERELEASE="--prerelease"
      ;;
    release)
      RELEASE_TAG="v${PLUGIN_VERSION}"
      RELEASE_TITLE="PlunderTube ${PLUGIN_VERSION}"
      IS_PRERELEASE=""
      ;;
  esac
  
  # Generate AI-enhanced release notes
  echo "📝 Generating AI-enhanced release notes..."
  RELEASE_NOTES_CONTENT=""
  
  # Try to generate AI-enhanced release notes if API key is available (prefer OpenRouter)
  if [ -n "$OPENROUTER_KEY_PRIVATE" ] || [ -n "$OPENAI_API_KEY" ]; then
    echo "🤖 Using AI to generate user-friendly release notes..."
    if [ -n "$OPENROUTER_KEY_PRIVATE" ]; then
      echo "   Using OpenRouter with model: ${RELEASE_NOTES_MODEL:-openai/gpt-4.1-mini}"
    fi
    if RELEASE_NOTES_CONTENT=$(python3 "${ROOT_DIR}/scripts/generate_release_notes.py" --version "${PLUGIN_VERSION}" --format sparkle --ai 2>/dev/null); then
      echo "✅ AI-enhanced release notes generated successfully"
    else
      echo "⚠️  AI generation failed, falling back to standard release notes"
      RELEASE_NOTES_CONTENT=""
    fi
  else
    echo "ℹ️  No API key found (set OPENROUTER_KEY_PRIVATE or OPENAI_API_KEY), using standard release notes"
  fi
  
  # Fallback to standard generation if AI failed or not available
  if [ -z "$RELEASE_NOTES_CONTENT" ]; then
    echo "📝 Generating standard release notes..."
    RELEASE_NOTES_CONTENT=$(python3 "${ROOT_DIR}/scripts/generate_release_notes.py" --version "${PLUGIN_VERSION}" --format sparkle 2>/dev/null || echo "<p>Minor updates and improvements.</p>")
  fi
  
  # Build complete release notes with download info
  RELEASE_NOTES="<h2>PlunderTube ${PLUGIN_VERSION}</h2>

<h3>📥 Download</h3>
<ul>
  <li><strong>DMG Installer</strong>: PlunderTube.${PLUGIN_VERSION}.dmg</li>
  <li><strong>PKG Installer</strong>: PlunderTube.${PLUGIN_VERSION}.pkg</li>
</ul>

<h3>📦 What's Included</h3>
<ul>
  <li>Audio Unit (AU) Plugin</li>
  <li>VST3 Plugin</li>
  <li>Standalone Application</li>
  <li>Automatic Python/UV environment setup</li>
  <li>FFmpeg and yt-dlp included</li>
</ul>

${RELEASE_NOTES_CONTENT}

<h3>💾 Installation</h3>
<ol>
  <li>Download the DMG file</li>
  <li>Open and run the PKG installer</li>
  <li>Restart your DAW if running</li>
</ol>

<h3>⚙️ System Requirements</h3>
<ul>
  <li>macOS 14.0 or later (ARM64)</li>
  <li>Compatible DAW for plugin formats</li>
</ul>

<p><strong>Release Type:</strong> $(echo $RELEASE_TYPE | tr '[:lower:]' '[:upper:]')</p>"

  # Create release
  echo "Creating release $RELEASE_TAG..."
  gh release create "$RELEASE_TAG" \
    --repo "danielraffel/pt-releases" \
    --title "$RELEASE_TITLE" \
    --notes "$RELEASE_NOTES" \
    $IS_PRERELEASE \
    "$DMG_PATH" \
    "$PKG_PATH" \
    "$ZIP_DMG_PATH"
  
  if [ $? -eq 0 ]; then
    echo "✅ Successfully published to pt-releases"
    echo "   Release URL: https://github.com/danielraffel/pt-releases/releases/tag/$RELEASE_TAG"
    
    # Generate appcast.xml
    echo "📝 Generating appcast.xml..."
    cat > "${DESKTOP}/appcast.xml" << EOF
<?xml version="1.0" encoding="utf-8"?>
<rss version="2.0" xmlns:sparkle="http://www.andymatuschak.org/xml-namespaces/sparkle">
  <channel>
    <title>PlunderTube</title>
    <description>PlunderTube Updates</description>
    <language>en</language>
    <item>
      <title>Version ${PLUGIN_VERSION}</title>
      <pubDate>$(date -u +"%a, %d %b %Y %H:%M:%S +0000")</pubDate>
      <enclosure url="https://github.com/danielraffel/pt-releases/releases/download/${RELEASE_TAG}/PlunderTube.${PLUGIN_VERSION}.dmg"
                 length="$(stat -f%z "$DMG_PATH" 2>/dev/null || stat -c%s "$DMG_PATH")"
                 type="application/octet-stream"
                 sparkle:version="${PLUGIN_VERSION}" />
      <sparkle:minimumSystemVersion>14.0</sparkle:minimumSystemVersion>
      <description><![CDATA[${RELEASE_NOTES}]]></description>
    </item>
  </channel>
</rss>
EOF
    
    # Upload appcast
    echo "📤 Uploading appcast.xml..."
    gh release upload "$RELEASE_TAG" \
      --repo "danielraffel/pt-releases" \
      "${DESKTOP}/appcast.xml" \
      --clobber
    
    echo "✅ Appcast uploaded to release"
    
    # Also update public pt-appcast repo
    echo "📝 Updating public appcast repository..."
    APPCAST_REPO_DIR="${DESKTOP}/pt-appcast-temp"
    
    # Clone or update the public appcast repo
    if ! git clone https://github.com/danielraffel/pt-appcast.git "$APPCAST_REPO_DIR" 2>/dev/null; then
      echo "⚠️  Could not clone pt-appcast repo. Please create it first:"
      echo "   gh repo create pt-appcast --public --description 'PlunderTube update feed'"
    else
      cd "$APPCAST_REPO_DIR"
      
      # Update appcast files based on release type
      if [ "$RELEASE_TYPE" = "release" ]; then
        # For production releases, update both files
        cp "${DESKTOP}/appcast.xml" .
        # Also update beta feed with this release
        cp "${DESKTOP}/appcast.xml" appcast-beta.xml
      else
        # For dev/beta, only update beta feed
        cp "${DESKTOP}/appcast.xml" appcast-beta.xml
      fi
      
      # Commit and push
      git config user.name "PlunderTube Release Bot"
      git config user.email "releases@plundertube.local"
      git add -A
      if git diff --staged --quiet; then
        echo "ℹ️  No changes to appcast files"
      else
        git commit -m "Update appcast for version ${PLUGIN_VERSION} (${RELEASE_TYPE})"
        git push origin main
        echo "✅ Public appcast repository updated"
      fi
      
      cd "$DESKTOP"
      rm -rf "$APPCAST_REPO_DIR"
    fi
  else
    echo "❌ Failed to publish release"
    exit 1
  fi
fi

# === CLEANUP ===
echo ""
echo "🧹 Cleaning up temporary files..."
rm -rf "$STAGING_DMG_DIR"
rm -rf "$SCRIPTS_DIR"
rm -rf "$RESOURCES_DIR"
rm -rf "$TEMP_ROOT"
rm -rf "$TEMP_SIGNING_DIR"
rm -f "$COMPONENT_PKG_PATH"
rm -f "$VST3_PKG_PATH"
rm -f "$STANDALONE_PKG_PATH"
rm -f "$RESOURCES_PKG_PATH"
rm -f "$DISTRIBUTION_FILE"

# Clean published files if requested (but keep PKG for testing)
if [ "$CLEAN_AFTER_PUBLISH" = true ] && [ "$PUBLISH_TO_RELEASES" = true ]; then
  echo "🗑️  Cleaning published files from Desktop (keeping PKG for testing)..."
  # rm -f "$PKG_PATH"  # Keep PKG for local testing
  rm -f "$DMG_PATH"
  rm -f "$ZIP_DMG_PATH"
  rm -f "${DESKTOP}/appcast.xml"
  echo "✅ Published files removed from Desktop"
  echo "📦 Kept PKG for testing: $PKG_PATH"
fi

# === FINAL SUMMARY ===
echo ""
echo "✅ DONE!"
echo "• Version: ${PLUGIN_VERSION}"
if [ "$PUBLISH_TO_RELEASES" = true ]; then
  echo "• Published to: https://github.com/danielraffel/pt-releases/releases/tag/$RELEASE_TAG"
else
  echo "• Notarized .pkg: $PKG_PATH"
  echo "• Distributable .dmg: $DMG_PATH"
  echo "• Compressed .dmg.zip: $ZIP_DMG_PATH"
fi

echo ""
echo "📋 RELEASE WORKFLOW:"
echo "1. Development: ./scripts/sign_and_package_plugin.sh --publish --clean"
echo "2. Beta testing: ./scripts/sign_and_package_plugin.sh --publish --clean --beta"
echo "3. Production: ./scripts/sign_and_package_plugin.sh --publish --clean --release"