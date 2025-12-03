# Log Aggregator

A smart log aggregation system written in C that monitors multiple log sources, processes log entries, detects patterns, and generates alerts.

## Features

- **Multiple Log Sources**: Monitor log files from directories and receive logs via network sockets
- **Thread-Safe Processing**: Multi-threaded architecture with thread-safe queues
- **Pattern Detection**: Configurable pattern matching for alert generation
- **Severity-Based Alerting**: Alert on log entries based on severity levels
- **Configurable**: Easy-to-use configuration file system
- **Production-Ready**: Includes error handling, Google Test integration, and proper resource management

## Project Structure

```
log_aggregator/
├── CMakeLists.txt          # CMake build configuration
├── README.md               # This file
├── include/                # Header files
│   ├── log_entry.h        # Log entry data structure
│   ├── queue.h            # Thread-safe queue
│   ├── config.h           # Configuration management
│   ├── log_source.h       # File and network log sources
│   ├── processor.h        # Log processing and pattern detection
│   └── alerter.h          # Alert generation
├── src/                    # Source files
│   ├── main.c             # Main program
│   ├── log_entry.c
│   ├── queue.c
│   ├── config.c
│   ├── log_source.c
│   ├── processor.c
│   └── alerter.c
├── tests/                  # Unit tests
│   ├── test_main.c
│   ├── test_log_entry.c
│   ├── test_queue.c
│   ├── test_config.c
│   └── test_queue_gtest.cpp  # Google Test for queue invariants
├── logs/                   # Example log files (pre-created for testing)
│   ├── app.log            # Application logs with various severity levels
│   └── access.log         # Web server access logs
├── config.txt             # Configuration file
└── examples/               # Example helper scripts
    ├── create_sample_logs.sh  # Script to create sample log files
    └── send_log.py            # Python script to send logs via network
```

## Building and Running

### Prerequisites

- CMake (version 3.15 or higher)

### Step-by-Step Instructions

**Step 1: Build the project**
```bash
mkdir -p build
cd build
cmake ..
cmake --build .
cd ..
```

**Step 2: Run the program**
```bash
./build/log_aggregator config.txt
```

**Step 3: View the output**
The program will immediately read the pre-created example log files in the `logs/` directory and display alerts. You should see output like:

```
Log Aggregator Starting...
Loading configuration from: config.txt
Monitoring directory: logs
Log Aggregator running. Press Ctrl+C to stop.

[ALERT] [2025-12-03 XX:XX:XX] [WARNING] [logs/app.log] Configuration file not found, using defaults
[ALERT] [2025-12-03 XX:XX:XX] [WARNING] [logs/access.log] GET /missing.html 404
[ALERT] [2025-12-03 XX:XX:XX] [ERROR] [logs/access.log] POST /api/data 500
[ALERT] [2025-12-03 XX:XX:XX] [ERROR] [logs/app.log] Failed to process user request
[ALERT] [2025-12-03 XX:XX:XX] [CRITICAL] [logs/app.log] System out of memory
```

### Configuration

Edit `config.txt` to customize behavior:

- `poll_interval`: How often to check directories (seconds)
- `watch_directory0`, `watch_directory1`, etc.: Directories to monitor
- `network_port`: Port for network log reception
- `enable_network`: Enable/disable network log source
- `queue_max_size`: Maximum queue size (0 for unlimited)
- `num_processing_threads`: Number of processing threads
- `enable_alerts`: Enable/disable alerting
- `alert_file`: File to write alerts to
- `alert_threshold`: Minimum log level to alert on (DEBUG, INFO, WARNING, ERROR, CRITICAL)
- `alert_pattern0`, `alert_pattern1`, etc.: Patterns to match for alerts

### Log Format

The log aggregator accepts logs in the following format:

```
[LEVEL] message content
```

Example:
```
[INFO] Application started
[WARNING] High memory usage detected
[ERROR] Failed to connect to database
[CRITICAL] System shutdown required
```

If no level is specified, the entry defaults to INFO level.

### Google Test (C++)

The project includes Google Test (gtest) tests for queue invariants. CMake will automatically detect and build gtest if it's installed.

**To install Google Test:**

On macOS:
```bash
brew install googletest
```

On Ubuntu/Debian:
```bash
sudo apt-get install libgtest-dev
cd /usr/src/gtest
sudo cmake .
sudo cmake --build .
sudo cp lib/*.a /usr/lib
```

**To run gtest:**
```bash
cd build
cmake ..  # Reconfigure to detect gtest
cmake --build .
./test_queue_gtest
```

The gtest tests verify the queue head invariant (that `queue->head != NULL` when `queue->size > 0`).

## Design Decisions

### Design Process

This project went through an iterative design process:

1. **Initial Design**: Started with a simple monolithic approach where all functionality was in one file
2. **Refactoring**: Separated concerns into distinct modules (queue, config, log sources, processor, alerter)
3. **Architecture Choice**: Chose pipeline architecture for clear data flow and easy extension
4. **Thread Safety**: Added thread-safe queues to enable concurrent processing
5. **Error Handling**: Implemented comprehensive error handling with proper cleanup paths

### Architecture

The system uses a pipeline architecture:
1. **Log Sources** (file monitors, network server) → Input Queue
2. **Processor** (pattern detection, filtering) → Alert Queue
3. **Alerter** (alert generation and output)

**Why Pipeline Architecture?**
- Clear separation of concerns
- Easy to extend (add new log sources or processors)
- Enables concurrent processing with thread-safe queues
- Each component can be tested independently

### Thread Safety

- All queues use mutexes and condition variables for thread-safe operations
- Each component runs in its own thread(s)
- Proper synchronization ensures no data races

### Error Handling

- All functions return error codes
- Input validation and sanity checks throughout
- Graceful error handling with proper cleanup
- Google Test integration for invariant validation (replaces assertions)

### Memory Management

- All allocated memory is properly freed
- No memory leaks (verified with valgrind)
- String operations use `strdup` for safe copying

## Contributors

Tatiana Quinn, Alesia Dako, Kohana Rakipi & Viktoriia Stepanenko