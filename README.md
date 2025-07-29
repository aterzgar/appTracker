# CV Application Tracker

A modern Qt-based GUI application to track job applications. This project uses SQLite for data storage and provides an intuitive interface for managing your job application process.

## Features

- **Modern GUI Interface**: Clean, user-friendly interface with dark theme
- **Add Applications**: Easily add new job applications with all relevant details
- **View Applications**: Display all applications in a sortable table format
- **Edit Applications**: Double-click any application to edit its details
- **Delete Applications**: Remove applications with confirmation dialog
- **Data Validation**: Automatic validation of dates and duplicate detection
- **Persistent Storage**: SQLite database for reliable data storage
- **Search & Sort**: Built-in table sorting and selection capabilities

## Application Fields

Each job application tracks the following information:
- **Company Name**: The company you applied to
- **Application Date**: When you submitted the application
- **Position**: The job title/position
- **Contact Person**: Your point of contact at the company
- **Status**: Current application status (Applied, Interview Scheduled, etc.)
- **Notes**: Additional notes and comments

## Requirements

- CMake 3.10 or higher
- Qt5 (Widgets component)
- SQLite3 development libraries
- C++17 compatible compiler (GCC, Clang)

### Installing Dependencies

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install cmake qt5-default libqt5widgets5 libsqlite3-dev build-essential
```

**Fedora/RHEL:**
```bash
sudo dnf install cmake qt5-qtbase-devel sqlite-devel gcc-c++
```

**Arch Linux:**
```bash
sudo pacman -S cmake qt5-base sqlite gcc
```

## Building the Application

1. Clone or download the project
2. Navigate to the project directory
3. Run the build script:

```bash
./build.sh
```

Or build manually:

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

## Running the Application

After building, run the application:

```bash
./build/bin/cvTracker
```

## Usage

1. **Adding Applications**: Click "Add Application" to open the form and enter job details
2. **Viewing Applications**: All applications are displayed in the main table
3. **Editing Applications**: Double-click any row or select and click "Edit Application"
4. **Deleting Applications**: Select a row and click "Delete Application"
5. **Refreshing**: Click "Refresh" to reload data from the database

## Database

The application automatically creates a SQLite database file in your system's application data directory:
- **Linux**: `~/.local/share/CV Tracker/applications.db`
- **Windows**: `%APPDATA%/CV Tracker/applications.db`
- **macOS**: `~/Library/Application Support/CV Tracker/applications.db`

## Project Structure

```
cvTracker/
├── src/
│   ├── main.cpp           # Application entry point
│   ├── mainwindow.h       # Main window header
│   ├── mainwindow.cpp     # Main window implementation
│   ├── db_manager.h       # Database manager header
│   └── db_manager.cpp     # Database operations
├── CMakeLists.txt         # Build configuration
├── build.sh              # Build script
└── README.md             # This file
```

## Features Overview

### Modern Interface
- Dark theme for reduced eye strain
- Intuitive button layout
- Responsive table with proper column sizing
- Dialog forms for data entry

### Data Management
- Automatic duplicate detection
- Date validation (YYYY-MM-DD format)
- Required field validation
- Safe database operations with prepared statements

### User Experience
- Double-click to edit functionality
- Confirmation dialogs for destructive operations
- Status dropdown with predefined options
- Calendar popup for date selection

## Development

The application is built using:
- **Qt5**: For the GUI framework
- **SQLite3**: For database operations
- **C++17**: Modern C++ features
- **CMake**: Cross-platform build system

## License

This project is open source and available under the MIT License.

## Contributing

Feel free to submit issues, feature requests, or pull requests to improve the application.