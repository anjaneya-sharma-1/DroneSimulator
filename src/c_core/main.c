#include "drone_scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void print_usage(const char *program_name) {
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  --drones <count>        Number of drones (default: use stdin config)\n");
    printf("  --charging <count>      Number of charging stations (default: 3)\n");
    printf("  --loading <count>       Number of loading bays (default: 5)\n");
    printf("  --duration <seconds>    Simulation duration (default: 30)\n");
    printf("  --config stdin          Read drone and task configuration from stdin\n");
    printf("  --help                  Show this help message\n");
}

int main(int argc, char *argv[]) {
    int num_drones = 0;
    int num_charging = 3;
    int num_loading = 5;
    int duration = 30;
    bool use_stdin_config = false;
    bool use_legacy_mode = false;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--drones") == 0 && i + 1 < argc) {
            num_drones = atoi(argv[++i]);
            use_legacy_mode = true;
            if (num_drones < 1 || num_drones > MAX_DRONES) {
                fprintf(stderr, "Error: Number of drones must be between 1 and %d\n", MAX_DRONES);
                return 1;
            }
        } else if (strcmp(argv[i], "--charging") == 0 && i + 1 < argc) {
            num_charging = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--loading") == 0 && i + 1 < argc) {
            num_loading = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--duration") == 0 && i + 1 < argc) {
            duration = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
            i++;
            if (strcmp(argv[i], "stdin") == 0) {
                use_stdin_config = true;
            }
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }
    
    Simulation sim;
    init_simulation(&sim, 0, num_charging, num_loading);
    
    if (use_stdin_config) {
        char line[512];
        int line_num = 0;
        while (fgets(line, sizeof(line), stdin)) {
            line_num++;
            line[strcspn(line, "\n")] = 0;
            
            if (strlen(line) == 0 || line[0] == '#') {
                continue;
            }
            
            if (strncmp(line, "DRONE", 5) == 0) {
                int speed, battery;
                if (sscanf(line, "DRONE %d %d", &speed, &battery) == 2) {
                    if (speed >= 1 && speed <= 3 && battery >= 20 && battery <= 100) {
                        add_drone(&sim, speed, battery);
                    } else {
                        fprintf(stderr, "Warning: Invalid drone config at line %d (speed 1-3, battery 20-100)\n", line_num);
                    }
                } else {
                    fprintf(stderr, "Warning: Malformed DRONE line %d\n", line_num);
                }
            } else if (strncmp(line, "TASK", 4) == 0) {
                char *tokens[100];
                int token_count = 0;
                
                char line_copy[512];
                strncpy(line_copy, line + 5, sizeof(line_copy) - 1);
                
                char *token = strtok(line_copy, " ");
                while (token != NULL && token_count < 100) {
                    tokens[token_count++] = token;
                    token = strtok(NULL, " ");
                }
                
                if (token_count < 4) {
                    fprintf(stderr, "Warning: Malformed TASK line %d (need: warehouse customer priority time)\n", line_num);
                    continue;
                }
                
                char warehouse[MAX_LOCATION_LEN];
                char customer[MAX_LOCATION_LEN];
                int priority, est_time;
                
                strncpy(warehouse, tokens[0], MAX_LOCATION_LEN - 1);
                warehouse[MAX_LOCATION_LEN - 1] = '\0';
                
                priority = atoi(tokens[token_count - 2]);
                est_time = atoi(tokens[token_count - 1]);
                
                customer[0] = '\0';
                for (int i = 1; i < token_count - 2; i++) {
                    if (i > 1) strcat(customer, " ");
                    strncat(customer, tokens[i], MAX_LOCATION_LEN - strlen(customer) - 1);
                }
                
                if (priority >= 1 && priority <= 3 && est_time >= 1 && est_time <= 100) {
                    char source[MAX_LOCATION_LEN];
                    snprintf(source, MAX_LOCATION_LEN, "Warehouse %s", warehouse);
                    add_task(&sim, source, customer, priority, est_time);
                } else {
                    fprintf(stderr, "Warning: Invalid task config at line %d (priority 1-3, time 1-100)\n", line_num);
                }
            } else if (strncmp(line, "START", 5) == 0) {
                break;
            }
        }
    } else if (use_legacy_mode) {
        int drone_speeds[] = {1, 2, 1, 2, 1};
        int drone_batteries[] = {100, 80, 90, 100, 75};
        
        for (int i = 0; i < num_drones && i < 5; i++) {
            add_drone(&sim, drone_speeds[i], drone_batteries[i]);
        }
        for (int i = 5; i < num_drones; i++) {
            add_drone(&sim, 1 + (i % 2), 80 + (i % 21));
        }
        
        add_task(&sim, "Warehouse A", "Customer 101", 2, 10);
        add_task(&sim, "Warehouse B", "Customer 102", 3, 8);
        add_task(&sim, "Warehouse A", "Customer 103", 1, 12);
        add_task(&sim, "Warehouse C", "Customer 104", 2, 15);
        add_task(&sim, "Warehouse B", "Customer 105", 3, 7);
    } else {
        fprintf(stderr, "Error: Either use --drones or --config stdin\n");
        print_usage(argv[0]);
        return 1;
    }
    
    if (sim.num_drones == 0) {
        fprintf(stderr, "Error: No drones configured. Please add at least one drone.\n");
        return 1;
    }
    
    pthread_mutex_lock(&sim.task_queue.mutex);
    int task_count = sim.task_queue.size;
    pthread_mutex_unlock(&sim.task_queue.mutex);
    
    if (task_count == 0) {
        fprintf(stderr, "Error: No tasks configured. Please add at least one task.\n");
        return 1;
    }
    
    start_simulation(&sim);
    sleep(duration);
    stop_simulation(&sim);
    print_statistics(&sim);
    
    return 0;
}
