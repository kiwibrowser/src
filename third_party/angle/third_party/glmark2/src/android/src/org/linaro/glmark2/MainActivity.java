/*
 * Copyright © 2012 Linaro Limited
 *
 * This file is part of the glmark2 OpenGL (ES) 2.0 benchmark.
 *
 * glmark2 is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * glmark2 is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * glmark2.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *  Alexandros Frantzis
 */
package org.linaro.glmark2;

import java.util.ArrayList;

import android.os.Bundle;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnDismissListener;
import android.content.Intent;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.widget.BaseAdapter;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Button;
import android.widget.EditText;
import android.widget.CheckBox;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.inputmethod.EditorInfo;
import android.util.Log;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.AdapterView.OnItemLongClickListener;
import android.widget.AdapterView;
import android.widget.TextView.OnEditorActionListener;

public class MainActivity extends Activity {
    public static final int DIALOG_ERROR_ID = 0;
    public static final int DIALOG_BENCHMARK_ACTIONS_ID = 1;
    public static final int DIALOG_SAVE_LIST_ID = 2;
    public static final int DIALOG_LOAD_LIST_ID = 3;
    public static final int DIALOG_DELETE_LIST_ID = 4;

    public static final int ACTIVITY_GLMARK2_REQUEST_CODE = 1;
    public static final int ACTIVITY_EDITOR_REQUEST_CODE = 2;

    /**
     * The supported benchmark item actions.
     */
    public enum BenchmarkItemAction {
        EDIT, DELETE, CLONE, MOVEUP, MOVEDOWN
    }

    BaseAdapter adapter;
    SceneInfo[] sceneInfoList;
    BenchmarkListManager benchmarkListManager;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        ArrayList<String> savedBenchmarks = null;

        if (savedInstanceState != null)
            savedBenchmarks = savedInstanceState.getStringArrayList("benchmarks");

        init(savedBenchmarks);
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putStringArrayList("benchmarks", benchmarkListManager.getBenchmarkList());
    }

    @Override
    protected Dialog onCreateDialog(int id, Bundle bundle) {
        final CharSequence[] benchmarkActions = {"Delete", "Clone", "Move Up", "Move Down"};
        final BenchmarkItemAction[] benchmarkActionsId = {
                BenchmarkItemAction.DELETE, BenchmarkItemAction.CLONE,
                BenchmarkItemAction.MOVEUP, BenchmarkItemAction.MOVEDOWN
        };
        final int finalId = id;

        Dialog dialog;

        switch (id) {
            case DIALOG_ERROR_ID:
                {
                AlertDialog.Builder builder = new AlertDialog.Builder(this);
                builder.setMessage(bundle.getString("message") + ": " +
                                   bundle.getString("detail"));
                builder.setCancelable(false);
                builder.setPositiveButton("OK", null);
                dialog = builder.create();
                }
                break;

            case DIALOG_BENCHMARK_ACTIONS_ID:
                {
                final int benchmarkPos = bundle.getInt("benchmark-pos");
                AlertDialog.Builder builder = new AlertDialog.Builder(this);
                builder.setTitle("Pick an action");
                builder.setItems(benchmarkActions, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int item) {
                        doBenchmarkItemAction(benchmarkPos, benchmarkActionsId[item], null);
                        dismissDialog(DIALOG_BENCHMARK_ACTIONS_ID);
                    }
                });
                dialog = builder.create();
                }
                break;

            case DIALOG_SAVE_LIST_ID:
                {
                AlertDialog.Builder builder = new AlertDialog.Builder(this);
                View layout = getLayoutInflater().inflate(R.layout.save_dialog, null);
                final EditText input = (EditText) layout.findViewById(R.id.listName);
                final CheckBox checkBox = (CheckBox) layout.findViewById(R.id.external);

                input.setOnEditorActionListener(new OnEditorActionListener() {
                    public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                        if (actionId == EditorInfo.IME_ACTION_DONE ||
                            (event != null && event.getKeyCode() == KeyEvent.KEYCODE_ENTER &&
                             event.getAction() == KeyEvent.ACTION_UP))
                        {
                            String listName = v.getText().toString();
                            try {
                                benchmarkListManager.saveBenchmarkList(listName,
                                                                       checkBox.isChecked());
                            }
                            catch (Exception ex) {
                                Bundle bundle = new Bundle();
                                bundle.putString("message", "Cannot save list to file " + listName);
                                bundle.putString("detail", ex.getMessage());
                                showDialog(DIALOG_ERROR_ID, bundle);
                            }
                            dismissDialog(DIALOG_SAVE_LIST_ID);
                        }
                        return true;
                    }
                });

                builder.setTitle("Save list as");
                builder.setView(layout);

                dialog = builder.create();
                dialog.getWindow().setSoftInputMode(
                        WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN |
                        WindowManager.LayoutParams.SOFT_INPUT_STATE_VISIBLE
                        );
                }
                break;

            case DIALOG_LOAD_LIST_ID:
            case DIALOG_DELETE_LIST_ID:
                {
                AlertDialog.Builder builder = new AlertDialog.Builder(this);
                if (id == DIALOG_LOAD_LIST_ID)
                    builder.setTitle("Load list");
                else
                    builder.setTitle("Delete list");
                final String[] savedLists = benchmarkListManager.getSavedLists();

                builder.setItems(savedLists, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int index) {
                        String desc = savedLists[index];
                        String filename = "";
                        boolean external = false;

                        if (desc.startsWith("internal/")) {
                            filename = desc.replace("internal/", "");
                            external = false;
                        }
                        else if (desc.startsWith("external/")) {
                            filename = desc.replace("external/", "");
                            external = true;
                        }
                            
                        try {
                            if (finalId == DIALOG_LOAD_LIST_ID) {
                                benchmarkListManager.loadBenchmarkList(filename, external);
                                adapter.notifyDataSetChanged();
                            }
                            else {
                                benchmarkListManager.deleteBenchmarkList(filename, external);
                            }

                        }
                        catch (Exception ex) {
                            Bundle bundle = new Bundle();
                            if (finalId == DIALOG_LOAD_LIST_ID)
                                bundle.putString("message", "Cannot load list " + desc);
                            else
                                bundle.putString("message", "Cannot delete list " + desc);
                            bundle.putString("detail", ex.getMessage());
                            showDialog(DIALOG_ERROR_ID, bundle);
                        }
                        dismissDialog(finalId);
                    }
                });

                dialog = builder.create();
                }
                break;

            default:
                dialog = null;
                break;
        }

        if (dialog != null) {
            dialog.setOnDismissListener(new OnDismissListener() {
                public void onDismiss(DialogInterface dialog) {
                    removeDialog(finalId);
                }
            });
        }

        return dialog;
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.main_options_menu, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        boolean ret = true;

        switch (item.getItemId()) {
            case R.id.save_benchmark_list:
                showDialog(DIALOG_SAVE_LIST_ID);
                ret = true;
                break;
            case R.id.load_benchmark_list:
                showDialog(DIALOG_LOAD_LIST_ID);
                ret = true;
                break;
            case R.id.delete_benchmark_list:
                showDialog(DIALOG_DELETE_LIST_ID);
                ret = true;
                break;
            case R.id.settings:
                startActivity(new Intent(MainActivity.this, MainPreferencesActivity.class));
                ret = true;
                break;
            case R.id.results:
                startActivity(new Intent(MainActivity.this, ResultsActivity.class));
                ret = true;
                break;
            case R.id.about:
                startActivity(new Intent(MainActivity.this, AboutActivity.class));
                ret = true;
                break;
            default:
                ret = super.onOptionsItemSelected(item);
                break;
        }

        return ret;
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == ACTIVITY_GLMARK2_REQUEST_CODE)
        {
            startActivity(new Intent(this, ResultsActivity.class));
            return;
        }
        else if (requestCode == ACTIVITY_EDITOR_REQUEST_CODE &&
                 resultCode == RESULT_OK)
        {
            String benchmarkText = data.getStringExtra("benchmark-text");
            int benchmarkPos = data.getIntExtra("benchmark-pos", 0);
            doBenchmarkItemAction(benchmarkPos, BenchmarkItemAction.EDIT, benchmarkText);
        }
    }

    /**
     * Initialize the activity.
     *
     * @param savedBenchmarks a list of benchmarks to load the list with (or null)
     */
    private void init(ArrayList<String> savedBenchmarks)
    {
        /* Initialize benchmark list manager */
        benchmarkListManager = new BenchmarkListManager(this, savedBenchmarks);
        final ArrayList<String> benchmarks = benchmarkListManager.getBenchmarkList();

        /* Get Scene information */
        sceneInfoList = Glmark2Native.getSceneInfo(getAssets());

        /* Set up the run button */
        Button button = (Button) findViewById(R.id.runButton);
        button.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(getBaseContext());
                Intent intent = new Intent(MainActivity.this, Glmark2Activity.class);
                String args = "";
                for (int i = 0; i < benchmarks.size() - 1; i++)
                    args += "-b \"" + benchmarks.get(i) + "\" ";
                if (prefs.getBoolean("run_forever", false))
                    args += "--run-forever ";
                if (prefs.getBoolean("log_debug", false))
                    args += "--debug ";
                if (!args.isEmpty())
                    intent.putExtra("args", args);
                
                if (prefs.getBoolean("show_results", true))
                    startActivityForResult(intent, ACTIVITY_GLMARK2_REQUEST_CODE);
                else
                    startActivity(intent);
            }
        });

        /* Set up the benchmark list view */
        ListView lv = (ListView) findViewById(R.id.benchmarkListView);
        adapter = new BenchmarkAdapter(this, R.layout.list_item, benchmarks);
        lv.setAdapter(adapter);

        lv.setOnItemClickListener(new OnItemClickListener() {
            public void onItemClick(AdapterView<?> parentView, View childView, int position, long id) {
                Intent intent = new Intent(MainActivity.this, EditorActivity.class);
                String t = benchmarks.get(position);
                if (position == benchmarks.size() - 1)
                    t = "";
                intent.putExtra("benchmark-text", t);
                intent.putExtra("benchmark-pos", position);
                intent.putExtra("scene-info", sceneInfoList);
                startActivityForResult(intent, ACTIVITY_EDITOR_REQUEST_CODE);
            }
        });

        lv.setOnItemLongClickListener(new OnItemLongClickListener() {
            public boolean onItemLongClick(AdapterView<?> parentView, View childView, int position, long id) {
                if (position < benchmarks.size() - 1) {
                    Bundle bundle = new Bundle();
                    bundle.putInt("benchmark-pos", position);
                    showDialog(DIALOG_BENCHMARK_ACTIONS_ID, bundle);
                }
                return true;
            }
        });

    }

    /**
     * Perform an action on an listview benchmark item.
     *
     * @param position the position of the item in the listview
     * @param action the action to perform
     * @param data extra data needed by some actions
     */
    private void doBenchmarkItemAction(int position, BenchmarkItemAction action, String data)
    {
        int scrollPosition = position;
        final ArrayList<String> benchmarks = benchmarkListManager.getBenchmarkList();

        switch(action) {
            case EDIT:
                if (position == benchmarks.size() - 1) {
                    benchmarks.add(position, data);
                    scrollPosition = position + 1;
                }
                else {
                    benchmarks.set(position, data);
                }
                break;
            case DELETE:
                benchmarks.remove(position);
                break;
            case CLONE:
                {
                    String s = benchmarks.get(position);
                    benchmarks.add(position, s);
                    scrollPosition = position + 1;
                }
                break;
            case MOVEUP:
                if (position > 0) {
                    String up = benchmarks.get(position - 1);
                    String s = benchmarks.get(position);
                    benchmarks.set(position - 1, s);
                    benchmarks.set(position, up);
                    scrollPosition = position - 1;
                }
                break;
            case MOVEDOWN:
                if (position < benchmarks.size() - 2) {
                    String down = benchmarks.get(position + 1);
                    String s = benchmarks.get(position);
                    benchmarks.set(position + 1, s);
                    benchmarks.set(position, down);
                    scrollPosition = position + 1;
                }
                break;
            default:
                break;
        }


        adapter.notifyDataSetChanged();

        /* Scroll the list view so that the item of interest remains visible */
        final int finalScrollPosition = scrollPosition;
        final ListView lv = (ListView) findViewById(R.id.benchmarkListView);
        lv.post(new Runnable() {
            @Override
            public void run() {
                lv.smoothScrollToPosition(finalScrollPosition);
            }
        });
    }

    /**
     * A ListView adapter that creates item views from benchmark strings.
     */
    private class BenchmarkAdapter extends ArrayAdapter<String> {
        private ArrayList<String> items;

        public BenchmarkAdapter(Context context, int textViewResourceId, ArrayList<String> items) {
            super(context, textViewResourceId, items);
            this.items = items;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            /* Get the view/widget to use */
            View v = convertView;
            if (v == null) {
                LayoutInflater vi = (LayoutInflater)getSystemService(Context.LAYOUT_INFLATER_SERVICE);
                v = vi.inflate(R.layout.list_item, null);
            }

            /* Split the benchmark into its scene name and its options */
            String benchmark = items.get(position);
            String[] ba = benchmark.split(":", 2);

            if (ba != null) {
                TextView title = (TextView) v.findViewById(R.id.title);
                TextView summary = (TextView) v.findViewById(R.id.summary);
                title.setText("");
                summary.setText("");

                if (title != null && ba.length > 0)
                    title.setText(ba[0]);
                if (summary != null && ba.length > 1)
                    summary.setText(ba[1]);
            }
            return v;
        }
    }

    static {
        System.loadLibrary("glmark2-android");
    }
}
