#include "Commands.h"
#include <ESP8266WebServer.h>

ESP8266WebServer server(80);

void handleCommand() {
    
}

Commands::Commands() {
    // Set up the '/command' endpoint to handle POST requests
    server.on("/command", HTTP_POST, handleCommand);

    // Start the web server
    server.begin();
}

// Define the HandleCommands method
void Commands::HandleCommands() {
    // Handle other commands or additional logic here if needed
}
