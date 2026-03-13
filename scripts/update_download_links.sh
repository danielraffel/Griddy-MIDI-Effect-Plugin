#!/bin/bash
# update_download_links.sh — Updates download links in README.md and gh-pages index.html
# Called automatically during publish, or manually: ./scripts/update_download_links.sh [version]
#
# Usage:
#   ./scripts/update_download_links.sh          # reads version from .env
#   ./scripts/update_download_links.sh 1.0.15   # explicit version

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$ROOT_DIR"

# Load .env
if [[ -f .env ]]; then
    set -a; source .env; set +a
fi

# Determine version
if [[ -n "${1:-}" ]]; then
    VERSION="$1"
else
    VERSION="${VERSION_MAJOR:-0}.${VERSION_MINOR:-0}.${VERSION_PATCH:-0}"
fi

GITHUB_USER="${GITHUB_USERNAME:-danielraffel}"
GITHUB_REPO="${PROJECT_NAME:-Griddy-MIDI-Effect-Plugin}"
REPO_SLUG="${GITHUB_USER}/${GITHUB_REPO}"
TAG="v${VERSION}"

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "Updating download links to version ${VERSION} (${TAG})..."

# ── 1. Update README.md ──────────────────────────────────────────────
README="$ROOT_DIR/README.md"
if [[ -f "$README" ]]; then
    # Replace versioned download URLs (matches patterns like /download/v1.0.12/Griddy_1.0.12.pkg)
    sed -i '' -E \
        "s|/releases/download/v[0-9]+\.[0-9]+\.[0-9]+/${GITHUB_REPO}_[0-9]+\.[0-9]+\.[0-9]+\.pkg|/releases/download/${TAG}/${GITHUB_REPO}_${VERSION}.pkg|g" \
        "$README"
    sed -i '' -E \
        "s|/releases/download/v[0-9]+\.[0-9]+\.[0-9]+/${GITHUB_REPO}_[0-9]+\.[0-9]+\.[0-9]+_Setup\.exe|/releases/download/${TAG}/${GITHUB_REPO}_${VERSION}_Setup.exe|g" \
        "$README"
    echo -e "${GREEN}✅ Updated README.md download links${NC}"
else
    echo -e "${YELLOW}⚠️  README.md not found${NC}"
fi

# ── 2. Update gh-pages index.html ────────────────────────────────────
# Check out gh-pages, update, commit, push
GHPAGES_BRANCH="gh-pages"
if git rev-parse --verify "$GHPAGES_BRANCH" &>/dev/null; then
    # Create a temp worktree for gh-pages
    TMPDIR=$(mktemp -d)
    git worktree add "$TMPDIR" "$GHPAGES_BRANCH" --quiet 2>/dev/null || {
        echo -e "${YELLOW}⚠️  Could not create gh-pages worktree${NC}"
        rm -rf "$TMPDIR"
        exit 0
    }

    INDEX="$TMPDIR/index.html"
    if [[ -f "$INDEX" ]]; then
        sed -i '' -E \
            "s|/releases/download/v[0-9]+\.[0-9]+\.[0-9]+/${GITHUB_REPO}_[0-9]+\.[0-9]+\.[0-9]+\.pkg|/releases/download/${TAG}/${GITHUB_REPO}_${VERSION}.pkg|g" \
            "$INDEX"
        sed -i '' -E \
            "s|/releases/download/v[0-9]+\.[0-9]+\.[0-9]+/${GITHUB_REPO}_[0-9]+\.[0-9]+\.[0-9]+_Setup\.exe|/releases/download/${TAG}/${GITHUB_REPO}_${VERSION}_Setup.exe|g" \
            "$INDEX"

        # Commit and push if changed
        cd "$TMPDIR"
        if ! git diff --quiet index.html; then
            git add index.html
            git commit -m "Update download links to ${TAG}" --quiet
            git push origin "$GHPAGES_BRANCH" --quiet
            echo -e "${GREEN}✅ Updated and pushed gh-pages index.html${NC}"
        else
            echo "gh-pages index.html already up to date"
        fi
        cd "$ROOT_DIR"
    else
        echo -e "${YELLOW}⚠️  No index.html on gh-pages${NC}"
    fi

    # Cleanup worktree
    git worktree remove "$TMPDIR" --force 2>/dev/null || rm -rf "$TMPDIR"
else
    echo -e "${YELLOW}⚠️  No gh-pages branch found${NC}"
fi

# ── 3. Commit README changes on main if needed ───────────────────────
cd "$ROOT_DIR"
if ! git diff --quiet README.md 2>/dev/null; then
    git add README.md
    git commit -m "Update download links to ${TAG}" --quiet
    echo -e "${GREEN}✅ Committed README.md changes${NC}"
fi

echo "Done."
