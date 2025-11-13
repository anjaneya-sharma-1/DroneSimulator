# Autonomous Delivery Drone Scheduler — Comprehensive Design & Reference

This README documents the DroneSimulator project in depth: constants, data structures, algorithms, threading model, UI interactions, build/run instructions, known limitations and a task-based roadmap for incremental, in-depth updates.

## 1. Project overview

The DroneSimulator is a hybrid application composed of:
- A C core simulation (`src/c_core/`) that implements a multi-threaded drone scheduler, drone worker threads, semaphores for resources and a priority queue for tasks.
- A Node/JS web server (`src/web/server/`) that accepts configuration from the browser UI and forwards it to the C core where applicable.
- A single-page web UI (`public/index.html`) that provides controls to add drones and tasks, configure simulation parameters, toggle theme, and start/stop the simulation.

The simulator models drones delivering tasks, consuming battery while delivering, and charging at limited charging stations. Tasks are prioritized and may preempt lower-priority work.

## 2. File map (high level)

- `public/index.html` — UI, CSS, and client-side JS (state, render, theme toggle, and config generation).
- `src/web/server/index.js` — server that accepts simulation config and formats it for the C core.
- `src/c_core/drone_scheduler.h` — public types, constants and function declarations for the simulation core.
- `src/c_core/drone_scheduler.c` — implementation: priority queue, scheduler thread, drone threads, logging, and simulation lifecycle.
- `src/c_core/main.c` — entry-point for running the C simulation; parses input lines (e.g., `DRONE <speed> <battery>`).

## 3. Global constants (single source)

The most important constants are defined in `src/c_core/drone_scheduler.h`. Current values:

- MAX_DRONES = 50
- MAX_TASKS = 200
- MAX_CHARGING_STATIONS = 3
- MAX_LOADING_BAYS = 5
- BATTERY_LOW_THRESHOLD = 20  // percent
- BATTERY_DRAIN_RATE = 5      // percent per second while delivering
- BATTERY_CHARGE_RATE = 10    // percent per second while charging

Notes:
- These constants are compiled into the C core and used by scheduling and drone logic. UI validation enforces the battery input range (20–100%).

## 4. Data structures

Defined in `drone_scheduler.h`:

- Task
  - task_id, source, destination
  - priority (1 high, 2 medium, 3 low)
  - estimated_time (seconds)
  - state (TASK_PENDING, ASSIGNED, IN_PROGRESS, COMPLETED, PREEMPTED)
  - assigned_drone, start_time, end_time

- Drone
  - drone_id
  - state (DRONE_IDLE, LOADING, DELIVERING, CHARGING, PREEMPTED)
  - battery_level (integer percent)
  - speed (1..3) used to scale delivery time
  - current_task pointer and thread handle
  - active flag and counters (tasks_completed, preempted_count)

- PriorityQueue
  - array of Task pointers implementing a min-heap using `priority` as the comparator

- Statistics and Simulation structs
  - Aggregated counters and synchronization primitives (mutex, semaphores)

## 5. Threading and synchronization model

- Each `Drone` has a dedicated worker thread (`drone_thread_func`) that:
  - waits for tasks assigned by the scheduler,
  - acquires a loading bay (semaphore) to simulate loading,
  - executes delivery while decrementing battery each second,
  - requests charging station (semaphore) when battery at/under threshold and charges to full.

- A single scheduler thread (`scheduler_thread_func`) continuously inspects the priority queue and assigns tasks to eligible drones.

- Semaphores:
  - `sim->loading_bays` limits concurrent drone loading operations.
  - `sim->charging_stations` limits concurrent charging operations.

- Mutexes protect shared structures: priority queue operations, statistics collection, and logging.

## 6. Scheduling algorithm (high level)

Scheduler purpose: pick the best drone for the highest-priority pending task, occasionally preempt lower-priority work for urgent ones.

Algorithm summary (as implemented):

1. If there are tasks in the priority queue, peek at the highest-priority task.
2. Find the best drone among `sim->drones[]` where:
   - drone is active
   - drone->battery_level > BATTERY_LOW_THRESHOLD
   - drone is IDLE and has no current task
   - prefer drone with highest battery level among eligible ones
3. If highest-priority task has priority == 1 (urgent), the scheduler attempts to preempt a currently executing drone with a lower-priority task, provided that drone has battery > BATTERY_LOW_THRESHOLD. Preemption steps:
   - mark the current drone's task as TASK_PREEMPTED and push it back into the priority queue
   - clear the drone's current_task and set drone->state = DRONE_PREEMPTED
   - increment relevant counters
4. If an eligible drone is found, pop the task from the queue and assign it: set task->state = TASK_ASSIGNED, update assigned_drone and set drone->current_task.

Reasoning and notes:
- The scheduler uses a greedy assignment strategy with battery-aware eligibility. It prefers drones with higher battery.
- Preemption is allowed only for urgent tasks and only if it is safe (battery above threshold for the preempted drone).

## 7. Drone behavior and battery logic (detailed)

Drone thread main loop (`drone_thread_func`) implements the following lifecycle:

1. If drone->current_task != NULL and state is not PREEMPTED:
   - If IDLE/LOADING: log and acquire loading bay semaphore, simulate loading (sleep 1s), release loading bay.
   - Set drone state to DELIVERING and task->state to TASK_IN_PROGRESS.
2. While DELIVERING and battery_level > BATTERY_LOW_THRESHOLD:
   - Compute delivery_time = max(1, task->estimated_time / drone->speed)
   - For each second of delivery: sleep(1) and decrement battery_level by BATTERY_DRAIN_RATE.
   - If battery_level <= BATTERY_LOW_THRESHOLD during delivery: log critical battery and break (delivery stops early and task will be preempted later).
3. If delivery completes: mark task->state = TASK_COMPLETED, record end_time, update statistics, task cleared, drone->state = IDLE.
4. If battery_level <= BATTERY_LOW_THRESHOLD and not charging:
   - If there was a current_task, mark it TASK_PENDING and push back to the priority queue.
   - Log and wait for a charging station semaphore.
   - While battery < 100: sleep(1) and increment battery by BATTERY_CHARGE_RATE (cap to 100).
   - Release charging station; set drone->state = IDLE.

Important: battery changes are integer percent steps per second. Charging is blocking until full (the drone occupies the charging station until it reaches 100%).

## 8. Priority queue implementation details

- `pq_push`: adds a task pointer at the end of the array, then sifts up comparing `priority` (smaller priority value = higher priority).
- `pq_pop`: returns the root (highest priority), replaces it with the last element, then sifts down.
- Access to the `PriorityQueue` is protected by a mutex for thread safety.

## 9. UI client flow (`public/index.html`)

- Client maintains `drones[]` and `tasks[]` arrays locally and displays them in the GUI.
- Adding a drone performs validation (battery between 20 and 100) and pushes `{speed, battery}` into `drones[]`.
- The web UI collects configuration and the Node server formats entries like `DRONE <speed> <battery>` for the C core's stdin (or config input mechanism in `src/web/server/index.js`).

Key JS functions (client):
- `addDrone()` — read speed/battery, validate, push to `drones` and re-render.
- `addTask()` — read task fields, validate, push to `tasks` and re-render.
- `startSimulation()` / `stopSimulation()` — trigger simulation lifecycle via server endpoint.

## 10. Main entry and config parsing (`src/c_core/main.c`)

- The C `main` parses lines like `DRONE <speed> <battery>` and `TASK <...>` and calls `add_drone()`/`add_task()` accordingly.
- Basic validation is performed at parsing time (speed range 1..3, battery 20..100).

## 11. Known limitations and suggested enhancements

Current simplifications:
- Battery is integer and uses fixed drain/charge constants.
- Drain rate is independent of speed, payload, or distance (only delivery_time divides by speed, which shortens the duration but drain per second is constant).
- Charging is blocking until 100%.
- No task progress persistence: preempted tasks restart from zero, not from partial progress.

Suggested improvements (prioritized):
1. Variable drain: make drain rate a function of speed and/or distance. (files: `drone_scheduler.c`, `drone_scheduler.h`)
2. Fractional battery and float math for finer granularity.
3. Opportunistic charging: allow partial charge to a configurable resume threshold.
4. Partial task progress tracking so preemption can resume from saved progress.
5. Add unit tests for priority queue and scheduler behaviors (C unit tests).

## 12. Task-based roadmap (so README and code can be updated part-by-part)

Each task below is self-contained. I can implement and update them one-by-one.

Task 1 — Document C core functions & add sequence diagrams
- Goal: expand README with function-level documentation for each function in `drone_scheduler.c` and include sequence diagrams for drone lifecycle and scheduler assignment.
- Files to update: `README.md` (this file), optionally `docs/diagrams/*.png` or `.svg`.
- Acceptance criteria: every top-level function has a short contract (inputs, outputs, side-effects), and at least 2 diagrams are added.

Task 2 — Make battery drain depend on speed
- Goal: change `BATTERY_DRAIN_RATE` from a constant to a formula (e.g., BASE_DRAIN + SPEED_FACTOR*speed) and update docs.
- Files to update: `src/c_core/drone_scheduler.h`, `src/c_core/drone_scheduler.c`, `README.md`.
- Acceptance criteria: drain rate is computed per-drone and documented; simulation behavior updated accordingly and verified by a short simulation run or logs.

Task 3 — Implement fractional battery and partial charging
- Goal: move battery_level from `int` to `double`, change charge/drain loops to use fractional increments and update UI to display decimals.
- Files to update: `drone_scheduler.h`, `drone_scheduler.c`, `public/index.html` (display), `src/web/server/index.js` (if needed).
- Acceptance criteria: battery displayed with one decimal, charging/draining updated and tested.

Task 4 — Add unit tests and CI
- Goal: Add C unit tests for `pq_push`/`pq_pop`, and simple scheduler scenarios. Add a GitHub Actions workflow to run tests.
- Files to update: `tests/`, `.github/workflows/ci.yml`, Makefile updates.
- Acceptance criteria: tests run on CI and pass.

Task 5 — Improve charging station behavior
- Goal: allow drones to charge to a configurable resume level (e.g., 60%) so they can resume tasks earlier, freeing charging stations.
- Files to update: `drone_scheduler.c`, `drone_scheduler.h`, `README.md`.
- Acceptance criteria: configurable resume threshold works and is documented.

Task 6 — UI improvements & telemetry
- Goal: surface drone telemetry (state, battery trend, recent events) in the UI and add a visual timeline.
- Files to update: `public/index.html`, CSS, and possible server endpoints for live events.
- Acceptance criteria: telemetry panel shows live updates; logs are readable.

## 13. How to run & build (developer notes)

Minimal steps (assumes standard POSIX tooling for C core):

1. Start the web server (in project root):

   - `node src/web/server/index.js` (or `npm run dev` if configured)

2. Serve `public/index.html` (the server may already host it), open in browser.

3. The UI composes configuration and sends to the server which starts the C core binary or pipes config to it. See `src/web/server/index.js` for exact wiring.

Building the C core (typical):

 - `cd src/c_core && make` (there is a Makefile in `src/c_core/`)

Notes for Windows development: the C core uses pthreads and POSIX APIs; building on Windows may require WSL or a POSIX-compatible toolchain.

### Deployment on Vercel (static UI)

This repository contains a browser UI that can be deployed as a *static site* on Vercel. Important constraints:

- The C simulation core (pthreads, semaphores) cannot run inside Vercel serverless functions.
- The included Node server (`src/web/server/index.js`) expects to spawn or talk to the native C binary and therefore is not compatible with Vercel serverless environment without modification.

What we deployed for Vercel in this repository:

- `vercel.json` — configuration that instructs Vercel to serve the `public/` folder as a static site.
- `api/*.js` — placeholder serverless endpoints (`/api/start`, `/api/stop`, `/api/run-simulation`) that return `501 Not Implemented` with guidance. These prevent errors if the UI tries to call `/api/*` when deployed and provide instructions.

Recommended options when deploying to Vercel:

1. Static-only deployment (fastest):
  - Deploy this repository to Vercel as-is. The UI will load and function locally, but API actions (start/stop) will return `501` from the placeholder serverless functions.
  - If you want the UI for visualization only (manual logs, local mode), this is fine.

2. Hybrid deployment (production-grade):
  - Self-host the C core and Node server on a VPS, cloud VM, or container (AWS EC2, DigitalOcean, Heroku with native buildpack, etc.). Build the C core with `cd src/c_core && make` and run `node src/web/server/index.js` to expose API endpoints.
  - Deploy the static frontend to Vercel and configure the UI to call your hosted API by replacing `fetch('/api/start', ...)` with the absolute URL of your server (for example, `https://simulation.example.com/api/start`). A simple change in `public/index.html` or a small runtime config variable will accomplish this.

3. Port to serverless (advanced):
  - Port the C core logic to Node.js (or compile to WebAssembly that can be executed in a serverless environment). This is a larger change but enables a completely serverless deployment on Vercel.

How to change the UI to point to your external API host (example):

 - Open `public/index.html` and find the `fetch('/api/start', ...)` call in the `startSimulation()` function. Replace the path with your host: `fetch('https://YOUR_HOST/api/start', { ... })`.

Vercel deployment quick steps:

1. Push this repository to GitHub/GitLab.
2. On vercel.com, import the repository.
3. Vercel will detect the static build configuration (see `vercel.json`) and publish the `public/` folder.

Limitations and notes:

- Live simulation (running the C core) cannot be executed within Vercel serverless functions — use one of the hybrid/self-hosting options for full functionality.
- The placeholder endpoints return helpful JSON to guide the operator and avoid silent failures in the UI.


## 14. Contact & next steps

If you'd like, I will start with Task 1 and expand the README to include per-function contracts and sequence diagrams. Tell me which Task to start with (1..6), or say "Follow your recommended order" and I'll proceed with Task 1.

---
_Generated on: 2025-11-13 — this README aims to be a single source of truth for the simulation logic. If you want the doc split into multiple files (`docs/`), I can split it and add diagrams and examples._
# 🚁 Autonomous Delivery Drone Scheduler Simulation

A comprehensive operating systems simulation demonstrating **priority scheduling**, **preemption**, **thread synchronization**, and **resource management** using real-world drone delivery scenarios.

## 📋 Project Overview

This project simulates a fleet of autonomous delivery drones managed by a central scheduler. Each drone operates as an independent thread, competing for shared resources while executing delivery tasks based on priority levels.

### Core Operating System Concepts Demonstrated

- **Multi-threading** with POSIX threads (pthreads)
- **Mutex locks** for critical section protection
- **Semaphores** for resource synchronization
- **Priority scheduling** with preemptive capabilities
- **Thread-safe data structures** (priority queue)
- **Resource contention** and deadlock prevention
- **Battery simulation** with dynamic state management

## 🏗️ Architecture

### C Core Implementation

The simulation core is written in **C** using:
- `pthread.h` - POSIX threads for concurrent drone operations
- `semaphore.h` - Semaphores for charging stations and loading bays
- `pthread_mutex` - Mutex locks for shared resource protection

**Key Components:**

1. **Drone Threads** - Each drone runs as an independent pthread
2. **Priority Queue** - Min-heap implementation for task scheduling
3. **Central Scheduler** - Continuous monitoring and task assignment
4. **Synchronization** - Semaphores control access to limited resources

### Web Visualization

- **Backend:** Express.js server spawns C process and streams output
- **Frontend:** Real-time HTML/CSS/JS dashboard with Server-Sent Events (SSE)
- **Communication:** Event streaming for live simulation updates

## 🚀 Features

### Dynamic Configuration
- ✅ Configurable number of drones (1-50)
- ✅ Adjustable charging stations and loading bays
- ✅ Variable simulation duration
- ✅ Custom drone speeds and battery levels

### Scheduling & Synchronization
- ✅ **Priority-based scheduling** (Priority 1 = Urgent, 3 = Low)
- ✅ **Preemption** - Urgent tasks interrupt lower-priority deliveries
- ✅ **Resource management** - Semaphores control charging/loading access
- ✅ **Battery simulation** - Automatic charging when battery drops below 20%
- ✅ **Task reassignment** - Paused tasks return to queue

### Visualization & Logging
- ✅ **Color-coded console output** (Red=Urgent, Yellow=Medium, Green=Low)
- ✅ **Real-time event streaming** to web dashboard
- ✅ **Comprehensive logging** of all thread operations
- ✅ **Statistics dashboard** showing:
  - Total tasks and completions
  - Average delivery time
  - Preemption count
  - Resource utilization

## 📦 Installation & Setup

### Prerequisites
- GCC compiler (for C compilation)
- Node.js 20+ (for web interface)
- POSIX-compliant system (Linux/macOS)

### Quick Start

```bash
# Install dependencies
npm install

# Compile C code and start web server
npm start
```

The web interface will be available at `http://localhost:5000`

## 🎮 Usage

### Web Interface (Recommended)

1. Open your browser to `http://localhost:5000`
2. Configure simulation parameters:
   - Number of drones
   - Charging stations
   - Loading bays
   - Simulation duration
3. Click "Start" to begin
4. Watch real-time logs and events
5. View statistics when simulation completes

### Command Line

```bash
# Build the C executable
cd src/c_core
make

# Run with default parameters (3 drones, 30 seconds)
./drone_scheduler

# Run with custom parameters
./drone_scheduler --drones 5 --charging 2 --loading 3 --duration 60

# View help
./drone_scheduler --help
```

## 🔧 Command Line Options

| Option | Description | Default |
|--------|-------------|---------|
| `--drones <count>` | Number of drones | 3 |
| `--charging <count>` | Number of charging stations | 3 |
| `--loading <count>` | Number of loading bays | 5 |
| `--duration <seconds>` | Simulation runtime | 30 |
| `--help` | Show help message | - |

## 📊 Example Simulation Flow

1. **Initialization**: System creates drones and adds tasks to priority queue
2. **Scheduler Start**: Central scheduler thread begins monitoring
3. **Task Assignment**: Scheduler assigns highest-priority task to available drone
4. **Resource Acquisition**: Drone waits for and acquires loading bay (semaphore)
5. **Delivery Execution**: Drone delivers package, consuming battery
6. **Battery Management**: Low battery triggers automatic charging
7. **Preemption**: Urgent task (Priority 1) interrupts lower-priority delivery
8. **Task Reassignment**: Preempted task returns to queue
9. **Completion**: Statistics calculated and displayed

## 🧩 Code Structure

```
.
├── src/
│   ├── c_core/                  # C implementation
│   │   ├── drone_scheduler.h    # Header with data structures
│   │   ├── drone_scheduler.c    # Core logic implementation
│   │   ├── main.c               # Entry point
│   │   └── Makefile             # Build configuration
│   └── web/
│       └── server/
│           └── index.js         # Express.js server
├── public/
│   └── index.html               # Web visualization
├── package.json                 # Node.js dependencies
└── README.md                    # This file
```

## 🎯 Key Algorithms

### Priority Queue (Min-Heap)
- Tasks ordered by priority (1 = highest)
- O(log n) insertion and extraction
- Thread-safe with mutex protection

### Priority Scheduling with Preemption
- Scheduler continuously monitors queue
- Assigns tasks to idle drones with sufficient battery
- Preempts lower-priority tasks when urgent deliveries arrive
- Reassigns preempted tasks to queue

### Resource Synchronization
```c
sem_t charging_stations;  // Limited charging access
sem_t loading_bays;       // Limited loading access
pthread_mutex_t log_mutex; // Thread-safe logging
```

## 📈 Statistics & Analysis

The simulation tracks:
- **Completed Tasks**: Total successful deliveries
- **Average Delivery Time**: Mean time from assignment to completion
- **Total Preemptions**: How many tasks were interrupted
- **Resource Utilization**: Charging station and loading bay usage
- **Per-Drone Metrics**: Tasks completed, preemptions, battery level

## 🔍 Educational Insights

### Scheduling Impact
- Priority scheduling ensures urgent deliveries complete first
- Preemption demonstrates real-time OS capabilities
- Resource contention shows need for synchronization

### Synchronization Necessity
- Mutex locks prevent race conditions in logging
- Semaphores prevent resource over-subscription
- Demonstrates deadlock-free design

### Thread Coordination
- Central scheduler coordinates multiple worker threads
- Shared state requires careful synchronization
- Event-driven architecture mirrors real OS design

## 🛠️ Technical Details

### Thread Safety
- All shared data structures protected by mutexes
- Priority queue has dedicated lock
- Statistics updates are atomic

### Synchronization Mechanisms
- **Semaphores**: Counting semaphores for resource pools
- **Mutexes**: Binary locks for critical sections
- **Thread-safe operations**: All shared state modifications protected

### Battery Simulation
- Battery drains 5% per delivery time unit
- Charging increases 10% per second
- Automatic charging triggered at 20% threshold

## 🎓 Learning Outcomes

Students/users will understand:
- How OS schedulers manage concurrent processes
- Why priority scheduling improves system responsiveness
- How semaphores prevent resource conflicts
- The trade-offs of preemptive vs. non-preemptive scheduling
- Real-world applications of OS concepts

## 🚧 Future Enhancements

- [ ] Multiple scheduling algorithm comparison (FCFS, Round Robin)
- [ ] Geographic routing with distance calculations
- [ ] Advanced battery degradation model
- [ ] Replay and step-through debugging
- [ ] Performance profiling and optimization
- [ ] Export simulation logs to JSON/CSV

## 📝 License

MIT License - Feel free to use for educational purposes

## 👨‍💻 Author

Built as an operating systems educational project demonstrating core OS concepts through a practical, visual simulation.

---

**Note**: This is a simulation for educational purposes. No real drones were harmed in the making of this project. 🚁
