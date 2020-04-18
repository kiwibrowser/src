// Copyright 2018 The Feed Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.google.android.libraries.feed.piet;

import static com.google.android.libraries.feed.common.Validators.checkState;

import android.content.Context;
import android.support.annotation.VisibleForTesting;
import android.view.Gravity;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;
import com.google.android.libraries.feed.piet.AdapterFactory.AdapterKeySupplier;
import com.google.android.libraries.feed.piet.AdapterFactory.SingletonKeySupplier;
import com.google.search.now.ui.piet.ElementsProto.Element;
import com.google.search.now.ui.piet.ElementsProto.ElementList;
import com.google.search.now.ui.piet.ElementsProto.GridCell;
import com.google.search.now.ui.piet.ElementsProto.GridCellWidth;
import com.google.search.now.ui.piet.ElementsProto.GridRow;
import com.google.search.now.ui.piet.StylesProto.StyleIdsStack;

/** An {@link ElementContainerAdapter} which manages {@code GridRow} slices. */
class GridRowAdapter extends ElementContainerAdapter<LinearLayout, ElementListAdapter, GridRow> {

  private static final String TAG = "GridRowAdapter";

  private GridRowAdapter(Context context, AdapterParameters parameters) {
    super(context, parameters, createView(context), KeySupplier.SINGLETON_KEY);
  }

  @Override
  GridRow getModelFromElement(Element baseElement) {
    if (!baseElement.hasGridRow()) {
      throw new IllegalArgumentException(
          String.format("Missing GridRow; has %s", baseElement.getElementsCase()));
    }
    return baseElement.getGridRow();
  }

  @Override
  void onCreateAdapter(GridRow gridRow, Element baseElement, FrameContext frameContext) {
    // Populate the LinearLayout with cells.
    for (GridCell cell : gridRow.getCellsList()) {
      ElementListAdapter cellAdapter;
      switch (cell.getCellContentCase()) {
        case ELEMENT_LIST:
          cellAdapter = getAdapterForChildList(cell.getElementList(), frameContext);
          cellAdapter.createAdapter(cell.getElementList(), frameContext);
          setLayoutParamsOnCell(cellAdapter, cell, frameContext);
          break;
        case ELEMENT_LIST_BINDING:
          cellAdapter = getAdapterForChildList(ElementList.getDefaultInstance(), frameContext);
          break;
        default:
          throw new IllegalArgumentException(
              String.format("Cell has no recognized content; got %s", cell.getCellContentCase()));
      }
      getBaseView().addView(cellAdapter.getView());
    }
  }

  @Override
  void onBindModel(GridRow gridRow, Element baseElement, FrameContext frameContext) {
    // Populate the LinearLayout with cells.
    checkState(
        gridRow.getCellsCount() == childAdapters.size(),
        "Number of cells mismatch: got %s cells, had %s adapters",
        gridRow.getCellsCount(),
        childAdapters.size());
    // When re-binding, we should be able to assume that the locations of bound or inline cells in
    // the list are the same, since we are re-binding the same template.
    for (int i = 0; i < gridRow.getCellsCount(); i++) {
      GridCell cell = gridRow.getCells(i);
      ElementListAdapter targetAdapter = childAdapters.get(i);
      switch (cell.getCellContentCase()) {
        case ELEMENT_LIST:
          targetAdapter.bindModel(cell.getElementList(), frameContext);
          break;
        case ELEMENT_LIST_BINDING:
          createAndBindChildListAdapter(
              targetAdapter, cell.getElementListBinding(), null, frameContext);

          // TODO: If a style on the bound ElementList could cause the adapter to create
          // a wrapper view (or lose one), remove the adapter's view from the LinearLayout and
          // re-add the result of getView().

          setLayoutParamsOnCell(targetAdapter, cell, frameContext);
          break;
        default:
          throw new IllegalArgumentException(
              String.format("Cell has no recognized content; got %s", cell.getCellContentCase()));
      }
    }
  }

  @Override
  void onUnbindModel() {
    super.onUnbindModel();

    // Bound cell layout is created at bind time. We can't reuse the layout in child adapters, so we
    // call release at unbind time so that the child adapter's children can be recycled.
    GridRow model = getRawModel();
    if (model != null) {
      for (int i = 0; i < model.getCellsCount(); i++) {
        GridCell cell = model.getCells(i);
        if (cell.hasElementListBinding()) {
          childAdapters.get(i).releaseAdapter();
        }
      }
    }
  }

  @Override
  StyleIdsStack getElementStyleIdsStack() {
    return getModel().getStyleReferences();
  }

  private void setLayoutParamsOnCell(
      ElementListAdapter cellAdapter, GridCell cell, FrameContext frameContext) {
    LayoutParams params = new LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.MATCH_PARENT);

    GridCellWidth gridCellWidth = null;
    if (cell.hasWidth()) {
      gridCellWidth = cell.getWidth();
    } else if (cell.hasWidthBinding()) {
      gridCellWidth = frameContext.getGridCellWidthFromBinding(cell.getWidthBinding());
    }

    // If we have a requested width, use it; otherwise, WRAP_CONTENT.
    if (gridCellWidth != null) {
      if (gridCellWidth.hasWeight()) {
        params.weight = gridCellWidth.getWeight();
        params.width = 0;
      } else if (gridCellWidth.hasDp()) {
        params.width = (int) ViewUtils.dpToPx(gridCellWidth.getDp(), getContext());
      }
    }

    switch (cellAdapter.getVerticalGravity()) {
      case GRAVITY_BOTTOM:
        params.gravity = Gravity.BOTTOM;
        break;
      case GRAVITY_MIDDLE:
        params.gravity = Gravity.CENTER_VERTICAL;
        break;
      case GRAVITY_TOP:
        params.gravity = Gravity.TOP;
        break;
      default:
        params.gravity = Gravity.NO_GRAVITY;
    }

    cellAdapter.setLayoutParams(params);
  }

  /** Create an Adapter for the {@code ElementList}. */
  private ElementListAdapter getAdapterForChildList(ElementList list, FrameContext frameContext) {

    // Most GridCells will only contain a single element in their list. We could consider an
    // optimization where we omit the list and just insert the single element's view. But this has
    // a number of complexities with backgrounds, paddings, and debugging the layout in general, so
    // we omit this behavior for now.

    ElementListAdapter listAdapter =
        getParameters().elementAdapterFactory.createElementListAdapter(list, frameContext);
    addChildAdapter(listAdapter);
    return listAdapter;
  }

  @VisibleForTesting
  static LinearLayout createView(Context context) {
    LinearLayout viewGroup = new LinearLayout(context);
    viewGroup.setOrientation(LinearLayout.HORIZONTAL);
    viewGroup.setLayoutParams(
        new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));
    viewGroup.setBaselineAligned(false);
    return viewGroup;
  }

  /** A {@link AdapterKeySupplier} for the {@link GridRowAdapter}. */
  static class KeySupplier extends SingletonKeySupplier<GridRowAdapter, GridRow> {
    @Override
    public String getAdapterTag() {
      return TAG;
    }

    @Override
    public GridRowAdapter getAdapter(Context context, AdapterParameters parameters) {
      return new GridRowAdapter(context, parameters);
    }
  }
}
