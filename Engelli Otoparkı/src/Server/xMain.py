import asyncio
from usermanager import AddUser
from modules import sqlite
from modules import camera
from modules import xLcd
import datetime

def get_input(prompt, default):
    return input(f"{prompt} (default: {default}): ") or default

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
    except Exception:
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
    cam, detector = camera.get_camera()
    try:
        while True:
            uuid = None
            while not uuid:
                uuid = camera.show_qr_frame(cam, detector)
                if uuid == "quit":
                    return
                if not uuid:
                    await asyncio.sleep(0.03)
            user_data = await check_user_flow(uuid)
            if user_data:
                status = "exit" if await get_last_status(uuid) == "entry" else "entry"
                await log_event(uuid, status)
                if status == "entry":
                    xLcd.Print_to_lcd(f"Welcome {user_data['name']}")
                    print(f"Welcome {user_data['name']}! ✔️")
                else:
                    xLcd.Print_to_lcd(f"Goodbye {user_data['name']}")
                    print(f"Goodbye {user_data['name']}! ✔️")
                await asyncio.sleep(5)
            else:
                print("User not found ❌")
                await asyncio.sleep(5)
    finally:
        camera.release_camera(cam)

async def main():
    option = input("Select an option:\n1. Add User\n2. Read User\n\n")
    if option == "1":
        await add_user_flow()
    elif option == "2":
        await read_user_flow()

if __name__ == "__main__":
    asyncio.run(main())