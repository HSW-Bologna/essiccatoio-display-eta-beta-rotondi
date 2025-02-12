const char *pars_descriptions[57][2] = {
    {"Lingua", "Language", },
    {"Livello di accesso", "Access level", },
    {"Visualizzare temperatura", "Show temperature", },
    {"Tasto menu", "Menu button", },
    {"Tempo tasto pausa", "Pause button time", },
    {"Tempo tasto stop", "Stop button time", },
    {"Disabilita allarmi", "Disable alarms", },
    {"Tempo allarme temperatura", "Temperature alarm delay", },
    {"Temperatura di sicurezza", "Safety temperature", },
    {"Tipo macchina occupata", "Busy signal type", },
    {"Direzione allarme filtro", "Filter alarm direction", },
    {"Direzione allarme inverter", "Inverter alarm direction", },
    {"Sonda di temperatura", "Temperature probe", },
    {"Tipo riscaldamento", "Heating mode", },
    {"Tentativi accesione gas", "Gas ignition attempts", },
    {"Ferma il tempo in pausa", "Stop time in pause", },
    {"Direzione contatto oblo'", "Porthole contact direction", },
    {"Direzione contatto macchina occupata", "Busy signal contact direction", },
    {"Durata", "Duration", },
    {"Tempo di rotazione", "Rotation time", },
    {"Tempo di pausa", "Pause time", },
    {"Temperatura", "Temperature", },
    {"Isteresi riscaldamento", "Heating hysteresis", },
    {"Isteresi raffreddamento", "Cooling hysteresis", },
    {"Umidita", "Humidity", },
    {"Riscaldamento progressivo", "Progressive heating", },
    {"Durata massima", "Max duration", },
    {"Cicli massimi", "Max cycles", },
    {"Tempo di ritardo", "Delay time", },
    {"Velocita'", "Speed", },
    {"Tipo", "Type", },
    {"Tempo allarme flusso aria", "Air flow alarm time", },
    {"Tempo ventilazione oblo' aperto", "Porthole open fan time", },
    {"Inversione ventola e cesto", "Invert fan and drum", },
    {"Abilita attesa temperatura", "Wait for temperature", },
    {"Abilita inversione", "Enable reverse", },
    {"Lingua massima utente", "Max user language", },
    {"Logo", "Logo", },
    {"Velocita' minima", "Minimum speed", },
    {"Velocita' massima", "Maximum speed", },
    {"Temperatura massima in ingresso", "Maximum input temperature", },
    {"Temperatura massima in uscita", "Maximum output temperature", },
    {"Temperatura di sicurezza in ingresso", "Input safety temperature", },
    {"Temperatura di sicurezza in uscita", "Output safety temperature", },
    {"Tempo di ritorno alla lingua iniziale", "Initial language reset period", },
    {"Tempo di ritorno alla pagina iniziale", "Initial page reset period", },
    {"Controllo umidita' residua", "Residual humidity check", },
    {"Autoavvio", "Autostart", },
    {"Direzione allarme emergenza", "Emergency alarm direction", },
    {"Tempo azzeramento ciclo", "Reset cycle time", },
    {"Tempo per gettone", "Time per coin", },
    {"Numero minimo di gettoni", "Minimum number of coins", },
    {"Tipo di richiesta pagamento", "Payment request type", },
    {"Numero di cicli prima della manutenzione", "Number of cycles before maintentance", },
    {"Cadenza avviso manutenzione", "Maintenance notice delay", },
    {"Durata avviso manutenzione", "Maintenance notice duration", },
    {"Tipo pagamento", "Payment type", },
};

const char *pars_nc_na[2][2] = {
    {"NC", "NC", },
    {"NA", "NO", },
};

const char *pars_manuale_automatico[2][2] = {
    {"Manuale", "Manual", },
    {"Automatico", "Automatic", },
};

const char *pars_lingue[5][5] = {
    {"Italiano", "Italian", "Italiano", "Italien", "Italienisch", },
    {"Inglese", "English", "Inglés", "Anglais", "Englisch", },
    {"Spagnolo", "Spanish", "Español", "Espagnol", "Spanisch", },
    {"Francese", "French", "Francés", "Français", "Französisch", },
    {"Tedesco", "German", "Alemán", "Allemand", "Deutsch", },
};

const char *pars_tipo_richiesta_pagamento[3][2] = {
    {"Inserire gettone", "Insert token", },
    {"Inserire monete", "Insert coin", },
    {"Pagare cassa", "Pay at desk", },
};

const char *pars_livello_accesso[2][2] = {
    {"Utente", "User", },
    {"Tecnico", "Technician", },
};

const char *pars_tipo_pagamento[4][2] = {
    {"Nessuno", "None", },
    {"Normalmente aperto", "Normally open", },
    {"Normalmente chiuso", "Normally closed", },
    {"Gettoniera", "Coin reader", },
};

const char *pars_tipo_macchina_occupata[3][2] = {
    {"Segnalazione allarmi e attivita'", "Signal alarms and activity", },
    {"Segnalazione allarmi", "Signal alarms", },
    {"Segnalazione attivita'", "Signal activity", },
};

const char *pars_tipo_riscaldamento[2][2] = {
    {"Electtrico", "Electric", },
    {"Gas", "Gas", },
};

const char *pars_sonda_temperatura[3][2] = {
    {"Ingresso", "Input", },
    {"Uscita", "Output", },
    {"Temperatura/umidita'", "Temperature/humidity", },
};

const char *pars_nosi[2][2] = {
    {"No", "No", },
    {"Si", "Yes", },
};

const char *pars_loghi[5][2] = {
    {"Nessuno", "None", },
    {"MSGroup", "MSGroup", },
    {"Rotondi", "Rotondi", },
    {"Hoover", "Hoover", },
    {"Unity Laundry", "Unity Laundry", },
};

