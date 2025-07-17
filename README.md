# Essiccatoio

## Compilazione

Il firmware dovrebbe essere compilato con i seguenti strumenti:
 - ESP-IDF v5.3.x (testato con v5.3.2)

## Caricamento Firmware

| binario | indirizzo |
|---------|-----------|
| bootloader.bin | 0x1000 |
| partition_table.bin | 0x8000 |
| ota_data_initial.bin | 0xd000 |
| essiccatoio-display-eta-beta-rotondi.bin | 0x10000 |


## TODO

 - Un delta per ogni programma, ricordandoseli quando viene cambiato durante il ciclo
 - La velocita' di rotazione finale va proporzionata alla massima
