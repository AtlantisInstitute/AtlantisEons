#!/bin/bash

# AtlantisEons Memory-MCP Integration Startup Script
# Launches the integrated memory system and performs initial synchronization

set -e

PROJECT_ROOT="/Users/danielvargas/Documents/Unreal Projects/AtlantisEons"
MCP_PORT="${MCP_PORT:-9011}"
LOG_FILE="$PROJECT_ROOT/memory-integration.log"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[$(date '+%H:%M:%S')]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[$(date '+%H:%M:%S')] ✅${NC} $1"
}

print_error() {
    echo -e "${RED}[$(date '+%H:%M:%S')] ❌${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[$(date '+%H:%M:%S')] ⚠️${NC} $1"
}

print_info() {
    echo -e "${CYAN}[$(date '+%H:%M:%S')] ℹ️${NC} $1"
}

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to check if port is available
port_available() {
    ! lsof -Pi :$1 -sTCP:LISTEN -t >/dev/null
}

# Function to wait for server to be ready
wait_for_server() {
    local max_attempts=30
    local attempt=1
    
    print_status "Waiting for MCP server to be ready..."
    
    while [ $attempt -le $max_attempts ]; do
        if curl -s "http://localhost:$MCP_PORT" >/dev/null 2>&1; then
            return 0
        fi
        
        if [ $((attempt % 5)) -eq 0 ]; then
            print_status "Still waiting for server... (attempt $attempt/$max_attempts)"
        fi
        
        sleep 1
        attempt=$((attempt + 1))
    done
    
    return 1
}

# Function to stop existing server if running
stop_existing_server() {
    local pids=$(lsof -ti :$MCP_PORT 2>/dev/null || true)
    if [ -n "$pids" ]; then
        print_warning "Found existing process on port $MCP_PORT. Stopping..."
        kill -TERM $pids 2>/dev/null || true
        sleep 2
        
        # Force kill if still running
        pids=$(lsof -ti :$MCP_PORT 2>/dev/null || true)
        if [ -n "$pids" ]; then
            print_warning "Force killing process on port $MCP_PORT"
            kill -KILL $pids 2>/dev/null || true
            sleep 1
        fi
    fi
}

# Function to validate environment
validate_environment() {
    print_status "Validating environment..."
    
    # Check if we're in the correct directory
    if [ ! -f "$PROJECT_ROOT/AtlantisEons.uproject" ]; then
        print_error "Not in AtlantisEons project directory. Please run from: $PROJECT_ROOT"
        exit 1
    fi
    
    # Check for required Node.js
    if ! command_exists node; then
        print_error "Node.js is not installed. Please install Node.js to continue."
        exit 1
    fi
    
    # Check for required files
    if [ ! -f "$PROJECT_ROOT/memory-mcp-server.js" ]; then
        print_error "memory-mcp-server.js not found. Please ensure the integrated memory system is properly set up."
        exit 1
    fi
    
    if [ ! -f "$PROJECT_ROOT/memory-integration-cli.js" ]; then
        print_error "memory-integration-cli.js not found. Please ensure the CLI tool is available."
        exit 1
    fi
    
    # Check for memory manager
    if [ ! -f "$PROJECT_ROOT/memory-manager.js" ]; then
        print_error "memory-manager.js not found. Please ensure the memory system is properly installed."
        exit 1
    fi
    
    # Check for node_modules
    if [ ! -d "$PROJECT_ROOT/node_modules" ]; then
        print_warning "node_modules not found. Installing dependencies..."
        cd "$PROJECT_ROOT"
        npm install
    fi
    
    # Ensure memory directory exists
    mkdir -p "$PROJECT_ROOT/.memory"
    
    print_success "Environment validation complete"
}

# Function to backup existing memory
backup_memory() {
    local backup_dir="$PROJECT_ROOT/.memory/backups"
    local timestamp=$(date +"%Y%m%d_%H%M%S")
    local backup_name="memory_backup_$timestamp"
    
    if [ -f "$PROJECT_ROOT/.memory/project-memory.json" ]; then
        print_status "Creating memory backup..."
        
        mkdir -p "$backup_dir"
        
        # Create backup archive
        cd "$PROJECT_ROOT/.memory"
        tar -czf "$backup_dir/$backup_name.tar.gz" *.json 2>/dev/null || true
        
        if [ -f "$backup_dir/$backup_name.tar.gz" ]; then
            print_success "Memory backed up to: $backup_name.tar.gz"
            
            # Keep only last 10 backups
            cd "$backup_dir"
            ls -t memory_backup_*.tar.gz 2>/dev/null | tail -n +11 | xargs rm -f 2>/dev/null || true
        else
            print_warning "Memory backup failed, but continuing..."
        fi
    fi
}

# Function to start the MCP server
start_mcp_server() {
    print_status "Starting Memory-Enhanced MCP Server on port $MCP_PORT..."
    
    cd "$PROJECT_ROOT"
    
    # Start server in background
    MCP_PORT=$MCP_PORT nohup node memory-mcp-server.js > "$LOG_FILE" 2>&1 &
    local server_pid=$!
    
    echo $server_pid > "$PROJECT_ROOT/.mcp-server.pid"
    
    print_info "Server PID: $server_pid"
    print_info "Logs: $LOG_FILE"
    
    # Wait for server to be ready
    if wait_for_server; then
        print_success "MCP server started successfully"
        return 0
    else
        print_error "MCP server failed to start or is not responding"
        
        # Show last few lines of log
        if [ -f "$LOG_FILE" ]; then
            print_error "Last few lines from log:"
            tail -10 "$LOG_FILE"
        fi
        
        return 1
    fi
}

# Function to perform initial sync
initial_sync() {
    print_status "Performing initial memory synchronization..."
    
    cd "$PROJECT_ROOT"
    
    # Force sync to ensure everything is up to date
    if node memory-integration-cli.js sync --force; then
        print_success "Initial sync completed successfully"
    else
        print_warning "Initial sync encountered issues, but server is running"
        return 1
    fi
    
    # Create initial snapshot
    print_status "Creating initial memory snapshot..."
    if node memory-integration-cli.js snapshot --description "System startup snapshot - $(date)"; then
        print_success "Initial snapshot created"
    else
        print_warning "Snapshot creation failed, but system is operational"
    fi
}

# Function to display system status
show_status() {
    print_info "=== ATLANTISEONS MEMORY-MCP INTEGRATION STATUS ==="
    
    # Server status
    if curl -s "http://localhost:$MCP_PORT" >/dev/null 2>&1; then
        print_success "MCP Server: Running on port $MCP_PORT"
    else
        print_error "MCP Server: Not responding"
        return 1
    fi
    
    # Memory stats
    print_status "Getting memory statistics..."
    cd "$PROJECT_ROOT"
    if node memory-integration-cli.js stats --details 2>/dev/null; then
        print_success "Memory system operational"
    else
        print_warning "Memory system status unclear"
    fi
    
    # Health check
    print_status "Performing health check..."
    if node memory-integration-cli.js health 2>/dev/null; then
        print_success "System health check passed"
    else
        print_warning "Health check issues detected"
    fi
    
    print_info "=== INTEGRATION READY ==="
    print_info "🔧 Use CLI: ./memory-integration-cli.js [command]"
    print_info "📊 Server logs: $LOG_FILE"
    print_info "🔄 Auto-sync: Every 30 seconds"
    print_info "📂 Memory dir: $PROJECT_ROOT/.memory"
}

# Function to show usage
show_usage() {
    echo -e "${CYAN}AtlantisEons Memory-MCP Integration Startup${NC}"
    echo ""
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  start     Start the integrated memory-MCP system (default)"
    echo "  stop      Stop the MCP server"
    echo "  restart   Restart the system"
    echo "  status    Show system status"
    echo "  backup    Create memory backup"
    echo "  logs      Show server logs"
    echo "  help      Show this help message"
    echo ""
    echo "Environment variables:"
    echo "  MCP_PORT  Server port (default: 9011)"
    echo ""
    echo "Examples:"
    echo "  $0                    # Start system"
    echo "  $0 status             # Check status"
    echo "  MCP_PORT=9012 $0      # Start on port 9012"
}

# Function to stop server
stop_server() {
    print_status "Stopping MCP server..."
    
    local pid_file="$PROJECT_ROOT/.mcp-server.pid"
    
    if [ -f "$pid_file" ]; then
        local pid=$(cat "$pid_file")
        if kill -0 "$pid" 2>/dev/null; then
            kill -TERM "$pid"
            print_success "Server stopped (PID: $pid)"
        else
            print_warning "Server was not running (stale PID file)"
        fi
        rm -f "$pid_file"
    else
        stop_existing_server
        print_success "Any existing server processes stopped"
    fi
}

# Function to show logs
show_logs() {
    if [ -f "$LOG_FILE" ]; then
        print_info "Showing server logs (press Ctrl+C to stop):"
        tail -f "$LOG_FILE"
    else
        print_error "Log file not found: $LOG_FILE"
        exit 1
    fi
}

# Main execution
main() {
    case "${1:-start}" in
        "start")
            print_info "🚀 Starting AtlantisEons Memory-MCP Integration..."
            validate_environment
            backup_memory
            stop_existing_server
            start_mcp_server && initial_sync && show_status
            ;;
        "stop")
            stop_server
            ;;
        "restart")
            print_info "🔄 Restarting AtlantisEons Memory-MCP Integration..."
            stop_server
            sleep 2
            validate_environment
            start_mcp_server && initial_sync && show_status
            ;;
        "status")
            show_status
            ;;
        "backup")
            backup_memory
            ;;
        "logs")
            show_logs
            ;;
        "help"|"-h"|"--help")
            show_usage
            ;;
        *)
            print_error "Unknown command: $1"
            show_usage
            exit 1
            ;;
    esac
}

# Ensure we're in the project directory
cd "$PROJECT_ROOT" 2>/dev/null || {
    print_error "Could not change to project directory: $PROJECT_ROOT"
    exit 1
}

# Run main function
main "$@" 