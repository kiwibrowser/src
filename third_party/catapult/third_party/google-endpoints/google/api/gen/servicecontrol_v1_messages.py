# Copyright 2016 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Generated message classes for servicecontrol version v1.

The Service Control API
"""
# NOTE: This file is originally auto-generated using google-apitools then
# style-correcting hand edits were applied.  New behaviour should not provided
# by hand, please re-generate and restyle.
from __future__ import absolute_import

from apitools.base.protorpclite import messages as _messages
from apitools.base.py import encoding

# This is currently not generated, but needs to be present otherwise the methods
# in apitools.base.py.encoding module that support json struct protobuf types
# does not work
from apitools.base.py import extra_types  # pylint: disable=unused-import

package = 'servicecontrol'


class Api(_messages.Message):
    """Api is a light-weight descriptor for a protocol buffer service.

    Enums:
      SyntaxValueValuesEnum: The source syntax of the service.

    Fields:
      methods: The methods of this api, in unspecified order.
      mixins: Included APIs. See Mixin.
      name: The fully qualified name of this api, including package name
        followed by the api's simple name.
      options: Any metadata attached to the API.
      sourceContext: Source context for the protocol buffer service represented
        by this message.
      syntax: The source syntax of the service.
      version: A version string for this api. If specified, must have the form
        `major-version.minor-version`, as in `1.10`. If the minor version is
        omitted, it defaults to zero. If the entire version field is empty, the
        major version is derived from the package name, as outlined below. If
        the field is not empty, the version in the package name will be verified
        to be consistent with what is provided here.  The versioning schema uses
        [semantic versioning](http://semver.org) where the major version number
        indicates a breaking change and the minor version an additive, non-
        breaking change. Both version numbers are signals to users what to
        expect from different versions, and should be carefully chosen based on
        the product plan.  The major version is also reflected in the package
        name of the API, which must end in `v<major-version>`, as in
        `google.feature.v1`. For major versions 0 and 1, the suffix can be
        omitted. Zero major versions must only be used for experimental, none-GA
        apis.
    """

    class SyntaxValueValuesEnum(_messages.Enum):
        """The source syntax of the service.

        Values:
          SYNTAX_PROTO2: Syntax `proto2`.
          SYNTAX_PROTO3: Syntax `proto3`.
        """
        SYNTAX_PROTO2 = 0
        SYNTAX_PROTO3 = 1

    methods = _messages.MessageField('Method', 1, repeated=True)
    mixins = _messages.MessageField('Mixin', 2, repeated=True)
    name = _messages.StringField(3)
    options = _messages.MessageField('Option', 4, repeated=True)
    sourceContext = _messages.MessageField('SourceContext', 5)
    syntax = _messages.EnumField('SyntaxValueValuesEnum', 6)
    version = _messages.StringField(7)


class AreaUnderCurveParams(_messages.Message):
    """AreaUnderCurveParams groups the metrics relevant to generating duration
    based metric from base (snapshot) metric and delta (change) metric.  The
    generated metric has two dimensions:    resource usage metric and the
    duration the metric applies.  Essentially the generated metric is the Area
    Under Curve(AUC) of the "duration - resource" usage curve. This AUC metric
    is readily appliable to billing since "billable resource usage" depends on
    resource usage and duration of the resource used.  A service config may
    contain multiple resources and corresponding metrics. AreaUnderCurveParams
    groups the relevant ones: which snapshot_metric and change_metric are used
    to produce which generated_metric.

    Fields:
      changeMetric: Change of resource usage at a particular timestamp. This
        should a DELTA metric.
      generatedMetric: Metric generated from snapshot_metric and change_metric.
        This is also a DELTA metric.
      snapshotMetric: Total usage of a resource at a particular timestamp. This
        should be a GAUGE metric.
    """

    changeMetric = _messages.StringField(1)
    generatedMetric = _messages.StringField(2)
    snapshotMetric = _messages.StringField(3)


class AuthProvider(_messages.Message):
    """Configuration for an anthentication provider, including support for [JSON
    Web Token (JWT)](https://tools.ietf.org/html/draft-ietf-oauth-json-web-
    token-32).

    Fields:
      id: The unique identifier of the auth provider. It will be referred to by
        `AuthRequirement.provider_id`.  Example: "bookstore_auth".
      issuer: Identifies the principal that issued the JWT. See
        https://tools.ietf.org/html/draft-ietf-oauth-json-web-
        token-32#section-4.1.1 Usually a URL or an email address.  Example:
        https://securetoken.google.com Example:
        1234567-compute@developer.gserviceaccount.com
      jwksUri: URL of the provider's public key set to validate signature of the
        JWT. See [OpenID Discovery](https://openid.net/specs/openid-connect-
        discovery-1_0.html#ProviderMetadata). Optional if the key set document:
        - can be retrieved from    [OpenID Discovery](https://openid.net/specs
        /openid-connect-discovery-1_0.html    of the issuer.  - can be inferred
        from the email domain of the issuer (e.g. a Google service account).
        Example: https://www.googleapis.com/oauth2/v1/certs
    """

    id = _messages.StringField(1)
    issuer = _messages.StringField(2)
    jwksUri = _messages.StringField(3)


class AuthRequirement(_messages.Message):
    """User-defined authentication requirements, including support for [JSON Web
    Token (JWT)](https://tools.ietf.org/html/draft-ietf-oauth-json-web-
    token-32).

    Fields:
      audiences: The list of JWT [audiences](https://tools.ietf.org/html/draft-
        ietf-oauth-json-web-token-32#section-4.1.3). that are allowed to access.
        A JWT containing any of these audiences will be accepted. When this
        setting is absent, only JWTs with audience
        "https://Service_name/API_name" will be accepted. For example, if no
        audiences are in the setting, LibraryService API will only accept JWTs
        with the following audience "https://library-
        example.googleapis.com/google.example.library.v1.LibraryService".
        Example:      audiences: bookstore_android.apps.googleusercontent.com,
        bookstore_web.apps.googleusercontent.com
      providerId: id from authentication provider.  Example:      provider_id:
        bookstore_auth
    """

    audiences = _messages.StringField(1)
    providerId = _messages.StringField(2)


class Authentication(_messages.Message):
    """`Authentication` defines the authentication configuration for an API.
    Example for an API targeted for external use:      name:
    calendar.googleapis.com     authentication:       rules:       - selector:
    "*"         oauth:           canonical_scopes:
    https://www.googleapis.com/auth/calendar        - selector:
    google.calendar.Delegate         oauth:           canonical_scopes:
    https://www.googleapis.com/auth/calendar.read

    Fields:
      providers: Defines a set of authentication providers that a service
        supports.
      rules: Individual rules for authentication.
    """

    providers = _messages.MessageField('AuthProvider', 1, repeated=True)
    rules = _messages.MessageField('AuthenticationRule', 2, repeated=True)


class AuthenticationRule(_messages.Message):
    """Authentication rules for the service.  By default, if a method has any
    authentication requirements, every request must include a valid credential
    matching one of the requirements. It's an error to include more than one
    kind of credential in a single request.  If a method doesn't have any auth
    requirements, request credentials will be ignored.

    Fields:
      allowWithoutCredential: Whether to allow requests without a credential.
        If quota is enabled, an API key is required for such request to pass the
        quota check.
      oauth: The requirements for OAuth credentials.
      requirements: Requirements for additional authentication providers.
      selector: Selects the methods to which this rule applies.  Refer to
        selector for syntax details.
    """

    allowWithoutCredential = _messages.BooleanField(1)
    oauth = _messages.MessageField('OAuthRequirements', 2)
    requirements = _messages.MessageField('AuthRequirement', 3, repeated=True)
    selector = _messages.StringField(4)


class Backend(_messages.Message):
    """`Backend` defines the backend configuration for a service.

    Fields:
      rules: A list of backend rules providing configuration for individual API
        elements.
    """

    rules = _messages.MessageField('BackendRule', 1, repeated=True)


class BackendRule(_messages.Message):
    """A backend rule provides configuration for an individual API element.

    Fields:
      address: The address of the API backend.
      deadline: The number of seconds to wait for a response from a request.
        The default depends on the deployment context.
      selector: Selects the methods to which this rule applies.  Refer to
        selector for syntax details.
    """

    address = _messages.StringField(1)
    deadline = _messages.FloatField(2)
    selector = _messages.StringField(3)


class Billing(_messages.Message):
    """Billing related configuration of the service.  The following example
    shows how to configure metrics for billing:      metrics:     - name:
    library.googleapis.com/read_calls       metric_kind: DELTA       value_type:
    INT64     - name: library.googleapis.com/write_calls       metric_kind:
    DELTA       value_type: INT64     billing:       metrics:       -
    library.googleapis.com/read_calls       - library.googleapis.com/write_calls
    The next example shows how to enable billing status check and customize the
    check behavior. It makes sure billing status check is included in the
    `Check` method of [Service Control API](https://cloud.google.com/service-
    control/). In the example, "google.storage.Get" method can be served when
    the billing status is either `current` or `delinquent`, while
    "google.storage.Write" method can only be served when the billing status is
    `current`:      billing:       rules:       - selector: google.storage.Get
    allowed_statuses:         - current         - delinquent       - selector:
    google.storage.Write         allowed_statuses: current  Mostly services
    should only allow `current` status when serving requests. In addition,
    services can choose to allow both `current` and `delinquent` statuses when
    serving read-only requests to resources. If there's no matching selector for
    operation, no billing status check will be performed.

    Fields:
      areaUnderCurveParams: Per resource grouping for delta billing based
        resource configs.
      metrics: Names of the metrics to report to billing. Each name must be
        defined in Service.metrics section.
      rules: A list of billing status rules for configuring billing status
        check.
    """

    areaUnderCurveParams = _messages.MessageField('AreaUnderCurveParams', 1, repeated=True)
    metrics = _messages.StringField(2, repeated=True)
    rules = _messages.MessageField('BillingStatusRule', 3, repeated=True)


class BillingStatusRule(_messages.Message):
    """Defines the billing status requirements for operations.  When used with
    [Service Control API](https://cloud.google.com/service-control/), the
    following statuses are supported:  - **current**: the associated billing
    account is up to date and capable of                paying for resource
    usages. - **delinquent**: the associated billing account has a correctable
    problem,                   such as late payment.  Mostly services should
    only allow `current` status when serving requests. In addition, services can
    choose to allow both `current` and `delinquent` statuses when serving read-
    only requests to resources. If the list of allowed_statuses is empty, it
    means no billing requirement.

    Fields:
      allowedStatuses: Allowed billing statuses. The billing status check passes
        if the actual billing status matches any of the provided values here.
      selector: Selects the operation names to which this rule applies. Refer to
        selector for syntax details.
    """

    allowedStatuses = _messages.StringField(1, repeated=True)
    selector = _messages.StringField(2)


class CheckError(_messages.Message):
    """Defines the errors to be returned in
    google.api.servicecontrol.v1.CheckResponse.check_errors. NOTE: The list of
    error code must be in sync with the list in http://cs/google3/tech/internal/
    env/framework/wrappers/chemist/chemist_wrapper.cc&l=525

    Enums:
      CodeValueValuesEnum: The error code.

    Fields:
      code: The error code.
      detail: The error detail.
    """

    class CodeValueValuesEnum(_messages.Enum):
        """The error code.

        Values:
          ERROR_CODE_UNSPECIFIED: This is never used in `CheckResponse`.
          NOT_FOUND: The consumer's project id is not found. Same as
            google.rpc.Code.NOT_FOUND.
          PERMISSION_DENIED: The consumer doesn't have access to the specified
            resource. Same as google.rpc.Code.PERMISSION_DENIED.
          RESOURCE_EXHAUSTED: Quota check failed. Same as
            google.rpc.Code.RESOURCE_EXHAUSTED.
          BUDGET_EXCEEDED: Budget check failed.
          DENIAL_OF_SERVICE_DETECTED: The request has been flagged as a DoS
            attack.
          LOAD_SHEDDING: The request should be rejected in order to protect the
            service from being overloaded.
          ABUSER_DETECTED: The consumer has been flagged as an abuser.
          SERVICE_NOT_ACTIVATED: The consumer hasn't activated the service.
          VISIBILITY_DENIED: The consumer cannot access the service due to
            visibility configuration.
          BILLING_DISABLED: The consumer cannot access the service because billing
            is disabled.
          PROJECT_DELETED: Consumer's project has been marked as deleted (soft
            deletion).
          PROJECT_INVALID: Consumer's project number or id does not represent a
            valid project.
          IP_ADDRESS_BLOCKED: Consumer's project does not allow requests from this
            IP address.
          REFERER_BLOCKED: Consumer's project does not allow requests from this
            referer address.
          CLIENT_APP_BLOCKED: Consumer's project does not allow requests from this
            client application.
          API_KEY_INVALID: The consumer's API key is invalid.
          API_KEY_EXPIRED: Consumer's API Key has expired.
          API_KEY_NOT_FOUND: Consumer's API Key not found in config record.
          SPATULA_HEADER_INVALID: Consumer's spatula header is invalid.
          NAMESPACE_LOOKUP_UNAVAILABLE: The backend server for looking up project
            id/number is unavailable.
          SERVICE_STATUS_UNAVAILABLE: The backend server for checking service
            status is unavailable.
          BILLING_STATUS_UNAVAILABLE: The backend server for checking billing
            status is unavailable.
          QUOTA_CHECK_UNAVAILABLE: The quota check feature is temporarily
            unavailable:  Could be due to either internal config error or server
            error
        """
        ERROR_CODE_UNSPECIFIED = 0
        NOT_FOUND = 1
        PERMISSION_DENIED = 2
        RESOURCE_EXHAUSTED = 3
        BUDGET_EXCEEDED = 4
        DENIAL_OF_SERVICE_DETECTED = 5
        LOAD_SHEDDING = 6
        ABUSER_DETECTED = 7
        SERVICE_NOT_ACTIVATED = 8
        VISIBILITY_DENIED = 9
        BILLING_DISABLED = 10
        PROJECT_DELETED = 11
        PROJECT_INVALID = 12
        IP_ADDRESS_BLOCKED = 13
        REFERER_BLOCKED = 14
        CLIENT_APP_BLOCKED = 15
        API_KEY_INVALID = 16
        API_KEY_EXPIRED = 17
        API_KEY_NOT_FOUND = 18
        SPATULA_HEADER_INVALID = 19
        NAMESPACE_LOOKUP_UNAVAILABLE = 20
        SERVICE_STATUS_UNAVAILABLE = 21
        BILLING_STATUS_UNAVAILABLE = 22
        QUOTA_CHECK_UNAVAILABLE = 23

    code = _messages.EnumField('CodeValueValuesEnum', 1)
    detail = _messages.StringField(2)


class CheckRequest(_messages.Message):
    """The request message of the Check method.

    Fields:
      operation: The operation to be checked.
    """

    operation = _messages.MessageField('Operation', 1)


class CheckResponse(_messages.Message):
    """The response message of the Check method.

    Fields:
      checkErrors: Indicate the decision of the check.  If no check errors are
        present, the service should process the operation. Otherwise the service
        should use the list of errors to determine the appropriate action.
      operationId: The same operation_id value used in the CheckRequest. Used
        for logging and diagnostics purpose.
    """

    checkErrors = _messages.MessageField('CheckError', 1, repeated=True)
    operationId = _messages.StringField(2)


class Context(_messages.Message):
    """`Context` defines which contexts an API requests.  Example:      context:
    rules:       - selector: "*"         requested:         -
    google.rpc.context.ProjectContext         - google.rpc.context.OriginContext
    The above specifies that all methods in the API request
    `google.rpc.context.ProjectContext` and `google.rpc.context.OriginContext`.
    Available context types are defined in package `google.rpc.context`.

    Fields:
      rules: List of rules for context, applicable to methods.
    """

    rules = _messages.MessageField('ContextRule', 1, repeated=True)


class ContextRule(_messages.Message):
    """A context rule provides information about the context for an individual
    API element.

    Fields:
      provided: A list of full type names of provided contexts.
      requested: A list of full type names of requested contexts.
      selector: Selects the methods to which this rule applies.  Refer to
        selector for syntax details.
    """

    provided = _messages.StringField(1, repeated=True)
    requested = _messages.StringField(2, repeated=True)
    selector = _messages.StringField(3)


class Control(_messages.Message):
    """Selects and configures the service controller used by the service.  The
    service controller handles features like abuse, quota, billing, logging,
    monitoring, etc.

    Fields:
      environment: The service control environment to use. If empty, no control
        plane feature (like quota and billing) will be enabled.
    """

    environment = _messages.StringField(1)


class CustomError(_messages.Message):
    """Customize service error responses.  For example, list any service
    specific protobuf types that can appear in error detail lists of error
    responses.  Example:      custom_error:       types:       -
    google.foo.v1.CustomError       - google.foo.v1.AnotherError

    Fields:
      rules: The list of custom error rules to select to which messages this
        should apply.
      types: The list of custom error detail types, e.g.
        'google.foo.v1.CustomError'.
    """

    rules = _messages.MessageField('CustomErrorRule', 1, repeated=True)
    types = _messages.StringField(2, repeated=True)


class CustomErrorRule(_messages.Message):
    """A custom error rule.

    Fields:
      isErrorType: Mark this message as possible payload in error response.
        Otherwise, objects of this type will be filtered when they appear in
        error payload.
      selector: Selects messages to which this rule applies.  Refer to selector
        for syntax details.
    """

    isErrorType = _messages.BooleanField(1)
    selector = _messages.StringField(2)


class CustomHttpPattern(_messages.Message):
    """A custom pattern is used for defining custom HTTP verb.

    Fields:
      kind: The name of this custom HTTP verb.
      path: The path matched by this custom verb.
    """

    kind = _messages.StringField(1)
    path = _messages.StringField(2)


class Distribution(_messages.Message):
    """Distribution represents a frequency distribution of double-valued sample
    points. It contains the size of the population of sample points plus
    additional optional information:    - the arithmetic mean of the samples   -
    the minimum and maximum of the samples   - the sum-squared-deviation of the
    samples, used to compute variance   - a histogram of the values of the
    sample points

    Fields:
      bucketCounts: The number of samples in each histogram bucket.
        `bucket_counts` are optional. If present, they must sum to the `count`
        value.  The buckets are defined below in `bucket_option`. There are N
        buckets. `bucket_counts[0]` is the number of samples in the underflow
        bucket. `bucket_counts[1]` to `bucket_counts[N-1]` are the numbers of
        samples in each of the finite buckets. And `bucket_counts[N] is the
        number of samples in the overflow bucket. See the comments of
        `bucket_option` below for more details.  Any suffix of trailing zeros
        may be omitted.
      count: The total number of samples in the distribution. Must be non-
        negative.
      explicitBuckets: Buckets with arbitrary user-provided width.
      exponentialBuckets: Buckets with exponentially growing width.
      linearBuckets: Buckets with constant width.
      maximum: The maximum of the population of values. Ignored if `count` is
        zero.
      mean: The arithmetic mean of the samples in the distribution. If `count`
        is zero then this field must be zero, otherwise validation of the
        request fails.
      minimum: The minimum of the population of values. Ignored if `count` is
        zero.
      sumOfSquaredDeviation: The sum of squared deviations from the mean:
        Sum[i=1..count]((x_i - mean)^2) where each x_i is a sample values. If
        `count` is zero then this field must be zero, otherwise validation of
        the request fails.
    """

    bucketCounts = _messages.IntegerField(1, repeated=True)
    count = _messages.IntegerField(2)
    explicitBuckets = _messages.MessageField('ExplicitBuckets', 3)
    exponentialBuckets = _messages.MessageField('ExponentialBuckets', 4)
    linearBuckets = _messages.MessageField('LinearBuckets', 5)
    maximum = _messages.FloatField(6)
    mean = _messages.FloatField(7)
    minimum = _messages.FloatField(8)
    sumOfSquaredDeviation = _messages.FloatField(9)


class Documentation(_messages.Message):
    """`Documentation` provides the information for describing a service.
    Example: <pre><code>documentation:   summary: >     The Google Calendar API
    gives access     to most calendar features.   pages:   - name: Overview
    content: &#40;== include google/foo/overview.md ==&#41;   - name: Tutorial
    content: &#40;== include google/foo/tutorial.md ==&#41;     subpages;     -
    name: Java       content: &#40;== include google/foo/tutorial_java.md
    ==&#41;   rules:   - selector: google.calendar.Calendar.Get     description:
    >       ...   - selector: google.calendar.Calendar.Put     description: >
    ... </code></pre> Documentation is provided in markdown syntax. In addition
    to standard markdown features, definition lists, tables and fenced code
    blocks are supported. Section headers can be provided and are interpreted
    relative to the section nesting of the context where a documentation
    fragment is embedded.  Documentation from the IDL is merged with
    documentation defined via the config at normalization time, where
    documentation provided by config rules overrides IDL provided.  A number of
    constructs specific to the API platform are supported in documentation text.
    In order to reference a proto element, the following notation can be used:
    <pre><code>&#91;fully.qualified.proto.name]&#91;]</code></pre> To override
    the display text used for the link, this can be used:
    <pre><code>&#91;display text]&#91;fully.qualified.proto.name]</code></pre>
    Text can be excluded from doc using the following notation:
    <pre><code>&#40;-- internal comment --&#41;</code></pre> Comments can be
    made conditional using a visibility label. The below text will be only
    rendered if the `BETA` label is available: <pre><code>&#40;--BETA: comment
    for BETA users --&#41;</code></pre> A few directives are available in
    documentation. Note that directives must appear on a single line to be
    properly identified. The `include` directive includes a markdown file from
    an external source: <pre><code>&#40;== include path/to/file
    ==&#41;</code></pre> The `resource_for` directive marks a message to be the
    resource of a collection in REST view. If it is not specified, tools attempt
    to infer the resource from the operations in a collection:
    <pre><code>&#40;== resource_for v1.shelves.books ==&#41;</code></pre> The
    directive `suppress_warning` does not directly affect documentation and is
    documented together with service config validation.

    Fields:
      documentationRootUrl: The URL to the root of documentation.
      overview: Declares a single overview page. For example:
        <pre><code>documentation:   summary: ...   overview: &#40;== include
        overview.md ==&#41; </code></pre> This is a shortcut for the following
        declaration (using pages style): <pre><code>documentation:   summary:
        ...   pages:   - name: Overview     content: &#40;== include overview.md
        ==&#41; </code></pre> Note: you cannot specify both `overview` field and
        `pages` field.
      pages: The top level pages for the documentation set.
      rules: Documentation rules for individual elements of the service.
      summary: A short summary of what the service does. Can only be provided by
        plain text.
    """

    documentationRootUrl = _messages.StringField(1)
    overview = _messages.StringField(2)
    pages = _messages.MessageField('Page', 3, repeated=True)
    rules = _messages.MessageField('DocumentationRule', 4, repeated=True)
    summary = _messages.StringField(5)


class DocumentationRule(_messages.Message):
    """A documentation rule provides information about individual API elements.

    Fields:
      deprecationDescription: Deprecation description of the selected
        element(s). It can be provided if an element is marked as `deprecated`.
      description: Description of the selected API(s).
      selector: The selector is a comma-separated list of patterns. Each pattern
        is a qualified name of the element which may end in "*", indicating a
        wildcard. Wildcards are only allowed at the end and for a whole
        component of the qualified name, i.e. "foo.*" is ok, but not "foo.b*" or
        "foo.*.bar". To specify a default for all applicable elements, the whole
        pattern "*" is used.
    """

    deprecationDescription = _messages.StringField(1)
    description = _messages.StringField(2)
    selector = _messages.StringField(3)


class Enum(_messages.Message):
    """Enum type definition.

    Enums:
      SyntaxValueValuesEnum: The source syntax.

    Fields:
      enumvalue: Enum value definitions.
      name: Enum type name.
      options: Protocol buffer options.
      sourceContext: The source context.
      syntax: The source syntax.
    """

    class SyntaxValueValuesEnum(_messages.Enum):
        """The source syntax.

        Values:
          SYNTAX_PROTO2: Syntax `proto2`.
          SYNTAX_PROTO3: Syntax `proto3`.
        """
        SYNTAX_PROTO2 = 0
        SYNTAX_PROTO3 = 1

    enumvalue = _messages.MessageField('EnumValue', 1, repeated=True)
    name = _messages.StringField(2)
    options = _messages.MessageField('Option', 3, repeated=True)
    sourceContext = _messages.MessageField('SourceContext', 4)
    syntax = _messages.EnumField('SyntaxValueValuesEnum', 5)


class EnumValue(_messages.Message):
    """Enum value definition.

    Fields:
      name: Enum value name.
      number: Enum value number.
      options: Protocol buffer options.
    """

    name = _messages.StringField(1)
    number = _messages.IntegerField(2, variant=_messages.Variant.INT32)
    options = _messages.MessageField('Option', 3, repeated=True)


class ExplicitBuckets(_messages.Message):
    """Describing buckets with arbitrary user-provided width.

    Fields:
      bounds: 'bound' is a list of strictly increasing boundaries between
        buckets. Note that a list of length N-1 defines N buckets because of
        fenceposting. See comments on `bucket_options` for details.  The i'th
        finite bucket covers the interval   [bound[i-1], bound[i]) where i
        ranges from 1 to bound_size() - 1. Note that there are no finite buckets
        at all if 'bound' only contains a single element; in that special case
        the single bound defines the boundary between the underflow and overflow
        buckets.  bucket number                   lower bound    upper bound  i
        == 0 (underflow)              -inf           bound[i]  0 < i <
        bound_size()            bound[i-1]     bound[i]  i == bound_size()
        (overflow)    bound[i-1]     +inf
    """

    bounds = _messages.FloatField(1, repeated=True)


class ExponentialBuckets(_messages.Message):
    """Describing buckets with exponentially growing width.

    Fields:
      growthFactor: The i'th exponential bucket covers the interval   [scale *
        growth_factor^(i-1), scale * growth_factor^i) where i ranges from 1 to
        num_finite_buckets inclusive. Must be larger than 1.0.
      numFiniteBuckets: The number of finite buckets. With the underflow and
        overflow buckets, the total number of buckets is `num_finite_buckets` +
        2. See comments on `bucket_options` for details.
      scale: The i'th exponential bucket covers the interval   [scale *
        growth_factor^(i-1), scale * growth_factor^i) where i ranges from 1 to
        num_finite_buckets inclusive. Must be strictly positive.
    """

    growthFactor = _messages.FloatField(1)
    numFiniteBuckets = _messages.IntegerField(2, variant=_messages.Variant.INT32)
    scale = _messages.FloatField(3)


class Field(_messages.Message):
    """A single field of a message type.

    Enums:
      CardinalityValueValuesEnum: The field cardinality.
      KindValueValuesEnum: The field type.

    Fields:
      cardinality: The field cardinality.
      defaultValue: The string value of the default value of this field. Proto2
        syntax only.
      jsonName: The field JSON name.
      kind: The field type.
      name: The field name.
      number: The field number.
      oneofIndex: The index of the field type in `Type.oneofs`, for message or
        enumeration types. The first type has index 1; zero means the type is
        not in the list.
      options: The protocol buffer options.
      packed: Whether to use alternative packed wire representation.
      typeUrl: The field type URL, without the scheme, for message or
        enumeration types. Example:
        `"type.googleapis.com/google.protobuf.Timestamp"`.
    """

    class CardinalityValueValuesEnum(_messages.Enum):
        """The field cardinality.

        Values:
          CARDINALITY_UNKNOWN: For fields with unknown cardinality.
          CARDINALITY_OPTIONAL: For optional fields.
          CARDINALITY_REQUIRED: For required fields. Proto2 syntax only.
          CARDINALITY_REPEATED: For repeated fields.
        """
        CARDINALITY_UNKNOWN = 0
        CARDINALITY_OPTIONAL = 1
        CARDINALITY_REQUIRED = 2
        CARDINALITY_REPEATED = 3

    class KindValueValuesEnum(_messages.Enum):
        """The field type.

        Values:
          TYPE_UNKNOWN: Field type unknown.
          TYPE_DOUBLE: Field type double.
          TYPE_FLOAT: Field type float.
          TYPE_INT64: Field type int64.
          TYPE_UINT64: Field type uint64.
          TYPE_INT32: Field type int32.
          TYPE_FIXED64: Field type fixed64.
          TYPE_FIXED32: Field type fixed32.
          TYPE_BOOL: Field type bool.
          TYPE_STRING: Field type string.
          TYPE_GROUP: Field type group. Proto2 syntax only, and deprecated.
          TYPE_MESSAGE: Field type message.
          TYPE_BYTES: Field type bytes.
          TYPE_UINT32: Field type uint32.
          TYPE_ENUM: Field type enum.
          TYPE_SFIXED32: Field type sfixed32.
          TYPE_SFIXED64: Field type sfixed64.
          TYPE_SINT32: Field type sint32.
          TYPE_SINT64: Field type sint64.
        """
        TYPE_UNKNOWN = 0
        TYPE_DOUBLE = 1
        TYPE_FLOAT = 2
        TYPE_INT64 = 3
        TYPE_UINT64 = 4
        TYPE_INT32 = 5
        TYPE_FIXED64 = 6
        TYPE_FIXED32 = 7
        TYPE_BOOL = 8
        TYPE_STRING = 9
        TYPE_GROUP = 10
        TYPE_MESSAGE = 11
        TYPE_BYTES = 12
        TYPE_UINT32 = 13
        TYPE_ENUM = 14
        TYPE_SFIXED32 = 15
        TYPE_SFIXED64 = 16
        TYPE_SINT32 = 17
        TYPE_SINT64 = 18

    cardinality = _messages.EnumField('CardinalityValueValuesEnum', 1)
    defaultValue = _messages.StringField(2)
    jsonName = _messages.StringField(3)
    kind = _messages.EnumField('KindValueValuesEnum', 4)
    name = _messages.StringField(5)
    number = _messages.IntegerField(6, variant=_messages.Variant.INT32)
    oneofIndex = _messages.IntegerField(7, variant=_messages.Variant.INT32)
    options = _messages.MessageField('Option', 8, repeated=True)
    packed = _messages.BooleanField(9)
    typeUrl = _messages.StringField(10)


class Http(_messages.Message):
    """Defines the HTTP configuration for a service. It contains a list of
    HttpRule, each specifying the mapping of an RPC method to one or more HTTP
    REST API methods.

    Fields:
      rules: A list of HTTP rules for configuring the HTTP REST API methods.
    """

    rules = _messages.MessageField('HttpRule', 1, repeated=True)


class HttpRequest(_messages.Message):
    """A common proto for logging HTTP requests.

    Fields:
      cacheFillBytes: The number of HTTP response bytes inserted into cache. Set
        only when a cache fill was attempted.
      cacheHit: Whether or not an entity was served from cache (with or without
        validation).
      cacheLookup: Whether or not a cache lookup was attempted.
      cacheValidatedWithOriginServer: Whether or not the response was validated
        with the origin server before being served from cache. This field is
        only meaningful if `cache_hit` is True.
      referer: The referer URL of the request, as defined in [HTTP/1.1 Header
        Field
        Definitions](http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html).
      remoteIp: The IP address (IPv4 or IPv6) of the client that issued the HTTP
        request. Examples: `"192.168.1.1"`, `"FE80::0202:B3FF:FE1E:8329"`.
      requestMethod: The request method. Examples: `"GET"`, `"HEAD"`, `"PUT"`,
        `"POST"`.
      requestSize: The size of the HTTP request message in bytes, including the
        request headers and the request body.
      requestUrl: The scheme (http, https), the host name, the path and the
        query portion of the URL that was requested. Example:
        `"http://example.com/some/info?color=red"`.
      responseSize: The size of the HTTP response message sent back to the
        client, in bytes, including the response headers and the response body.
      serverIp: The IP address (IPv4 or IPv6) of the origin server that the
        request was sent to.
      status: The response code indicating the status of response. Examples:
        200, 404.
      userAgent: The user agent sent by the client. Example: `"Mozilla/4.0
        (compatible; MSIE 6.0; Windows 98; Q312461; .NET CLR 1.0.3705)"`.
    """

    cacheFillBytes = _messages.IntegerField(1)
    cacheHit = _messages.BooleanField(2)
    cacheLookup = _messages.BooleanField(3)
    cacheValidatedWithOriginServer = _messages.BooleanField(4)
    referer = _messages.StringField(5)
    remoteIp = _messages.StringField(6)
    requestMethod = _messages.StringField(7)
    requestSize = _messages.IntegerField(8)
    requestUrl = _messages.StringField(9)
    responseSize = _messages.IntegerField(10)
    serverIp = _messages.StringField(11)
    status = _messages.IntegerField(12, variant=_messages.Variant.INT32)
    userAgent = _messages.StringField(13)


class HttpRule(_messages.Message):
    """`HttpRule` defines the mapping of an RPC method to one or more HTTP REST
    APIs.  The mapping determines what portions of the request message are
    populated from the path, query parameters, or body of the HTTP request.  The
    mapping is typically specified as an `google.api.http` annotation, see
    "google/api/annotations.proto" for details.  The mapping consists of a field
    specifying the path template and method kind.  The path template can refer
    to fields in the request message, as in the example below which describes a
    REST GET operation on a resource collection of messages:  ```proto service
    Messaging {   rpc GetMessage(GetMessageRequest) returns (Message) {
    option (google.api.http).get = "/v1/messages/{message_id}/{sub.subfield}";
    } } message GetMessageRequest {   message SubMessage {     string subfield =
    1;   }   string message_id = 1; // mapped to the URL   SubMessage sub = 2;
    // `sub.subfield` is url-mapped } message Message {   string text = 1; //
    content of the resource } ```  This definition enables an automatic,
    bidrectional mapping of HTTP JSON to RPC. Example:  HTTP | RPC -----|-----
    `GET /v1/messages/123456/foo`  | `GetMessage(message_id: "123456" sub:
    SubMessage(subfield: "foo"))`  In general, not only fields but also field
    paths can be referenced from a path pattern. Fields mapped to the path
    pattern cannot be repeated and must have a primitive (non-message) type.
    Any fields in the request message which are not bound by the path pattern
    automatically become (optional) HTTP query parameters. Assume the following
    definition of the request message:  ```proto message GetMessageRequest {
    message SubMessage {     string subfield = 1;   }   string message_id = 1;
    // mapped to the URL   int64 revision = 2;    // becomes a parameter
    SubMessage sub = 3;    // `sub.subfield` becomes a parameter } ```  This
    enables a HTTP JSON to RPC mapping as below:  HTTP | RPC -----|----- `GET
    /v1/messages/123456?revision=2&sub.subfield=foo` | `GetMessage(message_id:
    "123456" revision: 2 sub: SubMessage(subfield: "foo"))`  Note that fields
    which are mapped to HTTP parameters must have a primitive type or a repeated
    primitive type. Message types are not allowed. In the case of a repeated
    type, the parameter can be repeated in the URL, as in `...?param=A&param=B`.
    For HTTP method kinds which allow a request body, the `body` field specifies
    the mapping. Consider a REST update method on the message resource
    collection:  ```proto service Messaging {   rpc
    UpdateMessage(UpdateMessageRequest) returns (Message) {     option
    (google.api.http) = {       put: "/v1/messages/{message_id}"       body:
    "message"     };   } } message UpdateMessageRequest {   string message_id =
    1; // mapped to the URL   Message message = 2;   // mapped to the body } ```
    The following HTTP JSON to RPC mapping is enabled, where the representation
    of the JSON in the request body is determined by protos JSON encoding:  HTTP
    | RPC -----|----- `PUT /v1/messages/123456 { "text": "Hi!" }` |
    `UpdateMessage(message_id: "123456" message { text: "Hi!" })`  The special
    name `*` can be used in the body mapping to define that every field not
    bound by the path template should be mapped to the request body.  This
    enables the following alternative definition of the update method:  ```proto
    service Messaging {   rpc UpdateMessage(Message) returns (Message) {
    option (google.api.http) = {       put: "/v1/messages/{message_id}"
    body: "*"     };   } } message Message {   string message_id = 1;   string
    text = 2; } ```  The following HTTP JSON to RPC mapping is enabled:  HTTP |
    RPC -----|----- `PUT /v1/messages/123456 { "text": "Hi!" }` |
    `UpdateMessage(message_id: "123456" text: "Hi!")`  Note that when using `*`
    in the body mapping, it is not possible to have HTTP parameters, as all
    fields not bound by the path end in the body. This makes this option more
    rarely used in practice of defining REST APIs. The common usage of `*` is in
    custom methods which don't use the URL at all for transferring data.  It is
    possible to define multiple HTTP methods for one RPC by using the
    `additional_bindings` option. Example:  ```proto service Messaging {   rpc
    GetMessage(GetMessageRequest) returns (Message) {     option
    (google.api.http) = {       get: "/v1/messages/{message_id}"
    additional_bindings {         get:
    "/v1/users/{user_id}/messages/{message_id}"       }     };   } } message
    GetMessageRequest {   string message_id = 1;   string user_id = 2; } ```
    This enables the following two alternative HTTP JSON to RPC mappings:  HTTP
    | RPC -----|----- `GET /v1/messages/123456` | `GetMessage(message_id:
    "123456")` `GET /v1/users/me/messages/123456` | `GetMessage(user_id: "me"
    message_id: "123456")`  # Rules for HTTP mapping  The rules for mapping HTTP
    path, query parameters, and body fields to the request message are as
    follows:  1. The `body` field specifies either `*` or a field path, or is
    omitted. If omitted, it assumes there is no HTTP body. 2. Leaf fields
    (recursive expansion of nested messages in the    request) can be classified
    into three types:     (a) Matched in the URL template.     (b) Covered by
    body (if body is `*`, everything except (a) fields;         else everything
    under the body field)     (c) All other fields. 3. URL query parameters
    found in the HTTP request are mapped to (c) fields. 4. Any body sent with an
    HTTP request can contain only (b) fields.  The syntax of the path template
    is as follows:      Template = "/" Segments [ Verb ] ;     Segments =
    Segment { "/" Segment } ;     Segment  = "*" | "**" | LITERAL | Variable ;
    Variable = "{" FieldPath [ "=" Segments ] "}" ;     FieldPath = IDENT { "."
    IDENT } ;     Verb     = ":" LITERAL ;  The syntax `*` matches a single path
    segment. It follows the semantics of [RFC
    6570](https://tools.ietf.org/html/rfc6570) Section 3.2.2 Simple String
    Expansion.  The syntax `**` matches zero or more path segments. It follows
    the semantics of [RFC 6570](https://tools.ietf.org/html/rfc6570) Section
    3.2.3 Reserved Expansion.  The syntax `LITERAL` matches literal text in the
    URL path.  The syntax `Variable` matches the entire path as specified by its
    template; this nested template must not contain further variables. If a
    variable matches a single path segment, its template may be omitted, e.g.
    `{var}` is equivalent to `{var=*}`.  NOTE: the field paths in variables and
    in the `body` must not refer to repeated fields or map fields.  Use
    CustomHttpPattern to specify any HTTP method that is not included in the
    `pattern` field, such as HEAD, or "*" to leave the HTTP method unspecified
    for a given URL path rule. The wild-card rule is useful for services that
    provide content to Web (HTML) clients.

    Fields:
      additionalBindings: Additional HTTP bindings for the selector. Nested
        bindings must not contain an `additional_bindings` field themselves
        (that is, the nesting may only be one level deep).
      body: The name of the request field whose value is mapped to the HTTP
        body, or `*` for mapping all fields not captured by the path pattern to
        the HTTP body. NOTE: the referred field must not be a repeated field.
      custom: Custom pattern is used for defining custom verbs.
      delete: Used for deleting a resource.
      get: Used for listing and getting information about resources.
      mediaDownload: Do not use this. For media support, add instead
        [][google.bytestream.RestByteStream] as an API to your configuration.
      mediaUpload: Do not use this. For media support, add instead
        [][google.bytestream.RestByteStream] as an API to your configuration.
      patch: Used for updating a resource.
      post: Used for creating a resource.
      put: Used for updating a resource.
      selector: Selects methods to which this rule applies.  Refer to selector
        for syntax details.
    """

    additionalBindings = _messages.MessageField('HttpRule', 1, repeated=True)
    body = _messages.StringField(2)
    custom = _messages.MessageField('CustomHttpPattern', 3)
    delete = _messages.StringField(4)
    get = _messages.StringField(5)
    mediaDownload = _messages.MessageField('MediaDownload', 6)
    mediaUpload = _messages.MessageField('MediaUpload', 7)
    patch = _messages.StringField(8)
    post = _messages.StringField(9)
    put = _messages.StringField(10)
    selector = _messages.StringField(11)


class LabelDescriptor(_messages.Message):
    """A description of a label.

    Enums:
      ValueTypeValueValuesEnum: The type of data that can be assigned to the
        label.

    Fields:
      description: A human-readable description for the label.
      key: The label key.
      valueType: The type of data that can be assigned to the label.
    """

    class ValueTypeValueValuesEnum(_messages.Enum):
        """The type of data that can be assigned to the label.

        Values:
          STRING: A variable-length string. This is the default.
          BOOL: Boolean; true or false.
          INT64: A 64-bit signed integer.
        """
        STRING = 0
        BOOL = 1
        INT64 = 2

    description = _messages.StringField(1)
    key = _messages.StringField(2)
    valueType = _messages.EnumField('ValueTypeValueValuesEnum', 3)


class LinearBuckets(_messages.Message):
    """Describing buckets with constant width.

    Fields:
      numFiniteBuckets: The number of finite buckets. With the underflow and
        overflow buckets, the total number of buckets is `num_finite_buckets` +
        2. See comments on `bucket_options` for details.
      offset: The i'th linear bucket covers the interval   [offset + (i-1) *
        width, offset + i * width) where i ranges from 1 to num_finite_buckets,
        inclusive.
      width: The i'th linear bucket covers the interval   [offset + (i-1) *
        width, offset + i * width) where i ranges from 1 to num_finite_buckets,
        inclusive. Must be strictly positive.
    """

    numFiniteBuckets = _messages.IntegerField(1, variant=_messages.Variant.INT32)
    offset = _messages.FloatField(2)
    width = _messages.FloatField(3)


class LogDescriptor(_messages.Message):
    """A description of a log type. Example in YAML format:      - name:
    library.googleapis.com/activity_history       description: The history of
    borrowing and returning library items.       display_name: Activity
    labels:       - key: /customer_id         description: Identifier of a
    library customer

    Fields:
      description: A human-readable description of this log. This information
        appears in the documentation and can contain details.
      displayName: The human-readable name for this log. This information
        appears on the user interface and should be concise.
      labels: The set of labels that are available to describe a specific log
        entry. Runtime requests that contain labels not specified here are
        considered invalid.
      name: The name of the log. It must be less than 512 characters long and
        can include the following characters: upper- and lower-case alphanumeric
        characters [A-Za-z0-9], and punctuation characters including slash,
        underscore, hyphen, period [/_-.].
    """

    description = _messages.StringField(1)
    displayName = _messages.StringField(2)
    labels = _messages.MessageField('LabelDescriptor', 3, repeated=True)
    name = _messages.StringField(4)


class LogEntry(_messages.Message):
    """An individual log entry.

    Enums:
      SeverityValueValuesEnum: Optional. The severity of the log entry. The
        default value is `LogSeverity.DEFAULT`.

    Messages:
      LabelsValue: Optional. A set of user-defined (key, value) data that
        provides additional information about the log entry.
      ProtoPayloadValue: The log entry payload, represented as a protocol buffer
        that is expressed as a JSON object. You can only pass `protoPayload`
        values that belong to a set of approved types.
      StructPayloadValue: The log entry payload, represented as a structure that
        is expressed as a JSON object.

    Fields:
      httpRequest: Information about the HTTP request associated with this log
        entry, if applicable.  Deprecated. Please use fields 10-13.
      insertId: A unique ID for the log entry used for deduplication. If
        omitted, the implementation will generate one based on operation_id.
      labels: Optional. A set of user-defined (key, value) data that provides
        additional information about the log entry.
      log: The log to which this entry belongs. When a log entry is written, the
        value of this field is set by the logging system.  Deprecated. Please
        use fields 10-13.
      metadata: Information about the log entry.  Deprecated. Please use fields
        10-13.
      name: Required. The log to which this log entry belongs. Examples:
        `"syslog"`, `"book_log"`.
      operation: Optional. Information about an operation associated with the
        log entry, if applicable.  Deprecated. Please use fields 10-13.
      protoPayload: The log entry payload, represented as a protocol buffer that
        is expressed as a JSON object. You can only pass `protoPayload` values
        that belong to a set of approved types.
      severity: Optional. The severity of the log entry. The default value is
        `LogSeverity.DEFAULT`.
      structPayload: The log entry payload, represented as a structure that is
        expressed as a JSON object.
      textPayload: The log entry payload, represented as a Unicode string
        (UTF-8).
      timestamp: Optional. The time the event described by the log entry
        occurred. If omitted, defaults to operation start time.
    """

    class SeverityValueValuesEnum(_messages.Enum):
        """Optional. The severity of the log entry. The default value is
      `LogSeverity.DEFAULT`.

      Values:
        DEFAULT: The log entry has no assigned severity level.
        DEBUG: Debug or trace information.
        INFO: Routine information, such as ongoing status or performance.
        NOTICE: Normal but significant events, such as start up, shut down, or
          configuration.
        WARNING: Warning events might cause problems.
        ERROR: Error events are likely to cause problems.
        CRITICAL: Critical events cause more severe problems or brief outages.
        ALERT: A person must take an action immediately.
        EMERGENCY: One or more systems are unusable.
      """
        DEFAULT = 0
        DEBUG = 1
        INFO = 2
        NOTICE = 3
        WARNING = 4
        ERROR = 5
        CRITICAL = 6
        ALERT = 7
        EMERGENCY = 8

    @encoding.MapUnrecognizedFields('additionalProperties')
    class LabelsValue(_messages.Message):
        """Optional. A set of user-defined (key, value) data that provides
        additional information about the log entry.

        Messages:
          AdditionalProperty: An additional property for a LabelsValue object.

        Fields:
          additionalProperties: Additional properties of type LabelsValue
        """

        class AdditionalProperty(_messages.Message):
            """An additional property for a LabelsValue object.

            Fields:
              key: Name of the additional property.
              value: A string attribute.
            """

            key = _messages.StringField(1)
            value = _messages.StringField(2)

        additionalProperties = _messages.MessageField('AdditionalProperty', 1, repeated=True)

    @encoding.MapUnrecognizedFields('additionalProperties')
    class ProtoPayloadValue(_messages.Message):
        """The log entry payload, represented as a protocol buffer that is
        expressed as a JSON object. You can only pass `protoPayload` values that
        belong to a set of approved types.

        Messages:
          AdditionalProperty: An additional property for a ProtoPayloadValue
            object.

        Fields:
          additionalProperties: Properties of the object. Contains field @ype with
            type URL.
        """

        class AdditionalProperty(_messages.Message):
            """An additional property for a ProtoPayloadValue object.

            Fields:
              key: Name of the additional property.
              value: A extra_types.JsonValue attribute.
            """

            key = _messages.StringField(1)
            value = _messages.MessageField('extra_types.JsonValue', 2)

        additionalProperties = _messages.MessageField('AdditionalProperty', 1, repeated=True)

    @encoding.MapUnrecognizedFields('additionalProperties')
    class StructPayloadValue(_messages.Message):
        """The log entry payload, represented as a structure that is expressed as
        a JSON object.

        Messages:
          AdditionalProperty: An additional property for a StructPayloadValue
            object.

        Fields:
          additionalProperties: Properties of the object.
        """

        class AdditionalProperty(_messages.Message):
            """An additional property for a StructPayloadValue object.

            Fields:
              key: Name of the additional property.
             value: A extra_types.JsonValue attribute.
            """

            key = _messages.StringField(1)
            value = _messages.MessageField('extra_types.JsonValue', 2)

        additionalProperties = _messages.MessageField('AdditionalProperty', 1, repeated=True)

    httpRequest = _messages.MessageField('HttpRequest', 1)
    insertId = _messages.StringField(2)
    labels = _messages.MessageField('LabelsValue', 3)
    log = _messages.StringField(4)
    metadata = _messages.MessageField('LogEntryMetadata', 5)
    name = _messages.StringField(6)
    operation = _messages.MessageField('LogEntryOperation', 7)
    protoPayload = _messages.MessageField('ProtoPayloadValue', 8)
    severity = _messages.EnumField('SeverityValueValuesEnum', 9)
    structPayload = _messages.MessageField('StructPayloadValue', 10)
    textPayload = _messages.StringField(11)
    timestamp = _messages.StringField(12)


class LogEntryMetadata(_messages.Message):
    """Additional data that is associated with a log entry, set by the service
    creating the log entry.

    Enums:
      SeverityValueValuesEnum: The severity of the log entry. If omitted,
        `LogSeverity.DEFAULT` is used.

    Messages:
      LabelsValue: A set of (key, value) data that provides additional
        information about the log entry. If the log entry is from one of the
        Google Cloud Platform sources listed below, the indicated (key, value)
        information must be provided:  Google App Engine, service_name
        `appengine.googleapis.com`:        "appengine.googleapis.com/module_id",
        <module ID>       "appengine.googleapis.com/version_id", <version ID>
        and one of:       "appengine.googleapis.com/replica_index", <instance
        index>       "appengine.googleapis.com/clone_id", <instance ID>      or
        else provide the following Compute Engine labels:  Google Compute
        Engine, service_name `compute.googleapis.com`:
        "compute.googleapis.com/resource_type", "instance"
        "compute.googleapis.com/resource_id", <instance ID>

    Fields:
      labels: A set of (key, value) data that provides additional information
        about the log entry. If the log entry is from one of the Google Cloud
        Platform sources listed below, the indicated (key, value) information
        must be provided:  Google App Engine, service_name
        `appengine.googleapis.com`:        "appengine.googleapis.com/module_id",
        <module ID>       "appengine.googleapis.com/version_id", <version ID>
        and one of:       "appengine.googleapis.com/replica_index", <instance
        index>       "appengine.googleapis.com/clone_id", <instance ID>      or
        else provide the following Compute Engine labels:  Google Compute
        Engine, service_name `compute.googleapis.com`:
        "compute.googleapis.com/resource_type", "instance"
        "compute.googleapis.com/resource_id", <instance ID>
      projectId: The project ID of the Google Cloud Platform service that
        created the log entry.
      region: The region name of the Google Cloud Platform service that created
        the log entry.  For example, `"us-central1"`.
      serviceName: Required. The API name of the Google Cloud Platform service
        that created the log entry.  For example, `"compute.googleapis.com"`.
      severity: The severity of the log entry. If omitted, `LogSeverity.DEFAULT`
        is used.
      timestamp: The time the event described by the log entry occurred.
        Timestamps must be later than January 1, 1970.  If omitted, Stackdriver
        Logging will use the time the log entry is received.
      userId: The fully-qualified email address of the authenticated user that
        performed or requested the action represented by the log entry. If the
        log entry does not apply to an action taken by an authenticated user,
        then the field should be empty.
      zone: The zone of the Google Cloud Platform service that created the log
        entry. For example, `"us-central1-a"`.
    """

    class SeverityValueValuesEnum(_messages.Enum):
        """The severity of the log entry. If omitted, `LogSeverity.DEFAULT` is
        used.

        Values:
          DEFAULT: The log entry has no assigned severity level.
          DEBUG: Debug or trace information.
          INFO: Routine information, such as ongoing status or performance.
          NOTICE: Normal but significant events, such as start up, shut down, or
            configuration.
          WARNING: Warning events might cause problems.
          ERROR: Error events are likely to cause problems.
          CRITICAL: Critical events cause more severe problems or brief outages.
          ALERT: A person must take an action immediately.
          EMERGENCY: One or more systems are unusable.
        """
        DEFAULT = 0
        DEBUG = 1
        INFO = 2
        NOTICE = 3
        WARNING = 4
        ERROR = 5
        CRITICAL = 6
        ALERT = 7
        EMERGENCY = 8

    @encoding.MapUnrecognizedFields('additionalProperties')
    class LabelsValue(_messages.Message):
        """A set of (key, value) data that provides additional information about
        the log entry. If the log entry is from one of the Google Cloud Platform
        sources listed below, the indicated (key, value) information must be
        provided:  Google App Engine, service_name `appengine.googleapis.com`:
        "appengine.googleapis.com/module_id", <module ID>
        "appengine.googleapis.com/version_id", <version ID>           and one of:
        "appengine.googleapis.com/replica_index", <instance index>
        "appengine.googleapis.com/clone_id", <instance ID>      or else provide
        the following Compute Engine labels:  Google Compute Engine, service_name
        `compute.googleapis.com`:         "compute.googleapis.com/resource_type",
        "instance"        "compute.googleapis.com/resource_id", <instance ID>

        Messages:
          AdditionalProperty: An additional property for a LabelsValue object.

        Fields:
          additionalProperties: Additional properties of type LabelsValue
        """

        class AdditionalProperty(_messages.Message):
            """An additional property for a LabelsValue object.

            Fields:
              key: Name of the additional property.
              value: A string attribute.
            """

            key = _messages.StringField(1)
            value = _messages.StringField(2)

        additionalProperties = _messages.MessageField('AdditionalProperty', 1, repeated=True)

    labels = _messages.MessageField('LabelsValue', 1)
    projectId = _messages.StringField(2)
    region = _messages.StringField(3)
    serviceName = _messages.StringField(4)
    severity = _messages.EnumField('SeverityValueValuesEnum', 5)
    timestamp = _messages.StringField(6)
    userId = _messages.StringField(7)
    zone = _messages.StringField(8)


class LogEntryOperation(_messages.Message):
    """Additional information about a potentially long running operation with
    which a log entry is associated.

    Fields:
      first: True for the first entry associated with `id`.
      id: An opaque identifier. A producer of log entries should ensure that
        `id` is only reused for entries related to one operation.
      last: True for the last entry associated with `id`.
      producer: Ensures the operation can be uniquely identified. The
        combination of `id` and `producer` should be made globally unique by
        filling `producer` with a value that disambiguates the service that
        created `id`.
    """

    first = _messages.BooleanField(1)
    id = _messages.StringField(2)
    last = _messages.BooleanField(3)
    producer = _messages.StringField(4)


class Logging(_messages.Message):
    """Logging configuration of the service.  The following example shows how to
    configure logs to be sent to the producer and consumer projects. In the
    example, the `library.googleapis.com/activity_history` log is sent to both
    the producer and consumer projects, whereas the
    `library.googleapis.com/purchase_history` log is only sent to the producer
    project:      monitored_resources:     - type: library.googleapis.com/branch
    labels:       - key: /city         description: The city where the library
    branch is located in.       - key: /name         description: The name of
    the branch.     logs:     - name: library.googleapis.com/activity_history
    labels:       - key: /customer_id     - name:
    library.googleapis.com/purchase_history     logging:
    producer_destinations:       - monitored_resource:
    library.googleapis.com/branch         logs:         -
    library.googleapis.com/activity_history         -
    library.googleapis.com/purchase_history       consumer_destinations:       -
    monitored_resource: library.googleapis.com/branch         logs:         -
    library.googleapis.com/activity_history

    Fields:
      consumerDestinations: Logging configurations for sending logs to the
        consumer project. There can be multiple consumer destinations, each one
        must have a different monitored resource type. A log can be used in at
        most one consumer destination.
      producerDestinations: Logging configurations for sending logs to the
        producer project. There can be multiple producer destinations, each one
        must have a different monitored resource type. A log can be used in at
        most one producer destination.
    """

    consumerDestinations = _messages.MessageField('LoggingDestination', 1, repeated=True)
    producerDestinations = _messages.MessageField('LoggingDestination', 2, repeated=True)


class LoggingDestination(_messages.Message):
    """Configuration of a specific logging destination (the producer project or
    the consumer project).

    Fields:
      logs: Names of the logs to be sent to this destination. Each name must be
        defined in the Service.logs section.
      monitoredResource: The monitored resource type. The type must be defined
        in Service.monitored_resources section.
    """

    logs = _messages.StringField(1, repeated=True)
    monitoredResource = _messages.StringField(2)


class MediaDownload(_messages.Message):
    """Do not use this. For media support, add instead
    [][google.bytestream.RestByteStream] as an API to your configuration.

    Fields:
      enabled: Whether download is enabled.
    """

    enabled = _messages.BooleanField(1)


class MediaUpload(_messages.Message):
    """Do not use this. For media support, add instead
    [][google.bytestream.RestByteStream] as an API to your configuration.

    Fields:
      enabled: Whether upload is enabled.
    """

    enabled = _messages.BooleanField(1)


class Method(_messages.Message):
    """Method represents a method of an api.

    Enums:
      SyntaxValueValuesEnum: The source syntax of this method.

    Fields:
      name: The simple name of this method.
      options: Any metadata attached to the method.
      requestStreaming: If true, the request is streamed.
      requestTypeUrl: A URL of the input message type.
      responseStreaming: If true, the response is streamed.
      responseTypeUrl: The URL of the output message type.
      syntax: The source syntax of this method.
    """

    class SyntaxValueValuesEnum(_messages.Enum):
        """The source syntax of this method.

      Values:
        SYNTAX_PROTO2: Syntax `proto2`.
        SYNTAX_PROTO3: Syntax `proto3`.
      """
        SYNTAX_PROTO2 = 0
        SYNTAX_PROTO3 = 1

    name = _messages.StringField(1)
    options = _messages.MessageField('Option', 2, repeated=True)
    requestStreaming = _messages.BooleanField(3)
    requestTypeUrl = _messages.StringField(4)
    responseStreaming = _messages.BooleanField(5)
    responseTypeUrl = _messages.StringField(6)
    syntax = _messages.EnumField('SyntaxValueValuesEnum', 7)


class MetricDescriptor(_messages.Message):
    """Defines a metric type and its schema.

    Enums:
      MetricKindValueValuesEnum: Whether the metric records instantaneous
        values, changes to a value, etc.
      ValueTypeValueValuesEnum: Whether the measurement is an integer, a
        floating-point number, etc.

    Fields:
      description: A detailed description of the metric, which can be used in
        documentation.
      displayName: A concise name for the metric, which can be displayed in user
        interfaces. Use sentence case without an ending period, for example
        "Request count".
      labels: The set of labels that can be used to describe a specific instance
        of this metric type. For example, the
        `compute.googleapis.com/instance/network/received_bytes_count` metric
        type has a label, `loadbalanced`, that specifies whether the traffic was
        received through a load balanced IP address.
      metricKind: Whether the metric records instantaneous values, changes to a
        value, etc.
      name: Resource name. The format of the name may vary between different
        implementations. For examples:
        projects/{project_id}/metricDescriptors/{type=**}
        metricDescriptors/{type=**}
      type: The metric type including a DNS name prefix, for example
        `"compute.googleapis.com/instance/cpu/utilization"`. Metric types should
        use a natural hierarchical grouping such as the following:
        compute.googleapis.com/instance/cpu/utilization
        compute.googleapis.com/instance/disk/read_ops_count
        compute.googleapis.com/instance/network/received_bytes_count  Note that
        if the metric type changes, the monitoring data will be discontinued,
        and anything depends on it will break, such as monitoring dashboards,
        alerting rules and quota limits. Therefore, once a metric has been
        published, its type should be immutable.
      unit: The unit in which the metric value is reported. It is only
        applicable if the `value_type` is `INT64`, `DOUBLE`, or `DISTRIBUTION`.
        The supported units are a subset of [The Unified Code for Units of
        Measure](http://unitsofmeasure.org/ucum.html) standard:  **Basic units
        (UNIT)**  * `bit`   bit * `By`    byte * `s`     second * `min`   minute
        * `h`     hour * `d`     day  **Prefixes (PREFIX)**  * `k`     kilo
        (10**3) * `M`     mega    (10**6) * `G`     giga    (10**9) * `T`
        tera    (10**12) * `P`     peta    (10**15) * `E`     exa     (10**18) *
        `Z`     zetta   (10**21) * `Y`     yotta   (10**24) * `m`     milli
        (10**-3) * `u`     micro   (10**-6) * `n`     nano    (10**-9) * `p`
        pico    (10**-12) * `f`     femto   (10**-15) * `a`     atto
        (10**-18) * `z`     zepto   (10**-21) * `y`     yocto   (10**-24) * `Ki`
        kibi    (2**10) * `Mi`    mebi    (2**20) * `Gi`    gibi    (2**30) *
        `Ti`    tebi    (2**40)  **Grammar**  The grammar includes the
        dimensionless unit `1`, such as `1/s`.  The grammar also includes these
        connectors:  * `/`    division (as an infix operator, e.g. `1/s`). * `.`
        multiplication (as an infix operator, e.g. `GBy.d`)  The grammar for a
        unit is as follows:      Expression = Component { "." Component } { "/"
        Component } ;      Component = [ PREFIX ] UNIT [ Annotation ]
        | Annotation               | "1"               ;      Annotation = "{"
        NAME "}" ;  Notes:  * `Annotation` is just a comment if it follows a
        `UNIT` and is    equivalent to `1` if it is used alone. For examples,
        `{requests}/s == 1/s`, `By{transmitted}/s == By/s`. * `NAME` is a
        sequence of non-blank printable ASCII characters not    containing '{'
        or '}'.
      valueType: Whether the measurement is an integer, a floating-point number,
        etc.
    """

    class MetricKindValueValuesEnum(_messages.Enum):
        """Whether the metric records instantaneous values, changes to a value,
        etc.

        Values:
          METRIC_KIND_UNSPECIFIED: Do not use this default value.
          GAUGE: Instantaneous measurements of a varying quantity.
          DELTA: Changes over non-overlapping time intervals.
          CUMULATIVE: Cumulative value over time intervals that can overlap. The
            overlapping intervals must have the same start time.
        """
        METRIC_KIND_UNSPECIFIED = 0
        GAUGE = 1
        DELTA = 2
        CUMULATIVE = 3

    class ValueTypeValueValuesEnum(_messages.Enum):
        """Whether the measurement is an integer, a floating-point number, etc.

      Values:
        VALUE_TYPE_UNSPECIFIED: Do not use this default value.
        BOOL: The value is a boolean. This value type can be used only if the
          metric kind is `GAUGE`.
        INT64: The value is a signed 64-bit integer.
        DOUBLE: The value is a double precision floating point number.
        STRING: The value is a text string. This value type can be used only if
          the metric kind is `GAUGE`.
        DISTRIBUTION: The value is a `Distribution`.
        MONEY: The value is money.
      """
        VALUE_TYPE_UNSPECIFIED = 0
        BOOL = 1
        INT64 = 2
        DOUBLE = 3
        STRING = 4
        DISTRIBUTION = 5
        MONEY = 6

    description = _messages.StringField(1)
    displayName = _messages.StringField(2)
    labels = _messages.MessageField('LabelDescriptor', 3, repeated=True)
    metricKind = _messages.EnumField('MetricKindValueValuesEnum', 4)
    name = _messages.StringField(5)
    type = _messages.StringField(6)
    unit = _messages.StringField(7)
    valueType = _messages.EnumField('ValueTypeValueValuesEnum', 8)


class MetricValue(_messages.Message):
    """Represents a single metric value.

    Messages:
      LabelsValue: The labels describing the metric value. See comments on
        Operation.labels for the overriding relationship.

    Fields:
      boolValue: A boolean value.
      distributionValue: A distribution value.
      doubleValue: A double precision floating point value.
      endTime: The end of the time period over which this metric value's
        measurement applies.
      int64Value: A signed 64-bit integer value.
      labels: The labels describing the metric value. See comments on
        Operation.labels for the overriding relationship.
      moneyValue: A money value.
      startTime: The start of the time period over which this metric value's
        measurement applies. The time period has different semantics for
        different metric types (cumulative, delta, and gauge). See the metric
        definition documentation in the service configuration for details.
      stringValue: A text string value.
    """

    @encoding.MapUnrecognizedFields('additionalProperties')
    class LabelsValue(_messages.Message):
        """The labels describing the metric value. See comments on
        Operation.labels for the overriding relationship.

        Messages:
          AdditionalProperty: An additional property for a LabelsValue object.

        Fields:
          additionalProperties: Additional properties of type LabelsValue
        """

        class AdditionalProperty(_messages.Message):
            """An additional property for a LabelsValue object.

            Fields:
              key: Name of the additional property.
              value: A string attribute.
            """

            key = _messages.StringField(1)
            value = _messages.StringField(2)

        additionalProperties = _messages.MessageField('AdditionalProperty', 1, repeated=True)

    boolValue = _messages.BooleanField(1)
    distributionValue = _messages.MessageField('Distribution', 2)
    doubleValue = _messages.FloatField(3)
    endTime = _messages.StringField(4)
    int64Value = _messages.IntegerField(5)
    labels = _messages.MessageField('LabelsValue', 6)
    moneyValue = _messages.MessageField('Money', 7)
    startTime = _messages.StringField(8)
    stringValue = _messages.StringField(9)


class MetricValueSet(_messages.Message):
    """Represents a set of metric values in the same metric. Each metric value
    in the set should have a unique combination of start time, end time, and
    label values.

    Fields:
      metricName: The metric name defined in the service configuration.
      metricValues: The values in this metric.
    """

    metricName = _messages.StringField(1)
    metricValues = _messages.MessageField('MetricValue', 2, repeated=True)


class Mixin(_messages.Message):
    """Declares an API to be included in this API. The including API must
    redeclare all the methods from the included API, but documentation and
    options are inherited as follows:  - If after comment and whitespace
    stripping, the documentation   string of the redeclared method is empty, it
    will be inherited   from the original method.  - Each annotation belonging
    to the service config (http,   visibility) which is not set in the
    redeclared method will be   inherited.  - If an http annotation is
    inherited, the path pattern will be   modified as follows. Any version
    prefix will be replaced by the   version of the including API plus the root
    path if specified.  Example of a simple mixin:      package google.acl.v1;
    service AccessControl {       // Get the underlying ACL object.       rpc
    GetAcl(GetAclRequest) returns (Acl) {         option (google.api.http).get =
    "/v1/{resource=**}:getAcl";       }     }      package google.storage.v2;
    service Storage {       //       rpc GetAcl(GetAclRequest) returns (Acl);
    // Get a data record.       rpc GetData(GetDataRequest) returns (Data) {
    option (google.api.http).get = "/v2/{resource=**}";       }     }  Example
    of a mixin configuration:      apis:     - name: google.storage.v2.Storage
    mixins:       - name: google.acl.v1.AccessControl  The mixin construct
    implies that all methods in `AccessControl` are also declared with same name
    and request/response types in `Storage`. A documentation generator or
    annotation processor will see the effective `Storage.GetAcl` method after
    inherting documentation and annotations as follows:      service Storage {
    // Get the underlying ACL object.       rpc GetAcl(GetAclRequest) returns
    (Acl) {         option (google.api.http).get = "/v2/{resource=**}:getAcl";
    }       ...     }  Note how the version in the path pattern changed from
    `v1` to `v2`.  If the `root` field in the mixin is specified, it should be a
    relative path under which inherited HTTP paths are placed. Example:
    apis:     - name: google.storage.v2.Storage       mixins:       - name:
    google.acl.v1.AccessControl         root: acls  This implies the following
    inherited HTTP annotation:      service Storage {       // Get the
    underlying ACL object.       rpc GetAcl(GetAclRequest) returns (Acl) {
    option (google.api.http).get = "/v2/acls/{resource=**}:getAcl";       }
    ...     }

    Fields:
      name: The fully qualified name of the API which is included.
      root: If non-empty specifies a path under which inherited HTTP paths are
        rooted.
    """

    name = _messages.StringField(1)
    root = _messages.StringField(2)


class Money(_messages.Message):
    """Represents an amount of money with its currency type.

    Fields:
      currencyCode: The 3-letter currency code defined in ISO 4217.
      nanos: Number of nano (10^-9) units of the amount. The value must be
        between -999,999,999 and +999,999,999 inclusive. If `units` is positive,
        `nanos` must be positive or zero. If `units` is zero, `nanos` can be
        positive, zero, or negative. If `units` is negative, `nanos` must be
        negative or zero. For example $-1.75 is represented as `units`=-1 and
        `nanos`=-750,000,000.
      units: The whole units of the amount. For example if `currencyCode` is
        `"USD"`, then 1 unit is one US dollar.
    """

    currencyCode = _messages.StringField(1)
    nanos = _messages.IntegerField(2, variant=_messages.Variant.INT32)
    units = _messages.IntegerField(3)


class MonitoredResourceDescriptor(_messages.Message):
    """An object that describes the schema of a MonitoredResource object using a
    type name and a set of labels.  For example, the monitored resource
    descriptor for Google Compute Engine VM instances has a type of
    `"gce_instance"` and specifies the use of the labels `"instance_id"` and
    `"zone"` to identify particular VM instances.  Different APIs can support
    different monitored resource types. APIs generally provide a `list` method
    that returns the monitored resource descriptors used by the API.

    Fields:
      description: Optional. A detailed description of the monitored resource
        type that might be used in documentation.
      displayName: Optional. A concise name for the monitored resource type that
        might be displayed in user interfaces. For example, `"Google Cloud SQL
        Database"`.
      labels: Required. A set of labels used to describe instances of this
        monitored resource type. For example, an individual Google Cloud SQL
        database is identified by values for the labels `"database_id"` and
        `"zone"`.
      name: Optional. The resource name of the monitored resource descriptor:
        `"projects/{project_id}/monitoredResourceDescriptors/{type}"` where
        {type} is the value of the `type` field in this object and {project_id}
        is a project ID that provides API-specific context for accessing the
        type.  APIs that do not use project information can use the resource
        name format `"monitoredResourceDescriptors/{type}"`.
      type: Required. The monitored resource type. For example, the type
        `"cloudsql_database"` represents databases in Google Cloud SQL.
    """

    description = _messages.StringField(1)
    displayName = _messages.StringField(2)
    labels = _messages.MessageField('LabelDescriptor', 3, repeated=True)
    name = _messages.StringField(4)
    type = _messages.StringField(5)


class Monitoring(_messages.Message):
    """Monitoring configuration of the service.  The example below shows how to
    configure monitored resources and metrics for monitoring. In the example, a
    monitored resource and two metrics are defined. The
    `library.googleapis.com/book/returned_count` metric is sent to both producer
    and consumer projects, whereas the
    `library.googleapis.com/book/overdue_count` metric is only sent to the
    consumer project.      monitored_resources:     - type:
    library.googleapis.com/branch       labels:       - key: /city
    description: The city where the library branch is located in.       - key:
    /name         description: The name of the branch.     metrics:     - name:
    library.googleapis.com/book/returned_count       metric_kind: DELTA
    value_type: INT64       labels:       - key: /customer_id     - name:
    library.googleapis.com/book/overdue_count       metric_kind: GAUGE
    value_type: INT64       labels:       - key: /customer_id     monitoring:
    producer_destinations:       - monitored_resource:
    library.googleapis.com/branch         metrics:         -
    library.googleapis.com/book/returned_count       consumer_destinations:
    - monitored_resource: library.googleapis.com/branch         metrics:
    - library.googleapis.com/book/returned_count         -
    library.googleapis.com/book/overdue_count

    Fields:
      consumerDestinations: Monitoring configurations for sending metrics to the
        consumer project. There can be multiple consumer destinations, each one
        must have a different monitored resource type. A metric can be used in
        at most one consumer destination.
      producerDestinations: Monitoring configurations for sending metrics to the
        producer project. There can be multiple producer destinations, each one
        must have a different monitored resource type. A metric can be used in
        at most one producer destination.
    """

    consumerDestinations = _messages.MessageField('MonitoringDestination', 1, repeated=True)
    producerDestinations = _messages.MessageField('MonitoringDestination', 2, repeated=True)


class MonitoringDestination(_messages.Message):
    """Configuration of a specific monitoring destination (the producer project
    or the consumer project).

    Fields:
      metrics: Names of the metrics to report to this monitoring destination.
        Each name must be defined in Service.metrics section.
      monitoredResource: The monitored resource type. The type must be defined
        in Service.monitored_resources section.
    """

    metrics = _messages.StringField(1, repeated=True)
    monitoredResource = _messages.StringField(2)


class OAuthRequirements(_messages.Message):
    """OAuth scopes are a way to define data and permissions on data. For
    example, there are scopes defined for "Read-only access to Google Calendar"
    and "Access to Cloud Platform". Users can consent to a scope for an
    application, giving it permission to access that data on their behalf.
    OAuth scope specifications should be fairly coarse grained; a user will need
    to see and understand the text description of what your scope means.  In
    most cases: use one or at most two OAuth scopes for an entire family of
    products. If your product has multiple APIs, you should probably be sharing
    the OAuth scope across all of those APIs.  When you need finer grained OAuth
    consent screens: talk with your product management about how developers will
    use them in practice.  Please note that even though each of the canonical
    scopes is enough for a request to be accepted and passed to the backend, a
    request can still fail due to the backend requiring additional scopes or
    permissions.

    Fields:
      canonicalScopes: The list of publicly documented OAuth scopes that are
        allowed access. An OAuth token containing any of these scopes will be
        accepted.  Example:       canonical_scopes:
        https://www.googleapis.com/auth/calendar,
        https://www.googleapis.com/auth/calendar.read
    """

    canonicalScopes = _messages.StringField(1)


class Operation(_messages.Message):
    """Represents information regarding an operation.

    Enums:
      ImportanceValueValuesEnum: The importance of the data contained in the
        operation.

    Messages:
      LabelsValue: Labels describing the operation. Only the following labels
        are allowed:  Labels describing the monitored resource. The labels must
        be defined in the service configuration.  Default labels of the metric
        values. When specified, labels defined in the metric value overrule.
        Labels are defined and documented by Google Cloud Platform. For example:
        `cloud.googleapis.com/location: "us-east1"`.

    Fields:
      consumerId: Identity of the consumer who is using the service. This field
        should be filled in for the operations initiated by a consumer, but not
        for service initiated operations that are not related to a specific
        consumer.  The accepted format is dependent on the implementation. The
        Google Service Control implementation accepts four forms:
        "project:<project_id>", "project_number:<project_number>",
        "api_key:<api_key>" and "spatula_header:<spatula_header>".
      endTime: End time of the operation. Required when the operation is used in
        ControllerService.Report, but optional when the operation is used in
        ControllerService.Check.
      importance: The importance of the data contained in the operation.
      labels: Labels describing the operation. Only the following labels are
        allowed:  Labels describing the monitored resource. The labels must be
        defined in the service configuration.  Default labels of the metric
        values. When specified, labels defined in the metric value overrule.
        Labels are defined and documented by Google Cloud Platform. For example:
        `cloud.googleapis.com/location: "us-east1"`.
      logEntries: Represents information to be logged.
      metricValueSets: Represents information about this operation. Each
        MetricValueSet corresponds to a metric defined in the service
        configuration. The data type used in the MetricValueSet must agree with
        the data type specified in the metric definition.  Within a single
        operation, it is not allowed to have more than one MetricValue instances
        that have the same metric names and identical label value combinations.
        The existence of such duplicated MetricValue instances in a request
        causes the entire request being rejected with an invalid argument error.
      operationId: Identity of the operation. It must be unique within the scope
        of the service that the operation is generated. If the service calls
        Check() and Report() on the same operation, the two calls should carry
        the same id.  UUID version 4 is recommended, though not required. In the
        scenarios where an operation is computed from existing information and
        an idempotent id is desirable for deduplication purpose, UUID version 5
        is recommended. See RFC 4122 for details.
      operationName: Fully qualified name of the operation. Example of an RPC
        method name used as operation name:
        google.example.library.v1.LibraryService.CreateShelf Example of a
        service defined operation name:
        compute.googleapis.com/InstanceHeartbeat
      quotaProperties: Represents the properties needed for quota check.
        Applicable only if this operation is for a quota check request.
      startTime: Start time of the operation. Required.
    """

    class ImportanceValueValuesEnum(_messages.Enum):
        """The importance of the data contained in the operation.

        Values:
          LOW: The operation doesn't contain significant monetary value or audit
            trail. The API implementation may cache and aggregate the data. There
            is no deduplication based on `operation_id`. The data may be lost when
            rare and unexpected system failures occur.
          HIGH: The operation contains significant monetary value or audit trail.
            The API implementation doesn't cache and aggregate the data.
            Deduplication based on `operation_id` is performed for monetary
            values. If the method returns successfully, it's guaranteed that the
            data are persisted in durable storage.
        """
        LOW = 0
        HIGH = 1

    @encoding.MapUnrecognizedFields('additionalProperties')
    class LabelsValue(_messages.Message):
        """Labels describing the operation. Only the following labels are allowed:
        Labels describing the monitored resource. The labels must be defined in
        the service configuration.  Default labels of the metric values. When
        specified, labels defined in the metric value overrule.  Labels are
        defined and documented by Google Cloud Platform. For example:
        `cloud.googleapis.com/location: "us-east1"`.

        Messages:
          AdditionalProperty: An additional property for a LabelsValue object.

        Fields:
          additionalProperties: Additional properties of type LabelsValue
        """

        class AdditionalProperty(_messages.Message):
            """An additional property for a LabelsValue object.

            Fields:
              key: Name of the additional property.
              value: A string attribute.
            """

            key = _messages.StringField(1)
            value = _messages.StringField(2)

        additionalProperties = _messages.MessageField('AdditionalProperty', 1, repeated=True)

    consumerId = _messages.StringField(1)
    endTime = _messages.StringField(2)
    importance = _messages.EnumField('ImportanceValueValuesEnum', 3)
    labels = _messages.MessageField('LabelsValue', 4)
    logEntries = _messages.MessageField('LogEntry', 5, repeated=True)
    metricValueSets = _messages.MessageField('MetricValueSet', 6, repeated=True)
    operationId = _messages.StringField(7)
    operationName = _messages.StringField(8)
    quotaProperties = _messages.MessageField('QuotaProperties', 9)
    startTime = _messages.StringField(10)


class Option(_messages.Message):
    """A protocol buffer option, which can be attached to a message, field,
    enumeration, etc.

    Messages:
      ValueValue: The option's value. For example, `"com.google.protobuf"`.

    Fields:
      name: The option's name. For example, `"java_package"`.
      value: The option's value. For example, `"com.google.protobuf"`.
    """

    @encoding.MapUnrecognizedFields('additionalProperties')
    class ValueValue(_messages.Message):
        """The option's value. For example, `"com.google.protobuf"`.

        Messages:
          AdditionalProperty: An additional property for a ValueValue object.

        Fields:
          additionalProperties: Properties of the object. Contains field @ype with
            type URL.
        """

        class AdditionalProperty(_messages.Message):
            """An additional property for a ValueValue object.

            Fields:
              key: Name of the additional property.
              value: A extra_types.JsonValue attribute.
            """

            key = _messages.StringField(1)
            value = _messages.MessageField('extra_types.JsonValue', 2)

        additionalProperties = _messages.MessageField('AdditionalProperty', 1, repeated=True)

    name = _messages.StringField(1)
    value = _messages.MessageField('ValueValue', 2)


class Page(_messages.Message):
    """Represents a documentation page. A page can contain subpages to represent
    nested documentation set structure.

    Fields:
      content: The Markdown content of the page. You can use <code>&#40;==
        include {path} ==&#41;</code> to include content from a Markdown file.
      name: The name of the page. It will be used as an identity of the page to
        generate URI of the page, text of the link to this page in navigation,
        etc. The full page name (start from the root page name to this page
        concatenated with `.`) can be used as reference to the page in your
        documentation. For example: <pre><code>pages: - name: Tutorial
        content: &#40;== include tutorial.md ==&#41;   subpages:   - name: Java
        content: &#40;== include tutorial_java.md ==&#41; </code></pre> You can
        reference `Java` page using Markdown reference link syntax: `Java`.
      subpages: Subpages of this page. The order of subpages specified here will
        be honored in the generated docset.
    """

    content = _messages.StringField(1)
    name = _messages.StringField(2)
    subpages = _messages.MessageField('Page', 3, repeated=True)


class ProjectProperties(_messages.Message):
    """A descriptor for defining project properties for a service. One service
    may have many consumer projects, and the service may want to behave
    differently depending on some properties on the project. For example, a
    project may be associated with a school, or a business, or a government
    agency, a business type property on the project may affect how a service
    responds to the client. This descriptor defines which properties are allowed
    to be set on a project.  Example:     project_properties:      properties:
    - name: NO_WATERMARK        type: BOOL        description: Allows usage of
    the API without watermarks.      - name: EXTENDED_TILE_CACHE_PERIOD
    type: INT64

    Fields:
      properties: List of per consumer project-specific properties.
    """

    properties = _messages.MessageField('Property', 1, repeated=True)


class Property(_messages.Message):
    """Defines project properties.  API services can define properties that can
    be assigned to consumer projects so that backends can perform response
    customization without having to make additional calls or maintain additional
    storage. For example, Maps API defines properties that controls map tile
    cache period, or whether to embed a watermark in a result.  These values can
    be set via API producer console. Only API providers can define and set these
    properties.

    Enums:
      TypeValueValuesEnum: The type of this property.

    Fields:
      description: The description of the property
      name: The name of the property (a.k.a key).
      type: The type of this property.
    """

    class TypeValueValuesEnum(_messages.Enum):
        """The type of this property.

        Values:
          UNSPECIFIED: The type is unspecified, and will result in an error.
          INT64: The type is `int64`.
          BOOL: The type is `bool`.
          STRING: The type is `string`.
          DOUBLE: The type is 'double'.
        """
        UNSPECIFIED = 0
        INT64 = 1
        BOOL = 2
        STRING = 3
        DOUBLE = 4

    description = _messages.StringField(1)
    name = _messages.StringField(2)
    type = _messages.EnumField('TypeValueValuesEnum', 3)


class Quota(_messages.Message):
    """Quota configuration helps to achieve fairness and budgeting in service
    usage.  - Fairness is achieved through the use of short-term quota limits
    that are usually defined over a time window of several seconds or   minutes.
    When such a limit is applied, for example at the user   level, it ensures
    that no single user will monopolize the service   or a given customer's
    allocated portion of it. - Budgeting is achieved through the use of long-
    term quota limits   that are usually defined over a time window of one or
    more   days. These limits help client application developers predict the
    usage and help budgeting.  Quota enforcement uses a simple token-based
    algorithm for resource sharing.  The quota configuration structure is as
    follows:  - `QuotaLimit` defines a single enforceable limit with a specified
    token amount that can be consumed over a specific duration and   applies to
    a particular entity, like a project or an end user. If   the limit applies
    to a user, each user making the request will   get the specified number of
    tokens to consume. When the tokens   run out, the requests from that user
    will be blocked until the   duration elapses and the next duration window
    starts.  - `QuotaGroup` groups a set of quota limits.  - `QuotaRule` maps a
    method to a set of quota groups. This allows   sharing of quota groups
    across methods as well as one method   consuming tokens from more than one
    quota group. When a group   contains multiple limits, requests to a method
    consuming tokens   from that group must satisfy all the limits in that
    group.  Example:      quota:       groups:       - name: ReadGroup
    limits:         - description: Daily Limit           name: ProjectQpd
    default_limit: 10000           duration: 1d           limit_by:
    CLIENT_PROJECT          - description: Per-second Limit           name:
    UserQps           default_limit: 20000           duration: 100s
    limit_by: USER        - name: WriteGroup         limits:         -
    description: Daily Limit           name: ProjectQpd           default_limit:
    1000           max_limit: 1000           duration: 1d           limit_by:
    CLIENT_PROJECT          - description: Per-second Limit           name:
    UserQps           default_limit: 2000           max_limit: 4000
    duration: 100s           limit_by: USER        rules:       - selector: "*"
    groups:         - group: ReadGroup       - selector:
    google.calendar.Calendar.Update         groups:         - group: WriteGroup
    cost: 2       - selector: google.calendar.Calendar.Delete         groups:
    - group: WriteGroup  Here, the configuration defines two quota groups:
    ReadGroup and WriteGroup, each defining its own daily and per-second limits.
    Note that One Platform enforces per-second limits averaged over a duration
    of 100 seconds. The rules map ReadGroup for all methods, except for the
    Update and Delete methods. These two methods consume from WriteGroup, with
    Update method consuming at twice the rate as Delete method.  Multiple quota
    groups can be specified for a method. The quota limits in all of those
    groups will be enforced. Example:      quota:       groups:       - name:
    WriteGroup         limits:         - description: Daily Limit
    name: ProjectQpd           default_limit: 1000           max_limit: 1000
    duration: 1d           limit_by: CLIENT_PROJECT          - description: Per-
    second Limit           name: UserQps           default_limit: 2000
    max_limit: 4000           duration: 100s           limit_by: USER        -
    name: StorageGroup         limits:         - description: Storage Quota
    name: StorageQuota           default_limit: 1000           duration: 0
    limit_by: USER        rules:       - selector:
    google.calendar.Calendar.Create         groups:         - group:
    StorageGroup         - group: WriteGroup       - selector:
    google.calendar.Calendar.Delete         groups:         - group:
    StorageGroup  In the above example, the Create and Delete methods manage the
    user's storage space. In addition, Create method uses WriteGroup to manage
    the requests. In this case, requests to Create method need to satisfy all
    quota limits defined in both quota groups.  One can disable quota for
    selected method(s) identified by the selector by setting disable_quota to
    ture. For example,        rules:       - selector: "*"         group:
    - group ReadGroup       - selector: google.calendar.Calendar.Select
    disable_quota: true

    Fields:
      groups: List of `QuotaGroup` definitions for the service.
      rules: List of `QuotaRule` definitions, each one mapping a selected method
        to one or more quota groups.
    """

    groups = _messages.MessageField('QuotaGroup', 1, repeated=True)
    rules = _messages.MessageField('QuotaRule', 2, repeated=True)


class QuotaGroup(_messages.Message):
    """`QuotaGroup` defines a set of quota limits to enforce.

    Fields:
      billable: Indicates if the quota limits defined in this quota group apply
        to consumers who have active billing. Quota limits defined in billable
        groups will be applied only to consumers who have active billing. The
        amount of tokens consumed from billable quota group will also be
        reported for billing. Quota limits defined in non-billable groups will
        be applied only to consumers who have no active billing.
      description: User-visible description of this quota group.
      limits: Quota limits to be enforced when this quota group is used. A
        request must satisfy all the limits in a group for it to be permitted.
      name: Name of this quota group. Must be unique within the service.  Quota
        group name is used as part of the id for quota limits. Once the quota
        group has been put into use, the name of the quota group should be
        immutable.
    """

    billable = _messages.BooleanField(1)
    description = _messages.StringField(2)
    limits = _messages.MessageField('QuotaLimit', 3, repeated=True)
    name = _messages.StringField(4)


class QuotaGroupMapping(_messages.Message):
    """A quota group mapping.

    Fields:
      cost: Number of tokens to consume for each request. This allows different
        cost to be associated with different methods that consume from the same
        quota group. By default, each request will cost one token.
      group: The `QuotaGroup.name` of the group. Requests for the mapped methods
        will consume tokens from each of the limits defined in this group.
    """

    cost = _messages.IntegerField(1, variant=_messages.Variant.INT32)
    group = _messages.StringField(2)


class QuotaLimit(_messages.Message):
    """`QuotaLimit` defines a specific limit that applies over a specified
    duration for a limit type. There can be at most one limit for a duration and
    limit type combination defined within a `QuotaGroup`.

    Enums:
      LimitByValueValuesEnum: Limit type to use for enforcing this quota limit.
        Each unique value gets the defined number of tokens to consume from. For
        a quota limit that uses user type, each user making requests through the
        same client application project will get his/her own pool of tokens to
        consume, whereas for a limit that uses client project type, all users
        making requests through the same client application project share a
        single pool of tokens.

    Fields:
      defaultLimit: Default number of tokens that can be consumed during the
        specified duration. This is the number of tokens assigned when a client
        application developer activates the service for his/her project.
        Specifying a value of 0 will block all requests. This can be used if you
        are provisioning quota to selected consumers and blocking others.
        Similarly, a value of -1 will indicate an unlimited quota. No other
        negative values are allowed.
      description: Optional. User-visible, extended description for this quota
        limit. Should be used only when more context is needed to understand
        this limit than provided by the limit's display name (see:
        `display_name`).
      displayName: User-visible display name for this limit. Optional. If not
        set, the UI will provide a default display name based on the quota
        configuration. This field can be used to override the default display
        name generated from the configuration.
      duration: Duration of this limit in textual notation. Example: "100s",
        "24h", "1d". For duration longer than a day, only multiple of days is
        supported. We support only "100s" and "1d" for now. Additional support
        will be added in the future. "0" indicates indefinite duration.
      freeTier: Free tier value displayed in the Developers Console for this
        limit. The free tier is the number of tokens that will be subtracted
        from the billed amount when billing is enabled. This field can only be
        set on a limit with duration "1d", in a billable group; it is invalid on
        any other limit. If this field is not set, it defaults to 0, indicating
        that there is no free tier for this service.
      limitBy: Limit type to use for enforcing this quota limit. Each unique
        value gets the defined number of tokens to consume from. For a quota
        limit that uses user type, each user making requests through the same
        client application project will get his/her own pool of tokens to
        consume, whereas for a limit that uses client project type, all users
        making requests through the same client application project share a
        single pool of tokens.
      maxLimit: Maximum number of tokens that can be consumed during the
        specified duration. Client application developers can override the
        default limit up to this maximum. If specified, this value cannot be set
        to a value less than the default limit. If not specified, it is set to
        the default limit.  To allow clients to apply overrides with no upper
        bound, set this to -1, indicating unlimited maximum quota.
      name: Name of the quota limit.  Must be unique within the quota group.
        This name is used to refer to the limit when overriding the limit on a
        per-project basis.  If a name is not provided, it will be generated from
        the limit_by and duration fields.  The maximum length of the limit name
        is 64 characters.  The name of a limit is used as a unique identifier
        for this limit. Therefore, once a limit has been put into use, its name
        should be immutable. You can use the display_name field to provide a
        user-friendly name for the limit. The display name can be evolved over
        time without affecting the identity of the limit.
    """

    class LimitByValueValuesEnum(_messages.Enum):
        """Limit type to use for enforcing this quota limit. Each unique value
        gets the defined number of tokens to consume from. For a quota limit that
        uses user type, each user making requests through the same client
        application project will get his/her own pool of tokens to consume,
        whereas for a limit that uses client project type, all users making
        requests through the same client application project share a single pool
        of tokens.

        Values:
          CLIENT_PROJECT: ID of the project owned by the client application
            developer making the request.
          USER: ID of the end user making the request using the client
            application.
        """
        CLIENT_PROJECT = 0
        USER = 1

    defaultLimit = _messages.IntegerField(1)
    description = _messages.StringField(2)
    displayName = _messages.StringField(3)
    duration = _messages.StringField(4)
    freeTier = _messages.IntegerField(5)
    limitBy = _messages.EnumField('LimitByValueValuesEnum', 6)
    maxLimit = _messages.IntegerField(7)
    name = _messages.StringField(8)


class QuotaProperties(_messages.Message):
    """Represents the properties needed for quota operations.  Use the
    metric_value_sets field in Operation message to provide cost override with
    metric_name in <service_name>/quota/<quota_group_name>/cost format.
    Overrides for unmatched quota groups will be ignored. Costs are expected to
    be >= 0. Cost 0 will cause no quota check, but still traffic restrictions
    will be enforced.

    Enums:
      QuotaModeValueValuesEnum: Quota mode for this operation.

    Messages:
      LimitByIdsValue: LimitType IDs that should be used for checking quota. Key
        in this map should be a valid LimitType string, and the value is the ID
        to be used. For ex., an entry <USER, 123> will cause all user quota
        limits to use 123 as the user ID. See google/api/quota.proto for the
        definition of LimitType. CLIENT_PROJECT: Not supported. USER: Value of
        this entry will be used for enforcing user-level quota       limits. If
        none specified, caller IP passed in the
        servicecontrol.googleapis.com/caller_ip label will be used instead.
        If the server cannot resolve a value for this LimitType, an error
        will be thrown. No validation will be performed on this ID.

    Fields:
      limitByIds: LimitType IDs that should be used for checking quota. Key in
        this map should be a valid LimitType string, and the value is the ID to
        be used. For ex., an entry <USER, 123> will cause all user quota limits
        to use 123 as the user ID. See google/api/quota.proto for the definition
        of LimitType. CLIENT_PROJECT: Not supported. USER: Value of this entry
        will be used for enforcing user-level quota       limits. If none
        specified, caller IP passed in the
        servicecontrol.googleapis.com/caller_ip label will be used instead.
        If the server cannot resolve a value for this LimitType, an error
        will be thrown. No validation will be performed on this ID.
      quotaMode: Quota mode for this operation.
    """

    class QuotaModeValueValuesEnum(_messages.Enum):
        """Quota mode for this operation.

        Values:
          ACQUIRE: Decreases available quota by the cost specified for the
            operation. If cost is higher than available quota, operation fails and
            returns error.
          ACQUIRE_BEST_EFFORT: Decreases available quota by the cost specified for
            the operation. If cost is higher than available quota, operation does
            not fail and available quota goes down to zero but it returns error.
          CHECK: Does not change any available quota. Only checks if there is
            enough quota. No lock is placed on the checked tokens neither.
          RELEASE: Increases available quota by the operation cost specified for
            the operation.
        """
        ACQUIRE = 0
        ACQUIRE_BEST_EFFORT = 1
        CHECK = 2
        RELEASE = 3

    @encoding.MapUnrecognizedFields('additionalProperties')
    class LimitByIdsValue(_messages.Message):
        """LimitType IDs that should be used for checking quota. Key in this map
        should be a valid LimitType string, and the value is the ID to be used.
        For ex., an entry <USER, 123> will cause all user quota limits to use 123
        as the user ID. See google/api/quota.proto for the definition of
        LimitType. CLIENT_PROJECT: Not supported. USER: Value of this entry will
        be used for enforcing user-level quota       limits. If none specified,
        caller IP passed in the       servicecontrol.googleapis.com/caller_ip
        label will be used instead.       If the server cannot resolve a value for
        this LimitType, an error       will be thrown. No validation will be
        performed on this ID.

        Messages:
          AdditionalProperty: An additional property for a LimitByIdsValue object.

        Fields:
          additionalProperties: Additional properties of type LimitByIdsValue
        """

        class AdditionalProperty(_messages.Message):
            """An additional property for a LimitByIdsValue object.

            Fields:
              key: Name of the additional property.
              value: A string attribute.
            """

            key = _messages.StringField(1)
            value = _messages.StringField(2)

        additionalProperties = _messages.MessageField('AdditionalProperty', 1, repeated=True)

    limitByIds = _messages.MessageField('LimitByIdsValue', 1)
    quotaMode = _messages.EnumField('QuotaModeValueValuesEnum', 2)


class QuotaRule(_messages.Message):
    """`QuotaRule` maps a method to a set of `QuotaGroup`s.

    Fields:
      disableQuota: Indicates if quota checking should be enforced. Quota will
        be disabled for methods without quota rules or with quota rules having
        this field set to true. When this field is set to true, no quota group
        mapping is allowed.
      groups: Quota groups to be used for this method. This supports associating
        a cost with each quota group.
      selector: Selects methods to which this rule applies.  Refer to selector
        for syntax details.
    """

    disableQuota = _messages.BooleanField(1)
    groups = _messages.MessageField('QuotaGroupMapping', 2, repeated=True)
    selector = _messages.StringField(3)


class ReportError(_messages.Message):
    """Represents the processing error of one `Operation` in the request.

    Fields:
      operationId: The Operation.operation_id value from the request.
      status: Details of the error when processing the `Operation`.
    """

    operationId = _messages.StringField(1)
    status = _messages.MessageField('Status', 2)


class ReportRequest(_messages.Message):
    """The request message of the Report method.

    Fields:
      operations: Operations to be reported.  Typically the service should
        report one operation per request. Putting multiple operations into a
        single request is allowed, but should be used only when multiple
        operations are natually available at the time of the report.  If
        multiple operations are in a single request, the total request size
        should be no larger than 1MB. See ReportResponse.report_errors for
        partial failure behavior.
    """

    operations = _messages.MessageField('Operation', 1, repeated=True)


class ReportResponse(_messages.Message):
    """The response message of the Report method.

    Fields:
      reportErrors: The partial failures, one for each `Operation` in the
        request that failed processing. There are three possible combinations of
        the RPC status and this list:  1. The combination of a successful RPC
        status and an empty `report_errors`    list indicates a complete success
        where all `Operation`s in the    request are processed successfully. 2.
        The combination of a successful RPC status and a non-empty
        `report_errors` list indicates a partial success where some
        `Operation`s in the request are processed successfully. Each
        `Operation` that failed processing has a corresponding item    in this
        list. 3. A failed RPC status indicates a complete failure where none of
        the    `Operation`s in the request is processed successfully.
    """

    reportErrors = _messages.MessageField('ReportError', 1, repeated=True)


class Service(_messages.Message):
    """`Service` is the root object of the configuration schema. It describes
    basic information like the name of the service and the exposed API
    interfaces, and delegates other aspects to configuration sub-sections.
    Example:      type: google.api.Service     config_version: 1     name:
    calendar.googleapis.com     title: Google Calendar API     apis:     - name:
    google.calendar.Calendar     backend:       rules:       - selector: "*"
    address: calendar.example.com

    Fields:
      apis: A list of API interfaces exported by this service. Only the `name`
        field of the google.protobuf.Api needs to be provided by the
        configuration author, as the remaining fields will be derived from the
        IDL during the normalization process. It is an error to specify an API
        interface here which cannot be resolved against the associated IDL
        files.
      authentication: Auth configuration.
      backend: API backend configuration.
      billing: Billing configuration of the service.
      configVersion: The version of the service configuration. The config
        version may influence interpretation of the configuration, for example,
        to determine defaults. This is documented together with applicable
        options. The current default for the config version itself is `3`.
      context: Context configuration.
      control: Configuration for the service control plane.
      customError: Custom error configuration.
      documentation: Additional API documentation.
      enums: A list of all enum types included in this API service.  Enums
        referenced directly or indirectly by the `apis` are automatically
        included.  Enums which are not referenced but shall be included should
        be listed here by name. Example:      enums:     - name:
        google.someapi.v1.SomeEnum
      http: HTTP configuration.
      id: A unique ID for a specific instance of this message, typically
        assigned by the client for tracking purpose. If empty, the server may
        choose to generate one instead.
      logging: Logging configuration of the service.
      logs: Defines the logs used by this service.
      metrics: Defines the metrics used by this service.
      monitoredResources: Defines the monitored resources used by this service.
        This is required by the Service.monitoring and Service.logging
        configurations.
      monitoring: Monitoring configuration of the service.
      name: The DNS address at which this service is available, e.g.
        `calendar.googleapis.com`.
      producerProjectId: The id of the Google developer project that owns the
        service. Members of this project can manage the service configuration,
        manage consumption of the service, etc.
      projectProperties: Configuration of per-consumer project properties.
      quota: Quota configuration.
      systemParameters: Configuration for system parameters.
      systemTypes: A list of all proto message types included in this API
        service. It serves similar purpose as [google.api.Service.types], except
        that these types are not needed by user-defined APIs. Therefore, they
        will not show up in the generated discovery doc. This field should only
        be used to define system APIs in ESF.
      title: The product title associated with this service.
      types: A list of all proto message types included in this API service.
        Types referenced directly or indirectly by the `apis` are automatically
        included.  Messages which are not referenced but shall be included, such
        as types used by the `google.protobuf.Any` type, should be listed here
        by name. Example:      types:     - name: google.protobuf.Int32
      usage: Configuration controlling usage of this service.
      visibility: API visibility configuration.
    """

    apis = _messages.MessageField('Api', 1, repeated=True)
    authentication = _messages.MessageField('Authentication', 2)
    backend = _messages.MessageField('Backend', 3)
    billing = _messages.MessageField('Billing', 4)
    configVersion = _messages.IntegerField(5, variant=_messages.Variant.UINT32)
    context = _messages.MessageField('Context', 6)
    control = _messages.MessageField('Control', 7)
    customError = _messages.MessageField('CustomError', 8)
    documentation = _messages.MessageField('Documentation', 9)
    enums = _messages.MessageField('Enum', 10, repeated=True)
    http = _messages.MessageField('Http', 11)
    id = _messages.StringField(12)
    logging = _messages.MessageField('Logging', 13)
    logs = _messages.MessageField('LogDescriptor', 14, repeated=True)
    metrics = _messages.MessageField('MetricDescriptor', 15, repeated=True)
    monitoredResources = _messages.MessageField('MonitoredResourceDescriptor', 16, repeated=True)
    monitoring = _messages.MessageField('Monitoring', 17)
    name = _messages.StringField(18)
    producerProjectId = _messages.StringField(19)
    projectProperties = _messages.MessageField('ProjectProperties', 20)
    quota = _messages.MessageField('Quota', 21)
    systemParameters = _messages.MessageField('SystemParameters', 22)
    systemTypes = _messages.MessageField('Type', 23, repeated=True)
    title = _messages.StringField(24)
    types = _messages.MessageField('Type', 25, repeated=True)
    usage = _messages.MessageField('Usage', 26)
    visibility = _messages.MessageField('Visibility', 27)


class ServicecontrolServicesCheckRequest(_messages.Message):
    """A ServicecontrolServicesCheckRequest object.

    Fields:
      checkRequest: A CheckRequest resource to be passed as the request body.
      serviceName: The service name. The DNS address at which this service is
        available, such as `"pubsub.googleapis.com"`.  Please see
        `google.api.Service` for the definition of service name.
    """

    checkRequest = _messages.MessageField('CheckRequest', 1)
    serviceName = _messages.StringField(2, required=True)


class ServicecontrolServicesReportRequest(_messages.Message):
    """A ServicecontrolServicesReportRequest object.

    Fields:
      reportRequest: A ReportRequest resource to be passed as the request body.
      serviceName: The service name. The DNS address at which this service is
        available, such as `"pubsub.googleapis.com"`.  Please see
        `google.api.Service` for the definition of service name.
    """

    reportRequest = _messages.MessageField('ReportRequest', 1)
    serviceName = _messages.StringField(2, required=True)


class SourceContext(_messages.Message):
    """`SourceContext` represents information about the source of a protobuf
    element, like the file in which it is defined.

    Fields:
      fileName: The path-qualified name of the .proto file that contained the
        associated protobuf element.  For example:
        `"google/protobuf/source.proto"`.
    """

    fileName = _messages.StringField(1)


class StandardQueryParameters(_messages.Message):
    """Query parameters accepted by all methods.

    Enums:
      FXgafvValueValuesEnum: V1 error format.
      AltValueValuesEnum: Data format for response.

    Fields:
      f__xgafv: V1 error format.
      access_token: OAuth access token.
      alt: Data format for response.
      bearer_token: OAuth bearer token.
      callback: JSONP
      fields: Selector specifying which fields to include in a partial response.
      key: API key. Your API key identifies your project and provides you with
        API access, quota, and reports. Required unless you provide an OAuth 2.0
        token.
      oauth_token: OAuth 2.0 token for the current user.
      pp: Pretty-print response.
      prettyPrint: Returns response with indentations and line breaks.
      quotaUser: Available to use for quota purposes for server-side
        applications. Can be any arbitrary string assigned to a user, but should
        not exceed 40 characters.
      trace: A tracing token of the form "token:<tokenid>" to include in api
        requests.
      uploadType: Legacy upload protocol for media (e.g. "media", "multipart").
      upload_protocol: Upload protocol for media (e.g. "raw", "multipart").
    """

    class AltValueValuesEnum(_messages.Enum):
        """Data format for response.

        Values:
          json: Responses with Content-Type of application/json
          media: Media download with context-dependent Content-Type
          proto: Responses with Content-Type of application/x-protobuf
        """
        json = 0
        media = 1
        proto = 2

    class FXgafvValueValuesEnum(_messages.Enum):
        """V1 error format.

        Values:
          _1: v1 error format
          _2: v2 error format
        """
        _1 = 0
        _2 = 1

    f__xgafv = _messages.EnumField('FXgafvValueValuesEnum', 1)
    access_token = _messages.StringField(2)
    alt = _messages.EnumField('AltValueValuesEnum', 3, default=u'json')
    bearer_token = _messages.StringField(4)
    callback = _messages.StringField(5)
    fields = _messages.StringField(6)
    key = _messages.StringField(7)
    oauth_token = _messages.StringField(8)
    pp = _messages.BooleanField(9, default=True)
    prettyPrint = _messages.BooleanField(10, default=True)
    quotaUser = _messages.StringField(11)
    trace = _messages.StringField(12)
    uploadType = _messages.StringField(13)
    upload_protocol = _messages.StringField(14)


class Status(_messages.Message):
    """The `Status` type defines a logical error model that is suitable for
    different programming environments, including REST APIs and RPC APIs. It is
    used by [gRPC](https://github.com/grpc). The error model is designed to be:
    - Simple to use and understand for most users - Flexible enough to meet
    unexpected needs  # Overview  The `Status` message contains three pieces of
    data: error code, error message, and error details. The error code should be
    an enum value of google.rpc.Code, but it may accept additional error codes
    if needed.  The error message should be a developer-facing English message
    that helps developers *understand* and *resolve* the error. If a localized
    user-facing error message is needed, put the localized message in the error
    details or localize it in the client. The optional error details may contain
    arbitrary information about the error. There is a predefined set of error
    detail types in the package `google.rpc` which can be used for common error
    conditions.  # Language mapping  The `Status` message is the logical
    representation of the error model, but it is not necessarily the actual wire
    format. When the `Status` message is exposed in different client libraries
    and different wire protocols, it can be mapped differently. For example, it
    will likely be mapped to some exceptions in Java, but more likely mapped to
    some error codes in C.  # Other uses  The error model and the `Status`
    message can be used in a variety of environments, either with or without
    APIs, to provide a consistent developer experience across different
    environments.  Example uses of this error model include:  - Partial errors.
    If a service needs to return partial errors to the client,     it may embed
    the `Status` in the normal response to indicate the partial     errors.  -
    Workflow errors. A typical workflow has multiple steps. Each step may
    have a `Status` message for error reporting purpose.  - Batch operations. If
    a client uses batch request and batch response, the     `Status` message
    should be used directly inside batch response, one for     each error sub-
    response.  - Asynchronous operations. If an API call embeds asynchronous
    operation     results in its response, the status of those operations should
    be     represented directly using the `Status` message.  - Logging. If some
    API errors are stored in logs, the message `Status` could     be used
    directly after any stripping needed for security/privacy reasons.

    Messages:
       DetailsValueListEntry: A DetailsValueListEntry object.

    Fields:
      code: The status code, which should be an enum value of google.rpc.Code.
      details: A list of messages that carry the error details.  There will be a
          common set of message types for APIs to use.
      message: A developer-facing error message, which should be in English. Any
          user-facing error message should be localized and sent in the
          google.rpc.Status.details field, or localized by the client.
    """

    @encoding.MapUnrecognizedFields('additionalProperties')
    class DetailsValueListEntry(_messages.Message):
        """A DetailsValueListEntry object.

        Messages:
          AdditionalProperty: An additional property for a DetailsValueListEntry
          object.

        Fields:
           additionalProperties: Properties of the object. Contains field @ype with
           type URL.
        """

        class AdditionalProperty(_messages.Message):
            """An additional property for a DetailsValueListEntry object.

            Fields:
              key: Name of the additional property.
              value: A extra_types.JsonValue attribute.
            """

            key = _messages.StringField(1)
            value = _messages.MessageField('extra_types.JsonValue', 2)

        additionalProperties = _messages.MessageField('AdditionalProperty', 1, repeated=True)

    code = _messages.IntegerField(1, variant=_messages.Variant.INT32)
    details = _messages.MessageField('DetailsValueListEntry', 2, repeated=True)
    message = _messages.StringField(3)


class SystemParameter(_messages.Message):
    """Define a parameter's name and location. The parameter may be passed as
    either an HTTP header or a URL query parameter, and if both are passed the
    behavior is implementation-dependent.

    Fields:
      httpHeader: Define the HTTP header name to use for the parameter. It is
        case insensitive.
      name: Define the name of the parameter, such as "api_key", "alt",
        "callback", and etc. It is case sensitive.
      urlQueryParameter: Define the URL query parameter name to use for the
        parameter. It is case sensitive.
    """

    httpHeader = _messages.StringField(1)
    name = _messages.StringField(2)
    urlQueryParameter = _messages.StringField(3)


class SystemParameterRule(_messages.Message):
    """Define a system parameter rule mapping system parameter definitions to
    methods.

    Fields:
      parameters: Define parameters. Multiple names may be defined for a
        parameter. For a given method call, only one of them should be used. If
        multiple names are used the behavior is implementation-dependent. If
        none of the specified names are present the behavior is parameter-
        dependent.
      selector: Selects the methods to which this rule applies. Use '*' to
        indicate all methods in all APIs.  Refer to selector for syntax details.
    """

    parameters = _messages.MessageField('SystemParameter', 1, repeated=True)
    selector = _messages.StringField(2)


class SystemParameters(_messages.Message):
    """### System parameter configuration  A system parameter is a special kind
    of parameter defined by the API system, not by an individual API. It is
    typically mapped to an HTTP header and/or a URL query parameter. This
    configuration specifies which methods change the names of the system
    parameters.

    Fields:
      rules: Define system parameters.  The parameters defined here will
        override the default parameters implemented by the system. If this field
        is missing from the service config, default system parameters will be
        used. Default system parameters and names is implementation-dependent.
        Example: define api key and alt name for all methods  SystemParameters
        rules:     - selector: "*"       parameters:         - name: api_key
        url_query_parameter: api_key         - name: alt           http_header:
        Response-Content-Type  Example: define 2 api key names for a specific
        method.  SystemParameters   rules:     - selector: "/ListShelves"
        parameters:         - name: api_key           http_header: Api-Key1
        - name: api_key           http_header: Api-Key2
    """

    rules = _messages.MessageField('SystemParameterRule', 1, repeated=True)


class Type(_messages.Message):
    """A protocol buffer message type.

    Enums:
      SyntaxValueValuesEnum: The source syntax.

    Fields:
      fields: The list of fields.
      name: The fully qualified message name.
      oneofs: The list of types appearing in `oneof` definitions in this type.
      options: The protocol buffer options.
      sourceContext: The source context.
      syntax: The source syntax.
    """

    class SyntaxValueValuesEnum(_messages.Enum):
        """The source syntax.

        Values:
          SYNTAX_PROTO2: Syntax `proto2`.
          SYNTAX_PROTO3: Syntax `proto3`.
        """
        SYNTAX_PROTO2 = 0
        SYNTAX_PROTO3 = 1

    fields = _messages.MessageField('Field', 1, repeated=True)
    name = _messages.StringField(2)
    oneofs = _messages.StringField(3, repeated=True)
    options = _messages.MessageField('Option', 4, repeated=True)
    sourceContext = _messages.MessageField('SourceContext', 5)
    syntax = _messages.EnumField('SyntaxValueValuesEnum', 6)


class Usage(_messages.Message):
    """Configuration controlling usage of a service.

    Enums:
      ServiceAccessValueValuesEnum: Controls which users can see or activate the
        service.

    Fields:
      activationHooks: Services that must be contacted before a consumer can
        begin using the service. Each service will be contacted in sequence,
        and, if any activation call fails, the entire activation will fail. Each
        hook is of the form <service.name>/<hook-id>, where <hook-id> is
        optional; for example: 'robotservice.googleapis.com/default'.
      deactivationHooks: Services that must be contacted before a consumer can
        deactivate a service. Each service will be contacted in sequence, and,
        if any deactivation call fails, the entire deactivation will fail. Each
        hook is of the form <service.name>/<hook-id>, where <hook-id> is
        optional; for example: 'compute.googleapis.com/'.
      dependsOnServices: Services that must be activated in order for this
        service to be used. The set of services activated as a result of these
        relations are all activated in parallel with no guaranteed order of
        activation. Each string is a service name, e.g.
        `calendar.googleapis.com`.
      requirements: Requirements that must be satisfied before a consumer
        project can use the service. Each requirement is of the form
        <service.name>/<requirement-id>; for example
        'serviceusage.googleapis.com/billing-enabled'.
      rules: Individual rules for configuring usage on selected methods.
      serviceAccess: Controls which users can see or activate the service.
    """

    class ServiceAccessValueValuesEnum(_messages.Enum):
        """Controls which users can see or activate the service.

        Values:
          RESTRICTED: The service can only be seen/used by users identified in the
            service's access control policy.  If the service has not been
            whitelisted by your domain administrator for out-of-org publishing,
            then this mode will be treated like ORG_RESTRICTED.
          PUBLIC: The service can be seen/used by anyone.  If the service has not
            been whitelisted by your domain administrator for out-of-org
            publishing, then this mode will be treated like ORG_PUBLIC.  The
            discovery document for the service will also be public and allow
            unregistered access.
          ORG_RESTRICTED: The service can be seen/used by users identified in the
            service's access control policy and they are within the organization
            that owns the service.  Access is further constrained to the group
            controlled by the administrator of the project/org that owns the
            service.
          ORG_PUBLIC: The service can be seen/used by the group of users
            controlled by the administrator of the project/org that owns the
            service.
        """
        RESTRICTED = 0
        PUBLIC = 1
        ORG_RESTRICTED = 2
        ORG_PUBLIC = 3

    activationHooks = _messages.StringField(1, repeated=True)
    deactivationHooks = _messages.StringField(2, repeated=True)
    dependsOnServices = _messages.StringField(3, repeated=True)
    requirements = _messages.StringField(4, repeated=True)
    rules = _messages.MessageField('UsageRule', 5, repeated=True)
    serviceAccess = _messages.EnumField('ServiceAccessValueValuesEnum', 6)


class UsageRule(_messages.Message):
    """Usage configuration rules for the service.  NOTE: Under development.
    Use this rule to configure unregistered calls for the service. Unregistered
    calls are calls that do not contain consumer project identity. (Example:
    calls that do not contain an API key). By default, API methods do not allow
    unregistered calls, and each method call must be identified by a consumer
    project identity. Use this rule to allow/disallow unregistered calls.
    Example of an API that wants to allow unregistered calls for entire service.
    usage:       rules:       - selector: "*"         allow_unregistered_calls:
    true  Example of a method that wants to allow unregistered calls.
    usage:       rules:       - selector:
    "google.example.library.v1.LibraryService.CreateBook"
    allow_unregistered_calls: true

    Fields:
      allowUnregisteredCalls: True, if the method allows unregistered calls;
        false otherwise.
      selector: Selects the methods to which this rule applies. Use '*' to
        indicate all methods in all APIs.  Refer to selector for syntax details.
    """

    allowUnregisteredCalls = _messages.BooleanField(1)
    selector = _messages.StringField(2)


class Visibility(_messages.Message):
    """`Visibility` defines restrictions for the visibility of service elements.
    Restrictions are specified using visibility labels (e.g., TRUSTED_TESTER)
    that are elsewhere linked to users and projects.  Users and projects can
    have access to more than one visibility label. The effective visibility for
    multiple labels is the union of each label's elements, plus any unrestricted
    elements.  If an element and its parents have no restrictions, visibility is
    unconditionally granted.  Example:      visibility:       rules:       -
    selector: google.calendar.Calendar.EnhancedSearch         restriction:
    TRUSTED_TESTER       - selector: google.calendar.Calendar.Delegate
    restriction: GOOGLE_INTERNAL  Here, all methods are publicly visible except
    for the restricted methods EnhancedSearch and Delegate.

    Fields:
      enforceRuntimeVisibility: Controls whether visibility rules are enforced
        at runtime for requests to all APIs and methods.  If true, requests
        without method visibility will receive a NOT_FOUND error, and any non-
        visible fields will be scrubbed from the response messages. In service
        config version 0, the default is false. In later config versions, it's
        true.  Note, the `enforce_runtime_visibility` specified in a visibility
        rule overrides this setting for the APIs or methods asscoiated with the
        rule.
      rules: A list of visibility rules providing visibility configuration for
        individual API elements.
    """

    enforceRuntimeVisibility = _messages.BooleanField(1)
    rules = _messages.MessageField('VisibilityRule', 2, repeated=True)


class VisibilityRule(_messages.Message):
    """A visibility rule provides visibility configuration for an individual API
    element.

    Fields:
      enforceRuntimeVisibility: Controls whether visibility is enforced at
        runtime for requests to an API method. This setting has meaning only
        when the selector applies to a method or an API.  If true, requests
        without method visibility will receive a NOT_FOUND error, and any non-
        visible fields will be scrubbed from the response messages. The default
        is determined by the value of
        google.api.Visibility.enforce_runtime_visibility.
      restriction: Lists the visibility labels for this rule. Any of the listed
        labels grants visibility to the element.  If a rule has multiple labels,
        removing one of the labels but not all of them can break clients.
        Example:      visibility:       rules:       - selector:
        google.calendar.Calendar.EnhancedSearch         restriction:
        GOOGLE_INTERNAL, TRUSTED_TESTER  Removing GOOGLE_INTERNAL from this
        restriction will break clients that rely on this method and only had
        access to it through GOOGLE_INTERNAL.
      selector: Selects methods, messages, fields, enums, etc. to which this
        rule applies.  Refer to selector for syntax details.
    """

    enforceRuntimeVisibility = _messages.BooleanField(1)
    restriction = _messages.StringField(2)
    selector = _messages.StringField(3)


encoding.AddCustomJsonFieldMapping(
    StandardQueryParameters, 'f__xgafv', '$.xgafv',
    package=u'servicecontrol')
encoding.AddCustomJsonEnumMapping(
    StandardQueryParameters.FXgafvValueValuesEnum, '_1', '1',
    package=u'servicecontrol')
encoding.AddCustomJsonEnumMapping(
    StandardQueryParameters.FXgafvValueValuesEnum, '_2', '2',
    package=u'servicecontrol')
