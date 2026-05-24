# ขั้นตอน Deploy ด้วย Docker Desktop

คู่มือนี้สำหรับรันระบบ IoT Full Stack บนเครื่อง local ผ่าน Docker Desktop โดยใช้ `docker-compose.yml` และ `.env`

## 1. เปิด Docker Desktop

1. เปิดโปรแกรม Docker Desktop
2. รอจนสถานะ Docker เป็น `Running`
3. เปิด PowerShell ที่โฟลเดอร์โปรเจกต์นี้

```powershell
cd D:\GitHubData\lab-iots_infa_all_system
```

ตรวจสอบว่า Docker ใช้งานได้:

```powershell
docker version
docker compose version
```

## 2. สร้างไฟล์ .env

คัดลอกไฟล์ตัวอย่างเป็นไฟล์ใช้งานจริง:

```powershell
Copy-Item .env.example .env
```

เปิดไฟล์ `.env` แล้วแก้ค่าที่จำเป็น:

```powershell
notepad .env
```

ค่าที่ควรเปลี่ยนก่อนรันจริง:

```env
GF_SECURITY_ADMIN_PASSWORD=change-me-in-local-env
N8N_ENCRYPTION_KEY=change-me-to-32-char-random-key!!
NGROK_AUTHTOKEN=your-ngrok-authtoken-here
NGROK_DOMAIN=your-domain.ngrok-free.app
NGROK_URL=https://your-domain.ngrok-free.app
```

สร้าง `N8N_ENCRYPTION_KEY` แบบสุ่ม 32 ตัวอักษรด้วย PowerShell:

```powershell
-join ((1..32)|%{[char](Get-Random -Min 97 -Max 123)})
```

## 3. ตรวจสอบ Docker Compose

ตรวจว่า compose อ่านไฟล์ `.env` ได้ถูกต้อง:

```powershell
docker compose config
```

ถ้าไม่มี error แปลว่า config พร้อม deploy

## 4. Pull Images

ดาวน์โหลด Docker images ทั้งหมด:

```powershell
docker compose pull
```

Images ที่จะถูกใช้:

- `nodered/node-red`
- `eclipse-mosquitto`
- `n8nio/n8n`
- `ngrok/ngrok`
- `influxdb`
- `grafana/grafana`

## 5. Start Containers

เริ่มต้นทุก service:

```powershell
docker compose up -d
```

ตรวจสอบสถานะ container:

```powershell
docker compose ps
```

ทุก service ควรมีสถานะ `running`

## 6. ตรวจ Logs

ดู logs ทั้งระบบ:

```powershell
docker compose logs
```

ดู logs เฉพาะ service:

```powershell
docker compose logs nodered
docker compose logs mqtt
docker compose logs n8n
docker compose logs ngrok
docker compose logs influxdb
docker compose logs grafana
```

ดู logs แบบ realtime:

```powershell
docker compose logs -f
```

## 7. เปิดหน้า Web UI

หลัง container รันแล้ว เปิดบริการตาม URL นี้:

```text
Node-RED:         http://localhost:1880
MQTT TCP:         localhost:1883
MQTT WebSocket:   ws://localhost:9001
n8n:              http://localhost:5678
ngrok dashboard:  http://localhost:4040
InfluxDB:         http://localhost:8086
Grafana:          http://localhost:3000
```

## 8. ตั้งค่า InfluxDB ครั้งแรก

1. เปิด `http://localhost:8086`
2. คลิก `Get Started`
3. สร้าง user/password
4. สร้าง Organization
5. สร้าง Bucket
6. สร้าง API Token แบบ read/write สำหรับ bucket
7. บันทึกค่า Organization, Bucket และ Token ไว้

จากนั้นเปิด `.env`:

```powershell
notepad .env
```

เพิ่มหรือแก้ค่าต่อไปนี้:

```env
INFLUXDB_ORG=ชื่อ-organization-ของคุณ
INFLUXDB_BUCKET=ชื่อ-bucket-ของคุณ
INFLUXDB_TOKEN=token-api-ของคุณ
```

รีสตาร์ท service ที่ต้องใช้ค่า InfluxDB:

```powershell
docker compose restart nodered grafana
```

## 9. ตั้งค่า Grafana

1. เปิด `http://localhost:3000`
2. Login ด้วยค่าจาก `.env`

```env
GF_SECURITY_ADMIN_USER=admin
GF_SECURITY_ADMIN_PASSWORD=รหัสผ่านที่ตั้งไว้ในไฟล์ .env
```

3. ไปที่ `Connections` หรือ `Data sources`
4. เพิ่ม InfluxDB data source
5. ใช้ URL ภายใน Docker network:

```text
http://influxdb:8086
```

6. ใส่ Organization, Bucket และ Token จาก InfluxDB
7. กด Save & Test

## 10. ตั้งค่า Node-RED

1. เปิด `http://localhost:1880`
2. สร้าง flow ใหม่
3. ตั้งค่า MQTT broker เป็น:

```text
mqtt
```

Port:

```text
1883
```

4. ถ้าต้องเขียนข้อมูลลง InfluxDB ให้ใช้ host ภายใน Docker network:

```text
http://influxdb:8086
```

5. ถ้าต้องเรียก n8n webhook จาก Node-RED ให้ใช้ host:

```text
http://n8n:5678
```

## 11. ตั้งค่า n8n และ ngrok

1. เปิด `http://localhost:5678`
2. สร้างบัญชี n8n ครั้งแรก
3. เปิด ngrok dashboard ที่ `http://localhost:4040`
4. ตรวจว่า ngrok forward ไปยัง:

```text
n8n:5678
```

Webhook ภายนอกควรใช้ค่า:

```env
NGROK_URL=https://your-domain.ngrok-free.app
```

สำหรับ Google OAuth หรือ external webhooks ให้ใช้ URL จาก ngrok เป็น callback/webhook URL

## 12. คำสั่งจัดการระบบ

หยุด service ทั้งหมด:

```powershell
docker compose stop
```

เริ่ม service ที่หยุดไว้:

```powershell
docker compose start
```

รีสตาร์ททั้งหมด:

```powershell
docker compose restart
```

ปิดและลบ container แต่เก็บ volume ข้อมูลไว้:

```powershell
docker compose down
```

เปิดใหม่:

```powershell
docker compose up -d
```

## 13. Reset ระบบแบบล้างข้อมูลทั้งหมด

คำสั่งนี้จะลบ container และ volume ทั้งหมด ข้อมูล InfluxDB, Grafana, Node-RED, n8n และ Mosquitto จะหายทั้งหมด

```powershell
docker compose down -v
docker compose up -d
```

ใช้เฉพาะเมื่อต้องการเริ่มใหม่จากศูนย์จริง ๆ

## 14. ตรวจปัญหาเบื้องต้น

ดู container ที่รันอยู่:

```powershell
docker ps
```

ดู container ทั้งหมดรวมที่หยุดแล้ว:

```powershell
docker ps -a
```

ดู network:

```powershell
docker network ls
```

ดู volumes:

```powershell
docker volume ls
```

ถ้า port ชน ให้แก้ port ใน `.env` แล้วรัน:

```powershell
docker compose up -d
```

ถ้าแก้ค่า `.env` แล้ว service ยังไม่รับค่าใหม่ ให้ restart:

```powershell
docker compose restart
```

## 15. ใช้งาน ESP32 Energy Simulator ด้วย VS Code + PlatformIO

โฟลเดอร์ `esp32_energy_sim` คือ firmware สำหรับ ESP32 ที่จำลองระบบพลังงานไฟฟ้า 3 เฟส แล้วส่งข้อมูลผ่าน MQTT ไปยัง Mosquitto/Node-RED

ข้อมูลที่ firmware จำลอง:

- แรงดันไฟฟ้าแต่ละเฟส
- กระแสไฟฟ้าแต่ละเฟส
- กำลังไฟฟ้า kW
- Power factor
- ความถี่ไฟฟ้า Hz
- พลังงานสะสม kWh
- ค่า demand เฉลี่ยช่วง 15 นาที

Topic หลักที่ ESP32 ส่งข้อมูล:

```text
factory/factory_01/telemetry
```

Topic สถานะ:

```text
factory/factory_01/status
```

Topic รับคำสั่งควบคุม:

```text
factory/factory_01/control
```

### 15.1 ติดตั้งเครื่องมือ

ติดตั้งโปรแกรมและ extension ต่อไปนี้:

1. ติดตั้ง Visual Studio Code
2. เปิด VS Code
3. ไปที่ Extensions
4. ค้นหา `PlatformIO IDE`
5. กด Install
6. รอ PlatformIO ติดตั้ง core และ reload VS Code

### 15.2 เปิดโปรเจกต์ ESP32

ใน VS Code ให้เปิดโฟลเดอร์นี้:

```text
D:\GitHubData\lab-iots_infa_all_system\esp32_energy_sim
```

หรือเปิดจาก PowerShell:

```powershell
cd D:\GitHubData\lab-iots_infa_all_system\esp32_energy_sim
code .
```

ตรวจสอบว่ามีไฟล์หลัก:

```text
platformio.ini
src/main.cpp
include/config.example.h
```

### 15.3 สร้างไฟล์ config.h

ไฟล์ `include/config.h` เป็นไฟล์ config จริงของเครื่อง local และไม่ถูก commit เข้า Git เพราะมี Wi-Fi/password

คัดลอกจากไฟล์ตัวอย่าง:

```powershell
Copy-Item include\config.example.h include\config.h
```

เปิดไฟล์ config:

```powershell
notepad include\config.h
```

แก้ค่าให้ตรงกับเครือข่ายจริง:

```cpp
#define WIFI_SSID     "your-wifi-ssid"
#define WIFI_PASSWORD "your-wifi-password"

#define MQTT_BROKER   "192.168.1.100"
#define MQTT_PORT     1883

#define DEVICE_ID     "factory_01"
#define BASE_KW       150.0f
#define ALERT_KW      130.0f
```

ค่า `MQTT_BROKER` ต้องเป็น IP ของเครื่องที่รัน Docker Desktop ไม่ใช่ `localhost` เพราะ ESP32 เป็นอุปกรณ์อีกตัวใน network

หา IP เครื่อง Windows:

```powershell
ipconfig
```

ดูค่า `IPv4 Address` ของ Wi-Fi หรือ LAN ที่ใช้งานอยู่ เช่น:

```text
192.168.0.161
```

แล้วนำไปใส่:

```cpp
#define MQTT_BROKER "192.168.0.161"
```

### 15.4 Start Docker Stack ก่อน

ก่อน upload firmware ควรเปิด Docker stack ให้ MQTT broker พร้อมรับข้อมูล:

```powershell
cd D:\GitHubData\lab-iots_infa_all_system
docker compose up -d
docker compose ps
```

ตรวจว่า service `mqtt` running:

```powershell
docker compose logs mqtt
```

### 15.5 Build Firmware

ใน VS Code เปิด PlatformIO sidebar แล้วกด:

```text
Project Tasks > esp32dev > General > Build
```

หรือใช้ PowerShell:

```powershell
cd D:\GitHubData\lab-iots_infa_all_system\esp32_energy_sim
pio run
```

ถ้า build สำเร็จจะเห็นข้อความประมาณ:

```text
SUCCESS
```

### 15.6 Upload เข้า ESP32

เสียบ ESP32 ผ่าน USB แล้วตรวจว่า Windows มองเห็น serial port

ใน VS Code กด:

```text
Project Tasks > esp32dev > General > Upload
```

หรือใช้คำสั่ง:

```powershell
pio run --target upload
```

ถ้ามีหลาย COM port ให้ระบุ port เอง เช่น:

```powershell
pio run --target upload --upload-port COM5
```

### 15.7 เปิด Serial Monitor

หลัง upload เสร็จ เปิด serial monitor:

```powershell
pio device monitor
```

หรือระบุ port:

```powershell
pio device monitor --port COM5 --baud 115200
```

ใน VS Code กด:

```text
Project Tasks > esp32dev > Platform > Monitor
```

ควรเห็น log ประมาณ:

```text
WiFi connected
MQTT connected
Published telemetry
```

ออกจาก serial monitor:

```text
Ctrl + C
```

### 15.8 ตรวจข้อมูลเข้า MQTT / Node-RED

เปิด Node-RED:

```text
http://localhost:1880
```

สร้าง MQTT input node:

```text
Server: mqtt
Port: 1883
Topic: factory/factory_01/telemetry
```

ต่อเข้ากับ debug node แล้วกด Deploy

ถ้า ESP32 ทำงานถูกต้อง จะเห็น JSON payload เข้ามาทุก 5 วินาที

ตัวอย่าง payload:

```json
{
  "device_id": "factory_01",
  "voltage_l1": 220.4,
  "current_l1": 120.5,
  "power_kw": 145.2,
  "kwh_total": 320.8,
  "demand_15m_kw": 148.1
}
```

### 15.9 ส่งคำสั่งควบคุมจาก Node-RED ไป ESP32

ESP32 subscribe topic:

```text
factory/factory_01/control
```

คำสั่ง reset demand:

```json
{"cmd":"reset_demand"}
```

คำสั่งตั้ง load แบบ manual เช่น 80%:

```json
{"cmd":"set_load","value":0.8}
```

คำสั่งกลับไปใช้ auto daily load curve:

```json
{"cmd":"auto_load"}
```

ใน Node-RED ให้ใช้ MQTT output node:

```text
Server: mqtt
Port: 1883
Topic: factory/factory_01/control
```

### 15.10 Troubleshooting ESP32

ถ้า ESP32 ต่อ Wi-Fi ไม่ได้:

- ตรวจ `WIFI_SSID`
- ตรวจ `WIFI_PASSWORD`
- ตรวจว่า Wi-Fi เป็น 2.4 GHz เพราะ ESP32 ส่วนใหญ่ไม่รองรับ 5 GHz

ถ้า MQTT ต่อไม่ได้:

- ตรวจว่า Docker Desktop running
- ตรวจว่า `docker compose ps` แสดง service `mqtt` เป็น running
- ตรวจว่า `MQTT_BROKER` เป็น IP เครื่อง Windows ไม่ใช่ `localhost`
- ตรวจ firewall ของ Windows ว่าอนุญาต port `1883`
- ตรวจว่า ESP32 และเครื่อง Windows อยู่ network เดียวกัน

ถ้า upload ไม่ได้:

- กดปุ่ม `BOOT` บน ESP32 ค้างระหว่างเริ่ม upload
- เปลี่ยนสาย USB ที่รองรับ data
- ติดตั้ง driver USB-to-Serial เช่น CP210x หรือ CH340
- ระบุ COM port ด้วย `--upload-port COMx`

ถ้า serial monitor อ่านไม่ออก:

- ตรวจ baud rate เป็น `115200`
- กดปุ่ม reset บน ESP32 หลังเปิด monitor
