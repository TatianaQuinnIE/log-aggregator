#!/bin/bash
# Script to create sample log files for testing

mkdir -p logs

# Create sample log file 1
cat > logs/app.log << EOF
[INFO] Application started
[INFO] Loading configuration
[WARNING] Configuration file not found, using defaults
[INFO] Database connection established
[ERROR] Failed to process user request
[INFO] Request completed
[CRITICAL] System out of memory
EOF

# Create sample log file 2
cat > logs/access.log << EOF
[INFO] GET /index.html 200
[INFO] GET /about.html 200
[WARNING] GET /missing.html 404
[INFO] POST /api/login 200
[ERROR] POST /api/data 500
[INFO] GET /static/style.css 200
EOF

echo "Sample log files created in logs/ directory"