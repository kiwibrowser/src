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

import android.content.Context;
import android.support.annotation.VisibleForTesting;
import android.view.View;
import com.google.android.libraries.feed.piet.DebugLogger.MessageType;
import com.google.android.libraries.feed.piet.TemplateInstanceAdapter.TemplateAdapterModel;
import com.google.search.now.ui.piet.ElementsProto.BindingContext;
import com.google.search.now.ui.piet.ElementsProto.BindingValue;
import com.google.search.now.ui.piet.ElementsProto.CustomElement;
import com.google.search.now.ui.piet.ElementsProto.Element;
import com.google.search.now.ui.piet.ElementsProto.ElementList;
import com.google.search.now.ui.piet.ElementsProto.GridRow;
import com.google.search.now.ui.piet.ElementsProto.ImageElement;
import com.google.search.now.ui.piet.ElementsProto.SpacerElement;
import com.google.search.now.ui.piet.ElementsProto.TemplateInvocation;
import com.google.search.now.ui.piet.ElementsProto.TextElement;
import com.google.search.now.ui.piet.PietProto.Template;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/** Provides methods to create various adapter types based on bindings. */
class ElementAdapterFactory {
  private final AdapterFactory<CustomElementAdapter, CustomElement> customElementFactory;
  private final AdapterFactory<ChunkedTextElementAdapter, TextElement> chunkedTextElementFactory;
  private final AdapterFactory<ParameterizedTextElementAdapter, TextElement>
      parameterizedTextElementFactory;
  private final AdapterFactory<ImageElementAdapter, ImageElement> imageElementFactory;
  private final AdapterFactory<SpacerElementAdapter, SpacerElement> spacerElementFactory;
  private final AdapterFactory<GridRowAdapter, GridRow> gridRowFactory;
  private final AdapterFactory<ElementListAdapter, ElementList> elementListFactory;
  private final AdapterFactory<TemplateInstanceAdapter, TemplateAdapterModel>
      templateInvocationFactory;
  private final List<AdapterFactory<?, ?>> factories;

  ElementAdapterFactory(Context context, AdapterParameters parameters) {
    this(
        new AdapterFactory<>(context, parameters, new CustomElementAdapter.KeySupplier()),
        new AdapterFactory<>(context, parameters, new ChunkedTextElementAdapter.KeySupplier()),
        new AdapterFactory<>(
            context, parameters, new ParameterizedTextElementAdapter.KeySupplier()),
        new AdapterFactory<>(context, parameters, new ImageElementAdapter.KeySupplier()),
        new AdapterFactory<>(context, parameters, new SpacerElementAdapter.KeySupplier()),
        new AdapterFactory<>(context, parameters, new GridRowAdapter.KeySupplier()),
        new AdapterFactory<>(context, parameters, new ElementListAdapter.KeySupplier()),
        new AdapterFactory<>(
            context, parameters, new TemplateInstanceAdapter.TemplateKeySupplier()));
  }

  /** Testing-only constructor for mocking the factories. */
  @VisibleForTesting
  ElementAdapterFactory(
      AdapterFactory<CustomElementAdapter, CustomElement> customElementFactory,
      AdapterFactory<ChunkedTextElementAdapter, TextElement> chunkedTextElementFactory,
      AdapterFactory<ParameterizedTextElementAdapter, TextElement> parameterizedTextElementFactory,
      AdapterFactory<ImageElementAdapter, ImageElement> imageElementFactory,
      AdapterFactory<SpacerElementAdapter, SpacerElement> spacerElementFactory,
      AdapterFactory<GridRowAdapter, GridRow> gridRowFactory,
      AdapterFactory<ElementListAdapter, ElementList> elementListFactory,
      AdapterFactory<TemplateInstanceAdapter, TemplateAdapterModel> templateInvocationFactory) {
    this.customElementFactory = customElementFactory;
    this.chunkedTextElementFactory = chunkedTextElementFactory;
    this.parameterizedTextElementFactory = parameterizedTextElementFactory;
    this.imageElementFactory = imageElementFactory;
    this.spacerElementFactory = spacerElementFactory;
    this.gridRowFactory = gridRowFactory;
    this.elementListFactory = elementListFactory;
    this.templateInvocationFactory = templateInvocationFactory;
    factories =
        Collections.unmodifiableList(
            Arrays.asList(
                customElementFactory,
                chunkedTextElementFactory,
                parameterizedTextElementFactory,
                imageElementFactory,
                spacerElementFactory,
                gridRowFactory,
                elementListFactory,
                templateInvocationFactory));
  }

  ElementAdapter<? extends View, ?> createAdapterForElement(
      Element element, FrameContext frameContext) {
    switch (element.getElementsCase()) {
      case CUSTOM_ELEMENT:
        return customElementFactory.get(element.getCustomElement(), frameContext);
      case TEXT_ELEMENT:
        switch (element.getTextElement().getContentCase()) {
          case CHUNKED_TEXT:
          case CHUNKED_TEXT_BINDING:
            return chunkedTextElementFactory.get(element.getTextElement(), frameContext);
          case PARAMETERIZED_TEXT:
          case PARAMETERIZED_TEXT_BINDING:
            return parameterizedTextElementFactory.get(element.getTextElement(), frameContext);
          default:
            throw new IllegalArgumentException(
                String.format(
                    "Unsupported TextElement type: %s", element.getTextElement().getContentCase()));
        }
      case IMAGE_ELEMENT:
        return imageElementFactory.get(element.getImageElement(), frameContext);
      case SPACER_ELEMENT:
        return spacerElementFactory.get(element.getSpacerElement(), frameContext);
      case GRID_ROW:
        return gridRowFactory.get(element.getGridRow(), frameContext);
      case ELEMENT_LIST:
        return elementListFactory.get(element.getElementList(), frameContext);
      case ELEMENT_LIST_BINDING:
        BindingValue elementListBindingValue =
            frameContext.getElementListBindingValue(element.getElementListBinding());
        // TODO: support the various bits of metadata in BindingValue
        return elementListFactory.get(elementListBindingValue.getElementList(), frameContext);
      case TEMPLATE_BINDING:
        TemplateInvocation templateInvocation =
            frameContext.getTemplateInvocationFromBinding(element.getTemplateBinding());
        if (templateInvocation != null) {
          return createTemplateAdapter(templateInvocation, frameContext);
        } else {
          frameContext.reportError(
              MessageType.ERROR,
              String.format("Could not get template for %s", element.getTemplateBinding()));
          return templateInvocationFactory.get(
              new TemplateAdapterModel(
                  Template.getDefaultInstance(), BindingContext.getDefaultInstance()),
              frameContext);
        }
      case ELEMENTS_NOT_SET:
      default:
        throw new IllegalArgumentException(
            String.format("Unsupported Element type: %s", element.getElementsCase()));
    }
  }

  ElementListAdapter createElementListAdapter(ElementList elementList, FrameContext frameContext) {
    return elementListFactory.get(elementList, frameContext);
  }

  ParameterizedTextElementAdapter createAdapterForParameterizedText(
      TextElement textLine, FrameContext frameContext) {
    return parameterizedTextElementFactory.get(textLine, frameContext);
  }

  TemplateInstanceAdapter createTemplateAdapter(
      TemplateInvocation templateInvocation, FrameContext frameContext) {
    TemplateAdapterModel model =
        new TemplateAdapterModel(
            templateInvocation.getTemplateId(),
            frameContext,
            templateInvocation.getBindingContexts(0));
    return templateInvocationFactory.get(model, frameContext);
  }

  void releaseAdapter(ElementAdapter<? extends View, ?> adapter) {
    if (adapter instanceof CustomElementAdapter) {
      customElementFactory.release((CustomElementAdapter) adapter);
    } else if (adapter instanceof ChunkedTextElementAdapter) {
      chunkedTextElementFactory.release((ChunkedTextElementAdapter) adapter);
    } else if (adapter instanceof ParameterizedTextElementAdapter) {
      parameterizedTextElementFactory.release((ParameterizedTextElementAdapter) adapter);
    } else if (adapter instanceof ImageElementAdapter) {
      imageElementFactory.release((ImageElementAdapter) adapter);
    } else if (adapter instanceof SpacerElementAdapter) {
      spacerElementFactory.release((SpacerElementAdapter) adapter);
    } else if (adapter instanceof GridRowAdapter) {
      gridRowFactory.release((GridRowAdapter) adapter);
    } else if (adapter instanceof ElementListAdapter) {
      elementListFactory.release((ElementListAdapter) adapter);
    } else if (adapter instanceof TemplateInstanceAdapter) {
      templateInvocationFactory.release((TemplateInstanceAdapter) adapter);
    }
  }

  void purgeRecyclerPools() {
    for (AdapterFactory<?, ?> factory : factories) {
      factory.purgeRecyclerPool();
    }
  }
}
