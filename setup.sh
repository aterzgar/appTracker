#!/usr/bin/env bash
#
# setup.sh — Install dependencies and build appTracker.
#
# Usage:
#   ./setup.sh          # install deps + build
#   ./setup.sh --deps   # install deps only
#   ./setup.sh --build  # build only (skip deps)
#   ./setup.sh --ollama # install deps + build + setup Ollama
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

INSTALL_DEPS=false
BUILD_ONLY=false
INSTALL_OLLAMA=false

for arg in "$@"; do
    case "$arg" in
        --deps)    INSTALL_DEPS=true ;;
        --build)   BUILD_ONLY=true ;;
        --ollama)  INSTALL_OLLAMA=true ;;
        --help)
            echo "Usage: $0 [--deps|--build|--ollama]"
            echo "  (no flags)  Install deps + build"
            echo "  --deps      Install dependencies only"
            echo "  --build     Build only (skip deps)"
            echo "  --ollama    Install deps + build + setup Ollama (AI insights)"
            exit 0
            ;;
    esac
done

install_deps() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        case "$ID" in
            ubuntu|debian|linuxmint)
                echo "📦 Detected $NAME — installing dependencies..."
                sudo apt-get update
                sudo apt-get install -y \
                    build-essential cmake pkg-config \
                    qtbase5-dev libqt5widgets5 libqt5network5 libqt5test5 \
                    libsqlite3-dev g++
                ;;
            fedora|rhel|centos)
                echo "📦 Detected $NAME — installing dependencies..."
                sudo dnf install -y \
                    cmake qt5-qtbase-devel qt5-qtbase sqlite-devel gcc-c++
                ;;
            arch|manjaro)
                echo "📦 Detected $NAME — installing dependencies..."
                sudo pacman -S --noconfirm \
                    cmake qt5-base sqlite gcc pkg-config
                ;;
            opensuse-tumbleweed|opensuse-leap)
                echo "📦 Detected $NAME — installing dependencies..."
                sudo zypper install -y \
                    cmake pkg-config libqt5-qtbase-devel sqlite3-devel gcc-c++
                ;;
            *)
                echo "⚠️  Unknown distro '$ID'. Install deps manually:"
                echo "   - cmake ≥ 3.10"
                echo "   - qtbase5-dev"
                echo "   - libsqlite3-dev"
                echo "   - g++ / gcc-c++"
                exit 1
                ;;
        esac
    else
        echo "⚠️  Cannot detect OS. Install dependencies manually."
        exit 1
    fi
}

install_ollama() {
    if command -v ollama &>/dev/null; then
        echo "✅ Ollama already installed ($(ollama --version 2>/dev/null || echo 'unknown'))"
    else
        echo "🤖 Installing Ollama..."
        curl -fsSL https://ollama.com/install.sh | sh
    fi

    echo "📥 Pulling llama3.2 model (this may take a while, ~2 GB)..."
    ollama pull llama3.2

    echo "✅ Ollama ready. Start it with: ollama serve"
}

build() {
    echo "🔨 Building appTracker..."
    mkdir -p "$BUILD_DIR" && cd "$BUILD_DIR"
    cmake ..
    make -j"$(nproc)"
    echo ""
    echo "✅ Build complete. Run with:"
    echo "   make -C \"$BUILD_DIR\" run"
    echo "   or"
    echo "   cd \"$BUILD_DIR\" && ./bin/appTracker"
}

if [ "$BUILD_ONLY" = false ]; then
    install_deps
fi

if [ "$INSTALL_DEPS" = false ]; then
    build
fi

if [ "$INSTALL_OLLAMA" = true ]; then
    install_ollama
fi
