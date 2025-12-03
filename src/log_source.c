#include "log_source.h"
#include "log_entry.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>
#include <sys/select.h>
#include <pthread.h>

// Forward declarations for thread functions
static void* file_monitor_thread_func(void* arg);
static void* network_server_thread_func(void* arg);

// Helper function to read new lines from a file
static void read_new_lines(const char* filepath, FILE* file_handle, 
                          log_queue_t* queue, off_t* last_position) {
    struct stat st;
    if (stat(filepath, &st) != 0) {
        return;
    }
    
    // If file was truncated, reset position
    if (st.st_size < *last_position) {
        *last_position = 0;
        fseek(file_handle, 0, SEEK_SET);
    }
    
    char line[4096];
    while (fgets(line, sizeof(line), file_handle)) {
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        // Parse log entry (simple format: [LEVEL] message)
        log_level_t level = LOG_LEVEL_INFO;
        char* message = line;
        
        if (line[0] == '[') {
            char* end_bracket = strchr(line, ']');
            if (end_bracket) {
                *end_bracket = '\0';
                level = log_entry_parse_level(line + 1);
                message = end_bracket + 1;
                while (*message == ' ') message++; // Skip spaces
            }
        }
        
        log_entry_t* entry = log_entry_create(filepath, message, level, line);
        if (entry) {
            queue_enqueue(queue, entry);
        }
    }
    
    *last_position = ftell(file_handle);
}

int file_monitor_init(file_monitor_t* monitor, const char* directory, 
                      int poll_interval, log_queue_t* queue) {
    if (!monitor || !directory || !queue) {
        return -1;
    }
    
    monitor->directory = strdup(directory);
    monitor->poll_interval = poll_interval;
    monitor->queue = queue;
    monitor->running = false;
    
    if (!monitor->directory) {
        return -1;
    }
    
    return 0;
}

static void* file_monitor_thread_func(void* arg) {
    file_monitor_t* monitor = (file_monitor_t*)arg;
    
    // Track file handles and positions
    typedef struct {
        char* filepath;
        FILE* handle;
        off_t last_position;
    } file_tracker_t;
    
    file_tracker_t* files = NULL;
    size_t num_files = 0;
    size_t capacity = 16;
    files = (file_tracker_t*)calloc(capacity, sizeof(file_tracker_t));
    
    while (monitor->running) {
        // Scan directory for log files
        DIR* dir = opendir(monitor->directory);
        if (dir) {
            struct dirent* entry;
            while ((entry = readdir(dir)) != NULL) {
                // Skip . and ..
                if (entry->d_name[0] == '.') {
                    continue;
                }
                
                // Check if it's a regular file (simple check)
                char filepath[1024];
                snprintf(filepath, sizeof(filepath), "%s/%s", 
                        monitor->directory, entry->d_name);
                
                struct stat st;
                if (stat(filepath, &st) == 0 && S_ISREG(st.st_mode)) {
                    // Check if we're already tracking this file
                    size_t i;
                    for (i = 0; i < num_files; i++) {
                        if (strcmp(files[i].filepath, filepath) == 0) {
                            break;
                        }
                    }
                    
                    if (i == num_files) {
                        // New file, add it
                        if (num_files >= capacity) {
                            capacity *= 2;
                            file_tracker_t* new_files = (file_tracker_t*)realloc(files, 
                                    capacity * sizeof(file_tracker_t));
                            if (!new_files) {
                                // Realloc failed, skip this file
                                continue;
                            }
                            files = new_files;
                        }
                        files[num_files].filepath = strdup(filepath);
                        if (!files[num_files].filepath) {
                            // Memory allocation failed, skip this file
                            continue;
                        }
                        files[num_files].handle = fopen(filepath, "r");
                        files[num_files].last_position = 0;
                        if (files[num_files].handle) {
                            // Read existing content first
                            fseek(files[num_files].handle, 0, SEEK_SET); // Start from beginning
                            read_new_lines(filepath, files[num_files].handle,
                                         monitor->queue, &files[num_files].last_position);
                            // Now seek to end so we only read new content in future polls
                            fseek(files[num_files].handle, 0, SEEK_END);
                            files[num_files].last_position = ftell(files[num_files].handle);
                            num_files++;
                        } else {
                            free(files[num_files].filepath);
                        }
                    } else {
                        // Existing file, read new lines
                        if (files[i].handle) {
                            read_new_lines(files[i].filepath, files[i].handle,
                                         monitor->queue, &files[i].last_position);
                        }
                    }
                }
            }
            closedir(dir);
        }
        
        sleep(monitor->poll_interval);
    }
    
    // Cleanup
    for (size_t i = 0; i < num_files; i++) {
        if (files[i].handle) {
            fclose(files[i].handle);
        }
        free(files[i].filepath);
    }
    free(files);
    
    return NULL;
}

int file_monitor_start(file_monitor_t* monitor) {
    if (!monitor || monitor->running) {
        return -1;
    }
    
    monitor->running = true;
    if (pthread_create(&monitor->thread, NULL, file_monitor_thread_func, monitor) != 0) {
        monitor->running = false;
        return -1;
    }
    
    return 0;
}

void file_monitor_stop(file_monitor_t* monitor) {
    if (!monitor) {
        return;
    }
    
    monitor->running = false;
    pthread_join(monitor->thread, NULL);
}

void file_monitor_destroy(file_monitor_t* monitor) {
    if (!monitor) {
        return;
    }
    
    file_monitor_stop(monitor);
    free(monitor->directory);
}

int network_server_init(network_server_t* server, int port, log_queue_t* queue) {
    if (!server || !queue) {
        return -1;
    }
    
    server->port = port;
    server->queue = queue;
    server->running = false;
    server->server_fd = -1;
    
    return 0;
}

static void* network_server_thread_func(void* arg) {
    network_server_t* server = (network_server_t*)arg;
    
    // Create socket
    server->server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->server_fd < 0) {
        perror("Socket creation failed");
        return NULL;
    }
    
    // Set socket options
    int opt = 1;
    setsockopt(server->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Bind socket
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(server->port);
    
    if (bind(server->server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        close(server->server_fd);
        server->server_fd = -1;
        return NULL;
    }
    
    // Listen
    if (listen(server->server_fd, 5) < 0) {
        perror("Listen failed");
        close(server->server_fd);
        server->server_fd = -1;
        return NULL;
    }
    
    printf("Network server listening on port %d\n", server->port);
    
    while (server->running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        // Accept with timeout using select
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server->server_fd, &read_fds);
        
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        int select_result = select(server->server_fd + 1, &read_fds, NULL, NULL, &timeout);
        
        if (select_result > 0 && FD_ISSET(server->server_fd, &read_fds)) {
            int client_fd = accept(server->server_fd, (struct sockaddr*)&client_addr, &client_len);
            if (client_fd >= 0) {
                char client_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
                char source[256];
                snprintf(source, sizeof(source), "network:%s:%d", client_ip, ntohs(client_addr.sin_port));
                
                // Read log lines from client
                char buffer[4096];
                ssize_t bytes_read;
                while ((bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0)) > 0) {
                    buffer[bytes_read] = '\0';
                    
                    // Process each line
                    char* line = buffer;
                    char* newline;
                    while ((newline = strchr(line, '\n')) != NULL) {
                        *newline = '\0';
                        
                        if (strlen(line) > 0) {
                            log_level_t level = LOG_LEVEL_INFO;
                            char* message = line;
                            
                            // Parse [LEVEL] format
                            if (line[0] == '[') {
                                char* end_bracket = strchr(line, ']');
                                if (end_bracket) {
                                    *end_bracket = '\0';
                                    level = log_entry_parse_level(line + 1);
                                    message = end_bracket + 1;
                                    while (*message == ' ') message++;
                                }
                            }
                            
                            log_entry_t* entry = log_entry_create(source, message, level, line);
                            if (entry) {
                                queue_enqueue(server->queue, entry);
                            }
                        }
                        
                        line = newline + 1;
                    }
                }
                
                close(client_fd);
            }
        }
    }
    
    if (server->server_fd >= 0) {
        close(server->server_fd);
        server->server_fd = -1;
    }
    
    return NULL;
}

int network_server_start(network_server_t* server) {
    if (!server || server->running) {
        return -1;
    }
    
    server->running = true;
    if (pthread_create(&server->thread, NULL, network_server_thread_func, server) != 0) {
        server->running = false;
        return -1;
    }
    
    return 0;
}

void network_server_stop(network_server_t* server) {
    if (!server) {
        return;
    }
    
    server->running = false;
    if (server->thread) {
        pthread_join(server->thread, NULL);
    }
}

void network_server_destroy(network_server_t* server) {
    if (!server) {
        return;
    }
    
    network_server_stop(server);
}