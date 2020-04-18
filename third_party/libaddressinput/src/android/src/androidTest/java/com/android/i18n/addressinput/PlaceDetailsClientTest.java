package com.android.i18n.addressinput;

import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.test.ActivityInstrumentationTestCase2;
import com.android.i18n.addressinput.testing.TestActivity;
import com.google.common.util.concurrent.ListenableFuture;
import com.google.i18n.addressinput.common.AddressAutocompletePrediction;
import com.google.i18n.addressinput.common.AddressData;
import com.google.i18n.addressinput.common.AsyncRequestApi;
import com.google.i18n.addressinput.common.AsyncRequestApi.AsyncCallback;
import com.google.i18n.addressinput.common.JsoMap;
import java.util.concurrent.ExecutionException;
import org.json.JSONException;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

/** Unit tests for {@link PlaceDetailsClient}. */
public class PlaceDetailsClientTest extends ActivityInstrumentationTestCase2<TestActivity> {
  @Mock private AsyncRequestApi asyncRequestApi;
  @Mock private AddressAutocompletePrediction autocompletePrediction;

  @Captor ArgumentCaptor<AsyncCallback> callbackCaptor;

  private PlaceDetailsClient placeDetailsClient;

  public PlaceDetailsClientTest() {
    super(TestActivity.class);
  }

  @Override
  protected void setUp() {
    MockitoAnnotations.initMocks(this);

    placeDetailsClient = new PlaceDetailsClient("TEST_API_KEY", asyncRequestApi);
    when(autocompletePrediction.getPlaceId()).thenReturn("TEST_PLACE_ID");
  }

  public void testOnSuccess() throws InterruptedException, ExecutionException, JSONException {
    ListenableFuture<AddressData> addressData =
        placeDetailsClient.getAddressData(autocompletePrediction);

    verify(asyncRequestApi)
        .requestObject(any(String.class), callbackCaptor.capture(), eq(PlaceDetailsClient.TIMEOUT));
    callbackCaptor.getValue().onSuccess(JsoMap.buildJsoMap(TEST_RESPONSE));

    assertEquals(
        AddressData.builder()
            .setAddress("1600 Amphitheatre Parkway")
            .setLocality("Mountain View")
            .setAdminArea("CA")
            .setCountry("US")
            .setPostalCode("94043")
            .build(),
        addressData.get());
  }

  public void testOnFailure() {
    ListenableFuture<AddressData> addressData =
        placeDetailsClient.getAddressData(autocompletePrediction);

    verify(asyncRequestApi)
        .requestObject(any(String.class), callbackCaptor.capture(), eq(PlaceDetailsClient.TIMEOUT));
    callbackCaptor.getValue().onFailure();

    assertTrue(addressData.isCancelled());
  }

  private static final String TEST_RESPONSE =
      "{"
          + "  'result' : {"
          + "    'adr_address' : '\\u003cspan class=\\\"street-address\\\"\\u003e1600 Amphitheatre Parkway\\u003c/span\\u003e, \\u003cspan class=\\\"locality\\\"\\u003eMountain View\\u003c/span\\u003e, \\u003cspan class=\\\"region\\\"\\u003eCA\\u003c/span\\u003e \\u003cspan class=\\\"postal-code\\\"\\u003e94043\\u003c/span\\u003e, \\u003cspan class=\\\"country-name\\\"\\u003eUSA\\u003c/span\\u003e',"
          + "    'address_components' : ["
          + "      {"
          + "        'long_name' : 'United States',"
          + "        'short_name' : 'US',"
          + "        'types' : [ 'country', 'political' ]"
          + "      }"
          + "    ]"
          + "  }"
          + "}";
}
