# Smart Fire, Gas, and Environment Safety Monitoring System Using STM32F103C8T6

**A Multi-Sensor Embedded Safety Monitoring Prototype**

Department of Electrical and Computer Engineering (ECE), School of Engineering and Physical Sciences (SEPS)
North South University, Bashundhara, Dhaka-1229, Bangladesh

**Course:** CSE331L — Microprocessor Interfacing & Embedded Systems Lab
**Faculty:** Mosabber Uddin Ahmed (MUA3) | **Lab Instructor:** Moshiur Rahman
**Section:** 08 (Group 10) | **Semester:** Summer 26

**Team Members**

| Name | Student ID |
|---|---|
| Sazzad Hossain | 2211415042 |
| Md. Yousuf | 2211461642 |
| Md Rakibul Hasan | 2212346042 |
| Md. Nafees Ahommed | 2111934642 |

---

## 1. Overview

This project is a real-time, multi-sensor embedded safety monitoring system built around the **STM32F103C8T6 (Blue Pill)** microcontroller. Instead of relying on a single detector for a single hazard, the system reads **four sensors at once** — temperature/humidity, distance/presence, flame, and gas/smoke — and fuses them into one combined **SAFE / WARNING** decision, shown live on a 128x64 I2C OLED and signaled instantly through a buzzer and Red/Green status LEDs.

The firmware is written in bare-metal **C using STM32 HAL**, generated and configured with **STM32CubeMX / STM32CubeIDE**.

### 1.1 What the system does

- Continuously measures **temperature & humidity** using a DHT11 sensor.
- Continuously measures **distance/presence** using an HC-SR04 ultrasonic sensor (obstacle threshold: **50 cm**).
- Continuously checks for a **direct flame** using a flame sensor module (digital output).
- Continuously checks for **smoke / flammable gas** using an MQ-2 sensor (digital output).
- Combines all readings into a single system status, updated **every 500 ms**.
- Displays live sensor values and alerts on a **0.96" SSD1306 I2C OLED**.
- Drives a **buzzer** and **Red/Green LEDs** to give an instant audible + visual warning.

---

## 2. Hardware Components (Bill of Materials)

| # | Component | Qty | Purpose | Approx. Cost (Tk.) |
|---|---|---|---|---|
| 1 | STM32F103C8T6 (Blue Pill) | 1 | Central microcontroller (GPIO, Timer, I2C) | 250 – 350 |
| 2 | ST-LINK V2 Programmer | 1 | Flashing and debugging the STM32 | 300 – 450 |
| 3 | DHT11 Sensor | 1 | Temperature & humidity | 120 – 150 |
| 4 | HC-SR04 Ultrasonic Sensor | 1 | Distance / presence detection | 90 – 120 |
| 5 | Flame Sensor Module (digital out) | 1 | Direct flame detection | 60 – 70 |
| 6 | MQ-2 Gas/Smoke Sensor (digital DO out) | 1 | Smoke & flammable gas detection | 140 – 160 |
| 7 | 0.96" SSD1306 I2C OLED (128x64) | 1 | Live sensor & status display | 250 – 350 |
| 8 | Active Buzzer | 1 | Audible alarm | 30 – 50 |
| 9 | Red LED | 1 | Warning indicator | ~10 |
| 10 | Green LED | 1 | Safe indicator | ~10 |
| 11 | 220 Ω Resistor | 2 | Current-limiting for LEDs | ~15 |
| 12 | Breadboard (830-point) | 1 | Prototyping platform | ~140 |
| 13 | Jumper Wires | As required | Wiring all modules | ~185 |

**Estimated Total Project Cost:** Tk. 1,595 – 2,085 (varies by shop and component quality)

> **Note:** The MQ-2 and the flame sensor modules in this build are read on their **digital (DO) output only** — no analog/ADC wiring or voltage divider is required. If your MQ-2 module also breaks out an analog (AO) pin, it is simply left unconnected in this design.

---

## 3. Complete Pin Configuration (STM32F103C8T6 / Blue Pill)

This is the **exact pin map used in the firmware** (`Core/Src/main.c`), matching the CubeMX `331.ioc` configuration.

| STM32 Pin | Peripheral / Mode | Connected To | Direction | Notes |
|---|---|---|---|---|
| **PA8** | GPIO Input, Pull-down | HC-SR04 **ECHO** | Input | Reads echo pulse; timed with TIM1 |
| **PA9** | GPIO Output (Push-Pull, High speed) | HC-SR04 **TRIG** | Output | Sends 10 µs trigger pulse |
| **PA10** | GPIO Output (Push-Pull, High speed) | **Buzzer** (+) signal pin | Output | Active HIGH = buzzer ON |
| **PA11** | GPIO Output (Push-Pull) | **Red LED** anode (via 220 Ω resistor) | Output | Active HIGH = Red ON |
| **PA12** | GPIO Output (Push-Pull) | **Green LED** anode (via 220 Ω resistor) | Output | Active HIGH = Green ON |
| **PA13** | SWDIO (SYS) | ST-LINK V2 SWDIO | — | Programming/debug only, do not reuse |
| **PA14** | SWCLK (SYS) | ST-LINK V2 SWCLK | — | Programming/debug only, do not reuse |
| **PB0** | GPIO Input, Pull-up | **Flame Sensor** DO (digital out) | Input | Sensor pulls LOW when flame is detected (active-LOW) |
| **PB1** | GPIO Input, Pull-up | **MQ-2 Gas Sensor** DO (digital out) | Input | Sensor pulls LOW when gas/smoke exceeds its onboard threshold (active-LOW) |
| **PB6** | I2C1_SCL (Fast mode, 400 kHz) | OLED **SCL** | I2C Clock | Add 4.7 kΩ pull-up if your OLED breakout has none |
| **PB7** | I2C1_SDA (Fast mode, 400 kHz) | OLED **SDA** | I2C Data | Add 4.7 kΩ pull-up if your OLED breakout has none |
| **PB9** | GPIO Output→Input (bit-banged, Pull-up, High speed) | **DHT11** Data pin | Bidirectional | Single-wire protocol, idle HIGH |
| **PD0 (OSC_IN)** | HSE Oscillator In | 8 MHz crystal (on-board, Blue Pill) | — | Already present on the Blue Pill board |
| **PD1 (OSC_OUT)** | HSE Oscillator Out | 8 MHz crystal (on-board, Blue Pill) | — | Already present on the Blue Pill board |
| **3V3** | Power | VCC of DHT11, HC-SR04\*, Flame sensor, MQ-2, OLED | Power | See power note below |
| **5V** | Power (from ST-LINK/USB) | HC-SR04 VCC (recommended) | Power | HC-SR04 is typically a 5 V module |
| **GND** | Ground | Common ground for all modules | Power | All GND pins must be tied together |

\* Most **HC-SR04** modules are rated for 5 V and work more reliably at 5 V, but the Blue Pill's GPIO pins are **not 5 V-tolerant on all pins** — this firmware uses a **pull-down on the ECHO line (PA8)** and treats the HC-SR04 as a 3.3V-compatible signal. If you power the HC-SR04 from 5V, it is strongly recommended to use a simple resistor divider (e.g., 1 kΩ + 2 kΩ) on the ECHO line before PA8 to bring the 5V echo signal down to a safe ~3.3V for the STM32 input.

### 3.1 LED / Buzzer polarity logic (as implemented)

- **fireDetected** = `PB0 reads LOW` (flame sensor active-low)
- **gasDetected** = `PB1 reads LOW` (MQ-2 DO active-low)
- **obstacleDetected** = `Distance < 50 cm` measured on HC-SR04
- If **any** of the above is true → **Red LED ON, Green LED OFF, Buzzer ON** (Warning state)
- If **none** are true → **Green LED ON, Red LED OFF, Buzzer OFF** (Safe state)

> In this working firmware, the alarm logic uses **two states (SAFE / WARNING)** with **two LEDs (Red/Green)**, and the OLED screen shows *which* hazard triggered the warning (Fire / Gas / Obstacle) with a full-screen alert and a blinking border. This is a simplified, tested version of the three-tier SAFE/WARNING/CRITICAL concept described in the original project proposal; a third (yellow) LED can be added later on any free GPIO (e.g., PB10, PB12, PB13, PB14, PB15) to fully implement the three-tier scheme.

---

## 4. Wiring / Connection Guide (Step-by-Step)

### 4.1 Power rails first
1. Connect the Blue Pill **3V3** pin to the breadboard's positive rail (for 3.3V devices).
2. Connect the Blue Pill **GND** pin to the breadboard's negative/ground rail.
3. If you power the HC-SR04 from 5V, bring a separate 5V rail from the ST-LINK's 5V pin (or a USB 5V source) and **keep grounds common**.

### 4.2 DHT11 (Temperature & Humidity)
| DHT11 Pin | Connect To |
|---|---|
| VCC | 3V3 |
| GND | GND |
| DATA (single wire) | **PB9** |

*(If your DHT11 breakout does not already include a pull-up resistor, add a 4.7 kΩ–10 kΩ resistor between DATA and VCC.)*

### 4.3 HC-SR04 (Ultrasonic Distance)
| HC-SR04 Pin | Connect To |
|---|---|
| VCC | 5V (recommended) or 3V3 |
| GND | GND |
| TRIG | **PA9** |
| ECHO | **PA8** (use a voltage divider if VCC = 5V) |

### 4.4 Flame Sensor Module
| Flame Sensor Pin | Connect To |
|---|---|
| VCC | 3V3 |
| GND | GND |
| DO (digital out) | **PB0** |
| AO (analog out) | Not used / leave unconnected |

*Tip: turn the onboard potentiometer on the flame sensor module to adjust detection sensitivity/distance.*

### 4.5 MQ-2 Gas/Smoke Sensor
| MQ-2 Pin | Connect To |
|---|---|
| VCC | 3V3 (or 5V, per your module's spec) |
| GND | GND |
| DO (digital out) | **PB1** |
| AO (analog out) | Not used in this firmware |

*Tip: turn the onboard potentiometer on the MQ-2 module to set the gas concentration threshold at which DO switches. Let the sensor warm up for 1–2 minutes after power-on for stable readings.*

### 4.6 0.96" I2C OLED (SSD1306)
| OLED Pin | Connect To |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SCL | **PB6** |
| SDA | **PB7** |

*I2C address used in firmware: `0x78` (7-bit address `0x3C`, already set in `ssd1306.h`).*

### 4.7 Buzzer
| Buzzer Pin | Connect To |
|---|---|
| + (Signal) | **PA10** |
| − (GND) | GND |

### 4.8 Red & Green LEDs
| LED | Connect To |
|---|---|
| Red LED Anode | **PA11** → through a 220 Ω resistor |
| Red LED Cathode | GND |
| Green LED Anode | **PA12** → through a 220 Ω resistor |
| Green LED Cathode | GND |

### 4.9 ST-LINK V2 Programmer
| ST-LINK Pin | Blue Pill Pin |
|---|---|
| SWDIO | **PA13** |
| SWCLK | **PA14** |
| GND | GND |
| 3V3 | 3V3 (optional, if powering the board from ST-LINK) |

---

## 5. Software / Firmware

### 5.1 Toolchain
- **IDE:** STM32CubeIDE (project files: `.project`, `.cproject`, `331.ioc`)
- **HAL Library:** STM32Cube FW_F1 (STM32F1 HAL Driver)
- **Language:** C
- **Programmer/Debugger:** ST-LINK V2
- **Clock:** External 8 MHz crystal (HSE) → PLL → 72 MHz system clock

### 5.2 Project structure

```
├── 331.ioc                     # STM32CubeMX pin/peripheral configuration
├── Core/
│   ├── Inc/
│   │   ├── main.h
│   │   ├── ssd1306.h            # OLED driver header (I2C address, fonts config)
│   │   ├── fonts.h
│   │   └── stm32f1xx_hal_conf.h
│   └── Src/
│       ├── main.c               # All sensor reading + decision logic + display
│       ├── ssd1306.c             # OLED (SSD1306) driver
│       ├── fonts.c
│       └── stm32f1xx_it.c        # Interrupt handlers
├── Drivers/                     # STM32 HAL + CMSIS drivers (auto-generated)
├── Debug/                       # Build output (.elf, .hex, .bin, .map)
└── STM32F103C8TX_FLASH.ld       # Linker script
```

### 5.3 Peripherals used

| Peripheral | Used For |
|---|---|
| **GPIO** | All sensor inputs, LED/buzzer outputs, DHT11 bit-banging |
| **TIM1** (Prescaler = 71, 1 MHz tick) | Microsecond delay generation (`microDelay()`) — used for DHT11 timing and HC-SR04 pulse-width measurement |
| **I2C1** (Fast mode, 400 kHz) | Communication with the SSD1306 OLED |
| **SysTick / HAL_GetTick()** | Millisecond timing, 500 ms sensor sampling loop, timeouts |
| **SWD** | Flashing & debugging via ST-LINK |

### 5.4 How each sensor is read (as implemented in `main.c`)

**Flame Sensor (PB0):**
```c
fireDetected = !HAL_GPIO_ReadPin(FLAME_PORT, FLAME_PIN); // active LOW
```

**MQ-2 Gas Sensor (PB1):**
```c
gasDetected = !HAL_GPIO_ReadPin(GAS_PORT, GAS_PIN); // active LOW
```

**HC-SR04 Ultrasonic (PA9 = TRIG, PA8 = ECHO):**
1. Set TRIG HIGH, wait 10 µs (via TIM1), set TRIG LOW.
2. Wait for ECHO to go HIGH (start of pulse) — timeout 10 ms.
3. Capture TIM1 counter value (`Value1`).
4. Wait for ECHO to go LOW (end of pulse) — timeout 60 ms.
5. Capture TIM1 counter value (`Value2`).
6. `Distance (cm) = (Value2 - Value1) * 0.034 / 2`
7. If `Distance < 50 cm` → `obstacleDetected = 1`.

**DHT11 (PB9, single-wire, bit-banged):**
1. Pin reconfigured as output, pulled LOW for 20 ms (start signal), then HIGH for 30 µs, then reconfigured as input.
2. The DHT11 responds with an 80 µs LOW + 80 µs HIGH acknowledgment pulse.
3. 5 bytes (40 bits) are read: Humidity Integer, Humidity Decimal, Temperature Integer, Temperature Decimal, Checksum.
4. Data is validated: `RHI + RHD + TCI + TCD == SUM`; only then is the reading marked valid and used.

**OLED (SSD1306, I2C1):**
- Boot screen shown for 3 seconds on power-up.
- Normal screen shows: Distance, Temperature (°C), Humidity (%), Temperature (°F), and system status.
- Full-screen alert layouts (with a blinking border) are shown for **FIRE**, **GAS LEAK**, and **OBSTACLE** conditions, each also showing the live temperature/humidity/distance.

### 5.5 Main control flow (as implemented)

```
START
 → HAL_Init(), SystemClock_Config() [72 MHz from 8 MHz HSE + PLL]
 → MX_GPIO_Init(), MX_I2C1_Init(), MX_TIM1_Init()
 → Initialize DHT11 pin (output, idle HIGH)
 → Initialize OLED (SSD1306_Init), show boot screen (3 s)
 → LED self-test (Red 0.5s → Green ON)
 LOOP (every 500 ms):
     ReadAllSensors()        // Flame, Gas, HC-SR04, DHT11
     ControlIndicators()     // Red/Green LED + Buzzer logic
     UpdateDisplay()         // Draw OLED screen based on current state
 → HAL_Delay(10) between loop iterations
END LOOP (runs forever)
```

### 5.6 Decision logic (as implemented)

```
IF fireDetected OR gasDetected OR obstacleDetected:
     RED LED   = ON
     GREEN LED = OFF
     BUZZER    = ON
     OLED      = Full-screen alert (FIRE / GAS LEAK / OBSTACLE), blinking border
ELSE:
     RED LED   = OFF
     GREEN LED = ON
     BUZZER    = OFF
     OLED      = Normal live-data screen, "System: SAFE"
```

---

## 6. Building & Flashing the Firmware

1. Install **STM32CubeIDE** (free, from ST's website).
2. Clone this repository:
   ```bash
   git clone https://github.com/Yousufshishir/Yousufshishir-Smart-Fire-Gas-and-Environment-Safety-Monitoring-System-Using-STM32F103C8T6.git
   ```
3. Open **STM32CubeIDE** → `File` → `Open Projects from File System...` → select the cloned folder.
4. (Optional) Open `331.ioc` in the CubeMX perspective to review or edit pin/peripheral configuration.
5. Connect the **ST-LINK V2** to the Blue Pill's SWD pins (**PA13 = SWDIO, PA14 = SWCLK, GND, 3V3**).
6. Click **Build** (hammer icon) to compile the project.
7. Click **Run/Debug** (or use `Run As → STM32 C/C++ Application`) to flash the `.elf`/`.hex` to the board.
8. Power-cycle or reset the board — the OLED boot screen should appear within a few seconds.

---

## 7. Testing the System

| Test | How | Expected Result |
|---|---|---|
| Power-on | Apply power | OLED shows boot screen (3s) → Red LED blinks 0.5s → Green LED ON |
| Safe state | No hazard present | Green LED ON, Buzzer OFF, OLED shows Distance/Temp/Humidity/Status: SAFE |
| Flame test | Bring a lit lighter/match near the flame sensor (briefly, safely) | Red LED ON, Buzzer ON, OLED shows "!!! FIRE !!! EVACUATE NOW!" |
| Gas test | Bring a lighter (unlit, gas released) or alcohol swab near MQ-2 | Red LED ON, Buzzer ON, OLED shows "!! GAS LEAK !! GAS DETECTED" |
| Obstacle test | Place an object within 50 cm of the HC-SR04 | Red LED ON, Buzzer ON, OLED shows "! OBSTACLE !" with distance |
| Temp/Humidity | Compare OLED reading with a reference thermometer/hygrometer | Values within DHT11's rated accuracy (±2°C, ±5% RH) |

---

## 8. Troubleshooting

| Symptom | Likely Cause | Fix |
|---|---|---|
| OLED stays blank | Wrong I2C wiring or address | Check PB6=SCL, PB7=SDA, confirm address `0x78` in `ssd1306.h`, check pull-ups |
| DHT11 always reads `--` | Wrong data pin, no pull-up, or module needs 2s between reads | Confirm PB9 wiring, add 4.7–10kΩ pull-up, don't read faster than ~1 Hz |
| HC-SR04 distance always 0 or invalid | ECHO not wired to PA8, or 5V echo damaging the pin | Add echo voltage divider, re-check TRIG=PA9 / ECHO=PA8 |
| Flame/Gas always triggers | Sensitivity potentiometer set too high | Adjust the onboard potentiometer on the sensor module |
| Flame/Gas never triggers | Sensitivity too low, or DO wiring reversed | Adjust potentiometer; confirm DO pin, not AO, is wired |
| Can't flash via ST-LINK | Wrong SWD wiring or driver not installed | Check PA13/PA14/GND/3V3, install ST-LINK USB driver |

---

## 9. Expected Outcomes

- Real-time acquisition from all four sensors on the STM32F103C8T6.
- Live temperature and humidity display via DHT11 on the OLED.
- Real-time distance measurement and 50 cm presence/obstacle detection via HC-SR04.
- Direct flame detection and instant fire warning via the flame sensor.
- Early smoke/gas detection via the MQ-2 digital output.
- A single fused SAFE/WARNING system status shown continuously on the OLED.
- Immediate buzzer + LED response on any abnormal condition.
- A complete, low-cost, working Input → Processing → Output embedded pipeline suitable for basic indoor fire and environmental safety monitoring.

---

## 10. Future Improvements

- Add a third (Yellow) LED on a free GPIO pin (e.g., PB10) to fully implement the SAFE / WARNING / CRITICAL three-tier scheme from the original proposal.
- Add MQ-2 analog (AO) reading via ADC1 for graded gas-concentration levels instead of a single digital threshold.
- Add data logging or wireless reporting (UART/Bluetooth/Wi-Fi module) for remote monitoring.
- Add a push-button to silence/acknowledge the buzzer.
- Add battery power with a voltage regulator for portable operation.

---

## 11. License / Academic Note

This project was developed as coursework for **CSE331L: Microprocessor Interfacing & Embedded Systems Lab**, North South University, Summer 2026, Section 08, Group 10. It is intended for academic and educational use.
