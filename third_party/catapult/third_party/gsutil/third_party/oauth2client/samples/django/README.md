# Django Samples

These two sample Django apps provide a skeleton for the two main use cases of the
`oauth2client.contrib.django_util` helpers.

Please see the 
[core docs](https://oauth2client.readthedocs.io/en/latest/) for more information and usage examples.

## google_user

This is the simpler use case of the library. It assumes you are using Google OAuth as your primary
authorization and authentication mechanism for your application. Users log in with their Google ID 
and their OAuth2 credentials are stored inside the session. 
 
## django_user
 
This is the use case where the application is already using the Django authorization system and
has a Django model with a `django.contrib.auth.models.User` field, and would like to attach
a Google OAuth2 credentials object to that model. Users have to login, and then can login with 
their Google account to associate the Google account with the user in the Django system.
Credentials will be stored in the Django ORM backend.
