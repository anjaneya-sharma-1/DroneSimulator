#include "drone_scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

static Simulation *global_sim = NULL;

void log_event(Simulation *sim, const char *format, ...) {
    pthread_mutex_lock(&sim->log_mutex);
    
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    printf("[%02d:%02d:%02d] ", t->tm_hour, t->tm_min, t->tm_sec);
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    printf("\n");
    fflush(stdout);
    
    pthread_mutex_unlock(&sim->log_mutex);
}

void pq_init(PriorityQueue *pq) {
    pq->size = 0;
    pthread_mutex_init(&pq->mutex, NULL);
}

void pq_push(PriorityQueue *pq, Task *task) {
    pthread_mutex_lock(&pq->mutex);
    
    if (pq->size >= MAX_TASKS) {
        pthread_mutex_unlock(&pq->mutex);
        return;
    }
    
    int i = pq->size;
    pq->tasks[i] = task;
    pq->size++;
    
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (pq->tasks[i]->priority < pq->tasks[parent]->priority) {
            Task *temp = pq->tasks[i];
            pq->tasks[i] = pq->tasks[parent];
            pq->tasks[parent] = temp;
            i = parent;
        } else {
            break;
        }
    }
    
    pthread_mutex_unlock(&pq->mutex);
}

Task *pq_pop(PriorityQueue *pq) {
    pthread_mutex_lock(&pq->mutex);
    
    if (pq->size == 0) {
        pthread_mutex_unlock(&pq->mutex);
        return NULL;
    }
    
    Task *result = pq->tasks[0];
    pq->size--;
    
    if (pq->size > 0) {
        pq->tasks[0] = pq->tasks[pq->size];
        
        int i = 0;
        while (true) {
            int left = 2 * i + 1;
            int right = 2 * i + 2;
            int smallest = i;
            
            if (left < pq->size && pq->tasks[left]->priority < pq->tasks[smallest]->priority) {
                smallest = left;
            }
            if (right < pq->size && pq->tasks[right]->priority < pq->tasks[smallest]->priority) {
                smallest = right;
            }
            
            if (smallest != i) {
                Task *temp = pq->tasks[i];
                pq->tasks[i] = pq->tasks[smallest];
                pq->tasks[smallest] = temp;
                i = smallest;
            } else {
                break;
            }
        }
    }
    
    pthread_mutex_unlock(&pq->mutex);
    return result;
}

Task *pq_peek(PriorityQueue *pq) {
    pthread_mutex_lock(&pq->mutex);
    Task *result = (pq->size > 0) ? pq->tasks[0] : NULL;
    pthread_mutex_unlock(&pq->mutex);
    return result;
}

bool pq_is_empty(PriorityQueue *pq) {
    pthread_mutex_lock(&pq->mutex);
    bool empty = (pq->size == 0);
    pthread_mutex_unlock(&pq->mutex);
    return empty;
}

void pq_destroy(PriorityQueue *pq) {
    pthread_mutex_destroy(&pq->mutex);
}

void *drone_thread_func(void *arg) {
    Drone *drone = (Drone *)arg;
    Simulation *sim = global_sim;
    
    log_event(sim, ANSI_COLOR_CYAN "[Drone %d] Thread started (Speed: %d, Battery: %d%%)" ANSI_COLOR_RESET,
              drone->drone_id, drone->speed, drone->battery_level);
    
    while (sim->simulation_running && drone->active) {
        if (drone->current_task != NULL && drone->state != DRONE_PREEMPTED) {
            Task *task = drone->current_task;
            
            if (drone->state == DRONE_IDLE || drone->state == DRONE_LOADING) {
                log_event(sim, ANSI_COLOR_BLUE "[Drone %d] Waiting for loading bay..." ANSI_COLOR_RESET, 
                         drone->drone_id);
                sem_wait(&sim->loading_bays);
                
                pthread_mutex_lock(&sim->stats.mutex);
                sim->stats.loading_bay_uses++;
                pthread_mutex_unlock(&sim->stats.mutex);
                
                drone->state = DRONE_LOADING;
                const char *priority_color = (task->priority == 1) ? ANSI_COLOR_RED : 
                                             (task->priority == 2) ? ANSI_COLOR_YELLOW : ANSI_COLOR_GREEN;
                log_event(sim, "%s[Drone %d] Acquired loading bay - Loading task T%d (Priority %d: %s -> %s)" ANSI_COLOR_RESET,
                         priority_color, drone->drone_id, task->task_id, task->priority, 
                         task->source, task->destination);
                
                sleep(1);
                sem_post(&sim->loading_bays);
                log_event(sim, "[Drone %d] Released loading bay", drone->drone_id);
                
                drone->state = DRONE_DELIVERING;
                task->state = TASK_IN_PROGRESS;
                task->start_time = time(NULL);
            }
            
            if (drone->state == DRONE_DELIVERING && drone->battery_level > BATTERY_LOW_THRESHOLD) {
                int delivery_time = task->estimated_time / drone->speed;
                if (delivery_time < 1) delivery_time = 1;
                
                for (int i = 0; i < delivery_time && drone->state == DRONE_DELIVERING; i++) {
                    sleep(1);
                    drone->battery_level -= BATTERY_DRAIN_RATE;
                    
                    if (drone->battery_level <= BATTERY_LOW_THRESHOLD) {
                        log_event(sim, ANSI_COLOR_YELLOW "[Drone %d] Battery critical (%d%%), must charge!" ANSI_COLOR_RESET,
                                 drone->drone_id, drone->battery_level);
                        break;
                    }
                }
                
                if (drone->state == DRONE_DELIVERING) {
                    task->state = TASK_COMPLETED;
                    task->end_time = time(NULL);
                    double elapsed = difftime(task->end_time, task->start_time);
                    
                    pthread_mutex_lock(&sim->stats.mutex);
                    sim->stats.completed_tasks++;
                    sim->stats.total_delivery_time += elapsed;
                    pthread_mutex_unlock(&sim->stats.mutex);
                    
                    log_event(sim, ANSI_COLOR_GREEN "[Drone %d] ✓ Completed task T%d (%.0f seconds, Battery: %d%%)" ANSI_COLOR_RESET,
                             drone->drone_id, task->task_id, elapsed, drone->battery_level);
                    
                    drone->tasks_completed++;
                    drone->current_task = NULL;
                    drone->state = DRONE_IDLE;
                }
            }
        }
        
        if (drone->battery_level <= BATTERY_LOW_THRESHOLD && drone->state != DRONE_CHARGING) {
            if (drone->current_task != NULL && drone->state != DRONE_PREEMPTED) {
                log_event(sim, ANSI_COLOR_MAGENTA "[Scheduler] Pausing Drone %d task T%d due to low battery" ANSI_COLOR_RESET,
                         drone->drone_id, drone->current_task->task_id);
                drone->current_task->state = TASK_PENDING;
                pq_push(&sim->task_queue, drone->current_task);
                drone->current_task = NULL;
            }
            
            log_event(sim, "[Drone %d] Requesting charging station...", drone->drone_id);
            sem_wait(&sim->charging_stations);
            
            pthread_mutex_lock(&sim->stats.mutex);
            sim->stats.charging_station_uses++;
            pthread_mutex_unlock(&sim->stats.mutex);
            
            drone->state = DRONE_CHARGING;
            log_event(sim, ANSI_COLOR_YELLOW "[Drone %d] Acquired charging station (Battery: %d%%)" ANSI_COLOR_RESET,
                     drone->drone_id, drone->battery_level);
            
            while (drone->battery_level < 100) {
                sleep(1);
                drone->battery_level += BATTERY_CHARGE_RATE;
                if (drone->battery_level > 100) drone->battery_level = 100;
            }
            
            log_event(sim, ANSI_COLOR_GREEN "[Drone %d] Fully charged (100%%), releasing charging station" ANSI_COLOR_RESET,
                     drone->drone_id);
            sem_post(&sim->charging_stations);
            drone->state = DRONE_IDLE;
        }
        
        if (drone->state == DRONE_IDLE && drone->current_task == NULL) {
            sleep(1);
        }
    }
    
    log_event(sim, "[Drone %d] Thread terminated", drone->drone_id);
    return NULL;
}

void *scheduler_thread_func(void *arg) {
    Simulation *sim = (Simulation *)arg;
    
    log_event(sim, ANSI_COLOR_MAGENTA "══════════════════════════════════════" ANSI_COLOR_RESET);
    log_event(sim, ANSI_COLOR_MAGENTA "  SCHEDULER THREAD STARTED" ANSI_COLOR_RESET);
    log_event(sim, ANSI_COLOR_MAGENTA "══════════════════════════════════════" ANSI_COLOR_RESET);
    
    while (sim->simulation_running) {
        if (!pq_is_empty(&sim->task_queue)) {
            Task *highest_priority_task = pq_peek(&sim->task_queue);
            
            if (highest_priority_task != NULL) {
                Drone *best_drone = NULL;
                
                for (int i = 0; i < sim->num_drones; i++) {
                    if (sim->drones[i].active && sim->drones[i].battery_level > BATTERY_LOW_THRESHOLD) {
                        if (sim->drones[i].state == DRONE_IDLE && sim->drones[i].current_task == NULL) {
                            if (best_drone == NULL || sim->drones[i].battery_level > best_drone->battery_level) {
                                best_drone = &sim->drones[i];
                            }
                        }
                    }
                }
                
                if (highest_priority_task->priority == 1) {
                    for (int i = 0; i < sim->num_drones; i++) {
                        if (sim->drones[i].active && 
                            sim->drones[i].current_task != NULL && 
                            sim->drones[i].current_task->priority > 1 &&
                            sim->drones[i].battery_level > BATTERY_LOW_THRESHOLD) {
                            
                            log_event(sim, ANSI_COLOR_RED "[Scheduler] ⚠ PREEMPTION: Interrupting Drone %d (Task T%d, Priority %d) for urgent task T%d" ANSI_COLOR_RESET,
                                     sim->drones[i].drone_id, 
                                     sim->drones[i].current_task->task_id,
                                     sim->drones[i].current_task->priority,
                                     highest_priority_task->task_id);
                            
                            sim->drones[i].current_task->state = TASK_PREEMPTED;
                            pq_push(&sim->task_queue, sim->drones[i].current_task);
                            sim->drones[i].current_task = NULL;
                            sim->drones[i].state = DRONE_PREEMPTED;
                            sim->drones[i].preempted_count++;
                            
                            pthread_mutex_lock(&sim->stats.mutex);
                            sim->stats.total_preemptions++;
                            pthread_mutex_unlock(&sim->stats.mutex);
                            
                            best_drone = &sim->drones[i];
                            break;
                        }
                    }
                }
                
                if (best_drone != NULL) {
                    Task *task = pq_pop(&sim->task_queue);
                    if (task != NULL) {
                        task->state = TASK_ASSIGNED;
                        task->assigned_drone = best_drone->drone_id;
                        best_drone->current_task = task;
                        best_drone->state = DRONE_IDLE;
                        
                        const char *priority_color = (task->priority == 1) ? ANSI_COLOR_RED : 
                                                     (task->priority == 2) ? ANSI_COLOR_YELLOW : ANSI_COLOR_GREEN;
                        log_event(sim, "%s[Scheduler] Assigned task T%d (Priority %d) to Drone %d" ANSI_COLOR_RESET,
                                 priority_color, task->task_id, task->priority, best_drone->drone_id);
                    }
                }
            }
        }
        
        sleep(1);
    }
    
    log_event(sim, ANSI_COLOR_MAGENTA "[Scheduler] Thread terminated" ANSI_COLOR_RESET);
    return NULL;
}

void init_simulation(Simulation *sim, int num_drones, int num_charging, int num_loading) {
    global_sim = sim;
    sim->num_drones = 0;
    sim->simulation_running = false;
    sim->num_charging_stations = num_charging;
    sim->num_loading_bays = num_loading;
    
    pq_init(&sim->task_queue);
    
    pthread_mutex_init(&sim->stats.mutex, NULL);
    sim->stats.total_tasks = 0;
    sim->stats.completed_tasks = 0;
    sim->stats.total_preemptions = 0;
    sim->stats.total_delivery_time = 0.0;
    sim->stats.charging_station_uses = 0;
    sim->stats.loading_bay_uses = 0;
    
    sem_init(&sim->charging_stations, 0, num_charging);
    sem_init(&sim->loading_bays, 0, num_loading);
    pthread_mutex_init(&sim->log_mutex, NULL);
    
    printf("\n");
    log_event(sim, ANSI_COLOR_CYAN "╔════════════════════════════════════════════════════════╗" ANSI_COLOR_RESET);
    log_event(sim, ANSI_COLOR_CYAN "║   AUTONOMOUS DELIVERY DRONE SCHEDULER SIMULATION       ║" ANSI_COLOR_RESET);
    log_event(sim, ANSI_COLOR_CYAN "╚════════════════════════════════════════════════════════╝" ANSI_COLOR_RESET);
    log_event(sim, "Charging Stations: %d | Loading Bays: %d", num_charging, num_loading);
}

void add_drone(Simulation *sim, int speed, int battery) {
    if (sim->num_drones >= MAX_DRONES) {
        log_event(sim, "Cannot add more drones (max: %d)", MAX_DRONES);
        return;
    }
    
    Drone *drone = &sim->drones[sim->num_drones];
    drone->drone_id = sim->num_drones + 1;
    drone->state = DRONE_IDLE;
    drone->battery_level = battery;
    drone->speed = speed;
    drone->current_task = NULL;
    drone->active = true;
    drone->tasks_completed = 0;
    drone->preempted_count = 0;
    
    sim->num_drones++;
    log_event(sim, ANSI_COLOR_GREEN "[Setup] Added Drone %d (Speed: %d, Battery: %d%%)" ANSI_COLOR_RESET,
             drone->drone_id, speed, battery);
}

void add_task(Simulation *sim, const char *source, const char *dest, int priority, int est_time) {
    Task *task = (Task *)malloc(sizeof(Task));
    task->task_id = sim->stats.total_tasks + 1;
    strncpy(task->source, source, MAX_LOCATION_LEN - 1);
    strncpy(task->destination, dest, MAX_LOCATION_LEN - 1);
    task->priority = priority;
    task->estimated_time = est_time;
    task->state = TASK_PENDING;
    task->assigned_drone = -1;
    
    pq_push(&sim->task_queue, task);
    
    pthread_mutex_lock(&sim->stats.mutex);
    sim->stats.total_tasks++;
    pthread_mutex_unlock(&sim->stats.mutex);
    
    const char *priority_color = (priority == 1) ? ANSI_COLOR_RED : 
                                 (priority == 2) ? ANSI_COLOR_YELLOW : ANSI_COLOR_GREEN;
    log_event(sim, "%s[Setup] Added task T%d: %s → %s (Priority: %d, Est. Time: %ds)" ANSI_COLOR_RESET,
             priority_color, task->task_id, source, dest, priority, est_time);
}

void start_simulation(Simulation *sim) {
    sim->simulation_running = true;
    
    log_event(sim, "\n" ANSI_COLOR_MAGENTA "════════════ STARTING SIMULATION ════════════" ANSI_COLOR_RESET);
    
    for (int i = 0; i < sim->num_drones; i++) {
        pthread_create(&sim->drones[i].thread, NULL, drone_thread_func, &sim->drones[i]);
    }
    
    pthread_create(&sim->scheduler_thread, NULL, scheduler_thread_func, sim);
}

void stop_simulation(Simulation *sim) {
    log_event(sim, "\n" ANSI_COLOR_YELLOW "════════════ STOPPING SIMULATION ════════════" ANSI_COLOR_RESET);
    sim->simulation_running = false;
    
    pthread_join(sim->scheduler_thread, NULL);
    
    for (int i = 0; i < sim->num_drones; i++) {
        pthread_join(sim->drones[i].thread, NULL);
    }
    
    sem_destroy(&sim->charging_stations);
    sem_destroy(&sim->loading_bays);
    pthread_mutex_destroy(&sim->log_mutex);
    pthread_mutex_destroy(&sim->stats.mutex);
    pq_destroy(&sim->task_queue);
}

void print_statistics(Simulation *sim) {
    printf("\n");
    log_event(sim, ANSI_COLOR_CYAN "╔════════════════════════════════════════════════════════╗" ANSI_COLOR_RESET);
    log_event(sim, ANSI_COLOR_CYAN "║              SIMULATION STATISTICS                      ║" ANSI_COLOR_RESET);
    log_event(sim, ANSI_COLOR_CYAN "╚════════════════════════════════════════════════════════╝" ANSI_COLOR_RESET);
    log_event(sim, "Total Tasks: %d", sim->stats.total_tasks);
    log_event(sim, ANSI_COLOR_GREEN "Completed Tasks: %d" ANSI_COLOR_RESET, sim->stats.completed_tasks);
    log_event(sim, ANSI_COLOR_RED "Total Preemptions: %d" ANSI_COLOR_RESET, sim->stats.total_preemptions);
    
    if (sim->stats.completed_tasks > 0) {
        double avg_time = sim->stats.total_delivery_time / sim->stats.completed_tasks;
        log_event(sim, "Average Delivery Time: %.2f seconds", avg_time);
    }
    
    log_event(sim, "Charging Station Uses: %d", sim->stats.charging_station_uses);
    log_event(sim, "Loading Bay Uses: %d", sim->stats.loading_bay_uses);
    
    if (sim->num_charging_stations > 0 && sim->stats.charging_station_uses > 0) {
        double charging_util = (sim->stats.charging_station_uses * 100.0) / (sim->num_charging_stations * sim->stats.total_tasks);
        log_event(sim, "Charging Station Utilization: %.2f%%", charging_util);
    }
    
    printf("\n");
    log_event(sim, ANSI_COLOR_CYAN "─────────────── DRONE SUMMARY ───────────────" ANSI_COLOR_RESET);
    for (int i = 0; i < sim->num_drones; i++) {
        log_event(sim, "Drone %d: %d tasks completed, %d preemptions, Battery: %d%%",
                 sim->drones[i].drone_id, sim->drones[i].tasks_completed, 
                 sim->drones[i].preempted_count, sim->drones[i].battery_level);
    }
    printf("\n");
}
