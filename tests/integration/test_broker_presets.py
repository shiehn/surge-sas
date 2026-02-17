#!/usr/bin/env python3
"""
Integration tests for Surge XT broker-based preset control
Tests the full flow from SAS Assistant through broker to Surge plugin
"""

import socket
import json
import time
import threading
import os
import sys
import subprocess
from pathlib import Path

class MockBroker:
    """Mock broker server that simulates the SAS Plugin Router"""
    
    def __init__(self, socket_path="/tmp/sas-plugin-router.sock"):
        self.socket_path = socket_path
        self.server_socket = None
        self.clients = {}
        self.running = False
        self.log_file = open("/tmp/test-broker.log", "w")
        
    def start(self):
        """Start the mock broker server"""
        # Clean up old socket if it exists
        if os.path.exists(self.socket_path):
            os.unlink(self.socket_path)
            
        self.server_socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.server_socket.bind(self.socket_path)
        self.server_socket.listen(5)
        self.running = True
        
        print(f"[BROKER] Started on {self.socket_path}")
        self.log("Broker started")
        
        # Start accepting connections in a thread
        threading.Thread(target=self._accept_loop, daemon=True).start()
        
    def _accept_loop(self):
        """Accept incoming connections"""
        while self.running:
            try:
                client_socket, _ = self.server_socket.accept()
                threading.Thread(target=self._handle_client, args=(client_socket,), daemon=True).start()
            except:
                break
                
    def _handle_client(self, client_socket):
        """Handle a connected client"""
        client_id = None
        buffer = ""
        
        while self.running:
            try:
                data = client_socket.recv(4096).decode('utf-8')
                if not data:
                    break
                    
                buffer += data
                lines = buffer.split('\n')
                buffer = lines[-1]  # Keep incomplete line in buffer
                
                for line in lines[:-1]:
                    if line.strip():
                        msg = json.loads(line)
                        self.log(f"Received: {msg}")
                        
                        if msg.get('type') == 'register':
                            client_id = msg.get('plugin_sig')
                            self.clients[client_id] = {
                                'socket': client_socket,
                                'info': msg,
                                'last_heartbeat': time.time()
                            }
                            print(f"[BROKER] Registered plugin: {msg.get('plugin_name')} (sig: {client_id})")
                            
                        elif msg.get('type') == 'heartbeat':
                            if client_id in self.clients:
                                self.clients[client_id]['last_heartbeat'] = time.time()
                                
                        elif msg.get('ok') is not None:
                            # Response from plugin
                            print(f"[BROKER] Response from plugin: {msg}")
                            
            except Exception as e:
                print(f"[BROKER] Error handling client: {e}")
                break
                
        if client_id and client_id in self.clients:
            del self.clients[client_id]
            print(f"[BROKER] Client disconnected: {client_id}")
            
    def send_command(self, plugin_sig, command):
        """Send a command to a specific plugin"""
        if plugin_sig not in self.clients:
            print(f"[BROKER] Plugin not found: {plugin_sig}")
            return False
            
        try:
            client = self.clients[plugin_sig]
            msg = json.dumps(command) + '\n'
            client['socket'].send(msg.encode('utf-8'))
            self.log(f"Sent to {plugin_sig}: {command}")
            print(f"[BROKER] Sent command to {plugin_sig}: {command['op']}")
            return True
        except Exception as e:
            print(f"[BROKER] Error sending command: {e}")
            return False
            
    def get_connected_plugins(self):
        """Get list of connected plugins"""
        return [
            {
                'plugin_sig': sig,
                'plugin_name': info['info'].get('plugin_name'),
                'plugin_type': info['info'].get('plugin_type')
            }
            for sig, info in self.clients.items()
        ]
        
    def log(self, msg):
        """Log to file"""
        self.log_file.write(f"{time.strftime('%H:%M:%S')} {msg}\n")
        self.log_file.flush()
        
    def stop(self):
        """Stop the broker"""
        self.running = False
        if self.server_socket:
            self.server_socket.close()
        self.log_file.close()
        print("[BROKER] Stopped")


class TestClient:
    """Test client that simulates SAS Assistant sending commands"""
    
    def __init__(self, broker):
        self.broker = broker
        self.test_results = []
        
    def wait_for_plugin(self, timeout=10):
        """Wait for a Surge plugin to connect"""
        print(f"[TEST] Waiting for Surge plugin to connect (max {timeout}s)...")
        start = time.time()
        
        while time.time() - start < timeout:
            plugins = self.broker.get_connected_plugins()
            surge_plugins = [p for p in plugins if 'surge' in p.get('plugin_type', '').lower()]
            
            if surge_plugins:
                print(f"[TEST] Found Surge plugin: {surge_plugins[0]['plugin_name']}")
                return surge_plugins[0]['plugin_sig']
                
            time.sleep(0.5)
            
        return None
        
    def test_list_presets(self, plugin_sig):
        """Test listing presets"""
        print("[TEST] Testing list_presets...")
        
        command = {
            'op': 'list_presets',
            'request_id': 'test-list-001'
        }
        
        success = self.broker.send_command(plugin_sig, command)
        self.test_results.append({
            'test': 'list_presets',
            'sent': success,
            'command': command
        })
        
        return success
        
    def test_load_preset(self, plugin_sig, preset_name):
        """Test loading a preset"""
        print(f"[TEST] Testing load_preset: {preset_name}...")
        
        command = {
            'op': 'load_preset',
            'preset': preset_name,
            'request_id': 'test-load-001'
        }
        
        success = self.broker.send_command(plugin_sig, command)
        self.test_results.append({
            'test': 'load_preset',
            'preset': preset_name,
            'sent': success,
            'command': command
        })
        
        return success
        
    def test_set_parameter(self, plugin_sig, param_index, value):
        """Test setting a parameter"""
        print(f"[TEST] Testing set_param: index={param_index}, value={value}...")
        
        command = {
            'op': 'set_param',
            'index': param_index,
            'value': value,
            'request_id': 'test-param-001'
        }
        
        success = self.broker.send_command(plugin_sig, command)
        self.test_results.append({
            'test': 'set_param',
            'index': param_index,
            'value': value,
            'sent': success,
            'command': command
        })
        
        return success
        
    def test_get_params(self, plugin_sig):
        """Test getting parameters"""
        print("[TEST] Testing get_params...")
        
        command = {
            'op': 'get_params',
            'request_id': 'test-getparams-001'
        }
        
        success = self.broker.send_command(plugin_sig, command)
        self.test_results.append({
            'test': 'get_params',
            'sent': success,
            'command': command
        })
        
        return success
        
    def run_all_tests(self):
        """Run all tests"""
        print("\n" + "="*60)
        print("SURGE XT BROKER INTEGRATION TESTS")
        print("="*60 + "\n")
        
        # Wait for plugin
        plugin_sig = self.wait_for_plugin()
        
        if not plugin_sig:
            print("❌ FAILED: No Surge plugin connected!")
            print("\nPlease ensure:")
            print("1. Surge XT v1.7.0 is installed")
            print("2. REAPER is running with Surge XT loaded")
            print("3. The plugin is trying to connect to the broker")
            return False
            
        print(f"\n✅ Plugin connected: {plugin_sig}\n")
        
        # Run tests with delays to observe responses
        tests_passed = 0
        total_tests = 0
        
        # Test 1: List presets
        if self.test_list_presets(plugin_sig):
            tests_passed += 1
        total_tests += 1
        time.sleep(2)
        
        # Test 2: Load a preset
        if self.test_load_preset(plugin_sig, "Init Saw"):
            tests_passed += 1
        total_tests += 1
        time.sleep(2)
        
        # Test 3: Set parameter
        if self.test_set_parameter(plugin_sig, 0, 0.5):
            tests_passed += 1
        total_tests += 1
        time.sleep(2)
        
        # Test 4: Get parameters
        if self.test_get_params(plugin_sig):
            tests_passed += 1
        total_tests += 1
        time.sleep(2)
        
        # Test 5: Load another preset
        if self.test_load_preset(plugin_sig, "Creamy Keys"):
            tests_passed += 1
        total_tests += 1
        time.sleep(2)
        
        # Summary
        print("\n" + "="*60)
        print("TEST SUMMARY")
        print("="*60)
        print(f"Tests passed: {tests_passed}/{total_tests}")
        
        for result in self.test_results:
            status = "✅" if result.get('sent') else "❌"
            print(f"{status} {result['test']}: {result.get('preset', result.get('index', ''))}")
            
        return tests_passed == total_tests


def check_surge_running():
    """Check if Surge is running in REAPER"""
    try:
        # Check if router log shows recent activity
        log_path = "/tmp/surge-router.log"
        if os.path.exists(log_path):
            with open(log_path, 'r') as f:
                lines = f.readlines()
                if lines:
                    # Check last few lines for recent activity
                    recent = lines[-10:]
                    for line in recent:
                        if 'plugin_sig' in line or 'heartbeat' in line:
                            return True
    except:
        pass
    return False


def main():
    """Main test runner"""
    print("Surge XT Broker Integration Test Suite")
    print("--------------------------------------\n")
    
    # Check if we should use the real broker or mock
    use_real_broker = '--real' in sys.argv
    
    if use_real_broker:
        print("Using REAL broker at /tmp/sas-plugin-router.sock")
        print("Make sure SAS Assistant is running!\n")
        input("Press Enter when ready...")
        
        # Just run client tests against real broker
        client = TestClient(None)  # Will need modification for real broker
        print("❌ Real broker testing not yet implemented")
        return
        
    else:
        print("Starting mock broker for testing...\n")
        
        # Start mock broker
        broker = MockBroker()
        broker.start()
        
        # Give broker time to start
        time.sleep(1)
        
        # Check if Surge is available
        if not check_surge_running():
            print("⚠️  WARNING: Surge may not be running!")
            print("Please ensure REAPER is running with Surge XT v1.7.0 loaded\n")
            
        # Run tests
        client = TestClient(broker)
        success = client.run_all_tests()
        
        # Cleanup
        broker.stop()
        
        # Exit with appropriate code
        sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()