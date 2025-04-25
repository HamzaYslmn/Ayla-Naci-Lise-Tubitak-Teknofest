import cv2
import asyncio
from usermanager import AddUser
from modules import sqlite
import datetime

def get_input(prompt, default):
    return input(f"{prompt} (default: {default}): ") or default

def show_qr_frame(cap, detector):
    ret, frame = cap.read()
    if not ret:
        return None
    data, points, _ = detector.detectAndDecode(frame)
    if points is not None:
        pts = points[0].astype(int)
        for j in range(len(pts)):
            cv2.line(frame, tuple(pts[j]), tuple(pts[(j+1)%len(pts)]), (0,255,0), 3)
    cv2.imshow("QR Code Scanner", frame)
    key = cv2.waitKey(1000 if data else 1)
    if key & 0xFF == ord('q'):
        return "quit"
    return data or None

async def log_event(user_uuid, status):
    log = {
        "user_uuid": user_uuid,
        "status": status,
        "timestamp": datetime.datetime.now().isoformat(' ')
    }
    await sqlite.ensure_table("logs", log)
    await sqlite.create_db("logs", log)

async def get_last_status(user_uuid):
    await sqlite.ensure_table("logs", {
        "user_uuid": user_uuid,
        "status": "entry",
        "timestamp": datetime.datetime.now().isoformat(' ')
    })
    sql = 'SELECT status FROM logs WHERE user_uuid=? ORDER BY timestamp DESC LIMIT 1'
    result = await sqlite.execute_db(sql, (user_uuid,), fetch="one")
    return result["status"] if result else None

async def add_user_flow():
    name = get_input("Enter user name", "Ali Veli")
    degree = get_input("Enter degree of disability", "80%")
    user_uuid = await AddUser.add_user(name, degree)
    print("User added with UUID:", user_uuid)
    
async def check_user_flow(user_uuid):
    try:
        user_data = await sqlite.read_db("users", uuid=user_uuid)
    except Exception as e:
        return False
    if not user_data:
        return False
    return user_data
    
async def verified_flow():
    print("User verified ✔️")
    print("Door opened!")
    await asyncio.sleep(3)
    print("Door closed!")

async def read_user_flow():
    cap, detector = cv2.VideoCapture(0), cv2.QRCodeDetector()
    try:
        while True:
            uuid = None
            while not uuid:
                uuid = show_qr_frame(cap, detector)
                if uuid == "quit":
                    return
                if not uuid:
                    await asyncio.sleep(0.03)
            user_data = await check_user_flow(uuid)
            if user_data:
                status = "exit" if await get_last_status(uuid) == "entry" else "entry"
                await log_event(uuid, status)
                print(f"!    {status.capitalize()} - {user_data['name']} at {datetime.datetime.now().isoformat(' ')}")
                await asyncio.sleep(5)
            else:
                print("User not found ❌")
                await asyncio.sleep(5)

    finally:
        cap.release()
        cv2.destroyAllWindows()

async def main():
    option = input("Select an option:\n1. Add User\n2. Read User\n\n")
    if option == "1":
        await add_user_flow()
    elif option == "2":
        await read_user_flow()

if __name__ == "__main__":
    asyncio.run(main())