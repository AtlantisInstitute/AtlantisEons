# Touch Input System Setup Guide

## Overview
The Touch Input System provides mobile-friendly button controls for your AtlantisEons project. It allows touch devices to use on-screen buttons for:
- **Menu Button**: Opens inventory
- **Weapon Button**: Triggers attack
- **Dodge Button**: Activates dash
- **Shield Button**: Activates/deactivates block

## System Architecture

### Components
1. **TouchInputManager** (C++ Component)
   - Handles touch input detection and button binding
   - Automatically enables on mobile platforms
   - Provides cooldown protection against button spam

2. **WBP_SecondaryHUD** (Widget Blueprint)
   - Contains the visual UI buttons
   - Must include Button widgets with specific names

3. **AtlantisEonsCharacter** (C++ Character)
   - Owns the TouchInputManager component
   - Initializes the system when SecondaryHUD is created

## Setup Instructions

### Step 1: Enable Touch Input in Project Settings
1. Open **Project Settings** → **Input**
2. Set **Always Show Touch Interface** to `True`
3. Set **Show Console on Four Finger Tap** to `True` (optional)

### Step 2: Configure WBP_SecondaryHUD Blueprint
1. Open **WBP_SecondaryHUD** Blueprint in the editor
2. Add Button widgets to your HUD with these **exact names**:
   - `MenuButton` (for inventory)
   - `WeaponButton` (for attack)
   - `DodgeButton` (for dash)
   - `ShieldButton` (for block)

3. Position the buttons appropriately for touch devices
4. Set button styles and appearance as desired

### Step 3: Configure BP_Character Blueprint
1. Open **BP_Character** Blueprint
2. In the **Details Panel**, find **Components** section
3. Locate **Touch Input Manager Comp**
4. Configure these settings:
   - **Touch Input Enabled**: `True`
   - **Auto Enable On Mobile**: `True`
   - **Button Press Cooldown**: `0.1` (seconds)

### Step 4: Test the System
1. Compile the project
2. Run on a mobile device or enable touch emulation
3. Verify that buttons appear and respond to touch

## Button Functionality

### Menu Button
- **Action**: Opens the inventory system
- **Function Called**: `OwnerCharacter->OpenInventory()`
- **Cooldown**: Configurable (default 0.1s)

### Weapon Button  
- **Action**: Triggers a melee attack
- **Function Called**: `OwnerCharacter->MeleeAttack()`
- **Cooldown**: Configurable (default 0.1s)

### Dodge Button
- **Action**: Activates dash/dodge movement
- **Function Called**: Updates input state to trigger Blueprint dash logic
- **Cooldown**: Configurable (default 0.1s)
- **Implementation**: Temporarily sets dash key state to `true`, then resets after 0.1s

### Shield Button
- **Action**: Toggles blocking state
- **Function Called**: `OwnerCharacter->PerformBlock()` / `OwnerCharacter->ReleaseBlock()`
- **Behavior**: Toggle on/off with each press
- **Cooldown**: Configurable (default 0.1s)

## Advanced Configuration

### Button Name Alternatives
If you prefer different button names, the system will search for these alternatives:
- `MenuButton`, `Menu_Button`, `Btn_Menu`
- `WeaponButton`, `Weapon_Button`, `Btn_Weapon`
- `DodgeButton`, `Dodge_Button`, `Btn_Dodge`
- `ShieldButton`, `Shield_Button`, `Btn_Shield`

### Manual Touch Input Control
You can manually control touch input via Blueprint or C++:

```cpp
// Enable touch input
TouchInputManagerComp->SetTouchInputEnabled(true);

// Disable touch input
TouchInputManagerComp->SetTouchInputEnabled(false);

// Check if touch input is enabled
bool bIsEnabled = TouchInputManagerComp->IsTouchInputEnabled();
```

### Platform Detection
The system automatically detects mobile platforms:
- **Android**: Automatically enabled
- **iOS**: Automatically enabled  
- **Desktop**: Disabled by default (can be manually enabled)

### Debugging
The system provides extensive logging. Look for these log messages:
- `🎮 TouchInputManager:` - General system messages
- `📱 Menu button pressed` - Menu button activation
- `⚔️ Weapon button pressed` - Weapon button activation
- `💨 Dodge button pressed` - Dodge button activation
- `🛡️ Shield button pressed` - Shield button activation

## Troubleshooting

### Buttons Not Responding
1. **Check Button Names**: Ensure button widgets have correct names
2. **Check Binding**: Verify buttons are properly bound in Widget Blueprint
3. **Check Touch Input Enabled**: Verify `bTouchInputEnabled` is `true`
4. **Check Platform**: Manual enable may be needed on desktop

### Buttons Not Found
1. **Check Widget Tree**: Verify buttons exist in WBP_SecondaryHUD
2. **Check meta=(BindWidget)**: Ensure button properties have BindWidget meta tag
3. **Review Logs**: Check for "Button search results" log messages

### System Not Initializing
1. **Check SecondaryHUD Creation**: Verify SecondaryHUDClass is set in BP_Character
2. **Check Component Creation**: Verify TouchInputManagerComp exists in character
3. **Check Initialization**: Look for "Touch input manager initialized" log message

## Mobile Deployment Considerations

### Performance
- Button cooldowns prevent spam and improve performance
- Touch input detection is lightweight
- No performance impact when disabled

### UI Scaling
- Design buttons for different screen sizes
- Use anchors and responsive layout
- Test on various device resolutions

### Input Conflicts
- Touch buttons work alongside keyboard/gamepad input
- No conflicts with existing input system
- Touch input can be enabled/disabled dynamically

## Example Usage in Blueprint

You can also control the touch input system from Blueprint:

1. **Get Touch Input Manager Component**
2. **Call Set Touch Input Enabled**
3. **Configure Button Press Cooldown as needed**

The system is designed to be completely optional - your game will work normally without it, but gains touch functionality when the system is properly configured. 