$(call add-clean-step, rm -rf $(OUT_DIR)/target/common/obj/APPS/GLMark2*)
$(call add-clean-step, rm -rf $(PRODUCT_OUT)/system/app/GLMark2.apk)

$(call add-clean-step, rm -rf $(PRODUCT_OUT)/obj/SHARED_LIBRARIES/libglmark2-android_intermediates)
$(call add-clean-step, rm -rf $(PRODUCT_OUT)/obj/STATIC_LIBRARIES/libglmark2-matrix_intermediates)
$(call add-clean-step, rm -rf $(PRODUCT_OUT)/obj/STATIC_LIBRARIES/libglmark2-png_intermediates)
