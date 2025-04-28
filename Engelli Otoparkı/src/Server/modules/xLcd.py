from RPLCD.i2c import CharLCD

lcd = CharLCD(i2c_expander='PCF8574', address=0x3f, port=1, cols=20, rows=4, charmap='A00', auto_linebreaks=True)

def Print_to_lcd(text):
    replacements = str.maketrans('ĞÜŞİÇÖığüşçö', 'GUSICOigusco')
    lines = text.translate(replacements).split('\n')
    lcd.clear()
    for i, line in enumerate(lines[:4]):
        lcd.cursor_pos = (i, 0)
        lcd.write_string(line[:20])

if __name__ == "__main__":
    Print_to_lcd("merhaba ii \n helllllllo")