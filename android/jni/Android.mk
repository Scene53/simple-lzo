
LOCAL_PATH := $(call my-dir)/../../lzo


include $(CLEAR_VARS)
LOCAL_MODULE    := liblzo
LOCAL_SRC_FILES := \
	lzo.c

    
LOCAL_CFLAGS := -D__ANDROID__  
LOCAL_STATIC_LIBRARIES := android_native_app_glue
include $(BUILD_STATIC_LIBRARY)


