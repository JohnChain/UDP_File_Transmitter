# Copyright 2006 The Android Open Source Project
#ifeq ($(ASUS_FACTORY_BUILD), y)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SHARED_LIBRARIES := \
	libutils \
	libcutils \

LOCAL_SRC_FILES := conf.c udpclient.c utils.c queue.c 

# LOCAL_CFLAGS := -D_GNU_SOURCE 
LOCAL_MODULE := client
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := debug
include $(BUILD_EXECUTABLE)

#endif
