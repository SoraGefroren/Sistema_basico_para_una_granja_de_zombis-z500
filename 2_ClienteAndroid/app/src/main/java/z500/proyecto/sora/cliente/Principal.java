package z500.proyecto.sora.cliente;

import android.os.Bundle;
import android.support.v4.app.FragmentActivity;
import android.os.AsyncTask;
import android.util.Log;
import java.util.ArrayList;
import java.util.List;
import org.json.JSONObject;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

public class Principal extends FragmentActivity {
    // Variables de Fragmentos para Activity
    public static FragmentoPrincipal fP = new FragmentoPrincipal(); // Fragmento Principal
    public static FragmentoResultados fresults = new FragmentoResultados(); // Fragmento Resultados
    // Variable para el mecanismo de comunicacion del dispositivo con el Servidor, los Maestros y los Zombies
    public static ComunicacionConElServidor cces = new ComunicacionConElServidor(); // Sobre Hilos
    // Variables con candados para controlar la concurrencia de los procesos
    public final static Lock candadoResultados = new ReentrantLock();
    public final static Lock candadoListaZombies = new ReentrantLock();
    public final static Lock candadoListaSeleccts = new ReentrantLock();
    // Variables con listas de los resultados
    public static List<String> itemsResultados = new ArrayList<String>();
    public static List<String> itemsListaZombies = new ArrayList<String>();
    public static List<String> itemsZombiesSelecteds = new ArrayList<String>();

    // Mecanimo que crea a la actividad
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // Iniciar actividad
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_principal);
        // Crear y ejecutar tarea asincrona
        (new Thread() {
            public void run() {
                // Conectarse o reconectarse con el servidor
                Principal.cces.ReconectarWithServer("192.168.100.18", 9096);
                // Validar inicio de conexión
                if(Principal.cces.IsIniciado()){
                    Log.e("W.Evento", "Conectado al servidor ");
                }
                // Control de mensajes recuperados
                while(true){
                    try{
                        // Recuperar mensaje JSON String del servidor
                        String msjJson = Principal.cces.RecvMsjOfServer();
                        // Validar mensaje recuperado
                        if (!msjJson.equals("")) {
                            // Convertir mensaje JSON a objeto JSON
                            JSONObject jobj = new JSONObject(msjJson);
                            // Recuperar INTRUCCIÓN enviada
                            int clave = jobj.getInt("clave");
                            // Validar si es o no una presentación
                            if((clave == 0) || (clave == 3)){
                                // Variable de control
                                boolean sonResults = false;
                                // Validar clave
                                if (clave == 0) {
                                    // Extraer data resultante
                                    String msjData = jobj.getString("xDatos");
                                    String[] partsData = msjData.split("_");
                                    // Validar acciones a tomar
                                    if ((partsData.length > 1) && partsData[0].trim().equals("3")) {
                                        // LISTA DE ZOMBIES
                                        // Extraer datos de presentación
                                        msjJson = "\"IPZombie\": \""  + jobj.getString("xIP")+"\", " +
                                                "\"IPMaestro\": \"" + jobj.getString("zIP")+"\"";
                                        // Bloquear lista de zombies
                                        Principal.candadoListaZombies.lock();
                                        // Validar si es o no necesario agregar el zombie a la lista
                                        if(!Principal.itemsListaZombies.contains(msjJson)){
                                            Principal.itemsListaZombies.add(msjJson);
                                        }
                                        // Desbloquear lista de zombies
                                        Principal.candadoListaZombies.unlock();
                                    } else {
                                        // RESULTADOS
                                        // Convertir el objeto JSON resultante, en un String JSON
                                        String msjResult = "\"IPZombie:\":\"" + jobj.getString("xIP") + "\", "+
                                                "\"IPMaestro\": \"" + jobj.getString("zIP") + "\", " +
                                                "\"Resultados\": \""  + jobj.getString("xDatos") + "\"";
                                        // Bloquear lista de Resultados
                                        Principal.candadoResultados.lock();
                                        // Validar si es o no necesario agregar un resultado a lista
                                        if(!Principal.itemsResultados.contains(msjResult)){
                                            Principal.itemsResultados.add(msjResult);
                                        }
                                        // Desbloquear lista de Resultados
                                        Principal.candadoResultados.unlock();
                                    }
                                } else {
                                    // LISTA DE ZOMBIES
                                    // Extraer datos de presentación
                                    msjJson = "\"IPZombie\": \""  + jobj.getString("xIP")+"\", " +
                                            "\"IPMaestro\": \"" + jobj.getString("zIP")+"\"";
                                    // Bloquear lista de zombies
                                    Principal.candadoListaZombies.lock();
                                    // Validar si es o no necesario agregar el zombie a la lista
                                    if(!Principal.itemsListaZombies.contains(msjJson)){
                                        Principal.itemsListaZombies.add(msjJson);
                                    }
                                    // Desbloquear lista de zombies
                                    Principal.candadoListaZombies.unlock();
                                }
                            }else{
                                // RESULTADOS
                                // Convertir el objeto JSON resultante, en un String JSON
                                String msjResult = "\"IPZombie:\":\"" + jobj.getString("xIP") + "\", "+
                                        "\"IPMaestro\": \"" + jobj.getString("zIP") + "\", " +
                                        "\"Resultados\": \""  + jobj.getString("xDatos") + "\"";
                                // Bloquear lista de Resultados
                                Principal.candadoResultados.lock();
                                // Validar si es o no necesario agregar un resultado a lista
                                if(!Principal.itemsResultados.contains(msjResult)){
                                    Principal.itemsResultados.add(msjResult);
                                }
                                // Desbloquear lista de Resultados
                                Principal.candadoResultados.unlock();
                            }
                        }
                    }catch (Exception e){
                        // Marcar error de recepción de mensaje
                        Log.e("W.Evento", "Error al recibir el mensaje: "+ e.toString());
                    }
                    //*********************
                    //*********************
                }
            }
        }).start();

        // Insertar fragmento principal en el espacio de fragmento de la actividad
        getFragmentManager().beginTransaction().add(R.id.main_fragment_Layout, fP).commit();
        getFragmentManager().beginTransaction().addToBackStack(null);
    }

}
