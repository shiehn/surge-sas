#!/usr/bin/env python3
"""
Integration tests for Surge XT using REAL broker connection
Tests actual preset control through the SAS Plugin Router
"""

import socket
import json
import time
import sys
import os
from pathlib import Path

class BrokerClient:
    """Client that connects to the real SAS Plugin Router"""
    
    def __init__(self):
        self.socket = None
        self.connected = False
        self.socket_path = "/tmp/sas-plugin-router.sock"
        self.responses = []
        
    def connect(self):
        """Connect to the real broker"""
        # Try primary socket path
        if not os.path.exists(self.socket_path):
            # Try alternate path
            alt_path = os.path.expanduser("~/.sas/router.sock")
            if os.path.exists(alt_path):
                self.socket_path = alt_path
            else:
                print(f"❌ Broker socket not found at {self.socket_path} or {alt_path}")
                print("Please ensure SAS Assistant is running")
                return False
                
        try:
            self.socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            self.socket.connect(self.socket_path)
            self.connected = True
            print(f"✅ Connected to broker at {self.socket_path}")
            return True
        except Exception as e:
            print(f"❌ Failed to connect to broker: {e}")
            return False
            
    def send_command(self, command):
        """Send a command to the broker"""
        if not self.connected:
            return False
            
        try:
            msg = json.dumps(command) + '\n'
            self.socket.send(msg.encode('utf-8'))
            print(f"→ Sent: {command}")
            return True
        except Exception as e:
            print(f"❌ Error sending command: {e}")
            return False
            
    def receive_response(self, timeout=5):
        """Receive response from broker"""
        if not self.connected:
            return None
            
        self.socket.settimeout(timeout)
        try:
            data = self.socket.recv(4096).decode('utf-8')
            lines = data.strip().split('\n')
            
            for line in lines:
                if line:
                    response = json.loads(line)
                    self.responses.append(response)
                    print(f"← Received: {response}")
                    return response
        except socket.timeout:
            print("⏱️  Timeout waiting for response")
        except Exception as e:
            print(f"❌ Error receiving: {e}")
            
        return None
        
    def close(self):
        """Close connection"""
        if self.socket:
            self.socket.close()
            self.connected = False
            print("Connection closed")


class SurgeIntegrationTest:
    """Test Surge preset control through real broker"""
    
    def __init__(self):
        self.client = BrokerClient()
        self.plugin_sig = None
        self.test_results = []
        
    def setup(self):
        """Setup test environment"""
        print("\n" + "="*60)
        print("SURGE XT REAL BROKER INTEGRATION TEST")
        print("="*60 + "\n")
        
        print("Prerequisites:")
        print("1. SAS Assistant must be running")
        print("2. Surge XT v1.7.0 must be loaded in REAPER")
        print("3. The broker socket must exist\n")
        
        # Connect to broker
        if not self.client.connect():
            return False
            
        # Register as a test client
        print("\nRegistering as test client...")
        register_msg = {
            "type": "register",
            "client_type": "test",
            "client_name": "Surge Integration Test"
        }
        
        if not self.client.send_command(register_msg):
            return False
            
        # Wait for registration response
        response = self.client.receive_response()
        if response:
            print(f"Registration response: {response}")
            
        return True
        
    def discover_surge_plugin(self):
        """Discover connected Surge plugin"""
        print("\nDiscovering Surge plugins...")
        
        # Send probe to find plugins
        probe_msg = {
            "op": "probe",
            "request_id": "discover-001"
        }
        
        self.client.send_command(probe_msg)
        response = self.client.receive_response(timeout=3)
        
        if response and 'plugins' in response:
            for plugin in response['plugins']:
                if 'surge' in plugin.get('plugin_type', '').lower():
                    self.plugin_sig = plugin.get('plugin_sig')
                    print(f"✅ Found Surge: {plugin.get('plugin_name')} (sig: {self.plugin_sig})")
                    return True
                    
        # Alternative: try to get plugin list
        list_msg = {
            "op": "list_plugins",
            "request_id": "list-001"
        }
        
        self.client.send_command(list_msg)
        response = self.client.receive_response(timeout=3)
        
        if response:
            print(f"Plugin list response: {response}")
            
        return False
        
    def test_list_presets(self):
        """Test listing presets"""
        print("\n[TEST 1] List Presets")
        print("-" * 40)
        
        command = {
            "op": "list_presets",
            "plugin_sig": self.plugin_sig,
            "request_id": "test-list-presets"
        }
        
        if self.client.send_command(command):
            response = self.client.receive_response()
            
            if response and response.get('ok'):
                presets = response.get('data', {}).get('presets', [])
                print(f"✅ Received {len(presets)} presets")
                if presets:
                    print(f"   First 5: {presets[:5]}")
                self.test_results.append(('list_presets', True, response))
                return True
            else:
                print(f"❌ Failed: {response}")
                self.test_results.append(('list_presets', False, response))
                
        return False
        
    def test_load_preset(self, preset_name):
        """Test loading a preset"""
        print(f"\n[TEST 2] Load Preset: {preset_name}")
        print("-" * 40)
        
        command = {
            "op": "load_preset",
            "plugin_sig": self.plugin_sig,
            "preset": preset_name,
            "request_id": f"test-load-{preset_name}"
        }
        
        if self.client.send_command(command):
            response = self.client.receive_response()
            
            if response and response.get('ok'):
                print(f"✅ Preset loaded: {preset_name}")
                self.test_results.append((f'load_preset:{preset_name}', True, response))
                return True
            else:
                print(f"❌ Failed: {response}")
                self.test_results.append((f'load_preset:{preset_name}', False, response))
                
        return False
        
    def test_set_parameter(self, index, value):
        """Test setting a parameter"""
        print(f"\n[TEST 3] Set Parameter: index={index}, value={value}")
        print("-" * 40)
        
        command = {
            "op": "set_param",
            "plugin_sig": self.plugin_sig,
            "index": index,
            "value": value,
            "request_id": f"test-param-{index}"
        }
        
        if self.client.send_command(command):
            response = self.client.receive_response()
            
            if response and response.get('ok'):
                print(f"✅ Parameter set: index={index}, value={value}")
                self.test_results.append((f'set_param:{index}', True, response))
                return True
            else:
                print(f"❌ Failed: {response}")
                self.test_results.append((f'set_param:{index}', False, response))
                
        return False
        
    def test_get_parameters(self):
        """Test getting parameters"""
        print("\n[TEST 4] Get Parameters")
        print("-" * 40)
        
        command = {
            "op": "get_params",
            "plugin_sig": self.plugin_sig,
            "request_id": "test-get-params"
        }
        
        if self.client.send_command(command):
            response = self.client.receive_response()
            
            if response and response.get('ok'):
                params = response.get('data', {})
                print(f"✅ Received parameters")
                self.test_results.append(('get_params', True, response))
                return True
            else:
                print(f"❌ Failed: {response}")
                self.test_results.append(('get_params', False, response))
                
        return False
        
    def run_all_tests(self):
        """Run all integration tests"""
        if not self.setup():
            print("❌ Setup failed!")
            return False
            
        # Discover Surge plugin
        if not self.discover_surge_plugin():
            print("\n❌ No Surge plugin found!")
            print("\nTroubleshooting:")
            print("1. Check /tmp/surge-router.log for Surge connections")
            print("2. Ensure Surge XT v1.7.0 is loaded in REAPER")
            print("3. Verify SAS Assistant is running")
            
            # Try direct command anyway (without discovery)
            print("\nAttempting direct communication (using last known plugin_sig)...")
            # Read last plugin_sig from log if available
            try:
                with open("/tmp/surge-router.log", "r") as f:
                    lines = f.readlines()
                    for line in reversed(lines):
                        if "plugin_sig:" in line:
                            sig_start = line.find("plugin_sig:") + 11
                            self.plugin_sig = line[sig_start:].strip().split()[0]
                            print(f"Using plugin_sig from log: {self.plugin_sig}")
                            break
            except:
                pass
                
            if not self.plugin_sig:
                return False
                
        # Run test suite
        print("\n" + "="*60)
        print("RUNNING TEST SUITE")
        print("="*60)
        
        # Test 1: List presets
        self.test_list_presets()
        time.sleep(1)
        
        # Test 2: Load preset
        self.test_load_preset("Init Saw")
        time.sleep(2)
        
        # Test 3: Set parameter (cutoff frequency)
        self.test_set_parameter(0, 0.7)
        time.sleep(1)
        
        # Test 4: Get parameters
        self.test_get_parameters()
        time.sleep(1)
        
        # Test 5: Load another preset
        self.test_load_preset("Aggro Growlbass")
        time.sleep(2)
        
        # Print summary
        print("\n" + "="*60)
        print("TEST SUMMARY")
        print("="*60)
        
        passed = sum(1 for _, success, _ in self.test_results if success)
        total = len(self.test_results)
        
        print(f"\nTests passed: {passed}/{total}\n")
        
        for test_name, success, response in self.test_results:
            status = "✅ PASS" if success else "❌ FAIL"
            print(f"{status}: {test_name}")
            if not success and response:
                print(f"         Error: {response.get('error', 'Unknown error')}")
                
        # Cleanup
        self.client.close()
        
        return passed == total


def check_prerequisites():
    """Check if prerequisites are met"""
    issues = []
    
    # Check for broker socket
    if not os.path.exists("/tmp/sas-plugin-router.sock"):
        alt_path = os.path.expanduser("~/.sas/router.sock")
        if not os.path.exists(alt_path):
            issues.append("Broker socket not found - SAS Assistant may not be running")
            
    # Check for Surge log
    if os.path.exists("/tmp/surge-router.log"):
        # Check if log is recent (modified in last 60 seconds)
        mtime = os.path.getmtime("/tmp/surge-router.log")
        if time.time() - mtime > 60:
            issues.append("Surge log is stale - Surge may not be running")
    else:
        issues.append("Surge log not found - Surge may not be running")
        
    if issues:
        print("⚠️  Prerequisites check failed:")
        for issue in issues:
            print(f"   - {issue}")
        print("\nContinue anyway? (y/n): ", end='')
        if input().lower() != 'y':
            return False
            
    return True


def main():
    """Main entry point"""
    print("Surge XT Broker Integration Test")
    print("Using REAL broker implementation")
    print("-" * 40 + "\n")
    
    if not check_prerequisites():
        sys.exit(1)
        
    test = SurgeIntegrationTest()
    success = test.run_all_tests()
    
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()