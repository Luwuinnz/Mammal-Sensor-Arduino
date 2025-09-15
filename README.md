# Abstract
The **Dog Orthopedic Sensor** presents a portable, low-cost orthopedic gait analysis device for canine patients, designed to transform veterinary diagnostics by enabling faster, more cost-efficient assessment of orthopedic disorders and issues. It was developed as a **proof of concept** with the potential to evolve into a full diagnostic support tool. Powered by an **ESP32 microcontroller** and **MPU6050 accelerometer-gyroscope sensor**, the system captures precise, real-time motion data, storing it to an SD card and streaming it wirelessly for immediate review. The collected data is processed through a **Temporal Convolutional Network (TCN)** model to distinguish between healthy and unhealthy gait patterns, providing **machine learning–driven decision support** for veterinarians.  

By replacing expensive, laboratory-grade motion analysis systems with an accessible, field-ready solution, it reduces diagnostic costs, accelerates clinical decision-making, and supports earlier intervention and more effective treatment planning. Overall, it offers a practical tool to enhance orthopedic care and improve patient outcomes.

---

# Methodology

## Overview
This project was adopted in **January 2025** from the previous developer, *Cole Schreiner*, whose design utilized an Arduino Nano and a single MPU9250 sensor. However, the bulky breadboard and large case introduced significant weight, which risked causing discomfort for smaller or weaker dogs.  

The goal of this iteration was to **reduce the device’s size and weight** while simultaneously improving sensor accuracy, data collection, and real-time analysis capabilities.

---

## Prototyping Process
- The original design was dismantled and restructured.  
- The system transitioned from one **MPU9250** to **two MPU6050 sensors**, enabling gait data to be captured at two critical spine locations:  
  - **MPUA (shoulders)** – placed along the spine above the dog’s shoulders.  
  - **MPUB (hips)** – mounted on a TPU platform at the hips and secured with a stretchable harness.  

This **dual-sensor approach** provided richer insight into gait asymmetries and orthopedic motion.  

- The microcontroller was upgraded from the **Arduino Nano** to the **ESP32-WROOM-32**, allowing:  
  - Wi-Fi connectivity  
  - Onboard webserver hosting  
  - Real-time gait data visualization  

- After the prototyping stage, a lighter **ESP32-C3** variant replaced its predecessor—meeting the same functional requirements while reducing bulk for use on weaker and smaller dogs.  

- An **SD card module** was retained for local data logging. Recorded readings are formatted as **CSV files**, transferred to an **SQL database**, and categorized by breed and size before being processed by the TCN model.  

- The system is powered by a **7.4V Li-Po battery**, with all components connected via **braided wiring** to minimize wear.  

- The **main device enclosure** is lightweight, secured via Velcro, and mounted with a stretchable harness for adjustability across dog sizes.  

- Once finalized, components were soldered onto compact boards to reduce mass and eliminate the need for a breadboard.  

---

## Firmware and Data Handling
- Firmware samples **10 readings per second** from each MPU6050.  
- Sensors are mounted at **-90° offset**, requiring software-based recalibration for realistic alignment with canine movement.  
- Data (timestamp, gyroscope, accelerometer, and angle values) is stored in **CSV format** for streamlined database ingestion.  

---

## Webserver and Visualization
- The **ESP32’s onboard Wi-Fi** enables device-hosted webserver functionality.  
- Users can access a **live dashboard** that:  
  - Streams sensor readings  
  - Displays accelerometer mappings for each MPU  
  - Renders a simplified **dog skeleton** showing real-time hip and shoulder movement  

---

# TCN Model Implementation

### Input Format
- Sensor data segmented into **5–10 second windows** (~50–100 readings at 10 Hz).  
- Each segment typically captures at least one full stride cycle.  
- Windows labeled *healthy/unhealthy* based on veterinary input.  
- Enables detection of gait irregularities (uneven cadence, reduced hip rotation, asymmetric weight distribution).  

### Model Architecture
- Built with **1D dilated causal convolutions**.  
- Captures stride cycles and timing differences without recurrent layers (unlike LSTMs).  
- Uses **convolutional + residual connections** to model gait variations (limping, uneven cadence, reduced range of motion).  

### Training Process
- Data normalization (z-score or min-max scaling).  
- Augmentation strategies: stride speed variation, noise injection.  
- Supervised training minimizing classification loss between predictions and veterinary diagnosis.  

---

# Diagnosis Workflow

### Offline Phase (Model Training)
- Large-scale labeled data collected from multiple dogs.  
- Model retrained periodically to improve classification accuracy.  

### Online Phase (Device Use)
1. Dog wears device during a short walking session.  
2. Sensor readings streamed and logged.  
3. Data uploaded into SQL database.  
4. TCN model processes gait windows.  
5. Outputs **probability score** (Healthy vs. Unhealthy).  
6. Veterinarian/caretaker receives **summarized diagnostic suggestion**.  

---

# Limitations/Future Work

The Dog Orthopedic Sensor currently serves as a proof-of-concept platform rather than a finalized diagnostic tool. The most significant limitation is the limited availability of meaningful IMU data collected from dogs. While the framework for a Temporal Convolutional Network (TCN) model is already in place, it has not yet been trained due to the lack of veterinary-labeled gait datasets.

This stage is intentional. The system is designed first as a data collection and visualization tool, laying the groundwork for building the larger datasets needed to unlock its full diagnostic capabilities. Once sufficient IMU data is collected across different breeds, sizes, and orthopedic conditions, the TCN model can be trained and validated, transforming the prototype into a machine learning–driven diagnostic support system.

---

# Conclusion
This prototype establishes a **proof-of-concept** for a **lightweight, cost-efficient system** to monitor canine gait abnormalities.  

The integration of **motion sensors**, a **microcontroller**, and a **SQL-driven ML framework** provides a **reproducible platform** capable of supporting further analytical complexity.  

Emphasis is placed on **portability and scalability**, ensuring broad applicability across diverse canine populations.  

While not yet a diagnostic instrument, the system demonstrates the feasibility of **wearable IoT devices** for **objective musculoskeletal assessment**.  

In the long term, this approach has the potential to:  
- Reduce veterinary diagnostic costs  
- Support earlier, data-informed interventions  
- Improve canine healthcare outcomes  

---

# Team Members
- **Tiffany Lin** – Project Lead, Prototype Board Design/Firmware/Schematics, ML collaborator  
- **Cameron Potvin** – Project and ML Supervisor, Board Design/Case  
- **David Tanioka** – Machine Learning Collaborator  
