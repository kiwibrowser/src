#   Copyright (c) 2006-2007 Open Source Applications Foundation
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import urlparse, httplib, copy, base64, StringIO
import urllib

try:
    from xml.etree import ElementTree
except:
    from elementtree import ElementTree

__all__ = ['DAVClient']

def object_to_etree(parent, obj, namespace=''):
    """This function takes in a python object, traverses it, and adds it to an existing etree object"""
    
    if type(obj) is int or type(obj) is float or type(obj) is str:
        # If object is a string, int, or float just add it
        obj = str(obj)
        if obj.startswith('{') is False:
            ElementTree.SubElement(parent, '{%s}%s' % (namespace, obj))
        else:
            ElementTree.SubElement(parent, obj)
        
    elif type(obj) is dict:
        # If the object is a dictionary we'll need to parse it and send it back recusively
        for key, value in obj.items():
            if key.startswith('{') is False:
                key_etree = ElementTree.SubElement(parent, '{%s}%s' % (namespace, key))
                object_to_etree(key_etree, value, namespace=namespace)
            else:
                key_etree = ElementTree.SubElement(parent, key)
                object_to_etree(key_etree, value, namespace=namespace)
            
    elif type(obj) is list:
        # If the object is a list parse it and send it back recursively
        for item in obj:
            object_to_etree(parent, item, namespace=namespace)
            
    else:
        # If it's none of previous types then raise
        raise TypeError, '%s is an unsupported type' % type(obj)
        

class DAVClient(object):
    
    def __init__(self, url='http://localhost:8080'):
        """Initialization"""
        
        self._url = urlparse.urlparse(url)
        
        self.headers = {'Host':self._url[1], 
                        'User-Agent': 'python.davclient.DAVClient/0.1'} 
        
        
    def _request(self, method, path='', body=None, headers=None):
        """Internal request method"""
        self.response = None
        
        if headers is None:
            headers = copy.copy(self.headers)
        else:
            new_headers = copy.copy(self.headers)
            new_headers.update(headers)
            headers = new_headers
        
        if self._url.scheme == 'http':
            self._connection = httplib.HTTPConnection(self._url[1])
        elif self._url.scheme == 'https':
            self._connection = httplib.HTTPSConnection(self._url[1])
        else:
            raise Exception, 'Unsupported scheme'
        
        self._connection.request(method, path, body, headers)
            
        self.response = self._connection.getresponse()
        
        self.response.body = self.response.read()
        
        # Try to parse and get an etree
        try:
            self._get_response_tree()
        except:
            pass
        
            
    def _get_response_tree(self):
        """Parse the response body into an elementree object"""
        self.response.tree = ElementTree.fromstring(self.response.body)
        return self.response.tree
        
    def set_basic_auth(self, username, password):
        """Set basic authentication"""
        auth = 'Basic %s' % base64.encodestring('%s:%s' % (username, password)).strip()
        self._username = username
        self._password = password
        self.headers['Authorization'] = auth
        
    ## HTTP DAV methods ##
        
    def get(self, path, headers=None):
        """Simple get request"""
        self._request('GET', path, headers=headers)
        return self.response.body
        
    def head(self, path, headers=None):
        """Basic HEAD request"""
        self._request('HEAD', path, headers=headers)
        
    def put(self, path, body=None, f=None, headers=None):
        """Put resource with body"""
        if f is not None:
            body = f.read()
            
        self._request('PUT', path, body=body, headers=headers)
        
    def post(self, path, body=None, headers=None):
        """POST resource with body"""

        self._request('POST', path, body=body, headers=headers)
        
    def mkcol(self, path, headers=None):
        """Make DAV collection"""
        self._request('MKCOL', path=path, headers=headers)
        
    make_collection = mkcol
        
    def delete(self, path, headers=None):
        """Delete DAV resource"""
        self._request('DELETE', path=path, headers=headers)
        
    def copy(self, source, destination, body=None, depth='infinity', overwrite=True, headers=None):
        """Copy DAV resource"""
        # Set all proper headers
        if headers is None:
            headers = {'Destination':destination}
        else:
            headers['Destination'] = self._url.geturl() + destination
        if overwrite is False:
            headers['Overwrite'] = 'F'
        headers['Depth'] = depth
            
        self._request('COPY', source, body=body, headers=headers)
        
        
    def copy_collection(self, source, destination, depth='infinity', overwrite=True, headers=None):
        """Copy DAV collection"""
        body = '<?xml version="1.0" encoding="utf-8" ?><d:propertybehavior xmlns:d="DAV:"><d:keepalive>*</d:keepalive></d:propertybehavior>'
        
        # Add proper headers
        if headers is None:
            headers = {}
        headers['Content-Type'] = 'text/xml; charset="utf-8"'
        
        self.copy(source, destination, body=unicode(body, 'utf-8'), depth=depth, overwrite=overwrite, headers=headers)
        
        
    def move(self, source, destination, body=None, depth='infinity', overwrite=True, headers=None):
        """Move DAV resource"""
        # Set all proper headers
        if headers is None:
            headers = {'Destination':destination}
        else:
            headers['Destination'] = self._url.geturl() + destination
        if overwrite is False:
            headers['Overwrite'] = 'F'
        headers['Depth'] = depth
            
        self._request('MOVE', source, body=body, headers=headers)
        
        
    def move_collection(self, source, destination, depth='infinity', overwrite=True, headers=None):
        """Move DAV collection and copy all properties"""
        body = '<?xml version="1.0" encoding="utf-8" ?><d:propertybehavior xmlns:d="DAV:"><d:keepalive>*</d:keepalive></d:propertybehavior>'
        
        # Add proper headers
        if headers is None:
            headers = {}
        headers['Content-Type'] = 'text/xml; charset="utf-8"'

        self.move(source, destination, unicode(body, 'utf-8'), depth=depth, overwrite=overwrite, headers=headers)
        
        
    def propfind(self, path, properties='allprop', namespace='DAV:', depth=None, headers=None):
        """Property find. If properties arg is unspecified it defaults to 'allprop'"""
        # Build propfind xml
        root = ElementTree.Element('{DAV:}propfind')
        if type(properties) is str:
            ElementTree.SubElement(root, '{DAV:}%s' % properties)
        else:
            props = ElementTree.SubElement(root, '{DAV:}prop')
            object_to_etree(props, properties, namespace=namespace)
        tree = ElementTree.ElementTree(root)
        
        # Etree won't just return a normal string, so we have to do this
        body = StringIO.StringIO()
        tree.write(body)
        body = body.getvalue()
                
        # Add proper headers
        if headers is None:
            headers = {}
        if depth is not None:
            headers['Depth'] = depth
        headers['Content-Type'] = 'text/xml; charset="utf-8"'
        
        # Body encoding must be utf-8, 207 is proper response
        self._request('PROPFIND', path, body=unicode('<?xml version="1.0" encoding="utf-8" ?>\n'+body, 'utf-8'), headers=headers)
        
        if self.response is not None and hasattr(self.response, 'tree') is True:
            property_responses = {}
            for response in self.response.tree._children:
                property_href = response.find('{DAV:}href')
                property_stat = response.find('{DAV:}propstat')
                
                def parse_props(props):
                    property_dict = {}
                    for prop in props:
                        if prop.tag.find('{DAV:}') is not -1:
                            name = prop.tag.split('}')[-1]
                        else:
                            name = prop.tag
                        if len(prop._children) is not 0:
                            property_dict[name] = parse_props(prop._children)
                        else:
                            property_dict[name] = prop.text
                    return property_dict
                
                if property_href is not None and property_stat is not None:
                    property_dict = parse_props(property_stat.find('{DAV:}prop')._children)
                    property_responses[property_href.text] = property_dict
            return property_responses
        
    def proppatch(self, path, set_props=None, remove_props=None, namespace='DAV:', headers=None):
        """Patch properties on a DAV resource. If namespace is not specified the DAV namespace is used for all properties"""
        root = ElementTree.Element('{DAV:}propertyupdate')
        
        if set_props is not None:
            prop_set = ElementTree.SubElement(root, '{DAV:}set')
            object_to_etree(prop_set, set_props, namespace=namespace)
        if remove_props is not None:
            prop_remove = ElementTree.SubElement(root, '{DAV:}remove')
            object_to_etree(prop_remove, remove_props, namespace=namespace)
        
        tree = ElementTree.ElementTree(root)
        
        # Add proper headers
        if headers is None:
            headers = {}
        headers['Content-Type'] = 'text/xml; charset="utf-8"'
        
        self._request('PROPPATCH', path, body=unicode('<?xml version="1.0" encoding="utf-8" ?>\n'+body, 'utf-8'), headers=headers)
        
        
    def set_lock(self, path, owner, locktype='exclusive', lockscope='write', depth=None, headers=None):
        """Set a lock on a dav resource"""
        root = ElementTree.Element('{DAV:}lockinfo')
        object_to_etree(root, {'locktype':locktype, 'lockscope':lockscope, 'owner':{'href':owner}}, namespace='DAV:')
        tree = ElementTree.ElementTree(root)
        
        # Add proper headers
        if headers is None:
            headers = {}
        if depth is not None:
            headers['Depth'] = depth
        headers['Content-Type'] = 'text/xml; charset="utf-8"'
        headers['Timeout'] = 'Infinite, Second-4100000000'
        
        self._request('LOCK', path, body=unicode('<?xml version="1.0" encoding="utf-8" ?>\n'+body, 'utf-8'), headers=headers)
        
        locks = self.response.etree.finall('.//{DAV:}locktoken')
        lock_list = []
        for lock in locks:
            lock_list.append(lock.getchildren()[0].text.strip().strip('\n'))
        return lock_list
        

    def refresh_lock(self, path, token, headers=None):
        """Refresh lock with token"""
        
        if headers is None:
            headers = {}
        headers['If'] = '(<%s>)' % token
        headers['Timeout'] = 'Infinite, Second-4100000000'
        
        self._request('LOCK', path, body=None, headers=headers)
        
        
    def unlock(self, path, token, headers=None):
        """Unlock DAV resource with token"""
        if headers is None:
            headers = {}
        headers['Lock-Tocken'] = '<%s>' % token
        
        self._request('UNLOCK', path, body=None, headers=headers)
        





