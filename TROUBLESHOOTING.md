# Troubleshooting Guide: Blank Screen & Error Handling

## Problem: Blank Screen After Clicking START

When you click the START button and the screen goes blank, this guide helps you diagnose and fix the issue.

---

## Root Causes & Solutions

### 1. **Backend Server Not Running** (Most Common)

**Symptom:**
- Click START → screen goes blank
- No error message shown
- Event log area shows nothing or minimal content

**Why this happens:**
The Node.js backend server must be running on port 5000 to handle simulation requests. Without it, the frontend cannot communicate with the C simulation engine.

**How to fix:**

```bash
npm start
```

This command:
1. Compiles the C executable (`make -C src/c_core`)
2. Starts the Node.js server (`node src/web/server/index.js`)
3. Server listens on port 5000

Once running, you'll see:
```
Server running on http://0.0.0.0:5000
```

Then refresh your browser and try again.

---

### 2. **C Executable Not Compiled**

**Symptom:**
- Server starts but crashes immediately when you click START
- Console shows `Error: spawn ENOENT`

**Why this happens:**
The server tries to spawn `/src/c_core/drone_scheduler` but the binary doesn't exist.

**How to fix:**

```bash
npm run build:c
```

Or manually:

```bash
cd src/c_core
make clean
make
cd ../../
npm start
```

---

### 3. **Network/Connection Error**

**Symptom:**
- Error message: "Cannot connect to backend server"
- Error message: "Make sure the server is running: npm start"

**Why this happens:**
The frontend tried to reach `http://localhost:5000/api/start` but the connection failed. This could mean:
- Server is not running
- Server crashed
- Port 5000 is blocked or in use by another process

**How to fix:**

**Check if server is running:**
```bash
curl http://localhost:5000/api/status
```

**If connection refused:**
```bash
npm start
```

**If port 5000 is in use by another process:**
```bash
# Find and kill the process (Linux/Mac)
lsof -i :5000
kill -9 <PID>

# Then start fresh
npm start
```

---

### 4. **HTTP 501 Error: Backend Unavailable**

**Symptom:**
- Error message: "HTTP 501 error"
- Hint: "Make sure the backend server is running with npm start"

**Why this happens:**
You're accessing the app through Vercel or a static server that can't run the C executable. The Vercel deployment only serves static files and returns 501 errors for API calls.

**How to fix:**
- **For local development:** Use `npm start` (not Vercel)
- **For self-hosting:** Run the Node.js server on a dedicated host that supports C execution

---

## New Error Messages & What They Mean

The updated error handling shows clearer messages:

### ✅ Success Message
```
🔄 Connecting to server...
✅ Simulation started successfully!
```

### ❌ Server Connection Errors
```
❌ Cannot connect to backend server
💡 Make sure the server is running: npm start
📍 Technical details: [error message]
```

### ❌ Server Error Response
```
❌ Error: [specific error message]
💡 Hint: Make sure the backend server is running with "npm start"
```

### ❌ Simulation Error
```
❌ Error: [error from C simulation]
```

### ⚠️ Connection Lost During Simulation
```
⚠️ Connection lost with backend server
```

---

## Debugging Checklist

- [ ] Backend server is running (`npm start`)
- [ ] C executable is compiled (`npm run build:c`)
- [ ] Browser console shows no JavaScript errors (F12 to open)
- [ ] Event log shows actual error messages (not blank)
- [ ] At least one drone added
- [ ] At least one task added
- [ ] Port 5000 is not blocked
- [ ] Try clearing browser cache (Ctrl+Shift+Delete or Cmd+Shift+Delete)

---

## Architecture Overview

```
┌─────────────────────────────────────────┐
│         Browser (Frontend)              │
│  public/index.html                      │
│  • Displays UI                          │
│  • Sends config to backend              │
│  • Shows real-time logs                 │
└──────────────┬──────────────────────────┘
               │
        HTTP/SSE (Port 5000)
               │
┌──────────────▼──────────────────────────┐
│  Node.js Server                         │
│  src/web/server/index.js                │
│  • Receives simulation config           │
│  • Spawns C process                     │
│  • Forwards C output to browser         │
└──────────────┬──────────────────────────┘
               │
       Child Process (Spawn)
               │
┌──────────────▼──────────────────────────┐
│  C Simulation Engine                    │
│  src/c_core/drone_scheduler             │
│  • Pthreads for drones                  │
│  • Semaphores for resources             │
│  • Priority scheduling                  │
│  • Battery/charging logic               │
└─────────────────────────────────────────┘
```

---

## Key Improvements Made

1. **Better Error Messages**
   - Frontend now checks HTTP response status before parsing
   - Shows specific error messages instead of blank screen
   - Helpful hints for common issues (e.g., "npm start" reminder)

2. **Network Error Handling**
   - Distinguishes between different error types
   - Provides technical details for debugging
   - Friendly user-facing messages

3. **Connection Monitoring**
   - EventSource connection now reports when lost
   - Updates status indicator when connection fails
   - Prevents "zombie" running state

4. **Better Logging**
   - Shows connection progress ("🔄 Connecting to server...")
   - Success messages include ✅ emoji
   - Error messages include ❌ emoji
   - Connection loss shows ⚠️ emoji

---

## Still Having Issues?

If you still see a blank screen:

1. **Check browser console** (F12 → Console tab) for JavaScript errors
2. **Check server logs** - look for error messages when you click START
3. **Verify C compilation** - try `npm run build:c` again
4. **Try a fresh start:**
   ```bash
   npm run build:c
   npm start
   ```
5. **Restart browser** - close all tabs and refresh

---

## Development vs Production

| Scenario | How to Run | Backend |
|----------|-----------|---------|
| Local development | `npm start` | Node.js server on port 5000 |
| Vercel deployment | Browse URL | Static files only (no C support) |
| Self-hosted | `npm start` on Linux/WSL | Full support with C executable |

The application is designed for local development with `npm start` or self-hosted deployment. Vercel deployment is read-only (static files only).
