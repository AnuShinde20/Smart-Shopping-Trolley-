# Smart Trolley System

The **Smart Trolley System** is an innovative shopping solution that enhances the user experience with advanced features. It scans products using an ESP32-CAM, generates bills, and facilitates payment via QR codes. The accompanying mobile app provides separate admin and user functionalities for inventory management, staff management, and user data storage.

---

## Features

### Trolley Features
- **Product Scanning:** Uses ESP32-CAM for automatic product detection and scanning.
- **Billing System:** Automatically calculates the total cost of products and displays the bill.
- **QR Code Payment:** Enables secure and fast payment via QR code.

### Mobile App Features
#### Admin Page
- Manage inventory efficiently.
- Add, update, or remove staff details.
#### User Page
- View purchased product details.
- Store basic user information for a personalized experience.

---

## Tech Stack

### Hardware
- **ESP32-CAM:** For product scanning.
- **Other Components:** 
  - RFID reader (optional for cart access control).
  - Load cell (optional for weight-based verification).
  - Power supply.

### Software
- **Frontend:** HTML, CSS, JavaScript.
- **Backend:** Firebase Realtime Database.
- **Mobile App:** Cross-platform mobile app for admin and user management.

---

## Installation and Setup

### Hardware Setup
1. Assemble the ESP32-CAM on the trolley.
2. Connect the camera module to scan barcodes or QR codes.
3. Ensure compatibility with your power supply and any additional sensors.

### Software Setup
1. Clone this repository:
   ```bash
   git clone https://github.com/your-username/smart-trolley.git
