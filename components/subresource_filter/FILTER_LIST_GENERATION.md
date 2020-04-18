# Generating a Smaller Filter List
Filter lists can be quite large to store in memory, which is problematic on
memory constrained devices. The following steps demonstrate how to generate a
smaller filter list by filtering out the least-frequently-used rules on the top
N websites (according to Alexa rankings).

## 1. Gather the URL requests from the landing pages of the top N sites
This data is made available by the [HttpArchive](https://httparchive.org/)
project and is queryable via [BigQuery](https://bigquery.cloud.google.com/). A
short introduction to querying HttpArchive data is available
[here](https://www.igvita.com/2013/06/20/http-archive-bigquery-web-performance-answers/).
Because the output of our query is typically quite large, it's necessary to
have a Google Compute Engine account with a storage bucket created to write
the resulting table to.

The query to run is:
```sql
SELECT
  pages.url AS origin,
  requests.url AS request_url,
  requests.type AS request_type
FROM
    [httparchive:summary_requests.$ARCHIVE_DATE_AND_TYPE] AS requests
INNER JOIN (
  SELECT
    pageid,
    url
  FROM
    [httparchive:summary_pages.$ARCHIVE_DATE_AND_TYPE]
  WHERE
    rank IS NOT NULL
    AND rank <= $MAX_RANK) AS pages
ON
  requests.pageid = pages.pageid;
```

You'll need to replace `$ARCHIVE_DATE_AND_TYPE` with the table you're
interested, such as `2018.04_15_mobile`. Replace `$MAX_RANK` with the highest
ranked Alexa page you want to process (e.g., `100000`).

In the above query, the URLs from the top 100,000 sites are output. Change
100,000 to whichever value you wish. Since the output is too large to display
on the page,  the results will need to be written to a table in your Google
Cloud Project. To do this, press the 'show options' button below your query, and press the
'select table' button to create a table to write to in your project. You'll
also want to check the 'allow large results' checkbox.

Now run the query. The results should be available in the table you specified
in your project. Find the table on the BigQuery page and export it in JSON
format to a bucket that you create in your Google Cloud Storage. Since files
in buckets are restricted to 1GB, you'll have to shard the file over many
files. Select gzip compression and use `<your_bucket>/site_urls.*.json.gz` as
your destination.

Once exported, you can download the files from your bucket and extract them
into a single file for processing:

```sh
ls site_urls.*.gz | xargs gunzip -c > site_urls
```

## 2. Acquire a filter list in the indexed format
Chromium's tools are designed to work with a binary indexed version of filter
lists. You can use the `subresource_indexing_tool` to convert a text based
filter list to an indexed file.

An example using [EasyList](https://easylist.to/easylist/easylist.txt) follows:

```sh
1. ninja -C out/Release/ subresource_filter_tools
2. wget https://easylist.to/easylist/easylist.txt
3. out/Release/ruleset_converter --input_format=filter-list --output_format=unindexed-ruleset --input_files=easylist.txt --output_file=easylist_unindexed
4. out/Release/subresource_indexing_tool easylist_unindexed easylist_indexed
```

## 3. Generate the smaller filter list
```sh
1. ninja -C out/Release subresource_filter_tools
2. out/Release/subresource_filter_tool --ruleset=easylist_indexed match_rules --input_file=site_urls --min_matches=1 > smaller_list.txt
```

With `min_matches=1`, the rule will be included if it's used at least once while testing each URL from site_urls.

## 4. Turn the smaller list into a form usable by Chromium tools
The smaller filterlist has been generated. If you'd like to convert it to Chromium's binary indexed format, proceed with the following steps:

```sh
1. ninja -C out/Release/ subresource_filter_tools
2. out/Release/ruleset_converter --input_format=filter-list --output_format=unindexed-ruleset --input_files=smaller_list.txt --output_file=smaller_list_unindexed
3. out/Release/subresource_indexing_tool smaller_list_unindexed smaller_list_indexed
```
