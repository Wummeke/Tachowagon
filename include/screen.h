
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ I2C_SCL, /* data=*/ I2C_SDA);   // pin remapping with ESP8266 HW I2C

void screen_print(const char * text, uint8_t x, uint8_t y)
{
    u8g2.clearBuffer();                 // clear the internal memory
    u8g2.setFont(u8g2_font_crox2c_tr); // choose a suitable font
    u8g2.setCursor(x, y);              // set write position
    u8g2.print(text);         // write something to the internal memory
    u8g2.sendBuffer();                  // transfer internal memory to the display
}

void print_speed(float speed)
{
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_profont22_tr);
    u8g2.setCursor(0, 21);
    u8g2.printf("%.1f", speed);
    u8g2.drawStr(80, 21, "km/h");
    u8g2.sendBuffer();
}

void screen_setup()
{
    Serial.print("Screen Orientation set to: ");
    Serial.println(settings.screenrotated);
    Wire.begin(I2C_SDA, I2C_SCL);
    u8g2.begin();
    u8g2.setFlipMode(settings.screenrotated); // flip 180 degree clockwise
    screen_print("Starting...", 0 , 24); 
}