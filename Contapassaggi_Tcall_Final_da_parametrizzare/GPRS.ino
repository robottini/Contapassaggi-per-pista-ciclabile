void sendData() {
  // Mosfet for the power supply ON
  Serial.println(F("Start grps"));
  powerUp();
  Serial.println(F("End start grps"));

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  Serial.println(F("Initializing modem..."));
  modem.restart();
  // Or, use modem.init() if you don't need the complete restart

  String modemInfo = modem.getModemInfo();
  Serial.print(F("Modem: "));
  Serial.println(modemInfo);
  delay(500);


  byte Vbatt = int(modem.getBattVoltage() / 100);
  Serial.print(F("Batt (V):")); Serial.println(float(Vbatt) / 10.0);
  //  Serial.print(F("Battery V:")); Serial.println(float(batt)/10.0);
  delay(100);

  Serial.println(F("Waiting for network..."));
  if (!modem.waitForNetwork(40000L)) {
    Serial.println(F(" fail"));
    StopGprs();
    return;
  }
  else {
    Serial.println(F(" OK"));
  }


  if (modem.isNetworkConnected()) {
    Serial.println(F("Network connected"));
  }

  Serial.print(F("Connecting to APN: "));
  Serial.print(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    Serial.println(F("  APN fail"));
    StopGprs();
    return;
  }
  else {
    Serial.println(F("   APN OK"));
  }


  gsmTime = modem.getGSMDateTime(DATE_TIME);
  Serial.print("GSM Time:"); Serial.println(gsmTime);
  gsmDate = modem.getGSMDateTime(DATE_DATE);
  Serial.print("GSM Date:"); Serial.println(gsmDate);
  parseDateTime();

  IPAddress local = modem.localIP();
  Serial.print(F("Local IP: "));
  Serial.println(local);
  char appo3[3];
  char appo6[6];
  jsonStr[0] = '\0';
  strcat(jsonStr, "{  \"write_api_key\": \"");
  strcat(jsonStr, thingspeak_WriteAPIKey);
  strcat(jsonStr, "\", \"updates\": [");
  data_length = 52; //52 is the length of jsonStr
  for (byte i = 0; i < ore; i++) {
    riga[0] = '\0'; //clear the riga char array
    strcat(riga, "{ \"created_at\": \"");

    //Anno
    strcat(riga, "20");
    itoa(anno, appo3, 10);
    strcat(riga, appo3);
    strcat(riga, "-");

    //Mese
    appo3[0] = '0' + ((mese / 10) % 10); // tens digit
    appo3[1] = '0' + (mese % 10);      // ones digit
    appo3[2] = 0;        // ones digit
    strcat(riga, appo3); //mese
    strcat(riga,  "-");

    //Giorno
    appo3[0] = '0' + ((giorno / 10) % 10); // tens digit
    appo3[1] = '0' + (giorno % 10);      // ones digit
    appo3[2] = 0;        // ones digit
    strcat(riga, appo3); //giorno
    strcat(riga,  " ");

    //ora
    if ((i + 1) == 23 && ore == 23)
      itoa(23, appo3, 10);
    else
      itoa(i, appo3, 10);
    strcat(riga, appo3); // i

    //minuti e secondi
    //    strcat(riga, ":55:01 +0100");
    strcat(riga, ":55:01 +");
    appo3[0] = '0';
    appo3[1] = '0' + (fuso / 4);
    appo3[2] = 0;        // ones digit
    strcat(riga, appo3);
    strcat(riga, "00");


    //campo 1 - passaggi orari
    //campo 2 - passaggi orari cumulati
    strcat(riga, "\", \"field1\":");
    itoa(passaggi[i], appo6, 10);
    strcat(riga, appo6);  //passaggi[i]
    strcat(riga, ", \"field2\":");
    itoa(cumulati[i], appo6, 10);
    strcat(riga, appo6); //cumulati[i]
    strcat(riga, ", \"field4\":");

    //campo 4 - tensione batteria
    if (ora == i)
      batt[i] = Vbatt;
    dtostrf( (float(batt[i]) / 10.0), 3, 1, appo6);
    strcat(riga, appo6);

    //campo 5 - temperatura
    strcat(riga, ", \"field5\":");
    dtostrf( 0, 3, 1, appo3);
    strcat(riga, appo3);


    if (i < (ore - 1)) {
      strcat(riga, "},");
    }
    else {
      if (i == 23|| i == 22) {
        //campo 3 - passaggi giornalieri
        strcat(riga, ", \"field3\":");
        itoa(totaleGiorno, appo6, 10);
        strcat(riga, appo6);
      }

      strcat(riga, "}]}");
    }
    data_length += strlen(riga);
  }

  data_length = data_length + 1;
  Serial.print(F("Content-Length: ")); Serial.println(String(data_length));


  Serial.print(F("Connecting to "));
  Serial.println(server);

  // lengthString();
  client.stop();
  delay(1000);


  if (client.connect(server, 80)) {
    client.print("POST /channels/");
    client.print(thingspeak_Channel);
    client.println("/bulk_update.json HTTP/1.1"); // Replace YOUR-CHANNEL-ID with your ThingSpeak channel ID
    delay(50);
    client.println(F("Host: api.thigspeak.com"));
    delay(50);
    client.println(F("Connection: close"));
    delay(50);
    client.println(F("Content-Type: application/json"));
    delay(50);
    client.println("Content-Length: " + String(data_length));
    client.println();
    delay(100);

    client.print(jsonStr);
    Serial.print(jsonStr);
    delay(100);

    for (byte i = 0; i < ore; i++) {
      riga[0] = '\0'; //clear the riga char array
      strcat(riga, "{ \"created_at\": \"");

      //Anno
      strcat(riga, "20");
      itoa(anno, appo3, 10);
      strcat(riga, appo3);
      strcat(riga, "-");

      //Mese
      appo3[0] = '0' + ((mese / 10) % 10); // tens digit
      appo3[1] = '0' + (mese % 10);      // ones digit
      appo3[2] = 0;        // ones digit
      strcat(riga, appo3); //mese
      strcat(riga,  "-");

      //Giorno
      appo3[0] = '0' + ((giorno / 10) % 10); // tens digit
      appo3[1] = '0' + (giorno % 10);      // ones digit
      appo3[2] = 0;        // ones digit
      strcat(riga, appo3); //giorno
      strcat(riga,  " ");

      //ora
      if ((i + 1) == 23 && ore == 23)
        itoa(23, appo3, 10);
      else
        itoa(i, appo3, 10);
      strcat(riga, appo3); // i

      //minuti e secondi
      //strcat(riga, ":55:01 +0100");
      strcat(riga, ":55:01 +");
      appo3[0] = '0';
      appo3[1] = '0' + (fuso / 4);
      appo3[2] = 0;        // ones digit
      strcat(riga, appo3);
      strcat(riga, "00");

      //campo 1 - passaggi orari
      //campo 2 - passaggi orari cumulati
      strcat(riga, "\", \"field1\":");
      itoa(passaggi[i], appo6, 10);
      strcat(riga, appo6);  //passaggi[i]
      strcat(riga, ", \"field2\":");
      itoa(cumulati[i], appo6, 10);
      strcat(riga, appo6); //cumulati[i]


      //campo 4 - tensione batteria
      strcat(riga, ", \"field4\":");
      //     if (ora == i)
      //       batt[i] = Vbatt;
      dtostrf( (float(batt[i]) / 10.0), 3, 1, appo6);
      strcat(riga, appo6);

      //campo 5 - temperatura
      strcat(riga, ", \"field5\":");
      dtostrf( 0, 3, 1, appo3);
      strcat(riga, appo3);


      if (i < (ore - 1)) {
        strcat(riga, "},");
      }
      else {
        if (i == 23 || i == 22) {
          //campo 3 - passaggi giornalieri
          strcat(riga, ", \"field3\":");
          itoa(totaleGiorno, appo6, 10);
          strcat(riga, appo6);
        }

        strcat(riga, "}]}");
      }
      client.print(riga);
      Serial.println(riga);
      // riga = "";
      delay(100);
    }
    client.println();

  }
  else {
    StopGprs();
    Serial.println(F("Failure: Failed to connect to ThingSpeak"));
    return;
  }
  delay(3000); //Wait to receive the response

  boolean trovat = false;
  int ndx = 0;
  int endNdx = 23;
  unsigned long timeout = millis();
  while (client.connected() && millis() - timeout < 5000L) {
    while (client.available() && ndx < endNdx) {
      receivedChars[ndx] = client.read();
      ndx++;
    }
    if (!trovat) {
      Serial.print(F("receivedChars:")); Serial.println(receivedChars);
      int g = 0;
      while (g < endNdx - 8 && !codeFound) {
        if (receivedChars[g] == '/') {
          char code[4];
          code[0] = receivedChars[g + 5];
          code[1] = receivedChars[g + 6];
          code[2] = receivedChars[g + 7];
          code[3] = '\0';
          Serial.print(F("Code:")); Serial.println(code);
          if (strcmp(code, "202") == 0) {
            Serial.println(F("Saved correctly"));
            codeFound = true;
            lampeggioLed(3);
          }
        }
        g++;
      }
      Serial.println(F("Altri caratteri:"));
      trovat = true;
    }
    char c = client.read();
    Serial.print(c);
    timeout = millis();
  }

  Serial.println();

  // Shutdown
  client.stop();
  Serial.println(F("Server disconnected"));

  modem.gprsDisconnect();
  Serial.println(F("GPRS disconnected"));
  passaggi[0] = 0;
  cumulati[0] = 0;

  // Mosfet for the power supply OFF
  StopGprs();
}

void  StopGprs() {
  Serial.println(F("Start stop gprs"));
  powerDown();
  Serial.println(F("End stop gprs"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void parseDateTime() {
  //   GSM Time:17:41:34+04
  //   GSM Date:19/12/28
  int x = 0;
  int y = 0;
  x = gsmTime.indexOf(":");
  String valore;
  valore = gsmTime.substring(0, x);
  ora = valore.toInt();
  y = gsmTime.lastIndexOf(":");
  valore = gsmTime.substring(x + 1, y);
  minuti = valore.toInt();
  x = gsmTime.indexOf("+");
  valore = gsmTime.substring(y + 1, x);
  secondi = valore.toInt();
  valore = gsmTime.substring(x + 1, gsmTime.length());
  fuso = valore.toInt();
  Serial.println("Ora:" + String(ora) + " Minuti:" + String(minuti) + " Secondi:" + secondi + " Fuso:" + fuso);
  x = gsmDate.indexOf("/");
  valore = gsmDate.substring(0, x);
  if (!firstData)
    anno = valore.toInt();
  else
    annoFirstData = valore.toInt();
  y = gsmDate.lastIndexOf("/");
  valore = gsmDate.substring(x + 1, y);
  if (!firstData)
    mese = valore.toInt();
  else
    meseFirstData = valore.toInt();
  valore = "";
  valore = gsmDate.substring(y + 1, gsmDate.length());
  if (!firstData)
    giorno = valore.toInt();
  else
    giornoFirstData = valore.toInt();
  if (firstData) {
    DateTime firstDay = DateTime(annoFirstData, meseFirstData, giornoFirstData, ora, minuti , secondi);
    firstDay = firstDay + TimeSpan(-1, 0, 0, 0);
    mese = firstDay.month();
    giorno = firstDay.day();
  }
  Serial.println("giorno:" + String(giorno) + " Mese:" + String(mese) + " Anno:" + String(anno));
  rtc.adjust(DateTime(anno, mese, giorno, ora, minuti, secondi));
}



void powerUp()
{
  digitalWrite(MODEM_PWKEY, LOW);
  digitalWrite(MODEM_RST, HIGH);
  digitalWrite(MODEM_POWER_ON, HIGH);
  delay(3000);
}


void powerDown()
{
  digitalWrite(MODEM_POWER_ON, LOW);
  delay(1000);
}
