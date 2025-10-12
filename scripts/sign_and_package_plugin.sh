#!/bin/bash
set -e

# sign_and_package_plugin.sh - Build, sign, and package all plugin formats
# This script now supports AU, VST3, and Standalone formats

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

# === Auto-bump version (unless explicitly disabled) ===
if [ "$SKIP_VERSION_BUMP" != "1" ]; then
  echo "📦 Auto-bumping version based on recent commits..."
  python3 "${ROOT_DIR}/scripts/bump_version.py" --auto
else
  echo "📦 Skipping version bump (SKIP_VERSION_BUMP=1)"
fi

# Load version information
eval $(python3 "${ROOT_DIR}/scripts/bump_version.py" --export-only)
echo "✅ Building version: $PROJECT_VERSION"

# === Build plugins if needed ===
echo "🔍 Checking for existing builds..."
BUILD_DIR="${ROOT_DIR}/build"
AU_BUILD_PATH="${BUILD_DIR}/${PROJECT_NAME}_artefacts/Release/AU/${PROJECT_NAME}.component"
VST3_BUILD_PATH="${BUILD_DIR}/${PROJECT_NAME}_artefacts/Release/VST3/${PROJECT_NAME}.vst3"
STANDALONE_BUILD_PATH="${BUILD_DIR}/${PROJECT_NAME}_artefacts/Release/Standalone/${PROJECT_NAME}.app"

# Check if we need to build
NEED_BUILD=false
if [ ! -d "$AU_BUILD_PATH" ] && [ ! -d "$VST3_BUILD_PATH" ] && [ ! -d "$STANDALONE_BUILD_PATH" ]; then
  echo "⚠️ No Release builds found. Building all formats..."
  NEED_BUILD=true
fi

if [ "$NEED_BUILD" = true ] || [ "$FORCE_BUILD" = "1" ]; then
  echo "🏗 Building plugins in Release mode..."
  
  # Generate Xcode project if needed
  if [ ! -d "$BUILD_DIR" ] || [ ! -f "$BUILD_DIR/${PROJECT_NAME}.xcodeproj/project.pbxproj" ]; then
    echo "📦 Generating Xcode project..."
    cd "$ROOT_DIR"
    ./generate_and_open_xcode.sh release
  fi
  
  # Build all formats
  cd "$BUILD_DIR"
  echo "🎹 Building all plugin formats..."
  for scheme in ${PROJECT_NAME}_AU ${PROJECT_NAME}_VST3 ${PROJECT_NAME}_Standalone; do
    echo "   Building $scheme..."
    xcodebuild -project ${PROJECT_NAME}.xcodeproj \
               -scheme "$scheme" \
               -configuration Release \
               build
  done
  cd "$ROOT_DIR"
fi

# Define paths for all plugin formats (both build and system locations)
COMPONENT_PATH="$HOME/Library/Audio/Plug-Ins/Components/${PROJECT_NAME}.component"
VST3_PATH="$HOME/Library/Audio/Plug-Ins/VST3/${PROJECT_NAME}.vst3"
STANDALONE_PATH="/Applications/${PROJECT_NAME}.app"
DESKTOP="$HOME/Desktop"
ZIP_PATH="${DESKTOP}/${PROJECT_NAME}.zip"
PKG_PATH="${DESKTOP}/${PROJECT_NAME}.pkg"
DMG_PATH="${DESKTOP}/${PROJECT_NAME}.dmg"
STAGING_DMG_DIR="${DESKTOP}/${PROJECT_NAME}_Installer"
TEMP_DIR="${DESKTOP}/${PROJECT_NAME}_temp"

# Track which formats are available
HAS_AU=false
HAS_VST3=false
HAS_STANDALONE=false

# Clean up temp directory
rm -rf "$TEMP_DIR"
mkdir -p "$TEMP_DIR"

# Set default BUILD_FORMATS if not defined
if [ -z "$BUILD_FORMATS" ]; then
  BUILD_FORMATS="AU VST3 STANDALONE"
  echo "   Using default BUILD_FORMATS: $BUILD_FORMATS"
else
  echo "   Configured BUILD_FORMATS: $BUILD_FORMATS"
fi

# === Copy from build directory if not in system locations ===
echo "📂 Checking plugin locations..."

# Copy AU if it exists in build but not in system (and is in BUILD_FORMATS)
if [ -d "$AU_BUILD_PATH" ] && [ ! -d "$COMPONENT_PATH" ] && [[ "$BUILD_FORMATS" == *"AU"* ]]; then
  echo "   Copying AU from build to system location..."
  cp -R "$AU_BUILD_PATH" "$COMPONENT_PATH"
fi

# Copy VST3 if it exists in build but not in system (and is in BUILD_FORMATS)
if [ -d "$VST3_BUILD_PATH" ] && [ ! -d "$VST3_PATH" ] && [[ "$BUILD_FORMATS" == *"VST3"* ]]; then
  echo "   Copying VST3 from build to system location..."
  cp -R "$VST3_BUILD_PATH" "$VST3_PATH"
fi

# Copy Standalone if it exists in build but not in system (and is in BUILD_FORMATS)
if [ -d "$STANDALONE_BUILD_PATH" ] && [ ! -d "$STANDALONE_PATH" ] && [[ "$BUILD_FORMATS" == *"STANDALONE"* ]]; then
  echo "   Copying Standalone from build to Applications..."
  cp -R "$STANDALONE_BUILD_PATH" "$STANDALONE_PATH"
elif [ -d "$STANDALONE_BUILD_PATH" ] && [[ "$BUILD_FORMATS" != *"STANDALONE"* ]]; then
  echo "   Skipping Standalone (not in BUILD_FORMATS)"
fi

# === Validate and sign Audio Unit ===
if [ -d "$COMPONENT_PATH" ] && [[ "$BUILD_FORMATS" == *"AU"* ]]; then
  echo "🔍 Validating Audio Unit..."
  COMPONENT_BINARY="$COMPONENT_PATH/Contents/MacOS/${PROJECT_NAME}"
  if [ ! -f "$COMPONENT_BINARY" ]; then
    echo "⚠️ AU binary not found at $COMPONENT_BINARY"
  else
    # Check binary architecture
    ARCH_INFO=$(file "$COMPONENT_BINARY")
    if [[ ! "$ARCH_INFO" =~ "Mach-O" ]]; then
      echo "⚠️ AU binary is not a valid Mach-O binary"
    else
      echo "✅ AU validated: $(basename "$COMPONENT_PATH")"
      echo "   Architecture: $(lipo -archs "$COMPONENT_BINARY" 2>/dev/null || echo "unknown")"
      
      echo "🔏 Signing Audio Unit..."
      codesign --timestamp --options runtime --force --deep \
        --sign "$APP_CERT" "$COMPONENT_PATH"
      
      # Copy to temp directory for packaging
      cp -R "$COMPONENT_PATH" "$TEMP_DIR/"
      HAS_AU=true
    fi
  fi
else
  echo "ℹ️ Audio Unit not found at $COMPONENT_PATH"
fi

# === Validate and sign VST3 ===
if [ -d "$VST3_PATH" ] && [[ "$BUILD_FORMATS" == *"VST3"* ]]; then
  echo "🔍 Validating VST3..."
  VST3_BINARY="$VST3_PATH/Contents/MacOS/${PROJECT_NAME}"
  if [ ! -f "$VST3_BINARY" ]; then
    echo "⚠️ VST3 binary not found at $VST3_BINARY"
  else
    ARCH_INFO=$(file "$VST3_BINARY")
    if [[ ! "$ARCH_INFO" =~ "Mach-O" ]]; then
      echo "⚠️ VST3 binary is not a valid Mach-O binary"
    else
      echo "✅ VST3 validated: $(basename "$VST3_PATH")"
      echo "   Architecture: $(lipo -archs "$VST3_BINARY" 2>/dev/null || echo "unknown")"
      
      echo "🔏 Signing VST3..."
      codesign --timestamp --options runtime --force --deep \
        --sign "$APP_CERT" "$VST3_PATH"
      
      # Copy to temp directory for packaging
      cp -R "$VST3_PATH" "$TEMP_DIR/"
      HAS_VST3=true
    fi
  fi
else
  echo "ℹ️ VST3 not found at $VST3_PATH"
fi

# === Validate and sign Standalone ===
if [ -d "$STANDALONE_PATH" ] && [[ "$BUILD_FORMATS" == *"STANDALONE"* ]]; then
  echo "🔍 Validating Standalone app..."
  STANDALONE_BINARY="$STANDALONE_PATH/Contents/MacOS/${PROJECT_NAME}"
  if [ ! -f "$STANDALONE_BINARY" ]; then
    echo "⚠️ Standalone binary not found at $STANDALONE_BINARY"
  else
    ARCH_INFO=$(file "$STANDALONE_BINARY")
    if [[ ! "$ARCH_INFO" =~ "Mach-O" ]]; then
      echo "⚠️ Standalone binary is not a valid Mach-O binary"
    else
      echo "✅ Standalone validated: $(basename "$STANDALONE_PATH")"
      echo "   Architecture: $(lipo -archs "$STANDALONE_BINARY" 2>/dev/null || echo "unknown")"
      
      echo "🔏 Signing Standalone app..."
      codesign --timestamp --options runtime --force --deep \
        --sign "$APP_CERT" "$STANDALONE_PATH"
      
      # Copy to temp directory for packaging
      cp -R "$STANDALONE_PATH" "$TEMP_DIR/"
      HAS_STANDALONE=true
    fi
  fi
else
  echo "ℹ️ Standalone app not found at $STANDALONE_PATH"
fi

# Check if we have anything to package
if [ "$HAS_AU" = false ] && [ "$HAS_VST3" = false ] && [ "$HAS_STANDALONE" = false ]; then
  echo "❌ No plugins found to package!"
  echo "   Please build at least one format first"
  exit 1
fi

echo ""
echo "📦 Creating ZIP for notarization..."
echo "   Formats included:"
[ "$HAS_AU" = true ] && echo "   - Audio Unit"
[ "$HAS_VST3" = true ] && echo "   - VST3"
[ "$HAS_STANDALONE" = true ] && echo "   - Standalone"

# Create ZIP with all plugins
cd "$TEMP_DIR"
rm -f "$ZIP_PATH"
# Use zip instead of ditto for better control
zip -r "$ZIP_PATH" . -x "*.DS_Store"

echo "☁️ Notarizing ZIP..."
xcrun notarytool submit "$ZIP_PATH" \
  --apple-id "$APPLE_ID" \
  --team-id "$TEAM_ID" \
  --password "$APP_SPECIFIC_PASSWORD" \
  --wait

echo "📎 Stapling notarization tickets..."
cd "$DESKTOP"
rm -rf "${PROJECT_NAME}_stapled"
mkdir -p "${PROJECT_NAME}_stapled"
cd "${PROJECT_NAME}_stapled"
unzip -q "$ZIP_PATH"

# Debug: Show what's in the unzipped directory
echo "   Contents of stapled directory:"
ls -la

# Staple all available formats - they should be at the root level now
for item in *; do
  if [[ "$item" == *.component ]]; then
    echo "   Stapling Audio Unit: $item"
    xcrun stapler staple "$item"
  elif [[ "$item" == *.vst3 ]]; then
    echo "   Stapling VST3: $item"
    xcrun stapler staple "$item"
  elif [[ "$item" == *.app ]]; then
    echo "   Stapling Standalone: $item"
    xcrun stapler staple "$item"
  fi
done

echo "📦 Building .pkg installer..."
# Create a staging directory for the installer
INSTALLER_STAGING="$DESKTOP/${PROJECT_NAME}_installer_staging"
rm -rf "$INSTALLER_STAGING"
mkdir -p "$INSTALLER_STAGING"

# Build product archive with all available components
# We're still in ${PROJECT_NAME}_stapled directory after stapling

# Create proper directory structure for installer based on PKG_FORMATS
INSTALLER_ROOT="$INSTALLER_STAGING/root"

# Set default if PKG_FORMATS is not defined
if [ -z "$PKG_FORMATS" ]; then
  PKG_FORMATS="AU VST3"
  echo "   Using default PKG_FORMATS: $PKG_FORMATS"
fi

echo "   PKG will include: $PKG_FORMATS"

# Create directories based on configured formats
if [[ "$PKG_FORMATS" == *"AU"* ]]; then
  mkdir -p "$INSTALLER_ROOT/Library/Audio/Plug-Ins/Components"
fi
if [[ "$PKG_FORMATS" == *"VST3"* ]]; then
  mkdir -p "$INSTALLER_ROOT/Library/Audio/Plug-Ins/VST3"
fi
if [[ "$PKG_FORMATS" == *"STANDALONE"* ]]; then
  mkdir -p "$INSTALLER_ROOT/Applications"
fi

# Copy stapled plugins to proper locations based on PKG_FORMATS configuration
FOUND_COMPONENTS=false
for item in *; do
  if [[ "$item" == *.component ]] && [[ "$PKG_FORMATS" == *"AU"* ]]; then
    echo "   Adding Audio Unit to installer: $item"
    cp -R "$item" "$INSTALLER_ROOT/Library/Audio/Plug-Ins/Components/"
    FOUND_COMPONENTS=true
  elif [[ "$item" == *.vst3 ]] && [[ "$PKG_FORMATS" == *"VST3"* ]]; then
    echo "   Adding VST3 to installer: $item"
    cp -R "$item" "$INSTALLER_ROOT/Library/Audio/Plug-Ins/VST3/"
    FOUND_COMPONENTS=true
  elif [[ "$item" == *.app ]] && [[ "$PKG_FORMATS" == *"STANDALONE"* ]]; then
    echo "   Adding Standalone to installer: $item"
    cp -R "$item" "$INSTALLER_ROOT/Applications/"
    FOUND_COMPONENTS=true
  elif [[ "$item" == *.app ]] && [[ "$PKG_FORMATS" != *"STANDALONE"* ]]; then
    echo "   Skipping Standalone app (not in PKG_FORMATS)"
  fi
done

if [ "$FOUND_COMPONENTS" = false ]; then
  echo "❌ No components found to package!"
  exit 1
fi

# Build the package using --root method which handles duplicate bundle IDs
pkgbuild --root "$INSTALLER_ROOT" \
         --identifier "${PROJECT_BUNDLE_ID}.installer" \
         --version "$PROJECT_VERSION" \
         --sign "$INSTALLER_CERT" \
         "$PKG_PATH"

echo "☁️ Notarizing .pkg..."
xcrun notarytool submit "$PKG_PATH" \
  --apple-id "$APPLE_ID" \
  --team-id "$TEAM_ID" \
  --password "$APP_SPECIFIC_PASSWORD" \
  --wait

echo "📎 Stapling .pkg..."
xcrun stapler staple "$PKG_PATH"

echo "💽 Creating DMG with .pkg inside..."
mkdir -p "$STAGING_DMG_DIR"
cp "$PKG_PATH" "$STAGING_DMG_DIR/"

hdiutil create -volname "${PROJECT_NAME} Installer" \
  -srcfolder "$STAGING_DMG_DIR" \
  -ov -format UDZO "$DMG_PATH"

rm -rf "$STAGING_DMG_DIR"

# Clean up temporary directories
rm -rf "$TEMP_DIR"
rm -rf "${PROJECT_NAME}_stapled"
rm -rf "$INSTALLER_STAGING"

echo ""
echo "✅ DONE!"
echo "• Notarized .zip: $ZIP_PATH"
echo "• Notarized .pkg: $PKG_PATH"
echo "• Distributable .dmg: $DMG_PATH"
echo ""
echo "📋 ZIP includes:"
[ "$HAS_AU" = true ] && echo "   - Audio Unit (.component)"
[ "$HAS_VST3" = true ] && echo "   - VST3 (.vst3)"
[ "$HAS_STANDALONE" = true ] && echo "   - Standalone (.app)"
echo ""
echo "📦 PKG installer includes: $PKG_FORMATS"
echo ""
