# Contapassaggi-per-pista-ciclabile
Centralina per il monitoraggio del traffico su pista ciclabile

Realizzazione di un contatore per il monitoraggio dei flussi su una pista ciclabile. E’ basato su un modulo ESP32 TTGO T-Call con a bordo un modulo GPRS per la trasmissione dei dati. Il contatore ha le seguenti funzionalità:
-	Conta i passaggi di pedoni o biciclette davanti alla postazione del contatore 
-	E’ completamente autonomo in termini di energia: richiede un pannello solare e delle batterie e non necessita di un collegamento alla rete elettrica
-	E’ autonomo nell’invio dei dati ad un cloud gratuito (thingspeak) mediante un modulo gprs, di conseguenza non necessita di collegamento ad internet
-	Può essere installato su un palo a 3-4 metri di altezza e quindi fuori portata di eventuali vandali o ladri
-	E’ low cost con componenti di facile reperimento sul mercato 
-	Può essere costruito da chiunque possa accedere ad una stampante 3D, sappia usare un saldatore e abbia conoscenze di base del mondo Esp32.

Il repository è composto di questi file/directory:
- Contapassaggi per pista ciclabile - ESP32.pdf che contiene le istruzioni per la costruzione della centralina contapassaggi
- Contapassaggi_TCall_final_da_aremetrizzare  che contiene il codice sorgente per Esp32 da parametrizzare
- RTCLib che contiene la libreria per la gestione dell'RTC
- TinyGSM che contiene la libreria per la gestione del modulo SIM800 all'interno del T-CALL
