#include "main.h"

DEBUG_PRINT_ENABLE;

//Handle de la cola
extern QueueHandle_t cola_datos_calculados;
extern QueueHandle_t cola_tarar;
extern QueueHandle_t cola_fuerza;
extern SemaphoreHandle_t sem_medir_fuerza;

/*==================[declaraciones de funciones internas]====================*/

/*==================[declaraciones de funciones externas]====================*/

/*==================[definiciones de datos internos]=========================*/

extern volatile unsigned long OFFSET;

/*==================[funcion principal]======================================*/

// FUNCION PRINCIPAL, PUNTO DE ENTRADA AL PROGRAMA LUEGO DE ENCENDIDO O RESET.
int main( void )
{
    // ---------- CONFIGURACIONES ------------------------------
    // Inicializar y configurar la plataforma
    boardConfig();
    HX711Config();


    // UART for debug messages
    debugPrintConfigUart( UART, BAUD_RATE );
    debugPrintlnString( "TP Profesional." );

    // Led para dar se�al de vida
    gpioWrite( LED3, ON );

    // Creaci�n de la cola
	/*cola_crear(cola_datos_calculados,CANT_MEDICIONES,SIZE_UINT);
	cola_crear(cola_tarar,SIZE,SIZE_UINT);
	cola_crear(cola_fuerza,SIZE,SIZE_UINT);*/

	// Creaci�n del sem�foro
	//sem_crear(sem_medir_fuerza);

    // Creaci�n de la cola
    cola_datos_calculados = xQueueCreate(CANT_MEDICIONES,sizeof(unsigned int));
    cola_tarar = xQueueCreate(1,sizeof(unsigned int));
    cola_fuerza = xQueueCreate(1,sizeof(unsigned int));

    if (cola_datos_calculados == NULL || cola_tarar == NULL || cola_fuerza == NULL){
    	gpioWrite( LED_ERROR , ON );
		printf( MSG_ERROR_QUEUE );
		while(TRUE);
    }

    // Creaci�n del sem�foro
    sem_medir_fuerza = xSemaphoreCreateBinary();

    if(sem_medir_fuerza == NULL){
    	gpioWrite( LED_ERROR , ON );
		printf( MSG_ERROR_SEM );
		while(TRUE);
    }

    // Creaci�n y validacion de las tareas
	tarea_crear(tarea_tarar,"tarea_tarar",SIZE,0,PRIORITY+1,NULL);
	tarea_crear(tarea_Rx_WIFI,"tarea_Rx",SIZE,0,PRIORITY,NULL);
	tarea_crear(tarea_Tx_WIFI,"tarea_Tx",SIZE,0,PRIORITY,NULL);

    // Creaci�n de las tareas
/*
    BaseType_t res =
	xTaskCreate(
		tarea_tarar,                     // Funcion de la tarea a ejecutar
		( const char * )"tarea_tarar",   // Nombre de la tarea como String amigable para el usuario
		configMINIMAL_STACK_SIZE*2, 	// Cantidad de stack de la tarea
		0,                          	// Parametros de tarea
		tskIDLE_PRIORITY+2,         	// Prioridad de la tarea
		0                           	// Puntero a la tarea creada en el sistema
	);

	if(res == pdFAIL)
	{
		gpioWrite( LED_ERROR , ON );
		printf( MSG_ERROR_TASK );
		while(TRUE);
	}

	res =
    xTaskCreate(
    	tarea_Rx_WIFI,                     // Funcion de la tarea a ejecutar
        ( const char * )"tarea_Rx",   // Nombre de la tarea como String amigable para el usuario
        configMINIMAL_STACK_SIZE*2, 	// Cantidad de stack de la tarea
        0,                          	// Parametros de tarea
        tskIDLE_PRIORITY+1,         	// Prioridad de la tarea
        0                           	// Puntero a la tarea creada en el sistema
    );

    if(res == pdFAIL)
    {
    	gpioWrite( LED_ERROR , ON );
		printf( MSG_ERROR_TASK );
		while(TRUE);
    }

    res =
    xTaskCreate(
    	tarea_Tx_WIFI,                     // Funcion de la tarea a ejecutar
        ( const char * )"tarea_Tx",   // Nombre de la tarea como String amigable para el usuario
        configMINIMAL_STACK_SIZE*2, 	// Cantidad de stack de la tarea
        0,                          	// Parametros de tarea
        tskIDLE_PRIORITY+1,         	// Prioridad de la tarea
        0                           	// Puntero a la tarea creada en el sistema
    );

    if(res == pdFAIL)
    {
    	gpioWrite( LED_ERROR , ON );
		printf( MSG_ERROR_TASK );
		while(TRUE);
    }*/

    	// Iniciar scheduler
    vTaskStartScheduler();

    // ---------- REPETIR POR SIEMPRE --------------------------
    while( TRUE )
    {
        // Si cae en este while 1 significa que no pudo iniciar el scheduler
    }

    // NO DEBE LLEGAR NUNCA AQUI, debido a que a este programa se ejecuta
    // directamenteno sobre un microcontroladore y no es llamado por ningun
    // Sistema Operativo, como en el caso de un programa para PC.
    return 0;
}

/*==================[definiciones de funciones internas]=====================*/

/*==================[definiciones de funciones externas]=====================*/
/*

void tarea_Rx_WIFI( void* taskParmPtr )
{
	fsmButtonInit();

	while( 1 )
	{
		fsmButtonUpdate( TEC1 );
	 	vTaskDelay( 1 / portTICK_RATE_MS );
	}
}

// Implementacion de funcion de la tarea
void tarea_Tx_WIFI( void* taskParmPtr )
{
	unsigned long fu = 0;
	float p = 0;
	char str_aux[50] = {};
    // ---------- CONFIGURACIONES ------------------------------
	TickType_t xPeriodicity =  1000 / portTICK_RATE_MS;		// Tarea periodica cada 1000 ms
	TickType_t xLastWakeTime = xTaskGetTickCount();
	// ---------- REPETIR POR SIEMPRE --------------------------
    while( 1 )
    {
    	if(xQueueReceive(cola_fuerza , &fu,  portMAX_DELAY)){			// Esperamos tecla

			gpioWrite( LED1, ON );
			vTaskDelay(40 / portTICK_RATE_MS);
			gpioWrite( LED1, OFF );

			p = (fu - OFFSET) / SCALE;

			if (p > 300){
				debugPrintString("Negativo corregido");
				p = 0;
			}

			debugPrintlnUInt(OFFSET);
			debugPrintString( "Fuerza: " );
			debugPrintlnUInt(fu);
			sprintf(str_aux, "%2.2f",p);
			debugPrintString( "Peso: " );
			debugPrintlnInt(p);

			// Delay peri�dico
			vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
    	}

    }
}

// Tarea que mide la fuerza
void tarea_medir( void* taskParmPtr )
{
	// ---------- CONFIGURACIONES ------------------------------
		TickType_t xPeriodicity =  500 / portTICK_RATE_MS;		// Tarea periodica cada 1000 ms
		TickType_t xLastWakeTime = xTaskGetTickCount();
		TickType_t dif = *( (TickType_t*)  taskParmPtr );

		// Inicializaci�n de par�metros
		unsigned char i;

		//gpioWrite(DataPin,1);
		gpioWrite(ClockPin,0);

		//tarea_crear(tarea_esperar,"tarea_esperar",SIZE,0,PRIORITY+1,&TaskHandle_esperar);
		//tarea_crear(tarea_promediar,"tarea_promediar",SIZE,0,PRIORITY+1,NULL);


		// Crear tarea de espera
			BaseType_t res =
			xTaskCreate(
				tarea_esperar,                     	// Funcion de la tarea a ejecutar
				( const char * )"tarea_esperar",   	// Nombre de la tarea como String amigable para el usuario
				configMINIMAL_STACK_SIZE*2, 	// Cantidad de stack de la tarea
				0,                	// Parametros de tarea
				tskIDLE_PRIORITY+2,         	// Prioridad de la tarea
				&TaskHandle_esperar                          	// Puntero a la tarea creada en el sistema
			);

			if(res == pdFAIL)
			{
				//error
			}

			// Crear tarea de espera
			res =
			xTaskCreate(
				tarea_promediar,                     	// Funcion de la tarea a ejecutar
				( const char * )"tarea_promediar",   	// Nombre de la tarea como String amigable para el usuario
				configMINIMAL_STACK_SIZE*2, 	// Cantidad de stack de la tarea
				0,                	// Parametros de tarea
				tskIDLE_PRIORITY+2,         	// Prioridad de la tarea
				0                          	// Puntero a la tarea creada en el sistema
			);

			if(res == pdFAIL)
			{
				//error
			}

	    // ---------- REPETIR POR SIEMPRE --------------------------
		while ( 1 ){

			// Esperar a que el m�dulo HX711 est� listo
			if (xSemaphoreTake( sem_medir_fuerza  ,  portMAX_DELAY )){

				gpioWrite( LEDG , 1 );
				vTaskDelay( dif );
				gpioWrite( LEDG , 0 );

				Count = 0;

				// Medir 24 pulsos
				for (i=0;i<24;i++)
					{
						gpioWrite(ClockPin,1);
						Count=Count<<1;
						gpioWrite(ClockPin,0);
						if(gpioRead(DataPin)){

							Count++;
						}
					}
				// Hacer medici�n final
				gpioWrite(ClockPin,1);
				Count=Count^0x800000;
				gpioWrite(ClockPin,0);

			xQueueSend(cola_datos_calculados , &Count,  portMAX_DELAY);

			}

			// Delay peri�dico
			vTaskDelayUntil( &xLastWakeTime , xPeriodicity );
		}

}

// Tarea que espera a que el HX711 este listo para medir
void tarea_esperar( void* taskParmPtr )
{
    // ---------- CONFIGURACIONES ------------------------------
	TickType_t xPeriodicity =  50 / portTICK_RATE_MS;		// Tarea periodica cada 1000 ms
	TickType_t xLastWakeTime = xTaskGetTickCount();

    // ---------- REPETIR POR SIEMPRE --------------------------
	while ( 1 ){

			gpioWrite( LEDR , 1 );
			vTaskDelay( 40 / portTICK_RATE_MS );
			gpioWrite( LEDR , 0 );

			if( !gpioRead(DataPin) ){
				// Indicar que el m�dulo ya est� listo para empezar a medir
				xSemaphoreGive( sem_medir_fuerza );
				vTaskDelete(NULL);
			}

			// Delay peri�dico
			vTaskDelayUntil( &xLastWakeTime , xPeriodicity );

		}

}

// Tarea que promedia los valores medidos
void tarea_promediar( void* taskParmPtr )
{
    // ---------- CONFIGURACIONES ------------------------------
	TickType_t xPeriodicity =  500 / portTICK_RATE_MS;		// Tarea periodica cada 1000 ms
	TickType_t xLastWakeTime = xTaskGetTickCount();

	unsigned long f;
	unsigned long sum = 0;
	int contador = 0;
	unsigned long prom;
    // ---------- REPETIR POR SIEMPRE --------------------------
	while ( 1 ){

			gpioWrite( LEDB , 1 );
			vTaskDelay( 40 / portTICK_RATE_MS );
			gpioWrite( LEDB , 0 );

			if(xQueueReceive(cola_datos_calculados , &f,  portMAX_DELAY)){			// Esperamos tecla
				//debugPrintString( "Espacio en cola: " );
				//debugPrintlnUInt(uxQueueSpacesAvailable(cola_datos_calculados));
				if (contador >= cant_promediada){
					prom = sum / cant_promediada;
					if (!tarado){
						xQueueSend(cola_tarar , &prom,  portMAX_DELAY);
						vTaskDelete(TaskHandle_medir);
						vTaskDelete(NULL);
					}
					else{
						xQueueSend(cola_fuerza , &prom,  portMAX_DELAY);
						vTaskDelete(TaskHandle_medir);
						vTaskDelete(NULL);
					}
				}
				else{
					sum += f;
					contador++;
					xSemaphoreGive( sem_medir_fuerza );
				}
			}

			// Delay peri�dico
			vTaskDelayUntil( &xLastWakeTime , xPeriodicity );

		}

}

// Tarea que setea el OFFSET
void tarea_tarar( void* taskParmPtr )
{
    // ---------- CONFIGURACIONES ------------------------------
	TickType_t xPeriodicity =  500 / portTICK_RATE_MS;		// Tarea periodica cada 1000 ms
	TickType_t xLastWakeTime = xTaskGetTickCount();

	TickType_t tiempo_diff = 40/ portTICK_RATE_MS;

	//tarea_crear(tarea_medir,"tarea_medir",SIZE,&tiempo_diff,PRIORITY+1,&TaskHandle_medir);

	// Crear tarea en freeRTOS
		BaseType_t res =
		xTaskCreate(
			tarea_medir,                     	// Funcion de la tarea a ejecutar
			( const char * )"tarea_medir",   	// Nombre de la tarea como String amigable para el usuario
			configMINIMAL_STACK_SIZE*2, 	// Cantidad de stack de la tarea
			&tiempo_diff,                	// Parametros de tarea
			tskIDLE_PRIORITY+2,         	// Prioridad de la tarea
			&TaskHandle_medir                          	// Puntero a la tarea creada en el sistema
		);

		if(res == pdFAIL)
		{
			//error
		}
	unsigned long offset;
    // ---------- REPETIR POR SIEMPRE --------------------------
	while ( 1 ){

			gpioWrite( LED2 , 1 );
			vTaskDelay( 40 / portTICK_RATE_MS );
			gpioWrite( LED2 , 0 );

			if(xQueueReceive(cola_tarar , &offset,  portMAX_DELAY)){			// Esperamos tecla
				OFFSET = offset;
				debugPrintString( "Offset: " );
				debugPrintlnUInt(offset);
				tarado = true;
				gpioWrite( LED2, ON );
				vTaskDelete(NULL);
			}
			else{
				debugPrintString( "Offset no paso" );
			}

			// Delay peri�dico
			vTaskDelayUntil( &xLastWakeTime , xPeriodicity );

		}

}

/*==================[fin del archivo]========================================*/
