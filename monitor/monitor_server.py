#!/usr/bin/env python3
"""
Simple monitor server for distributed system visualization.
Serves the HTML monitor and provides a REST API for the broker to report status.
"""

from http.server import HTTPServer, SimpleHTTPRequestHandler
import json
import os
from urllib.parse import urlparse, parse_qs

# Store system state
state = {
    'producers': {},  # {id: {connected: true/false, message_count: N}}
    'consumers': {},  # {id: {connected: true/false, message_count: N}}
    'broker': {'active': True, 'total_messages': 0},
    'messages': []  # Recent message events for animation
}

class MonitorHandler(SimpleHTTPRequestHandler):
    def do_GET(self):
        parsed = urlparse(self.path)
        
        if parsed.path == '/status':
            # Return current system state as JSON
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            self.wfile.write(json.dumps(state).encode())
        
        elif parsed.path == '/':
            # Serve the monitor HTML
            self.path = '/monitor.html'
            return SimpleHTTPRequestHandler.do_GET(self)
        
        else:
            return SimpleHTTPRequestHandler.do_GET(self)
    
    def do_POST(self):
        parsed = urlparse(self.path)
        
        if parsed.path == '/update':
            # Broker sends updates here
            content_length = int(self.headers['Content-Length'])
            post_data = self.rfile.read(content_length)
            data = json.loads(post_data.decode())
            
            # Update state based on broker report
            if 'event' in data:
                event = data['event']
                
                if event == 'producer_connected':
                    state['producers'][data['id']] = {'connected': True, 'message_count': 0}
                
                elif event == 'producer_disconnected':
                    if data['id'] in state['producers']:
                        state['producers'][data['id']]['connected'] = False
                
                elif event == 'consumer_connected':
                    state['consumers'][data['id']] = {'connected': True, 'message_count': 0}
                
                elif event == 'consumer_disconnected':
                    if data['id'] in state['consumers']:
                        state['consumers'][data['id']]['connected'] = False
                
                elif event == 'message_sent':
                    state['broker']['total_messages'] += 1
                    state['messages'].append({
                        'from': data.get('from', 'producer'),
                        'to': data.get('to', 'broker'),
                        'timestamp': data.get('timestamp', 0)
                    })
                    # Keep only last 100 messages
                    state['messages'] = state['messages'][-100:]
            
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            self.wfile.write(json.dumps({'status': 'ok'}).encode())
        
        else:
            self.send_response(404)
            self.end_headers()

def run_server(port=8080):
    # Change to monitor directory
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    
    server_address = ('', port)
    httpd = HTTPServer(server_address, MonitorHandler)
    print(f'🎨 Monitor server running on http://localhost:{port}')
    print(f'📊 Open http://localhost:{port} in your browser to view the visualization')
    print('\nPress Ctrl+C to stop\n')
    httpd.serve_forever()

if __name__ == '__main__':
    run_server()
