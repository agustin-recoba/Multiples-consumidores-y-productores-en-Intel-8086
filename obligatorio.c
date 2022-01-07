// Autor: Agustín Recoba

#define CANT_CANALES 16
#define MAX_BUFFER 32
#define ESTADO 128

#define CON 0x1
#define HAB 0x2

typedef unsigned char byte;

// Estructura de un canal
byte PP[CANT_CANALES];
byte PC[CANT_CANALES];
byte PP_control[CANT_CANALES];
byte frec_consumo[CANT_CANALES];
byte tics_rest[CANT_CANALES];
byte controlAnt[CANT_CANALES];
byte dondeQuitar[CANT_CANALES];
byte dondePoner[CANT_CANALES];
byte cantIngresados[CANT_CANALES];

// Arreglo de buffers
byte buffers[CANT_CANALES*MAX_BUFFER];

byte cant_canales_config;

void main(void) {

    // Deshabilito interrupciones
    disable();

    //Instalar vector de interrupciones
    
    cant_canales_config = 0; // cantidad de canales configurados antes de hab = true
    
    while (true) {
        byte entrada = in(ESTADO);
        if (entrada == 0x02) {
            PP[cant_canales_config] = in(ESTADO);
            PP_control[cant_canales_config] = in(ESTADO);
            PC[cant_canales_config] = in(ESTADO);
            frec_consumo[cant_canales_config] = in(ESTADO);
            tics_rest[cant_canales_config] = frec_consumo[cant_canales_config];
            dondeQuitar[cant_canales_config] = 0;
            dondePoner[cant_canales_config] = 0;
            cantIngresados[cant_canales_config] = 0;
            controlAnt[cant_canales_config] = 0;

            cant_canales_config++;
        } else if (entrada == 0x01 || cant_canales_config == CANT_CANALES)
            break;
    } 

    // Habilito interrupciones
    enable();

    // pooling para entrada de datos
    while(true) {
        for (byte i = 0; i < cant_canales_config; i++) {
            byte control = in(PP_control[i]);
            control = control & 0x01;
            if (control == 0x01) {
                if (controlAnt[i] == 0x00) { // si hubo un flanco ascendente
                    producir(i);
                }
            }
            controlAnt[i] = control;
        }  
    }
}

// Procedimiento llamado cuando se debe enviar un dato por el canal i
void consumir(byte i) {
    char aConsumir;
    if (cantIngresados[i] > 0) {
        cantIngresados[i]--; 
        aConsumir = buffers[i*MAX_BUFFER + dondeQuitar[i]];
        dondeQuitar[i] = (dondeQuitar[i] + 1) % MAX_BUFFER;
    } else { // undeflow
        aConsumir = 0x0;
    }
    out(PC[i], aConsumir);
}

// Prcedimiento llamado cuando se debe leer un dato del canal i
void producir(byte i) {
    char aProducir = in(PP[i]);
    if (cantIngresados[i] < MAX_BUFFER) {
        cantIngresados[i]++;
        buffer[i * MAX_BUFFER + dondePoner] = aProducir;
        dondePoner[i] = (dondePoner[i] + 1) % MAX_BUFFER;
    } else { // overflow
        out(ESTADO, PP[i]);
    }
}

void interrupt timer(void) {
    for (byte i = 0; i < cant_canales_config; i++) {
        tics_rest[i]--;
        if (tics_rest[i] == 0) {
            consumir(i);
            tics_rest[i] = frec_consumo[i];
        }
    }
}