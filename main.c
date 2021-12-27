/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include "project.h"

#define and &&
#define or ||

/***Control de la EEPROM***/
#define fila_1_byte_0 0x00000000
#define fila_2_byte_0 0x00000010
#define fila_3_byte_0 0x00000020

/***Variables para el control del sistema***/
typedef struct Variables{
    uint16 ms:11;
    uint16 IR_1:1;
    uint16 IR_2:1;
    uint16 pasajero_subiendo:1;
    uint16 pasajero_bajando:1;
    uint16 contador_pasajeros:11;
    
} Variable;
Variable variable;

void imprimir(void);  // Función que permite la comunicación con el usuario

/***Funciones correspondiente a las interrupciones externas***/
CY_ISR_PROTO(Cronometro);
CY_ISR_PROTO(Infrarrojo_1);
CY_ISR_PROTO(Infrarrojo_2);

int main(void)
{
    CyGlobalIntEnable; /* Enable global interrupts. */

    /***Inicializacion de Interrupciones***/
    isr_IR_1_StartEx(Infrarrojo_1);
    isr_IR_2_StartEx(Infrarrojo_2);
    isr_contador_StartEx(Cronometro);
    
    /***Inicializacion de Componentes***/
    LCD_Start();
    EEPROM_Start();
    Contador_Start();
    
    /***Inicializacion del Sistema***/
    LCD_ClearDisplay();
    variable.contador_pasajeros = EEPROM_ReadByte(fila_1_byte_0);                   // Lee la ultima cantidad de pasajeros
    
    LCD_Position(0,3);
    LCD_PrintString("***Limobus***");
    LCD_Position(1,0);
    LCD_PrintNumber(variable.contador_pasajeros);                                   // Escribe la ultima cantidad de pasajeros
    LCD_PrintString(" Pasajeros abordo");
    

    for(;;)
    {
        /***** Pasajero Subiendo *****/
        // Condicion: Activacion del IR_1 + Que no se haya activado el IR_2 antes
        if (variable.IR_1 == 1 and D0_Sensor_IR_1_Read() == 0 and variable.pasajero_bajando == 0){
            variable.IR_1 = 0;
            
            variable.ms = 0;                                                        // Inicia el cronometro de 1.5s = 1500ms
            variable.pasajero_subiendo = 1;                                         // Activa bandera que identifica el proceso de subida
        }
        
        // Condicion: Bandera que identifica el proceso de SUBIDA ACTIVADA + se activa el IR_2
        if (variable.IR_2 == 1 and D0_Sensor_IR_2_Read() == 0 and variable.pasajero_subiendo == 1){
            variable.IR_2 = 0;
            variable.pasajero_subiendo = 0;                                         // Se desactiva la bandera del proceso de subida
            
            variable.contador_pasajeros++;                                          // Se aumenta la cantidad de pasajeros 
            CyPins_SetPin(alerta_Buzzer);                                           // se activa un sonido y se imprime
            imprimir();
        }
        
        /***** Pasajero Bajando *****/
        // Condicion: Activacion del IR_2 + Que no se haya activado el IR_1 antes
        if (variable.IR_2 == 1 and D0_Sensor_IR_2_Read() == 0 and variable.pasajero_subiendo == 0){
            variable.IR_2 = 0;
            
            variable.ms = 0;                                                        // Inicia el cronometro de 1.5s = 1500ms
            variable.pasajero_bajando = 1;                                          // Activa bandera que identifica el proceso de bajada
        }
        
        // Condicion: Bandera que identifica el proceso de BAJADA ACTIVADA + se activa el IR_1
        if (variable.IR_1 == 1 and D0_Sensor_IR_1_Read() == 0 and variable.pasajero_bajando == 1){
            variable.IR_1 = 0;
            variable.pasajero_bajando = 0;                                          // Se desactiva la bandera del proceso de bajada
            
            if (variable.contador_pasajeros > 0) variable.contador_pasajeros--;     // Se disminuye la cantidad de pasajeros si aun hay alguno
            CyPins_SetPin(alerta_Buzzer);                                           // se activa un sonido y se imprime
            imprimir();
        }
    }
}

void imprimir(void){
    LCD_ClearDisplay();
    LCD_Position(0,5);
    LCD_PrintString("!Limobus!");
    EEPROM_WriteByte(variable.contador_pasajeros, fila_1_byte_0);                   // Se guarda en la memoria los pasajeros actuales
    CyPins_ClearPin(alerta_Buzzer);
    
    if (variable.contador_pasajeros < 11){                                          // Rutina de impresion de cantidad de pasajeros
        CyPins_ClearPin(alerta_LED);
        LCD_Position(1,0);
        LCD_PrintNumber(variable.contador_pasajeros);
        LCD_PrintString(" Pasajeros abordo");
    }
    
    else {                                                                          // Rutina de impresion cuando se super el limite de pasajeros
        LCD_Position(0,3);
        LCD_PrintString("***BUS LLENO***");
        CyPins_SetPin(alerta_LED);
    }
    
}

CY_ISR(Infrarrojo_1){
    variable.IR_1 = 1;                                                              // Bandera del sensor Infrarojo 1
}

CY_ISR(Infrarrojo_2){
    variable.IR_2 = 1;                                                              // Bandera del sensor Infrarojo 2
}

CY_ISR(Cronometro){
    variable.ms++;
    if (variable.ms > 1500){                                                        // Clave de todo: Sí se reinicia el cronometro
        variable.pasajero_subiendo = 0;                                             // en el "for infinito" y pasan 1.5s = 1500ms
        variable.pasajero_bajando = 0;                                              // se restablecen las banderas que identifican
    }                                                                               // la subida o bajada
}
/* [] END OF FILE */
