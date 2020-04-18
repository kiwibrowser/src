// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// The include directives are put into Javascript-style comments to prevent
// parsing errors in non-flattened mode. The flattener still sees them.
// Note that this makes the flattener to comment out the first line of the
// included file but that's all right since any javascript file should start
// with a copyright comment anyway.

// <include src="error_util.js">

// <include src="../../file_manager/common/js/metrics_base.js">
// <include src="video_player_metrics.js">

// <include src="../../file_manager/common/js/lru_cache.js">
// <include src="../../image_loader/image_loader_client.js">

// <include src="../../../webui/resources/js/cr.js">
// <include src="../../../webui/resources/js/util.js">
// <include src="../../../webui/resources/js/load_time_data.js">

// <include src="../../../webui/resources/js/event_tracker.js">

// <include src="../../../webui/resources/js/cr/ui.js">
// <include src="../../../webui/resources/js/cr/event_target.js">

// <include src="../../../webui/resources/js/cr/ui/array_data_model.js">
// <include src="../../../webui/resources/js/cr/ui/position_util.js">
// <include src="../../../webui/resources/js/cr/ui/menu_item.js">
// <include src="../../../webui/resources/js/cr/ui/menu.js">
// <include src="../../../webui/resources/js/cr/ui/menu_button.js">
// <include src="../../../webui/resources/js/cr/ui/context_menu_handler.js">

(function() {
'use strict';

// <include src="../../../webui/resources/js/load_time_data.js">
// <include src="../../../webui/resources/js/i18n_template_no_process.js">

// <include src="../../file_manager/common/js/async_util.js">
// <include src="../../file_manager/common/js/file_type.js">
// <include src="../../file_manager/common/js/util.js">
// <include src="../../file_manager/common/js/volume_manager_common.js">
// <include src="../../file_manager/foreground/js/mouse_inactivity_watcher.js">
// <include src="../../file_manager/foreground/js/volume_manager_wrapper.js">

// <include src="cast/cast_extension_discoverer.js">
// <include src="cast/cast_video_element.js">
// <include src="cast/media_manager.js">
// <include src="cast/caster.js">

// <include src="media_controls.js">

// <include src="video_player.js">

window.unload = unload;

})();
