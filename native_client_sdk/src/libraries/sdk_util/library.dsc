{
  'TARGETS': [
    {
      'NAME' : 'sdk_util',
      'TYPE' : 'lib',
      'SOURCES' : [
        'thread_pool.cc'
      ]
    }
  ],
  'HEADERS': [
    {
      'FILES': [
        'atomicops.h',
        'auto_lock.h',
        'macros.h',
        'ref_object.h',
        'scoped_ref.h',
        'simple_lock.h',
        'string_util.h',
        'thread_pool.h',
        'thread_safe_queue.h'
      ],
      'DEST': 'include/sdk_util',
    }
  ],
  'DEST': 'src',
  'NAME': 'sdk_util',
}
