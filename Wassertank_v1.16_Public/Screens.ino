
void  printWiFidetails(){
  Serial.println();
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("signal strength (RSSI): ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
  Serial.println();
  Serial.println();
}


void OLED_Screen1(){                        // Wellcome screen
  u8g2.setFont(u8g2_font_7x14B_tr);         // choose a small font for status dispay
  u8g2.clearBuffer();
  u8g2.drawStr(0,11,"    Wellcome to");
  u8g2.setFont(u8g2_font_profont22_tf);
  u8g2.drawStr(0,32, "PegelRegler");
  u8g2.setFont(u8g2_font_7x14B_tr);
  u8g2.drawStr(0,51,"    Connecting..");
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0,64,"v1.16");  u8g2.sendBuffer();
}

void OLED_Screen2(){                      // Connected screen
  char oledTxt[25]      = "";
  u8g2.setFont(u8g2_font_7x14B_tr);       // choose a small font for status dispay
  u8g2.clearBuffer();
  u8g2.drawStr(0,11,"WiFi connected");
  u8g2.drawStr(0,27,"SSID: ");
  WiFi.SSID().toCharArray(oledTxt, sizeof(oledTxt));
  u8g2.drawStr(37,27,oledTxt);
  u8g2.drawStr(0,43,"IP: ");
  sprintf(oledTxt, "%03d.%03d.%03d.%03d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  u8g2.drawStr(23,43, oledTxt);  
  u8g2.drawStr(0,60,"RSSI: ");  
  sprintf (oledTxt, "%03i", WiFi.RSSI());
  u8g2.drawStr(38,60,oledTxt); 
  u8g2.drawStr(58,60," dBm");  u8g2.sendBuffer();
}


void OLED_Screen4(float rainx[4]){                      // Main Screen
  String test ;
  char oledTxt[25]      = "";
  float totalRain = 0;
  char result[6];
  u8g2.setFont(u8g2_font_7x14B_tr);       
  u8g2.clearBuffer();
  
  u8g2.drawStr(0,11,"Soll: ");
  u8g2.drawStr(70,11,"Ist: ");
  sprintf (oledTxt, "%03i", Sollwert);
  u8g2.drawStr(40,11, oledTxt);
  sprintf (oledTxt, "%03i", Istwert);
  u8g2.drawStr(100,11, oledTxt);
 
  for (int i=0; i <= 3; i++){ 
    totalRain += rain[i];
  }
    dtostrf(totalRain, 4, 1, result);
    test = "Regen: ";
    test = test + result + " mm/9h";
    test.toCharArray(oledTxt, test.length()+1) ;
    u8g2.drawStr(0,26,oledTxt);
    u8g2.drawStr(0,41,"Messwert:");
    //test = FiltVal + " mm";
    test = FiltVal;
    test = test + " mm";
    test.toCharArray(oledTxt, test.length()+1) ;
    u8g2.drawStr(70,41,oledTxt);
    if (weatherOK) {
      u8g2.drawStr(0,56,"Wetterdaten: OK");
    }
    else{
      u8g2.drawStr(0,56,"Wetterdaten: Keine");
    }
  u8g2.sendBuffer(); 
}


void OLED_Screen3(float rainx[4], float windx[4], float temp[4]){                      // Weather details
  char result[6];
  String test ;
  char oledTxt[25];
  float totalRain = 0;
  u8g2.setFont(u8g2_font_6x12_mf);
  u8g2.clearBuffer();
  u8g2.drawStr(0,9," Rain   Wind   Temp");

  for (int i=0; i <= 3; i++){ 
    dtostrf(rainx[i], 4, 1, result);
    test = result;
    test.toCharArray(oledTxt, test.length()+1) ;
    u8g2.drawStr(0,(i*11)+20,oledTxt);
    totalRain += rain[i];
  }
    dtostrf(totalRain, 4, 1, result);
    test = result;
    test = test + " mm Total Regen";
    test.toCharArray(oledTxt, test.length()+1) ;
    u8g2.drawStr(0,64,oledTxt);

  for (int i=0; i <= 3; i++){ 
    dtostrf(windx[i], 4, 1, result);
    test = result;
    test.toCharArray(oledTxt, test.length()+1) ;
    u8g2.drawStr(45,(i*11)+20,oledTxt);
  }
  for (int i=0; i <= 3; i++){ 
    dtostrf(temp[i], 4, 1, result);
    test = result;
    test.toCharArray(oledTxt, test.length()+1) ;
    u8g2.drawStr(90,(i*11)+20,oledTxt);
  }
  u8g2.sendBuffer();
}

void Screenchange(){
    if (MainScreen){
      OLED_Screen3(rain, wind, temp);      //OLED_Screen3(rain,wind,temp);
      MainScreen =0;
    }
    else{
      OLED_Screen4(rain);      //Main screen;
      MainScreen =1;
    }
    lastScreenchange = millis();
}
