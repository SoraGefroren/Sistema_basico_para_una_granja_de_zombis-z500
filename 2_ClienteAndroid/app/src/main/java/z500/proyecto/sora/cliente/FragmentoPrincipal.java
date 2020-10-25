package z500.proyecto.sora.cliente;

import android.app.Fragment;
import android.app.FragmentTransaction;
import android.os.Bundle;
import android.util.SparseBooleanArray;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.AdapterView;
import android.widget.Button;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.Iterator;

public class FragmentoPrincipal extends Fragment {

    // Variables para vistas de la aplicacion
    private View vista;
    private ListView lvZombies;
    public static String configParaResults = "";
    public static ArrayList<String> listSelects = new ArrayList<String>();
    private ArrayList<String> mysSelects = new ArrayList<String>();
    private ArrayAdapter<String> adaptador;
    private String[] strDataZom;

    // Variables de estancia de elementos de la aplicacion
    private Button btnActualizar;
    private Button btnDescrubrir;
    private Button btnBarrer;

    // Crear Fragmento
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    // Inicializar vista
    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        // Inflar vista de un fragmento
        vista = inflater.inflate(R.layout.fragmento_principal, container, false);
        // Inicializar elementos de fragmento
        lvZombies = (ListView) vista.findViewById(R.id.lv_ListaZombies);
        btnActualizar = (Button) vista.findViewById(R.id.btn_Actualizar);
        btnDescrubrir = (Button) vista.findViewById(R.id.btn_Descrubrir);
        btnBarrer = (Button) vista.findViewById(R.id.btn_Barrer);

        // Generar adaptador para controlar los elementos en el ListView
        adaptador = new ArrayAdapter<String>(vista.getContext(), android.R.layout.simple_list_item_multiple_choice, new String[]{});
        // Asignar adaptador al ListView
        lvZombies.setAdapter(adaptador);
        lvZombies.setChoiceMode(ListView.CHOICE_MODE_SINGLE); // CHOICE_MODE_MULTIPLE
        // Aplicar cambios en ListView con zombies
        adaptador.notifyDataSetChanged();

        // Acciones del Boton Actualizar ***********************************************************
        btnActualizar.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // Bloquear lista de zombies
                Principal.candadoListaZombies.lock();
                // Limpiar lista de zombies
                if(Principal.itemsListaZombies.size() > 0){
                    Principal.itemsListaZombies.clear();
                }
                // Desbloquear lista de zombies
                Principal.candadoListaZombies.unlock();
                // Validar que exista una conexión activa
                if(Principal.cces.IsIniciado()){
                    // Crear y ejecutar tarea asincrona
                    (new Thread() {
                        public void run() {
                            // Enviar mensaje, solicitando PRESENTARSE
                            Principal.cces.SendMsjToServer("{" +
                                    " \"clave\":3, " +
                                    " \"xIP\":\"0.0.0.0\", " +
                                    " \"zIP\":\"0.0.0.0\", " +
                                    " \"xMAC\":\"000000A\", " +
                                    " \"xHostN\":\"\", " +
                                    " \"xDatos\":\"\" " +
                                    "}");
                        }
                    }).start();
                }
            }
        });

        // Crear tarea asincrona para actualizar lista de zombies
        (new Thread() {
            public void run() {
                while (true) {
                    try {
                        // Bloquear lista de zombies
                        Principal.candadoListaZombies.lock();
                        // Convertir lista de datos sobre zombies a un arreglo de string sobre zombies
                        strDataZom = Principal.itemsListaZombies.toArray(new String[Principal.itemsListaZombies.size()]);
                        // Desbloquear lista de zombies
                        Principal.candadoListaZombies.unlock();
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

        // Acciones del Boton Descubrir ************************************************************
        btnDescrubrir.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // Validar si existen zombies con los cuales trabajar
                if(lvZombies.getCount() > 0){
                    // Actualizar lista de items seleccionados
                    actualizarListaItemsSeleccionados();
                    // Bloquear lista de seleccionados
                    Principal.candadoListaSeleccts.lock();
                    // Validar si existen zombies seleccionados
                    if(Principal.itemsZombiesSelecteds.size() <= 0) {
                        // Emitir mensaje de error
                        Toast.makeText(getActivity(), "Ningun Zombie seleccionado", Toast.LENGTH_SHORT).show();
                        return;
                    }
                    // Preparar variable con datos de zombie
                    Iterator<String> it = Principal.itemsZombiesSelecteds.iterator();
                    listSelects.clear();
                    // Recorrer items zombies seleccionados
                    while(it.hasNext()){
                        // String de item seleccionado
                        listSelects.add(it.next());
                    }
                    // Desbloquear lista de seleccionados
                    Principal.candadoListaSeleccts.unlock();
                    // Indicar el tipo de resultados a mostrar
                    configParaResults = "DESCUBRIR";
                    // Preparar framento para su reemplazo
                    FragmentTransaction ft = getActivity().getFragmentManager().beginTransaction();
                    // Remplazar fragmento en actividad
                    ft.replace(R.id.main_fragment_Layout, Principal.fresults);
                    ft.addToBackStack(null);
                    ft.commit();
                } else {
                    // Emitir mensaje de error
                    Toast.makeText(getActivity(), "Ningun Zombie seleccionado", Toast.LENGTH_SHORT).show();
                }
            }
        });

        // Acciones del Boton Barrer ***************************************************************
        btnBarrer.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // Validar si existen zombies con los cuales trabajar
                if(lvZombies.getCount() > 0){
                    // Actualizar lista de items seleccionados
                    actualizarListaItemsSeleccionados();
                    // Bloquear lista de seleccionados
                    Principal.candadoListaSeleccts.lock();
                    // Validar si existen zombies seleccionados
                    if(Principal.itemsZombiesSelecteds.size() <= 0) {
                        // Emitir mensaje de error
                        Toast.makeText(getActivity(), "Ningun Zombie seleccionado", Toast.LENGTH_SHORT).show();
                        return;
                    }
                    // Preparar variable con datos de zombie
                    Iterator<String> it = Principal.itemsZombiesSelecteds.iterator();
                    listSelects.clear();
                    // Recorrer items zombies seleccionados
                    while(it.hasNext()){
                        // String de item seleccionado
                        listSelects.add(it.next());
                    }
                    // Desbloquear lista de seleccionados
                    Principal.candadoListaSeleccts.unlock();
                    // Indicar el tipo de resultados a mostrar
                    configParaResults = "BARRER";
                    // Preparar framento para su reemplazo
                    FragmentTransaction ft = getActivity().getFragmentManager().beginTransaction();
                    // Remplazar fragmento en actividad
                    ft.replace(R.id.main_fragment_Layout, Principal.fresults);
                    ft.addToBackStack(null);
                    ft.commit();
                    //*/
                }else{
                    // Emitir mensaje de error
                    Toast.makeText(getActivity(), "Ningun Zombie seleccionado", Toast.LENGTH_SHORT).show();
                }
            }
        });

        lvZombies.setOnItemClickListener(
            new AdapterView.OnItemClickListener() {
                public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                    // Recuperar datos de zombie, dada una posición
                    String zombSeleData = lvZombies.getItemAtPosition(position).toString();
                    // Validat si debe eliminar el item seleccionado
                    if(!mysSelects.contains(zombSeleData)){
                        // Agrega elemento para indicar su selección
                        mysSelects.add(zombSeleData);
                        // Seleccionar item
                        lvZombies.setItemChecked(position, true);
                    } else {
                        // Remover elemento para indicar que no esta seleccionado
                        mysSelects.remove(zombSeleData);
                        // Validar si tambien remover de lista de zombies seleccionados
                        if(Principal.itemsZombiesSelecteds.contains(zombSeleData)){
                            Principal.itemsZombiesSelecteds.remove(zombSeleData);
                        }
                        // Deseleccionar item
                        lvZombies.setItemChecked(position, false);
                    }
                }
            }
        );

        // Devolver vista de fragmento
        return vista;
    }

    // Crear actividad
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
                // Actualizar lista de items seleccionados
                actualizarListaItemsSeleccionados();
                // Validar que existan items para trabajar
                if (strDataZom.length > 0) {
                    // Generar adaptador para controlar los elementos en el ListView
                    adaptador = new ArrayAdapter<String>(vista.getContext(), android.R.layout.simple_list_item_multiple_choice, strDataZom);
                    // Asignar adaptador al ListView
                    lvZombies.setAdapter(adaptador);
                    lvZombies.setChoiceMode(ListView.CHOICE_MODE_SINGLE); //CHOICE_MODE_MULTIPLE
                    // Aplicar cambios en ListView con zombies
                    adaptador.notifyDataSetChanged();
                } else {
                    // Generar adaptador para controlar los elementos en el ListView
                    adaptador = new ArrayAdapter<String>(vista.getContext(), android.R.layout.simple_list_item_multiple_choice, new String[]{});
                    // Asignar adaptador al ListView
                    lvZombies.setAdapter(adaptador);
                    lvZombies.setChoiceMode(ListView.CHOICE_MODE_SINGLE); // CHOICE_MODE_MULTIPLE
                    // Aplicar cambios en ListView con zombies
                    adaptador.notifyDataSetChanged();
                }
                // Restaurar items seleccionados
                restaurarItemsSeleccionados();
            }
        });
    }

    // Actualizar lista de items seleccionados
    private void actualizarListaItemsSeleccionados(){
        // Bloquear lista de seleccionados
        Principal.candadoListaSeleccts.lock();
        // Si existen zombies, se reinicia lista de zombies seleccionados
        if(Principal.itemsZombiesSelecteds.size() > 0){
            Principal.itemsZombiesSelecteds.clear();
        }
        int numZoms = lvZombies.getCount();
        // Validar si existen zombies con los cuales trabajar
        if(numZoms > 0){
            // Se toman, "si existen", los items seleccionados en el ListView de zombies
            SparseBooleanArray seleccionados = lvZombies.getCheckedItemPositions();
            // Se recorren los items seleccionados en el ListView de zombies
            for(int i = 0;  i < seleccionados.size() ; i++){
                // Se recupera la llave o posición de los zombies seleccionados
                int llave = seleccionados.keyAt(i);
                // Valida si existe un item, dada una llave especifica
                if(seleccionados.get(llave)){
                    // Recuperar datos de zombie, dada una posición
                    String zombie = lvZombies.getItemAtPosition(llave).toString();
                    // Valida si se puede agregar a los items seleccionados
                    if(!Principal.itemsZombiesSelecteds.contains(zombie)){
                        Principal.itemsZombiesSelecteds.add(zombie);
                    }
                }
            }
        }
        // Desbloquear lista de seleccionados
        Principal.candadoListaSeleccts.unlock();
    }

    // Restaurar items seleccionados
    private void restaurarItemsSeleccionados(){
        // Bloquear lista de seleccionados
        Principal.candadoListaSeleccts.lock();
        // Validar si existen zombies seleccionados
        if(Principal.itemsZombiesSelecteds.size() > 0){
            // Preparar variable con datos de zombie
            Iterator<String> it = Principal.itemsZombiesSelecteds.iterator();
            int numZoms = lvZombies.getCount();
            // Validar si existen zombies con los cuales trabajar
            if(numZoms > 0){
                // Recorrer items zombies seleccionados
                while(it.hasNext()){
                    // String de item seleccionado
                    String zombSel = it.next();
                    // Recorrer items de lista
                    for (int iii = 0; iii < numZoms; iii++) {
                        // Recuperar datos de zombie, dada una posición
                        String zombData = lvZombies.getItemAtPosition(iii).toString();
                        // Validar si se debe o no seleccionar el item
                        if (zombSel.equals(zombData)) {
                            // Seleccionar item
                            lvZombies.setItemChecked(iii, true);
                        }
                    }
                }
            }
        }
        // Desbloquear lista de seleccionados
        Principal.candadoListaSeleccts.unlock();
    }
}
