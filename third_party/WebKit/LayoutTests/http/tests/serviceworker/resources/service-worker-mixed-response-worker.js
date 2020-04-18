importScripts('/resources/get-host-info.js');

var HOST_INFO = get_host_info();
var PARTIAL_RESOURCE_PATH =
    '/serviceworker/resources/service-worker-mixed-response.php';

function get_query_params(url) {
  var search = (new URL(url)).search;
  if (!search) {
    return {};
  }
  var ret = {};
  var params = search.substring(1).split('&');
  params.forEach(function(param) {
      var element = param.split('=');
      ret[decodeURIComponent(element[0])] = decodeURIComponent(element[1]);
    });
  return ret;
}

function generate_partial_byte_response(position) {
  return new Response(
      'Ogg'.substr(position, 1),
      {
        status: 206,
        headers: {
          'content-type': 'audio/ogg',
          // 12983 is the file size of media/content/silence.oga.
          'content-range': 'bytes ' + position + '-' + position + '/12983'
        }
      });
}

function fetch_same_origin_partial_resource(position) {
  return fetch(HOST_INFO['HTTP_ORIGIN'] + PARTIAL_RESOURCE_PATH +
               '?position=' + position)
}

function fetch_cross_origin_partial_resource(position) {
  return fetch(HOST_INFO['HTTP_REMOTE_ORIGIN'] + PARTIAL_RESOURCE_PATH +
               '?position=' + position)
}

self.addEventListener('fetch', function(event) {
    if (event.request.url.indexOf('blank.html') != -1) {
      // The request is for the page load.
      return;
    }
    var params = get_query_params(event.request.url);
    if (event.request.headers.get('range') == 'bytes=0-') {
      if (params['SW_FIRST'] == 'gen') {
        event.respondWith(generate_partial_byte_response(0));
      } else if (params['SW_FIRST'] == 'same') {
        event.respondWith(fetch_same_origin_partial_resource(0));
      } else if (params['SW_FIRST'] == 'cross') {
        event.respondWith(fetch_cross_origin_partial_resource(0));
      }
    } else if (event.request.headers.get('range') == 'bytes=1-') {
      if (params['SW_SECOND'] == 'gen') {
        event.respondWith(generate_partial_byte_response(1));
      } else if (params['SW_SECOND'] == 'same') {
        event.respondWith(fetch_same_origin_partial_resource(1));
      } else if (params['SW_SECOND'] == 'cross') {
        event.respondWith(fetch_cross_origin_partial_resource(1));
      }
    }
  });
