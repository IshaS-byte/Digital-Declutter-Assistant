# Digital Declutter Assistant

A powerful desktop application designed to help users manage and clean up their digital files efficiently. This project combines a C++ backend server with a React frontend to provide an intuitive interface for organizing and removing unnecessary files from your system.

## Features

- **Smart File Browsing**: Navigate through your file system with an intuitive directory explorer
- **File Management**: View, create, and delete files with ease
- **Advanced Cleanup**: Intelligently scan and delete files by type and modification date
- **Real-time Scanning**: Recursively scan directories and preview files before deletion
- **Safe Operations**: Confirm deletions before executing to prevent accidental data loss
- **Cross-Platform File System Support**: Works seamlessly with Windows drives and directories

## Project Structure

### Backend (C++ Server)
- `server.cpp` - HTTP server built with Winsock2, handles all file operations and cleanup tasks
- `main.cpp` - Console application for file management operations
- `httplib.h` - HTTP library for server communication
- `ws_server.hpp` - WebSocket server support

### Frontend (React + Vite)
- `src/App.jsx` - Main React component managing UI and API communication
- `src/App.css` - Application styling with modern design
- `vite.config.js` - Vite configuration for development and building
- `eslint.config.js` - ESLint rules for code quality

## Installation & Setup

### Prerequisites
- Node.js (for frontend development)
- C++ compiler (g++ with MinGW on Windows)
- Windows OS (currently Windows-specific due to Windows API dependencies)

### Backend Setup
1. Compile the C++ server:
   ```bash
   g++ -fdiagnostics-color=always -g server.cpp -o server.exe -lws2_32
   ```
2. Run the server:
   ```bash
   .\server.exe
   ```
   The server will start on `http://localhost:8080`

### Frontend Setup
1. Install dependencies:
   ```bash
   npm install
   ```
2. Run the development server:
   ```bash
   npm run dev
   ```
3. Build for production:
   ```bash
   npm run build
   ```



## How to Use This App

Using the Digital Declutter Assistant is simple and straightforward:

1. **Start the App**: Open the application in your browser (usually at `http://localhost:3000` for the frontend) and run g++ server.cpp -o server.exe -lws2_32 -std=c++17 and then ./server.exe in a different terminal, keep both running at the same time.

2. **Choose a Directory**: Click on a folder in your file system to browse its contents.Click refresh if the view does not update.

3. **Refresh Files**: Reload file list to show any changes

4. **Smart Clean Up**:
   - Click the "Clean" button to start a cleanup operation
   - Enter the file type you want to remove (e.g., `.txt`, `.log`, `.tmp`)
   - Set how many days old the files should be (e.g., delete files older than 30 days)
   - Click "Scan" to see what will be deleted

5. **Review Before Deleting**: The app will show you a list of files that match your criteria

6. **Confirm and Delete**: If everything looks good, click "Delete" to remove the selected files

6. **Create File**: You can also create new empty files in any folder



## How It Works

1. **Browse**: Start the application and navigate through your file system
2. **Scan**: Configure cleanup parameters (file type, age of files)
3. **Review**: Preview the files that will be deleted before proceeding
4. **Clean**: Execute the cleanup operation with confirmation

### API Endpoints

The backend server provides the following endpoints:

- `GET /drives` - List all available drives
- `GET /list?dir=<path>` - List files and directories in a given path
- `POST /scan-cleanup` - Scan for files matching cleanup criteria
- `POST /execute-cleanup` - Execute file deletion
- `DELETE /delete` - Delete a specific file
- `POST /create` - Create a new file

## Technology Stack

- **Frontend**: React 18 with Vite for fast development and building
- **Backend**: C++ with Winsock2 for network communication
- **Styling**: Custom CSS with modern design principles
- **Build Tool**: Vite for optimized production builds
