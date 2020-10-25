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

// Librerias con funciones generales
#include "zomutils/utiles.h"

// Definiciones - PUERTOS
//----------------------------------------------------------------------------------------------------------------------
#define PoAf_Listen	9097	// Puerto por donde este maestro, escucha al servidor (Entrada) (* Es el puerto de entrada para este maestro)
#define PORT_Listen	5025	// Puerto por donde este maestro, escucha a sus zombies (Entrada) (* Es el puerto de entrada para este maestro)
#define PORT_Speak	5000	// Puerto por donde este maestro, responde a sus zombies (Salida) (* Se espera, sea el puerto de entrada del lado del zombie)

// Variables
//----------------------------------------------------------------------------------------------------------------------
pthread_mutex_t candadoHilo_toServ;
char host_name[] = "localhost"; // 0.0.0.0 // Direcci�n del servidor
int sock_toServ; // ID del Socket de conexi�n al Servidor

// Anuncio de funciones y metodos
//----------------------------------------------------------------------------------------------------------------------

void iniciarEscuchaDelServidor();
void *fun_enviarMsjAlServidor(void *mensaje);

void *fun_iniciarEscuchaDeZombies(void *mensaje);
void *fun_enviarMsjALosZombies(void *mensaje);

void *fun_presentarseConElServidor(void *mensaje);

// Funciones adicionales
//----------------------------------------------------------------------------------------------------------------------

int reconectarConServidor();

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
            iniciarEscuchaDelServidor();
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
        iniciarEscuchaDelServidor();
    #endif
    // Finalizar programa
	return 0;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

// Funciones
//----------------------------------------------------------------------------------------------------------------------
void iniciarEscuchaDelServidor() {
    // Validar creaci�n de socket de conexi�n al Servidor
    if((sock_toServ = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		printf("Error al crear el socket de conexión para el Sevidor.\n");
		return;
	}
	// Estructura de la conexion
	struct hostent *serverHostname = gethostbyname(host_name);
    struct sockaddr_in serverWeb;
    // Configurar conexi�n
    bzero((char *) &serverWeb, sizeof(serverWeb));
    serverWeb.sin_family = AF_INET;
	serverWeb.sin_port = htons(PoAf_Listen);
   	serverWeb.sin_addr = *((struct in_addr *)(serverHostname->h_addr_list[0]));
   	// Realizar conexi�n con el servidor
	if(connect(sock_toServ, (struct sockaddr *) &serverWeb, sizeof(struct sockaddr)) == -1){
		printf("Error al iniciar la conexion con el Servidor.\n");
		return;
   	}
   	// Bandera de proceso Entrada/Salida
    bool terminarProceso = 0;
    // Inicializar el candado para los hilos.
	pthread_mutex_init(&candadoHilo_toServ, NULL);
    // Configurar Hilo
	pthread_attr_t attrLZ;			// Atributos del Hilo
	pthread_t idHiloDeZombies;	    // Identificador del Hilo
	pthread_attr_init(& attrLZ);
	pthread_create(&idHiloDeZombies, &attrLZ, fun_iniciarEscuchaDeZombies, NULL);
	pthread_attr_destroy(& attrLZ);
    // Ciclo infinito de escucha del Servidor
	while (!terminarProceso) {
		printf("--------------------------------------------------------------\n");
		printf("Escuchando al servidor...\n");
		// Variable para recibir mensaje
		struct formato_msj mensaje;
		// Recibir mensaje/petici�n
		recv(sock_toServ, (char*) &mensaje, sizeof(mensaje), 0);
		// Validar propiedad de mensaje
		if ((mensaje.clave == 3) || (string(inet_ntoa(mensaje.zIP)) == string(inet_ntoa(myIP)))) {
            // Variables
            pthread_t idHiloRespuesta;	    	// Identificador del Hilo
            pthread_attr_t attrHiloRespuesta;	// Atributos del Hilo
            // Atender mensaje
            switch(mensaje.clave){
                case 0:
                    // Terminar proceso
                    printf("Orden - Términar.\n");
                    terminarProceso = 1;
                    break;
                case 1:
                    printf("Orden - Presentarse.\n");
                        pthread_attr_init(& attrHiloRespuesta);
                        pthread_create(&idHiloRespuesta, &attrHiloRespuesta, fun_presentarseConElServidor, &mensaje);
                        pthread_attr_destroy(& attrHiloRespuesta);
                    break;
                default:
                    printf("Orden - Enviar mensaje a Zombies.\n");
                        pthread_attr_init(& attrHiloRespuesta);
                        pthread_create(&idHiloRespuesta, &attrHiloRespuesta, fun_enviarMsjALosZombies, &mensaje);
                        pthread_attr_destroy(& attrHiloRespuesta);
                    break;
            }
		} else {
            // Si, por error, se recibe un mensaje no dirigido a si mismo
            printf("Orden - No correspondida.\n");
        }
		printf("--------------------------------------------------------------\n");
	}
	// Cerrar socket
	close(sock_toServ);
}

void *fun_enviarMsjAlServidor(void * mensaje) {
    // Variables mensaje
    struct formato_msj msjServidor = *(struct formato_msj *) mensaje;
    // Bloquear hilo de comunicaci�n con el servidor
    pthread_mutex_lock(&candadoHilo_toServ);
    // Enviar mensajes al servidor
    int rt = send(sock_toServ, (char*) &msjServidor, sizeof(msjServidor), 0);
    // Desbloquear hilo de comunicaci�n con el servidor
    pthread_mutex_unlock(&candadoHilo_toServ);
    // Validar envio de mensaje
    if ( rt == -1) {
        printf("Error reenviar el mensaje del Zombie, al Servidor.\n");
        // Bloquear hilo de comunicaci�n con el servidor
        pthread_mutex_lock(&candadoHilo_toServ);
            // Mensaje de reconexi�n
            printf("Reconectando...\n");
            // Intentar reconectar con Servidor
            while (reconectarConServidor() != 0){
                printf("Reconectando...\n");
            }
        // Desbloquear hilo de comunicaci�n con el servidor
        pthread_mutex_unlock(&candadoHilo_toServ);
    } else {
        printf("Mensaje reenviado al Servidor.\n");
    }
    // Imprimir mensaje enviado
    imprimirStruct(msjServidor);
    return 0;
}

void *fun_iniciarEscuchaDeZombies(void * mensaje) {
	// Variables
	int sin_len;
	int socket_descriptor;	    // Id del Socket
	bool terminarProceso = 0;   // Bandera de proceso Entrada/Salida
	// Iniciar estructura de direcciones de socket para protocolos
	struct sockaddr_in sin;
	// Configurar socket de direcciones para comunicaci�n
	memset(&(sin.sin_zero), '\0', 8);
 	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(PORT_Listen); // Puerto por donde escucha el maestro (Entrada)
	sin_len = sizeof(sin); // Tama�o del SOKECT de comunicaci�n

	// Crea un socket UDP y lo une al puerto
	socket_descriptor = socket(AF_INET, SOCK_DGRAM, 0);
	bind(socket_descriptor, (struct sockaddr *)&sin, sizeof(sin));

	// Ciclo infinito de escucha de Zombies
	while(!terminarProceso){
		// Espera a la respuesta de los Zombies
		printf("--------------------------------------------------------------\n");
		printf("Escuchando a Zombies...\n");
		// Recibir mensaje
		struct formato_msj msjZombi;
        // Si es WINDOWS
        #if defined(WIN32) || defined(_WIN32) || defined(_WIN64)
		    recvfrom(socket_descriptor, (char*) &msjZombi, sizeof(msjZombi), 0, (struct sockaddr *) &sin, &sin_len); // Esperando respuesta de Zombies en forma de "Spell_msj"
        #elif defined(linux) || defined(__linux)
            recvfrom(socket_descriptor, (char*) &msjZombi, sizeof(msjZombi), 0, (struct sockaddr *) &sin, (socklen_t*) &sin_len); // Esperando respuesta de Zombies en forma de "Spell_msj"
        #endif
		// Reenviar MENSAJE de Zombies para el SERVIDOR
		printf("Reenviando mensaje de Zombie, al Servidor...\n");
		pthread_t idHiloRespuesta;	        // Identificador del Hilo
		pthread_attr_t attrHiloRespuesta;	// Atributos del Hilo
		pthread_attr_init(& attrHiloRespuesta);
		pthread_create(&idHiloRespuesta, &attrHiloRespuesta, fun_enviarMsjAlServidor, &msjZombi);
		pthread_attr_destroy(& attrHiloRespuesta);
		printf("--------------------------------------------------------------\n");
	}
	close(socket_descriptor);
	return 0;
}

void *fun_enviarMsjALosZombies(void *mensaje) {
    // Variables mensaje
    struct formato_msj msjZombie = *(struct formato_msj *) mensaje;
    int socket_descriptor; // Descriptor del socket
    int bOptVal = 1;
    //Estructura de direcciones en socket
    struct sockaddr_in socketZombie;
    memset(&(socketZombie.sin_zero), '\0', 8);
    // Configurar socket
    socketZombie.sin_family = AF_INET;
    socketZombie.sin_port = htons(PORT_Speak); // Puerto por donde habla a los Zombies (Salida)
    socketZombie.sin_addr.s_addr = inet_addr("255.255.255.255"); // Preparar Broadcast a toda la red
    //Crear socket Zombie UDP
    socket_descriptor = socket(AF_INET, SOCK_DGRAM, 0);
    // Validar creaci�n de Socket para Zombies
    if (setsockopt(socket_descriptor, SOL_SOCKET, SO_BROADCAST, (char *) &bOptVal, sizeof(bOptVal)) < 0) {
        printf("Error al crear Socket para hablar con los Zombies.\n");
    } else {
        // Estructura del hechizo
        msjZombie.zIP.s_addr = inet_addr(inet_ntoa(myIP));	// Direci�n IP del Maestro, Mi direccion IP
        //Llamando a los Zombies...
        if (sendto(socket_descriptor, (char *) &msjZombie, sizeof(msjZombie), 0, (struct sockaddr *) &socketZombie, sizeof(socketZombie)) == -1) {
            printf("Error al enviar el mensaje a los Zombies.\n");
        } else {
            // Cerrar socket
            printf("Mensaje enviado a los Zombies.\n");
            close(socket_descriptor);
        }
    }
    // Imprimir mensaje enviado
    imprimirStruct(msjZombie);
    return 0;
}

void *fun_presentarseConElServidor(void *mensaje) {
    // Variable de Mensaje para el Servidor
    struct formato_msj msjServidor = *(struct formato_msj *) mensaje;
    // Bloquear hilo de comunicaci�n con el servidor
    pthread_mutex_lock(&candadoHilo_toServ);
    // Estructurando respuesta para el Servidor
    msjServidor.zIP.s_addr = inet_addr(inet_ntoa(myIP));
    // Variables
    int iax;
    // Limpiar y agregar direccion MAC del Zombie
    for(iax = 0; iax < TAM_DirMAC; iax++){
        msjServidor.xMAC[iax] = 0;
        msjServidor.xMAC[iax] = myMAC[iax];
    }
    // Limpiar y agregar nombre de host
    for (iax = 0; iax < TAM_HostN; iax++) {
        msjServidor.xHostN[iax] = 0;
        msjServidor.xHostN[iax] = myHostName[iax];
    }
    // Limpiar espacio de datos
    for(iax = 0; iax < TAM_Datos; iax++){
        msjServidor.xDatos[iax] = 0;
    }
    // Enviar mensaje al servidor
    int rt = send(sock_toServ, (char *) &msjServidor, sizeof(msjServidor), 0);
    // Desbloquear hilo de comunicaci�n con el servidor
    pthread_mutex_unlock(&candadoHilo_toServ);
    // Validar envio de datos al servidor
    if(rt == -1){
        printf("Error al enviar mensaje al Servidor.\n");
    }else{
        printf("Mensaje enviado al Servidor.\n");
    }
    // Imprimir mensaje enviado
    imprimirStruct(msjServidor);
    return 0;
}

// Funciones adicionales
//----------------------------------------------------------------------------------------------------------------------
int reconectarConServidor() {
    // Dormir hilo de conexi�n al Servidor
    sleep(10);
    // Crear Socket de conexi�n con el Servidor
    if((sock_toServ = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        printf("Error al crear el socket de conexión para el Sevidor, en la reconexion.\n");
        return -1;
    }
    // Estructura de la conexion
    struct hostent *server_hostname = gethostbyname(host_name);
    struct sockaddr_in serverWeb;
    bzero((char *) &serverWeb, sizeof(serverWeb));
    // Configurar conexi�n
    serverWeb.sin_family = AF_INET;
    serverWeb.sin_port = htons(PoAf_Listen);
    serverWeb.sin_addr = *((struct in_addr *)(server_hostname->h_addr_list[0]));
    // Realizar conexi�n con el Servidor
    if(connect(sock_toServ, (struct sockaddr *)&serverWeb, sizeof(struct sockaddr)) == -1){
        printf("Error al reconertarse con el Servidor.\n");
        return -1;
    }else{
        printf("Reconexion exitosa con el Servidor.\n");
    }
    return 0;
}


