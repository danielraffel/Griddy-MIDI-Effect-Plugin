#!/bin/bash

# Quick build script for local testing without code signing
# Usage: ./scripts/quick_build.sh [target] [config] [--bump]
#   target: standalone (default), au, vst, all
#   config: debug (default), release
#   --bump: increment build number for Logic Pro recognition

set -e

# Parse arguments
TARGET="standalone"
CONFIG="debug"
BUMP_BUILD=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --bump)
            BUMP_BUILD=true
            shift
            ;;
        standalone|au|vst|all)
            TARGET="$1"
            shift
            ;;
        debug|release)
            CONFIG="$1"
            shift
            ;;
        *)
            echo "Unknown argument: $1"
            echo "Usage: $0 [target] [config] [--bump]"
            echo "  target: standalone, au, vst, all (default: standalone)"
            echo "  config: debug, release (default: debug)"
            echo "  --bump: increment build number for DAW recognition"
            exit 1
            ;;
    esac
done

# Convert config to proper case
if [ "$CONFIG" = "release" ]; then
    BUILD_CONFIG="Release"
else
    BUILD_CONFIG="Debug"
fi

echo "🚀 Quick Build - Target: $TARGET, Config: $BUILD_CONFIG"

# Load environment
if [ -f .env ]; then
    set -o allexport
    source .env
    set +o allexport
else
    echo "❌ .env file not found"
    exit 1
fi

# Handle version bumping for test builds
if [ "$BUMP_BUILD" = true ]; then
    echo "📦 Incrementing build number for test build..."
    python3 scripts/bump_version.py --build-only
    echo "✅ Build number incremented (semantic version unchanged)"
else
    echo "📦 Using current version (no bump)"
fi

# Always load version info and EXPORT for CMake
eval $(python3 scripts/bump_version.py --export-only)
# Export version variables so CMake can see them
export PROJECT_VERSION
export PROJECT_VERSION_MAJOR
export PROJECT_VERSION_MINOR
export PROJECT_VERSION_PATCH
export AU_VERSION_INT
export BUILD_NUMBER
echo "🏗 Building version $PROJECT_VERSION (build $BUILD_NUMBER)"
echo "   AU Version: $AU_VERSION_INT (Logic Pro will see this)"

# Check if we need to regenerate (smart detection)
NEED_REGEN=0

# Check if build directory exists AND has Xcode project
if [ ! -d "build" ] || [ ! -d "build/Griddy.xcodeproj" ]; then
    echo "📦 Build directory missing or incomplete - will regenerate"
    NEED_REGEN=1
fi

# CRITICAL: Check if version in CMake cache matches current version
if [ -f "build/CMakeCache.txt" ]; then
    CMAKE_VERSION=$(grep "CMAKE_PROJECT_VERSION:STATIC=" "build/CMakeCache.txt" 2>/dev/null | cut -d= -f2 || echo "unknown")
    if [ "$CMAKE_VERSION" != "$PROJECT_VERSION" ]; then
        echo "⚠️  Version mismatch in CMake: cached=$CMAKE_VERSION, current=$PROJECT_VERSION"
        echo "📦 Will regenerate to update version"
        NEED_REGEN=1
    fi
fi

# Check if CMakeLists.txt is newer than build directory
if [ -f "CMakeLists.txt" ] && [ -d "build" ]; then
    if [ "CMakeLists.txt" -nt "build" ]; then
        echo "📦 CMakeLists.txt changed - will regenerate"
        NEED_REGEN=1
    fi
fi

# Check if .env is newer than build directory
if [ -f ".env" ] && [ -d "build" ]; then
    if [ ".env" -nt "build" ]; then
        echo "📦 .env changed - will regenerate"
        NEED_REGEN=1
    fi
fi

# Allow override
if [ "$FORCE_REGEN" = "1" ]; then
    echo "📦 Forced regeneration requested"
    NEED_REGEN=1
fi

# Generate or skip based on need
if [ "$NEED_REGEN" = "1" ]; then
    echo "🔧 Regenerating Xcode project..."
    # Skip version bump since we already handled it above
    SKIP_VERSION_BUMP=1 ./generate_and_open_xcode.sh "$CONFIG"
else
    echo "⚡ Skipping regeneration - using existing build"
fi

# Navigate to build directory
cd build || { echo "❌ Failed to enter build directory"; exit 1; }

# Build based on target
case "$TARGET" in
    standalone)
        echo "🎹 Building Standalone app..."
        xcodebuild -project Griddy.xcodeproj \
                   -scheme Griddy_Standalone \
                   -configuration "$BUILD_CONFIG" \
                   CODE_SIGN_IDENTITY="" \
                   CODE_SIGNING_REQUIRED=NO \
                   build
        
        APP_PATH="Griddy_artefacts/$BUILD_CONFIG/Standalone/Griddy.app"
        if [ -d "$APP_PATH" ]; then
            echo ""
            echo "✅ Built: build/$APP_PATH"
            echo "📂 Run with: open \"$PWD/$APP_PATH\""
        fi
        ;;
        
    au)
        echo "🎛 Building Audio Unit..."
        xcodebuild -project Griddy.xcodeproj \
                   -scheme Griddy_AU \
                   -configuration "$BUILD_CONFIG" \
                   CODE_SIGN_IDENTITY="" \
                   CODE_SIGNING_REQUIRED=NO \
                   build
        
        COMPONENT_PATH="Griddy_artefacts/$BUILD_CONFIG/AU/Griddy.component"
        if [ -d "$COMPONENT_PATH" ]; then
            echo ""
            echo "✅ Built: build/$COMPONENT_PATH"
            echo "📂 To test in Logic Pro:"
            echo "   cp -r \"$PWD/$COMPONENT_PATH\" ~/Library/Audio/Plug-Ins/Components/"
            echo "   # Then restart Logic Pro"
        fi
        ;;
        
    vst)
        echo "🎚 Building VST3..."
        xcodebuild -project Griddy.xcodeproj \
                   -scheme Griddy_VST3 \
                   -configuration "$BUILD_CONFIG" \
                   CODE_SIGN_IDENTITY="" \
                   CODE_SIGNING_REQUIRED=NO \
                   build
        
        VST_PATH="Griddy_artefacts/$BUILD_CONFIG/VST3/Griddy.vst3"
        if [ -d "$VST_PATH" ]; then
            echo ""
            echo "✅ Built: build/$VST_PATH"
            echo "📂 To test in your DAW:"
            echo "   cp -r \"$PWD/$VST_PATH\" ~/Library/Audio/Plug-Ins/VST3/"
        fi
        ;;
        
    all)
        echo "🎹 Building all targets..."
        # Build all targets
        for scheme in Griddy_Standalone Griddy_AU Griddy_VST3; do
            echo "Building $scheme..."
            xcodebuild -project Griddy.xcodeproj \
                       -scheme "$scheme" \
                       -configuration "$BUILD_CONFIG" \
                       CODE_SIGN_IDENTITY="" \
                       CODE_SIGNING_REQUIRED=NO \
                       build
        done
        
        echo ""
        echo "✅ All targets built successfully"
        echo "📂 Artifacts in: build/Griddy_artefacts/$BUILD_CONFIG/"
        ;;
        
    *)
        echo "❌ Unknown target: $TARGET"
        echo "Valid targets: standalone, au, vst, all"
        exit 1
        ;;
esac

echo ""
echo "🎉 Quick build complete!"
echo "📊 Version: $PROJECT_VERSION (build $BUILD_NUMBER)"