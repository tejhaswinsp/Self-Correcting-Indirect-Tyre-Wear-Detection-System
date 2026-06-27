# 🚗 Self-Correcting Indirect Tyre Wear Detection System

> **A smart, low-cost system that detects tyre wear indirectly and automatically corrects vehicle drift using wheel speed and IMU data.**

---

## 🌟 About the Project

Tyres wear out over time, reducing their **effective rolling radius**. This can cause a vehicle to drift even when moving in a straight line.

Instead of using expensive tyre-mounted sensors, our project estimates tyre wear **indirectly** by comparing wheel speeds and monitoring the vehicle's motion.

The system then **automatically adjusts motor speeds** to keep the vehicle moving straight.

---

## ✨ Features

✅ Indirect tyre wear detection

✅ Automatic drift correction

✅ Real-time wheel RPM monitoring

✅ IMU-based yaw measurement

✅ Bluetooth communication

✅ Adaptive motor speed control

---

## ⚙️ How It Works

```text
Wheel Encoders
      │
      ▼
 Measure Wheel RPM
      │
      ▼
 Estimate Tyre Radius
      │
      ▼
 MPU6050 detects Yaw Drift
      │
      ▼
 Calculate Correction
      │
      ▼
 Adjust Left & Right Motor Speed
      │
      ▼
 Vehicle Moves Straight
```

---

## 📐 Principle Used

The system is based on the simple relation

[
v = R\omega
]

Where

* **v** → Linear velocity
* **R** → Tyre radius
* **ω** → Angular velocity (RPM)

Since all wheels move at nearly the same vehicle speed,

[
R_1\omega_1 = R_2\omega_2
]

Using the measured RPM values, the system estimates the **effective rolling radius** of each tyre.

A smaller estimated radius indicates possible tyre wear.

---

## 🛠 Hardware Used

| 🔧 Component     | Purpose                   |
| ---------------- | ------------------------- |
| ESP32            | Main Controller           |
| MPU6050          | Gyroscope & Accelerometer |
| Optical Encoders | Measure Wheel RPM         |
| Motor Driver     | Drive DC Motors           |
| DC Motors        | Vehicle Movement          |
| Bluetooth        | Wireless Communication    |

---

## 💻 Software

* Arduino IDE
* C++
* ESP32 Arduino Core
* BluetoothSerial Library
* Wire (I2C)

---

## 🚀 Workflow

```text
Start
   │
   ▼
Bluetooth "GO"
   │
   ▼
Sensor Calibration
   │
   ▼
Run Vehicle
   │
   ▼
Read RPM + IMU
   │
   ▼
Estimate Tyre Radius
   │
   ▼
Detect Drift
   │
   ▼
Correct Motor PWM
   │
   ▼
Display Results
```

---

## 📱 Sample Output

```text
Run #2

Yaw : -1.85°

Correction
Left  : +3
Right : -3

Wheel RPM

FR : 125.4
FL : 123.8
RL : 126.1
RR : 124.7

Estimated Tyre Radius

FR : 3.22 cm
FL : 3.19 cm
RL : 3.25 cm
RR : 3.21 cm
```

---

## 🎯 Applications

🚘 Electric Vehicles

🤖 Autonomous Robots

🛞 Predictive Tyre Maintenance

🏭 Industrial AGVs

📚 Embedded Systems Learning

---

## 👨‍💻 Team

* **Tejhaswin S P**
* **Roshan V**
* **Harishram R V**
* **Vijay Surya**
* **Pranav Parasuram**
* **Shri Rishabh**

### 👨‍🏫 Mentor

**Dr. Karthik C**

---

<img width="1080" height="835" alt="image" src="https://github.com/user-attachments/assets/d55798e2-a674-4d4b-9921-7b831ad76a98" />


Give the repository a **⭐ Star** and feel free to fork it, improve it, or use it for learning and research.

**Made with ❤️ using ESP32, MPU6050, and Embedded Systems.**
