#ifndef DRONE_SCHEDULER_H
#define DRONE_SCHEDULER_H

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <time.h>

#define MAX_DRONES 50
#define MAX_TASKS 200
#define MAX_CHARGING_STATIONS 3
#define MAX_LOADING_BAYS 5
#define BATTERY_LOW_THRESHOLD 20
#define BATTERY_DRAIN_RATE 5
#define BATTERY_CHARGE_RATE 10
#define MAX_LOCATION_LEN 50

typedef enum {
    DRONE_IDLE,
    DRONE_LOADING,
    DRONE_DELIVERING,
    DRONE_CHARGING,
    DRONE_PREEMPTED
} DroneState;

typedef enum {
    TASK_PENDING,
    TASK_ASSIGNED,
    TASK_IN_PROGRESS,
    TASK_COMPLETED,
    TASK_PREEMPTED
} TaskState;

typedef struct {
    int task_id;
    char source[MAX_LOCATION_LEN];
    char destination[MAX_LOCATION_LEN];
    int priority;
    int estimated_time;
    TaskState state;
    int assigned_drone;
    time_t start_time;
    time_t end_time;
} Task;

typedef struct {
    int drone_id;
    DroneState state;
    int battery_level;
    int speed;
    Task *current_task;
    pthread_t thread;
    bool active;
    int tasks_completed;
    int preempted_count;
} Drone;

typedef struct {
    Task *tasks[MAX_TASKS];
    int size;
    pthread_mutex_t mutex;
} PriorityQueue;

typedef struct {
    int total_tasks;
    int completed_tasks;
    int total_preemptions;
    double total_delivery_time;
    int charging_station_uses;
    int loading_bay_uses;
    pthread_mutex_t mutex;
} Statistics;

typedef struct {
    Drone drones[MAX_DRONES];
    int num_drones;
    PriorityQueue task_queue;
    Statistics stats;
    sem_t charging_stations;
    sem_t loading_bays;
    pthread_mutex_t log_mutex;
    pthread_t scheduler_thread;
    bool simulation_running;
    int num_charging_stations;
    int num_loading_bays;
} Simulation;

void init_simulation(Simulation *sim, int num_drones, int num_charging, int num_loading);
void add_task(Simulation *sim, const char *source, const char *dest, int priority, int est_time);
void add_drone(Simulation *sim, int speed, int battery);
void start_simulation(Simulation *sim);
void stop_simulation(Simulation *sim);
void print_statistics(Simulation *sim);
void *drone_thread_func(void *arg);
void *scheduler_thread_func(void *arg);

void pq_init(PriorityQueue *pq);
void pq_push(PriorityQueue *pq, Task *task);
Task *pq_pop(PriorityQueue *pq);
Task *pq_peek(PriorityQueue *pq);
bool pq_is_empty(PriorityQueue *pq);
void pq_destroy(PriorityQueue *pq);

void log_event(Simulation *sim, const char *format, ...);

#endif
