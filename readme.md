# App Tracker

A modern Qt-based desktop application to track job applications, visualise analytics, and get AI-powered insights via [Ollama](https://ollama.com) (local, unlimited, no API key needed).

## Features

- **Modern GUI** — Clean, dark-themed interface with sortable data table
- **Full CRUD** — Add, edit, and delete applications with validation and duplicate detection
- **Analytics Dashboard** — Pie charts, status summaries, and interview/rejection rates
- **AI Insights** — One-click Ollama-powered recommendations based on your actual data
- **Search & Sort** — Filter applications and sort by any column
- **Persistent Storage** — SQLite database, auto-created in the platform's standard app-data directory

## Application Fields

| Field | Description |
|---|---|
| Company Name | The company you applied to |
| Application Date | Date of submission (YYYY-MM-DD) |
| Position | Job title/position |
| Contact Person | Your point of contact at the company |
| Status | Applied · Interviews · Offer Received · Rejected · Withdrawn |
| Notes | Additional details |

## Quick Start

### Build & Run

```bash
git clone https://github.com/aterzgar/appTracker.git
cd appTracker
./setup.sh
make run
```

For AI insights, add `--ollama` to auto-install Ollama and pull a model:

```bash
./setup.sh --ollama
```

### Prerequisites

| Component | Required | Why |
|---|---|---|
| CMake ≥ 3.10 | ✅ | Build system |
| Qt 5 (Widgets, Network, Test) | ✅ | GUI framework |
| SQLite3 dev libs | ✅ | Database |
| C++17 compiler (GCC/Clang) | ✅ | Language |
| [Ollama](https://ollama.com) | Optional | AI insights (local) |

**Ubuntu/Debian:**
```bash
sudo apt install cmake pkg-config qtbase5-dev libqt5widgets5 libqt5network5 libqt5test5 libsqlite3-dev g++
```

**Fedora/RHEL:**
```bash
sudo dnf install cmake qt5-qtbase-devel qt5-qtbase sqlite-devel gcc-c++
```

**Arch Linux:**
```bash
sudo pacman -S cmake qt5-base sqlite gcc
```

Or simply run `./setup.sh` — it detects your distro and installs everything automatically.

### AI Insights (Optional)

1. Install Ollama: `./setup.sh --ollama`
2. Run Ollama: `ollama serve`
3. Open **Analytics → Generate AI Insights** in the app

## Usage

| Action | How |
|---|---|
| Add application | Click **+ New Application** and fill in the form |
| Edit application | Select a row → click **Edit** (or right-click → Edit) |
| Delete application | Select a row → click **Delete** (confirms first) |
| Refresh data | Click **Refresh** |
| View analytics | Click **Analytics** for charts and summary statistics |
| Get AI insights | In the Analytics dialog, click **Generate AI Insights** |

## Database

The SQLite database is created automatically in the platform's standard app-data directory:

| Platform | Path |
|---|---|
| Linux | `~/.local/share/AppTracker/applications.db` |
| macOS | `~/Library/Application Support/AppTracker/applications.db` |
| Windows | `%APPDATA%/AppTracker/applications.db` |

The database filename defaults to `applications.db` and can be overridden via the `database/file` QSettings key.

## Project Structure

```
appTracker/
├── src/
│   ├── main.cpp                 # Entry point
│   ├── mainwindow.h/.cpp        # Main window + dialogs
│   ├── db_manager.h/.cpp        # SQLite persistence layer
│   ├── application_service.h/.cpp  # Business logic + prompt builder
│   ├── analytics_visualizer.h/.cpp # Pie charts + summary widgets
│   └── llm_client.h/.cpp        # Ollama API client
├── tests/
│   ├── test_db_manager.cpp
│   ├── test_analytics.cpp
│   ├── test_application_logic.cpp
│   ├── test_application_service.cpp
│   └── CMakeLists.txt
├── CMakeLists.txt
├── .gitignore
└── README.md
```

## Running Tests

```bash
mkdir build_tests && cd build_tests
cmake -DBUILD_TESTS=ON ..
make -j$(nproc)
ctest --output-on-failure
```

See [tests/TESTING.md](tests/TESTING.md) for details.

## Tech Stack

- **Qt 5** — GUI framework (Widgets, Network, Test)
- **SQLite 3** — Local database
- **C++17** — Modern C++
- **CMake** — Cross-platform build system
- **Ollama** — Local LLM inference

## License

MIT License — see [LICENSE](LICENSE).

