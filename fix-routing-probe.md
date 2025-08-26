# Routing Probe Fix

## Current Issue
The routing probe tries to read VST parameters 900-903 to get plugin signatures, but gets 0 values.

## The Real Solution
**We don't need VST parameters!** Surge already sends its plugin signature in the registration message.

## What's Working
1. ✅ Surge connects to broker at `/tmp/sas-plugin-router.sock`
2. ✅ Surge sends registration with `plugin_sig: 0000ee62-0004-00002fbe344c9589`
3. ✅ Broker receives the plugin signatures

## What Needs Fixing
The routing probe should:
1. Listen for Surge registrations at the broker
2. Note the plugin signatures
3. Query REAPER for track/FX info
4. Send routing updates matching signatures to tracks

## Simplified Architecture
```
Surge Instance 1 ──────┐
  (sig: 0000ee62-0003) │
                       ▼
Surge Instance 2 ──► BROKER ◄── Routing Probe
  (sig: 0000ee62-0004) ▲         (maps sigs to tracks)
                       │
                    Assistant
                 (sends commands)
```

## Implementation
Instead of reading VST params, the routing probe should:
```javascript
// Listen to broker for plugin registrations
broker.on('plugin_registered', (plugin) => {
  if (plugin.plugin_type === 'surge-xt-sas') {
    // Track this plugin signature
    knownPlugins.set(plugin.plugin_sig, plugin);
    
    // Query REAPER for track info
    const trackInfo = await getTrackInfoForPlugin(plugin);
    
    // Send routing update
    broker.sendRoutingUpdate({
      plugin_sig: plugin.plugin_sig,
      routing: `${projectGuid}/${trackGuid}/${fxGuid}`
    });
  }
});
```

This eliminates the need for VST parameter reading entirely!