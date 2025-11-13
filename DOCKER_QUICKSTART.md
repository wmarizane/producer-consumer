# Docker Setup - Step by Step (Never Used Docker Before!)

## Step 1: Install Docker

### On macOS:
1. Go to https://www.docker.com/products/docker-desktop
2. Click "Download for Mac"
3. Open the downloaded file and drag Docker to Applications
4. Open Docker Desktop from Applications
5. Wait for it to start (you'll see a whale icon in your menu bar at the top)

### On Windows:
1. Go to https://www.docker.com/products/docker-desktop
2. Click "Download for Windows"
3. Run the installer
4. Restart your computer
5. Open Docker Desktop

### Verify Docker is Working:
Open Terminal and type:
```bash
docker --version
```

You should see something like: `Docker version 24.0.6`

## Step 2: Run Your Distributed System!

### Super Easy Way - One Command:
```bash
./docker_demo.sh
```

That's it! The script will:
- Check if Docker is installed
- Build your containers
- Start broker, producer, and 4 consumers
- Show you the logs

### Manual Way (if you want to learn):

#### Start everything:
```bash
docker-compose up --build
```

#### Stop everything:
Press `Ctrl+C`, then:
```bash
docker-compose down
```

## Step 3: See It Working!

### View Logs from Each Component:

In new terminal windows, run:
```bash
# Watch the broker
docker logs -f transaction-broker

# Watch consumer 1
docker logs -f transaction-consumer-1

# Watch consumer 2  
docker logs -f transaction-consumer-2
```

### See All Containers:
```bash
docker ps
```

### See Resource Usage:
```bash
docker stats
```

### Open Monitor Dashboard:
Open your browser to: http://localhost:8081

## What's Happening?

Docker is running each component in an isolated "container" (like a mini virtual machine):

```
Your Computer
├── Container 1: Broker     (512MB RAM, 1 CPU)
├── Container 2: Producer   (256MB RAM, 0.5 CPU)
├── Container 3: Consumer 1 (256MB RAM, 0.5 CPU)
├── Container 4: Consumer 2 (256MB RAM, 0.5 CPU)
├── Container 5: Consumer 3 (256MB RAM, 0.5 CPU)
└── Container 6: Consumer 4 (256MB RAM, 0.5 CPU)
```

Each container thinks it's on its own machine! This simulates a real distributed system.

## Experimenting

### Run with Different Numbers of Consumers:

#### Just 1 consumer:
```bash
docker-compose up broker producer consumer1
```

#### Just 2 consumers:
```bash
docker-compose up broker producer consumer1 consumer2
```

#### All 4 consumers (default):
```bash
docker-compose up
```

### Clean Up Everything:
```bash
docker-compose down
```

## Common Issues

### "Docker daemon is not running"
→ Start Docker Desktop application

### "Port 9100 already in use"
→ Stop local processes: `pkill -f broker_exe`

### "Out of disk space"
→ Clean up old containers: `docker system prune -a`

## Need Help?

Check `DOCKER_GUIDE.md` for detailed explanations!
