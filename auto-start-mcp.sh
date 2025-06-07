#!/bin/bash

# AtlantisEons MCP Auto-Startup Script
# Ensures the MCP server starts automatically when needed

PROJECT_DIR="/Users/danielvargas/Documents/Unreal Projects/AtlantisEons"
LOG_FILE="$PROJECT_DIR/auto-startup.log"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log_message() {
    echo "[$(date '+%H:%M:%S')] $1" | tee -a "$LOG_FILE"
}

check_and_start() {
    cd "$PROJECT_DIR" || exit 1
    
    log_message "🔍 Checking MCP server status..."
    
    # Check if server is running
    if ./start-memory-integration.sh status > /dev/null 2>&1; then
        log_message "✅ MCP server is already running"
        echo -e "${GREEN}✅ MCP server is already running${NC}"
        return 0
    else
        log_message "🚀 Starting MCP server..."
        echo -e "${YELLOW}🚀 Starting MCP server...${NC}"
        
        # Start the server
        ./start-memory-integration.sh start
        
        # Wait a moment and verify it started
        sleep 3
        
        if ./start-memory-integration.sh status > /dev/null 2>&1; then
            log_message "✅ MCP server started successfully"
            echo -e "${GREEN}✅ MCP server started successfully${NC}"
            return 0
        else
            log_message "❌ Failed to start MCP server"
            echo -e "${RED}❌ Failed to start MCP server${NC}"
            return 1
        fi
    fi
}

install_login_item() {
    echo -e "${BLUE}📝 Setting up auto-startup on login...${NC}"
    
    # Create LaunchAgent plist file for macOS
    PLIST_PATH="$HOME/Library/LaunchAgents/com.atlantiseons.mcp.plist"
    
    cat > "$PLIST_PATH" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>com.atlantiseons.mcp</string>
    <key>ProgramArguments</key>
    <array>
        <string>$PROJECT_DIR/auto-start-mcp.sh</string>
        <string>start</string>
    </array>
    <key>RunAtLoad</key>
    <true/>
    <key>KeepAlive</key>
    <false/>
    <key>StandardOutPath</key>
    <string>$LOG_FILE</string>
    <key>StandardErrorPath</key>
    <string>$LOG_FILE</string>
</dict>
</plist>
EOF
    
    # Load the LaunchAgent
    launchctl load "$PLIST_PATH" 2>/dev/null || true
    
    echo -e "${GREEN}✅ Auto-startup configured! MCP server will start automatically on login.${NC}"
    log_message "✅ Auto-startup configured for user login"
}

uninstall_login_item() {
    echo -e "${BLUE}🗑️  Removing auto-startup...${NC}"
    
    PLIST_PATH="$HOME/Library/LaunchAgents/com.atlantiseons.mcp.plist"
    
    # Unload and remove the LaunchAgent
    launchctl unload "$PLIST_PATH" 2>/dev/null || true
    rm -f "$PLIST_PATH"
    
    echo -e "${GREEN}✅ Auto-startup removed${NC}"
    log_message "✅ Auto-startup removed"
}

show_help() {
    echo "AtlantisEons MCP Auto-Startup Script"
    echo ""
    echo "Usage: $0 [command]"
    echo ""
    echo "Commands:"
    echo "  start              - Check and start MCP server if needed"
    echo "  install-auto       - Set up automatic startup on login"
    echo "  uninstall-auto     - Remove automatic startup"
    echo "  status             - Show current server status"
    echo "  help               - Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 start           # Start server if not running"
    echo "  $0 install-auto    # Set up auto-startup on login"
    echo "  $0 status          # Check current status"
}

# Main script logic
case "${1:-start}" in
    "start")
        check_and_start
        ;;
    "install-auto")
        install_login_item
        ;;
    "uninstall-auto")
        uninstall_login_item
        ;;
    "status")
        cd "$PROJECT_DIR" || exit 1
        ./start-memory-integration.sh status
        ;;
    "help"|"-h"|"--help")
        show_help
        ;;
    *)
        echo -e "${RED}❌ Unknown command: $1${NC}"
        show_help
        exit 1
        ;;
esac 