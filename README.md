# Lab IoTs Infrastructure All System

ระบบ IoT Full Stack สำหรับทดลองและวางโครงสร้างระบบตรวจสอบพลังงาน/ข้อมูลโรงงาน โดยใช้ Docker Compose เป็นตัวจัดการบริการหลัก

โปรเจกต์นี้ออกแบบมาให้เริ่มต้นแบบสะอาด ไม่มี flow หรือ dashboard ที่เตรียมไว้ล่วงหน้า ผู้ใช้งานสามารถสร้าง Node-RED flows, InfluxDB buckets และ Grafana dashboards ได้เองจากศูนย์

## ผู้ออกแบบ

ออกแบบโดย ณัฐพล จะสูงเนิน  
Line: `thaitechzone`  
Tel: `0939391546`

## Services ในระบบ

| Service | หน้าที่ | URL / Port |
| --- | --- | --- |
| Node-RED | สร้าง IoT flow, รับข้อมูล MQTT, ประมวลผล, ส่งข้อมูลไป InfluxDB | `http://localhost:1880` |
| Mosquitto MQTT | MQTT broker สำหรับรับส่งข้อมูลจาก ESP32 หรือ IoT devices | `localhost:1883` |
| Mosquitto WebSocket | MQTT ผ่าน WebSocket | `ws://localhost:9001` |
| InfluxDB | Time-series database สำหรับเก็บข้อมูล sensor/energy | `http://localhost:8086` |
| Grafana | Dashboard visualization สำหรับแสดงผลข้อมูลจาก InfluxDB | `http://localhost:3000` |

## โครงสร้างไฟล์

```text
.
├── docker-compose.yml
├── .env.example
├── .gitignore
├── DEPLOY_DOCKER_DESKTOP.md
├── README.md
└── mosquitto/
    └── config/
        └── mosquitto.conf
```

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

ค่าที่ควรเปลี่ยนก่อนรัน:

```env
GF_SECURITY_ADMIN_PASSWORD=change-me-in-local-env
```

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

เริ่มทุก container:

```powershell
docker compose up -d
```

ตรวจสอบสถานะ:

```powershell
docker compose ps
```

ดู logs:

```powershell
docker compose logs
```

ดู logs แบบ realtime:

```powershell
docker compose logs -f
```

## การตั้งค่า InfluxDB ครั้งแรก

1. เปิด `http://localhost:8086`
2. คลิก `Get Started`
3. สร้าง user/password
4. สร้าง Organization
5. สร้าง Bucket
6. สร้าง API Token ที่มีสิทธิ์ read/write กับ bucket
7. บันทึกค่า Organization, Bucket และ Token

จากนั้นเพิ่มค่าต่อไปนี้ใน `.env`:

```env
INFLUXDB_ORG=ชื่อ-organization-ของคุณ
INFLUXDB_BUCKET=ชื่อ-bucket-ของคุณ
INFLUXDB_TOKEN=token-api-ของคุณ
```

รีสตาร์ท service ที่ต้องใช้ข้อมูล InfluxDB:

```powershell
docker compose restart nodered grafana
```

## การใช้งาน Grafana

เปิด Grafana:

```text
http://localhost:3000
```

Login ด้วยค่าจาก `.env`:

```env
GF_SECURITY_ADMIN_USER=admin
GF_SECURITY_ADMIN_PASSWORD=รหัสผ่านที่ตั้งไว้
```

เพิ่ม InfluxDB data source:

```text
URL: http://influxdb:8086
Organization: ค่าจาก InfluxDB
Bucket: ค่าจาก InfluxDB
Token: API Token จาก InfluxDB
```

หลังจาก Save & Test สำเร็จ สามารถสร้าง dashboard สำหรับแสดงข้อมูลพลังงานหรือ sensor ได้

## การใช้งาน Node-RED

เปิด Node-RED:

```text
http://localhost:1880
```

ค่าการเชื่อมต่อ MQTT broker ภายใน Docker network:

```text
Host: mqtt
Port: 1883
```

ค่าการเชื่อมต่อ InfluxDB ภายใน Docker network:

```text
URL: http://influxdb:8086
```

ตัวอย่าง flow ที่ควรสร้าง:

- MQTT subscribe รับข้อมูลจาก ESP32
- Function node ตรวจสอบ/แปลง payload
- InfluxDB node สำหรับบันทึก time-series data
- Switch node ตรวจ threshold เช่น ค่า kW เกินกำหนด
- Debug node หรือ HTTP request node สำหรับตรวจสอบ/ส่งต่อข้อมูลไปยังระบบภายนอกตามต้องการ

## การใช้งาน MQTT กับ ESP32

ESP32 หรือ IoT device ที่อยู่ใน network เดียวกับเครื่อง host สามารถ publish มาที่:

```text
Host: IP เครื่องที่รัน Docker Desktop
Port: 1883
Protocol: MQTT
```

ตัวอย่าง topic:

```text
factory/energy/meter01
factory/energy/meter02
factory/alerts/overload
```

ตัวอย่าง payload:

```json
{
  "device_id": "meter01",
  "voltage": 220.5,
  "current": 12.4,
  "power_kw": 2.73,
  "energy_kwh": 154.8
}
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

## ตรวจสอบปัญหาเบื้องต้น

ดู container ที่กำลังรัน:

```powershell
docker ps
```

ดู container ทั้งหมด:

```powershell
docker ps -a
```

ดู Docker networks:

```powershell
docker network ls
```

ดู Docker volumes:

```powershell
docker volume ls
```

ดู logs เฉพาะ service:

```powershell
docker compose logs nodered
docker compose logs mqtt
docker compose logs influxdb
docker compose logs grafana
```

## หมายเหตุด้านความปลอดภัย

- ห้าม commit ไฟล์ `.env` เพราะมี password/token จริง
- `.gitignore` ตั้งค่าให้ ignore `.env` แล้ว
- สำหรับ production ควรตั้ง password ที่แข็งแรง และควรเปิด authentication ให้ MQTT
- Mosquitto config ปัจจุบันเปิด `allow_anonymous true` เพื่อความสะดวกใน lab/dev

## เอกสารเพิ่มเติม

ดูขั้นตอน deploy แบบละเอียดสำหรับ Docker Desktop ได้ที่:

[DEPLOY_DOCKER_DESKTOP.md](DEPLOY_DOCKER_DESKTOP.md)
