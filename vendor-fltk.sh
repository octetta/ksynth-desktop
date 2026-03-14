#!/usr/bin/env sh
# vendor-fltk.sh
# Clones FLTK 1.4.x into vendor/fltk and strips git history.
# Run once, commit the result. That's it.

set -e

DEST="vendor/fltk"
TAG="release-1.4.1"

if [ -d "$DEST/CMakeLists.txt" ] || [ -f "$DEST/CMakeLists.txt" ]; then
    echo "$DEST already populated. Remove it first to re-vendor."
    exit 1
fi

mkdir -p vendor

echo "Cloning FLTK $TAG..."
git clone --depth=1 --branch "$TAG" \
    https://github.com/fltk/fltk.git "$DEST"

rm -rf "$DEST/.git"

echo ""
echo "Done. Now run:"
echo "  git add vendor/fltk"
echo "  git commit -m 'vendor: add FLTK $TAG'"
echo "  make"
