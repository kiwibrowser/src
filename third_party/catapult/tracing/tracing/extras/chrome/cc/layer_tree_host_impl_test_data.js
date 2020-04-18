// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

// A single LTHI sort of manually created from a Google search for cats.
// This cannot be const because this file may be loaded more than once.
global.g_catLTHIEvents = [
  {
    'name': 'cc::Picture',
    'args': {
      'snapshot': {
        'params': {
          'opaque_rect': [
            -15,
            -15,
            0,
            0
          ],
          'layer_rect': [
            -15,
            -15,
            1260,
            1697
          ]
        },
        'skp64': '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' // @suppress longLineCheck
      }
    },
    'pid': 1,
    'ts': 100,
    'cat': 'disabled-by-default-cc.debug',
    'tid': 1,
    'ph': 'O',
    'id': 'PICTURE_1'
  },
  {
    'name': 'AnalyzeTask',
    'args': {
      'data': {
        'source_frame_number': 107,
        'tile_id': {
          'id_ref': 'TILE_1'
        },
        'resolution': 'HIGH_RESOLUTION',
        'is_tile_in_pending_tree_now_bin': true
      }
    },
    'pid': 1,
    'ts': 101,
    'cat': 'cc',
    'tid': 1,
    'ph': 'B'
  },
  {
    'name': 'AnalyzeTask',
    'args': {},
    'pid': 1,
    'ts': 105,
    'cat': 'cc',
    'tid': 1,
    'ph': 'E'
  },
  {
    'name': 'RasterTask',
    'args': {
      'data': {
        'source_frame_number': 107,
        'tile_id': {
          'id_ref': 'TILE_1'
        },
        'resolution': 'HIGH_RESOLUTION',
        'is_tile_in_pending_tree_now_bin': true
      }
    },
    'pid': 1,
    'ts': 110,
    'cat': 'cc',
    'tid': 1,
    'ph': 'B'
  },
  {
    'name': 'RasterTask',
    'args': {},
    'pid': 1,
    'ts': 150,
    'cat': 'cc',
    'tid': 1,
    'ph': 'E'
  },
  {
    'name': 'RasterTask',
    'args': {
      'data': {
        'source_frame_number': 107,
        'tile_id': {
          'id_ref': 'TILE_2'
        },
        'resolution': 'HIGH_RESOLUTION',
        'is_tile_in_pending_tree_now_bin': true
      }
    },
    'pid': 1,
    'ts': 170,
    'cat': 'cc',
    'tid': 1,
    'ph': 'B'
  },
  {
    'name': 'RasterTask',
    'args': {},
    'pid': 1,
    'ts': 180,
    'cat': 'cc',
    'tid': 1,
    'ph': 'E'
  },
  {
    'name': 'cc::LayerTreeHostImpl',
    'args': {
      'snapshot': {
        'device_viewport_size': {
          'width': 2460,
          'height': 1606
        },
        'active_tree': {
          'source_frame_number': 7,
          'root_layer': {
            'tilings': [
              {
                'content_scale': 2,
                'content_bounds': {
                  'width': 2460,
                  'height': 3334
                },
                'num_tiles': 1
              },
              {
                'content_scale': 0.25,
                'content_bounds': {
                  'width': 308,
                  'height': 417
                },
                'num_tiles': 1
              }
            ],
            'coverage_tiles': [
              {
                'geometry_rect': [0, 0, 256, 256],
                'tile': {
                  'id_ref': 'TILE_1'
                }
              },
              {
                'geometry_rect': [256, 0, 256, 256]
              },
              {
                'geometry_rect': [512, 0, 256, 256]
              },
              {
                'geometry_rect': [0, 256, 256, 512]
              },
              {
                'geometry_rect': [256, 256, 512, 512]
              }
            ],
            'gpu_memory_usage': 22069248,
            'draws_content': 1,
            'layer_id': 6,
            'invalidation': [10, 20, 30, 40],
            'bounds': {
              'width': 1230,
              'height': 1667
            },
            'children': [
              {
                'tilings': [
                  {
                    'content_scale': 2,
                    'content_bounds': {
                      'width': 200,
                      'height': 100
                    },
                    'num_tiles': 1
                  }
                ],
                'gpu_memory_usage': 128000,
                'draws_content': 1,
                'layer_id': 7,
                'invalidation': [],
                'bounds': {
                  'width': 100,
                  'height': 50
                },
                'children': [
                ],
                'ideal_contents_scale': 2,
                'layer_quad': [
                  0,
                  0,
                  200,
                  0,
                  200,
                  100,
                  0,
                  100
                ],
                'pictures': [
                ],
                'debug_info': {
                  'annotated_invalidation_rects': [
                    {
                      'geometry_rect': [11, 22, 33, 44],
                      'reason': 'appeared',
                      'client': 'client1'
                    },
                    {
                      'geometry_rect': [22, 33, 44, 55],
                      'reason': 'disappeared',
                      'client': 'client2'
                    },
                  ]
                },
                'id': 'cc::PictureLayerImpl/LAYER_2'
              }
            ],
            'ideal_contents_scale': 2,
            'layer_quad': [
              0,
              -1022,
              2460,
              -1022,
              2460,
              2312,
              0,
              2312
            ],
            'pictures': [
              {
                'id_ref': 'PICTURE_1'
              }
            ],
            'id': 'cc::PictureLayerImpl/LAYER_1'
          },
          'render_surface_layer_list': [
            {'id_ref': 'LAYER_1'},
            {'id_ref': 'LAYER_2'}
          ],
          'id': 'cc::LayerTreeImpl/0x7d246ee0'
        },
        'tiles': [
          {
            'active_priority': {
              'time_to_visible_in_seconds': 0,
              'resolution': 'HIGH_RESOLUTION',
              'distance_to_visible_in_pixels': 0
            },
            'pending_priority': {
              'time_to_visible_in_seconds': 3.4028234663852886e+38,
              'resolution': 'NON_IDEAL_RESOLUTION',
              'distance_to_visible_in_pixels': 3.4028234663852886e+38
            },
            'managed_state': {
              'resolution': 'HIGH_RESOLUTION',
              'is_solid_color': false,
              'is_using_gpu_memory': true,
              'has_resource': true,
              'scheduled_priority': 10,
              'distance_to_visible': 0,
              'gpu_memory_usage': 1024000
            },
            'layer_id': '6',
            'picture_pile': {
              'id_ref': 'PICTURE_1'
            },
            'contents_scale': 2,
            'content_rect': [0, 0, 1024, 1024],
            'id': 'cc::Tile/TILE_1'
          },
          {
            'active_priority': {
              'time_to_visible_in_seconds': 0,
              'resolution': 'HIGH_RESOLUTION',
              'distance_to_visible_in_pixels': 0
            },
            'pending_priority': {
              'time_to_visible_in_seconds': 3.4028234663852886e+38,
              'resolution': 'NON_IDEAL_RESOLUTION',
              'distance_to_visible_in_pixels': 3.4028234663852886e+38
            },
            'managed_state': {
              'resolution': 'HIGH_RESOLUTION',
              'is_solid_color': false,
              'is_using_gpu_memory': true,
              'has_resource': true,
              'scheduled_priority': 12,
              'distance_to_visible': 0,
              'gpu_memory_usage': 1024000
            },
            'layer_id': '6',
            'picture_pile': {
              'id_ref': 'PICTURE_1'
            },
            'contents_scale': 2,
            'content_rect': [0, 1024, 1024, 1024],
            'id': 'cc::Tile/TILE_2'
          }
        ]
      }
    },
    'pid': 1,
    'ts': 500,
    'cat': 'disabled-by-default-cc.debug',
    'tid': 28163,
    'ph': 'O',
    'id': 'LTHI_1'
  },
  {
    'name': 'cc::DisplayItemList',
    'args': {
      'snapshot': {
        'params': {
          'layer_rect': [
            -15,
            -15,
            1260,
            1697
          ],
          'items': [
            'BeginClipDisplayItem',
            'EndClipDisplayItem'
          ]
        },
        'skp64': '[base 64 encoded skia picture]'
      }
    },
    'pid': 1,
    'ts': 300,
    'cat': 'disabled-by-default-cc.debug',
    'tid': 1,
    'ph': 'O',
    'id': 'PICTURE_3'
  }
];
