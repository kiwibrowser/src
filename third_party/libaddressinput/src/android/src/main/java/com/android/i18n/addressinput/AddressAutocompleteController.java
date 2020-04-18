/*
 * Copyright (C) 2016 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.i18n.addressinput;

import android.content.Context;
import android.os.AsyncTask;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AutoCompleteTextView;
import android.widget.BaseAdapter;
import android.widget.Filter;
import android.widget.Filterable;
import android.widget.LinearLayout;
import android.widget.TextView;
import com.google.common.annotations.VisibleForTesting;
import com.google.common.util.concurrent.FutureCallback;
import com.google.i18n.addressinput.common.AddressAutocompleteApi;
import com.google.i18n.addressinput.common.AddressAutocompletePrediction;
import com.google.i18n.addressinput.common.AddressData;
import com.google.i18n.addressinput.common.OnAddressSelectedListener;
import java.util.ArrayList;
import java.util.List;

/** Controller for address autocomplete results. */
class AddressAutocompleteController {

  private static final String TAG = "AddressAutocompleteCtrl";

  private AddressAutocompleteApi autocompleteApi;
  private PlaceDetailsApi placeDetailsApi;
  private AddressAdapter adapter;
  private OnAddressSelectedListener listener;

  private TextWatcher textChangedListener =
      new TextWatcher() {
        @Override
        public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

        @Override
        public void onTextChanged(CharSequence s, int start, int before, int after) {
          getAddressPredictions(s.toString());
        }

        @Override
        public void afterTextChanged(Editable s) {}
      };

  private AdapterView.OnItemClickListener onItemClickListener =
      new AdapterView.OnItemClickListener() {
        @Override
        public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
          if (listener != null) {
            AddressAutocompletePrediction prediction =
                (AddressAutocompletePrediction)
                    adapter.getItem(position).getAutocompletePrediction();

            (new AsyncTask<AddressAutocompletePrediction, Void, AddressData>() {
                  @Override
                  protected AddressData doInBackground(
                      AddressAutocompletePrediction... predictions) {
                    try {
                      return placeDetailsApi.getAddressData(predictions[0]).get();
                    } catch (Exception e) {
                      cancel(true);
                      Log.i(TAG, "Error getting place details: ", e);
                      return null;
                    }
                  }

                  @Override
                  protected void onPostExecute(AddressData addressData) {
                    Log.e(TAG, "AddressData: " + addressData.toString());
                    listener.onAddressSelected(addressData);
                  }
                })
                .execute(prediction);
          } else {
            Log.i(TAG, "No onAddressSelected listener.");
          }
        }
      };

  AddressAutocompleteController(
      Context context, AddressAutocompleteApi autocompleteApi, PlaceDetailsApi placeDetailsApi) {
    this.placeDetailsApi = placeDetailsApi;
    this.autocompleteApi = autocompleteApi;

    adapter = new AddressAdapter(context);
  }

  AddressAutocompleteController setView(AutoCompleteTextView textView) {
    textView.setAdapter(adapter);
    textView.setOnItemClickListener(onItemClickListener);
    textView.addTextChangedListener(textChangedListener);

    return this;
  }

  AddressAutocompleteController setOnAddressSelectedListener(OnAddressSelectedListener listener) {
    this.listener = listener;
    return this;
  }

  void getAddressPredictions(final String query) {
    if (!autocompleteApi.isConfiguredCorrectly()) {
      return;
    }

    autocompleteApi.getAutocompletePredictions(
        query,
        new FutureCallback<List<? extends AddressAutocompletePrediction>>() {
          @Override
          public void onSuccess(List<? extends AddressAutocompletePrediction> predictions) {
            List<AddressPrediction> wrappedPredictions = new ArrayList<>();

            for (AddressAutocompletePrediction prediction : predictions) {
              wrappedPredictions.add(new AddressPrediction(query, prediction));
            }

            adapter.refresh(wrappedPredictions);
          }

          @Override
          public void onFailure(Throwable error) {
            Log.i(TAG, "Error getting autocomplete predictions: ", error);
          }
        });
  }

  @VisibleForTesting
  static class AddressPrediction {
    private String prefix;
    private AddressAutocompletePrediction autocompletePrediction;

    AddressPrediction(String prefix, AddressAutocompletePrediction prediction) {
      this.prefix = prefix;
      this.autocompletePrediction = prediction;
    }

    String getPrefix() {
      return prefix;
    };

    AddressAutocompletePrediction getAutocompletePrediction() {
      return autocompletePrediction;
    };

    @Override
    public final String toString() {
      return getPrefix();
    }
  }

  // The main purpose of this custom adapter is the custom getView function.
  // This adapter extends BaseAdapter instead of ArrayAdapter because ArrayAdapter has a filtering
  // bug that is triggered by the AutoCompleteTextView (see
  // http://www.jaysoyer.com/2014/07/filtering-problems-arrayadapter/).
  @VisibleForTesting
  static class AddressAdapter extends BaseAdapter implements Filterable {
    private Context context;

    private List<AddressPrediction> predictions;

    AddressAdapter(Context context) {
      this.context = context;
      this.predictions = new ArrayList<AddressPrediction>();
    }

    public AddressAdapter refresh(List<AddressPrediction> newPredictions) {
      predictions = newPredictions;
      notifyDataSetChanged();

      return this;
    }

    @Override
    public int getCount() {
      return predictions.size();
    }

    @Override
    public AddressPrediction getItem(int position) {
      return predictions.get(position);
    }

    @Override
    public long getItemId(int position) {
      return position;
    }

    // No-op filter.
    // Results from the PlaceAutocomplete API don't need to be filtered any further.
    @Override
    public Filter getFilter() {
      return new Filter() {
        @Override
        public Filter.FilterResults performFiltering(CharSequence constraint) {
          Filter.FilterResults results = new Filter.FilterResults();
          results.count = predictions.size();
          results.values = predictions;

          return results;
        }

        @Override
        public void publishResults(CharSequence constraint, Filter.FilterResults results) {}
      };
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
      LayoutInflater inflater =
          (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
      LinearLayout view =
          convertView instanceof LinearLayout
              ? (LinearLayout) convertView
              : (LinearLayout)
                  inflater.inflate(R.layout.address_autocomplete_dropdown_item, parent, false);
      AddressPrediction prediction = predictions.get(position);

      TextView line1 = (TextView) view.findViewById(R.id.line_1);
      if (line1 != null) {
        line1.setText(prediction.getAutocompletePrediction().getPrimaryText());
      }

      TextView line2 = (TextView) view.findViewById(R.id.line_2);
      line2.setText(prediction.getAutocompletePrediction().getSecondaryText());

      return view;
    }
  }
}
