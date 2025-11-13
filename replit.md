# Autonomous Delivery Drone Scheduler Simulation

## Project Overview

This is an operating systems educational project that simulates autonomous delivery drone scheduling using C with pthreads, mutexes, and semaphores. The simulation demonstrates priority scheduling, preemption, and resource synchronization through a real-world-inspired logistics scenario.

## Recent Changes (November 13, 2025)

- ✅ Initial project setup with C core and Node.js web interface
- ✅ Implemented complete pthread-based simulation with priority queue
- ✅ Added mutex locks and semaphores for resource synchronization
- ✅ Created priority scheduler with preemption support
- ✅ Implemented battery consumption and charging logic
- ✅ Built Express.js server to spawn C process and stream output
- ✅ Created real-time web dashboard with Server-Sent Events
- ✅ Added comprehensive color-coded logging system
- ✅ Implemented statistics tracking (avg delivery time, preemptions, resource utilization)
- ✅ **NEW: Dynamic drone configuration** - Add drones with custom speed (1-3x) and battery levels (20-100%)
- ✅ **NEW: Dynamic task configuration** - Add tasks with custom customer names, warehouse selection (A/B/C), and priority levels
- ✅ **NEW: Interactive dashboard** - Forms to add/remove drones and tasks before starting simulation
- ✅ **NEW: Robust input parsing** - Handles customer names with spaces properly
- ✅ **NEW: Backward compatibility** - Original CLI interface with --drones flag still works

## Project Architecture

### C Core (src/c_core/)
- **drone_scheduler.h** - Data structure definitions for Drone, Task, PriorityQueue, Statistics
- **drone_scheduler.c** - Core implementation with pthreads, mutexes, semaphores
- **main.c** - Entry point with stdin config parser and CLI argument handling
- **Makefile** - Compilation configuration

### Web Interface
- **src/web/server/index.js** - Express.js server that spawns C process and streams output
- **public/index.html** - Real-time visualization dashboard with dynamic configuration forms

## Key Features

1. **Dynamic Drone Configuration**: 
   - Add any number of drones (1-50)
   - Set custom speed (Slow/Medium/Fast: 1x-3x)
   - Set custom battery level (20-100%)
   - View all added drones in real-time list
   - Remove individual drones before starting

2. **Dynamic Task Configuration**:
   - Add unlimited tasks
   - Select warehouse (A, B, or C) - fixed at 3 warehouses
   - Enter custom customer names/IDs (supports spaces)
   - Choose priority (🔴 Urgent/🟡 Medium/🟢 Low: 1-3)
   - Set estimated delivery time
   - View all tasks with color-coded priorities
   - Remove individual tasks before starting

3. **Priority Scheduling**: Tasks prioritized 1-3 (1=urgent, 3=low)
4. **Preemption**: Urgent tasks interrupt lower-priority deliveries
5. **Resource Management**: Semaphores control access to charging stations and loading bays
6. **Battery Simulation**: Automatic charging when battery drops below 20%
7. **Real-time Logging**: Color-coded console output streamed to web interface
8. **Statistics**: Average delivery time, preemptions, resource utilization

## Technology Stack

- **C** with pthread.h and semaphore.h for core simulation
- **GCC** compiler for C code
- **Node.js 20** with Express.js for web server
- **Server-Sent Events (SSE)** for real-time updates
- **HTML/CSS/JavaScript** for frontend visualization

## User Preferences

- User wants full control over drone and task configuration via dashboard
- Supports custom customer names (not just random numbers)
- Fixed at 3 warehouses (A, B, C)
- Priority selection per task
- Battery percentage configuration per drone
- Educational focus on demonstrating OS scheduling principles

## How to Use

### Web Interface (Recommended):
1. Open web interface at http://localhost:5000

2. **Add Drones**:
   - Select drone speed (1x, 2x, or 3x)
   - Set battery level (20-100%)
   - Click "➕ Add Drone"
   - Repeat for each drone you want

3. **Add Tasks**:
   - Select warehouse (A, B, or C)
   - Enter customer name/ID (e.g., "Customer-101" or "Big Store Chain")
   - Select priority (🔴 Urgent, 🟡 Medium, or 🟢 Low)
   - Set estimated delivery time in seconds
   - Click "➕ Add Task"
   - Repeat for each delivery task

4. **Configure Resources**:
   - Set number of charging stations (default: 3)
   - Set number of loading bays (default: 5)
   - Set simulation duration in seconds

5. **Run Simulation**:
   - Click "▶️ Start" to begin
   - Watch real-time logs showing:
     - Drone operations
     - Task assignments (color-coded by priority)
     - Resource locking/unlocking
     - Preemptions when urgent tasks arrive
     - Battery charging events
   - View statistics when complete

### Command Line (Legacy):
```bash
cd src/c_core
./drone_scheduler --drones 5 --charging 2 --loading 3 --duration 60
```

## Configuration Format

The C program accepts configuration via stdin:
```
DRONE <speed> <battery>
TASK <warehouse> <customer_name> <priority> <estimated_time>
START
```

Example:
```
DRONE 2 100
DRONE 1 85
TASK A Customer-101 2 10
TASK B Big Store Chain 1 12
TASK C John Doe Residence 3 8
START
```

## Project Goals

This simulation demonstrates:
- Multi-threading with POSIX threads
- Priority-based preemptive scheduling
- Synchronization with mutexes and semaphores
- Resource contention and management
- Thread-safe data structures
- Real-world application of OS concepts
- Dynamic configuration and user control
