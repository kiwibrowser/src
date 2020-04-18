package com.example.photoviewersample;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.UriMatcher;
import android.content.res.AssetFileDescriptor;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.List;

public class SampleProvider extends ContentProvider {
    private static final int PHOTOS = 1;
    private static final int PHOTO_INDIVIDUAL_1 = 2;
    private static final int PHOTO_INDIVIDUAL_2 = 3;
    private static final int PHOTO_INDIVIDUAL_3 = 4;
    private static final int PHOTO_INDIVIDUAL_4 = 5;

    private static final String PROVIDER_URI = "com.example.photoviewersample.SampleProvider";

 // Creates a UriMatcher object.
    private static final UriMatcher sUriMatcher = new UriMatcher(UriMatcher.NO_MATCH);

    static
    {
        sUriMatcher.addURI(PROVIDER_URI, "photos", PHOTOS);
        sUriMatcher.addURI(PROVIDER_URI, "photos/1", PHOTO_INDIVIDUAL_1);
        sUriMatcher.addURI(PROVIDER_URI, "photos/2", PHOTO_INDIVIDUAL_2);
        sUriMatcher.addURI(PROVIDER_URI, "photos/3", PHOTO_INDIVIDUAL_3);
        sUriMatcher.addURI(PROVIDER_URI, "photos/4", PHOTO_INDIVIDUAL_4);
    }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        // TODO Auto-generated method stub
        return 0;
    }

    @Override
    public String getType(Uri uri) {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public boolean onCreate() {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs,
            String sortOrder) {
        MatrixCursor matrix = new MatrixCursor(projection);

        /*
         * Choose the table to query and a sort order based on the code returned for the incoming
         * URI. Here, too, only the statements for table 3 are shown.
         */
        switch (sUriMatcher.match(uri)) {
            // If the incoming URI was for all of the photos table
            case PHOTOS:
                addRow(matrix, PHOTO_INDIVIDUAL_1);
                addRow(matrix, PHOTO_INDIVIDUAL_2);
                addRow(matrix, PHOTO_INDIVIDUAL_3);
                addRow(matrix, PHOTO_INDIVIDUAL_4);
                break;

            // If the incoming URI was for a single row
            case PHOTO_INDIVIDUAL_1:
                addRow(matrix, PHOTO_INDIVIDUAL_1);
                break;
            case PHOTO_INDIVIDUAL_2:
                addRow(matrix, PHOTO_INDIVIDUAL_2);
                break;
            case PHOTO_INDIVIDUAL_3:
                addRow(matrix, PHOTO_INDIVIDUAL_3);
                break;
            case PHOTO_INDIVIDUAL_4:
                addRow(matrix, PHOTO_INDIVIDUAL_4);
                break;

            default:
                // If the URI is not recognized, you should do some error handling here.
        }
        // call the code to actually do the query

        return matrix;
    }

    /**
     * Adds a single row to the Cursor. A real implementation should
     * check the projection to properly match the columns.
     */
    private void addRow(MatrixCursor matrix, int match_id) {
        switch (match_id) {
            case PHOTO_INDIVIDUAL_1:
                matrix.newRow()
                .add("content://" + PROVIDER_URI + "/photos/1")                 // uri
                .add("blah.png")                                                // displayName
                .add("content://" + PROVIDER_URI + "/photos/1/contentUri")      // contentUri
                .add("content://" + PROVIDER_URI + "/photos/1/thumbnailUri")    // thumbnailUri
                .add("image/png");                                              // contentType
                break;
            case PHOTO_INDIVIDUAL_2:
                matrix.newRow()
                .add("content://" + PROVIDER_URI + "/photos/2")                 // uri
                .add("johannson.png")                                           // displayName
                .add("content://" + PROVIDER_URI + "/photos/2/contentUri")      // contentUri
                .add("content://" + PROVIDER_URI + "/photos/2/thumbnailUri")    // thumbnailUri
                .add("image/png");                                              // contentType
                break;
            case PHOTO_INDIVIDUAL_3:
                matrix.newRow()
                .add("content://" + PROVIDER_URI + "/photos/3")                 // uri
                .add("planets.png")                                             // displayName
                .add("content://" + PROVIDER_URI + "/photos/3/contentUri")      // contentUri
                .add("content://" + PROVIDER_URI + "/photos/3/thumbnailUri")    // thumbnailUri
                .add("image/png");                                              // contentType
                break;
            case PHOTO_INDIVIDUAL_4:
                matrix.newRow()
                .add("content://" + PROVIDER_URI + "/photos/4")                 // uri
                .add("galaxy.png")                                              // displayName
                .add("content://" + PROVIDER_URI + "/photos/4/contentUri")      // contentUri
                .add("content://" + PROVIDER_URI + "/photos/4/thumbnailUri")    // thumbnailUri
                .add("image/png");                                              // contentType
                break;

            default:
                // If the URI is not recognized, you should do some error handling here.
        }
    }

    @Override
    public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
        // TODO Auto-generated method stub
        return 0;
    }

    @Override
    public AssetFileDescriptor openAssetFile(Uri uri, String mode) throws FileNotFoundException {
        List<String> pathSegments = uri.getPathSegments();
        final int id = Integer.parseInt(pathSegments.get(1));
        String fileName;
        switch (id) {
            case 1:
                fileName = "blah.png";
                break;
            case 2:
                fileName = "johannson.png";
                break;
            case 3:
                fileName = "planets.png";
                break;
            case 4:
                fileName = "galaxy.png";
                break;
            default:
                fileName = null;
                break;
        }
        try {
            return getContext().getAssets().openFd(fileName);
        } catch (IOException e) {
            e.printStackTrace();
            return null;
        }
    }
}
