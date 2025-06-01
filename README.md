# SensorFusion 🧭✨

A simple Arduino-based Sensor Fusion project combining data from multiple sensors (e.g., accelerometer, gyroscope, magnetometer) to estimate orientation and movement with improved accuracy.

## 🔧 Features
- Read data from IMU sensors like MPU6050 or MPU9250
- Fuse accelerometer + gyroscope + magnetometer data
- Calculate roll, pitch, and yaw
- Simple serial output for real-time data monitoring
- Lightweight and optimized for AVR boards like Arduino Uno, Mega, etc.

## 📁 Project Structure

my-sketch/
├── my-sketch.ino # Main Arduino sketch
├── fusion.h / fusion.cpp # (Optional) Fusion algorithm implementation
└── README.md # This file

SensorFusion/
├── LICENSE
├── README.md
├── CHANGELOG.md
├── sketches/
│   ├── imu-test/
│   │   └── imu-test.ino
│   ├── kalman-filter/
│   │   └── kalman-filter.ino
│   ├── complementary-filter/
│   │   └── complementary-filter.ino
│   └── sensorfusion-main/
│       └── sensorfusion-main.ino
└── libraries/
    └── CustomIMU/
        ├── CustomIMU.h
        └── CustomIMU.cpp


## 📁 Available Sketches

| Name                  | Description                          |
|-----------------------|--------------------------------------|
| `imu-test`            | Basic sensor reading + serial output |
| `kalman-filter`       | Sensor fusion using Kalman Filter    |
| `complementary-filter`| Lightweight fusion technique         |
| `sensorfusion-main`   | Full demo with filtering & display   |

## 🔌 Hardware Requirements
- Arduino Uno, Nano, Mega, etc.
- IMU sensor (e.g., MPU6050, MPU9250, BNO055)
- Jumper wires
- Breadboard (optional)

## 🚀 Getting Started

1. Clone the repo:
   ```bash
   git clone https://github.com/AminSheykhzadeh/SensorFusion.git
Open my-sketch.ino in Arduino IDE.

Select your board & COM port.

Upload the code and open Serial Monitor.

📊 Output Example
Roll:  3.12°
Pitch: -1.45°
Yaw:  125.76°
📦 Dependencies
Make sure to install these libraries in Arduino IDE:

Wire.h

Adafruit_Sensor

MPU6050 or equivalent IMU library

🧠 To Do
 Add Kalman or complementary filter

 Improve drift compensation

 Visualize data using Processing or Python

🤝 Contributing
Pull requests are welcome! For major changes, please open an issue first.

📄 License
MIT License — feel free to use and adapt as needed.

---

# My Scope :)

cd ~/Arduino/my-sketch
git add .
git commit -m "Added a comment to my sketch"
git push

git add LICENSE
git commit -m "Add MIT license"
git push



✅ Step-by-Step: Add a Version Tag in Git
Let’s say your code is stable and you want to mark it as v1.0.0.

1. Go to your project folder:

cd ~/Arduino/my-sketch
2. Tag the current commit:

git tag v1.0.0
3. Push the tag to GitHub:

git push origin v1.0.0
🏷️ What Tags Are For
You can create tags for:

Tag Name	Purpose
v1.0.0	First stable release
v1.1.0	Added new features
v1.1.1	Bugfix release
v2.0.0-beta	Beta of a big new version

🧠 Optional: Annotated Tags (with message)
You can also add a message:

git tag -a v1.0.0 -m "Initial stable version of SensorFusion"
git push origin v1.0.0
📋 View Your Tags

git tag


🔄 . Automate Version Bumping (Optional)
Option A: Minimal CLI method
You can bump version numbers manually and tag them with Git:


git tag v1.0.1 -m "Bugfix: Fixed sensor drift issue"
git push origin v1.0.1
Option B: Use a script (semi-automated)
Create a script like bump.sh:


#!/bin/bash
VERSION=$1

if [ -z "$VERSION" ]; then
  echo "Usage: ./bump.sh v1.0.1"
  exit 1
fi

git tag -a $VERSION -m "Release $VERSION"
git push origin $VERSION
echo "✅ Tagged and pushed $VERSION"
Make it executable:


chmod +x bump.sh
Now you can do:


./bump.sh v1.0.1
Would you like full automation using a tool like standard-version or GitHub Actions for release workflows too? I can walk you through those if needed!


✍️ How to Restructure Your Repo:
Make the folder structure:

mkdir -p sketches/imu-test sketches/kalman-filter sketches/sensorfusion-main
mv *.ino sketches/your-folder-name/
Commit & push:

git add .
git commit -m "Reorganize sketches into subfolders"
git push


Let’s auto-organize your sketches based on your current folder and .ino files.

🧠 Assumption:
You're working in this path:

bash
Copy
Edit
~/Arduino/my-sketch/
And it contains files like:

bash
Copy
Edit
imu-test.ino  
kalman-filter.ino  
sensorfusion-main.ino  
complementary-filter.ino  
🛠️ Step-by-Step Terminal Commands:
Copy-paste this into your terminal (inside ~/Arduino/my-sketch/):

bash
Copy
Edit
mkdir -p sketches

for sketch in *.ino; do
  name="${sketch%.ino}"
  mkdir -p "sketches/$name"
  mv "$sketch" "sketches/$name/$sketch"
done
✅ This will:

Create a sketches/ folder.

Move each .ino file into its own subfolder inside sketches/.

📁 Final Structure:

my-sketch/
├── sketches/
│   ├── imu-test/
│   │   └── imu-test.ino
│   ├── kalman-filter/
│   │   └── kalman-filter.ino
│   ├── complementary-filter/
│   │   └── complementary-filter.ino
│   └── sensorfusion-main/
│       └── sensorfusion-main.ino
🧾 Final Steps:

git add .
git commit -m "Organized all sketches into individual folders"
git push