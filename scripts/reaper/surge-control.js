#!/usr/bin/env node

const net = require('net');
const readline = require('readline');

// Configuration
const SURGE_HOST = 'localhost';
const SURGE_PORT = 7833;
const TIMEOUT = 5000;

// ANSI color codes for terminal output
const colors = {
    reset: '\x1b[0m',
    bright: '\x1b[1m',
    green: '\x1b[32m',
    red: '\x1b[31m',
    yellow: '\x1b[33m',
    blue: '\x1b[34m',
    cyan: '\x1b[36m'
};

class SurgeController {
    constructor() {
        this.client = null;
    }

    connect() {
        return new Promise((resolve, reject) => {
            this.client = new net.Socket();
            
            const timeout = setTimeout(() => {
                this.client.destroy();
                reject(new Error('Connection timeout'));
            }, TIMEOUT);

            this.client.connect(SURGE_PORT, SURGE_HOST, () => {
                clearTimeout(timeout);
                resolve();
            });

            this.client.on('error', (err) => {
                clearTimeout(timeout);
                reject(err);
            });
        });
    }

    sendCommand(command) {
        return new Promise((resolve, reject) => {
            if (!this.client) {
                reject(new Error('Not connected'));
                return;
            }

            let responseData = '';
            const timeout = setTimeout(() => {
                this.client.removeAllListeners('data');
                reject(new Error('Response timeout'));
            }, TIMEOUT);

            const dataHandler = (data) => {
                responseData += data.toString();
                
                // Check if we have a complete JSON response
                try {
                    const response = JSON.parse(responseData);
                    clearTimeout(timeout);
                    this.client.removeListener('data', dataHandler);
                    resolve(response);
                } catch (e) {
                    // Not complete JSON yet, keep accumulating
                }
            };

            this.client.on('data', dataHandler);
            this.client.write(command + '\n');
        });
    }

    disconnect() {
        if (this.client) {
            this.client.destroy();
            this.client = null;
        }
    }

    async loadPreset(presetName) {
        const response = await this.sendCommand(`PRESET:LOAD:${presetName}`);
        return response;
    }

    async getPresetList() {
        const response = await this.sendCommand('PRESET:LIST');
        return response;
    }

    async setParameter(paramId, value) {
        const response = await this.sendCommand(`PARAM:SET:${paramId}:${value}`);
        return response;
    }

    async ping() {
        const response = await this.sendCommand('PING');
        return response;
    }
}

// Command-line interface
async function main() {
    const args = process.argv.slice(2);
    
    if (args.length === 0) {
        printUsage();
        process.exit(1);
    }

    const controller = new SurgeController();

    try {
        // Connect to Surge
        process.stdout.write(`${colors.cyan}Connecting to Surge XT at ${SURGE_HOST}:${SURGE_PORT}...${colors.reset} `);
        await controller.connect();
        console.log(`${colors.green}✓${colors.reset}`);

        const command = args[0].toLowerCase();

        switch (command) {
            case 'preset':
            case 'load':
                if (args.length < 2) {
                    console.error(`${colors.red}Error: Preset name required${colors.reset}`);
                    printUsage();
                    process.exit(1);
                }
                const presetName = args.slice(1).join(' ');
                await loadPresetCommand(controller, presetName);
                break;

            case 'list':
                await listPresetsCommand(controller);
                break;

            case 'param':
            case 'set':
                if (args.length < 3) {
                    console.error(`${colors.red}Error: Parameter ID and value required${colors.reset}`);
                    printUsage();
                    process.exit(1);
                }
                await setParameterCommand(controller, args[1], args[2]);
                break;

            case 'ping':
            case 'health':
                await pingCommand(controller);
                break;

            case 'interactive':
            case 'i':
                await interactiveMode(controller);
                break;

            default:
                console.error(`${colors.red}Unknown command: ${command}${colors.reset}`);
                printUsage();
                process.exit(1);
        }

    } catch (error) {
        console.error(`${colors.red}Error: ${error.message}${colors.reset}`);
        
        if (error.code === 'ECONNREFUSED') {
            console.error(`\nMake sure Surge XT is running and TCP control is enabled.`);
            console.error(`The TCP control server should be listening on port ${SURGE_PORT}.`);
        }
        
        process.exit(1);
    } finally {
        controller.disconnect();
    }
}

async function loadPresetCommand(controller, presetName) {
    console.log(`${colors.cyan}Loading preset: ${colors.bright}${presetName}${colors.reset}`);
    
    const response = await controller.loadPreset(presetName);
    
    if (response.status === 'ok') {
        console.log(`${colors.green}✓ Successfully loaded preset: ${response.preset}${colors.reset}`);
    } else {
        console.error(`${colors.red}✗ Failed to load preset: ${response.message}${colors.reset}`);
        process.exit(1);
    }
}

async function listPresetsCommand(controller) {
    console.log(`${colors.cyan}Fetching preset list...${colors.reset}`);
    
    const response = await controller.getPresetList();
    
    if (response.status === 'ok') {
        console.log(`\n${colors.bright}Available Presets:${colors.reset}`);
        
        if (response.presets && response.presets.length > 0) {
            response.presets.forEach((preset, index) => {
                console.log(`  ${colors.yellow}${(index + 1).toString().padStart(3)}.${colors.reset} ${preset}`);
            });
            console.log(`\n${colors.cyan}Total: ${response.presets.length} presets${colors.reset}`);
        } else {
            console.log(`  ${colors.yellow}No presets available${colors.reset}`);
        }
    } else {
        console.error(`${colors.red}✗ Failed to get preset list: ${response.message}${colors.reset}`);
        process.exit(1);
    }
}

async function setParameterCommand(controller, paramId, value) {
    console.log(`${colors.cyan}Setting parameter ${paramId} to ${value}...${colors.reset}`);
    
    const response = await controller.setParameter(paramId, value);
    
    if (response.status === 'ok') {
        console.log(`${colors.green}✓ Successfully set ${response.param} to ${response.value}${colors.reset}`);
    } else {
        console.error(`${colors.red}✗ Failed to set parameter: ${response.message}${colors.reset}`);
        process.exit(1);
    }
}

async function pingCommand(controller) {
    console.log(`${colors.cyan}Checking connection...${colors.reset}`);
    
    const response = await controller.ping();
    
    if (response.status === 'ok') {
        console.log(`${colors.green}✓ Surge XT TCP Control is online${colors.reset}`);
        if (response.version) {
            console.log(`  Version: ${response.version}`);
        }
    } else {
        console.error(`${colors.red}✗ Connection check failed${colors.reset}`);
        process.exit(1);
    }
}

async function interactiveMode(controller) {
    console.log(`${colors.bright}${colors.cyan}Surge XT Interactive Control${colors.reset}`);
    console.log(`${colors.cyan}Type 'help' for commands, 'exit' to quit${colors.reset}\n`);

    const rl = readline.createInterface({
        input: process.stdin,
        output: process.stdout,
        prompt: `${colors.blue}surge> ${colors.reset}`
    });

    rl.prompt();

    rl.on('line', async (line) => {
        const parts = line.trim().split(/\s+/);
        const cmd = parts[0]?.toLowerCase();

        if (!cmd) {
            rl.prompt();
            return;
        }

        try {
            switch (cmd) {
                case 'exit':
                case 'quit':
                case 'q':
                    console.log(`${colors.cyan}Goodbye!${colors.reset}`);
                    rl.close();
                    return;

                case 'help':
                case 'h':
                case '?':
                    printInteractiveHelp();
                    break;

                case 'load':
                case 'preset':
                    if (parts.length < 2) {
                        console.error(`${colors.red}Usage: load <preset name>${colors.reset}`);
                    } else {
                        const presetName = parts.slice(1).join(' ');
                        await loadPresetCommand(controller, presetName);
                    }
                    break;

                case 'list':
                case 'ls':
                    await listPresetsCommand(controller);
                    break;

                case 'set':
                case 'param':
                    if (parts.length < 3) {
                        console.error(`${colors.red}Usage: set <param_id> <value>${colors.reset}`);
                    } else {
                        await setParameterCommand(controller, parts[1], parts[2]);
                    }
                    break;

                case 'ping':
                    await pingCommand(controller);
                    break;

                default:
                    console.error(`${colors.red}Unknown command: ${cmd}${colors.reset}`);
                    console.log(`Type 'help' for available commands`);
            }
        } catch (error) {
            console.error(`${colors.red}Error: ${error.message}${colors.reset}`);
        }

        rl.prompt();
    });

    rl.on('close', () => {
        process.exit(0);
    });
}

function printInteractiveHelp() {
    console.log(`
${colors.bright}Available Commands:${colors.reset}
  ${colors.yellow}load <name>${colors.reset}     Load a preset by name
  ${colors.yellow}list${colors.reset}            List all available presets
  ${colors.yellow}set <id> <val>${colors.reset}  Set a parameter value
  ${colors.yellow}ping${colors.reset}            Check connection status
  ${colors.yellow}help${colors.reset}            Show this help message
  ${colors.yellow}exit${colors.reset}            Exit interactive mode
`);
}

function printUsage() {
    console.log(`
${colors.bright}Surge XT TCP Control CLI${colors.reset}

${colors.bright}Usage:${colors.reset}
  surge-control <command> [options]

${colors.bright}Commands:${colors.reset}
  ${colors.yellow}preset <name>${colors.reset}        Load a preset by name
  ${colors.yellow}load <name>${colors.reset}          Same as preset
  ${colors.yellow}list${colors.reset}                 List all available presets
  ${colors.yellow}param <id> <value>${colors.reset}   Set a parameter value
  ${colors.yellow}set <id> <value>${colors.reset}     Same as param
  ${colors.yellow}ping${colors.reset}                 Check if Surge TCP control is running
  ${colors.yellow}interactive${colors.reset}          Enter interactive mode
  ${colors.yellow}i${colors.reset}                    Same as interactive

${colors.bright}Examples:${colors.reset}
  surge-control preset "Deep Bass"
  surge-control list
  surge-control param cutoff 0.75
  surge-control interactive

${colors.bright}Note:${colors.reset}
  Surge XT must be running with TCP control enabled on port ${SURGE_PORT}.
`);
}

// Run the main function
main().catch(console.error);