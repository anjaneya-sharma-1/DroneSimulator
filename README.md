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
