<?php
/*
Dromaeo Test Suite
Copyright (c) 2010 John Resig

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

$server = 'mysql.dromaeo.com';
$user = 'dromaeo';
$pass = 'dromaeo';

require('JSON.php');

$json = new Services_JSON();
$sql = mysql_connect( $server, $user, $pass );

mysql_select_db( 'dromaeo' );

$id = preg_replace('/[^\d,]/', '', $_REQUEST['id']);

if ( $id ) {
	$sets = array();
	$ids = split(",", $id);

	foreach ($ids as $i) {
		$query = mysql_query( sprintf("SELECT * FROM runs WHERE id=%s;",
			mysql_real_escape_string($i)));
		$data = mysql_fetch_assoc($query);

		$query = mysql_query( sprintf("SELECT * FROM results WHERE run_id=%s;",
			mysql_real_escape_string($i)));
		$results = array();
	
		while ( $row = mysql_fetch_assoc($query) ) {
			array_push($results, $row);
		}

		$data['results'] = $results;
		$data['ip'] = '';

		array_push($sets, $data);
	}

	echo $json->encode($sets);
} else {
	$data = $json->decode(str_replace('\\"', '"', $_REQUEST['data']));

	if ( $data ) {
		mysql_query( sprintf("INSERT into runs VALUES(NULL,'%s','%s',NOW(),'%s');",
			mysql_real_escape_string($_SERVER['HTTP_USER_AGENT']),
			mysql_real_escape_string($_SERVER['REMOTE_ADDR']),
			mysql_real_escape_string(str_replace(';', "", $_REQUEST['style']))
		));

		$id = mysql_insert_id();

		if ( $id ) {

			foreach ($data as $row) {
				mysql_query( sprintf("INSERT into results VALUES(NULL,'%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s');",
					$id, $row->collection, $row->version, $row->name, $row->scale, $row->median, $row->min, $row->max, $row->mean, $row->deviation, $row->runs) );
			}

			echo $id;
		}
	}
}
?>
