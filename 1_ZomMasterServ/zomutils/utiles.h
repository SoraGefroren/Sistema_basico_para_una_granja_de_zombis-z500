// Libreria para "printf"
#include <stdio.h>

// Librerias para "recuperarHostName"
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>

// Para escaneo de servicios
#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include <algorithm>
#include <future>
#include <cstring>

// Librerias adicionales
//----------------------------------------------------------------------------------------------------------------------
#if defined(_WIN32) || defined(__WIN32__)
    // Libreria para estructura "in_addr"
    #include <winsock2.h>
    // Librerias para recuperar direcci�n IP
    #include <windows.h>
    #include <iphlpapi.h>
    // Para escaneo de servicios
    #include <ws2tcpip.h>
    #include <wspiapi.h>
    #include <w32api.h>
    // Adicionales
    #pragma comment(lib,"WS2_32.lib")
    #pragma comment(lib, "iphlpapi.lib")
#elif defined(linux) || defined(__linux)
    // Libreria para estructura "in_addr"
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <sys/ioctl.h>
    #include <linux/if.h>
    // Librerias para recuperar direcci�n IP
    #include <netdb.h>
#endif

// Librerias String
#include <strings.h>
#include <sstream>
#include <string>
using namespace std;

// Librerias JSON
#include "rapidjson/writer.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
using namespace rapidjson;

// Definiciones generales
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
#define TAM_DirMAC        8  // Tama�o de la direcci�n MAC
#define TAM_HostN       100  // Tama�o para el nombre del host
#define TAM_Datos      2500  // Tama�o para datos enviados/recibidos

// Tamaño json
#define TAM_JsonBuffer  7500

// BZERO
#if defined(_WIN32) || defined(__WIN32__)
    #define bzero(b, len) (memset((b), '\0', (len)), (void) 0)
#endif

// Estructuras de datos
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
struct formato_msj {
    int clave = 0;             // N�mero de petici�n
	struct in_addr xIP;		   // Direccion IP (Propia)
	struct in_addr zIP;		   // Direccion IP (Solicitante)
	char  xMAC[TAM_DirMAC];    // Direccion MAC
	char  xHostN[TAM_HostN];   // Host Name o Nombre del Host
	char  xDatos[TAM_Datos];   // Espacio auxiliar para datos
};

// Variables
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
struct in_addr myIP;        // Mi direccion IP
char myMAC[TAM_DirMAC];	    // Mi direccion MAC
char myHostName[TAM_HostN];	// Mi nombre de host

// Funciones y métodos
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

char* recuperarHostName(int tamHN) {
    // Variables
    char * vAuxToHN = 0;
	char * myNameAux = (char *) malloc(tamHN);
	// Preparar arreglo de caracteres
	memset(myNameAux, 0 , tamHN);
	// Si es WINDOWS o LINUX
	#if defined(WIN32) || defined(_WIN32) || defined(_WIN64)
        // Obtener nombre de Host
        vAuxToHN = getenv("COMPUTERNAME");
        // Validar
        if (vAuxToHN != 0) {
            myNameAux = vAuxToHN;
        }
    #elif defined(linux) || defined(__linux)
        // Obtener nombre de Host
        vAuxToHN = getenv("HOSTNAME");
        // Validar
        if (vAuxToHN != 0) {
            myNameAux = vAuxToHN;
        } else {
            // Formatear variable auxiliar
            vAuxToHN = (char *) malloc(tamHN);
            // Preparar arreglo de caracteres
            memset(vAuxToHN, 0 , tamHN);
            // Validar y obtener nombre de host
            if (gethostname(vAuxToHN, tamHN) == 0) {
                myNameAux = vAuxToHN;
            }
            delete []vAuxToHN;
        }
    #endif
	// Regresar resultado
	return myNameAux;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

char* recuperarDirMAC() {
    // Variables
    char *macAddr = (char*)malloc(18);
    char *macDireccion  = 0;
    // Windows o Linux
    #if defined(_WIN32) || defined(__WIN32__)
        // Variables
        PIP_ADAPTER_INFO AdapterInfo;
        DWORD dwBufLen = sizeof(IP_ADAPTER_INFO);
        AdapterInfo = (IP_ADAPTER_INFO *) malloc(sizeof(IP_ADAPTER_INFO));
        // Validar existencia del adaptador
        if (AdapterInfo == NULL) {
            // printf("Error asignando la memoria necesaria para llamar al GetAdaptersinfo\n");
            free(macAddr);
            return macDireccion;
        }
        // Realizar una llamada inicial al GetAdaptersInfo para obtener el tama�o necesario de la variable dwBufLen
        if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == ERROR_BUFFER_OVERFLOW) {
            free(AdapterInfo);
            AdapterInfo = (IP_ADAPTER_INFO *) malloc(dwBufLen);
            // Validar existencia del adaptador
            if (AdapterInfo == NULL) {
              // printf("Error al localizar la memorioa necesaria para llamar al GetAdaptersinfo\n");
              free(macAddr);
              return macDireccion;
            }
        }
        // Validar por error en creaci�n de adaptadores
        if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == NO_ERROR) {
            // Contiene puntero a la informaci�n actual del adaptador
            PIP_ADAPTER_INFO pAdapter = AdapterInfo;
            do {
                if(pAdapter->DhcpEnabled) {
                    if (pAdapter->LeaseObtained != 0) {
                        // Obtener del punterp pAdapter-> AddressLength las partes de la direcci�n MAC
                        sprintf(  macAddr, "%02X:%02X:%02X:%02X:%02X:%02X",
                                pAdapter->Address[0], pAdapter->Address[1],
                                pAdapter->Address[2], pAdapter->Address[3],
                                pAdapter->Address[4], pAdapter->Address[5]);
                        // printf("IP: %s, Mac: %s\n", pAdapter->IpAddressList.IpAddress.String,  macAddr);
                    }
                }
                pAdapter = pAdapter->Next;
            } while(pAdapter);
        }
        free(AdapterInfo);
    #elif defined(linux) || defined(__linux)
        // Variables
        int fd;
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        // Socket para obtener la direccion MAC
        fd = socket(AF_INET, SOCK_DGRAM, 0);
        ifr.ifr_addr.sa_family = AF_INET;
        // Utilizo la interface Eth0
        strncpy(ifr.ifr_name , "eth0" , IFNAMSIZ-1);
        // Si existe Eth0
        if (0 == ioctl(fd, SIOCGIFHWADDR, &ifr)) {
            // Obtengo la direccion MAC
            macAddr = reinterpret_cast<char *>((unsigned char *) ifr.ifr_hwaddr.sa_data);
        }else{
            // Cambio de interface a Wlan0
            strncpy(ifr.ifr_name , "wlan0" , IFNAMSIZ-1);
            // Si existe la interface Wlan0
            if (0 == ioctl(fd, SIOCGIFHWADDR, &ifr)) {
                // Obtengo la direccion MAC
                macAddr = reinterpret_cast<char *>((unsigned char *) ifr.ifr_hwaddr.sa_data);
            } else {
                // Cambio de interface a Enp0s3
                strncpy(ifr.ifr_name , "enp0s3" , IFNAMSIZ-1);
                // Si existe la interface Enp0s3
                if (0 == ioctl(fd, SIOCGIFHWADDR, &ifr)) {
                    // Obtengo la direccion MAC
                    macAddr = reinterpret_cast<char *>((unsigned char *) ifr.ifr_hwaddr.sa_data);
                }
            }
        }
        close(fd);
    #endif
    // Transladar resultados MAC
    macDireccion = macAddr;
    // Regresar resultado
  	return macDireccion;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
char* recuperarDirIP() {
    // Variables
    char *addressIP = 0;
    // Si es WINDOWS
	#if defined(WIN32) || defined(_WIN32) || defined(_WIN64)
        // Variables
        char *auxIpDir;
        PIP_ADAPTER_INFO pAdapterInfo;
        pAdapterInfo = (IP_ADAPTER_INFO *) malloc(sizeof(IP_ADAPTER_INFO));
        ULONG buflen = sizeof(IP_ADAPTER_INFO);
        // Validaci�n de adaptador
        if(GetAdaptersInfo(pAdapterInfo, &buflen) == ERROR_BUFFER_OVERFLOW) {
            free(pAdapterInfo);
            pAdapterInfo = (IP_ADAPTER_INFO *) malloc(buflen);
        }
        // Validaci�n para obtener adaptador
        if(GetAdaptersInfo(pAdapterInfo, &buflen) == NO_ERROR) {
            // Variables
            PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
            // Recorrer adaptadores
            while (pAdapter) {
                // Validar que el adaptador este habilitado
                if(pAdapter->DhcpEnabled) {
                    // Validar dirección obtenida
                    if (pAdapter->LeaseObtained != 0) {
                        // Recupera dirección IP
                        auxIpDir = new char[sizeof(pAdapter->IpAddressList.IpAddress.String)];
                        strcpy(auxIpDir, string(pAdapter->IpAddressList.IpAddress.String).c_str());
                    }
                }
                //else {
                    // printf("\tHabilitado DHCP: No\n");
                //}
                //if(pAdapter->HaveWins) {
                //    printf("\tHay victorias: Si\n");
                //    printf("\t\tServidor primario de victorias: \t%s\n", pAdapter->PrimaryWinsServer.IpAddress.String);
                //    printf("\t\tServidor secundario de victorias: \t%s\n", pAdapter->SecondaryWinsServer.IpAddress.String);
                //} else {
                //    printf("\tHay victorias: No\n");
                //}
                pAdapter = pAdapter->Next;
            }
        }
        //else {
        //    // printf("Fallo la llamada al GetAdaptersInfo.\n");
        //}
        if (pAdapterInfo) {
            free(pAdapterInfo);
        }
        // Validar datos obtenidos
        if (strcmp(auxIpDir, "")) {
            // Transpasar valor IP obtenido
            addressIP = auxIpDir;
        }
    #elif defined(linux) || defined(__linux)
        // Variables
        int family, seEncontro;
        struct ifaddrs *ifaddr;
        // Definir variable
        addressIP = (char *) malloc(NI_MAXHOST);
        // Preparar arreglo de caracteres
        memset(addressIP, 0 , NI_MAXHOST);
        //**********************************************************
        // Validación al recuperar IP
        if (getifaddrs(&ifaddr) == -1) {
            //perror("Error al intentar obtener la direcci�n IP");
            sprintf(addressIP, "0.0.0.0");
            return addressIP;
        }
        // Recorrer resultados
        for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
            // La direcci�n de la interface es igual a null?
            if (ifa->ifa_addr == NULL) {
                continue;
            }
            // Recuperar Direcci�n IP
            seEncontro = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), addressIP, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            // Ver por la interface de Red Inalambrica: Wlan0
            if((strcmp(ifa->ifa_name, "wlan0") == 0) && (ifa->ifa_addr->sa_family==AF_INET)) {
                if (seEncontro != 0){
                    //perror("Error en la obtenci�n de la direcci�n por Wlan0\n");
                    sprintf(addressIP, "0.0.0.0");
                    return addressIP;
                }
                break;
            // Ver por la interface de Red Alambrica: Eth0
            } else if ((strcmp(ifa->ifa_name,"eth0")==0) && (ifa->ifa_addr->sa_family==AF_INET)) {
                if (seEncontro != 0){
                    //perror("Error en la obtenci�n de la direcci�n por Eth0\n");
                    sprintf(addressIP, "0.0.0.0");
                    return addressIP;
                }
                break;
            // Ver por la interface de Red Alambrica: enp0s3
            } else if ((strcmp(ifa->ifa_name,"enp0s3")==0) && (ifa->ifa_addr->sa_family==AF_INET)) {
                if (seEncontro != 0){
                    //perror("Error en la obtenci�n de la direcci�n por enp0s3\n");
                    sprintf(addressIP, "0.0.0.0");
                    return addressIP;
                }
                break;
            } else {
                sprintf(addressIP, "0.0.0.0");
            }
        }
        // Liberar objeto
        freeifaddrs(ifaddr);
    #endif
	// Regresar resultado
	return addressIP;
}

// Funciones adicionales
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
struct formato_msj processJsonToStruct(char *mensajeJson){
    // Variables
    struct formato_msj mensaje;
    // Objeto JSON
    Document document;
    document.Parse((const char *) mensajeJson);
    // Asignar valores a mensaje
    mensaje.clave = document["clave"].GetInt();
    mensaje.xIP.s_addr = inet_addr(document["xIP"].GetString());
    mensaje.zIP.s_addr = inet_addr(document["zIP"].GetString());
    sprintf(mensaje.xMAC, "%s", document["xMAC"].GetString());
    sprintf(mensaje.xHostN, "%s", document["xHostN"].GetString());
    sprintf(mensaje.xDatos, "%s", document["xDatos"].GetString());
    // Devolver mensaje
    return mensaje;
}

char* processStructToJson(struct formato_msj msjStruct) {
    // Mensaje JSON
	char *mensajeJson = (char *) malloc(TAM_JsonBuffer);
    // Variables
    StringBuffer strBff;
    Writer<StringBuffer> writer(strBff);
    // Pasar datos a String
    writer.StartObject();
    writer.Key("clave");
    writer.Uint(msjStruct.clave);
    writer.Key("xIP");
    writer.String(inet_ntoa(msjStruct.xIP));
    writer.Key("zIP");
    writer.String(inet_ntoa(msjStruct.zIP));
    writer.Key("xMAC");
    writer.String(msjStruct.xMAC);
    writer.Key("xHostN");
    writer.String(msjStruct.xHostN);
    writer.Key("xDatos");
    writer.String(msjStruct.xDatos);
    writer.EndObject();
    // Extraer cadena Json
    sprintf(mensajeJson, "%s\n", strBff.GetString());
	// Regresar resultado
	return mensajeJson;
}

void imprimirStruct(struct formato_msj msjStruct) {
    // printf("==================================================\n");
    printf("__________Clave: %d.\n", msjStruct.clave);
    printf("__Dirección xIP: %s.\n", inet_ntoa(msjStruct.xIP));
    printf("__Dirección zIP: %s.\n", inet_ntoa(msjStruct.zIP));
    printf("______Host name: %s.\n", msjStruct.xHostN);
    printf("_______Dir. MAC: %02x-%02x-%02x-%02x-%02x-%02x.\n", msjStruct.xMAC[0], msjStruct.xMAC[1], msjStruct.xMAC[2], msjStruct.xMAC[3], msjStruct.xMAC[4], msjStruct.xMAC[5]);
    printf("__________Datos: %s.\n", msjStruct.xDatos);
    //printf("==================================================\n");
}

char* construir_xDatos(int iClave, string mensajeDatos) {
    // Mensaje final
    char * msjResult;
    char msjFinDatos[TAM_Datos];
    // Preparar mensaje
    strcpy (msjFinDatos, "");

    // Construir direcci�n IP objetivo
    char aClave[(((sizeof iClave) * CHAR_BIT) + 2)/3 + 2];
    sprintf(aClave, "%d", iClave);
    strcat (msjFinDatos, aClave);
    strcat (msjFinDatos, "_");
    strcat (msjFinDatos, mensajeDatos.c_str());
    // Armar resultado
    msjResult = msjFinDatos;
    // Regresar resultados
    return msjResult;
}

char* processIntoToCharP(int iClave) {
    // Mensaje final
    char * msjResult;
    // Construir direcci�n IP objetivo
    char aClave[(((sizeof iClave) * CHAR_BIT) + 2)/3 + 2];
    sprintf(aClave, "%d", iClave);
    // Armar resultado
    msjResult = aClave;
    // Regresar resultados
    return msjResult;
}

