// Librerias
//----------------------------------------------------------------------------------------------------------------------
#if defined(_WIN32) || defined(__WIN32__)
    // Libreria para estructura "in_addr"
    #include <winsock2.h>
#elif defined(linux) || defined(__linux)
    // Libreria para estructura "in_addr"
    #include <arpa/inet.h>
    // Librerias para recuperar direcci�n IP
    #include <netdb.h>
    // Librerias de correcci�n
    #include <sys/ioctl.h>
    #include <ifaddrs.h>
    #include <net/if.h>
    #include <pthread.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <sys/types.h>
	#include <sys/stat.h>
	#include <unistd.h>
	#include <errno.h>
	#include <fcntl.h>
	#include <sys/ipc.h>
    #include <sys/msg.h>
    #include <net/ethernet.h>
    #include <net/if_arp.h>
#endif

// Libreria "OTRAS"
#include <cstring>
#include <fcntl.h>
#include <errno.h>
#include <cstdio>

// Librerias con funciones generales
#include "zomutils/utiles.h"

// Definiciones - PUERTOS
//----------------------------------------------------------------------------------------------------------------------
#define PORT_Listen_M	9097	// Puerto por donde este servidor, escucha a sus maestros (Entrada) (* Es el puerto de entrada de los maestros, de este servidor)
#define PORT_Listen_C 	9096	// Puerto por donde este servidor, escucha a sus clientes (Entrada) (* Es el puerto de entrada de los clientes, de este servidor)

// Parametros del Servidor
#define NUMMax_Masters  100
#define NUMMax_Clients  100

// Variables
//----------------------------------------------------------------------------------------------------------------------
int *arryMaestros;	// = (int *)malloc(1);
int *arryClientes;	// = (int *)malloc(1);

int numMaestros = 0;
int numClientes = 0;

pthread_mutex_t candadoHiloMaestro; // Variable candado
pthread_mutex_t candadoHiloCliente; // Variable candado

// Anuncio de funciones y metodos
//----------------------------------------------------------------------------------------------------------------------
void iniciarEscuchaDeClientes();
void *fun_escucharAMaestros(void *mensaje);

void agregarCliente(int conexion);
void *fun_admiConnCliente(void *conexion);

// Metodos o funciones
void *fun_enviarMsjAClientes(void *mensaje);
void *fun_enviarMsjAMaestros(void *mensaje);

void agregarMaestro(int conexion);
void *fun_admiConnMaestro(void *conexion);

void quitarCliente(int conexion);
void *reenviarMensajesAClientes (void *mensaje);

void quitarMaestro(int conexion);
void *reenviarMensajesAMaestros (void *mensaje);

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

int main() {
    // Si Windows
    #if defined(_WIN32) || defined(__WIN32__)
        // Variables para validar red
        int errNet;
        WSADATA wsaData;
        WORD wVersionRequested;
        // Inicializar requerimientos de red
        wVersionRequested = MAKEWORD( 2, 2 );
        errNet = WSAStartup( wVersionRequested, &wsaData );
        // Validar inicio de red
        if(errNet != 0) {
            // Mostrar codigo de error
            printf("Fallo en la red, código de error: %d\n", errNet);
            // Limpiar configuraci�n de red
            WSACleanup();
        } else {
            // Recuperar mi informaci�n
            // Host Name
            sprintf(myHostName, "%s", recuperarHostName(TAM_HostN));
            printf("Host name: %s.\n", myHostName);
            // Direcci�n MAC
            sprintf(myMAC, "%s", recuperarDirMAC());
            printf("Dir. MAC: %s.\n", myMAC);
            // Direcci�n IP
            DWORD axIP = inet_addr(recuperarDirIP());
            myIP.S_un.S_addr = axIP;
            printf("Dir. IP: %s.\n", inet_ntoa(myIP));
            // Iniciar comunicaciones
            iniciarEscuchaDeClientes();
            // Limpiar configuraci�n de red
            WSACleanup();
        }
    #elif defined(linux) || defined(__linux)
        // Recuperar mi informaci�n
        // Host Name
        sprintf(myHostName, "%s", recuperarHostName(TAM_HostN));
        printf("Host name: %s.\n", myHostName);
        // Direcci�n MAC
        sprintf(myMAC, "%s", recuperarDirMAC());
        printf("Dir. MAC: %s.\n", myMAC);
        // Direcci�n IP
        myIP.s_addr = inet_addr(recuperarDirIP());
        printf("Dir. IP: %s.\n", inet_ntoa(myIP));
        // Iniciar comunicaciones
        iniciarEscuchaDeClientes();
    #endif
    // Finalizar programa
	return 0;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

// Funciones
//----------------------------------------------------------------------------------------------------------------------
void iniciarEscuchaDeClientes() {
    // Inicializar candados para hilos
    pthread_mutex_init(&candadoHiloMaestro, NULL);
    pthread_mutex_init(&candadoHiloCliente, NULL);
    // -----------------------------------------------------------------------------------------------------------------
    pthread_t idHilo_comunicaConMaestros;       // Identificador del Hilo
    pthread_attr_t attr_comunicaConMaestros;	// Atributos del Hilo
    pthread_attr_init(& attr_comunicaConMaestros);
    pthread_create(&idHilo_comunicaConMaestros, &attr_comunicaConMaestros, fun_escucharAMaestros, NULL);
    pthread_attr_destroy(& attr_comunicaConMaestros);
    // -----------------------------------------------------------------------------------------------------------------
    int socketParaClientes;
    // Socket TCP del servidor
    socketParaClientes = socket(AF_INET, SOCK_STREAM, 0);
    // Validar creaci�n del socket
    if (socketParaClientes < 0){
        // ------------------------------------------------
        printf("Error al crear el socket para Clientes.\n");
    } else {
        // Estructura de direcciones para el Servidor
        struct sockaddr_in serv_addr;
        // Estructura de direcciones para el Servidor
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(PORT_Listen_C);
        // Validar BINDING / Creaci�n de Sockets
        if (bind(socketParaClientes, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
            // ------------------------------------------------
            printf("Error en el binding del socket de Clientes.\n");
        }else{
            // Escucha n�mero de conexiones a clientes
            if(listen(socketParaClientes, numClientes) < 0){
                // ------------------------------------------------
                printf("Error en el listen para los Clientes.\n");
            }else{
                // Variables
                bool terminarProceso = 0;
                // Ciclo infinito de escucha
                while (!terminarProceso) {
                    // Variables
                    int client_Proxy_len = sizeof(struct sockaddr_in);
                    struct sockaddr_in cli_addr;
                    int conexionConCliente;
                    // SI WINDOWS
                    #if defined(WIN32) || defined(_WIN32) || defined(_WIN64)
                        // Aceptar la conexion del cliente
                        conexionConCliente = accept(socketParaClientes,(struct sockaddr *) &cli_addr, &client_Proxy_len);
                    #elif defined(linux) || defined(__linux)
                        // Aceptar la conexion del cliente
                        conexionConCliente = accept(socketParaClientes,(struct sockaddr *) &cli_addr, (socklen_t*) &client_Proxy_len);
                    #endif
                    // Validaci�n de conexi�n
                    if (conexionConCliente < 0) {
                        // ------------------------------------------------
                        printf("Error al aceptar la conexión de Cliente.\n");
                    }else{
                        // Agregar conexi�n con cliente
                        agregarCliente(conexionConCliente);
                        // Preparar hilo para administrar conexi�n
                        pthread_t idHilo_Cliente;	    // Identificador del Hilo
                        pthread_attr_t attr_Cliente;	// Atributos del Hilo
                        pthread_attr_init(& attr_Cliente);
                        pthread_create(&idHilo_Cliente, &attr_Cliente, fun_admiConnCliente, &conexionConCliente);
                        pthread_attr_destroy(& attr_Cliente);
                    }
                }
                close(socketParaClientes);
            }
        }
    }
}

void *fun_escucharAMaestros(void *mensaje) {
    // Recibir los mensajes de maestros
    int sockParaMaestros;
	// Socket TCP del servidor
	sockParaMaestros = socket(AF_INET, SOCK_STREAM, 0);
	if (sockParaMaestros < 0){
		 // ------------------------------------------------
        printf("Error al crear el socket para Maestros.\n");
	} else {
		// Estructura de direcciones para el Servidor y el Cliente
		struct sockaddr_in serv_addr;
		// Poner la estructura de direcciones del servidor en 0
		bzero((char *) &serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(PORT_Listen_M);
        // Validar Bindin
		if (bind(sockParaMaestros, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
			// ------------------------------------------------
            printf("Error en el binding del socket de Maestros.\n");
		}else{
			// Escucha "Servidor" numConexiones
			if(listen(sockParaMaestros, numMaestros) < 0){
				// ------------------------------------------------
                printf("Error en el listen para los Maestros.\n");
			}else{
				// Variables
                bool terminarProceso = 0;
                // Ciclo infinito de escucha
                while (!terminarProceso) {
                    // Variables
					int client_Proxy_len = sizeof(struct sockaddr_in);
					struct sockaddr_in cli_addr;
                    int conexionConMaestro;
                    // SI WINDOWS
                    #if defined(WIN32) || defined(_WIN32) || defined(_WIN64)
					    // Aceptar la conexion del cliente
					    conexionConMaestro = accept(sockParaMaestros,(struct sockaddr *) &cli_addr, &client_Proxy_len);
                    #elif defined(linux) || defined(__linux)
                        // Aceptar la conexion del cliente
                        conexionConMaestro = accept(sockParaMaestros,(struct sockaddr *) &cli_addr, (socklen_t*) &client_Proxy_len);
                    #endif
                    // Validaci�n de conexi�n
					if (conexionConMaestro < 0){
						// ------------------------------------------------
                        printf("Error al aceptar la conexión de Maestro.\n");
					}else{
						// Agregar conexi�n con maestro
						agregarMaestro(conexionConMaestro);
						// Preparar hilo para administrar conexi�n
						pthread_t idHilo_Master;	// Identificador del Hilo
						pthread_attr_t attr_Master;	// Atributos del Hilo
						pthread_attr_init(& attr_Master);
						pthread_create(&idHilo_Master, &attr_Master, fun_admiConnMaestro, &conexionConMaestro);
						pthread_attr_destroy(& attr_Master);
					}
				}
				close(sockParaMaestros);
			}
		}
	}
	return 0;
}

void agregarCliente(int conexion) {
    // Agregar conciencia de Cliente conectado
    // Bloquear hilo de clientes
    pthread_mutex_lock(&candadoHiloCliente);
    // Aumentar el numero de clientes
    numClientes++;
    // Validar cantidad de clientes en conexi�n
    if(numClientes < 2){
        // In crementa el tama�o del arreglo de clientes
        arryClientes = (int *) malloc(numClientes);
        // Reserva la conexi�n en la �ltima posici�n
        arryClientes[numClientes - 1] = conexion;
    }else{
        // Respaldar conexiones antiguas
        int iax;
        int numCliAntes = numClientes - 1;
        int *tempo = (int *) malloc(numCliAntes);
        for(iax = 0; iax < numCliAntes; iax++){
            tempo[iax] = arryClientes[iax];
        }
        // Reiniciar el arreglo de clientes
        arryClientes = NULL;
        arryClientes = (int *) malloc(numClientes);
        for (iax = 0; iax < numCliAntes; iax++) {
            arryClientes[iax] = tempo[iax];
        }
        // Salvar la nueva conexi�n en la �ltima posici�n
        arryClientes[numCliAntes] = conexion;
    }
    // Desbloquear hilo de clientes
    pthread_mutex_unlock(&candadoHiloCliente);
}

void *fun_admiConnCliente(void *conexion) {
    // Administrar Conexi�n con Cliente
    // Variables
    int conCliente = *(int *) conexion;
	int vivir = 1;
    // ------------------------------------------------
    printf("Entro un Cliente: %d.\n", conCliente);
	// Ciclo infinito de comunicaci�n con el cliente
	while(vivir){
        // Variables
		int iax;
		char mensajeJson[TAM_JsonBuffer];
		for(iax = 0; iax < TAM_JsonBuffer; iax++){
			mensajeJson[iax] = 0;
		}
        // Recibiendo mensaje de Cliente
		int nR = recv(conCliente, (char *) mensajeJson, TAM_JsonBuffer, 0);
		// Evaluar resultado de obteci�n de mensaje
		if((nR < 0)){
			printf("Se perdio conexion con el Cliente: %d.\n", conCliente);
			quitarCliente(conCliente);
			vivir = 0;
		}else if ((mensajeJson[0] == 0) || (!mensajeJson)) {
			printf("No se recibió algo del Cliente\n");
		}else{
			// Anunciar datos recibidos del cliente
			printf("Mensaje recibido del cliente:\n%s\n--------------------\n", mensajeJson);
 			struct formato_msj mensajeParaMaestro = processJsonToStruct(mensajeJson);
            // Preparar mensaje para maestro
			pthread_t idHilo_envioMsjMaster;	// Identificador del Hilo
			pthread_attr_t attr_envioMsjMaster;	// Atributos del Hilo
			pthread_attr_init(& attr_envioMsjMaster);
			pthread_create(&idHilo_envioMsjMaster, &attr_envioMsjMaster, reenviarMensajesAMaestros, &mensajeParaMaestro);
			pthread_attr_destroy(& attr_envioMsjMaster);
		}
	}
	close(conCliente);
	return 0;
}

void agregarMaestro(int conexion) {
    // Agregar conciencia de Maestro conectado
    // Bloquear hilo de maestros
    pthread_mutex_lock(&candadoHiloMaestro);
    // Aumentar el n�mero de maestros
    numMaestros++;
    // Validar cantidad de maestros
    if(numMaestros < 2){
        // Incrementar el espacio del arreglode maestros
        arryMaestros = (int *) malloc(numMaestros);
        arryMaestros[numMaestros - 1] = conexion;
    }else{
        // Respaldar conexiones antiguas
        int iax;
        int numMatrosAntes = numMaestros - 1;
        int *tempo = (int *) malloc(numMatrosAntes);
        for(iax = 0; iax < numMatrosAntes; iax++){
            tempo[iax] = arryMaestros[iax];
        }
        // Reiniciar el arreglo de maestros
        arryMaestros = NULL;
        arryMaestros = (int *) malloc(numMaestros);
        for(iax = 0; iax < numMatrosAntes; iax++){
            arryMaestros[iax] = tempo[iax];
        }
        // Salvar la nueva conexi�n en la �ltima posici�n
        arryMaestros[numMatrosAntes] = conexion;
    }
    // Desbloquear hilo de maestros
    pthread_mutex_unlock(&candadoHiloMaestro);
}

void *fun_admiConnMaestro (void *conexion) {
    // Administrar Conexi�n con Cliente
    // Variables
    int conMaestro = *(int *) conexion;
    int vivir = 1;
    // ------------------------------------------------
    printf("Entro un Maestro: %d.\n", conMaestro);
    // Ciclo infinito de conexi�n con el maestro
    while(vivir){
        // Variables
        struct formato_msj msjParaCliente; // = getSpellMsjStruct(mensajeJsonX);
        int nR = recv(conMaestro, (char *) &msjParaCliente, sizeof(msjParaCliente), 0);
        // Evaluar resultado de obteci�n de mensaje
        if((nR < 0)){
            printf("Se perdio conexión con el Maestro: %d.\n", conMaestro);
            quitarMaestro(conMaestro);
            vivir = 0;
        }else{
            // Imprimir mensaje enviado
            printf("Mensajen recibido del Maestro.\n");
            imprimirStruct(msjParaCliente);
            // ---------------------------------------------------------
            pthread_t idHilo_envioMsjCliente;	// Identificador del Hilo
            pthread_attr_t attr_envioMsjCliente;	// Atributos del Hilo
            pthread_attr_init(& attr_envioMsjCliente);
            pthread_create(&idHilo_envioMsjCliente, &attr_envioMsjCliente, reenviarMensajesAClientes, &msjParaCliente);
            pthread_attr_destroy(& attr_envioMsjCliente);
        }
    }
    close(conMaestro);
    return 0;
}

void quitarCliente(int conexion) {
    // Bloquear hilo cliente
    pthread_mutex_lock(&candadoHiloCliente);
    // Validar existencia de clientes
    if(numClientes > 0){
        // Reducir el n�mero de clientes
        numClientes--;
        // Variables
        int *tempo = (int *) malloc(numClientes);
        int iax_1 = 0;
        int iax_2 = 0;
        // Respaldar conexi�n a otros clientes
        while((iax_1 < numClientes) && (iax_2 < (numClientes + 1))){
            if(arryClientes[iax_2] != conexion){
                tempo[iax_1] = arryClientes[iax_2];
                iax_1++;
            }
            iax_2++;
        }
        // Reiniciar arreglo de clientes
        arryClientes = (int *) malloc(numClientes);
        // Tranferir conexi�n a otros clientes guardada
        for (iax_1 = 0; iax_1 < numClientes; iax_1++){
            arryClientes[iax_1] = tempo[iax_1];
        }
    }
    // Desbloquear hilo cliente
    pthread_mutex_unlock(&candadoHiloCliente);
}

void *reenviarMensajesAClientes(void *mensaje) {
    // Bloquear hilo de Cliente
    pthread_mutex_lock(&candadoHiloCliente);
    // Variables
    int iax;
    int rtm;
    char mensajeJson[TAM_JsonBuffer];
    struct formato_msj msjParaCliente = *(struct formato_msj *) mensaje;
    // Imprimir mensaje enviado
    printf("Reenviando mensaje al cliente...\n");
    imprimirStruct(msjParaCliente);
    // msjParaCliente.zIP.s_addr = inet_addr(inet_ntoa(myIP));	// Mi direccion IP
    // Convertir Structura a JSON
    sprintf(mensajeJson, "%s", processStructToJson(msjParaCliente));
    // Recorrer arreglo de Clientes
    for(iax = 0; iax < numClientes; iax++){
        // Ajuste de valor de variables
        rtm = -1;
        // Si es valido el Cliente
        if((arryClientes[iax] >= 0)){
            rtm = send(arryClientes[iax], (char *) mensajeJson, TAM_JsonBuffer, 0);
        }
        // Validar resultado de env�o
        if( rtm == -1){
            printf("Error al reenviar el mensaje al Cliente: %d.\n", arryClientes[iax]);
        }else{
            printf("Mensaje reenviado al Cliente: %d.\n", arryClientes[iax]);
        }
    }
    // Imprimir mensaje enviado
    printf("Mensaje para Clientes:\n%s\n--------------------.\n", mensajeJson);
    // Desbloquear hilo de Cliente
    pthread_mutex_unlock(&candadoHiloCliente);
    return 0;
}

void quitarMaestro(int conexion) {
    // Bloquear hilo maestro
    pthread_mutex_lock(&candadoHiloMaestro);
    // Validar conexiones a maestros
    if(numMaestros > 0){
        // Reducir conexiones a maestros
        numMaestros--;
        // Respaldar conexiones
        int *tempo = (int *) malloc(numMaestros);
        int iax_1 = 0;
        int iax_2 = 0;
        while((iax_1 < numMaestros) && (iax_2 < (numMaestros + 1))){
            if(arryMaestros[iax_2] != conexion){
                tempo[iax_1] = arryMaestros[iax_2];
                iax_1++;
            }
            iax_2++;
        }
        // Reiniciar arreglo de conexiones a maestros
        arryMaestros = (int *) malloc(numMaestros);
        for(iax_1 = 0; iax_1 < numMaestros; iax_1++){
            arryMaestros[iax_1] = tempo[iax_1];
        }
    }
    // Desbloquear hilo maestro
    pthread_mutex_unlock(&candadoHiloMaestro);
}

void *reenviarMensajesAMaestros(void *mensaje) {
    // Variables
    int iax;
    struct formato_msj msjParaMaestro = *(struct formato_msj *) mensaje;
    // msjParaMaestro.zIP.s_addr = inet_addr(inet_ntoa(myIP));	// Mi direccion IP
    // Bloquear hilo de Maestros
    pthread_mutex_lock(&candadoHiloMaestro);
    // Recorrer arreglo de conexiones a maestros
    for(iax = 0; iax < numMaestros; iax++){
        // Enviar mensaje
        if(send(arryMaestros[iax], (char *) &msjParaMaestro, sizeof(msjParaMaestro), 0) == -1){
            printf("Error al reenviar el mensaje al Maestro: %d.\n", arryMaestros[iax]);
        }else{
            printf("Mensaje reenviado al Maestro: %d.\n", arryMaestros[iax]);
        }
    }
    // Imprimir mensaje enviado
    imprimirStruct(msjParaMaestro);
    // Desbloquear hilo de Maestros
    pthread_mutex_unlock(&candadoHiloMaestro);
    return 0;
}
