# Real-Time Active Noise Cancellation (ANC) System using ESP32

An embedded Active Noise Cancellation (ANC) system designed for Electronics & Communication (EC) engineering, utilizing destructive acoustic interference to attenuate low-frequency ambient environmental noise in real-time.

---

## 🛠️ Hardware Architecture

| Component | Specification | Function |
| :--- | :--- | :--- |
| **Microcontroller** | ESP32 DevKit V1 (Dual-Core 240 MHz) | DSP & Real-Time Anti-Noise Engine |
| **Acoustic Sensor** | INMP441 MEMS Omnidirectional Microphone | Reference Noise Input (I2S Digital) |
| **Power Amplifier** | MAX98357A 3.2W Class-D Mono I2S Amp | Audio Output Driver |
| **Acoustic Transducer** | 4-Inch DY VOICE Woofer (4Ω, 20W) | Inverted Acoustic Wave Emitter |
| **Power Supply** | USB 5V / 3.7V Li-ion Battery | System Power |

---

## 📐 Circuit Wiring & Pinout

### 1. INMP441 Microphone &rarr; ESP32 (I2S_NUM_0)
* **VDD** &rarr; `3.3V` *(Strictly 3.3V)*
* **GND** &rarr; `GND`
* **SD** &rarr; `GPIO 32` (Serial Data)
* **WS** &rarr; `GPIO 27` (Word Select / Left-Right Clock)
* **SCK** &rarr; `GPIO 14` (Serial Clock)
* **L/R** &rarr; `GND` (Left Channel selection)

### 2. MAX98357A Amplifier &rarr; ESP32 (I2S_NUM_1)
* **VIN** &rarr; `5V / VIN`
* **GND** &rarr; `GND`
* **DIN** &rarr; `GPIO 22` (Data In)
* **BCLK** &rarr; `GPIO 26` (Bit Clock)
* **LRC** &rarr; `GPIO 25` (Left/Right Word Clock)
* **SD** &rarr; `VIN` (Active / Keep Awake)
* **GAIN** &rarr; *Unconnected (Default 9 dB)*
* **OUT+ & OUT-** &rarr; 4-Inch Woofer screw terminals

---

## 🔬 Scientific Principle: Destructive Interference

Active Noise Cancellation relies on superposition of acoustic pressure waves:
$$\text{Residual Sound: } e(n) = x(n) + y(n)$$

Where:
* $x(n)$ is the incoming ambient noise wave.
* $y(n) = -w \cdot x(n - \Delta)$ is the generated anti-noise wave (180° inverted phase with acoustic delay compensation $\Delta$).
* When acoustic waves collide in the room:
$$x(n) + (-x(n)) \approx 0 \implies \text{Noise Cancellation}$$

---

## 💻 Control Software & Web Dashboard

The project includes an interactive web dashboard running on Python standard library:
* **Real-time Dual Oscilloscope:** Live visualization of the incoming noise wave (Cyan) vs anti-noise wave (Pink) vs residual sound (Green).
* **Metrics:** Live RMS, dB SPL level, and Attenuation metrics (-dB).
* **Controls:** 1-Click ANC ON/OFF toggle, Adaptive Gain slider, and Acoustic Delay compensation slider.
* **Component Schematic:** Interactive animated wiring diagram showing online status of all 4 modules.

### How to Run Dashboard:
1. Double click `Run_ANC_Dashboard.bat` (or run `python dashboard/server.py`).
2. Open your browser to `http://127.0.0.1:3000`.

---

## 📂 Project Structure

```text
├── anc_firmware/           # Core Production ANC Firmware (ESP32)
│   └── anc_firmware.ino
├── dashboard/              # Live Visualizer & Control Software
│   ├── server.py           # Python backend & Serial Bridge
│   └── web/
│       └── index.html      # Responsive Dark-Mode UI & Canvas Oscilloscope
├── Run_ANC_Dashboard.bat   # 1-Click Windows Launcher
├── amp_test/               # Diagnostic unit test: Amplifier & Woofer
├── mic_test/               # Diagnostic unit test: INMP441 Microphone
├── quick_test/             # Diagnostic unit test: Full System Self-Check
└── README.md
```
