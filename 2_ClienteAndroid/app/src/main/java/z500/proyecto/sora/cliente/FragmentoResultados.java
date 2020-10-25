package z500.proyecto.sora.cliente;

import android.app.Fragment;
import android.app.FragmentTransaction;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.LayoutInflater;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Iterator;

public class FragmentoResultados extends Fragment {

    // Variables para elementos del fragmento
    private View vista;
    private Button btnMenu;
    private TextView txtTitulo;
    private ListView lvResultados;
    public ArrayAdapter<String> adaptador;
    private String[] strDataRs;

    private String configParaRs = "";
    private ArrayList<String> listSelects = new ArrayList<String>();

    // Crear actividad
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    // Crear vista de actividad
    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        // Asignar valores de configuración y búsqueda
        configParaRs = FragmentoPrincipal.configParaResults;
        listSelects = FragmentoPrincipal.listSelects;
        // Asignar fragmento consola
        vista = inflater.inflate(R.layout.fragmento_resultados, container, false);
        // Asignar botones y lista de resultados
        btnMenu = (Button) vista.findViewById(R.id.btn_VolverMainMenu);
        txtTitulo = (TextView) vista.findViewById(R.id.txt_titulo);
        lvResultados = (ListView) vista.findViewById(R.id.lv_Resultados);

        // Generar adaptador para controlar los elementos en el ListView
        adaptador = new ArrayAdapter<String>(vista.getContext(), android.R.layout.simple_list_item_multiple_choice, new String[]{});
        // Asignar adaptador al ListView
        lvResultados.setAdapter(adaptador);
        lvResultados.setChoiceMode(ListView.CHOICE_MODE_NONE);
        // Aplicar cambios en ListView de resultados
        adaptador.notifyDataSetChanged();

        // Accion del ListView *******************************************************************
        lvResultados.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                // Recuperar data del item seleccionado
                String dataResult = (String) (lvResultados.getItemAtPosition(position)).toString();
                // Emitir mensaje con data del item seleccionado
                Toast.makeText(getActivity(), "Informacion:\n"+dataResult, Toast.LENGTH_SHORT).show();
            }
        });

        // Bloquear lista de resultados
        Principal.candadoResultados.lock();
        // Limpiar lista de resultados
        if(Principal.itemsResultados.size() > 0){
            Principal.itemsResultados.clear();
        }
        // Desbloquear lista de resultados
        Principal.candadoResultados.unlock();
        // Validar que exista una conexión activa
        if(Principal.cces.IsIniciado()){
            // Crear y ejecutar tarea asincrona
            (new Thread() {
                public void run() {
                    // Elegir el tipo de petición
                    if (configParaRs.equals("DESCUBRIR")) {
                        // Asignar titulo
                        txtTitulo.setText(configParaRs);
                        // Preparar variable con datos de zombie
                        Iterator<String> it = listSelects.iterator();
                        // Recorrer items zombies seleccionados
                        while(it.hasNext()){
                            // Preparar valor de dirección de Zombie y Maestros
                            String ipZombie = "";
                            String ipMaster = "";
                            // Extraer dirección IP del Zombie y Maestro
                            try {
                                JSONObject jobj = new JSONObject("{" + it.next() + "}");
                                ipZombie = jobj.getString("IPZombie");
                                ipMaster = jobj.getString("IPMaestro");
                            } catch (Exception ex) {
                                Log.e("W.Evento", "Error extraer data JSON para solicituar acción - "+ ex.toString());
                                ipZombie = "0.0.0.0";
                                ipMaster = "0.0.0.0";
                            }
                            // Enviar mensaje, solicitando DESCUBRIR
                            Principal.cces.SendMsjToServer("{" +
                                    " \"clave\":4, " +
                                    " \"xIP\":\""+ipZombie+"\", " +
                                    " \"zIP\":\""+ipMaster+"\", " +
                                    " \"xMAC\":\"000000A\", " +
                                    " \"xHostN\":\"\", " +
                                    " \"xDatos\":\"\" " +
                                    "}");
                        }
                    } else if (configParaRs.equals("BARRER")) {
                        // Asignar titulo
                        txtTitulo.setText(configParaRs);
                        // Preparar variable con datos de zombie
                        Iterator<String> it = listSelects.iterator();
                        // Recorrer items zombies seleccionados
                        while(it.hasNext()){
                            // Preparar valor de dirección de Zombie y Maestros
                            String ipZombie = "";
                            String ipMaster = "";
                            // Extraer dirección IP del Zombie y Maestro
                            try {
                                JSONObject jobj = new JSONObject("{" + it.next() + "}");
                                ipZombie = jobj.getString("IPZombie");
                                ipMaster = jobj.getString("IPMaestro");
                            } catch (Exception ex) {
                                Log.e("W.Evento", "Error extraer data JSON para solicituar acción - "+ ex.toString());
                                ipZombie = "0.0.0.0";
                                ipMaster = "0.0.0.0";
                            }
                            // Enviar mensaje, solicitando BARRER
                            Principal.cces.SendMsjToServer("{" +
                                    " \"clave\":5, " +
                                    " \"xIP\":\""+ipZombie+"\", " +
                                    " \"zIP\":\""+ipMaster+"\", " +
                                    " \"xMAC\":\"000000A\", " +
                                    " \"xHostN\":\"\", " +
                                    " \"xDatos\":\"\" " +
                                    "}");
                        }
                    }
                }
            }).start();
        }

        // Accion del Boton Menu *******************************************************************
        btnMenu.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // Reemplazar fragmento de la actividad por otro fragmento
                FragmentTransaction ft = getActivity().getFragmentManager().beginTransaction();
                ft.replace(R.id.main_fragment_Layout, Principal.fP);
                ft.addToBackStack(null);
                ft.commit();
            }
        });

        // Crear tarea asincrona para actualizar lista de resultados
        (new Thread() {
            public void run() {
                while (true) {
                    try {
                        // Bloquear lista de resultados
                        Principal.candadoResultados.lock();
                        // Convertir lista de datos sobre resultados a un arreglo de string sobre resultados
                        strDataRs = Principal.itemsResultados.toArray(new String[Principal.itemsResultados.size()]);
                        // Desbloquear lista de resultados
                        Principal.candadoResultados.unlock();
                        // Actualizar ListView
                        actualizarListView();
                        // Dormir tarea
                        Thread.sleep(2500);
                    } catch (InterruptedException e) {
                        // Mostrar error
                        e.printStackTrace();
                    }
                }
            }
        }).start();

        // Regresar vista creada
        return vista;
    }

    // Al estar creando la actividad
    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
    }

    // Actualizar ListView
    private void actualizarListView() {
        // Correr hilo en actividad
        getActivity().runOnUiThread(new Runnable() {
            @Override
            public void run() {
                // Validar que existan items para trabajar
                if (strDataRs.length > 0) {
                    // Generar adaptador para controlar los elementos en el ListView
                    adaptador = new ArrayAdapter<String>(vista.getContext(), android.R.layout.simple_list_item_multiple_choice, strDataRs);
                    // Asignar adaptador al ListView
                    lvResultados.setAdapter(adaptador);
                    lvResultados.setChoiceMode(ListView.CHOICE_MODE_NONE);
                    // Aplicar cambios en ListView con zombies
                    adaptador.notifyDataSetChanged();
                } else {
                    // Generar adaptador para controlar los elementos en el ListView
                    adaptador = new ArrayAdapter<String>(vista.getContext(), android.R.layout.simple_list_item_multiple_choice, new String[]{});
                    // Asignar adaptador al ListView
                    lvResultados.setAdapter(adaptador);
                    lvResultados.setChoiceMode(ListView.CHOICE_MODE_NONE);
                    // Aplicar cambios en ListView con zombies
                    adaptador.notifyDataSetChanged();
                }
            }
        });
    }



}
