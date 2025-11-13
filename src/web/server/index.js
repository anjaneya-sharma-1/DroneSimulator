const express = require('express');
const cors = require('cors');
const { spawn } = require('child_process');
const path = require('path');

const app = express();
const PORT = 5000;

app.use(cors());
app.use(express.json());
app.use(express.static(path.join(__dirname, '../../../public')));

let simulationProcess = null;
let clients = [];

app.get('/api/status', (req, res) => {
    res.json({ 
        running: simulationProcess !== null,
        clients: clients.length
    });
});

app.post('/api/start', (req, res) => {
    if (simulationProcess) {
        return res.status(400).json({ error: 'Simulation already running' });
    }

    const { drones = [], tasks = [], charging = 3, loading = 5, duration = 40 } = req.body;
    
    if (drones.length === 0) {
        return res.status(400).json({ error: 'At least one drone is required' });
    }
    
    if (tasks.length === 0) {
        return res.status(400).json({ error: 'At least one task is required' });
    }

    const args = [
        '--charging', charging.toString(),
        '--loading', loading.toString(),
        '--duration', duration.toString(),
        '--config', 'stdin'
    ];

    const executablePath = path.join(__dirname, '../../c_core/drone_scheduler');
    
    simulationProcess = spawn(executablePath, args);
    
    let configInput = '';
    drones.forEach(drone => {
        configInput += `DRONE ${drone.speed} ${drone.battery}\n`;
    });
    
    tasks.forEach(task => {
        configInput += `TASK ${task.warehouse} ${task.customer} ${task.priority} ${task.estimatedTime}\n`;
    });
    
    configInput += 'START\n';
    
    simulationProcess.stdin.write(configInput);
    simulationProcess.stdin.end();
    
    simulationProcess.stdout.on('data', (data) => {
        const output = data.toString();
        console.log(output);
        
        clients.forEach(client => {
            if (!client.finished) {
                client.write(`data: ${JSON.stringify({ type: 'log', data: output })}\n\n`);
            }
        });
    });

    simulationProcess.stderr.on('data', (data) => {
        console.error(`Error: ${data}`);
        clients.forEach(client => {
            if (!client.finished) {
                client.write(`data: ${JSON.stringify({ type: 'error', data: data.toString() })}\n\n`);
            }
        });
    });

    simulationProcess.on('close', (code) => {
        console.log(`Simulation process exited with code ${code}`);
        clients.forEach(client => {
            if (!client.finished) {
                client.write(`data: ${JSON.stringify({ type: 'complete', code })}\n\n`);
            }
        });
        simulationProcess = null;
    });

    res.json({ success: true, message: 'Simulation started' });
});

app.post('/api/stop', (req, res) => {
    if (!simulationProcess) {
        return res.status(400).json({ error: 'No simulation running' });
    }

    simulationProcess.kill();
    simulationProcess = null;
    
    res.json({ success: true, message: 'Simulation stopped' });
});

app.get('/api/stream', (req, res) => {
    res.setHeader('Content-Type', 'text/event-stream');
    res.setHeader('Cache-Control', 'no-cache');
    res.setHeader('Connection', 'keep-alive');
    
    res.write(`data: ${JSON.stringify({ type: 'connected' })}\n\n`);
    
    clients.push(res);
    
    req.on('close', () => {
        clients = clients.filter(client => client !== res);
        res.finished = true;
    });
});

app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, '../../../public/index.html'));
});

app.listen(PORT, '0.0.0.0', () => {
    console.log(`Server running on http://0.0.0.0:${PORT}`);
});
