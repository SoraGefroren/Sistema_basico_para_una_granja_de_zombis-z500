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
#define PORT_Listen_S	9096	// Puerto por donde este cliente, se comunica con el servidor (Salida)

// Variables
//----------------------------------------------------------------------------------------------------------------------
char host_name[] = "192.168.100.18"; // 0.0.0.0 // Direcci�n del servidor

// Anuncio de funciones y metodos
//----------------------------------------------------------------------------------------------------------------------
void conectarAlServidor();
void enviarMensajeAlServidor(int conexion);
void escucharRespuestaDelServidor(int conexion);

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
            conectarAlServidor();
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
        conectarAlServidor();
    #endif
    // Finalizar programa
	return 0;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

// Funciones
//----------------------------------------------------------------------------------------------------------------------
void conectarAlServidor() {
    // Variables
    int socketCliente ;
    // Validar creaci�n de socket
	if((socketCliente = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		printf("Error al crear la socket hacia el Servidor.\n");
		return;
	}
	// Estructura de la conexion
	struct hostent *server_hostname = gethostbyname(host_name);
    struct sockaddr_in serverWeb;
    // Configuraci�n de conexi�n
    bzero((char *) &serverWeb, sizeof(serverWeb));
    serverWeb.sin_family = AF_INET;
	serverWeb.sin_port = htons(PORT_Listen_S);
   	serverWeb.sin_addr = *((struct in_addr *)(server_hostname->h_addr_list[0]));
   	// Conexion al Servidor
	if(connect(socketCliente, (struct sockaddr *)&serverWeb, sizeof(struct sockaddr)) == -1){
		printf("Error al iniciar la conexion con el Servidor.\n");
		return;
   	}else{
   		printf("Conexion con el Servidor correcta. :D\n");
   	}
   	// Comunicaci�n con el servidor
    printf("--------------------------------------------------------------\n");
    enviarMensajeAlServidor(socketCliente);
    printf("--------------------------------------------------------------\n");
    printf("--------------------------------------------------------------\n");
    escucharRespuestaDelServidor(socketCliente);
    printf("--------------------------------------------------------------\n");
	//Cerrar conexion
	close(socketCliente);
}

void enviarMensajeAlServidor(int conexion) {
    // Variable mensaje
    struct formato_msj msjStruct;
	// Estructurando respuesta para el Servidor
	msjStruct.clave = 4;
	msjStruct.zIP.s_addr = inet_addr(host_name);
    msjStruct.xIP.s_addr = inet_addr(inet_ntoa(myIP));
	// Variable
    int iax;
    // Limpiar y agregar direccion MAC del Zombie
    for(iax = 0; iax < TAM_DirMAC; iax++) {
        msjStruct.xMAC[iax] = 0;
        msjStruct.xMAC[iax] = myMAC[iax];
    }
    // Limpiar y agregar nombre de host
    for (iax = 0; iax < TAM_HostN; iax++) {
        msjStruct.xHostN[iax] = 0;
        msjStruct.xHostN[iax] = myHostName[iax];
    }
    // ---------------------------------------------------------------
	sprintf(msjStruct.xDatos, "%s", "");
	strcpy (msjStruct.xDatos, "");
    // Cadena JSON
	char mensajeJson[TAM_JsonBuffer];
	// Prepara espacio de cadena JSON
	for(iax = 0; iax < TAM_JsonBuffer; iax++){
		mensajeJson[iax] = 0;
	}
 	// Convertir estructura a cadena
	sprintf(mensajeJson, "%s", processStructToJson(msjStruct));
	// Vakudar conexi�n y envio de datos
	if(send(conexion, (char *) mensajeJson, TAM_JsonBuffer, 0) == -1){
		printf("Error al enviar el mensaje al Servidor.\n");
	}else{
		printf("Mensaje enviado al Servidor.\n");
	}
    // Mensaje enviado
	printf("Mensaje Json:\n%s\n--------------------\n", mensajeJson);
}

void escucharRespuestaDelServidor(int conexion) {
    // Variables
	int iax;
	char mensajeJson[TAM_JsonBuffer];
	// Recibir mensaje JSON de Servidor
	int rNr = recv(conexion, mensajeJson, TAM_JsonBuffer, 0);
	// Evaluar mensaje leido desde Servidor
    if (rNr < 0) {
        // Mensaje de error
        printf("Se perdio conexión con el Servidor: %d.\n", conexion);
    } else {
        struct formato_msj msjStruct;
        // Objeto JSON
        Document document;
        document.Parse((const char *) mensajeJson);
        // Asignar valores a mensaje
        msjStruct.clave = document["clave"].GetInt();
        msjStruct.xIP.s_addr = inet_addr(document["xIP"].GetString());
        msjStruct.zIP.s_addr = inet_addr(document["zIP"].GetString());
        sprintf(msjStruct.xMAC, "%s", document["xMAC"].GetString());
        sprintf(msjStruct.xHostN, "%s", document["xHostN"].GetString());
        sprintf(msjStruct.xDatos, "%s", document["xDatos"].GetString());
        //------------------------------------------------
        // Imprimir mensaje enviado
        printf("Mensaje Json:\n%s\n--------------------\n", mensajeJson);
        imprimirStruct(msjStruct);
        //------------------------------------------------
    }
}
