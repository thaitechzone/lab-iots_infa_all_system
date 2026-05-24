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
