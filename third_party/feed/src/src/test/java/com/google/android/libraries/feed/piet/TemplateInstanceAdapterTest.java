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

import static com.google.android.libraries.feed.common.testing.RunnableSubject.assertThatRunnable;
import static com.google.android.libraries.feed.piet.StyleProvider.DEFAULT_STYLE;
import static com.google.android.libraries.feed.piet.StyleProvider.DEFAULT_STYLE_PROVIDER;
import static com.google.common.truth.Truth.assertThat;
import static org.mockito.MockitoAnnotations.initMocks;

import android.app.Activity;
import android.content.Context;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;
import android.widget.TextView;
import com.google.android.libraries.feed.common.testing.Suppliers;
import com.google.android.libraries.feed.host.config.DebugBehavior;
import com.google.android.libraries.feed.piet.TemplateInstanceAdapter.TemplateAdapterModel;
import com.google.android.libraries.feed.piet.TemplateInstanceAdapter.TemplateKey;
import com.google.android.libraries.feed.piet.TemplateInstanceAdapter.TemplateKeySupplier;
import com.google.android.libraries.feed.piet.host.ActionHandler;
import com.google.android.libraries.feed.piet.host.AssetProvider;
import com.google.android.libraries.feed.piet.host.CustomElementProvider;
import com.google.android.libraries.feed.piet.host.HostBindingProvider;
import com.google.search.now.ui.piet.BindingRefsProto.ParameterizedTextBindingRef;
import com.google.search.now.ui.piet.BindingRefsProto.TemplateBindingRef;
import com.google.search.now.ui.piet.ElementsProto.BindingContext;
import com.google.search.now.ui.piet.ElementsProto.BindingValue;
import com.google.search.now.ui.piet.ElementsProto.CustomElement;
import com.google.search.now.ui.piet.ElementsProto.Element;
import com.google.search.now.ui.piet.ElementsProto.ElementList;
import com.google.search.now.ui.piet.ElementsProto.TemplateInvocation;
import com.google.search.now.ui.piet.ElementsProto.TextElement;
import com.google.search.now.ui.piet.PietProto.Frame;
import com.google.search.now.ui.piet.PietProto.PietSharedState;
import com.google.search.now.ui.piet.PietProto.Stylesheet;
import com.google.search.now.ui.piet.PietProto.Template;
import com.google.search.now.ui.piet.StylesProto.Style;
import com.google.search.now.ui.piet.StylesProto.StyleIdsStack;
import com.google.search.now.ui.piet.TextProto.ParameterizedText;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;

/** Tests of the {@link TemplateInstanceAdapter}. */
@RunWith(RobolectricTestRunner.class)
public class TemplateInstanceAdapterTest {

  private static final int FRAME_COLOR = 12345;
  private static final int TEMPLATE_COLOR = 54321;
  private static final String FRAME_STYLESHEET_ID = "coolcat";
  private static final String TEXT_STYLE_ID = "catalog";
  private static final String TEMPLATE_ID = "duplicat";
  private static final String OTHER_TEMPLATE_ID = "alleycat";
  private static final String TEXT_BINDING_ID = "ofmiceandmen";
  private static final String TEXT_CONTENTS = "afewmilessouth";
  private static final String TEXT_CONTENTS_2 = "gangaftagley";

  private static final Stylesheet TEMPLATE_STYLESHEET =
      Stylesheet.newBuilder()
          .addStyles(Style.newBuilder().setStyleId(TEXT_STYLE_ID).setColor(TEMPLATE_COLOR))
          .build();
  private static final Template DEFAULT_TEMPLATE =
      Template.newBuilder()
          .setTemplateId(TEMPLATE_ID)
          .setElementList(
              ElementList.newBuilder()
                  .addElements(
                      Element.newBuilder()
                          .setTextElement(
                              TextElement.newBuilder()
                                  .setStyleReferences(
                                      StyleIdsStack.newBuilder().addStyleIds(TEXT_STYLE_ID))
                                  .setParameterizedTextBinding(
                                      ParameterizedTextBindingRef.newBuilder()
                                          .setBindingId(TEXT_BINDING_ID)))))
          .build();
  private static final Template OTHER_TEMPLATE =
      Template.newBuilder()
          .setTemplateId(OTHER_TEMPLATE_ID)
          .setElementList(ElementList.getDefaultInstance())
          .build();

  private static final Frame DEFAULT_FRAME =
      Frame.newBuilder()
          .setStylesheet(
              Stylesheet.newBuilder()
                  .setStylesheetId(FRAME_STYLESHEET_ID)
                  .addStyles(Style.newBuilder().setStyleId(TEXT_STYLE_ID).setColor(FRAME_COLOR)))
          .addTemplates(DEFAULT_TEMPLATE)
          .addTemplates(OTHER_TEMPLATE)
          .build();

  private static final BindingContext DEFAULT_BINDING =
      BindingContext.newBuilder()
          .addBindingValues(
              BindingValue.newBuilder()
                  .setBindingId(TEXT_BINDING_ID)
                  .setParameterizedText(ParameterizedText.newBuilder().setText(TEXT_CONTENTS)))
          .build();

  private static final TemplateInvocation DEFAULT_TEMPLATE_INVOCATION =
      TemplateInvocation.newBuilder()
          .setTemplateId(TEMPLATE_ID)
          .addBindingContexts(DEFAULT_BINDING)
          .build();

  private static final TemplateAdapterModel DEFAULT_TEMPLATE_MODEL =
      new TemplateAdapterModel(DEFAULT_TEMPLATE, DEFAULT_BINDING);

  @Mock private AssetProvider assetProvider;
  @Mock private CustomElementProvider customElementProvider;
  @Mock private ActionHandler actionHandler;

  private Context context;
  private AdapterParameters adapterParameters;
  private FrameContext frameContext;

  private TemplateInstanceAdapter adapter;
  private List<PietSharedState> pietSharedStates;

  @Before
  public void setUp() throws Exception {
    initMocks(this);
    context = Robolectric.setupActivity(Activity.class);
    pietSharedStates = new ArrayList<>();
    frameContext =
        FrameContext.createFrameContext(
            DEFAULT_FRAME,
            DEFAULT_STYLE,
            pietSharedStates,
            DebugBehavior.VERBOSE,
            new DebugLogger(),
            assetProvider,
            customElementProvider,
            new HostBindingProvider(),
            actionHandler);
    adapterParameters = new AdapterParameters(context, Suppliers.of(null));

    adapter = new TemplateInstanceAdapter(context, adapterParameters);
  }

  @Test
  public void testOnCreateAdapter_templateNotFound() {
    assertThatRunnable(
            () -> new TemplateAdapterModel("NOT_A_REAL_ID", frameContext, DEFAULT_BINDING))
        .throwsAnExceptionOfType(NullPointerException.class)
        .that()
        .hasMessageThat()
        .contains("Template was not found: NOT_A_REAL_ID");
  }

  @Test
  public void testOnCreateAdapter_templateMismatchFails() {
    adapter.createAdapter(DEFAULT_TEMPLATE_MODEL, frameContext);

    assertThatRunnable(
            () ->
                adapter.createAdapter(
                    new TemplateAdapterModel(OTHER_TEMPLATE, DEFAULT_BINDING), frameContext))
        .throwsAnExceptionOfType(IllegalArgumentException.class)
        .that()
        .hasMessageThat()
        .contains("Adapter was constructed already with different template");
  }

  @Test
  public void testOnCreateAdapter_recreateDoesNothing() {
    adapter.createAdapter(DEFAULT_TEMPLATE_MODEL, frameContext);
    RecyclerKey originalKey = adapter.getKey();
    View originalView = adapter.getView();

    Template equivalentTemplate = DEFAULT_TEMPLATE.toBuilder().build();
    assertThat(equivalentTemplate).isNotSameAs(DEFAULT_TEMPLATE);
    assertThat(equivalentTemplate).isEqualTo(DEFAULT_TEMPLATE);
    frameContext =
        FrameContext.createFrameContext(
            DEFAULT_FRAME.toBuilder().clearTemplates().addTemplates(equivalentTemplate).build(),
            DEFAULT_STYLE,
            pietSharedStates,
            DebugBehavior.VERBOSE,
            new DebugLogger(),
            assetProvider,
            customElementProvider,
            new HostBindingProvider(),
            actionHandler);

    adapter.createAdapter(DEFAULT_TEMPLATE_MODEL, frameContext);
    assertThat(adapter.getKey()).isSameAs(originalKey);
    assertThat(adapter.getView()).isSameAs(originalView);
  }

  @Test
  public void testOnBindModel_templateMismatchFails() {
    adapter.createAdapter(DEFAULT_TEMPLATE_MODEL, frameContext);

    assertThatRunnable(
            () ->
                adapter.bindModel(
                    new TemplateAdapterModel(OTHER_TEMPLATE, DEFAULT_BINDING), frameContext))
        .throwsAnExceptionOfType(IllegalArgumentException.class)
        .that()
        .hasMessageThat()
        .contains("Model template does not match");
  }

  @Test
  public void testOnCreateAndBind_singleInstanceWithDefaultStyle() {
    adapter.createAdapter(DEFAULT_TEMPLATE_MODEL, frameContext);

    // Check the recycling key
    assertThat(adapter.getKey()).isEqualTo(new TemplateKey(DEFAULT_TEMPLATE, pietSharedStates));

    // Assert that all child views have been created
    LinearLayout templateView = adapter.getBaseView();
    assertThat(templateView.getChildCount()).isEqualTo(1);

    LinearLayout elementListView = (LinearLayout) templateView.getChildAt(0);

    // Check that default layout params are set.
    assertThat(elementListView.getLayoutParams().width).isEqualTo(LayoutParams.MATCH_PARENT);
    assertThat(elementListView.getLayoutParams().height).isEqualTo(LayoutParams.WRAP_CONTENT);

    // Check for child element
    assertThat(elementListView.getChildCount()).isEqualTo(1);
    TextView textElementView = (TextView) elementListView.getChildAt(0);

    // Text style comes from the default style since template doesn't set one.
    assertThat(textElementView.getTextColors().getDefaultColor())
        .isEqualTo(DEFAULT_STYLE_PROVIDER.getColor());

    // No text before binding
    assertThat(textElementView.getText().toString()).isEmpty();

    // Binding sets text.
    adapter.bindModel(DEFAULT_TEMPLATE_MODEL, frameContext);
    assertThat(textElementView.getText().toString()).isEqualTo(TEXT_CONTENTS);
  }

  @Test
  public void testOnCreateAndBind_singleInstanceWithTemplateStyle() {
    Template defaultTemplateWithLocalStylesheet =
        DEFAULT_TEMPLATE.toBuilder().setStylesheet(TEMPLATE_STYLESHEET).build();
    TemplateAdapterModel model =
        new TemplateAdapterModel(defaultTemplateWithLocalStylesheet, DEFAULT_BINDING);
    frameContext =
        FrameContext.createFrameContext(
            DEFAULT_FRAME
                .toBuilder()
                .clearTemplates()
                .addTemplates(defaultTemplateWithLocalStylesheet)
                .build(),
            DEFAULT_STYLE,
            pietSharedStates,
            DebugBehavior.VERBOSE,
            new DebugLogger(),
            assetProvider,
            customElementProvider,
            new HostBindingProvider(),
            actionHandler);

    adapter.createAdapter(model, frameContext);

    // Assert that all child views have been created and that text is set.
    LinearLayout templateView = adapter.getBaseView();
    LinearLayout elementListView = (LinearLayout) templateView.getChildAt(0);
    TextView textElementView = (TextView) elementListView.getChildAt(0);

    // Text style comes from the Template stylesheet.
    assertThat(textElementView.getTextColors().getDefaultColor()).isEqualTo(TEMPLATE_COLOR);

    // Text is not populated yet
    assertThat(textElementView.getText().toString()).isEmpty();

    // Text is populated after binding.
    adapter.bindModel(model, frameContext);
    assertThat(textElementView.getText().toString()).isEqualTo(TEXT_CONTENTS);
  }

  @Test
  public void testReleaseAdapter_doesNothing() {
    adapter.createAdapter(DEFAULT_TEMPLATE_MODEL, frameContext);
    adapter.bindModel(DEFAULT_TEMPLATE_MODEL, frameContext);
    LinearLayout templateView = adapter.getBaseView();
    assertThat(templateView.getChildCount()).isEqualTo(1);
    LinearLayout elementListView = (LinearLayout) templateView.getChildAt(0);
    assertThat(((TextView) elementListView.getChildAt(0)).getText().toString())
        .isEqualTo(TEXT_CONTENTS);
    ElementListAdapter listAdapter = adapter.childAdapters.get(0);

    adapter.releaseAdapter();

    // Assert that nothing has changed.
    assertThat(templateView.getChildCount()).isEqualTo(1);
    assertThat(elementListView.getChildCount()).isEqualTo(1);
    assertThat(((TextView) elementListView.getChildAt(0)).getText().toString())
        .isEqualTo(TEXT_CONTENTS);
    assertThat(adapter.childAdapters).containsExactly(listAdapter);
  }

  @Test
  public void testOverlaysAreRecreatedOnReBind() {
    Element overlayElement =
        Element.newBuilder()
            .setTemplateInvocation(DEFAULT_TEMPLATE_INVOCATION)
            .addOverlayElements(
                ElementList.newBuilder()
                    .addElements(
                        Element.newBuilder()
                            .setTextElement(
                                TextElement.newBuilder()
                                    .setParameterizedText(
                                        ParameterizedText.newBuilder().setText(TEXT_CONTENTS)))))
            .build();

    adapter.createAdapter(DEFAULT_TEMPLATE_MODEL, overlayElement, frameContext);
    adapter.bindModel(DEFAULT_TEMPLATE_MODEL, overlayElement, frameContext);
    assertThat(adapter.overlays).hasSize(1);
    assertThat(adapter.getView()).isNotEqualTo(adapter.getBaseView());
    assertThat(((FrameLayout) adapter.getView()).getChildCount()).isEqualTo(2);

    adapter.releaseAdapter();
    assertThat(adapter.overlays).isEmpty();
    assertThat(adapter.getView()).isSameAs(adapter.getBaseView());

    adapter.createAdapter(DEFAULT_TEMPLATE_MODEL, overlayElement, frameContext);
    adapter.bindModel(DEFAULT_TEMPLATE_MODEL, overlayElement, frameContext);
    assertThat(adapter.overlays).hasSize(1);
    assertThat(adapter.getView()).isNotEqualTo(adapter.getBaseView());
    assertThat(((FrameLayout) adapter.getView()).getChildCount()).isEqualTo(2);
  }

  @Test
  public void testCreateViewGroup() {
    LinearLayout viewGroup = TemplateInstanceAdapter.createView(context);

    assertThat(viewGroup.getOrientation()).isEqualTo(LinearLayout.VERTICAL);
    assertThat(viewGroup.getLayoutParams().width).isEqualTo(LayoutParams.MATCH_PARENT);
    assertThat(viewGroup.getLayoutParams().height).isEqualTo(LayoutParams.WRAP_CONTENT);
  }

  @Test
  public void testTemplateKay_equalWithSameObjects() {
    Template template = Template.newBuilder().setTemplateId("T").build();
    List<PietSharedState> sharedStates =
        convertToPietSharedState(PietSharedState.newBuilder().addTemplates(template).build());
    TemplateKey key1 = new TemplateKey(template, sharedStates);
    TemplateKey key2 = new TemplateKey(template, sharedStates);

    assertThat(key1.hashCode()).isEqualTo(key2.hashCode());
    assertThat(key1).isEqualTo(key2);
  }

  @Test
  public void testTemplateKey_equalWithDifferentTemplateObject() {
    Template template1 = Template.newBuilder().setTemplateId("T").build();
    Template template2 = Template.newBuilder().setTemplateId("T").build();
    PietSharedState sharedState = PietSharedState.newBuilder().addTemplates(template1).build();
    TemplateKey key1 = new TemplateKey(template1, convertToPietSharedState(sharedState));
    TemplateKey key2 = new TemplateKey(template2, convertToPietSharedState(sharedState));

    assertThat(key1.hashCode()).isEqualTo(key2.hashCode());
    assertThat(key1).isEqualTo(key2);
  }

  private List<PietSharedState> convertToPietSharedState(PietSharedState... pietSharedStates) {
    List<PietSharedState> sharedStates = new ArrayList<>();
    Collections.addAll(sharedStates, pietSharedStates);
    return sharedStates;
  }

  @Test
  public void testTemplateKey_equalWithDifferentSharedStateObject() {
    Template template = Template.newBuilder().setTemplateId("T").build();
    PietSharedState sharedState1 = PietSharedState.newBuilder().addTemplates(template).build();
    PietSharedState sharedState2 = PietSharedState.newBuilder().addTemplates(template).build();
    TemplateKey key1 = new TemplateKey(template, convertToPietSharedState(sharedState1));
    TemplateKey key2 = new TemplateKey(template, convertToPietSharedState(sharedState2));

    assertThat(key1.hashCode()).isEqualTo(key2.hashCode());
    assertThat(key1).isEqualTo(key2);
  }

  @Test
  public void testTemplateKey_differentWithDifferentLengthSharedStates() {
    Template template = Template.newBuilder().setTemplateId("T").build();
    PietSharedState sharedState1 = PietSharedState.newBuilder().addTemplates(template).build();
    TemplateKey key1 = new TemplateKey(template, convertToPietSharedState(sharedState1));
    TemplateKey key2 =
        new TemplateKey(template, convertToPietSharedState(sharedState1, sharedState1));

    assertThat(key1.hashCode()).isNotEqualTo(key2.hashCode());
    assertThat(key1).isNotEqualTo(key2);
  }

  @Test
  public void testTemplateKey_differentWithDifferentTemplate() {
    Template template1 = Template.newBuilder().setTemplateId("T1").build();
    Template template2 = Template.newBuilder().setTemplateId("T2").build();
    List<PietSharedState> sharedStates =
        convertToPietSharedState(PietSharedState.newBuilder().addTemplates(template1).build());
    TemplateKey key1 = new TemplateKey(template1, sharedStates);
    TemplateKey key2 = new TemplateKey(template2, sharedStates);

    assertThat(key1.hashCode()).isNotEqualTo(key2.hashCode());
    assertThat(key1).isNotEqualTo(key2);
  }

  @Test
  public void testTemplateKey_differentWithDifferentSharedState() {
    Template template = Template.newBuilder().setTemplateId("T").build();
    PietSharedState sharedState1 = PietSharedState.newBuilder().addTemplates(template).build();
    PietSharedState sharedState2 = PietSharedState.getDefaultInstance();
    TemplateKey key1 = new TemplateKey(template, convertToPietSharedState(sharedState1));
    TemplateKey key2 = new TemplateKey(template, convertToPietSharedState(sharedState2));

    assertThat(key1.hashCode()).isNotEqualTo(key2.hashCode());
    assertThat(key1).isNotEqualTo(key2);
  }

  @Test
  public void testTemplateKeySupplier() {
    TemplateKeySupplier keySupplier = new TemplateKeySupplier();

    assertThat(keySupplier.getKey(frameContext, DEFAULT_TEMPLATE_MODEL))
        .isEqualTo(new TemplateKey(DEFAULT_TEMPLATE, pietSharedStates));
  }

  @Test
  public void testGetModelFromElement() {
    Element elementWithModel =
        Element.newBuilder().setTemplateBinding(TemplateBindingRef.getDefaultInstance()).build();
    assertThatRunnable(() -> adapter.getModelFromElement(elementWithModel))
        .throwsAnExceptionOfType(IllegalArgumentException.class)
        .that()
        .hasMessageThat()
        .contains("Templates must be re-bound");

    Element elementWithWrongModel =
        Element.newBuilder().setCustomElement(CustomElement.getDefaultInstance()).build();
    assertThatRunnable(() -> adapter.getModelFromElement(elementWithWrongModel))
        .throwsAnExceptionOfType(IllegalArgumentException.class)
        .that()
        .hasMessageThat()
        .contains("Templates must be re-bound");

    Element emptyElement = Element.getDefaultInstance();
    assertThatRunnable(() -> adapter.getModelFromElement(emptyElement))
        .throwsAnExceptionOfType(IllegalArgumentException.class)
        .that()
        .hasMessageThat()
        .contains("Templates must be re-bound");
  }

  @Test
  public void testTemplateAdapterModel_getters() {
    TemplateAdapterModel model = new TemplateAdapterModel(DEFAULT_TEMPLATE, DEFAULT_BINDING);
    assertThat(model.getTemplate()).isSameAs(DEFAULT_TEMPLATE);
    assertThat(model.getBindingContext()).isSameAs(DEFAULT_BINDING);
  }

  @Test
  public void testTemplateAdapterModel_lookUpTemplate() {
    TemplateAdapterModel model =
        new TemplateAdapterModel(DEFAULT_TEMPLATE.getTemplateId(), frameContext, DEFAULT_BINDING);
    assertThat(model.getTemplate()).isSameAs(DEFAULT_TEMPLATE);
    assertThat(model.getBindingContext()).isSameAs(DEFAULT_BINDING);
  }

  @Test
  public void testTemplateAdapterModel_equalsSame() {
    TemplateAdapterModel model1 = new TemplateAdapterModel(DEFAULT_TEMPLATE, DEFAULT_BINDING);
    TemplateAdapterModel model2 = new TemplateAdapterModel(DEFAULT_TEMPLATE, DEFAULT_BINDING);
    assertThat(model1).isEqualTo(model2);
    assertThat(model1.hashCode()).isEqualTo(model2.hashCode());
  }

  @Test
  public void testTemplateAdapterModel_equalsOtherInstance() {
    TemplateAdapterModel model1 = new TemplateAdapterModel(DEFAULT_TEMPLATE, DEFAULT_BINDING);
    TemplateAdapterModel model2 =
        new TemplateAdapterModel(
            DEFAULT_TEMPLATE.toBuilder().build(), DEFAULT_BINDING.toBuilder().build());
    assertThat(model1.getTemplate()).isNotSameAs(model2.getTemplate());
    assertThat(model1.getBindingContext()).isNotSameAs(model2.getBindingContext());
    assertThat(model1).isEqualTo(model2);
    assertThat(model1.hashCode()).isEqualTo(model2.hashCode());
  }

  @Test
  public void testTemplateAdapterModel_notEquals() {
    TemplateAdapterModel model1 = new TemplateAdapterModel(DEFAULT_TEMPLATE, DEFAULT_BINDING);
    TemplateAdapterModel model2 =
        new TemplateAdapterModel(
            DEFAULT_TEMPLATE.toBuilder().clearTemplateId().build(), DEFAULT_BINDING);
    TemplateAdapterModel model3 =
        new TemplateAdapterModel(
            DEFAULT_TEMPLATE, DEFAULT_BINDING.toBuilder().clearBindingValues().build());
    assertThat(model1).isNotEqualTo(model2);
    assertThat(model1.hashCode()).isNotEqualTo(model2.hashCode());
    assertThat(model1).isNotEqualTo(model3);
    assertThat(model1.hashCode()).isNotEqualTo(model3.hashCode());
  }
}
