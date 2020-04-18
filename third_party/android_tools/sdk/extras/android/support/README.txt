Support Libraries for Android.

This SDK component contains static libraries providing access to newer APIs
on older platforms and various helper classes.

To use those libraries, simply copy them as static libraries into your project.

Each library is called v<api>, indicating the minimum API level that they require.


*** V4 ***

v4/android-support-v4.jar contains:
- Fragment API. New in API 11 (3.0 - Honeycomb). http://developer.android.com/reference/android/app/Fragment.html
- Loader API. New in API 11 (3.0 - Honeycomb). http://developer.android.com/reference/android/app/LoaderManager.html
- CursorAdapter / ResourceCursorAdapter / SimpleCursorAdapter. These are the API 11 versions.
- MenuCompat allows calling MenuItem.setShowAsAction which only exists on API 11.

v4/src/ is the source code for the compatibility library
v4/samples/ provides a sample app using the library.


*** V13 ***

v13/android-support-v13.jar provides the same features as v4, plus:
- FragmentPagerAdapter: Implementation of PagerAdapter that represents each page as a Fragment.

v13/src/ is the source code for the compatibility library, not including the v4 source
v13/samples/ provides a sample app using the library.
