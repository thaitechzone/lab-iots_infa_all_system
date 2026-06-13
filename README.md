# Lab IoTs Infrastructure All System

ระบบ IoT Full Stack สำหรับทดลองและวางโครงสร้างระบบตรวจสอบพลังงาน/ข้อมูลโรงงาน โดยใช้ Docker Compose เป็นตัวจัดการบริการหลัก

โปรเจกต์นี้ออกแบบมาให้เริ่มต้นแบบสะอาด ไม่มี flow หรือ dashboard ที่เตรียมไว้ล่วงหน้า ผู้ใช้งานสามารถสร้าง Node-RED flows, InfluxDB buckets และ Grafana dashboards ได้เองจากศูนย์

## ผู้ออกแบบ

ออกแบบโดย ณัฐพล จะสูงเนิน  
Line: `thaitechzone`  
Tel: `0939391546`

## Services ในระบบ

| Service | หน้าที่ | URL / Port | Container Name |
| --- | --- | --- | --- |
| Node-RED | สร้าง IoT flow, รับข้อมูล MQTT, ประมวลผล, ส่งข้อมูลไป InfluxDB | `http://localhost:NODERED_PORT` | `iot_nodered` |
| Mosquitto MQTT | MQTT broker สำหรับรับส่งข้อมูลจาก ESP32 หรือ IoT devices | `localhost:MQTT_PORT` | `iot_mosquitto` |
| Mosquitto WebSocket | MQTT ผ่าน WebSocket | `ws://localhost:MQTT_WS_PORT` | `iot_mosquitto` |
| InfluxDB | Time-series database สำหรับเก็บข้อมูล sensor/energy | `http://localhost:INFLUXDB_PORT` | `iot_influxdb` |
| Grafana | Dashboard visualization สำหรับแสดงผลข้อมูลจาก InfluxDB | `http://localhost:GRAFANA_PORT` | `iot_grafana` |

**หมายเหตุ:** Port ทั้งหมดสามารถตั้งค่าได้ในไฟล์ `.env` ค่า default:
- `NODERED_PORT=1880`
- `MQTT_PORT=1883`
- `MQTT_WS_PORT=9001`
- `INFLUXDB_PORT=8086`
- `GRAFANA_PORT=3000`

## โครงสร้างไฟล์

```text
.
├── docker-compose.yml              # Docker Compose configuration
├── .env.example                    # ตัวอย่างไฟล์ environment variables
├── .env                            # ไฟล์ environment variables (สร้างจาก .env.example)
├── .gitignore                      # Git ignore rules
├── DEPLOY_DOCKER_DESKTOP.md        # คำแนะนำการใช้ Docker Desktop
├── README.md                       # ไฟล์นี้
└── mosquitto/
    └── config/
        └── mosquitto.conf          # MQTT broker configuration
```

**ไฟล์สำคัญ:**
- `.env` - **ต้องสร้างเอง** จากไฟล์ `.env.example` (ไม่ควร commit)
- `docker-compose.yml` - ตั้งค่า services, networks, volumes
- `mosquitto/config/mosquitto.conf` - ตั้งค่า MQTT broker

## Requirements

ก่อนใช้งานต้องติดตั้ง:

- Docker Desktop
- Git

ตรวจสอบ Docker:

```powershell
docker version
docker compose version
```

## เริ่มต้นใช้งาน

เข้าโฟลเดอร์โปรเจกต์:

```powershell
cd D:\GitHubData\lab-iots_infa_all_system
```

สร้างไฟล์ `.env` จากตัวอย่าง:

```powershell
Copy-Item .env.example .env
```

เปิดไฟล์ `.env` เพื่อแก้ค่า:

```powershell
notepad .env
```

**ค่าที่ต้องจำเป็นต้องเปลี่ยน:**

```env
GF_SECURITY_ADMIN_PASSWORD=change-me-in-local-env
```

**ค่าอื่นๆ ที่อาจต้องปรับแต่ง:**
- `TZ` - ตั้งค่า timezone (default: `Asia/Bangkok`)
- `NODERED_PORT`, `MQTT_PORT`, `MQTT_WS_PORT`, `INFLUXDB_PORT`, `GRAFANA_PORT` - ปรับเปลี่ยนพอร์ตถ้าชนกับบริการอื่นๆ

## ตัวแปรสภาพแวดล้อม (.env)

ไฟล์ `.env` ควบคุมการตั้งค่าของเซอร์วิส:

```env
# ────── ตั้งค่าทั่วไป ──────
TZ=Asia/Bangkok                          # Timezone

# ────── พอร์ตบริการ ──────
MQTT_PORT=1883                           # MQTT port
MQTT_WS_PORT=9001                        # MQTT WebSocket port
NODERED_PORT=1880                        # Node-RED port
INFLUXDB_PORT=8086                       # InfluxDB port
GRAFANA_PORT=3000                        # Grafana port

# ────── Grafana ──────
GF_SECURITY_ADMIN_USER=admin             # Grafana admin username
GF_SECURITY_ADMIN_PASSWORD=your-password # ⚠️  เปลี่ยนเป็น password ที่แข็งแรง
```

**เทคนิค:** หาก port ใดชนกับบริการอื่นบนเครื่อง ให้แก้ไขค่า port ในไฟล์ `.env` เช่น:
```env
GRAFANA_PORT=3001
INFLUXDB_PORT=8087
```

## Docker Network และการเชื่อมต่อ

ระบบใช้ custom Docker bridge network ชื่อ `iot_net` เพื่อให้บริการต่างๆ สามารถสื่อสารกันได้:

**การเชื่อมต่อจากภายนอก (จาก host/ESP32):**
- ใช้ `localhost` หรือ IP ของเครื่อง host
- ใช้พอร์ตที่ตั้งไว้ใน `.env` (default: `1883`, `1880`, `8086`, `3000`)

**การเชื่อมต่อภายใน Docker (จากบริการหนึ่งไปอีกบริการหนึ่ง):**
- ใช้ container name แทน localhost:
  - MQTT: `mqtt:1883`
  - InfluxDB: `influxdb:8086`
  - Node-RED: `nodered:1880`
  - Grafana: `grafana:3000`

**ตัวอย่าง:** ใน Node-RED เพื่อเชื่อมต่อ MQTT broker ใหม่ใหม่:
- **Host:** `mqtt` (ไม่ใช่ `localhost`)
- **Port:** `1883`
- **URL:** `mqtt://mqtt:1883`

## ตรวจสอบ Compose Config

ก่อน deploy ควรตรวจสอบไฟล์ compose:

```powershell
docker compose config
```

ถ้าไม่มี error แปลว่าสามารถรัน stack ได้

## Download Docker Images

```powershell
docker compose pull
```

โปรเจกต์นี้ใช้ image แบบ `latest` เพื่อให้เหมาะกับ lab/dev environment:

```yaml
nodered/node-red:latest
eclipse-mosquitto:latest
influxdb:latest
grafana/grafana:latest
```

หมายเหตุ: หากนำไปใช้ production ควร pin version ของ image หลังทดสอบเสถียรแล้ว

## Start ระบบ

### เริ่มทุก container:

```powershell
docker compose up -d
```

ระบบจะเริ่มทั้งหมด 5 services:
- `iot_nodered` (Node-RED)
- `iot_mosquitto` (MQTT Broker)
- `iot_influxdb` (InfluxDB)
- `iot_grafana` (Grafana)

### ตรวจสอบสถานะ:

```powershell
docker compose ps
```

ผลลัพธ์ควรแสดง container ทั้งหมดมี status `Up`:
```
NAME                COMMAND                  STATUS           PORTS
iot_nodered         npm start -- --userDir   Up 2 seconds     0.0.0.0:1880->1880/tcp
iot_mosquitto       /docker-entrypoint.sh    Up 3 seconds     0.0.0.0:1883->1883/tcp
iot_influxdb        /entrypoint.sh influxd   Up 4 seconds     0.0.0.0:8086->8086/tcp
iot_grafana         /run.sh                  Up 2 seconds     0.0.0.0:3000->3000/tcp
```

### ดู logs:

```powershell
docker compose logs
```

### ดู logs แบบ real-time:

```powershell
docker compose logs -f
```

### ดู logs เฉพาะบริการ:

```powershell
docker compose logs nodered      # Node-RED
docker compose logs mqtt         # MQTT
docker compose logs influxdb     # InfluxDB
docker compose logs grafana      # Grafana
```

### Auto-Restart Policy

ทุก container ตั้งค่า `restart: unless-stopped` หมายความว่า:
- ❌ ถ้า container crash → จะ restart อัตโนมัติ
- ❌ ถ้าเครื่อง reboot → ทุก container จะ start อัตโนมัติ
- ✓ เฉพาะเมื่อคุณหยุด (`docker compose stop`) container ด้วยตนเอง จึงจะไม่ restart

## การตั้งค่า InfluxDB ครั้งแรก

1. เปิด `http://localhost:8086`
2. คลิก `Get Started`
3. สร้าง user/password
4. สร้าง Organization
5. สร้าง Bucket
6. สร้าง API Token ที่มีสิทธิ์ read/write กับ bucket
7. บันทึกค่า Organization, Bucket และ Token ไว้ใช้ตอนตั้งค่า Grafana หรือ Node-RED

## การใช้งาน Grafana

เปิด Grafana:

```text
http://localhost:{GRAFANA_PORT}
```
(default: `http://localhost:3000`)

### Login เข้า Grafana

ใช้ credentials จากไฟล์ `.env`:

```
Username: {GF_SECURITY_ADMIN_USER} (default: admin)
Password: {GF_SECURITY_ADMIN_PASSWORD}
```

### เพิ่ม InfluxDB Data Source

1. ไปที่ **Administration → Data Sources → Add data source**
2. เลือก **InfluxDB**
3. ตั้งค่าดังนี้:

```
Name: InfluxDB
URL: http://influxdb:8086
Organization: <ชื่อ Organization จาก InfluxDB>
Bucket: <ชื่อ Bucket จาก InfluxDB>
Token: <API Token จาก InfluxDB>
```

4. คลิก **Save & Test** เพื่อตรวจสอบการเชื่อมต่อ
5. สร้าง Dashboards เพื่อแสดงข้อมูลพลังงานและ sensor

## การใช้งาน Node-RED

เปิด Node-RED:

```text
http://localhost:{NODERED_PORT}
```
(default: `http://localhost:1880`)

### การเชื่อมต่อ MQTT Broker

**ภายใน Docker (ใช้ในภายใน Node-RED):**
```
Host: mqtt
Port: 1883
Protocol: mqtt
```

**จากภายนอก (จาก ESP32/IoT devices):**
```
Host: IP ของเครื่อง host
Port: {MQTT_PORT} (default: 1883)
Protocol: mqtt
```

### การเชื่อมต่อ InfluxDB

**ภายใน Docker (ใช้ใน InfluxDB nodes):**
```
URL: http://influxdb:8086
Organization: <สร้างเองใน InfluxDB>
Bucket: <สร้างเองใน InfluxDB>
Token: <API Token จาก InfluxDB>
```

**จากภายนอก (Grafana, เบราว์เซอร์):**
```
URL: http://localhost:{INFLUXDB_PORT}
```
(default: `http://localhost:8086`)

ตัวอย่าง flow ที่ควรสร้าง:

- MQTT subscribe รับข้อมูลจาก ESP32
- Function node ตรวจสอบ/แปลง payload
- InfluxDB node สำหรับบันทึก time-series data
- Switch node ตรวจ threshold เช่น ค่า kW เกินกำหนด
- Debug node หรือ HTTP request node สำหรับตรวจสอบ/ส่งต่อข้อมูลไปยังระบบภายนอกตามต้องการ

## การใช้งาน MQTT กับ ESP32 / IoT Devices

### การเชื่อมต่อจาก ESP32 หรือ IoT Device

ESP32 หรือ IoT device ที่อยู่ใน network เดียวกับเครื่อง host สามารถเชื่อมต่อได้:

```c
// ESP32 Example (ArduinoIDE)
#include <PubSubClient.h>

const char* mqtt_server = "192.168.x.x";  // IP ของเครื่อง host
const int mqtt_port = 1883;               // MQTT port จากไฟล์ .env

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  // Publish data
  client.publish("factory/energy/meter01", "{\"power_kw\": 2.73}");
}
```

**สำคัญ:** ใช้ **IP address ของเครื่อง host** ไม่ใช่ `localhost` (localhost ใช้ได้เฉพาะในเครื่องเดียว)

### Topic Structure (ตัวอย่าง)

```
factory/energy/meter01          # Meter #1
factory/energy/meter02          # Meter #2
factory/energy/grid             # Grid power data
factory/alerts/overload         # Alert: Overload
factory/system/status           # System status
```

### Payload Format (ตัวอย่าง)

```json
{
  "device_id": "meter01",
  "timestamp": "2026-06-13T10:30:45Z",
  "voltage": 220.5,
  "current": 12.4,
  "power_kw": 2.73,
  "energy_kwh": 154.8,
  "frequency": 50.0
}
```

### MQTT Broker Details

- **Internal (Docker network):** `mqtt://mqtt:1883`
- **External (from ESP32):** `mqtt://{HOST_IP}:{MQTT_PORT}`
- **WebSocket:** `ws://{HOST_IP}:{MQTT_WS_PORT}`
- **Username/Password:** ไม่จำเป็น (anonymous allowed in dev mode)
- **Retained messages:** ถูกเก็บไว้บน broker

### ทดสอบการเชื่อมต่อ MQTT

ใช้ MQTT client เช่น `mosquitto_pub` หรือ `MQTT.js`:

```powershell
# Windows: install MQTT client
# choco install mosquitto-clients

mosquitto_pub -h localhost -p 1883 -t "factory/energy/meter01" -m '{"power_kw": 2.73}'
```

## คำสั่งจัดการระบบ

หยุด container:

```powershell
docker compose stop
```

เริ่ม container ที่หยุดไว้:

```powershell
docker compose start
```

รีสตาร์ททั้งหมด:

```powershell
docker compose restart
```

ปิดและลบ container แต่เก็บ volume:

```powershell
docker compose down
```

ปิดและล้างข้อมูลทั้งหมด:

```powershell
docker compose down -v
```

ใช้ `down -v` เฉพาะเมื่อต้องการลบข้อมูลทั้งหมดและเริ่มใหม่จากศูนย์

## ตรวจสอบและแก้ไขปัญหา

### 1. ตรวจสอบ Container Status

ดู container ที่กำลังรัน:

```powershell
docker ps
```

ดู container ทั้งหมด (รวม stopped):

```powershell
docker ps -a
```

### 2. ดู Logs เพื่อหา Error

```powershell
docker compose logs                 # Logs ทั้งหมด
docker compose logs -f              # Real-time logs
docker compose logs nodered         # Logs ของ Node-RED
docker compose logs mqtt            # Logs ของ MQTT
docker compose logs influxdb        # Logs ของ InfluxDB
docker compose logs grafana         # Logs ของ Grafana
```

### 3. ตรวจสอบ Port Conflicts

ถ้า container ไม่ start อาจเป็นเพราะ port ชนกับบริการอื่น:

```powershell
# Windows: ดู process ที่ใช้ port ต่างๆ
netstat -ano | findstr :1883        # MQTT port
netstat -ano | findstr :1880        # Node-RED port
netstat -ano | findstr :8086        # InfluxDB port
netstat -ano | findstr :3000        # Grafana port
```

**แก้ไข:** แก้ port ใน `.env`:
```env
MQTT_PORT=1883          # เปลี่ยนจาก 1883 เป็น 1884 หรือ port อื่น
NODERED_PORT=1880
INFLUXDB_PORT=8086
GRAFANA_PORT=3000
```

### 4. ตรวจสอบ Docker Networks

```powershell
docker network ls                   # ดู networks ทั้งหมด
docker network inspect iot_net      # ดูรายละเอียด iot_net network
```

### 5. ตรวจสอบ Docker Volumes

```powershell
docker volume ls                    # ดู volumes ทั้งหมด
docker volume inspect {volume_name} # ดูรายละเอียด volume
```

### 6. ลบและสร้างใหม่ (Reset)

ลบ containers แต่เก็บข้อมูล (volumes):

```powershell
docker compose down
docker compose up -d
```

ลบ containers และ volumes ทั้งหมด (⚠️ ข้อมูลหายไป):

```powershell
docker compose down -v
docker compose up -d
```

### 7. Connection Issues ใน Node-RED/Grafana

**ถ้า Node-RED ไม่สามารถเชื่อมต่อ MQTT:**
- ตรวจสอบใช้ `mqtt` (ชื่อ container) แทน `localhost`
- Port ต้องเป็น `1883` (port ภายใน Docker, ไม่ใช่ `MQTT_PORT`)

**ถ้า Grafana ไม่สามารถเชื่อมต่อ InfluxDB:**
- URL ต้องเป็น `http://influxdb:8086` (ไม่ใช่ `localhost`)
- ตรวจสอบ Organization, Bucket, Token ถูกต้อง

## Volumes และ Data Persistence

ระบบใช้ **named volumes** เพื่อเก็บข้อมูลถาวร:

| Volume | Service | ที่เก็บข้อมูล | เนื้อหา |
| --- | --- | --- | --- |
| `nodered_data` | Node-RED | `/data` | Flows, settings, custom nodes |
| `mosquitto_data` | MQTT | `/mosquitto/data` | Retained messages |
| `mosquitto_log` | MQTT | `/mosquitto/log` | MQTT broker logs |
| `influxdb_data` | InfluxDB | `/var/lib/influxdb2` | Time-series data |
| `influxdb_config` | InfluxDB | `/etc/influxdb2` | Configuration, tokens |
| `grafana_data` | Grafana | `/var/lib/grafana` | Dashboards, datasources, users |

**ข้อมูลจะยังคงอยู่แม้**:
- ❌ Container crash → restart อัตโนมัติ
- ❌ `docker compose stop` → start ใหม่
- ✓ `docker compose down` → ข้อมูลยังอยู่ในระบบ

**ข้อมูลจะหายเมื่อ**:
- ✓ `docker compose down -v` → ลบ volumes ด้วย (⚠️ ข้อมูลหายไปสำหรับกำหนด!)

### Backup Volumes

การ backup Node-RED flows:

```powershell
# ก่อนทำการเปลี่ยนแปลงใหญ่ๆ
docker compose exec nodered tar -czf /data/backup-flows.tar.gz /data
```

## หมายเหตุด้านความปลอดภัย

### Development Mode (Current)

ระบบปัจจุบันตั้งค่าสำหรับ **lab/dev environment**:

- ✓ MQTT anonymous allowed (`allow_anonymous true`)
- ✓ Grafana sign-up disabled (`GF_USERS_ALLOW_SIGN_UP=false`)
- ❌ Default password: `change-me-in-local-env`

### Production Considerations

เมื่อนำไปใช้ production ต้องปรับแต่ง:

1. **เปลี่ยน Password:**
   ```env
   GF_SECURITY_ADMIN_PASSWORD=<strong-password-here>
   ```

2. **เปิด MQTT Authentication:**
   แก้ไข `mosquitto/config/mosquitto.conf`:
   ```
   allow_anonymous false
   password_file /mosquitto/config/passwd
   ```

3. **ปกป้อง Sensitive Files:**
   - ❌ ห้าม commit `.env` (มี credentials จริง)
   - ✓ `.gitignore` ตั้งค่าให้ ignore `.env` แล้ว
   - ✓ ใช้ `.env.example` เป็นตัวอย่าง

4. **ตั้งค่า Token Expiration:**
   - ตั้ง API Token ใน InfluxDB ให้มี expiration date
   - Rotate tokens regularly

5. **Enable HTTPS:**
   - Reverse proxy ด้วย nginx หรือ Apache
   - ใช้ SSL certificates (Let's Encrypt)

6. **Network Security:**
   - จำกัดการเข้าถึง port จาก IP specific
   - ใช้ firewall rules
   - วาง VPN สำหรับการเข้าถึงระยะไกล

## เอกสารเพิ่มเติม

ดูขั้นตอน deploy แบบละเอียดสำหรับ Docker Desktop ได้ที่:

[DEPLOY_DOCKER_DESKTOP.md](DEPLOY_DOCKER_DESKTOP.md)
