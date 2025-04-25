import uuid
import asyncio
from modules import sqlite
import qrcode

Name = "Ali Veli"
Degree_of_disability = "80%"

async def add_user(name, degree_of_disability):
    user = {
        "uuid": str(uuid.uuid4()),
        "name": name,
        "degree_of_disability": degree_of_disability
    }
    await sqlite.create_db("users", user)
    print("User added:", user)

    # Generate QR code with user's UUID
    qr = qrcode.QRCode(
        version=1,
        error_correction=qrcode.constants.ERROR_CORRECT_L,
        box_size=10,
        border=4,
    )
    qr.add_data(user["uuid"])
    qr.make(fit=True)
    img = qr.make_image(fill_color="black", back_color="white")
    img.save(f"{user['uuid']}_qrcode.png")
    qr.print_ascii()

    return user["uuid"]