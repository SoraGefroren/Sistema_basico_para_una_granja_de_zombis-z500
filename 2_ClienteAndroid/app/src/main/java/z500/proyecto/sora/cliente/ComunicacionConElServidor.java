package z500.proyecto.sora.cliente;

import android.util.Log;
import java.io.IOException;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.InetAddress;
import java.io.PrintWriter;
import java.net.Socket;

public class ComunicacionConElServidor {
    // Variables de conexion
    private Socket socket;
    private PrintWriter escritor;
    private BufferedReader lector;
    // Variable para saber el estado de la clase
    private boolean iniciado = false;
    // Constructor
    public ComunicacionConElServidor(){
        iniciado = false;
    }
    // Conectar o reconectar aplicación con el servidor
    public void ReconectarWithServer(String SERV_IP, int PORT){
        try{
            // Crear instancia de de la conexión con el servidor
            InetAddress serverAddr = InetAddress.getByName(SERV_IP);
            socket= new Socket (serverAddr,PORT);
            // Crear instancia para leer y escribir datos con el servidor
            lector = new BufferedReader (new InputStreamReader(socket.getInputStream()));
            escritor = new PrintWriter (new OutputStreamWriter(socket.getOutputStream()),true);
            // Indica que se ha logrado la conexión
            iniciado = true;
            Log.e("W.Evento", "Local________________________________________________\n" + socket.getLocalAddress().toString());
            Log.e("W.Evento", "Remoto________________________________________________\n" + socket.getRemoteSocketAddress().toString());
        }catch (Exception e){
            Log.e("W.Evento", "-------------------------------------------------");
            Log.e("W.Evento", "CCES_Error al conectar con el servidor: " + e.toString());
            Log.e("W.Evento", "-------------------------------------------------");
            // Indica que no se ha logrado la conexión
            iniciado = false;
        }
    }
    // Función para enviar mensaje al servidor
    public void SendMsjToServer(String msj_toServ) {
        try{
            // Preparar medio para escribir mensaje STRING en el servidor
            escritor = new PrintWriter (new OutputStreamWriter(socket.getOutputStream()),true);
            // Enviar mensaje en el servidor
            escritor.write(msj_toServ.toCharArray());
            escritor.flush();
        }catch (Exception e){
            // Marcar error de recepción de mensaje
            Log.e("W.Evento", "Error al enviar el mensaje: "+ e.toString());
        }
    }
    // Función que recive los mensajes del servidor
    public String RecvMsjOfServer(){
        // Validar que este iniciada la conexión
        if(iniciado){
            try {
                // Prepara espacio para recibir mensaje (Tamaño de Buffer de 7500)
                char[] msj_ofServer = new char[7500];
                // Preparar medio para leer mensaje del servidor
                lector = new BufferedReader (new InputStreamReader(socket.getInputStream()));
                Log.e("W.Evento", "Lector - READY?" + lector.ready()+"\n");
                // Leer mensaje del servidor
                 lector.read(msj_ofServer);
                // Preparar medio tratar mensaje
                String msj_ofServ = new String("");
                // Variables de apoyo
                int iax = 0;
                char cax = ' ';
                // Recorrer mensaje recuperado del servidor
                while( (iax < 7500) && ( (cax != '\n') && (cax != '}') ) ){
                    // Concatenar mensaje para convertirlo en un String
                    cax = msj_ofServer[iax];
                    msj_ofServ = msj_ofServ + cax;
                    iax++;
                }
                // Validar si el mensaje no es nulo
                if(msj_ofServ != null){
                    // Devolver mensaje
                    return msj_ofServ;
                }else{
                    // Devolver string vacio
                    return "";
                }
            } catch (Exception e) {
                // Marcar error de recepción de mensaje
                Log.e("W.Evento", "Error durante la lectura del mensaje: "+ e.toString());
                // Devolver string vacio
                return "";
            }
        }else{
            // Devolver string vacio
            return "";
        }
    }

    // Cerrar conexión con el servidor
    public boolean ClosedConexionWithServer(){
        try {
            // Cerrar conexión
            socket.close();
            iniciado = false;
            return true;
        } catch (Exception e) {
            return false;
        }
    }

    // Indicar si la conexión esta o no iniciada
    public boolean IsIniciado(){
        return iniciado;
    }
}
